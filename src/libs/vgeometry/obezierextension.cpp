/************************************************************************
 **
 **  @file   obezierextension.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  Prolongement d'une courbe de Bezier cubique au-dela de ses extremites.
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

#include "obezierextension.h"

#include "vgeometrydef.h"

#include <cmath>

namespace
{
// Direction the curve is travelling in when it reaches `end`, read from its handles. The handle
// itself gives the tangent exactly; reading it from a small chord instead tilts the prolongation by
// a few thousandths of a radian, so two curves whose end handles are both horizontal no longer
// prolong in parallel. A zero handle leaves the tangent undefined, and the next control point then
// gives the limit (the second order term). The far point is the last resort.
auto EndDirection(const QPointF &end, const QPointF &handle, const QPointF &next, const QPointF &farPoint)
    -> QPointF
{
    if (!VFuzzyComparePoints(handle, end))
    {
        return end - handle;
    }

    if (!VFuzzyComparePoints(next, end))
    {
        return end - next;
    }

    return end - farPoint;
}

// Straight piece of the wanted length, leaving `from` in the given direction.
auto StraightPiece(const QPointF &from, const QPointF &direction, qreal length, const QPointF &fallback)
    -> OBezierExtension::Piece
{
    QPointF dir = direction;
    qreal norm = std::hypot(dir.x(), dir.y());

    if (norm <= 0.0)
    {
        dir = fallback;
        norm = std::hypot(dir.x(), dir.y());
    }

    if (norm <= 0.0 || length <= 0.0)
    {
        return {from, from, from, from};
    }

    const QPointF step = (length / 3.0 / norm) * dir;
    return {from, from + step, from + 2.0 * step, from + 3.0 * step};
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OBezierExtension::After(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, qreal length)
    -> Piece
{
    // The prolongation follows the curve's real end direction, straight: following the cubic
    // polynomial instead makes it hook back on itself a few pixels further, which reads as a
    // 180 degree turn.
    const QPointF travel = EndDirection(p4, p3, p2, p1);

    return StraightPiece(p4, travel, length, p4 - p1);
}

//---------------------------------------------------------------------------------------------------------------------
auto OBezierExtension::Before(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, qreal length)
    -> Piece
{
    // Same reasoning as After, on the other end of the curve. The fallback chord points away from
    // the curve, so the piece is still built outwards.
    const QPointF travel = EndDirection(p1, p2, p3, p4);

    const Piece straight = StraightPiece(p1, travel, length, p1 - p4);

    // Back to the curve's own direction: the piece runs from its far end towards the curve.
    return {straight.p4, straight.p3, straight.p2, straight.p1};
}

//---------------------------------------------------------------------------------------------------------------------
auto OBezierExtension::Normalize(qreal length, qreal curveLength) -> qreal
{
    return length < 0.0 ? curveLength + length : length;
}
