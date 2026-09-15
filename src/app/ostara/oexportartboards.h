/************************************************************************
 **
 **  @file   oexportartboards.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Export mode — the sheets a tiled plan is cut into.
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

#ifndef OEXPORTARTBOARDS_H
#define OEXPORTARTBOARDS_H

#include <QRectF>

/// @brief Where a tiled plan is cut — the frame, the columns, and nothing else.
///
/// The nesting lays out ONE continuous plan and knows nothing of sheets; a model that is
/// not a roll is a reading of its result, not a constraint on it. That is why the cutting
/// lives here, away from the engine, and why this file holds no scene and no widget: a
/// quantity that can be wrong arithmetically has to be readable on its own.
///
/// The same grid is the screen's and, one day, the PDF's page boundaries — a page being an
/// artboard, its edge is not drawn there, it IS the page. Both read it from here so that
/// the two cannot come apart.
namespace OExportArtboards
{
/// The small side of a frame is never needed: the plan is exactly as wide as the columns
/// of it put together, so the bin's own width divided back gives it, and one figure fewer
/// is one figure fewer to keep in agreement.
struct Grid
{
    /// False when the run is not tiled, or when there is nothing to cut up. The canvas
    /// then draws its bin exactly as it always did, and draws no grid at all.
    bool valid{false};

    int columns{0};
    int rows{0};

    qreal cellWidthPx{0.0};
    qreal cellHeightPx{0.0};

    /// Every sheet of every row, from the plan's origin — the last row whole, whether or
    /// not the nesting reached the bottom of it. A sheet is never cut short.
    QRectF planRect{};
};

/// @brief Cut a plan @p binWidthPx wide and @p usedHeightPx tall into sheets.
///
/// @param binWidthPx    the plan's width, in pixels — the frame's small side times the columns
/// @param usedHeightPx  the height the nesting came out at, in pixels
/// @param frameHeightMm the frame's long side, in MILLIMETRES — the only length converted here
/// @param columns       how many sheets side by side — at least 1
auto Compute(qreal binWidthPx, qreal usedHeightPx, qreal frameHeightMm, int columns) -> Grid;
} // namespace OExportArtboards

#endif // OEXPORTARTBOARDS_H
