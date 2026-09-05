/************************************************************************
 **
 **  @file   mainwindow_graded.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  Graded mode entry: mode button handling and size-range flow.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
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

#include "dialogs/ogradedrangedialog.h"
#include "mainwindow_helpers.h"
#include "mainwindowsnogui.h"
#include "widgets/InfoBlock/OPieceCartoucheTable.h"
#include "widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"
#include "widgets/ODockManager.h"
#include "widgets/TopBar.h"
#include "xml/vpattern.h"
#include "xml/vpattern_graded.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QCoreApplication>
#include <QDomDocument>
#include <QEventLoop>
#include <QHash>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QStringList>
#include <QtGlobal>


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionGraded(bool checked)
{
    if (not checked)
    {
        ui->actionGraded->setChecked(true);
        return;
    }

    ArrowTool(true);

    const Draw prevMode = VAbstractValApplication::VApp()->GetDrawMode();

    ui->actionDraw->setChecked(false);
    ui->actionDetails->setChecked(false);
    ui->actionGraded->setChecked(true);
    ui->actionLayout->setChecked(false);

    // Guard: Graded needs at least one piece (created in Details) and a loaded
    // measurements table. The multi-size requirement is checked by the entry
    // flow (EnterGradedMode).
    if (not VAbstractValApplication::VApp()->getOpeningPattern() &&
        (pattern->DataPieces()->isEmpty() || m_m.isNull()))
    {
        QMessageBox::information(this, tr("Graded mode"),
                                 tr("You can't use Graded mode yet. Please, create at least one "
                                    "workpiece and load a measurements table."),
                                 QMessageBox::Ok, QMessageBox::Ok);
        ui->actionGraded->setChecked(false);
        prevMode == Draw::Calculation ? ui->actionDraw->setChecked(true)
                                      : ui->actionDetails->setChecked(true);
        return;
    }

    // Switch to Graded first (empty canvas, graded chrome), so the entry dialog opens
    // ABOVE the empty graded canvas — not from the mode the user clicked in.
    qCDebug(vMainWindow, "Show graded scene");
    m_dockManager->saveDockStateForMode(prevMode);

    if (not switchScene(Draw::Graded))
    {
        return;
    }

    SetEnableWidgets(true);
    m_dockManager->applyModeDocks(Draw::Graded);
    m_topBar->SetGradedChrome(true);

    // Entry flow: create or reuse the size range for the active measurements table.
    // Cancel (or no usable table) → back to the previous mode.
    if (not EnterGradedMode())
    {
        prevMode == Draw::Calculation ? ActionDraw(true) : ActionDetails(true);
        return;
    }

    // Paint the just-switched Graded scene before the (potentially long) capture starts, so a
    // later "please wait" dialog appears over the Graded canvas — not over the mode that was
    // left. On the very first build the entry dialog's own event loop already painted it.
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    m_gradedForceCapture = true; // entering from another mode → always refresh the silhouettes
    BuildGradedScene();
    // Frame the whole graded arrangement on entry (same as the "Zoom fit best" action).
    ui->view->ZoomFitBest();

    m_statusLabel->setText(QString());
}

//---------------------------------------------------------------------------------------------------------------------
bool MainWindow::EnterGradedMode()
{
    if (doc == nullptr || doc->MPath().isEmpty())
    {
        return false;
    }

    const QString canonical =
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());

    OPieceCartoucheTable reader;
    reader.Load();
    if (not reader.HasMultiSizeTable() || reader.AxisNames().isEmpty())
    {
        QMessageBox::information(this, tr("Graded mode"),
                                 tr("Graded mode needs a multi-size measurements table (.omt)."),
                                 QMessageBox::Ok);
        return false;
    }

    const QStringList axes = reader.AxisNames();
    QMap<QString, QStringList> axisValues;
    for (const QString &axis : axes)
    {
        axisValues.insert(axis, reader.AxisValues(axis));
    }

    // Reuse an existing range for this table — only if still coherent with the current
    // table (graded axis + frozen values of every OTHER axis must still exist).
    const auto *existing = pattern->SizeRanges();
    QString staleRangeToDrop; // dropped only on the accept path (never on Cancel)
    for (auto it = existing->cbegin(); it != existing->cend(); ++it)
    {
        if (it->measurement != canonical)
        {
            continue;
        }

        const QStringList gradedValues = axisValues.value(it->sizeAxis);
        const int lo = static_cast<int>(gradedValues.indexOf(it->minValue));
        const int hi = static_cast<int>(gradedValues.indexOf(it->maxValue));

        bool frozenValuesOk = true;
        for (auto av = it->axisValues.cbegin(); av != it->axisValues.cend(); ++av)
        {
            if (!axes.contains(av.key()) || !axisValues.value(av.key()).contains(av.value()))
            {
                frozenValuesOk = false;
                break;
            }
        }

        if (axes.contains(it->sizeAxis) && lo >= 0 && hi >= 0 && lo <= hi && frozenValuesOk)
        {
            m_activeGradedRangeName = it.key();
            return true;
        }

        // Stale range (table changed): defer the removal until the user accepts a new one —
        // a Cancel must NOT lose the stored range (undo-free destructive edit otherwise).
        staleRangeToDrop = it.key();
        QMessageBox::warning(this, tr("Graded mode"),
                             tr("The stored size range no longer matches the loaded "
                                "measurements table. It was removed — please, create a new one."),
                             QMessageBox::Ok);
        break;
    }

    auto *settings = VAbstractApplication::VApp()->Settings();
    const QString libPath = settings->GetPathMeasurementLibrary();
    const bool inLibrary = MeasurementLibraryScanner::IsInLibrary(libPath, canonical);
    const QString declared = inLibrary ? settings->GetSizeAxisForMeasurement(canonical) : QString();

    OGradedRangeDialog::Config config;
    config.canonicalPath = canonical;
    config.axisNames = axes;
    config.axisValues = axisValues;
    config.axisStepNeeded = declared.isEmpty();
    config.declaredAxis = declared;
    QStringList takenNames = existing->keys();
    takenNames.removeAll(staleRangeToDrop); // the stale range's own name may be reused
    config.takenNames = takenNames;

    OGradedRangeDialog dialog(config, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    // Persist the axis declaration (software store) when the table lives in the library.
    if (config.axisStepNeeded && inLibrary)
    {
        settings->SetSizeAxisForMeasurement(canonical, dialog.sizeAxis());
    }

    OSizerange range = dialog.range();
    if (not range.IsValid())
    {
        return false;
    }

    QHash<QString, OSizerange> ranges = *pattern->SizeRanges();
    ranges.remove(staleRangeToDrop); // deferred stale drop — only now that a new range is accepted
    if (ranges.contains(range.name))
    {
        QMessageBox::warning(this, tr("Graded mode"),
                             tr("A size range with this name already exists."), QMessageBox::Ok);
        return false;
    }
    ranges.insert(range.name, range);
    pattern->SetSizeRanges(ranges);

    QDomElement rootElement = doc->documentElement();
    AddSizeRanges(doc, rootElement, pattern);
    doc->SetModified(true);

    m_activeGradedRangeName = range.name;
    return true;
}
