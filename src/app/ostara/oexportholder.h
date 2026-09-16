/************************************************************************
 **
 **  @file   oexportholder.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Export mode — the frame a hand drags a footprint by.
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

#ifndef OEXPORTHOLDER_H
#define OEXPORTHOLDER_H

#include <QGraphicsRectItem>
#include <QObject>
#include <QPointF>

#include "oexportnesting.h"

/// @brief The rectangle drawn around a selected footprint, and the four corners that turn it.
///
/// Its own frame IS the footprint's: the rect is `(0, 0, extentX, extentY)`, the position is
/// `(x, y)` and the rotation is `rotation`. That works because a placement's origin is the
/// min-bbox corner of the footprint BEFORE rotation — so local (0, 0) is that corner, and the
/// rect's four corners are the footprint's four corners. The handles therefore land on the
/// footprint without a single conversion, and a holder drawn from anything else would need a
/// second definition of the very formula OExportFootprint exists to state once.
///
/// It is an input device and nothing more. It never moves itself, never writes a placement
/// and never touches the scene: it reports where the mouse is and the controller decides what
/// that means. Everything a gesture has to respect — the plan's edges, the frozen page count,
/// the model write — belongs to the controller, and none of it is reachable from here.
class OExportHolderItem final : public QObject, public QGraphicsRectItem
{
    Q_OBJECT

public:
    enum
    {
        Type = QGraphicsItem::UserType + 1210
    };

    explicit OExportHolderItem(QGraphicsItem *parent = nullptr);

    /// @brief Dress the holder on @p placement. Called at selection, and on every frame of a
    ///        gesture, since the controller previews by recomputing the placement.
    void SetPlacement(const OExportPlacement &placement);

    /// @brief The view's current scale, so a handle keeps the same size on screen.
    ///
    /// Not a refinement: the mode frames the whole plan, so the view sits near 0.2 while the
    /// plan is thousands of pixels wide, and a handle of five scene units would be drawn a
    /// single pixel across — invisible, and impossible to grab.
    void SetViewScale(qreal scale);

    auto type() const -> int override;
    auto boundingRect() const -> QRectF override;
    auto shape() const -> QPainterPath override;

signals:
    /// A gesture began on the body — the footprint is being moved.
    void MoveStarted(const QPointF &scenePos);
    /// A gesture began on one of the four corners — the footprint is being turned.
    void RotateStarted(const QPointF &scenePos);
    /// The mouse moved during a gesture, in scene coordinates.
    void GestureMoved(const QPointF &scenePos);
    /// The button came up. Whether anything moved is the controller's to decide, by comparing
    /// the placement it previewed against the one it started from.
    void GestureFinished();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    enum class Handle
    {
        None, ///< the body
        TopLeft,
        TopRight,
        BottomRight,
        BottomLeft
    };

    auto HitTestHandle(const QPointF &localPos) const -> Handle;
    auto HandleCentre(Handle handle) const -> QPointF;

    auto BodyCursor() const -> QCursor;
    auto HandleCursor() const -> QCursor;
    void ApplyCursor(Handle handle);

    /// Half the painted square, in DEVICE pixels — divided by the view scale on the way out.
    static constexpr qreal kHandleHalfPx = 5.0;
    /// Half the grab area. Wider than the paint on purpose: a handle you can see but not
    /// catch is worse than no handle at all.
    static constexpr qreal kHandleHitHalfPx = 11.0;

    qreal m_viewScale{1.0};
    Handle m_handle{Handle::None};
    bool m_dragging{false};
};

#endif // OEXPORTHOLDER_H
