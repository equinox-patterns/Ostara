/************************************************************************
 **
 **  @file   mainwindow_measurements_slots.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Measurement slot methods extracted from mainwindow.cpp
 **
 **  @note   Extracted from mainwindow.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring. Tape methods removed 06-08-2026.
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
#include "../vformat/vmeasurements.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../ifc/xml/utils.h"
#include "core/vapplication.h"
#include "dialogs/DialogNewMeasurements.h"
#include "dialogs/DialogOpenMeasurements.h"
#include "dialogs/DialogManageMeasurements.h"
#include "widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"
#include "widgets/MeasurementWorkspace/MeasurementWindow.h"
#include "widgets/MeasurementWorkspace/MeasurementWorkspace.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QTimer>

#include "../vwidgets/vwidgetpopup.h"


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::LoadAndConnectMeasurements(const QString &path)
{
    const QString patternPath = VAbstractValApplication::VApp()->GetPatternPath();
    QString mutablePath = path;
    if (!LoadMeasurements(patternPath, mutablePath))
    {
        return;
    }

    if (!doc->MPath().isEmpty())
    {
        m_watcher->removePath(AbsoluteMPath(patternPath, doc->MPath()));
    }
    doc->SetMPath(RelativeMPath(patternPath, path));
    m_watcher->addPath(path);
    PatternChangesWereSaved(false);
    ui->actionEditMeasurements->setEnabled(true);
    doc->LiteParseTree(Document::FullLiteParse);
    UpdateWindowTitle();
    const QString msg = tr("Measurements loaded");
    statusBar()->showMessage(msg, 5000);
    VWidgetPopup::PopupMessage(this, msg);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::OnLoadMeasurements()
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

    // .vst/.vit → open MeasurementWindow for conversion (like Welcome Screen)
    const QFileInfo fi(path);
    const QString suffix = fi.suffix().toLower();
    if (suffix == QStringLiteral("vst") || suffix == QStringLiteral("vit"))
    {
        auto *mw = new MeasurementWindow(this);
        mw->setAttribute(Qt::WA_DeleteOnClose);
        mw->SetOrigin(MeasurementWorkspace::Origin::FromDraw);
        mw->SetModeless(true);
        if (!mw->LoadFile(path))
        {
            delete mw;
            return;
        }
        // When the converted file is saved, load it into the canvas
        connect(mw, &MeasurementWindow::measurementsSaved, this,
                [this](const QString &savedPath) {
            if (!savedPath.isEmpty())
            {
                LoadAndConnectMeasurements(savedPath);
            }
        });
        mw->show();
        return;
    }

    // .omt/.otu → direct load into canvas
    LoadAndConnectMeasurements(path);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::OnEditMeasurements()
{
    if (doc->MPath().isEmpty())
    {
        statusBar()->showMessage(tr("No measurement table loaded"), 5000);
        return;
    }

    const QString absoluteMPath = AbsoluteMPath(
        VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());

    if (absoluteMPath.isEmpty() || !QFileInfo::exists(absoluteMPath))
    {
        QMessageBox::warning(this, tr("File not found"),
            tr("The measurement file could not be found at:\n%1").arg(absoluteMPath));
        return;
    }

    auto *mw = new MeasurementWindow(this);
    mw->setAttribute(Qt::WA_DeleteOnClose);
    mw->SetOrigin(MeasurementWorkspace::Origin::FromDraw);
    mw->SetModeless(true);

    if (!mw->LoadFile(absoluteMPath))
    {
        delete mw;
        return;
    }

    connect(mw, &MeasurementWindow::measurementsSaved, this,
            [this](const QString &savedPath) {
        if (savedPath.isEmpty())
        {
            return;
        }
        // Reload measurements into the canvas
        const QString patternPath = VAbstractValApplication::VApp()->GetPatternPath();
        QString mutablePath = savedPath;
        if (!LoadMeasurements(patternPath, mutablePath))
        {
            return;
        }
        doc->LiteParseTree(Document::FullLiteParse);
        // Delay the popup so the modeless window has time to close first
        const QString msg = tr("Measurements reloaded");
        QTimer::singleShot(150, this, [this, msg]() {
            VWidgetPopup::PopupMessage(this, msg);
        });
    });

    mw->show();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::OnNewMeasurements()
{
    DialogNewMeasurements dlg(this);
    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }

    auto *mw = new MeasurementWindow(this);
    mw->setAttribute(Qt::WA_DeleteOnClose);
    mw->SetOrigin(MeasurementWorkspace::Origin::FromDraw);
    mw->SetModeless(true);
    mw->NewDocument(dlg.MeasurementsType(), dlg.Unit());
    mw->SetTableName(dlg.TableName());

    connect(mw, &MeasurementWindow::measurementsSaved, this,
            [this](const QString &savedPath) {
        if (savedPath.isEmpty())
        {
            return;
        }
        LoadAndConnectMeasurements(savedPath);
    });

    mw->show();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionManageMeasurements()
{
    DialogManageMeasurements dlg(this);
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
    mw->setAttribute(Qt::WA_DeleteOnClose);
    mw->SetOrigin(MeasurementWorkspace::Origin::FromDraw);
    mw->SetModeless(true);

    if (!mw->LoadFile(path))
    {
        delete mw;
        return;
    }

    // When saved, if this file is the one currently loaded in the canvas, reload it
    connect(mw, &MeasurementWindow::measurementsSaved, this,
            [this](const QString &savedPath) {
        if (savedPath.isEmpty())
        {
            return;
        }
        if (!doc->MPath().isEmpty())
        {
            const QString currentMPath = AbsoluteMPath(
                VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());
            if (currentMPath == savedPath)
            {
                const QString patternPath = VAbstractValApplication::VApp()->GetPatternPath();
                QString mutablePath = savedPath;
                if (LoadMeasurements(patternPath, mutablePath))
                {
                    doc->LiteParseTree(Document::FullLiteParse);
                    VWidgetPopup::PopupMessage(this, tr("Measurements reloaded"));
                }
            }
        }
    });

    mw->show();
}
