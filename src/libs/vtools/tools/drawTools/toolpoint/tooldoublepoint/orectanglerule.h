/************************************************************************
 **
 **  @file   orectanglerule.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  the three points a rectangle is made of, from its base point
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

#ifndef ORECTANGLERULE_H
#define ORECTANGLERULE_H

#include <QPointF>
#include <QtGlobal>

/**
 * @brief The rectangle, in one place.
 *
 * A rectangle is a base point and two lengths: three points follow, clockwise on screen — B to the right of A
 * by the width, C below B by the height, D below A by the height. Which side the width is, and which the
 * height, is what the two pastilles of the window name: the top side (A→B) is the width, the right side
 * (B→C) is the height.
 *
 * The rule lives here and nowhere else: the window builds its preview with it, the tool creates its points
 * with it, and the two therefore cannot disagree about what a rectangle is.
 */
class ORectangleRule
{
public:
    ORectangleRule() = delete;

    /** @brief The four corners of the rectangle, clockwise from the base point.
     *
     * @p width and @p height are already in pixels — the caller owns the unit conversion, exactly as the
     * dialog and the tool do for every other tool. A negative length is allowed and mirrors that side; a
     * null one gives a flat rectangle, which is still a construction the user asked for.
     */
    struct Corners
    {
        QPointF a{};
        QPointF b{};
        QPointF c{};
        QPointF d{};
    };

    static auto Build(const QPointF &base, qreal widthPx, qreal heightPx) -> Corners;
};

//---------------------------------------------------------------------------------------------------------------------
inline auto ORectangleRule::Build(const QPointF &base, qreal widthPx, qreal heightPx) -> Corners
{
    // Clockwise on screen, where y grows downwards: right, then down, then left, then up.
    return Corners{base,
                   QPointF(base.x() + widthPx, base.y()),
                   QPointF(base.x() + widthPx, base.y() + heightPx),
                   QPointF(base.x(), base.y() + heightPx)};
}

#endif // ORECTANGLERULE_H
