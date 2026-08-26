/************************************************************************
 **
 **  @file   mainwindow_background.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Background image methods extracted from mainwindow.cpp
 **
 **  @note   Extracted from mainwindow.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "widgets/ODockManager.h"
#include "../vtools/tools/backgroundimage/vbackgroundimageitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundimagecontrols.h"
#include "../vtools/tools/backgroundimage/vbackgroundpixmapitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundsvgitem.h"
#include "../vtools/undocommands/image/addbackgroundimage.h"
#include "../vtools/undocommands/image/deletebackgroundimage.h"
#include "../ifc/xml/vbackgroundpatternimage.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "dialogs/dialogaddbackgroundimage.h"
#include "dialogs/vwidgetbackgroundimages.h"
#include "core/vapplication.h"

#include "../ifc/xml/utils.h"
#include "../vmisc/def.h"
#include "../vmisc/vcommonsettings.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QProcess>

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::PlaceBackgroundImage(const QPointF &pos, const QString &fileName)
{
    DialogAddBackgroundImage dialog(fileName, this);
    if (dialog.exec() == QDialog::Rejected)
    {
        qCritical() << tr("Unable to add background image");
        return;
    }

    VBackgroundPatternImage image = VBackgroundPatternImage::FromFile(fileName, dialog.BuiltIn());
    image.SetName(dialog.Name());

    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
    image.SetOpacity(settings->GetBackgroundImageDefOpacity() / 100.);

    QTransform m;
    m.translate(pos.x(), pos.y());

    QTransform imageMatrix = image.Matrix();
    imageMatrix *= m;

    image.SetMatrix(m);

    if (not image.IsValid())
    {
        qCritical() << tr("Invalid image. Error: %1").arg(image.ErrorString());
        return;
    }

    auto *addBackgroundImage = new AddBackgroundImage(image, doc);
    connect(addBackgroundImage, &AddBackgroundImage::AddItem, this, &MainWindow::AddBackgroundImageItem);
    connect(addBackgroundImage, &AddBackgroundImage::DeleteItem, this, &MainWindow::DeleteBackgroundImageItem);
    VApplication::VApp()->getUndoStack()->push(addBackgroundImage);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RemoveBackgroundImage(const QUuid &id)
{
    VBackgroundPatternImage const image = doc->GetBackgroundImage(id);

    auto *deleteBackgroundImage = new DeleteBackgroundImage(image, doc);
    connect(deleteBackgroundImage, &DeleteBackgroundImage::AddItem, this, &MainWindow::AddBackgroundImageItem);
    connect(deleteBackgroundImage, &DeleteBackgroundImage::DeleteItem, this, &MainWindow::DeleteBackgroundImageItem);
    VApplication::VApp()->getUndoStack()->push(deleteBackgroundImage);
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::InitBackgroundImageItem(const VBackgroundPatternImage &image) -> VBackgroundImageItem *
{
    VBackgroundImageItem  *item = nullptr;
    if (image.Type() == PatternImage::Raster)
    {
        item = new VBackgroundPixmapItem(image, doc);
    }
    else if (image.Type() == PatternImage::Vector || image.Type() == PatternImage::Unknown)
    {
        item = new VBackgroundSVGItem(image, doc);
    }

    if (item != nullptr)
    {
        connect(item, &VBackgroundImageItem::UpdateControls, m_backgroudcontrols,
                &VBackgroundImageControls::UpdateControls);
        connect(item, &VBackgroundImageItem::ActivateControls, m_backgroudcontrols,
                &VBackgroundImageControls::ActivateControls);
        connect(item, &VBackgroundImageItem::DeleteImage, this, &MainWindow::RemoveBackgroundImage);
        connect(this, &MainWindow::EnableBackgroundImageSelection, item, &VBackgroundImageItem::EnableSelection);
        connect(item, &VBackgroundImageItem::ShowImageInExplorer, this, &MainWindow::ShowBackgroundImageInExplorer);
        connect(item, &VBackgroundImageItem::SaveImage, this, &MainWindow::SaveBackgroundImage);
        connect(m_backgroudcontrols, &VBackgroundImageControls::ActiveImageChanged, m_dockManager->backgroundImagesWidget(),
                &VWidgetBackgroundImages::ImageSelected);
        connect(m_dockManager->backgroundImagesWidget(), &VWidgetBackgroundImages::SelectImage, m_backgroudcontrols,
                &VBackgroundImageControls::ActivateControls);
    }

    return item;
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::NewBackgroundImageItem(const VBackgroundPatternImage &image)
{
    if (m_backgroudcontrols == nullptr)
    {
        m_backgroudcontrols = new VBackgroundImageControls(doc);
        connect(m_sceneDraw, &VMainGraphicsScene::ItemByMouseRelease, m_backgroudcontrols,
                &VBackgroundImageControls::DeactivateControls);
        m_sceneDraw->addItem(m_backgroudcontrols);
    }

    if (m_backgroundImages.contains(image.Id()))
    {
        VBackgroundImageItem *item = m_backgroundImages.value(image.Id());
        if (item != nullptr)
        {
            item->SetImage(image);
            item->setVisible(true);
        }
    }
    else
    {
        VBackgroundImageItem *item = InitBackgroundImageItem(image);
        if (item != nullptr)
        {
            m_backgroundImages.insert(image.Id(), item);
            m_sceneDraw->addItem(item);
        }
    }

    VMainGraphicsView::NewSceneRect(m_sceneDraw, ui->view);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionAddBackgroundImage()
{
    const QString fileName =
        QFileDialog::getOpenFileName(this, tr("Select background image"), QDir::homePath(), PrepareImageFilters(),
                                     nullptr, VAbstractApplication::VApp()->NativeFileDialog());
    if (not fileName.isEmpty())
    {
        QRect const viewportRect(0, 0, ui->view->viewport()->width(), ui->view->viewport()->height());
        PlaceBackgroundImage(ui->view->mapToScene(viewportRect.center()), fileName);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::AddBackgroundImageItem(const QUuid &id)
{
    NewBackgroundImageItem(doc->GetBackgroundImage(id));

    if (m_dockManager->backgroundImagesWidget() != nullptr)
    {
        m_dockManager->backgroundImagesWidget()->UpdateImages();
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::DeleteBackgroundImageItem(const QUuid &id)
{
    if (m_backgroundImages.contains(id))
    {
        VBackgroundImageItem *item = m_backgroundImages.value(id);
        emit ui->view->itemClicked(nullptr); // Hide visualization to avoid a crash
        item->setVisible(false);             // Do not remove the item from scene to prevent crashes.
        if (m_backgroudcontrols != nullptr && m_backgroudcontrols->Id() == id)
        {
            m_backgroudcontrols->ActivateControls(QUuid());
        }

        if (m_dockManager->backgroundImagesWidget() != nullptr)
        {
            m_dockManager->backgroundImagesWidget()->UpdateImages();
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowBackgroundImageInExplorer(const QUuid &id)
{
    ShowInGraphicalShell(doc->GetBackgroundImage(id).FilePath());
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SaveBackgroundImage(const QUuid &id)
{
    VBackgroundPatternImage const image = doc->GetBackgroundImage(id);

    if (not image.IsValid())
    {
        qCritical() << tr("Unable to save image. Error: %1").arg(image.ErrorString());
        return;
    }

    if (image.ContentData().isEmpty())
    {
        qCritical() << tr("Unable to save image. No data.");
        return;
    }

    const QByteArray imageData = QByteArray::fromBase64(image.ContentData());
    QMimeType const mime = MimeTypeFromByteArray(imageData);
    QString path = QDir::homePath() + QDir::separator() + tr("untitled");
    QStringList filters;

    if (mime.isValid())
    {
        if (QStringList const suffixes = mime.suffixes(); not suffixes.isEmpty())
        {
            path += '.'_L1 + suffixes.at(0);
        }

        filters.append(mime.filterString());
    }

    filters.append(tr("All files") + " (*.*)"_L1);

    QString const filter = filters.join(QStringLiteral(";;"));

    QString const filename = QFileDialog::getSaveFileName(this, tr("Save Image"), path, filter, nullptr,
                                                          VAbstractApplication::VApp()->NativeFileDialog());
    if (not filename.isEmpty())
    {
        QFile file(filename);
        if (file.open(QIODevice::WriteOnly))
        {
            file.write(imageData);
        }
        else
        {
            qCritical() << tr("Unable to save image. Error: %1").arg(file.errorString());
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ParseBackgroundImages()
{
    // No memory leak. Scene should take care of these items
    m_backgroudcontrols = nullptr; // force creating new controls
    m_backgroundImages.clear();    // clear dangling pointers

    QVector<VBackgroundPatternImage> const allImages = doc->GetBackgroundImages();
    for (const auto &image : allImages)
    {
        NewBackgroundImageItem(image);
    }
    m_dockManager->backgroundImagesWidget()->UpdateImages();
}

