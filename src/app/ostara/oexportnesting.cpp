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

#include <QFile>
#include <QObject>
#include <QTextStream>

#include <cmath>

namespace
{
/// The engine works in fixed-point: one pixel is kTrameScale units. Every
/// coordinate handed to it, and every coordinate it returns, uses that scale.
constexpr double kTrameScale = 1e6;

// ─────────────────────────────────────────────────────────────────────────────
// TEMPORARY DIAGNOSTIC — to be removed once the render is validated.
// Writes the real numbers of one nesting run to a fixed file so the shape of the
// input and of the result can be inspected instead of guessed.
const QString kDumpPath = QStringLiteral("/tmp/ostara-export-nesting.log");

void Dump(const QString &line)
{
    QFile file(kDumpPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        QTextStream out(&file);
        out << line << '\n';
    }
}

/// Appends a separator. It must NOT truncate: oexportmode writes what it knows
/// before the nesting runs off the GUI thread, and truncating here threw it away.
void DumpReset()
{
    Dump(QStringLiteral("=== nesting ==="));
}

/// Bounding box and size of a polygon, in pixels and in millimetres.
void DumpPolygon(const QString &what, const Clipper2Lib::Path64 &path)
{
    if (path.empty())
    {
        Dump(QStringLiteral("%1 : EMPTY").arg(what));
        return;
    }

    int64_t xMin = path.front().x;
    int64_t xMax = xMin;
    int64_t yMin = path.front().y;
    int64_t yMax = yMin;
    for (const Clipper2Lib::Point64 &p : path)
    {
        xMin = std::min(xMin, p.x);
        xMax = std::max(xMax, p.x);
        yMin = std::min(yMin, p.y);
        yMax = std::max(yMax, p.y);
    }

    const double wPx = static_cast<double>(xMax - xMin) / kTrameScale;
    const double hPx = static_cast<double>(yMax - yMin) / kTrameScale;
    const double kMmPerPx = 25.4 / 96.0;

    Dump(QStringLiteral("%1 : pts=%2 min=[%3,%4] max=[%5,%6] taille=%7x%8 px = %9x%10 mm")
             .arg(what)
             .arg(path.size())
             .arg(static_cast<double>(xMin) / kTrameScale, 0, 'f', 2)
             .arg(static_cast<double>(yMin) / kTrameScale, 0, 'f', 2)
             .arg(static_cast<double>(xMax) / kTrameScale, 0, 'f', 2)
             .arg(static_cast<double>(yMax) / kTrameScale, 0, 'f', 2)
             .arg(wPx, 0, 'f', 2)
             .arg(hPx, 0, 'f', 2)
             .arg(wPx * kMmPerPx, 0, 'f', 2)
             .arg(hPx * kMmPerPx, 0, 'f', 2));
}
// ─────────────────────────────────────────────────────────────────────────────

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
auto OExportNesting::Run(const QVector<OExportSizeLayer> &layers, double binWidthPx) -> OExportNestingResult
{
    OExportNestingResult result;
    result.binWidthPx = binWidthPx;

    DumpReset();
    Dump(QStringLiteral("binWidthPx=%1 (%2 mm)")
             .arg(binWidthPx, 0, 'f', 2)
             .arg(binWidthPx * 25.4 / 96.0, 0, 'f', 2));
    Dump(QStringLiteral("layers=%1").arg(layers.size()));

    if (layers.isEmpty())
    {
        result.error = QObject::tr("The size range carries no piece to nest.");
        return result;
    }
    if (binWidthPx <= 0.0)
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

        Dump(QStringLiteral("--- taille '%1' : %2 pièces").arg(layer.label).arg(layer.pieces.size()));

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
                Dump(QStringLiteral("    IGNORÉE clé='%1' : contour externe = %2 point(s)")
                         .arg(piece.key)
                         .arg(node.polyline.size()));
                continue;
            }
            enginePiece.contourNodes.append(node);

            DumpPolygon(QStringLiteral("    clé='%1'").arg(piece.key), node.polyline);

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

    Dump(QStringLiteral("=== empreintes : %1").arg(static_cast<int>(footprint.polygons.size())));
    for (size_t i = 0; i < footprint.polygons.size(); ++i)
    {
        DumpPolygon(QStringLiteral("  EMPREINTE '%1' (tailles=%2)")
                        .arg(footprint.keys.at(i))
                        .arg(footprint.sizeCounts.at(i)),
                    footprint.polygons[i]);
    }

    // Fixed width, unbounded height: the height is what the nesting produces.
    const Bin bin(binWidthPx);
    const Engine engine{Engine::Config{}};
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

    Dump(QStringLiteral("=== placements : %1").arg(result.placements.size()));
    for (const OExportPlacement &placement : result.placements)
    {
        Dump(QStringLiteral("  x=%1 y=%2 rot=%3 origine=[%4,%5]")
                 .arg(placement.x, 0, 'f', 2)
                 .arg(placement.y, 0, 'f', 2)
                 .arg(placement.rotation, 0, 'f', 2)
                 .arg(placement.originX, 0, 'f', 2)
                 .arg(placement.originY, 0, 'f', 2));
    }
    Dump(QStringLiteral("=== usedHeightPx=%1 (%2 mm)")
             .arg(result.usedHeightPx, 0, 'f', 2)
             .arg(result.usedHeightPx * 25.4 / 96.0, 0, 'f', 2));

    result.ok = true;
    return result;
}
