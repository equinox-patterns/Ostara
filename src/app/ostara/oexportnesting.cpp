/************************************************************************
 **
 **  @file   oexportnesting.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — feed the Trame nesting engine with the pattern's
 **          own geometry and bring back the placements.
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

#include "oexportnesting.h"

#include "oexportgeometry.h"

#include "../vlayout/vlayoutpoint.h"

#include "model/Piece.h"
#include "nesting/Bin.h"
#include "nesting/Engine.h"
#include "nesting/Footprint.h"

#include <QObject>

#include <cmath>

namespace
{
/// The engine works in fixed-point: one pixel is kTrameScale units. Every
/// coordinate handed to it, and every coordinate it returns, uses that scale.
constexpr double kTrameScale = 1e6;

/// @param offset the piece's alignment shift, in pixels — the same one the canvas
///               applies, so the bin and the screen show the same geometry.
auto ToPath64(const QVector<VLayoutPoint> &points, const QPointF &offset) -> Clipper2Lib::Path64
{
    Clipper2Lib::Path64 path;
    path.reserve(static_cast<size_t>(points.size()));

    for (const VLayoutPoint &point : points)
    {
        path.push_back({static_cast<int64_t>(std::llround((point.x() + offset.x()) * kTrameScale)),
                        static_cast<int64_t>(std::llround((point.y() + offset.y()) * kTrameScale))});
    }

    return path;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportNesting::Run(const QVector<OExportSizeLayer> &layers, const OExportNestingParams &params)
    -> OExportNestingResult
{
    OExportNestingResult result;
    result.binWidthPx = params.binWidthPx;

    if (layers.isEmpty())
    {
        result.error = QObject::tr("The size range carries no piece to nest.");
        return result;
    }
    if (params.binWidthPx <= 0.0)
    {
        result.error = QObject::tr("The document width must be greater than zero.");
        return result;
    }

    // The engine never sees a size: it sees one footprint per piece — or per group,
    // which then moves as a single rigid block.
    QVector<SizeLayer> engineLayers;
    engineLayers.reserve(layers.size());

    for (const OExportSizeLayer &layer : layers)
    {
        SizeLayer engineLayer;
        engineLayer.label = layer.label;

        for (const OExportPieceGeometry &piece : layer.pieces)
        {
            Piece enginePiece;
            enginePiece.name = piece.name;
            enginePiece.groupName = piece.groupName;
            // The group's identity travels separately from its name: the engine groups
            // on the key, and displays the name.
            enginePiece.groupKey = piece.groupId == 0 ? QString() : piece.key;

            ContourNode node;
            node.type = QStringLiteral("Contour");
            node.polyline = ToPath64(piece.piece.GetExternalContourPoints(), piece.alignDelta);
            if (node.polyline.size() < 3)
            {
                continue;
            }
            enginePiece.contourNodes.append(node);

            engineLayer.pieces.append(enginePiece);
        }

        if (!engineLayer.pieces.isEmpty())
        {
            engineLayers.append(engineLayer);
        }
    }

    if (engineLayers.isEmpty())
    {
        result.error = QObject::tr("No piece of this range could be prepared for nesting.");
        return result;
    }

    const Footprint::Result footprint = Footprint::compute(engineLayers);
    if (footprint.polygons.empty())
    {
        result.error = QObject::tr("The size range carries no usable contour.");
        return result;
    }

    // Fixed width, unbounded height: the height is what the nesting produces.
    //
    // The margin and the gap arrive already in pixels, like the width and the polygons:
    // the engine consumes every length on the same scale, and the conversion from the
    // millimetres the user set is made once, at the caller — handed over raw, 5.0 would
    // mean 5 px, which is 1.3 mm, and not the policy Rabnest applies.
    const Bin bin(params.binWidthPx, params.marginPx);
    Engine::Config config;
    config.gap = params.gapPx;

    // The grid, and only on a tiled run. It is what lets the engine adjust the
    // plan once it is placed — keeping the footprint borders off the cut lines,
    // and spending the dead band the last artboard row keeps for free. Left at
    // zero, the engine places exactly as it did before tiling existed.
    config.artboardCount  = params.artboardCount;
    config.artboardHeight = params.artboardHeightPx;

    const Engine engine{config};
    const std::vector<Placement> placements = engine.place(bin, footprint.polygons);

    if (placements.size() != footprint.polygons.size())
    {
        result.error = QObject::tr("The nesting engine returned an incomplete result.");
        return result;
    }

    result.keys.reserve(static_cast<int>(footprint.keys.size()));
    for (const QString &key : footprint.keys)
    {
        result.keys.append(key);
    }

    for (size_t i = 0; i < placements.size(); ++i)
    {
        // The engine marks an unplaced piece with a negative rotation and reports
        // nothing else — no status, no exception.
        if (placements[i].rotation < 0.0)
        {
            result.error = QObject::tr("The document is too narrow: some pieces could not be placed.");
            return result;
        }

        int64_t xMin = footprint.polygons[i].front().x;
        int64_t yMin = footprint.polygons[i].front().y;
        for (const auto &point : footprint.polygons[i])
        {
            xMin = std::min(xMin, point.x);
            yMin = std::min(yMin, point.y);
        }

        OExportPlacement placement;
        placement.x = placements[i].x;
        placement.y = placements[i].y;
        placement.rotation = placements[i].rotation;
        placement.originX = static_cast<double>(xMin) / kTrameScale;
        placement.originY = static_cast<double>(yMin) / kTrameScale;
        result.placements.append(placement);
    }

    result.usedHeightPx = Engine::UsedHeight(placements, footprint.polygons, bin);

    result.ok = true;
    return result;
}
