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
#include "core/ogradedfavorites.h"
#include "mainwindow_helpers.h"
#include "mainwindowsnogui.h"
#include "widgets/InfoBlock/OPieceCartoucheTable.h"
#include "widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"
#include "widgets/ODockManager.h"
#include "widgets/owidgetsizeranges.h"
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

    // Keep the Size Ranges dock in sync with the range that is now active.
    RefreshSizeRangesDock();

    // Size Styles dock: rows = the size axis values of the loaded table.
    RefreshSizeStylesDock();

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

    QStringList axes;
    QMap<QString, QStringList> axisValues;
    if (not LoadGradedTableAxes(&axes, &axisValues))
    {
        QMessageBox::information(this, tr("Graded mode"),
                                 tr("Graded mode needs a multi-size measurements table (.omt)."),
                                 QMessageBox::Ok);
        return false;
    }

    // Reuse an existing range for this table — deterministic: keep the previously
    // active range when it is still coherent, otherwise the coherent range with the
    // lowest name. Stale ranges (table changed) are only dropped on the accept path
    // (a Cancel must not lose stored ranges — undo-free destructive edit otherwise).
    const auto *existing = pattern->SizeRanges();
    QString staleRangeToDrop;

    QStringList coherent;
    QStringList stale;
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
            coherent.append(it.key());
        }
        else
        {
            stale.append(it.key());
        }
    }

    if (not coherent.isEmpty())
    {
        if (m_activeGradedRangeName.isEmpty() || !coherent.contains(m_activeGradedRangeName))
        {
            m_activeGradedRangeName = coherent.first();
            for (int i = 1; i < coherent.size(); ++i)
            {
                if (coherent.at(i) < m_activeGradedRangeName)
                {
                    m_activeGradedRangeName = coherent.at(i);
                }
            }
        }
        return true;
    }

    if (not stale.isEmpty())
    {
        staleRangeToDrop = stale.first();
        for (int i = 1; i < stale.size(); ++i)
        {
            if (stale.at(i) < staleRangeToDrop)
            {
                staleRangeToDrop = stale.at(i);
            }
        }
        QMessageBox::warning(this, tr("Graded mode"),
                             tr("The stored size range no longer matches the loaded "
                                "measurements table. It was removed — please, create a new one."),
                             QMessageBox::Ok);
    }

    // Creation dialog + store — shared with the dock's "Add a size range" flow.
    const QString newName = CreateGradedRange(canonical, axes, axisValues, staleRangeToDrop);
    if (newName.isEmpty())
    {
        return false;
    }

    m_activeGradedRangeName = newName;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
QString MainWindow::CreateGradedRange(const QString &canonical, const QStringList &axes,
                                      const QMap<QString, QStringList> &axisValues,
                                      const QString &staleNameToDrop)
{
    if (doc == nullptr)
    {
        return {};
    }

    const auto *existing = pattern->SizeRanges();
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
    QStringList takenNames = existing != nullptr ? existing->keys() : QStringList();
    takenNames.removeAll(staleNameToDrop); // a dropped stale range may reuse its name
    config.takenNames = takenNames;

    // Offer the table's favorites (per chosen axis) in the create/load step.
    config.favoriteNames = [settings, canonical](const QString &axisName) -> QStringList {
        return OGradedFavorites::FavoriteNames(settings, canonical, axisName);
    };

    OGradedRangeDialog dialog(config, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return {};
    }

    // Persist the axis declaration (software store) when the table is in the library.
    if (config.axisStepNeeded && inLibrary)
    {
        settings->SetSizeAxisForMeasurement(canonical, dialog.sizeAxis());
    }

    OSizerange range;
    if (dialog.LoadedFromFavorite())
    {
        // Load from a favorite: rebuild the range straight from the stored definition.
        range = OGradedFavorites::RangeFromFavorite(settings, canonical, dialog.sizeAxis(),
                                                    dialog.FavoriteName());
    }
    else
    {
        range = dialog.range();
    }
    if (not range.IsValid())
    {
        return {};
    }

    QHash<QString, OSizerange> ranges =
        existing != nullptr ? *existing : QHash<QString, OSizerange>();
    ranges.remove(staleNameToDrop);
    if (ranges.contains(range.name))
    {
        QMessageBox::warning(this, tr("Graded mode"),
                             tr("A size range with this name already exists."), QMessageBox::Ok);
        return {};
    }
    ranges.insert(range.name, range);
    pattern->SetSizeRanges(ranges);

    QDomElement rootElement = doc->documentElement();
    AddSizeRanges(doc, rootElement, pattern);
    doc->SetModified(true);

    return range.name;
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::LoadGradedTableAxes(QStringList *axes, QMap<QString, QStringList> *axisValues) -> bool
{
    OPieceCartoucheTable reader;
    reader.Load();
    if (not reader.HasMultiSizeTable() || reader.AxisNames().isEmpty())
    {
        return false;
    }

    if (axes != nullptr)
    {
        *axes = reader.AxisNames();
    }
    if (axisValues != nullptr)
    {
        axisValues->clear();
        for (const QString &axis : reader.AxisNames())
        {
            axisValues->insert(axis, reader.AxisValues(axis));
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshSizeRangesDock()
{
    if (m_dockManager != nullptr && m_dockManager->sizeRangesWidget() != nullptr)
    {
        m_dockManager->sizeRangesWidget()->UpdateList(m_activeGradedRangeName);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RebuildGradedCanvas(bool zoomFitBest)
{
    if (m_sceneGraded == nullptr || VAbstractValApplication::VApp()->GetDrawMode() != Draw::Graded)
    {
        return;
    }

    m_gradedForceCapture = true;
    BuildGradedScene();
    if (zoomFitBest)
    {
        ui->view->ZoomFitBest();
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool MainWindow::AddGradedRange()
{
    if (doc == nullptr || doc->MPath().isEmpty())
    {
        return false;
    }

    const QString canonical =
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());
    QStringList axes;
    QMap<QString, QStringList> axisValues;
    if (not LoadGradedTableAxes(&axes, &axisValues))
    {
        QMessageBox::information(this, tr("Graded mode"),
                                 tr("Graded mode needs a multi-size measurements table (.omt)."),
                                 QMessageBox::Ok);
        return false;
    }

    const QString newName = CreateGradedRange(canonical, axes, axisValues, QString());
    if (newName.isEmpty())
    {
        return false;
    }

    // The new range becomes the active one and its layout is captured.
    m_activeGradedRangeName = newName;
    RebuildGradedCanvas(true);
    RefreshSizeRangesDock();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActivateGradedRange(const QString &rangeName)
{
    const auto *existing = pattern->SizeRanges();
    if (existing == nullptr || !existing->contains(rangeName))
    {
        return;
    }
    if (m_activeGradedRangeName == rangeName)
    {
        return; // already active
    }

    m_activeGradedRangeName = rangeName;
    RebuildGradedCanvas(true);
    RefreshSizeRangesDock();
}

//---------------------------------------------------------------------------------------------------------------------
bool MainWindow::DuplicateGradedRange(const QString &rangeName)
{
    const auto *existing = pattern->SizeRanges();
    if (existing == nullptr || !existing->contains(rangeName))
    {
        return false;
    }

    const OSizerange source = existing->value(rangeName);

    // Fresh unique name: "<name> copy", "<name> copy 2", …
    QString copyName = source.name + QLatin1String(" copy");
    int suffix = 2;
    while (existing->contains(copyName))
    {
        copyName = source.name + QStringLiteral(" copy %1").arg(suffix);
        ++suffix;
    }

    // Copy the WHOLE range: definition AND content (piece/group positions,
    // children geometry). The copy is a distinct range — later edits never
    // touch the source.
    OSizerange copy = source;
    copy.name = copyName;

    QHash<QString, OSizerange> ranges = *existing;
    ranges.insert(copyName, copy);
    pattern->SetSizeRanges(ranges);

    QDomElement rootElement = doc->documentElement();
    AddSizeRanges(doc, rootElement, pattern);
    doc->SetModified(true);

    // The copy becomes the active range so its (identical) layout is shown.
    m_activeGradedRangeName = copyName;
    RebuildGradedCanvas(true);
    RefreshSizeRangesDock();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool MainWindow::EditGradedRange(const QString &rangeName)
{
    if (doc == nullptr || doc->MPath().isEmpty())
    {
        return false;
    }

    const auto *existing = pattern->SizeRanges();
    if (existing == nullptr || !existing->contains(rangeName))
    {
        return false;
    }
    const OSizerange stored = existing->value(rangeName);

    const QString canonical =
        AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());
    if (stored.measurement != canonical)
    {
        QMessageBox::information(this, tr("Graded mode"),
                                 tr("This size range belongs to another measurements table. Load "
                                    "its table to edit it."),
                                 QMessageBox::Ok);
        return false;
    }

    QStringList axes;
    QMap<QString, QStringList> axisValues;
    if (not LoadGradedTableAxes(&axes, &axisValues))
    {
        return false;
    }

    OGradedRangeDialog::Config config;
    config.canonicalPath = canonical;
    config.axisNames = axes;
    config.axisValues = axisValues;
    config.axisStepNeeded = false; // Edit keeps the range's own graded axis
    config.declaredAxis = stored.sizeAxis;
    QStringList takenNames = existing->keys();
    takenNames.removeAll(rangeName); // the edited range may keep its name
    config.takenNames = takenNames;
    config.editing = true;
    config.initial = stored;

    OGradedRangeDialog dialog(config, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    OSizerange edited = dialog.range();
    if (not edited.IsValid())
    {
        return false;
    }

    // The dialog returns the definition only — keep the range content untouched.
    edited.pieceContent = stored.pieceContent;
    edited.groupContent = stored.groupContent;

    auto *settings = VAbstractApplication::VApp()->Settings();
    const bool wasFavorite =
        OGradedFavorites::IsFavorite(settings, stored.measurement, stored.sizeAxis, stored.name);
    // Renaming a favorited range onto another favorite's name is refused.
    if (wasFavorite && edited.name != stored.name
        && OGradedFavorites::IsFavorite(settings, stored.measurement, stored.sizeAxis, edited.name))
    {
        QMessageBox::warning(this, tr("Graded mode"),
                             tr("A favorite already has this name. Choose another name."),
                             QMessageBox::Ok);
        return false;
    }

    QHash<QString, OSizerange> ranges = *existing;
    ranges.remove(rangeName);
    if (ranges.contains(edited.name))
    {
        QMessageBox::warning(this, tr("Graded mode"),
                             tr("A size range with this name already exists."), QMessageBox::Ok);
        return false;
    }
    ranges.insert(edited.name, edited);
    pattern->SetSizeRanges(ranges);

    QDomElement rootElement = doc->documentElement();
    AddSizeRanges(doc, rootElement, pattern);
    doc->SetModified(true);

    // Keep the software favorite in sync when the edited range was a favorite.
    if (wasFavorite)
    {
        OGradedFavorites::RemoveFavorite(settings, stored);
        OGradedFavorites::StoreFavorite(settings, edited);
    }

    // The active range may have been renamed, or its size interval changed (the
    // layer count depends on min/max) → force a full re-capture.
    if (m_activeGradedRangeName == rangeName)
    {
        m_activeGradedRangeName = edited.name;
        RebuildGradedCanvas();
    }

    RefreshSizeRangesDock();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
bool MainWindow::DeleteGradedRange(const QString &rangeName)
{
    const auto *existing = pattern->SizeRanges();
    if (existing == nullptr || !existing->contains(rangeName))
    {
        return false;
    }

    QHash<QString, OSizerange> ranges = *existing;
    ranges.remove(rangeName);
    pattern->SetSizeRanges(ranges);

    QDomElement rootElement = doc->documentElement();
    AddSizeRanges(doc, rootElement, pattern);
    doc->SetModified(true);

    // Deleting the active range: activate the remaining one with the lowest name,
    // or clear the canvas when none is left (the list is then empty).
    if (m_activeGradedRangeName == rangeName)
    {
        QString next;
        if (not ranges.isEmpty())
        {
            auto it = ranges.cbegin();
            next = it.key();
            for (++it; it != ranges.cend(); ++it)
            {
                if (it.key() < next)
                {
                    next = it.key();
                }
            }
        }
        m_activeGradedRangeName = next;

        if (m_sceneGraded != nullptr && VAbstractValApplication::VApp()->GetDrawMode() == Draw::Graded)
        {
            if (next.isEmpty())
            {
                m_sceneGraded->clear();
            }
            else
            {
                RebuildGradedCanvas();
            }
        }
    }

    RefreshSizeRangesDock();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RenameGradedRange(const QString &oldName, const QString &newName)
{
    const auto *existing = pattern->SizeRanges();
    if (existing == nullptr || !existing->contains(oldName) || existing->contains(newName))
    {
        // Nothing to rename — refresh the list so the widget reverts if needed.
        RefreshSizeRangesDock();
        return;
    }

    QHash<QString, OSizerange> ranges = *existing;
    OSizerange range = ranges.take(oldName);
    const OSizerange oldRange = range; // favorite bookkeeping uses the pre-rename range

    auto *settings = VAbstractApplication::VApp()->Settings();
    const bool wasFavorite =
        OGradedFavorites::IsFavorite(settings, oldRange.measurement, oldRange.sizeAxis, oldRange.name);
    // Renaming a favorited range onto another favorite's name is refused — nothing
    // has been stored yet, so the dock list refresh reverts the edited cell.
    if (wasFavorite && newName != oldRange.name
        && OGradedFavorites::IsFavorite(settings, oldRange.measurement, oldRange.sizeAxis, newName))
    {
        RefreshSizeRangesDock();
        return;
    }

    range.name = newName; // definition and content are untouched
    ranges.insert(newName, range);
    pattern->SetSizeRanges(ranges);

    QDomElement rootElement = doc->documentElement();
    AddSizeRanges(doc, rootElement, pattern);
    doc->SetModified(true);

    // Keep the software favorite in sync (the favorite follows the range name).
    if (wasFavorite)
    {
        OGradedFavorites::RemoveFavorite(settings, oldRange);
        OGradedFavorites::StoreFavorite(settings, range);
    }

    if (m_activeGradedRangeName == oldName)
    {
        m_activeGradedRangeName = newName;
    }

    RefreshSizeRangesDock();
}
