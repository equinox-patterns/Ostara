/************************************************************************
 **
 **  @file   mainwindow_measurements_slots.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Measurement slot methods extracted from mainwindow.cpp
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
#include "../vformat/vmeasurements.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/dialogs/dialogselectmeasurementstype.h"
#include "../ifc/xml/utils.h"
#include "core/vapplication.h"
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>

#include "../vwidgets/vwidgetpopup.h"
#include <QTimer>

using namespace Qt::Literals::StringLiterals;
using namespace std::chrono_literals;


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ConnectMeasurements()
{
    DialogSelectMeasurementsType selector(this);
    if (selector.exec() != QDialog::Accepted)
    {
        return;
    }

    const MeasurementsType type = selector.Type();

    QString filter;
    QString defaultPath;
    bool updateIndividualPath = false;

    if (type == MeasurementsType::Individual)
    {
        filter = tr("Individual measurements") + " (*.vit);;"_L1 + tr("All files") + " (*.*)"_L1;
        defaultPath = VAbstractValApplication::VApp()->ValentinaSettings()->GetPathIndividualMeasurements();
        updateIndividualPath = true;
    }
    else if (type == MeasurementsType::Multisize)
    {
        filter = tr("Multisize measurements") + " (*.vst);;"_L1 + tr("All files") + " (*.*)"_L1;
        defaultPath = VAbstractValApplication::VApp()->ValentinaSettings()->GetPathMultisizeMeasurements();
    }
    else
    {
        return; // unknown type
    }

    QString mPath = QFileDialog::getOpenFileName(
        this,
        tr("Open file"),
        defaultPath,
        filter,
        nullptr,
        VAbstractApplication::VApp()->NativeFileDialog());

    if (mPath.isEmpty())
    {
        return;
    }

    const QString patternPath = VAbstractValApplication::VApp()->GetPatternPath();
    if (!LoadMeasurements(patternPath, mPath))
    {
        return;
    }

    if (!doc->MPath().isEmpty())
    {
        m_watcher->removePath(AbsoluteMPath(patternPath, doc->MPath()));
    }
    ui->actionUnloadMeasurements->setEnabled(true);
    doc->SetMPath(RelativeMPath(patternPath, mPath));
    m_watcher->addPath(mPath);
    PatternChangesWereSaved(false);
    ui->actionEditCurrent->setEnabled(true);
    statusBar()->showMessage(tr("Measurements loaded"), 5000);
    doc->LiteParseTree(Document::FullLiteParse);
    UpdateWindowTitle();

    if (updateIndividualPath)
    {
        VAbstractValApplication::VApp()->ValentinaSettings()
            ->SetPathIndividualMeasurements(QFileInfo(mPath).absolutePath());
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UnloadMeasurements()
{
    if (doc->MPath().isEmpty())
    {
        ui->actionUnloadMeasurements->setDisabled(true);
        return;
    }

    if (doc->ListMeasurements().isEmpty())
    {
        m_watcher->removePath(AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath()));

        const MeasurementsType oldType = VAbstractValApplication::VApp()->GetMeasurementsType();
        VAbstractValApplication::VApp()->SetMeasurementsType(MeasurementsType::Unknown);

        m_m.clear();

        VAbstractValApplication::VApp()->SetDimensionHeight(0);
        VAbstractValApplication::VApp()->SetDimensionSize(0);
        VAbstractValApplication::VApp()->SetDimensionHip(0);
        VAbstractValApplication::VApp()->SetDimensionWaist(0);

        VAbstractValApplication::VApp()->SetDimensionHeightLabel(QString());
        VAbstractValApplication::VApp()->SetDimensionSizeLabel(QString());
        VAbstractValApplication::VApp()->SetDimensionHipLabel(QString());
        VAbstractValApplication::VApp()->SetDimensionWaistLabel(QString());

        if (oldType == MeasurementsType::Multisize)
        {
            m_currentDimensionA = 0;
            m_currentDimensionB = 0;
            m_currentDimensionC = 0;

            ToolBarOption();
        }
        else if (oldType == MeasurementsType::Individual)
        {
            VAbstractValApplication::VApp()->SetCustomerBirthDate(QDate());
            VAbstractValApplication::VApp()->SetCustomerEmail(QString());
            VAbstractValApplication::VApp()->SetCustomerName(QString());
            VAbstractValApplication::VApp()->SetMeasurementsUnits(Unit::LAST_UNIT_DO_NOT_USE);
            VAbstractValApplication::VApp()->SetDimensionSizeUnits(Unit::LAST_UNIT_DO_NOT_USE);
        }

        doc->SetMPath(QString());
        emit doc->UpdatePatternLabel();
        PatternChangesWereSaved(false);
        ui->actionEditCurrent->setEnabled(false);
        ui->actionUnloadMeasurements->setDisabled(true);
        statusBar()->showMessage(tr("Measurements unloaded"), 5000);

        UpdateWindowTitle();
    }
    else
    {
        qCWarning(vMainWindow, "%s",
                  qUtf8Printable(tr("Couldn't unload measurements. Some of them are used in the pattern.")));
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowMeasurements()
{
    if (not doc->MPath().isEmpty())
    {
        const QString absoluteMPath = AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());

        QStringList arguments;
        arguments.append(absoluteMPath);
        arguments.append("-u"_L1);
        arguments.append(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));

        if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize)
        {
            if (m_currentDimensionA > 0)
            {
                arguments.append("-a"_L1);
                arguments.append(QString::number(m_currentDimensionA));
            }

            if (m_currentDimensionB > 0)
            {
                arguments.append("-b"_L1);
                arguments.append(QString::number(m_currentDimensionB));
            }

            if (m_currentDimensionC > 0)
            {
                arguments.append("-c"_L1);
                arguments.append(QString::number(m_currentDimensionC));
            }
        }

        if (isNoScaling)
        {
            arguments.append("--"_L1 + LONG_OPTION_NO_HDPI_SCALING);
        }

        const QString path = VApplication::TapeFilePath();
        qDebug("Opening Tape: path = %s.", qUtf8Printable(path));
        VApplication::StartDetachedProcess(path, arguments);
    }
    else
    {
        ui->actionEditCurrent->setEnabled(false);
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::MeasurementsChanged(const QString &path)
{
    qCDebug(vMainWindow, "Measurements file changed: %s", qUtf8Printable(path));
    m_mChanges = false;
    if (QFileInfo const checkFile(path); checkFile.exists())
    {
        m_mChanges = true;
        m_mChangesAsked = false;
        m_measurementsSyncTimer->start(1500ms);
    }
    else
    {
        for (int i = 0; i <= 1000; i = i + 10)
        {
            if (checkFile.exists())
            {
                m_mChanges = true;
                m_mChangesAsked = false;
                m_measurementsSyncTimer->start(1500ms);
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    UpdateWindowTitle();
    ui->actionSyncMeasurements->setEnabled(m_mChanges);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SyncMeasurements()
{
    if (m_mChanges)
    {
        const QString patternPath = VAbstractValApplication::VApp()->GetPatternPath();
        QString path = AbsoluteMPath(patternPath, doc->MPath());
        qCDebug(vMainWindow, "Syncing measurements: %s", qUtf8Printable(path));

        // Temporarily remove the path to prevent infinite synchronization after a format conversion.
        m_watcher->removePath(path);

        if (UpdateMeasurements(patternPath, path, m_currentDimensionA, m_currentDimensionB, m_currentDimensionC))
        {
            const QString msg = tr("Measurements have been synced");
            qCDebug(vMainWindow, "%s", qUtf8Printable(msg));
            statusBar()->showMessage(msg, 5000);
            VWidgetPopup::PopupMessage(this, msg);
            doc->LiteParseTree(Document::FullLiteParse);
            StoreDimensions();
            m_mChanges = false;
            m_mChangesAsked = true;
            m_measurementsSyncTimer->stop();
            UpdateWindowTitle();
            ui->actionSyncMeasurements->setEnabled(m_mChanges);
        }
        else
        {
            qCWarning(vMainWindow, "%s", qUtf8Printable(tr("Couldn't sync measurements.")));
        }

        if (not m_watcher->files().contains(path))
        {
            m_watcher->addPath(path);
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::MeasurementsSyncTimerTimeout()
{
    if (isActiveWindow())
    {
        static bool asking = false;
        if (not asking && m_mChanges && not m_mChangesAsked)
        {
            asking = true;
            m_mChangesAsked = true;
            m_measurementsSyncTimer->stop();
            qCDebug(vMainWindow, "Asking user to sync measurements.");
            const auto answer = QMessageBox::question(
                this, tr("Measurements"),
                tr("Measurements were changed. Do you want to sync measurements now?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            qCDebug(vMainWindow, "User answered sync prompt: %s",
                    answer == QMessageBox::Yes ? "Yes" : "No");
            if (answer == QMessageBox::Yes)
            {
                SyncMeasurements();
            }
            asking = false;
        }
    }
}

