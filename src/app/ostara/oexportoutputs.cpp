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
#include "oexportgeometry.h"
#include "oexportnesting.h"
#include "widgets/ODockManager.h"
#include "widgets/owidgetoutputs.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
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
auto MainWindow::RunExportNesting(qreal documentWidth, const QString &rangeName, bool singleSize, qreal baseA,
                                  qreal baseB, qreal baseC, const OGradedSizeRanges::Context &context)
    -> std::optional<OExportOutput>
{
    const Unit unit = VAbstractValApplication::VApp()->patternUnits();
    const double binWidthPx = ToPixel(documentWidth, unit);

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

    if (singleSize)
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

            const qreal baseOfA = baseOr(baseA, savedA);
            const qreal baseOfB = baseOr(baseB, savedB);
            const qreal baseOfC = baseOr(baseC, savedC);

            applySize(baseOfA, baseOfB, baseOfC);
            layers = geometry.CollectCurrentSize(
                QStringLiteral("A=%1 B=%2 C=%3").arg(baseOfA).arg(baseOfB).arg(baseOfC));
        }
    }
    else
    {
        const QVector<OExportGeometry::OExportSizeRef> sizes = geometry.RangeSizes(rangeName);

        // The range pins the axes it does not grade, and the pieces must be resolved with
        // those values. The document's own ones send an axis it does not carry back to the
        // measurement file's base, and every piece built from a formula — which is all of
        // them — then comes out at a geometry the range does not name. The graded capture
        // feeds its pieces this exact way. An axis the range pins nothing at keeps the
        // document's value.
        const OExportGeometry::OExportFixedBases fixed = geometry.RangeFixedBases(rangeName);
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
        QtConcurrent::run(&pool, &OExportNesting::Run, layers, binWidthPx);

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
        const auto it = ranges->constFind(rangeName);
        if (it != ranges->cend())
        {
            exportRange = it.value();
            range = &exportRange;
        }
    }

    OExportOutput output;
    output.name = tr("Unnamed"); // no Name field exists yet
    output.measurementCanonical = context.canonical;
    output.sizeAxis = context.axis;
    output.range = exportRange;
    output.hasRange = (range != nullptr);
    output.rangeName = rangeName;
    output.documentWidth = documentWidth;
    output.layers = std::move(layers);
    output.result = result;
    return output;
}

//---------------------------------------------------------------------------------------------------------------------
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
        // Frame the whole nesting — the same thing the "Zoom fit best" action (Cmd+=)
        // does. The former call framed m_sceneExport->sceneRect(), a rect NewSceneRect()
        // had just widened to the union of the items AND the view's visible area: the bin
        // came out framed inside its own scrollable scene instead of filling the viewport.
        ui->view->ZoomFitBest();
    }

    // The status label belongs to what IS shown, not to the run that produced it.
    const Unit unit = VAbstractValApplication::VApp()->patternUnits();
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
