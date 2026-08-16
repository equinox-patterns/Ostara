/************************************************************************
 **
 **  @file   mainwindow_welcome.cpp
 **  @date   09-08-2026
 **
 **  @brief  WelcomeScreen measurement slots for MainWindow.
 **
 **  @note   Created by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara WelcomeScreen integration.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys
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
#include "WelcomeScreen.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vabstractvalapplication.h"
#include "dialogs/DialogNewMeasurements.h"
#include "dialogs/DialogOpenMeasurements.h"
#include "widgets/MeasurementWorkspace/MeasurementWindow.h"
#include "widgets/MeasurementWorkspace/MeasurementWorkspace.h"
#include <QFileInfo>

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::AddRecentMeasurementAndRefresh(const QString &path)
{
    if (path.isEmpty() || !QFileInfo::exists(path))
    {
        return;
    }
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    QStringList files = settings->GetRecentMeasurementFileList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > MaxRecentFilesStored)
    {
        files.removeLast();
    }
    settings->SetRecentMeasurementFileList(files);

    m_welcomeScreen->UpdateRecentItems(settings->GetRecentFileList(), files);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::WelcomeNewMeasurement()
{
    DialogNewMeasurements dlg(this);
    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }
    auto *mw = new MeasurementWindow(this);
    mw->SetOrigin(MeasurementWorkspace::Origin::FromHome);
    mw->setAttribute(Qt::WA_DeleteOnClose);
    connect(mw, &MeasurementWindow::measurementsSaved, this, &MainWindow::AddRecentMeasurementAndRefresh);
    mw->NewDocument(dlg.MeasurementsType(), dlg.Unit());
    mw->SetTableName(dlg.TableName());
    mw->show();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::WelcomeOpenMeasurement()
{
    DialogOpenMeasurements dlg(this);
    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }
    const QString path = dlg.selectedPath();
    if (path.isEmpty())
    {
        return;
    }
    auto *mw = new MeasurementWindow(this);
    mw->SetOrigin(MeasurementWorkspace::Origin::FromHome);
    mw->setAttribute(Qt::WA_DeleteOnClose);
    connect(mw, &MeasurementWindow::measurementsSaved, this, &MainWindow::AddRecentMeasurementAndRefresh);
    mw->show();
    mw->LoadFile(path);
    if (path.endsWith(QStringLiteral(".omt"), Qt::CaseInsensitive)
        || path.endsWith(QStringLiteral(".otu"), Qt::CaseInsensitive))
    {
        AddRecentMeasurementAndRefresh(path);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::WelcomeOpenRecentMeasurement(const QString &path)
{
    if (path.isEmpty())
    {
        return;
    }
    auto *mw = new MeasurementWindow(this);
    mw->SetOrigin(MeasurementWorkspace::Origin::FromHome);
    mw->setAttribute(Qt::WA_DeleteOnClose);
    connect(mw, &MeasurementWindow::measurementsSaved, this, &MainWindow::AddRecentMeasurementAndRefresh);
    mw->show();
    mw->LoadFile(path);
    AddRecentMeasurementAndRefresh(path);
}
