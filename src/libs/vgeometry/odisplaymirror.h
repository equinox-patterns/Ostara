/************************************************************************
 **
 **  @file   odisplaymirror.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12 November, 2026
 **
 **  @brief  Where a piece's decorations go when the piece is shown mirrored.
 **
 **          A piece carries blocks and annotations as CHILDREN, and a child is not mirrored the way a
 **          path is: it is re-laid. Reflecting the box through a transform would reverse the words it
 **          holds, and a mirrored cartouche is an unreadable one — so the box is placed again where
 **          the reflection would have put it, and drawn upright. This one rule is shared by every
 **          canvas that does it, so a cartouche cannot sit on one side in Details and the other in
 **          the cutting plan.
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

#ifndef ODISPLAYMIRROR_H
#define ODISPLAYMIRROR_H

#include <QPointF>
#include <QSizeF>
#include <QTransform>

namespace ODisplayMirror
{
/**
 * @brief The corner and the angle a box must be given to appear reflected about a vertical line.
 *
 * One helper for a rule with three readers — the export canvas, the Graded canvas and whatever comes
 * next — so the arithmetic is derived once.
 *
 * ⚠️ **Negating the position and the angle is not enough, and the near miss looks right.** A child's
 * position is the box's CORNER — both block and annotation items set their rect at (0, 0) from it —
 * and Qt turns an item about its transform origin, which defaults to that very corner. A reflection
 * reverses the box's own x direction, so the child's new local (0, 0) has to land where its TOP-RIGHT
 * corner was. Without that term the box slides by one rotated width, and a cartouche a width off
 * still reads as "roughly in place" — which is exactly why this is derived rather than eyeballed.
 *
 * ⚠️ The angle is only NEGATED, never a negative scale: the child is re-laid, not reflected through a
 * transform, so the glyphs it holds stay upright.
 *
 * @param topLeft  the box's corner, in the carrier's own frame, before the mirror
 * @param size     the box's size
 * @param rotation the child's RECORDED angle — the one the item classes turn by its negation
 * @param axisX    the vertical line the piece is reflected about, in the same frame
 */
inline auto Placement(const QPointF &topLeft, const QSizeF &size, qreal rotation, qreal axisX)
    -> QPointF
{
    QTransform turn;
    turn.rotate(-rotation); // the child's own rotation, as it will be applied to it

    // Where the box's top-right corner lands in the carrier's frame — the point that becomes the new
    // top-left once the box is reflected.
    const QPointF topRight = topLeft + turn.map(QPointF(size.width(), 0.0));
    return {2.0 * axisX - topRight.x(), topRight.y()};
}
} // namespace ODisplayMirror

#endif // ODISPLAYMIRROR_H
