/************************************************************************
 **
 **  @file   oexportcorrection.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Export mode — correcting a nesting plan by hand.
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

#ifndef OEXPORTCORRECTION_H
#define OEXPORTCORRECTION_H

#include <QList>
#include <QObject>
#include <QPointer>
#include <QPointF>
#include <QRectF>
#include <QTransform>
#include <QUndoStack>

#include <functional>

#include "oexportnesting.h"

class OExportHolderItem;
struct OExportOutput;
class OExportOutputStore;
class QGraphicsItem;
class VMainGraphicsScene;

/// @brief Lets a hand move and turn a footprint of the plan shown in the Export canvas.
///
/// The nesting lays a plan out as well as arithmetic can; past that point the eye is the only
/// judge, and this is where the eye gets its hands. One click picks a FOOTPRINT — every size
/// of a piece, or every member and every size of a group — and the holder that appears is its
/// rectangle. Children follow because they were never separate: the plan draws each size under
/// its footprint's transform, and this moves that transform.
///
/// What it holds is never an item pointer. The selection is `(outputId, placementIndex)`,
/// because Build() clears the scene on every display and anything pointing into the old one
/// would be dangling before the user could look at it. The follow set is re-derived from the
/// scene by the role the canvas stamps on what it draws.
///
/// Nothing here re-runs the nesting. A gesture writes one placement into a result that already
/// exists, which is why Trame is untouched by this feature — and why the page count cannot
/// move: a row follows `usedHeightPx`, and `SetPlacement` cannot reach it.
class OExportCorrection final : public QObject
{
    Q_OBJECT

public:
    /// @param repaint called with a generation id after a correction is written or undone.
    ///                The store is not a QObject and cannot announce anything, and only the
    ///                window knows whether the generation written is the one on screen.
    OExportCorrection(VMainGraphicsScene *scene, OExportOutputStore *store,
                      std::function<void(quint32)> repaint, QObject *parent = nullptr);

    /// @brief The rectangle a gesture may not leave — the plan as it is drawn.
    ///
    /// On a tiled run this is the sheets the result was cut into, dead band of the last row
    /// included; otherwise the bin. Read from the same OExportArtboards::Grid the canvas drew
    /// from, so a piece can never be put where the plan is not.
    void SetPlan(const QRectF &plan);

    /// @brief The view's scale, passed on to the holder so its handles keep their size.
    void SetViewScale(qreal scale);

    /// @brief Called by ShowExportOutput BEFORE Build, which clears the scene.
    ///
    /// Ends any gesture in flight — Build would destroy the item holding the mouse grab and
    /// the drag would stop dead on the next movement — drops the holder, and drops the
    /// selection when @p outputId is not the generation it was made on.
    void OnSceneAboutToBeRebuilt(quint32 outputId);

    /// @brief Called by ShowExportOutput AFTER Build: dresses the holder again.
    void OnSceneRebuilt(const OExportOutput &output);

    /// @brief Forget everything: the pattern is closing, or the GUI is going away.
    void Reset();

    auto UndoStack() -> QUndoStack *;

private:
    void OnSceneMouseMove(const QPointF &scenePos);
    void OnItemPressed(QGraphicsItem *item);

    void OnMoveStarted(const QPointF &scenePos);
    void OnRotateStarted(const QPointF &scenePos);
    void OnGestureMoved(const QPointF &scenePos);
    void OnGestureFinished();

    /// One item of a footprint, with the scene mapping a preview needs. Captured once, when the
    /// gesture begins: a preview has to be computed from where the item was BEFORE the gesture,
    /// not from where the previous preview left it, or every movement would re-apply the whole
    /// motion on top of the last. See OExportFootprint::ApplySceneDelta.
    struct FollowItem
    {
        QGraphicsItem *item{nullptr};
        QTransform baseScene{};
    };

    /// Every item the canvas drew for one placement: each size's silhouette, and the decor
    /// carrier. Re-derived rather than remembered — see the note on the class.
    auto FollowSet(int index) const -> QList<FollowItem>;

    void Select(int index);
    void ClearSelection();
    void DressHolder(const OExportPlacement &placement);
    void Preview(const OExportPlacement &placement);
    void Commit(const OExportPlacement &placement);

    VMainGraphicsScene *m_scene{nullptr};
    OExportOutputStore *m_store{nullptr};
    std::function<void(quint32)> m_repaint{};
    QUndoStack m_undoStack{};

    QRectF m_plan{};
    qreal m_viewScale{1.0};

    /// Where the selection is, and the only identity it keeps. 0 means nothing is selected.
    quint32 m_outputId{0};
    int m_index{-1};

    /// The last scene position the scene reported, used to hit-test on a press.
    QPointF m_lastScenePos{};

    /// Frozen at press, so a preview is always recomputed from the same origin and a long
    /// gesture cannot accumulate an error the way a per-move delta would.
    OExportPlacement m_start{};
    OExportPlacement m_preview{};
    QPointF m_startCentre{};
    QPointF m_pressScene{};
    qreal m_pressAngle{0.0};
    bool m_rotating{false};
    bool m_gesturing{false};
    QList<FollowItem> m_follow{};

    QPointer<OExportHolderItem> m_holder{};
};

#endif // OEXPORTCORRECTION_H
