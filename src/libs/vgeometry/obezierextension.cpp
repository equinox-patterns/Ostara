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

#include <cmath>

namespace
{
// Parameter step used to read the direction the curve is really travelling in at its ends.
constexpr qreal directionStep = 1e-3;

auto PointAt(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, qreal t) -> QPointF
{
    const qreal mt = 1.0 - t;
    return (mt * mt * mt) * p1 + (3.0 * mt * mt * t) * p2 + (3.0 * mt * t * t) * p3 + (t * t * t) * p4;
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
    // Direction the curve is really travelling in when it reaches its end. The prolongation follows
    // it straight: following the cubic polynomial instead makes it hook back on itself a few pixels
    // further, which reads as a 180 degree turn.
    const QPointF travel = p4 - PointAt(p1, p2, p3, p4, 1.0 - directionStep);

    return StraightPiece(p4, travel, length, p4 - p1);
}

//---------------------------------------------------------------------------------------------------------------------
auto OBezierExtension::Before(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, qreal length)
    -> Piece
{
    // Same reasoning as After, on the other end of the curve. The fallback chord points away from
    // the curve, so the piece is still built outwards.
    const QPointF travel = p1 - PointAt(p1, p2, p3, p4, directionStep);

    const Piece straight = StraightPiece(p1, travel, length, p1 - p4);

    // Back to the curve's own direction: the piece runs from its far end towards the curve.
    return {straight.p4, straight.p3, straight.p2, straight.p1};
}

//---------------------------------------------------------------------------------------------------------------------
auto OBezierExtension::Normalize(qreal length, qreal curveLength) -> qreal
{
    return length < 0.0 ? curveLength + length : length;
}
