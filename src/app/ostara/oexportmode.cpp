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
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFuture>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QTextStream>
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

    // The dialog comes first: what to nest and on which bin is decided before the
    // mode is entered. Cancelling means staying where we were.
    OExportModeDialog dialog(pattern, m_m, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        ui->actionExport->setChecked(false);
        return;
    }

    const qreal documentWidth = dialog.DocumentWidth();
    const QString rangeName = dialog.SizeRangeName();
    const bool singleSize = dialog.IsSingleSize();
    const qreal baseA = dialog.DimensionBase(0);
    const qreal baseB = dialog.DimensionBase(1);
    const qreal baseC = dialog.DimensionBase(2);

    ArrowTool(true);

    ui->actionDraw->setChecked(false);
    ui->actionDetails->setChecked(false);
    ui->actionGraded->setChecked(false);
    ui->actionLayout->setChecked(false);
    ui->actionExport->setChecked(true);

    qCDebug(vMainWindow, "Show export scene");
    m_dockManager->saveDockStateForMode(VAbstractValApplication::VApp()->GetDrawMode());

    // The export chrome is the Graded one: the centre of the TopBar is hidden.
    // Leaving Graded must be done explicitly — ActionGraded lives in the Graded
    // files and knows nothing about this mode.
    if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Graded)
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

    RunExportNesting(documentWidth, rangeName, singleSize, baseA, baseB, baseC);
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
    const auto applySize = [this](qreal a, qreal b, qreal c)
    {
        m_currentDimensionA = a;
        m_currentDimensionB = b;
        m_currentDimensionC = c;
        doc->SetDimensionAValue(a);
        doc->SetDimensionBValue(b);
        doc->SetDimensionCValue(c);
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
        for (const OExportGeometry::OExportSizeRef &size : sizes)
        {
            switch (size.axisIndex)
            {
                case 0:
                    applySize(size.base, savedB, savedC);
                    break;
                case 1:
                    applySize(savedA, size.base, savedC);
                    break;
                default:
                    applySize(savedA, savedB, size.base);
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

    // TEMPORARY DIAGNOSTIC — to be removed once the render is validated.
    {
        QFile file(QStringLiteral("/tmp/ostara-export-nesting.log"));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&file);
            out << "=== Export nesting — diagnostic ===\n";
            out << "unité du patron = " << UnitsToStr(unit, true) << '\n';
            out << "largeur saisie = " << documentWidth << " → " << binWidthPx << " px\n";
            out << "plage demandée = " << rangeName << '\n';
            out << "tailles résolues = " << layers.size() << '\n';
            out << "diagnostic géométrie = " << geometry.LastDiagnostic() << '\n';
        }
    }

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

    OExportCanvas::Build(m_sceneExport, layers, result);
    VMainGraphicsView::NewSceneRect(m_sceneExport, ui->view);
    ui->view->fitInView(m_sceneExport->sceneRect(), Qt::KeepAspectRatio);

    const double width = FromPixel(result.binWidthPx, unit);
    const double height = FromPixel(result.usedHeightPx, unit);
    m_statusLabel->setText(tr("%1 × %2 %3").arg(QString::number(width, 'f', 1),
                                                 QString::number(height, 'f', 1),
                                                 UnitsToStr(unit, true)));
    return true;
}
