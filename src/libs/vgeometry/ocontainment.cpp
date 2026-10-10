/************************************************************************
 **
 **  @file   ocontainment.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/10/2026
 **
 **  @brief  Keeps a child of a piece inside the rectangle that forms the
 **          piece (see ocontainment.h).
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

#include "ocontainment.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace
{
/// Two placements closer than this are one and the same. A millionth of a pixel is far below
/// anything a canvas or a printed sheet can show, and far above a floating-point round-off.
constexpr qreal kSamePlacement = 1e-6;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OContainment::IsUsable(const QRectF &rect) -> bool
{
    // A rectangle reduced to a line or to a point has no width or height to divide by, and a
    // null one is what an empty path hands out.
    return not rect.isNull() && rect.width() > 0.0 && rect.height() > 0.0;
}

//---------------------------------------------------------------------------------------------------------------------
auto OContainment::FitPoints(const QRectF &container, const QVector<QPointF> &points) -> Fit
{
    Fit fit;
    if (not IsUsable(container))
    {
        return fit; // nothing to stay inside
    }

    for (const QPointF &point : points)
    {
        if (container.contains(point))
        {
            continue;
        }

        qreal dPtX = 0.0;
        if (point.x() < container.left())
        {
            dPtX = container.left() - point.x();
        }
        else if (point.x() > container.right())
        {
            dPtX = container.right() - point.x();
        }

        qreal dPtY = 0.0;
        if (point.y() < container.top())
        {
            dPtY = container.top() - point.y();
        }
        else if (point.y() > container.bottom())
        {
            dPtY = container.bottom() - point.y();
        }

        // The furthest point on each axis decides the move; the others follow it, so one
        // translation brings them all back at once.
        if (std::fabs(dPtX) > std::fabs(fit.translation.x()))
        {
            fit.translation.setX(dPtX);
        }
        if (std::fabs(dPtY) > std::fabs(fit.translation.y()))
        {
            fit.translation.setY(dPtY);
        }

        fit.contained = false;
    }

    return fit;
}

//---------------------------------------------------------------------------------------------------------------------
auto OContainment::FitsInside(const QRectF &container, const QVector<QPointF> &points) -> bool
{
    if (not IsUsable(container) || points.isEmpty())
    {
        return false;
    }

    qreal left = points.constFirst().x();
    qreal right = left;
    qreal top = points.constFirst().y();
    qreal bottom = top;
    for (const QPointF &point : points)
    {
        left = (std::min)(left, point.x());
        right = (std::max)(right, point.x());
        top = (std::min)(top, point.y());
        bottom = (std::max)(bottom, point.y());
    }

    return (right - left) <= container.width() && (bottom - top) <= container.height();
}

//---------------------------------------------------------------------------------------------------------------------
auto OContainment::SamePlacement(const QPointF &first, const QPointF &second) -> bool
{
    return std::fabs(first.x() - second.x()) < kSamePlacement && std::fabs(first.y() - second.y()) < kSamePlacement;
}
