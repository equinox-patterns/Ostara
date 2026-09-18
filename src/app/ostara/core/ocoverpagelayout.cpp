/************************************************************************
 **
 **  @file   ocoverpagelayout.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The page a cover page is written on, and where the bands stand on it.
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

#include "ocoverpagelayout.h"

#include "vmisc/def.h"

namespace
{
using Placement = OCoverPageTemplate::Placement;

/**
 * How much of a band's width its content may take.
 *
 * The two columns of the middle band are half the page each, and without this their contents would
 * meet at the seam and read as one line rather than two. The same share is kept for the full-width
 * bands so that everything on the page shares one margin — a band touching the edge while a column
 * does not would look like a mistake rather than a choice.
 */
constexpr qreal kContentShare = 0.95;

/** Half the page, for the middle band's two columns. */
constexpr qreal kColumnShare = 0.5;

/**
 * The content width of one area, and where it starts.
 *
 * The content is centred in the area and takes at most kContentShare of it — so the margin is the
 * same on both sides, and two neighbouring columns are kept apart by twice what one of them gives
 * up.
 */
auto ContentRect(const QRectF &area) -> QRectF
{
    const qreal width = area.width() * kContentShare;
    return {area.left() + (area.width() - width) / 2.0, area.top(), width, area.height()};
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageLayout::PageSize() -> QSizeF
{
    return {ToPixel(kPageWidthCm, Unit::Cm), ToPixel(kPageHeightCm, Unit::Cm)};
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageLayout::Place(
    const QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>> &bandLines)
    -> QMap<OCoverPageTemplate::Placement, QRectF>
{
    const QSizeF sheet = PageSize();

    // A band with nothing in it takes no height at all: it is not a gap on the page, it is a band
    // the user has not filled.
    const auto HeightOf = [&bandLines](Placement placement) -> qreal {
        const QVector<OCartoucheRenderLine> lines = bandLines.value(placement);
        return lines.isEmpty() ? 0.0 : OCartouchePainter::NaturalSize(lines).height();
    };

    const qreal topHeight = HeightOf(Placement::Top);
    const qreal middleHeight = qMax(HeightOf(Placement::MiddleLeft), HeightOf(Placement::MiddleRight));
    const qreal bottomHeight = HeightOf(Placement::Bottom);
    const qreal totalHeight = topHeight + middleHeight + bottomHeight;

    const qreal halfWidth = sheet.width() * kColumnShare;

    QMap<Placement, QRectF> cells;
    qreal y = sheet.height() / 2.0 - totalHeight / 2.0;

    if (topHeight > 0.0)
    {
        cells.insert(Placement::Top, ContentRect(QRectF(0.0, y, sheet.width(), topHeight)));
    }
    y += topHeight;

    // Both columns start at the same height, and the band is as tall as the taller of the two: one
    // of them being shorter does not lift the other's first line.
    if (middleHeight > 0.0)
    {
        cells.insert(Placement::MiddleLeft, ContentRect(QRectF(0.0, y, halfWidth, middleHeight)));
        cells.insert(Placement::MiddleRight,
                     ContentRect(QRectF(halfWidth, y, halfWidth, middleHeight)));
    }
    y += middleHeight;

    if (bottomHeight > 0.0)
    {
        cells.insert(Placement::Bottom, ContentRect(QRectF(0.0, y, sheet.width(), bottomHeight)));
    }

    return cells;
}
