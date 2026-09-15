/************************************************************************
 **
 **  @file   oexportoutputs.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/09/2026
 **
 **  @brief  Export mode — producing a generation, keeping it, and showing it.
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
#include "dialogs/owaitdialog.h"
#include "oexportalignment.h"
#include "oexportcanvas.h"
#include "oexportpdf.h"

#include "dialogs/oexportpdfdialog.h"

#include <QDir>
#include <QStandardPaths>
#include "oexportgeometry.h"
#include "oexportnesting.h"
#include "widgets/ODockManager.h"
#include "widgets/owidgetoutputs.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QMessageBox>
#include <QThreadPool>
#include <QtConcurrent>

#include <memory>
#include <optional>
#include <utility>

namespace
{
/// One nesting at a time, and never on the global pool: it is shared with the rest
/// of the application, and the documented trap of this codebase is a nested
/// submission that deadlocks the pool.
constexpr int kExportNestingThreads = 1;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RunExportNesting resolve the range, nest it on the imposed width, and hand the
 *        result back. It draws nothing: the caller files the generation and shows it.
 *
 * @return the generation, or nothing when the run failed — the reason has already been
 *         shown to the user.
 */
auto MainWindow::RunExportNesting(const OExportRunRequest &request, const OGradedSizeRanges::Context &context)
    -> std::optional<OExportOutput>
{
    // Every length of the request is stored in millimetres and the engine consumes pixels:
    // the one conversion of the run happens here, once. The unit the user typed in plays no
    // part in it — it says how the figures are READ, not what they mean, which is why the
    // same run gives the same plan whatever unit it was entered in.
    OExportNestingParams params;
    params.binWidthPx = ToPixel(request.documentWidthMm, Unit::Mm);
    params.marginPx = ToPixel(request.marginMm, Unit::Mm);
    params.gapPx = ToPixel(request.gapMm, Unit::Mm);

    // The engine has no abort hook: this dialog is shown while it works and taken down
    // after. No Cancel button, so nothing on screen promises an interruption that
    // cannot happen.
    std::unique_ptr<OWaitDialog> waitDialog(new OWaitDialog(tr("Nesting. Please, wait."), this));
    waitDialog->show();
    QCoreApplication::processEvents();

    // Resolving the geometry re-parses the document once per size of the range, so it
    // stays on the GUI thread: VLayoutPiece::Create reads the application settings, and
    // the container it reads is the one the scenes are built from.
    OExportGeometry geometry(pattern, doc, m_m);

    // The size the document was found on — the range traversal puts it back there.
    const qreal savedA = m_currentDimensionA;
    const qreal savedB = m_currentDimensionB;
    const qreal savedC = m_currentDimensionC;

    // The only way the export puts the document on a size: the one Details uses for its
    // own drop-downs. Reading the measurements here instead — which is what this code
    // used to do — leaves the pieces on the previous size, and every layer of the range
    // then comes back with the same geometry.
    //
    // The document itself is deliberately left alone: the live combination is what puts
    // it on a size, and GradationChanged() reads that — the Graded path resolves a whole
    // range the same way, without writing anything. Writing it here added a `dimensionX`
    // attribute to documents that do not carry one, and the schema refuses any value
    // below 1 (dimesionValue): such a document could no longer be saved at all.
    const auto applySize = [this](qreal a, qreal b, qreal c)
    {
        m_currentDimensionA = a;
        m_currentDimensionB = b;
        m_currentDimensionC = c;
        GradationChanged();
    };

    QVector<OExportSizeLayer> layers;

    // A table without axes — an .otu, or a .vit — describes a single size. There is no
    // size to put the document on, and no reason to try: the Layout mode, which has
    // nested a single size since long before this one existed, never touches the document
    // either. Its measurements resolve to what they already hold whatever the bases say —
    // an .otu's shifts are zero and its base is zero — so the size path would re-read the
    // whole table and re-parse the pattern only to arrive where it started.
    const bool hasAxes = !m_m.isNull() && !m_m->Dimensions().isEmpty();

    if (request.singleSize)
    {
        if (!hasAxes)
        {
            // Already on its only size: the geometry is read as it stands.
            layers = geometry.CollectCurrentSize(tr("Single size"));
        }
        else
        {
            // The user asked for this size: the document stays on it.
            //
            // An axis the measurement table does not carry has no size to offer, and the
            // dialog says so with -1 — a value shaped like a size without being one, handed
            // to the pieces as if it were. The document's own value stands instead, and the
            // axis's first size when the document has none either: 1 is the only base the
            // schema accepts as a floor. The range path below guards its bases the same way.
            const auto baseOr = [](qreal fromDialog, qreal fromDocument)
            { return fromDialog > 0.0 ? fromDialog : (fromDocument > 0.0 ? fromDocument : 1.0); };

            const qreal baseOfA = baseOr(request.baseA, savedA);
            const qreal baseOfB = baseOr(request.baseB, savedB);
            const qreal baseOfC = baseOr(request.baseC, savedC);

            applySize(baseOfA, baseOfB, baseOfC);
            layers = geometry.CollectCurrentSize(
                QStringLiteral("A=%1 B=%2 C=%3").arg(baseOfA).arg(baseOfB).arg(baseOfC));
        }
    }
    else
    {
        const QVector<OExportGeometry::OExportSizeRef> sizes = geometry.RangeSizes(request.rangeName);

        // The range pins the axes it does not grade, and the pieces must be resolved with
        // those values. The document's own ones send an axis it does not carry back to the
        // measurement file's base, and every piece built from a formula — which is all of
        // them — then comes out at a geometry the range does not name. The graded capture
        // feeds its pieces this exact way. An axis the range pins nothing at keeps the
        // document's value.
        const OExportGeometry::OExportFixedBases fixed = geometry.RangeFixedBases(request.rangeName);
        const qreal fixedA = fixed.a > 0.0 ? fixed.a : savedA;
        const qreal fixedB = fixed.b > 0.0 ? fixed.b : savedB;
        const qreal fixedC = fixed.c > 0.0 ? fixed.c : savedC;

        for (const OExportGeometry::OExportSizeRef &size : sizes)
        {
            switch (size.axisIndex)
            {
                case 0:
                    applySize(size.base, fixedB, fixedC);
                    break;
                case 1:
                    applySize(fixedA, size.base, fixedC);
                    break;
                default:
                    applySize(fixedA, fixedB, size.base);
                    break;
            }

            layers += geometry.CollectCurrentSize(size.label);
        }

        // The document was only borrowed to walk the range: put it back where it was.
        applySize(savedA, savedB, savedC);
    }

    // Each size sits where the model puts it, and a piece pinned at its fold does not
    // drift like a piece growing from its centre. Unioning them into one footprint only
    // means something once they line up — which is what Graded does before it draws,
    // and what the nesting needs for the same reason.
    OExportAlignment::Apply(layers, pattern);

    int pieceCount = 0;
    for (const OExportSizeLayer &layer : layers)
    {
        pieceCount += layer.pieces.size();
    }

    if (pieceCount == 0)
    {
        waitDialog.reset();
        QMessageBox::warning(this, tr("Export mode"),
                             tr("This size range carries no piece to nest.") + QStringLiteral("\n\n")
                                 + geometry.LastDiagnostic(),
                             QMessageBox::Ok, QMessageBox::Ok);
        return std::nullopt;
    }

    // The annealing is pure computation over its inputs — it runs off the GUI thread.
    QThreadPool pool;
    pool.setMaxThreadCount(kExportNestingThreads);

    QFuture<OExportNestingResult> future =
        QtConcurrent::run(&pool, &OExportNesting::Run, layers, params);

    QFutureWatcher<OExportNestingResult> watcher;
    QEventLoop loop;
    connect(&watcher, &QFutureWatcher<OExportNestingResult>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(future);
    loop.exec();

    const OExportNestingResult result = future.result();
    waitDialog.reset();

    if (!result.ok)
    {
        QMessageBox::warning(this, tr("Export mode"), result.error, QMessageBox::Ok, QMessageBox::Ok);
        return std::nullopt;
    }

    // The range is the authority on where the decor of the reference layer goes: it holds
    // an override for every owner the user positioned in Graded, and the mode reads it
    // without ever writing to it. A single-size run names no range — there is nothing to
    // read, and every piece keeps the geometry it carries.
    //
    // The generation takes a COPY of it: a generation is frozen, so editing or deleting
    // the live range must not move what was already nested.
    OSizerange exportRange;
    const OSizerange *range = nullptr;
    if (const QHash<QString, OSizerange> *ranges = pattern->SizeRanges(); ranges != nullptr)
    {
        const auto it = ranges->constFind(request.rangeName);
        if (it != ranges->cend())
        {
            exportRange = it.value();
            range = &exportRange;
        }
    }

    OExportOutput output;
    // Left nameless on purpose: the name comes from the entry dialog, which this function
    // never sees, and the caller sets it before filing the generation. Nothing downstream
    // reads it here.
    output.name.clear();
    output.measurementCanonical = context.canonical;
    output.sizeAxis = context.axis;
    output.range = exportRange;
    output.hasRange = (range != nullptr);
    output.rangeName = request.rangeName;
    output.documentWidthMm = request.documentWidthMm;

    // What this run was asked for. Kept so the run can be asked again — the Refresh entry
    // replays this exact request, and these values are the part of it nothing else records.
    output.singleSize = request.singleSize;
    output.baseA = request.baseA;
    output.baseB = request.baseB;
    output.baseC = request.baseC;
    output.marginMm = request.marginMm;
    output.gapMm = request.gapMm;
    output.unit = request.unit;
    output.printModelToken = request.printModelToken;
    output.columns = request.columns;
    output.layers = std::move(layers);
    output.result = result;
    return output;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GenerateNamedExport nest a run, file it under its name, and show it.
 *
 * The one way a generation enters the dock: the entry dialog's own Generate button and
 * every line of its list to generate come through here, so that a run asked for one by one
 * and a run asked for as part of a batch are the same run.
 */
auto MainWindow::GenerateNamedExport(const OExportRunRequest &request, const QString &name,
                                     const OGradedSizeRanges::Context &context) -> bool
{
    std::optional<OExportOutput> output = RunExportNesting(request, context);
    if (not output.has_value())
    {
        // The failure has already been reported. Nothing is filed and nothing is taken
        // away: the mode stays, and a line that failed keeps its line in the list.
        return false;
    }

    // As given, never uniquified here: the name was reserved where it was derived — against
    // the generations the dock lists and against the lines of the list to generate — and it
    // is the name the user read on the line she asked for. A second uniqueness pass could
    // only rename the generation away from what she agreed to.
    output->name = name;

    const quint32 id = m_exportOutputs.Append(std::move(*output));

    // Raised AFTER the run, never inside it: even if resolving the geometry emitted
    // patternChanged, the state that comes out of a successful generation is "up to
    // date" — otherwise the warning would greet the user on every re-entry, and
    // "nothing changed, show the last generation" would never be true twice.
    m_exportOutputs.SetStale(false);

    RefreshOutputsDock();

    if (const OExportOutput *stored = m_exportOutputs.Find(id))
    {
        ShowExportOutput(*stored, /*zoomFit*/ true);
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RenameExportOutput write the new name from the dock into the store and refresh.
 *
 * The dock already rejected empty and conflicting names before emitting renameRequested —
 * this is the write-through, no validation. The id may be unknown (the store lost the row
 * to a deletion between emit and slot) — in that case the dock is refreshed so its row
 * disappears, and the canvas stays on whatever was shown.
 */
void MainWindow::RenameExportOutput(quint32 id, const QString &newName)
{
    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr)
    {
        // The row is gone: a deletion landed between the dock's emit and our slot. Re-pull
        // so the dock stops showing a ghost, and the canvas is left alone — the active one
        // may already point at a real row.
        RefreshOutputsDock();
        return;
    }

    if (newName == existing->name)
    {
        // Defensive: the dock's OnNameEdited trims and compares before emitting, but a
        // trimmed name that happens to equal the stored one is a no-op — still refresh so
        // any concurrent state (active id, visibility filter) reads through.
        RefreshOutputsDock();
        return;
    }

    m_exportOutputs.SetName(id, newName);
    RefreshOutputsDock();
}

/**
 * @brief SetActiveExportOutput show the generation @p id in the canvas.
 *
 * This is the whole point of keeping a generation: the click recomputes nothing. No
 * nesting, no geometry resolution — the canvas is handed what the generation already
 * carries, exactly as on the first display.
 *
 * The result is framed like the first display, because each generation is a different
 * bin: keeping the previous zoom would leave a larger result half off-screen. The
 * already-active row is left alone, so re-clicking it cannot throw away a zoom the user
 * set herself.
 */
void MainWindow::SetActiveExportOutput(quint32 id)
{
    const OExportOutput *output = m_exportOutputs.Find(id);
    if (output == nullptr || m_exportOutputs.ActiveId() == id)
    {
        return;
    }

    m_exportOutputs.SetActiveId(id);
    ShowExportOutput(*output, /*zoomFit*/ true);
    RefreshOutputsDock();
}

/**
 * @brief RefreshExportOutput re-run the nesting of the generation @p id, and keep the row.
 *
 * This is the entry the staleness warning points at (§3.1 of the spec): the user decides
 * *whether* to regenerate and *which* one, and nothing here happens on its own. The run is
 * replayed with what the generation recorded when it was made — a generation carries what
 * it takes to reproduce itself. The context, though, is re-resolved rather than replayed:
 * the row is listed because it matches the current one, so the current one is the right
 * one.
 *
 * The identity and the name survive — the result is new, the row is not. The refreshed
 * generation is also the one that gets shown, in every case: the run takes minutes, and
 * leaving the canvas on another result would make the click look like it did nothing.
 *
 * Nothing here is undoable, like the rest of the dock: a generation is session-scoped and
 * never part of the document, so it has no place on the pattern's undo stack.
 */
void MainWindow::RefreshExportOutput(quint32 id)
{
    // The fields are read into the call before the run starts, so the pointer going stale
    // during it — the run shows a dialog and pumps events — is not a hazard.
    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr)
    {
        return; // the row is gone; there is nothing to regenerate
    }

    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);

    std::optional<OExportOutput> output = RunExportNesting(RequestOf(*existing), context);
    if (not output.has_value())
    {
        return; // the failure has already been reported, and the old result is still there
    }

    m_exportOutputs.Replace(id, std::move(*output));

    // Same rule as a fresh generation: what comes out of a successful run is up to date.
    m_exportOutputs.SetStale(false);

    m_exportOutputs.SetActiveId(id);
    RefreshOutputsDock();

    if (const OExportOutput *refreshed = m_exportOutputs.Find(id))
    {
        ShowExportOutput(*refreshed, /*zoomFit*/ true);
    }
}

/**
 * @brief DeleteExportOutput drop the generation @p id, once the user has confirmed.
 *
 * A generation is session-scoped — it is never written to the .ostara (§6.1 of the spec) —
 * so this removes nothing from the document and **Undo cannot bring it back**. That is not
 * a shortcoming to hide: the confirmation says it in as many words, because the user of
 * this mode will reach for Ctrl+Z and must not be surprised.
 *
 * The guard lives here rather than in the dock so that every caller is covered — the
 * confirmation is part of what deleting means, not a property of the button that happens
 * to call it today.
 */
void MainWindow::DeleteExportOutput(quint32 id)
{
    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr)
    {
        return;
    }

    const QString name = existing->name;

    if (QMessageBox::question(this, tr("Export mode"),
                              tr("Are you sure you want to delete \"%1\"?\n\n"
                                 "This action cannot be undone.")
                                  .arg(name),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
        != QMessageBox::Yes)
    {
        return; // the row stays, and the canvas is untouched
    }

    m_exportOutputs.Remove(id);

    // The canvas follows the list. ActiveVisibleOutput() answers exactly this question —
    // the active generation when the filter still lists it, otherwise the last one it
    // keeps — so the deletion lands on the rule the mode already uses instead of a second
    // one written here.
    if (const OExportOutput *next = ActiveVisibleOutput())
    {
        m_exportOutputs.SetActiveId(next->id);
        ShowExportOutput(*next, /*zoomFit*/ true);
    }
    else
    {
        // Nothing left to show. The scene is emptied the way a pattern load empties it, and
        // the scene rect is recomputed — otherwise the view keeps framing a result that is
        // no longer there. The status label said the size of what was displayed, so it goes
        // with it.
        m_sceneExport->clear();
        VMainGraphicsView::NewSceneRect(m_sceneExport, ui->view);
        m_statusLabel->clear();
    }

    RefreshOutputsDock();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportOutputToPdf(quint32 id)
{
    const OExportOutput *existing = m_exportOutputs.Find(id);
    if (existing == nullptr)
    {
        return;
    }

    // Where it opens: the range's own export folder when it has one — this plan's other
    // files are already there — and Documents when it has not, or when it has gone.
    QString directory = existing->hasRange ? existing->range.exportPath : QString();
    if (directory.isEmpty() || !QDir(directory).exists())
    {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    OExportPdfDialog dialog(existing->name, directory, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    if (!OExportPdf::Write(*existing, dialog.FullPath(), pattern, doc))
    {
        QMessageBox::warning(this, tr("Export mode"),
                             tr("\"%1\" could not be exported.").arg(existing->name));
        return;
    }

    QMessageBox::information(this, tr("Export mode"),
                             tr("\"%1\" was exported to\n%2").arg(existing->name, dialog.FullPath()));
}

/**
 * @brief ShowExportOutput draw a kept generation in the Export scene.
 *
 * Recomputes nothing — no nesting, and no geometry resolution: it hands the canvas what
 * the generation carries. That is the whole point of keeping one.
 *
 * The decor, though, is still resolved against the live pattern and document
 * (OExportCanvas::Build reads them for the grainlines, the groups, the information block
 * and the per-size trace styles). Placements and geometry are frozen; the decor follows.
 * Whatever is to be frozen must be owned by the generation first.
 */
void MainWindow::ShowExportOutput(const OExportOutput &output, bool zoomFit)
{
    // A click that arrives after the mode was left must not clear another mode's scene.
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Export)
    {
        return;
    }

    OExportCanvas::Build(m_sceneExport, output.layers, output.result,
                         output.hasRange ? &output.range : nullptr, pattern, doc);

    if (zoomFit)
    {
        // Frame the whole nesting, with air around it: a plain "Zoom fit best" would run the
        // plan against all four edges of the viewport, and a plan is read as a whole — its
        // border is not what the user came to look at. The margin is one ZoomOut step past
        // the fit, which is what the mode wants to open on every time.
        ui->view->ZoomFitBestFramed();
    }

    // The status label belongs to what IS shown, not to the run that produced it — and it
    // is written in the unit the generation was asked in, not the pattern's: the figures
    // the user just read in the dialog are the ones that have to come back here.
    const Unit unit = output.unit;
    const double width = FromPixel(output.result.binWidthPx, unit);
    const double height = FromPixel(output.result.usedHeightPx, unit);
    m_statusLabel->setText(tr("%1 × %2 %3").arg(QString::number(width, 'f', 1),
                                                 QString::number(height, 'f', 1),
                                                 UnitsToStr(unit, true)));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VisibleExportOutputIds the generations of the current (table, axis) context.
 *
 * A generation made under another measurements table is not listed — the same rule the
 * Size Ranges dock lists a range by, asked of the same foyer rather than copied.
 */
auto MainWindow::VisibleExportOutputIds() const -> QVector<quint32>
{
    const OGradedSizeRanges::Context context = OGradedSizeRanges::CurrentContext(doc);

    QVector<quint32> ids;
    const QVector<OExportOutput> &outputs = m_exportOutputs.Outputs();
    ids.reserve(outputs.size());
    for (const OExportOutput &output : outputs)
    {
        if (OGradedSizeRanges::MatchesContext(output.measurementCanonical, output.sizeAxis, context))
        {
            ids.append(output.id);
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ActiveVisibleOutput the generation to show: the active one when the filter lists
 *        it, otherwise the last one the filter keeps.
 *
 * The filter can hide the active generation — the measurements table changed under the
 * session — and showing a row the list does not carry would be an invisible selection.
 */
auto MainWindow::ActiveVisibleOutput() const -> const OExportOutput *
{
    const QVector<quint32> visible = VisibleExportOutputIds();
    if (visible.isEmpty())
    {
        return nullptr;
    }

    if (const OExportOutput *active = m_exportOutputs.Find(m_exportOutputs.ActiveId());
        active != nullptr && visible.contains(active->id))
    {
        return active;
    }

    return m_exportOutputs.Find(visible.constLast());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RefreshOutputsDock push the filtered rows to the Outputs dock.
 *
 * The dock never sees the store: the rule lives here, with the only owner of the current
 * context, and the dock only draws what it is handed.
 */
void MainWindow::RefreshOutputsDock()
{
    OWidgetOutputs *widget = m_dockManager != nullptr ? m_dockManager->outputsWidget() : nullptr;
    if (widget == nullptr)
    {
        return;
    }

    QVector<OExportOutputRow> rows;
    const QVector<quint32> visible = VisibleExportOutputIds();
    rows.reserve(visible.size());
    for (quint32 id : visible)
    {
        if (const OExportOutput *output = m_exportOutputs.Find(id))
        {
            rows.append(OExportOutputRow{output->id, output->name});
        }
    }

    widget->SetOutputs(rows);
    widget->UpdateList(m_exportOutputs.ActiveId());
}
