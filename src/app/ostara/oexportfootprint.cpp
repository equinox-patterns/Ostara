/************************************************************************
 **
 **  @file   oexportfootprint.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Export mode — where a footprint sits, and where a hand may put it.
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

#include "oexportfootprint.h"

#include <QGraphicsItem>

#include <algorithm>
#include <cmath>

namespace
{
/// The footprint's local rectangle: its origin is the placement's origin, and its size is the
/// extent the nesting measured. Everything below is built from this and the frame.
auto LocalRect(const OExportPlacement &placement) -> QRectF
{
    return QRectF(0.0, 0.0, placement.extentX, placement.extentY);
}

/// Where the footprint's OWN origin lands, and which way it is turned: `u ↦ (x, y) + R(θ)·u`.
///
/// This is NOT Matrix(), and using Matrix() here was a real bug. Matrix() places a point of the
/// DOCUMENT — it carries `T(-origin)`, because `origin` is the footprint's min-bbox corner in
/// document coordinates. The rectangle below is expressed from that origin already, so the
/// shift would be applied twice: a footprint sitting neatly at (18, 18) in the plan came out at
/// x = -2871, the clamp saw it outside and shoved it 2871 pixels to the right on the first
/// movement. Measured, on screen, before it was understood.
auto Frame(const OExportPlacement &placement) -> QTransform
{
    QTransform frame;
    frame.translate(placement.x, placement.y);
    frame.rotate(placement.rotation);
    return frame;
}

/// Slide @p placement along one axis so that its box falls inside [@p lo, @p hi].
///
/// A footprint wider than the plan cannot be made to fit, and refusing the move would leave
/// it stuck against an edge it is not touching — so it is centred on that axis instead. The
/// plan is frozen; the footprint is not, and the two do not have to be compatible.
auto SlideInto(qreal &position, qreal boxLo, qreal boxHi, qreal lo, qreal hi) -> void
{
    if (boxHi - boxLo > hi - lo)
    {
        position += (lo + hi) / 2.0 - (boxLo + boxHi) / 2.0;
        return;
    }

    // The correction to apply is the one that brings the box back, which is 0 when it is
    // already in — hence clamping zero rather than clamping the position.
    position += std::min(std::max(0.0, lo - boxLo), hi - boxHi);
}

auto HeldIn(const OExportPlacement &placement, const QRectF &plan) -> OExportPlacement
{
    const QRectF box = OExportFootprint::Rect(placement);

    OExportPlacement out = placement;
    SlideInto(out.x, box.left(), box.right(), plan.left(), plan.right());
    SlideInto(out.y, box.top(), box.bottom(), plan.top(), plan.bottom());
    return out;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportFootprint::Matrix(const OExportPlacement &placement) -> QTransform
{
    QTransform matrix;
    matrix.translate(placement.x, placement.y);
    matrix.rotate(placement.rotation);
    matrix.translate(-placement.originX, -placement.originY);
    return matrix;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFootprint::PieceMatrix(const OExportPlacement &placement, const QPointF &alignDelta)
    -> QTransform
{
    QTransform matrix;
    matrix.translate(placement.x, placement.y);
    matrix.rotate(placement.rotation);
    matrix.translate(-placement.originX + alignDelta.x(), -placement.originY + alignDelta.y());
    return matrix;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFootprint::Centre(const OExportPlacement &placement) -> QPointF
{
    return Frame(placement).map(LocalRect(placement).center());
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFootprint::Corners(const OExportPlacement &placement) -> QPolygonF
{
    const QTransform frame = Frame(placement);
    const QRectF local = LocalRect(placement);

    return QPolygonF{frame.map(local.topLeft()),
                     frame.map(local.topRight()),
                     frame.map(local.bottomRight()),
                     frame.map(local.bottomLeft())};
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFootprint::Outline(const OExportPlacement &placement) -> QPolygonF
{
    // Frame() and never Matrix() — see the note on that function. The outline is expressed from
    // the footprint's origin already, and Matrix() would take that origin off a second time.
    return Frame(placement).map(placement.outline);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFootprint::Rect(const OExportPlacement &placement) -> QRectF
{
    if (placement.outline.isEmpty())
    {
        // No outline to measure — a placement that never came from a run. The box of the
        // holder's rectangle is the safe answer: it never leaves a piece more room than the
        // shape itself would, which errs towards refusing a move rather than allowing one that
        // puts ink off the paper.
        return Corners(placement).boundingRect();
    }

    return Outline(placement).boundingRect();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFootprint::Moved(const OExportPlacement &placement, const QPointF &delta,
                            const QRectF &plan) -> OExportPlacement
{
    OExportPlacement out = placement;
    out.x += delta.x();
    out.y += delta.y();
    return HeldIn(out, plan);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFootprint::Rotated(const OExportPlacement &placement, qreal deltaDegrees,
                               const QRectF &plan) -> OExportPlacement
{
    // The centre is a function of the placement, so it is taken from the placement the turn
    // starts at. A rotation applied after a move therefore turns the piece about where it now
    // is, which is what the eye expects — anchoring on the generation's original point would
    // send the piece orbiting on the second turn.
    const QPointF pivot = Centre(placement);

    // A placement is a frame with origin at `origin` and size `extent`, so what the rotation
    // does to `(x, y)` is exactly what it does to the local (0, 0) corner: same rigid motion,
    // same centre, expressed in the scene. The origin and the extent are untouched, because
    // neither depends on the angle — they describe the footprint the engine measured, not the
    // one on screen.
    QTransform turn;
    turn.rotate(deltaDegrees);
    const QPointF moved = turn.map(QPointF(placement.x - pivot.x(), placement.y - pivot.y()));

    OExportPlacement out = placement;
    out.rotation = placement.rotation + deltaDegrees;
    out.x = pivot.x() + moved.x();
    out.y = pivot.y() + moved.y();
    return HeldIn(out, plan);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFootprint::AngleOf(const QPointF &centre, const QPointF &scenePos) -> qreal
{
    // atan2 in a y-down frame grows in the same direction QTransform::rotate() turns, so the
    // difference of two readings is the angle to apply, sign included.
    return qRadiansToDegrees(std::atan2(scenePos.y() - centre.y(), scenePos.x() - centre.x()));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFootprint::ApplySceneDelta(QGraphicsItem *item, const QTransform &baseScene,
                                       const QTransform &delta)
{
    if (item == nullptr)
    {
        return;
    }

    // MEASURED, not assumed: an item whose position and rotation are both zero has a scene
    // mapping equal to its transform, exactly. So the whole placement is folded into the
    // transform and the two properties are cleared — no model of Qt's composition order is
    // needed, and none is trusted. `baseScene * delta` composes for row vectors, where the left
    // factor is applied first: where the item was, then the gesture's motion.
    //
    // Nothing reads those two back. Build() recreates every item it stamped, and the follow set
    // is re-derived at each press rather than remembered.
    item->setPos(0.0, 0.0);
    item->setRotation(0.0);
    item->setTransform(baseScene * delta, false);
}
