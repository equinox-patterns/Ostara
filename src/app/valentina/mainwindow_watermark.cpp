/************************************************************************
 **
 **  @file   mainwindow_watermark.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Watermark methods extracted from mainwindow.cpp
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
#include "../vmisc/vabstractvalapplication.h"
#include "../vlayout/dialogs/watermarkwindow.h"
#include "core/vapplication.h"

#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::CreateWatermark()
{
    CleanWaterkmarkEditors();
    OpenWatermark();
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::EditCurrentWatermark()
{
    CleanWaterkmarkEditors();

    QString const watermarkFile = doc->GetWatermarkPath();
    if (not watermarkFile.isEmpty())
    {
        OpenWatermark(watermarkFile);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::LoadWatermark()
{
    const QString filter(tr("Watermark files") + QStringLiteral(" (*.vwm)"));
    QString const dir = QDir::homePath();
    qDebug("Run QFileDialog::getOpenFileName: dir = %s.", qUtf8Printable(dir));
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open file"), dir, filter, nullptr,
                                                          VAbstractApplication::VApp()->NativeFileDialog());
    if (filePath.isEmpty())
    {
        return;
    }

    if (doc->SetWatermarkPath(filePath))
    {
        ui->actionRemoveWatermark->setEnabled(true);
        ui->actionEditCurrentWatermark->setEnabled(true);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RemoveWatermark()
{
    if (doc->SetWatermarkPath(QString()))
    {
        ui->actionRemoveWatermark->setEnabled(false);
        ui->actionEditCurrentWatermark->setEnabled(false);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::CleanWaterkmarkEditors()
{
    QMutableListIterator i(m_watermarkEditors);
    while (i.hasNext())
    {
        QPointer<WatermarkWindow> const watermarkEditor = i.next();
        if (watermarkEditor.isNull())
        {
            i.remove();
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::OpenWatermark(const QString &path)
{
    for (const auto &watermarkEditor : std::as_const(m_watermarkEditors))
    {
        if (not watermarkEditor.isNull() && not watermarkEditor->CurrentFile().isEmpty() &&
            watermarkEditor->CurrentFile() == AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), path))
        {
            watermarkEditor->show();
            return;
        }
    }

    auto *watermark = new WatermarkWindow(VAbstractValApplication::VApp()->GetPatternPath(), this);
    connect(watermark, &WatermarkWindow::New, this, [this]() { OpenWatermark(); });
    connect(watermark, &WatermarkWindow::OpenAnother, this, [this](const QString &path) { OpenWatermark(path); });
    m_watermarkEditors.append(watermark);
    watermark->show();
    watermark->Open(path);
}

