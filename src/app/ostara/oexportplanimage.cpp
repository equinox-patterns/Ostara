/************************************************************************
 **
 **  @file   oexportplanimage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Export mode — the size an image of the plan is asked for.
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

#include "oexportplanimage.h"

#include "../vmisc/def.h"

#include <QtMath>

namespace
{
/// Millimetres at @p dpi, in pixels.
///
/// Written out rather than taken from ToPixel(), which converts at PrintDPI: the resolution
/// asked for is the whole point of a raster here, and borrowing a helper that has 96 baked
/// into it would be a conversion that lies about what it does.
constexpr auto PixelsOf(qreal mm, qreal dpi) -> qreal
{
    return mm * dpi / 25.4;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlanImage::PlanOf(const QRectF &drawnRect) -> OExportPlanImage::Plan
{
    return {drawnRect, FromPixel(drawnRect.width(), Unit::Mm), FromPixel(drawnRect.height(), Unit::Mm)};
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlanImage::OtherSideMm(const Plan &plan, qreal targetMm, Axis axis) -> qreal
{
    const qreal pinned = (axis == Axis::Width) ? plan.widthMm : plan.heightMm;
    const qreal other = (axis == Axis::Width) ? plan.heightMm : plan.widthMm;

    if (targetMm <= 0.0 || pinned <= 0.0 || other <= 0.0)
    {
        return 0.0;
    }

    return targetMm * other / pinned;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlanImage::SizeFor(const Plan &plan, qreal targetMm, Axis axis, qreal dpi) -> Target
{
    Target target;

    if (plan.rectPx.isEmpty() || plan.widthMm <= 0.0 || plan.heightMm <= 0.0 || targetMm <= 0.0
        || dpi <= 0.0)
    {
        return target; // invalid — the caller refuses on it, and nothing is guessed
    }

    const qreal otherMm = OtherSideMm(plan, targetMm, axis);
    if (otherMm <= 0.0)
    {
        return target;
    }

    // Both counts come from the two millimetres the dialog shows, so the figure the user reads
    // and the file she gets cannot come from two arithmetics.
    //
    // The qMax is not a taste: a side that lands under half a pixel rounds to zero, and a QSize
    // with a zero side is EMPTY — QImage would be null, save() would write nothing at all, and
    // the file would exist carrying zero bytes. A tiny image is a poor image; an empty file
    // taken for a success is the one outcome worth forbidding outright.
    const QSize counted(qMax(1, qRound(PixelsOf(targetMm, dpi))),
                        qMax(1, qRound(PixelsOf(otherMm, dpi))));

    target.sizePx = (axis == Axis::Width) ? counted : counted.transposed();
    target.dpi = dpi;
    return target;
}
