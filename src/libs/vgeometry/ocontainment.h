/************************************************************************
 **
 **  @file   ocontainment.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/10/2026
 **
 **  @brief  Keeps a child of a piece inside the rectangle that forms the
 **          piece. The rule is the grainline's — a child is described by
 **          the points that carry its shape (the ends of the grainline,
 **          the corners of a block or of an annotation box) and it stays
 **          inside when all of them do — written once here and shared, so
 **          the grainline, the information blocks and the annotations can
 **          never drift apart on it.
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

#ifndef OCONTAINMENT_H
#define OCONTAINMENT_H

#include <QPointF>
#include <QRectF>
#include <QVector>

/**
 * @brief Keeping a child inside the rectangle that forms the piece.
 *
 * The points handed over are expressed in the frame of the rectangle — the frame the child's
 * own position lives in — so the translation this module returns is applied to that position,
 * never converted.
 */
namespace OContainment
{
/**
 * @brief Result of fitting a set of points inside a rectangle.
 *
 * @a contained is true when every point already is inside, in which case @a translation is
 * (0, 0) — a caller can then leave the child alone without a second test.
 */
struct Fit
{
    bool contained{true};
    QPointF translation{};
};

/**
 * @brief Whether a rectangle can hold anything at all.
 *
 * A null rectangle — an empty path, a piece with nothing drawn — has no inside: enforcing
 * containment against it would drag every child to a corner of nothing.
 */
auto IsUsable(const QRectF &rect) -> bool;

/**
 * @brief Smallest translation that brings every point of @p points inside @p container.
 *
 * Each axis is driven by the point that is furthest outside it — a point already contained
 * never pulls the others — which is exactly what the grainline has always done with its ends.
 * An unusable container contains nothing to enforce and is reported as already satisfied.
 */
auto FitPoints(const QRectF &container, const QVector<QPointF> &points) -> Fit;

/**
 * @brief Whether a shape can be inside @p container at all.
 *
 * A shape wider or taller than the rectangle has no inside position: the furthest-point rule would
 * then make it swing from one edge to the other at every pass, and a caller that publishes the
 * fitted position back to the model would make the model swing with it. Such a shape has to be left
 * where it is — a block bigger than the piece it sits on, for instance, stays centred on it.
 */
auto FitsInside(const QRectF &container, const QVector<QPointF> &points) -> bool;

/**
 * @brief Whether two placements are one and the same.
 *
 * A fitted position lands exactly on the boundary of the rectangle, and the arithmetic of the fit
 * can miss it by a floating-point round-off. A caller that publishes a fitted position back to the
 * model must let that round-off through: otherwise the model reports a child that moved at every
 * refresh, and the refresh chases itself.
 */
auto SamePlacement(const QPointF &first, const QPointF &second) -> bool;
} // namespace OContainment

#endif // OCONTAINMENT_H
