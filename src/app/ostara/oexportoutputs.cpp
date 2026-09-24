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
#include "core/oexportcuttingguard.h"
#include "core/oexportoutput.h"
#include "core/oexporttemplatestorage.h"
#include "dialogs/oexportprintmodel.h"
#include "dialogs/owaitdialog.h"
#include "oexportalignment.h"
#include "oexportartboards.h"
#include "oexportcanvas.h"
#include "oexportcoverpage.h"
#include "oexportpdf.h"
#include "oexportplanimage.h"
#include "oexportplanimagewriter.h"
#include "oexportsheets.h"

#include "dialogs/oexportimagedialog.h"
#include "dialogs/oexportimagelistdialog.h"
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
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QMessageBox>
#include <QScopeGuard>
#include <QStringList>
#include <QThreadPool>
#include <QtConcurrent>

#include <atomic>
#include <memory>
#include <optional>
#include <utility>

namespace
{
/// One nesting at a time, and never on the global pool: it is shared with the rest
/// of the application, and the documented trap of this codebase is a nested
/// submission that deadlocks the pool.
constexpr int kExportNestingThreads = 1;

/// @brief A duration said out loud: "2 minutes and 35 seconds".
///
/// Voiced rather than summed — "155 seconds" is the same figure and not the same sentence — and
/// each unit takes its plural from the count in front of it. Qt's %n would ask a translation for
/// that plural, and these strings carry none: the source has to read right on its own, so the two
/// forms are chosen here and only the bare words are handed to tr(). They are handed to
/// MainWindow's context rather than a local one, so that lupdate files them beside the label they
/// end up in.
///
/// A part that is zero is left out, because nobody says "2 minutes and 0 second", and a run under
/// a minute is seconds on their own: "0 minutes and 45 seconds" is a figure the reader has to take
/// apart before it means anything.
auto GenerationTimeText(qint64 milliseconds) -> QString
{
    const qint64 totalSeconds = milliseconds / 1000;
    const qint64 hours = totalSeconds / 3600;
    const qint64 minutes = (totalSeconds % 3600) / 60;
    const qint64 seconds = totalSeconds % 60;

    const auto counted = [](qint64 count, const QString &unit) {
        return QStringLiteral("%1 %2").arg(count).arg(count == 1 ? unit : unit + QLatin1Char('s'));
    };

    QStringList parts;
    if (hours > 0)
    {
        parts << counted(hours, MainWindow::tr("hour"));
    }
    if (minutes > 0)
    {
        parts << counted(minutes, MainWindow::tr("minute"));
    }
    if (seconds > 0)
    {
        parts << counted(seconds, MainWindow::tr("second"));
    }

    // Only a clock that has not ticked yet reaches this, but the label must not be left ending on
    // a separator because of it.
    if (parts.isEmpty())
    {
        parts << counted(0, MainWindow::tr("second"));
    }

    // The last part is the one spoken with "and": "1 hour, 5 minutes and 12 seconds".
    const QString last = parts.takeLast();
    return parts.isEmpty() ? last
                           : MainWindow::tr("%1 and %2").arg(parts.join(QStringLiteral(", ")), last);
}
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
    // Started before the run does anything, so that the figure reported covers the whole wait and
    // not only the engine: what the user spent is the time in front of the dialog.
    QElapsedTimer generationTimer;
    generationTimer.start();

    // Every length of the request is stored in millimetres and the engine consumes pixels:
    // the one conversion of the run happens here, once. The unit the user typed in plays no
    // part in it — it says how the figures are READ, not what they mean, which is why the
    // same run gives the same plan whatever unit it was entered in.
    // **The run's way out, declared before anything can read it.** Held by a shared pointer and not
    // by this window: the engine is handed a lambda that reads it and runs on a worker thread, and
    // the dialog that sets it is destroyed before the result is read. Created here, ahead of the
    // params that carry the question, so that nothing is handed over that does not yet exist.
    auto abandoned = std::make_shared<std::atomic<bool>>(false);

    OExportNestingParams params;
    // **Le tissu plié divise la laize par deux, et c'est ici que ça se dit** — le seul endroit où
    // une longueur de la requête devient une longueur du run. Le tissu plié en deux dans le sens de
    // la laize donne un bac de 70 pour une laize de 140, et tout le reste suit sans rien savoir :
    // le bac, le canvas, la page du PDF et l'image lisent tous `binWidthPx`.
    // La laize ENTRÉE, elle, ne bouge pas : c'est le tissu acheté, et le nom de la sortie le dit
    // encore (`Cut-140`).
    const qreal fabricWidthMm = request.foldedFabric ? request.documentWidthMm / 2.0
                                                     : request.documentWidthMm;
    params.binWidthPx = ToPixel(fabricWidthMm, Unit::Mm);
    params.marginPx = ToPixel(request.marginMm, Unit::Mm);
    params.gapPx = ToPixel(request.gapMm, Unit::Mm);
    // The grid follows the same rule as every other length: converted once, here.
    // A run that is not tiled carries a zero height — the roll and A0 — and the
    // engine then has no grid, so no adjustment pass.
    params.artboardCount    = request.columns;
    params.artboardHeightPx = ToPixel(request.frameHeightMm, Unit::Mm);

    // Et le tissu plié, qui ne se dit pas seulement ici : c'est le nesting qui en tire la rotation
    // d'une pièce au pli et son ancrage au bord. Voir OExportNestingParams::foldedFabric.
    params.foldedFabric = request.foldedFabric;

    // **The block the cover page reserves — and only when its format keeps it in a sheet.** The
    // seat is the format's own answer (OExportCoverPage), read from the token the run was asked
    // for: a roll reserves the block at its corner, A3 and Tabloid too, and the three formats
    // that give the page a sheet of its own reserve NOTHING here — their nesting is the one they
    // always had.
    //
    // Left empty for "No cover page" as well, which is the same test one step earlier: an empty
    // name is the single representation the whole feature reads.
    //
    // Converted here, like every other length, and from millimetres: the block is stated in
    // millimetres because that is the unit a format is stated in.
    if (!request.coverPageTemplateName.isEmpty())
    {
        // Already in pixels, and read with the RUN's margin: the block is the page's own rectangle,
        // off the plan's edges by the figure the user set — the same figure the Bin below is built
        // with, so the block and the nesting agree about where the paper's edges are.
        params.coverZonePx = OExportCoverPage::BlockPx(
            OExportCoverPage::SeatOf(FindPrintModel(request.printModelToken)),
            ToPixel(request.marginMm, Unit::Mm));
    }

    // Asked on the worker thread, so it reads an atomic and nothing else: no widget, no document,
    // nothing a thread that is not the GUI thread may touch.
    params.abandoned = [abandoned]() { return abandoned->load(); };

    // What separates the two runs. False leaves the engine turning every piece freely, which is
    // what a print run wants and what it has always done.
    params.cuttingPlan = request.cuttingPlan;
    params.oneWayLayout = request.oneWayLayout;

    // **Closing this dialog is how the run is given up.** The engine has no abort hook of its own,
    // so one is handed to it below: the cross sets a flag, the engine reads it between its
    // iterations, and the run stops where it stands. There is no Cancel button because there is
    // nothing left to cancel — what a cross means is exactly this, and it now means it.
    std::unique_ptr<OWaitDialog> waitDialog(new OWaitDialog(tr("Nesting. Please, wait."), this));

    // The cross on the dialog is the request; what the engine reads is the flag it was handed.
    connect(waitDialog.get(), &QDialog::rejected, this, [abandoned]() { abandoned->store(true); });

    waitDialog->show();
    QCoreApplication::processEvents();

    // Resolving the geometry re-parses the document once per size of the range, so it
    // stays on the GUI thread: VLayoutPiece::Create reads the application settings, and
    // the container it reads is the one the scenes are built from.
    OExportGeometry geometry(pattern, doc, m_m);
    geometry.SetCuttingPlan(request.cuttingPlan);
    // Et si le tissu est plié : c'est de là que la pièce au pli garde son pli au lieu d'être
    // dépliée, et qu'une pièce coupée deux fois n'est posée qu'une.
    geometry.SetFoldedFabric(request.foldedFabric);

    // A cutting plan decomposes the groups: it keeps the member the user chose for each and drops
    // the others, so a group is not unioned into a single footprint. Empty means every piece —
    // which is what a print run asks for, and what a pattern with no group asks for anyway.
    if (not request.groupMemberChoices.isEmpty())
    {
        const QVector<quint32> kept = OExportCuttingGuard::PiecesToCut(request.groupMemberChoices, pattern);
        geometry.SetPieceFilter(QSet<quint32>(kept.cbegin(), kept.cend()));
    }

    // ── Ce que la marche des tailles abîme, et qu'il faut remettre ─────────────────────────────
    //
    // Marcher les tailles re-parse le document, et un parseur qui réécrit une formule fait
    // `modified = true` puis `emit patternChanged(false)`. Ce signal est branché en direct sur
    // `MainWindow::PatternChangesWereSaved`, qui marque la fenêtre modifiée, la mise en page périmée
    // et les sorties périmées — **alors que l'utilisatrice n'a rien touché**. Elle lisait donc « tu as
    // modifié la pièce » après un simple run.
    //
    // Le mode Graded a la parade depuis toujours, et c'est elle qu'on reprend ici : un `QSignalBlocker`
    // sur le document pendant toute la traversée — le signal n'atteint jamais le slot, donc les trois
    // effets n'ont pas lieu — plus une sauvegarde de `IsModified()` pour la seule chose qui a
    // réellement changé. ⚠️ Restaurer le drapeau SANS le bloqueur ne suffirait pas : la fenêtre et les
    // drapeaux périmés auraient déjà été touchés.
    const qreal savedA = m_currentDimensionA;
    const qreal savedB = m_currentDimensionB;
    const qreal savedC = m_currentDimensionC;
    const bool wasModified = doc->IsModified();
    const bool wasWindowModified = isWindowModified();
    const bool wasLayoutStale = m_layoutSettings != nullptr && m_layoutSettings->IsLayoutStale();

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
    bool sizeMoved = false;
    const auto applySize = [this, &sizeMoved](qreal a, qreal b, qreal c)
    {
        m_currentDimensionA = a;
        m_currentDimensionB = b;
        m_currentDimensionC = c;
        GradationChanged();
        sizeMoved = true;
    };

    // The range is the authority on where the decor of the reference layer goes: it holds
    // an override for every owner the user positioned in Graded, and the mode reads it
    // without ever writing to it. A single-size run names no range — there is nothing to
    // read, and every piece keeps the geometry it carries.
    //
    // The generation takes a COPY of it: a generation is frozen, so editing or deleting
    // the live range must not move what was already nested.
    //
    // ⚠️ Resolved HERE, before the sizes are walked, and not where it used to be: the frozen decor
    // below is resolved on the FIRST layer, while the document is still on that layer's size, and it
    // needs the range at that moment.
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

    // ── The decor of the reference layer, frozen at generation ────────────────────────────────
    //
    // A piece's cartouche and annotations do not live in the piece: they live in the DOCUMENT, where
    // their position is re-materialised every time it is put on a size, and where a rotation is a
    // formula evaluated against the live variables. Drawing them from the document therefore draws
    // the size it is on at the moment of the DRAWING — outlines frozen, children alive, and the two
    // drift apart the moment a run generated at one size is looked at from another. They are resolved
    // ONCE, here, and travel in the output beside the frozen geometry.
    //
    // ⚠️ **Only the reference layer carries decor** — the first one; the canvas strips every other.
    // And the resolution must happen DURING the walk, on that first layer: past the walk the range
    // path has already put the document back on the size it was found on.
    // Le décor des GROUPES de la couche de référence, résolu au même moment et pour la même raison —
    // voir OExportGroupDecor. Rangé à part parce que le résultat du nesting n'existe pas encore : il
    // naîtra de l'appel au moteur, plus bas.
    QHash<quint32, OExportGroupDecor> groupDecor;

    const auto freezeDecor = [&](OExportSizeLayer &layer) {
        QHash<quint32, QVector<OExportPieceGeometry>> members;

        for (OExportPieceGeometry &piece : layer.pieces)
        {
            piece.decor = ResolvePieceDecor(piece, range, pattern, doc);

            // The grainline is the same story read from the same document — and it is frozen HERE,
            // in the same breath, because it is the same defect: it stayed invisible only for as long
            // as a single-size run left the document on the run's own size.
            piece.grainline = ResolvePieceGrainline(piece, range, request.unit, pattern,
                                                    request.cuttingPlan);

            if (piece.groupId != 0)
            {
                members[piece.groupId].append(piece);
            }
        }

        // Un groupe n'est résolu qu'UNE fois, sur l'ensemble de ses membres : c'est leur union qui
        // donne son cadre, et c'est d'elle que vient le repli d'un enfant jamais placé.
        for (auto it = members.cbegin(); it != members.cend(); ++it)
        {
            groupDecor.insert(it.key(),
                              ResolveGroupDecor(it.value(), it.key(), range, request.unit, pattern,
                                                doc, request.cuttingPlan));
        }
    };

    QVector<OExportSizeLayer> layers;

    // A table without axes — an .otu, or a .vit — describes a single size. There is no
    // size to put the document on, and no reason to try: the Layout mode, which has
    // nested a single size since long before this one existed, never touches the document
    // either. Its measurements resolve to what they already hold whatever the bases say —
    // an .otu's shifts are zero and its base is zero — so the size path would re-read the
    // whole table and re-parse the pattern only to arrive where it started.
    const bool hasAxes = !m_m.isNull() && !m_m->Dimensions().isEmpty();

    // ⚠️ The document's signals are cut for the whole walk. The parsers still run and still set
    // `modified` — nothing can stop that — but `patternChanged` never reaches
    // `PatternChangesWereSaved`, so the window, the layout and the outputs are never marked. That is
    // the bug. Restoring `IsModified()` alone would NOT have been enough: those three marks would
    // already have been made.
    //
    // `blockSignals` rather than a scoped blocker because a `QSignalBlocker` cannot be closed early:
    // it would have to live to the end of the function, and the nesting below — which talks to a
    // worker thread and drives a wait dialog — has no business running with the document mute.
    const bool wasSignalsBlocked = doc->signalsBlocked();
    doc->blockSignals(true);

    if (request.singleSize)
    {
        if (!hasAxes)
        {
            // Already on its only size: the geometry is read as it stands.
            layers = geometry.CollectCurrentSize(tr("Single size"));
            freezeDecor(layers.last());
        }
        else
        {
            // The document is put on the requested size for the run, and given back afterwards like
            // the range path — see the restore below.
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
            freezeDecor(layers.last());
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

            // The first size of the range is the reference layer — the only one that carries decor.
            // The document is on it right now; it will not be again after the walk.
            if (layers.size() == 1)
            {
                freezeDecor(layers.last());
            }
        }

        // The document was not put back here: the restore is shared with the single-size path, below,
        // because both borrow the document and both must give it back.
    }

    doc->blockSignals(wasSignalsBlocked);

    // ── Rendre le document tel qu'on l'a trouvé ────────────────────────────────────────────────
    //
    // The document was only borrowed to walk: put it back where it was found, **whatever path ran**.
    // The single-size path used to keep it — "the user asked for this size" — and that is what left
    // Details rendering the export's size, since the Details scene is built from the live combination
    // and its size drop-downs read it too. It no longer has to stay: the reference layer's decor is
    // frozen into the output, so nothing that gets displayed needs the document on the run's size.
    if (sizeMoved)
    {
        applySize(savedA, savedB, savedC);
    }

    doc->SetModified(wasModified);
    setWindowModified(wasWindowModified);
    if (m_layoutSettings != nullptr)
    {
        m_layoutSettings->SetLayoutStale(wasLayoutStale);
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

    if (result.abandoned)
    {
        // **Nothing is said about it.** The user closed the dialog to stop the run; a message
        // telling them it did not finish would be the application arguing with a decision that was
        // theirs to make. Nothing is filed, nothing is changed, and the mode stays where it is.
        return std::nullopt;
    }

    if (!result.ok)
    {
        QMessageBox::warning(this, tr("Export mode"), result.error, QMessageBox::Ok, QMessageBox::Ok);
        return std::nullopt;
    }

    // The range is resolved ABOVE, before the sizes are walked: the frozen decor needs it while the
    // document is still on the reference layer's size.

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
    output.frameHeightMm = request.frameHeightMm;
    output.marksTemplateName = request.marksTemplateName;
    output.coverPageTemplateName = request.coverPageTemplateName;
    output.pinnedAxes = request.pinnedAxes;
    output.groupMemberChoices = request.groupMemberChoices;
    output.cuttingPlan = request.cuttingPlan;
    output.oneWayLayout = request.oneWayLayout;
    output.foldedFabric = request.foldedFabric;
    output.layers = std::move(layers);
    output.result = result;

    // Le décor des groupes de la couche de référence, résolu pendant la marche des tailles. Il rejoint
    // le résultat ici, et pas ailleurs : c'est la seule chose que le canvas reçoit qui appartienne à
    // UNE génération — une pièce a sa place dans les couches, un groupe n'en a pas.
    output.result.groupDecor = groupDecor;
    output.generationMs = generationTimer.elapsed();
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
    // **The window must not be deleted while this runs.** A generation waits on a loop of its own —
    // the nesting engine has no abort hook, so that loop outlives the dialog announcing it — and a
    // deferred deletion is performed by whichever loop is running when control returns to one. A
    // close asked for here would therefore have been carried out by the run's own loop, deleting
    // this window while the run was still working through it, which is a method of it. The close is
    // remembered instead, and granted once the count is back to zero: see MainWindow::closeEvent.
    BeginWorkInFlight();
    const auto workGuard = qScopeGuard([this]() { EndWorkInFlight(); });

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
 * @brief Redraws the shown generation with what has just been edited under it.
 *
 * **TWO things are drawn under a plan, and this slot serves both**: the template of assembly marks,
 * saved from the editor, and the cover page, written from its dock. Neither moves a piece, so both
 * are handed the same way.
 *
 * The nesting is NOT re-run. Nothing about the plan changed — the decor is drawn under it — so the
 * generation is handed back to the canvas exactly as it was, the same redraw the manual correction
 * asks for. That keeps the zoom the user set and spares a run of several minutes for a frame that
 * moved by a point, or for a comma in a free text.
 *
 * Nothing happens when no generation is shown, which is the common case: the editor is a window of
 * its own, and a user may edit templates before nesting anything. And nothing happens in another
 * draw mode — ShowExportOutput guards that itself, because its scene is not the one on screen then.
 */
void MainWindow::RefreshExportDrawing()
{
    const OExportOutput *output = m_exportOutputs.Find(m_exportOutputs.ActiveId());
    if (output == nullptr)
    {
        return;
    }

    ShowExportOutput(*output, /*zoomFit*/ false);
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

    // The history is dropped, and it has to be: every command in it names a placement by its
    // index, and the run above has just written a whole new set of them. The indexes still
    // exist, they just no longer mean the same thing — an undo would move a piece to where a
    // DIFFERENT result had put it, silently, which is the one failure mode worse than losing
    // the history. A stack cannot be pruned command by command without breaking the order the
    // redo side depends on, so it goes entirely.
    m_exportCorrection->Reset();

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
        //
        // The correction is told before the clear, because this branch reaches neither of the
        // two places that would normally tell it: no ShowExportOutput runs, so no
        // OnSceneAboutToBeRebuilt, and clear() destroys the holder straight out from under it.
        // The id 0 is not the generation being deleted — it is "nothing is shown", which is
        // exactly the situation, and the correction drops the selection on that answer.
        if (m_exportCorrection != nullptr)
        {
            m_exportCorrection->OnSceneAboutToBeRebuilt(0);
        }

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

    // The very template the canvas draws with, resolved by the very same call — so the paper
    // cannot carry marks the screen did not show, nor miss ones it did.
    OExportTemplate marks;
    bool framed = false;
    const bool hasMarks = ExportMarksOf(*existing, marks, framed);

    // The very cover page the canvas draws with, resolved by the very same call against the live
    // document. Everything the page states comes from the RUN — the sizes of a range, the values
    // the frozen axes hold — so what is delivered is what the paper carries and not what the
    // template demonstrated; and what belongs to the pattern is re-read here, which is what makes
    // an edit need no new nesting. An empty one is a state and not an error: "No cover page" is
    // one of the answers the print widget offers.
    const OExportCoverPage::Drawing cover = OExportCoverPage::Of(*existing, doc);

    // The dialog's one choice, straight through: whether the layout is written as a layer the
    // reader can switch off. It opens on no, and no is the answer that cannot lose anything. The
    // cover page is not that choice and never was: it goes out of every layer whatever this says.
    if (!OExportPdf::Write(*existing, dialog.FullPath(), pattern, doc,
                           hasMarks ? &marks : nullptr, framed, dialog.AddLayoutLayer(),
                           cover.IsEmpty() ? nullptr : &cover))
    {
        QMessageBox::warning(this, tr("Export mode"),
                             tr("\"%1\" could not be exported.").arg(existing->name));
        return;
    }

    QMessageBox::information(this, tr("Export mode"),
                             tr("\"%1\" was exported to\n%2").arg(existing->name, dialog.FullPath()));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ExportActiveOutputToImage()
{
    // A click that arrives after the mode was left must not photograph another mode's scene —
    // the same guard, on the same thing, that ShowExportOutput opens on.
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Export)
    {
        return;
    }

    // What the canvas is showing, read by the rule the mode already has. The scope button speaks
    // for the plan on screen and for no other — a toolbar button has no row to point at.
    const OExportOutput *active = ActiveVisibleOutput();

    const QVector<quint32> ids = VisibleExportOutputIds();
    if (ids.isEmpty())
    {
        return; // nothing the dock lists, so nothing to offer and nothing to write
    }

    // Step 1 opens its size field on the plan of the generation on show — the only figure that is
    // not a guess. Without one, on the first the dock lists: the dialog still has to open on
    // something, and what it carries is a starting point for a field about to be changed.
    const OExportOutput *reference =
        active != nullptr ? active : m_exportOutputs.Find(ids.constFirst());
    if (reference == nullptr)
    {
        return;
    }

    const OExportPlanImage::Plan referencePlan =
        OExportPlanImage::PlanOf(ComputeExportDrawing(*reference).drawnRect);

    // The range's own export folder when it has one, Documents otherwise: the rule the PDF opens
    // on, so that the two exports of one generation land side by side.
    QString directory = reference->hasRange ? reference->range.exportPath : QString();
    if (directory.isEmpty() || !QDir(directory).exists())
    {
        directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    OExportImageDialog options(directory, referencePlan, active != nullptr, this);
    if (options.exec() != QDialog::Accepted)
    {
        return;
    }

    // Step 2: which generations, and under which names. One line for one generation is the same
    // question as one line for six, so it is the same dialog.
    QVector<OExportImageListDialog::Row> rows;
    if (options.Chosen() == OExportImageDialog::Scope::Active)
    {
        // The button that answers this is dead when nothing is shown, so the pointer is not
        // expected to be null — reading it anyway costs one line and cannot be a crash.
        if (active != nullptr)
        {
            rows.append({active->id, active->name});
        }
    }
    else
    {
        for (const quint32 id : ids)
        {
            if (const OExportOutput *output = m_exportOutputs.Find(id))
            {
                rows.append({output->id, output->name});
            }
        }
    }

    OExportImageListDialog list(rows, options.Format(), this);
    if (list.exec() != QDialog::Accepted)
    {
        return;
    }

    // The run. Every generation is drawn into **a scene of its own** — the canvas holds the one on
    // show and no other, so a batch cannot borrow it — and a scene built here carries no correction
    // holder: that one is mounted on the displayed scene alone, which is why no image can hold a
    // dashed frame and four handles meant for the hand.
    QStringList written;
    QStringList failed;

    for (const OExportImageListDialog::Row &row : list.Chosen())
    {
        const OExportOutput *output = m_exportOutputs.Find(row.id);
        if (output == nullptr)
        {
            continue;
        }

        const OExportDrawing drawing = ComputeExportDrawing(*output);
        const OExportPlanImage::Plan plan = OExportPlanImage::PlanOf(drawing.drawnRect);

        // An SVG's page IS a physical size, and PrintDPI is what expresses it; a PNG's pixels ARE
        // the size, so its resolution is the user's. One figure for the whole run — the size was
        // asked for once — and each generation's own proportions give it its other side.
        const OExportPlanImage::Target target = OExportPlanImage::SizeFor(
            plan, options.TargetMm(), options.Axis(),
            options.Format() == OExportPlanImageWriter::Format::Svg ? PrintDPI : options.Dpi());

        QGraphicsScene scene;
        DrawExportDrawing(&scene, *output, drawing);

        const QString path = QDir(options.Directory()).filePath(row.name);
        if (OExportPlanImageWriter::Write(path, &scene, plan, target, options.Format(), output->name))
        {
            written.append(row.name);
        }
        else
        {
            // A generation that cannot be written does not stop the others. Its line is said at the
            // end with the rest, which is the only place that knows which generation it was.
            failed.append(row.name);
        }
    }

    // ONE dialog for the whole run, whatever its length: a box per file would be a box the user
    // has to dismiss six times to learn one thing.
    QStringList lines;
    if (!written.isEmpty())
    {
        lines << tr("%n file(s) written to %1", "", static_cast<int>(written.size()))
                     .arg(options.Directory());
        lines << QString();
        lines << written;
    }

    if (failed.isEmpty())
    {
        QMessageBox::information(this, tr("Export mode"), lines.join(QLatin1Char('\n')));
        return;
    }

    if (!lines.isEmpty())
    {
        lines << QString();
    }
    lines << tr("%n file(s) could not be written:", "", static_cast<int>(failed.size()));
    lines << failed;

    QMessageBox::warning(this, tr("Export mode"), lines.join(QLatin1Char('\n')));
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::ExportMarksOf(const OExportOutput &output, OExportTemplate &marks, bool &framed) const
    -> bool
{
    // By NAME, and resolved afresh every time — never a copy taken at nesting time. A template
    // is a file the user edits, and a run that had copied its frame and its marks would go on
    // drawing what the file said the day it was nested, while the editor shows something else:
    // the drawing would then be lying about the very thing it is there to show.
    const bool hasMarks = OExportTemplateStorage::Resolve(output.marksTemplateName, &marks);

    // Whether the format carries a frame — the two framed formats, and them alone. Read off the
    // table rather than guessed from the geometry: the frame is a property of the model, and its
    // sheet happens to be the size the frame is.
    const OExportPrintModel *model = FindPrintModel(output.printModelToken);
    framed = model != nullptr && model->hasFrame;

    return hasMarks;
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::ComputeExportDrawing(const OExportOutput &output) const -> OExportDrawing
{
    OExportDrawing drawing;

    // How the plan is cut, when the run is tiled — computed from the generation alone, since
    // that is all a kept run has. An invalid grid means one continuous sheet, and the canvas
    // then draws its bin exactly as it always did.
    drawing.artboards = OExportArtboards::Compute(
        output.result.binWidthPx, output.result.usedHeightPx, output.frameHeightMm, output.columns);

    // **The cover page is resolved here, and nowhere else on this path.** The template by name, the
    // document read live, and the values filled FROM THE RUN (§6) — so a page the user edits is the
    // one redrawn, without a nesting being run. The canvas is handed the result: it places and it
    // paints, and it looks nothing up.
    drawing.cover = OExportCoverPage::Of(output, doc);

    // The block, when the format keeps the page in a sheet — the SAME figure the canvas will hand
    // to its own Compute, read from the same rule. The extra sheet is not passed: it is not a cell.
    const QRectF coverReserved =
        (drawing.cover.seat == OExportCoverPage::Seat::InSheet && drawing.artboards.valid)
            ? OExportCoverPage::PaperPx(drawing.cover.seat, drawing.cover.marginPx,
                                        drawing.artboards.planRect, drawing.artboards.cellWidthPx,
                                        drawing.artboards.cellHeightPx)
            : QRectF();

    drawing.sheets =
        OExportSheets::Compute(drawing.artboards, output.result.placements, coverReserved);

    // What the canvas is about to draw, computed ONCE. Three readers want it — the rectangle a
    // hand may not leave, the image an export writes, and the figures of the status label — and
    // three readings of the same thing are three answers waiting to differ.
    drawing.drawnRect =
        drawing.artboards.valid
            ? drawing.artboards.planRect
            : QRectF(0.0, 0.0, output.result.binWidthPx, output.result.usedHeightPx);

    return drawing;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::DrawExportDrawing(QGraphicsScene *scene, const OExportOutput &output,
                                   const OExportDrawing &drawing) const
{
    // The template of assembly marks, resolved here and now rather than stored in the run: the
    // file behind the name can be edited, renamed or deleted between two rebuilds, and what is
    // drawn has to be what the file says today. A name that resolves to nothing falls back to the
    // shipped template, and a library with neither draws no marks at all.
    OExportTemplate marks;
    bool framed = false;
    const bool hasMarks = ExportMarksOf(output, marks, framed);

    OExportCanvas::Build(scene, output.layers, output.result,
                         output.hasRange ? &output.range : nullptr, pattern, doc, nullptr,
                         drawing.artboards, hasMarks ? &marks : nullptr, framed, &drawing.cover,
                         {output.cuttingPlan, output.foldedFabric});
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

    // The figures of the drawing and the drawing itself are two calls, so that the correction can
    // slip between them: it has to be told the scene is ABOUT to be rebuilt while the item holding
    // its mouse grab still exists. Both come from the one place that decides what is drawn.
    const OExportDrawing drawing = ComputeExportDrawing(output);

    if (m_exportCorrection != nullptr)
    {
        // The rectangle a hand may not leave is the one the canvas is about to draw: the
        // sheets when the run is tiled — the dead band of the last row included, since it is
        // paper the nesting already paid for — and the bin otherwise. Read from the same grid
        // Build gets, never from a second computation of the same thing.
        m_exportCorrection->SetPlan(drawing.drawnRect);

        // Before the rebuild, which empties the scene: a gesture in flight has to be abandoned
        // while the item holding the mouse grab still exists, and a selection made on another
        // generation has to fall, because a placement index means nothing in a new result.
        m_exportCorrection->OnSceneAboutToBeRebuilt(output.id);
    }

    DrawExportDrawing(m_sceneExport, output, drawing);

    if (m_exportCorrection != nullptr)
    {
        // After the rebuild: the holder is not something Build draws — it must never reach the
        // PDF, which calls the same Build for its own scene — so it is mounted here, on the
        // screen's scene alone, and dressed on the footprint that was selected.
        m_exportCorrection->OnSceneRebuilt(output);
        m_exportCorrection->SetViewScale(ui->view->transform().m11());
    }

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
    //
    // On a tiled run what is shown is the PAPER, not the nesting: the plan's extent reaches the
    // bottom of its last row whether or not the pieces do, so the label gives the sheets' own
    // extent — and, for the count, the sheets that will actually be printed. A blank sheet is not
    // written, and a count that promised it would be one the user could not reconcile with the
    // file. The run's height is nowhere on screen, and a figure nobody can see is a figure nobody
    // can check.
    const Unit unit = output.unit;
    const double width = FromPixel(drawing.drawnRect.width(), unit);
    const double height = FromPixel(drawing.drawnRect.height(), unit);

    QString label = tr("%1 × %2 %3").arg(QString::number(width, 'f', 1),
                                         QString::number(height, 'f', 1),
                                         UnitsToStr(unit, true));

    if (drawing.artboards.valid)
    {
        // **The cover page's own sheet is a page, and it is said so.** It is printed like the
        // others — it is the first page of the file — but it is not a cell of the grid, so the
        // plan's own count does not hold it. Only the ExtraSheet seat adds one; the two other seats
        // put the page on paper the plan already pays for, and the count stays silent about them.
        const bool extraSheet = drawing.cover.seat == OExportCoverPage::Seat::ExtraSheet;
        label += extraSheet
                     ? tr(" · %n pages (cover page included)", nullptr, drawing.sheets.count + 1)
                     : tr(" · %n pages", nullptr, drawing.sheets.count);
    }

    // On every run, not only the tiled ones: a roll and a sheet of A0 are generated too, and a
    // figure that appeared only under a grid would be missing exactly where the cost is least
    // obvious. The count that goes with it is the run's own — see generationMs — so selecting
    // another row in the dock, or undoing a correction, brings back the time it was made in.
    label += tr(" · %1").arg(GenerationTimeText(output.generationMs));

    m_statusLabel->setText(label);
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
