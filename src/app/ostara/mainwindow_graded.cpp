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
#include "core/ogradedrangerenames.h"
#include "core/oexportfavorites.h"
#include "core/ogradedsizeranges.h"
#include "mainwindow_helpers.h"
#include "mainwindowsnogui.h"
#include "core/osizetable.h"
#include "widgets/DrawToolBar.h"
#include "widgets/ODockManager.h"
#include "widgets/owidgetsizeranges.h"
#include "widgets/TopBar.h"
#include "xml/vpattern.h"
#include "xml/opattern_graded.h"
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
#include <QFileInfo>
#include <QHash>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUndoCommand>
#include <QtGlobal>

#include <optional>

namespace
{
using GradedRanges = QHash<QString, OSizerange>;

/**
 * @brief One edit of the graded size ranges, undoable — the five gestures of the CRUD.
 *
 * **The whole set is the unit.** A range is a keyed definition: creating, duplicating, editing,
 * renaming and deleting one are all moves in that key space, so a command that snapshots the key
 * space before and after covers the five without five commands each re-deriving what changed.
 *
 * It writes what the gestures wrote directly until now — the model (`VContainer::SetSizeRanges`)
 * and the document (`AddSizeRanges`, which announces the change) — and on undo it gives back what
 * a move has to give back: the ranges, the active one, and the list the dock shows. The canvas
 * needs nothing from it: `QUndoStack::indexChanged` already schedules a rebuild.
 */
class OSaveGradedRanges final : public QUndoCommand
{
public:
    OSaveGradedRanges(MainWindow *window, VContainer *data, VAbstractPattern *doc,
                      const GradedRanges &before, const GradedRanges &after,
                      const QString &activeBefore, const QString &activeAfter, const QString &text,
                      QUndoCommand *parent = nullptr)
      : QUndoCommand(text, parent),
        m_window(window),
        m_data(data),
        m_doc(doc),
        m_before(before),
        m_after(after),
        m_activeBefore(activeBefore),
        m_activeAfter(activeAfter)
    {
    }

    /// The two states of a rename, when this command is one. Empty: nothing moved in the key space.
    void SetRename(const std::optional<OSizerange> &before, const std::optional<OSizerange> &after)
    {
        m_renameBefore = before;
        m_renameAfter = after;
    }

    void redo() override
    {
        Apply(m_after, m_activeAfter);
        if (m_renameBefore.has_value() && m_renameAfter.has_value())
        {
            m_window->ApplyRangeRename(*m_renameBefore, *m_renameAfter);
        }
    }

    void undo() override
    {
        Apply(m_before, m_activeBefore);
        if (m_renameBefore.has_value() && m_renameAfter.has_value())
        {
            m_window->RevertRangeRename(*m_renameAfter, *m_renameBefore);
        }
    }

private:
    void Apply(const GradedRanges &ranges, const QString &active)
    {
        m_data->SetSizeRanges(ranges);
        QDomElement root = m_doc->documentElement();
        // Announces the change by itself — see VAbstractPattern::NotifyPatternChanged.
        AddSizeRanges(m_doc, root, m_data);
        m_window->RestoreGradedRangesState(active);
    }

    MainWindow *m_window;
    VContainer *m_data;
    VAbstractPattern *m_doc;
    GradedRanges m_before;
    GradedRanges m_after;
    QString m_activeBefore;
    QString m_activeAfter;
    std::optional<OSizerange> m_renameBefore;
    std::optional<OSizerange> m_renameAfter;
};
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::PushGradedRangesCommand(const QHash<QString, OSizerange> &before,
                                         const QHash<QString, OSizerange> &after,
                                         const QString &activeBefore, const QString &activeAfter,
                                         const QString &text, const OSizerange *renameFrom,
                                         const OSizerange *renameTo)
{
    auto *command =
        new OSaveGradedRanges(this, pattern, doc, before, after, activeBefore, activeAfter, text);
    if (renameFrom != nullptr && renameTo != nullptr)
    {
        command->SetRename(*renameFrom, *renameTo);
    }
    VAbstractApplication::VApp()->getUndoStack()->push(command);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RestoreGradedRangesState(const QString &activeRangeName)
{
    m_activeGradedRangeName = activeRangeName;

    if (activeRangeName.isEmpty() && m_sceneGraded != nullptr)
    {
        // Nothing left to draw — and the deferred rebuild skips an empty active range, so the
        // scene would go on showing a range that no longer exists.
        m_sceneGraded->clear();
    }

    RefreshSizeRangesDock();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ApplyRangeRename(const OSizerange &oldRange, const OSizerange &newRange)
{
    PropagateRangeRename(oldRange, newRange);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RevertRangeRename(const OSizerange &renamedRange, const OSizerange &formerRange)
{
    // The forward entry is taken back FIRST: leaving it beside the reverse one would make A→B and
    // B→A, a loop the journal deliberately stops at — and a command still holding B would never
    // find the range that has its name back.
    OGradedRangeRenames::Forget(formerRange.name);
    PropagateRangeRename(renamedRange, formerRange);
}

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

    switchScene(Draw::Graded);

    SetEnableWidgets(true);
    m_dockManager->applyModeDocks(Draw::Graded);

    // applyModeDocks() ends by restoring the Graded dock layout, and restoreState()
    // can re-show the work toolbar — it is part of the QMainWindow state even though
    // it is not managed by the dock manager. Draw/Details re-show it after
    // applyModeDocks(); Graded must re-hide it symmetrically, since the toolbar is
    // structurally empty in Graded mode.
    m_drawToolBar->hide();

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

    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);
    const QString canonical = context.canonical;

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
        if (not OGradedSizeRanges::SameMeasurementTable(it->measurement, canonical))
        {
            continue;
        }
        // Both conditions must hold: right table AND right grading axis. A range
        // saved under another axis of the same table stays in the file but is
        // never offered here (not coherent, not stale — it must never be dropped).
        if (not context.axis.isEmpty() && it->sizeAxis != context.axis)
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
                                "measurements table. It was removed - please, create a new one."),
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
    const OGradedSizeRanges::Context context = OGradedSizeRanges::ContextFor(canonical);
    const QString &declared = context.axis;

    OGradedRangeDialog::Config config;
    config.canonicalPath = canonical;
    config.axisNames = axes;
    config.axisValues = axisValues;
    config.axisStepNeeded = declared.isEmpty();
    config.declaredAxis = declared;
    QStringList takenNames = existing != nullptr ? existing->keys() : QStringList();
    takenNames.removeAll(staleNameToDrop); // a dropped stale range may reuse its name
    config.takenNames = takenNames;
    if (existing != nullptr)
    {
        for (auto it = existing->cbegin(); it != existing->cend(); ++it)
        {
            config.takenNameTables.insert(it.key(), it->measurement);
        }
    }
    config.takenNameTables.remove(staleNameToDrop);
    config.exportPath = settings->GetPathLayoutExport();
    config.patternNameToken = GradedPatternNameToken();

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
    if (config.axisStepNeeded && context.inLibrary)
    {
        settings->SetSizeAxisForMeasurement(canonical, dialog.sizeAxis());
    }

    // The ranges of the pattern, minus the stale one the dialog is allowed to reclaim. Copied ONCE,
    // before anything is inserted: one gesture is one command, so one Ctrl+Z takes the whole batch
    // back — a push per range would take one per range.
    const QHash<QString, OSizerange> before =
        existing != nullptr ? *existing : QHash<QString, OSizerange>();
    QHash<QString, OSizerange> ranges = before;
    ranges.remove(staleNameToDrop);

    QStringList added;
    if (dialog.LoadedFromFavorite())
    {
        // One range per ticked favorite, in the list's own order. The dialog only lets a favorite be
        // ticked when no range of the pattern already carries its name (the name is the key), so a
        // collision cannot come from here — but a favorite the store no longer resolves is skipped
        // rather than turned into a range with no ends.
        for (const QString &favorite : dialog.FavoriteNames())
        {
            const OSizerange loaded =
                OGradedFavorites::RangeFromFavorite(settings, canonical, dialog.sizeAxis(), favorite);
            if (not loaded.IsValid() || ranges.contains(loaded.name))
            {
                continue;
            }
            ranges.insert(loaded.name, loaded);
            added.append(loaded.name);
        }
    }
    else
    {
        const OSizerange range = dialog.range();
        if (not range.IsValid())
        {
            return {};
        }
        if (ranges.contains(range.name))
        {
            QMessageBox::warning(this, tr("Graded mode"),
                                 tr("A size range with this name already exists."), QMessageBox::Ok);
            return {};
        }
        ranges.insert(range.name, range);
        added.append(range.name);
    }

    if (added.isEmpty())
    {
        return {};
    }

    // **Undoable, like every other move of the ranges — and once for the whole gesture.** The command
    // writes the model and the document, and leaves ONE range active: the first one added, the one the
    // list shows at the top. The graded canvas draws a single range at a time, so "the last one read"
    // would make the active range depend on the order of a loop.
    PushGradedRangesCommand(before, ranges, m_activeGradedRangeName, added.first(),
                            added.size() == 1 ? tr("Add size range") : tr("Add size ranges"));

    return added.first();
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::LoadGradedTableAxes(QStringList *axes, QMap<QString, QStringList> *axisValues) -> bool
{
    OSizeTable reader;
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
    auto *widget = m_dockManager != nullptr ? m_dockManager->sizeRangesWidget() : nullptr;
    if (widget == nullptr)
    {
        return;
    }
    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);
    widget->SetTableFilter(context.canonical, context.axis);
    widget->UpdateList(m_activeGradedRangeName);
}

//---------------------------------------------------------------------------------------------------------------------
QString MainWindow::CurrentLoadedTableAxis() const
{
    return OGradedSizeRanges::CurrentContext(doc).axis;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGradedAfterTableAxisChange()
{
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Graded)
    {
        return; // Draw/Details re-filters when Graded is next entered
    }

    // Re-run the entry selection against the NEW declared axis: it re-filters the
    // stored ranges, re-picks (or creates) the active range, and drops nothing —
    // exactly what "leave Graded, re-enter" would do, minus the scene switch.
    if (EnterGradedMode())
    {
        m_gradedForceCapture = true; // the axis changed → silhouettes must be recaptured
        BuildGradedScene();
        ui->view->ZoomFitBest();
    }
    RefreshSizeRangesDock();
    RefreshSizeStylesDock();
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

    // The command made the new range active and refreshed the list; the canvas rebuild is
    // scheduled by `indexChanged`. Fit the view once that rebuild has run — same event queue,
    // later ticket, so the zoom lands on the new scene and not on the old one.
    QTimer::singleShot(0, this, [this] { ui->view->ZoomFitBest(); });
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

    const QHash<QString, OSizerange> before = *existing;
    QHash<QString, OSizerange> ranges = before;
    ranges.insert(copyName, copy);

    // The copy becomes the active range so its (identical) layout is shown.
    PushGradedRangesCommand(before, ranges, m_activeGradedRangeName, copyName,
                            tr("Duplicate size range"));
    QTimer::singleShot(0, this, [this] { ui->view->ZoomFitBest(); });
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
    for (auto it = existing->cbegin(); it != existing->cend(); ++it)
    {
        config.takenNameTables.insert(it.key(), it->measurement);
    }
    config.takenNameTables.remove(rangeName);
    config.editing = true;
    config.initial = stored;
    auto *settings = VAbstractApplication::VApp()->Settings();
    config.exportPath = settings->GetPathLayoutExport();
    config.patternNameToken = GradedPatternNameToken();

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

    // Renaming a favorited range onto another favorite's name is refused.
    if (RangeRenameHitsFavorite(stored, edited.name))
    {
        return false;
    }

    const QHash<QString, OSizerange> before = *existing;
    QHash<QString, OSizerange> ranges = before;
    ranges.remove(rangeName);
    if (ranges.contains(edited.name))
    {
        QMessageBox::warning(this, tr("Graded mode"),
                             tr("A size range with this name already exists."), QMessageBox::Ok);
        return false;
    }
    ranges.insert(edited.name, edited);

    // The dialog may have renamed the range. The command carries both names, so an undo moves
    // everything that followed the name — the favorites and the generations — back with it.
    const bool renamed = edited.name != rangeName;
    const QString activeAfter =
        m_activeGradedRangeName == rangeName ? edited.name : m_activeGradedRangeName;

    PushGradedRangesCommand(before, ranges, m_activeGradedRangeName, activeAfter,
                            tr("Edit size range"), renamed ? &stored : nullptr,
                            renamed ? &edited : nullptr);
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

    const QHash<QString, OSizerange> before = *existing;
    QHash<QString, OSizerange> ranges = before;
    ranges.remove(rangeName);

    // Deleting the active range: the next one is the lowest name still in the dock's
    // list — the same (table, axis) context the dock lists by. Taking it from the whole
    // hash would activate a range the user cannot see, leaving the list with no row
    // marked active. When nothing matches, the canvas and the list are simply empty:
    // the user leaves Graded, or adds a range.
    QString activeAfter = m_activeGradedRangeName;
    if (m_activeGradedRangeName == rangeName)
    {
        const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);
        QString next;
        for (auto it = ranges.cbegin(); it != ranges.cend(); ++it)
        {
            if (!OGradedSizeRanges::Matches(it.value(), context))
            {
                continue;
            }
            if (next.isEmpty() || it.key() < next)
            {
                next = it.key();
            }
        }
        activeAfter = next;
    }

    PushGradedRangesCommand(before, ranges, m_activeGradedRangeName, activeAfter,
                            tr("Delete size range"));
    return true;
}

/**
 * @brief Whether renaming @p oldRange to @p newName would collide with a favorite — and says so.
 *
 * The software store of favorites is keyed by the range's NAME, so two favorites of the same
 * context cannot carry it. When it happens the move is refused, and the dialog says why rather
 * than leaving a rename that silently did nothing.
 *
 * One definition for the two rename paths (the dock's inline editor and the edit dialog): the
 * guard, and the sentence it shows, were written twice before this.
 */
auto MainWindow::RangeRenameHitsFavorite(const OSizerange &oldRange, const QString &newName) -> bool
{
    if (newName == oldRange.name)
    {
        return false; // an edition, not a rename — no name is being taken
    }

    auto *settings = VAbstractApplication::VApp()->Settings();
    const bool hits =
        OGradedFavorites::IsFavorite(settings, oldRange.measurement, oldRange.sizeAxis, oldRange.name)
        && OGradedFavorites::IsFavorite(settings, oldRange.measurement, oldRange.sizeAxis, newName);
    if (hits)
    {
        QMessageBox::warning(this, tr("Graded mode"), tr("A favorite already has this name. Choose another name."),
                             QMessageBox::Ok);
    }
    return hits;
}

/**
 * @brief Everything that named @p oldRange follows it to @p newRange.
 *
 * A size range is addressed by its NAME — it is the key of `SizeRanges()` — so renaming one is
 * a move in the key space, and three things outside the pattern were holding the old name:
 *
 * - the **graded favorite**, whose record is the range's definition (name, min/max, frozen axis
 *   values). Removed and stored again rather than renamed, because an edition moves the
 *   definition as much as the name;
 * - the **favorites of the Export mode**, whose recipe names the range it was built on, resolved
 *   against the pattern at replay time;
 * - the **generations of this session**, replayed by name in the same way. Left alone, each of
 *   them replays into a size loop with nothing to walk and comes out empty — which is what the
 *   user saw when she refreshed a plan made before a rename. Their own name is NOT touched:
 *   what was broken was the reference, not the label.
 */
void MainWindow::PropagateRangeRename(const OSizerange &oldRange, const OSizerange &newRange)
{
    auto *settings = VAbstractApplication::VApp()->Settings();

    if (OGradedFavorites::IsFavorite(settings, oldRange.measurement, oldRange.sizeAxis, oldRange.name))
    {
        OGradedFavorites::RemoveFavorite(settings, oldRange);
        OGradedFavorites::StoreFavorite(settings, newRange);
    }

    if (oldRange.name == newRange.name)
    {
        return; // an edition: nothing outside the range addresses it by a name that moved
    }

    // Written down so that the content commands already on the undo stack — which hold the name
    // the range had when they were pushed, and which Qt does not let us reach — can still find
    // it. See OGradedRangeRenames.
    OGradedRangeRenames::Remember(oldRange.name, newRange.name);

    OExportFavorites::RenameRange(settings, oldRange.measurement, oldRange.sizeAxis, oldRange.name, newRange.name);

    for (const OExportOutput &output : m_exportOutputs.Outputs())
    {
        if (output.rangeName != oldRange.name)
        {
            continue;
        }

        OExportOutput renamed = output;
        renamed.rangeName = newRange.name;
        // The frozen copy carries the range's IDENTITY, not its measurements: a copy still
        // naming the range it no longer is would be a copy that lies about what it holds.
        // Replace() keeps the generation's own name — only what it points at moves.
        renamed.range.name = newRange.name;
        m_exportOutputs.Replace(renamed.id, renamed);
    }
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

    const QHash<QString, OSizerange> before = *existing;
    OSizerange range = before.value(oldName);
    const OSizerange oldRange = range; // everything that follows the name uses the pre-rename range

    // Renaming a favorited range onto another favorite's name is refused — nothing
    // has been stored yet, so the dock list refresh reverts the edited cell.
    if (RangeRenameHitsFavorite(oldRange, newName))
    {
        RefreshSizeRangesDock();
        return;
    }

    range.name = newName; // definition and content are untouched
    QHash<QString, OSizerange> ranges = before;
    ranges.remove(oldName);
    ranges.insert(newName, range);

    // The favorite follows the range name, and so does everything else that named it — the
    // command carries both names, so an undo moves them back. See PropagateRangeRename.
    const QString activeAfter =
        m_activeGradedRangeName == oldName ? newName : m_activeGradedRangeName;
    PushGradedRangesCommand(before, ranges, m_activeGradedRangeName, activeAfter,
                            tr("Rename size range"), &oldRange, &range);
}
