/************************************************************************
 **
 **  @file   oexportmode.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — entry point and flow.
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
#include "dialogs/oexportmodedialog.h"
#include "dialogs/owaitdialog.h"
#include "oexportalignment.h"
#include "oexportcanvas.h"
#include "oexportgeometry.h"
#include "oexportnesting.h"
#include "widgets/DrawToolBar.h"
#include "widgets/ODockManager.h"
#include "widgets/TopBar.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QMessageBox>
#include <QThreadPool>
#include <QtConcurrent>

#include <memory>

namespace
{
/// One nesting at a time, and never on the global pool: it is shared with the rest
/// of the application, and the documented trap of this codebase is a nested
/// submission that deadlocks the pool.
constexpr int kExportNestingThreads = 1;
} // namespace

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

        // Clicking the mode button while already in the mode asks for another run —
        // a nesting that failed (a bin too narrow, say) must not force the user out
        // of the mode and back in.
        if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Export)
        {
            OExportModeDialog dialog(pattern, m_m, this);
            if (dialog.exec() == QDialog::Accepted)
            {
                RunExportNesting(dialog.DocumentWidth(), dialog.SizeRangeName(), dialog.IsSingleSize(),
                                 dialog.DimensionBase(0), dialog.DimensionBase(1), dialog.DimensionBase(2));
            }
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
    // still on the loop that posted them.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // What to nest and on which bin is decided here, on the empty export canvas.
    // Cancelling gives the previous mode back — the mode was already switched.
    OExportModeDialog dialog(pattern, m_m, this);
    if (dialog.exec() != QDialog::Accepted)
    {
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

    RunExportNesting(dialog.DocumentWidth(), dialog.SizeRangeName(), dialog.IsSingleSize(),
                     dialog.DimensionBase(0), dialog.DimensionBase(1), dialog.DimensionBase(2));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RunExportNesting resolve the range, nest it on the imposed width, and draw it.
 */
auto MainWindow::RunExportNesting(qreal documentWidth, const QString &rangeName, bool singleSize, qreal baseA,
                                  qreal baseB, qreal baseC) -> bool
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

    if (singleSize)
    {
        // The user asked for this size: the document stays on it.
        applySize(baseA, baseB, baseC);
        layers = geometry.CollectCurrentSize(QStringLiteral("A=%1 B=%2 C=%3").arg(baseA).arg(baseB).arg(baseC));
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
        return false;
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
        return false;
    }

    // The range is the authority on where the decor of the reference layer goes: it holds
    // an override for every owner the user positioned in Graded, and the mode reads it
    // without ever writing to it. A single-size run names no range — there is nothing to
    // read, and every piece keeps the geometry it carries.
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

    OExportCanvas::Build(m_sceneExport, layers, result, range, pattern);

    // Frame the whole nesting — the very same thing the "Zoom fit best" action (Cmd+=)
    // does, which is what this is meant to reproduce. The former call framed
    // m_sceneExport->sceneRect(), a rect NewSceneRect() had just widened to the union of
    // the items AND the view's visible area: the bin came out framed inside its own
    // scrollable scene instead of filling the viewport. ZoomFitBest() frames
    // VisibleItemsBoundingRect() alone, clamps the factor to MinScale/MaxScale and
    // reports the resulting scale. It calls NewSceneRect() itself, so the explicit call
    // that preceded it is dropped.
    ui->view->ZoomFitBest();

    const double width = FromPixel(result.binWidthPx, unit);
    const double height = FromPixel(result.usedHeightPx, unit);
    m_statusLabel->setText(tr("%1 × %2 %3").arg(QString::number(width, 'f', 1),
                                                 QString::number(height, 'f', 1),
                                                 UnitsToStr(unit, true)));
    return true;
}
