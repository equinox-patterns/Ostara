/************************************************************************
 **
 **  @file   oexportmode.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — entry point and entry flow. Producing a generation,
 **          keeping it, and showing it live in oexportoutputs.cpp.
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
#include "mainwindow_helpers.h"
#include "core/ogradedsizeranges.h"
#include "core/oexportoutput.h"
#include "dialogs/oexportmodedialog.h"
#include "dialogs/ooutputsstale.h"
#include "widgets/DrawToolBar.h"
#include "widgets/ODockManager.h"
#include "widgets/TopBar.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QCoreApplication>
#include <QMessageBox>
#include <QStringList>

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ActionExport show export scene.
 * @param checked true - button checked.
 */
void MainWindow::ActionExport(bool checked)
{
    if (not checked)
    {
        ui->actionExport->setChecked(true);

        // Clicking the mode button while already in the mode follows the SAME rule as a
        // fresh entry: an empty list asks what to nest, a filled one warns or shows. A
        // nesting that failed — a bin too narrow, say — must not force the user out of
        // the mode and back in, and nothing is ever regenerated on its own.
        if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Export)
        {
            EnterExportFlow();
        }
        return;
    }

    if (not hasOpenPattern())
    {
        ui->actionExport->setChecked(false);
        QMessageBox::information(this, tr("Export mode"),
                                 tr("You can't use Export mode yet. Please, open a pattern."),
                                 QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    const Draw prevMode = VAbstractValApplication::VApp()->GetDrawMode();

    ArrowTool(true);

    // ArrowTool arms the rubber band, which is what the drawing modes want and this one does
    // not: a drag on this canvas means "move what is under the cursor", and a rubber band
    // would draw a selection rectangle over the plan on the first movement that misses.
    ui->view->AllowRubberBand(false);

    ui->actionDraw->setChecked(false);
    ui->actionDetails->setChecked(false);
    ui->actionGraded->setChecked(false);
    ui->actionLayout->setChecked(false);
    ui->actionExport->setChecked(true);

    // Switch to Export first (empty canvas, export chrome), so the entry dialog opens
    // ABOVE the export canvas — not over the mode the user clicked in. Same order as
    // Graded (mainwindow_graded.cpp, ActionGraded).
    qCDebug(vMainWindow, "Show export scene");
    m_dockManager->saveDockStateForMode(prevMode);

    // The export chrome is the Graded one: the centre of the TopBar is hidden.
    // Leaving Graded must be done explicitly — ActionGraded lives in the Graded
    // files and knows nothing about this mode.
    if (prevMode == Draw::Graded)
    {
        m_topBar->SetGradedChrome(false);
    }
    m_topBar->SetExportChrome(true);

    if (not switchScene(Draw::Export))
    {
        return;
    }

    SetEnableWidgets(true);
    m_dockManager->applyModeDocks(Draw::Export);
    // The work toolbar is not managed by the dock manager — make sure leaving Graded
    // (which hides it) never leaves Export without it.
    m_drawToolBar->show();

    m_statusLabel->setText(QString());

    // Leaving Draw tears widgets down with deleteLater(): the tab strip of the
    // {Groups, Messages} pair that the dock restore replaces, and the Draw tool groups
    // that DrawToolBar::BuildLayout() rebuilds. A deferred deletion is not performed
    // once a new event loop is entered — Qt, QObject::deleteLater(): "entering and
    // leaving a new event loop (e.g., by opening a modal dialog) will not perform the
    // deferred deletion; for the object to be deleted, the control must return to the
    // event loop from which deleteLater() was called" — and processEvents() does not
    // process DeferredDelete either. Left pending, both stay on screen as ghosts (two
    // tab titles over hidden docks, the Draw group names over an Export toolbar) for
    // the whole entry dialog and the whole nesting. Flush them here, while control is
    // still on the loop that posted them — and before EnterExportFlow(), which opens
    // every modal this mode has.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // What to nest, on which bin, and whether the listed generations still match the
    // pattern: all of it is decided here, on the empty export canvas.
    if (not EnterExportFlow())
    {
        // Cancelling gives the previous mode back — the mode was already switched.
        ui->actionExport->setChecked(false);

        switch (prevMode)
        {
            case Draw::Calculation:
                ActionDraw(true);
                break;
            case Draw::Graded:
                ActionGraded(true);
                break;
            case Draw::Layout:
                ActionLayout(true);
                break;
            default:
                ActionDetails(true);
                break;
        }
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief EnterExportFlow decide what entering the Export mode does, and do it.
 *
 * An empty list asks what to nest — there is nothing to show yet. A filled one shows the
 * generation that was open, after warning the user when the pattern has moved on since it
 * was made: a modification does not propagate to a generation, and she is the one who
 * decides whether to regenerate.
 *
 * Nothing is ever relaunched here.
 *
 * @return false only when the generation dialog was cancelled on a fresh entry — the
 *         caller then gives the previous mode back.
 */
auto MainWindow::EnterExportFlow() -> bool
{
    // The dock and the rule below read the SAME list: the generations of the current
    // (table, axis) context. One made under another measurements table is in the session
    // but not in the list — the same rule the Size Ranges dock lists a range by.
    RefreshOutputsDock();

    if (VisibleExportOutputIds().isEmpty())
    {
        // The auto-open on an empty filtered list and the dock's "Add an output" button
        // share the same nesting path — but the cancel semantics differ (a fresh entry
        // returns false so the caller can replay the previous mode; a dock click is a
        // no-op inside the mode). See AddExportOutput() / AddExportOutputFromEntry().
        return AddExportOutputFromEntry();
    }

    if (m_exportOutputs.IsStale() && OOutputsStale::ShouldWarn())
    {
        OOutputsStale::Show(this);
    }

    if (const OExportOutput *output = ActiveVisibleOutput())
    {
        ShowExportOutput(*output, /*zoomFit*/ true);
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddExportOutput open the generation dialog from the dock's "Add an output" button:
 *        a cancellation is a no-op (the mode stays, the dock can be clicked again).
 *
 * @return always true — a failed nesting keeps the state as it was and the reason has
 *         been shown to the user.
 */
auto MainWindow::AddExportOutput() -> bool
{
    return AddExportOutputImpl(/*allowCancel*/ true);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddExportOutputFromEntry open the generation dialog on a fresh Export-mode entry:
 *        a cancellation signals the caller to replay the previous mode.
 *
 * @return false only when the dialog was cancelled; a failed nesting keeps the state as
 *         it was and returns true.
 */
auto MainWindow::AddExportOutputFromEntry() -> bool
{
    return AddExportOutputImpl(/*allowCancel*/ false);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddExportOutputImpl shared body of AddExportOutput() / AddExportOutputFromEntry().
 *
 * The only difference between the two entry points is the cancellation contract — the
 * nesting path is the same. @p allowCancel true means "cancellation is a no-op" (dock
 * button); false means "cancellation means the user gave up on the entry, replay the
 * previous mode".
 *
 * That contract is about a dialog refused with nothing done. A dialog closed after its
 * list to generate has fired is refused as well, but its generations are in the dock —
 * which is why the outcome is read back from the dialog instead of assumed from the
 * refusal.
 */
auto MainWindow::AddExportOutputImpl(bool allowCancel) -> bool
{
    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);

    // The names the dock lists, filtered exactly as the dock filters: a row the filter
    // hides is not on screen, and a name the user cannot see is not one she can collide
    // with. Two runs of the same settings over the same range are legitimate — the same
    // range asked for twice is not a mistake — so a name already taken is suffixed rather
    // than refused, which is what the entry dialog used to do.
    //
    // Read before the dialog rather than after it: the naming happens in the dialog now —
    // a line of the list is named the moment it is added, not when it runs.
    QStringList takenNames;
    const QVector<quint32> listed = VisibleExportOutputIds();
    takenNames.reserve(listed.size());
    for (quint32 id : listed)
    {
        if (const OExportOutput *existing = m_exportOutputs.Find(id))
        {
            takenNames.append(existing->name);
        }
    }

    OExportModeDialog dialog(pattern, m_m, context, takenNames, this);

    // The list to generate fires its own runs, and the dialog cannot nest — it holds no
    // document, no pattern and no scene. Handing it the one operation it lacks keeps the
    // dependency pointing the way it already does: the dialog asks, the window runs.
    dialog.SetRunGenerator([this, &context](const OExportRunRequest &request, const QString &name)
                           { return GenerateNamedExport(request, name, context); });

    if (dialog.exec() != QDialog::Accepted)
    {
        // The list may have fired a batch before the dialog was closed: the refusal is then
        // the user leaving a dialog whose work is done, not a user giving up on the entry
        // — and replaying the previous mode over those generations would be wrong.
        return dialog.HasGenerated() || not allowCancel;
    }

    // What the Print tab describes, generated once. The name is settled by the dialog: it
    // is the only side that knows the axis labels a single size is named by, and the only
    // one that can see the list a name must not collide with.
    //
    // A failure has already been reported and the mode stays. The dialog can be reopened
    // by clicking the mode button or the dock button again — the list is still in whatever
    // state it was in before this run.
    GenerateNamedExport(dialog.CurrentRequest(), dialog.CurrentName(), context);
    return true;
}
