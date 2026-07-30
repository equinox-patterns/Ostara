/************************************************************************
 **
 **  @file   mainwindow_labels.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Label auto-arrange methods extracted from mainwindow.cpp
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
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/labelarrange/vdrawlabelcollector.h"
#include "../vwidgets/labelarrange/vlabelarrangeengine.h"
#include "../vtools/undocommands/label/movelabel.h"
#include "../vtools/undocommands/label/movedoublelabel.h"
#include "../vtools/undocommands/label/operationmovelabel.h"
#include "../vtools/undocommands/label/movesegmentlabel.h"
#include <QLabel>
#include <QMenu>
#include <QProgressBar>
#include <QFuture>
#include <QFutureWatcher>
#include <QEventLoop>
#include <QtConcurrent>

using namespace Qt::Literals::StringLiterals;


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RunAutoArrangeLabels()
{
    if (m_currentTool != Tool::Arrow)
    {
        return;
    }

    VMainGraphicsScene *const activeScene = m_drawMode ? m_sceneDraw : m_sceneDetails;
    auto [arrangeData, requests] = VDrawLabelCollector::Collect(activeScene);

    if (arrangeData.isEmpty())
    {
        return;
    }

    // Run the placement engine on a worker thread so the UI stays alive (progress bar animates).
    // setEnabled(false) blocks mouse/keyboard input to prevent re-entrant invocations.
    m_progressBar->setRange(0, 0); // indeterminate spinner
    m_progressBar->setValue(0);
    m_statusLabel->setVisible(false);
    m_progressBar->setVisible(true);
    setEnabled(false);

    // Structured-binding names cannot be captured by name in lambdas (Clang limitation),
    // so copy to a regular named variable first.
    const QVector<LabelArrangeData> labelData = arrangeData;

    QFutureWatcher<QVector<QPointF>> watcher;
    QEventLoop loop;
    QObject::connect(&watcher, &QFutureWatcher<QVector<QPointF>>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(QtConcurrent::run([&labelData]() { return VLabelArrangeEngine::Arrange(labelData); }));
    loop.exec();

    setEnabled(true);
    m_progressBar->setVisible(false);
    m_statusLabel->setVisible(true);

    const QVector<QPointF> newOffsets = watcher.result();

    QUndoStack *const undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(tr("Auto-arrange labels"));

    for (int i = 0; i < requests.size(); ++i)
    {
        const QPointF newOff = newOffsets.at(i);
        const QPointF curOff = arrangeData.at(i).currentOffset;
        if (qAbs(newOff.x() - curOff.x()) < 0.1 && qAbs(newOff.y() - curOff.y()) < 0.1)
        {
            continue;
        }

        const LabelMoveRequest &req = requests.at(i);
        switch (req.kind)
        {
            case LabelMoveRequest::Kind::Point:
                undoStack->push(new MoveLabel(doc, newOff, req.toolId));
                break;
            case LabelMoveRequest::Kind::Double:
                undoStack->push(new MoveDoubleLabel(doc, newOff, static_cast<MoveDoublePoint>(req.extraIdx),
                                                    req.toolId, req.pointId));
                break;
            case LabelMoveRequest::Kind::Operation:
                undoStack->push(new OperationMoveLabel(req.toolId, doc, newOff, req.pointId));
                break;
            case LabelMoveRequest::Kind::Segment:
                undoStack->push(new MoveSegmentLabel(doc, newOff, req.toolId,
                                                     static_cast<SegmentLabel>(req.extraIdx)));
                break;
            default:
                break;
        }
    }

    undoStack->endMacro();
}

