/************************************************************************
 **
 **  @file   mainwindow_export.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Export methods extracted from mainwindow.cpp
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
#include "mainwindow_helpers.h"
#include "vtheme.h"
#include "../vlayout/vlayoutexporter.h"
#include "../vlayout/dialogs/dialoglayoutscale.h"
#include "../vformat/vpatternrecipe.h"
#include "../ifc/exception/vexceptioninvalidhistory.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "core/vapplication.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QProcess>

using namespace Qt::Literals::StringLiterals;


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportDraw(const QString &fileName)
{
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wnoexcept")

    VLayoutExporter exporter;

    QT_WARNING_POP

    exporter.SetFileName(fileName);

    int const verticalScrollBarValue = ui->view->verticalScrollBar()->value();
    int const horizontalScrollBarValue = ui->view->horizontalScrollBar()->value();

    QTransform const viewTransform = ui->view->transform();
    ui->view->ZoomFitBest(); // Resize all labels
    ui->view->repaint();
    ui->view->ZoomOriginal(); // Set to original scale

    // Enable all items on scene
    const QList<QGraphicsItem *> qItems = m_sceneDraw->items();
    for (auto *item : qItems)
    {
        item->setEnabled(true);
        if (item->type() == VGraphicsSimpleTextItem::Type)
        {
            auto *text = dynamic_cast<VGraphicsSimpleTextItem *>(item);
            text->RefreshColor(); // Regular update doesn't work on labels
        }
    }

    ui->view->repaint();

    m_sceneDraw->SetOriginsVisible(false);

    const QRectF rect = m_sceneDraw->VisibleItemsBoundingRect();
    m_sceneDraw->update(rect);
    exporter.SetImageRect(rect);
    exporter.SetOffset(rect.topLeft()); // Correct positions to fit SVG view rect

    exporter.ExportToSVG(m_sceneDraw, QList<QGraphicsItem *>());

    m_sceneDraw->SetOriginsVisible(true);

    // Restore scale, scrollbars and current active pattern piece
    ui->view->setTransform(viewTransform);
    VMainGraphicsView::NewSceneRect(ui->view->scene(), ui->view);
    ScaleChanged(ui->view->transform().m11());

    ui->view->verticalScrollBar()->setValue(verticalScrollBarValue);
    ui->view->horizontalScrollBar()->setValue(horizontalScrollBarValue);

    m_sceneDraw->EnableTools();
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportDrawAs(bool checked)
{
    Q_UNUSED(checked)

    auto Uncheck = qScopeGuard([this] { ui->actionExportDraw->setChecked(false); });

    QString const filters(tr("Scalable Vector Graphics files") + QStringLiteral("(*.svg)"));
    QString const dir = QDir::homePath() + '/'_L1 + FileName() + QStringLiteral(".svg");
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save draw"), dir, filters, nullptr,
                                                    VAbstractApplication::VApp()->NativeFileDialog());

    if (fileName.isEmpty())
    {
        return;
    }

    if (QFileInfo const f(fileName); f.suffix().isEmpty() || f.suffix() != "svg"_L1)
    {
        fileName += ".svg"_L1;
    }

    ExportDraw(fileName);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportLayoutAs(bool checked)
{
    Q_UNUSED(checked)

    auto Uncheck = qScopeGuard([this] { ui->actionLayoutExportAs->setChecked(false); });

    if (m_layoutSettings->IsLayoutStale() && VPrintLayout::ContinueIfLayoutStale(this) == QMessageBox::No)
    {
        return;
    }

    try
    {
        m_dialogSaveLayout = QSharedPointer<DialogSaveLayout>(new DialogSaveLayout(
            static_cast<int>(m_layoutSettings->LayoutScenes().size()), Draw::Layout, FileName(), this));
        m_dialogSaveLayout->SetBoundaryTogetherWithNotches(m_layoutSettings->IsBoundaryTogetherWithNotches());

        if (m_dialogSaveLayout->exec() == QDialog::Rejected)
        {
            m_dialogSaveLayout.clear();
            return;
        }

        ExportData(QVector<VLayoutPiece>());
        m_dialogSaveLayout.clear();
    }
    catch (const VException &e)
    {
        m_dialogSaveLayout.clear();
        qCritical("%s\n\n%s\n\n%s", qUtf8Printable(tr("Export error.")), qUtf8Printable(e.ErrorMessage()),
                  qUtf8Printable(e.DetailedInformation()));
        return;
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportDetailsAs(bool checked)
{
    Q_UNUSED(checked)

    auto Uncheck = qScopeGuard([this] { ui->actionDetailExportAs->setChecked(false); });

    QVector<DetailForLayout> const detailsInLayout = SortDetailsForLayout(pattern->DataPieces());

    if (detailsInLayout.isEmpty())
    {
        QMessageBox::information(this, tr("Layout mode"),
                                 tr("You don't have enough details to export. Please, "
                                    "include at least one detail in layout."),
                                 QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QVector<VLayoutPiece> listDetails;
    try
    {
        listDetails = PrepareDetailsForLayout(detailsInLayout);
    }
    catch (VException &e)
    {
        QMessageBox::warning(this, tr("Export details"), tr("Can't export details.") + " \n"_L1 + e.ErrorMessage(),
                             QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    try
    {
        m_dialogSaveLayout =
            QSharedPointer<DialogSaveLayout>(new DialogSaveLayout(1, Draw::Modeling, FileName(), this));

        VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
        m_dialogSaveLayout->SetBoundaryTogetherWithNotches(settings->IsBoundaryTogetherWithNotches());

        if (m_dialogSaveLayout->exec() == QDialog::Rejected)
        {
            m_dialogSaveLayout.clear();
            return;
        }

        ExportData(listDetails);
        m_dialogSaveLayout.clear();
    }
    catch (const VException &e)
    {
        m_dialogSaveLayout.clear();
        qCritical("%s\n\n%s\n\n%s", qUtf8Printable(tr("Export error.")), qUtf8Printable(e.ErrorMessage()),
                  qUtf8Printable(e.DetailedInformation()));
        return;
    }
}



//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionExportRecipe_triggered()
{
    QString const filters(tr("Recipe files") + QStringLiteral("(*.vpr)"));
    QString const fileName = QFileDialog::getSaveFileName(
        this, tr("Export recipe"), QDir::homePath() + '/' + tr("recipe") + QStringLiteral(".vpr"), filters, nullptr,
        VAbstractApplication::VApp()->NativeFileDialog());
    if (fileName.isEmpty())
    {
        return;
    }

    try
    {
        VPatternRecipe recipe(doc);
        QString error;
        if (not recipe.SaveDocument(fileName, error))
        {
            qCWarning(vMainWindow, "%s", qUtf8Printable(tr("Could not save recipe. %1").arg(error)));
        }
    }
    catch (const VExceptionInvalidHistory &e)
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Could not create recipe file. %1").arg(e.ErrorMessage())));
    }
}

