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

#include "oexportartboards.h"
#include "oexportgeometry.h"
#include "oexportsheets.h"

#include "../vlayout/vlayoutpoint.h"

#include "model/Piece.h"
#include "nesting/Bin.h"
#include "nesting/Engine.h"
#include "nesting/Footprint.h"
#include "nesting/TiledEngine.h"

#include "../vmisc/def.h"

#include <QObject>

#include <cmath>
#include <limits>
#include <utility>
#include <vector>

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

/// Every piece placed. The engine marks an unplaced one with a negative
/// rotation and reports nothing else — no status, no exception.
auto AllPlaced(const std::vector<Placement> &placements) -> bool
{
    for (const Placement &p : placements)
    {
        if (p.rotation < 0.0)
        {
            return false;
        }
    }
    return true;
}

/// The engine's raw placements, turned into what the mode consumes.
///
/// The footprint's own frame: the min corner gives the origin, and the max
/// corner gives the extent. Both come from the polygon the engine actually
/// placed — never from the layers, which would be a second union and not
/// necessarily the same one.
auto ToExportPlacements(const std::vector<Placement> &placements,
                        const std::vector<Clipper2Lib::Path64> &polygons)
    -> QVector<OExportPlacement>
{
    QVector<OExportPlacement> out;
    out.reserve(static_cast<int>(placements.size()));

    for (size_t i = 0; i < placements.size(); ++i)
    {
        int64_t xMin = polygons[i].front().x;
        int64_t yMin = polygons[i].front().y;
        int64_t xMax = xMin;
        int64_t yMax = yMin;
        for (const auto &point : polygons[i])
        {
            xMin = std::min(xMin, point.x);
            yMin = std::min(yMin, point.y);
            xMax = std::max(xMax, point.x);
            yMax = std::max(yMax, point.y);
        }

        OExportPlacement placement;
        placement.x = placements[i].x;
        placement.y = placements[i].y;
        placement.rotation = placements[i].rotation;
        placement.originX = static_cast<double>(xMin) / kTrameScale;
        placement.originY = static_cast<double>(yMin) / kTrameScale;
        placement.extentX = static_cast<double>(xMax - xMin) / kTrameScale;
        placement.extentY = static_cast<double>(yMax - yMin) / kTrameScale;

        // The outline, brought into the frame the extent describes: origin at local (0, 0) and
        // pixels. It is what a hand's limits are measured against — see the note on the field.
        placement.outline.reserve(static_cast<int>(polygons[i].size()));
        for (const auto &point : polygons[i])
        {
            placement.outline.append(QPointF(static_cast<double>(point.x - xMin) / kTrameScale,
                                             static_cast<double>(point.y - yMin) / kTrameScale));
        }

        out.append(placement);
    }

    return out;
}

/// How many sheets a plan prints.
///
/// The rule belongs to OSTARA, not to Trame: `Bin.h` says the grid is not part
/// of the bin model, and the count is read off the placements by the one
/// function that counts them. Trame is asked for two plans and told nothing
/// about pages — it is here, and only here, that they are compared.
auto PrintedSheets(const QVector<OExportPlacement> &placements,
                   double usedHeightPx, const OExportNestingParams &params) -> int
{
    const double mmPerPx = 1.0 / ToPixel(1.0, Unit::Mm);

    const OExportArtboards::Grid grid =
        OExportArtboards::Compute(params.binWidthPx, usedHeightPx,
                                  params.artboardHeightPx * mmPerPx,
                                  params.artboardCount);

    if (!grid.valid)
    {
        return (std::numeric_limits<int>::max)();
    }

    // The cover page's reserved block is handed over like the grid itself, and it is the SAME block
    // for both candidate plans: it is therefore a constant in the comparison below, and the engine's
    // choice can only move through the placements — which is what it is there for.
    return OExportSheets::Compute(grid, placements, params.coverZonePx).count;
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

    QHash<QString, QVector<double>> anglesByKey;

    // Les touches qui vont AU BORD — celles d'une pièce au pli, sur du tissu plié. Une seconde
    // carte, à côté des angles, parce que les deux se remplissent au même endroit et se relisent au
    // même endroit : l'ancrage et la rotation d'une pièce au pli sont une seule décision.
    QHash<QString, Anchor> anchorByKey;

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
            // Le contour MAPPÉ : c'est lui qui porte le miroir d'un plan de coupe. Sur un run
            // d'impression la matrice de la pièce est l'identité, donc les deux getters rendent le
            // même contour et rien ne bouge.
            node.polyline = ToPath64(piece.piece.GetMappedExternalContourPoints(), piece.alignDelta);
            if (node.polyline.size() < 3)
            {
                continue;
            }
            enginePiece.contourNodes.append(node);

            engineLayer.pieces.append(enginePiece);

            if (params.cuttingPlan)
            {
                // Keyed, because the footprint is what the engine constrains — a group is nested as
                // one block, and its members share the key.
                double foldAngle = 0.0;
                if (params.foldedFabric && piece.onFold)
                {
                    // ── Une pièce au pli, sur du tissu plié ──────────────────────────────────────
                    //
                    // Elle n'a plus qu'UNE rotation : celle qui met sa foldline verticale, le corps
                    // dans le tissu — l'autre le mettrait dehors. Et elle est **collée au bord** :
                    // sa foldline doit tomber EXACTEMENT sur le pli du tissu, et à cinq millimètres
                    // près, la pièce dépliée est fausse de dix.
                    //
                    // ⚠️ Le pli l'emporte sur la grainline, ici et pas ailleurs : `PieceDirection`
                    // préfère la grainline quand elle est allumée, et c'est juste partout sauf sur
                    // une pièce qu'on coupe AU PLI — c'est le pli du tissu qui la porte.
                    if (!OExportCuttingDirection::FoldAngle(piece.piece, &foldAngle))
                    {
                        // Aucune des deux rotations ne met le pli sur un bord de sa propre pièce : il
                        // n'y a pas de rotation à trouver, et poser la pièce quand même serait un
                        // plan faux qui a l'air fini. On le dit, et on nomme la pièce.
                        result.error = QObject::tr("The piece \"%1\" (id %2) is cut on the fold, but "
                                                   "its fold line is not one of its own edges. Check "
                                                   "its fold line in the Details mode.")
                                           .arg(piece.name)
                                           .arg(piece.id);
                        return result;
                    }
                    anglesByKey.insert(piece.key, QVector<double>{foldAngle});
                    anchorByKey.insert(piece.key, Anchor::LeftWall);
                }
                else
                {
                    anglesByKey.insert(piece.key,
                                       OExportCuttingDirection::AllowedAngles(piece.direction, params.oneWayLayout));
                }
            }
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

    // The direction of each piece, by footprint key — and in the FOOTPRINT's order, not the layer's:
    // `Footprint::compute` regroups and reorders what it is given, and the engine's per-piece
    // constraints have to line up with what the engine will actually receive.
    //
    // Empty on a print run, and an empty list is a piece nobody constrained: the engine keeps its
    // whole sweep, exactly as it did before this parameter existed.
    std::vector<std::vector<double>> allowedAngles;

    // Les pièces collées au bord, dans le MÊME ordre et pour la même raison : le moteur les reçoit
    // parallèlement à ce qu'il nidifie, et `Footprint::compute` regroupe et réordonne.
    // Vide sur un run d'impression, et sur tout plan qui n'a rien collé au bord — et vide, le
    // moteur place exactement comme il l'a toujours fait.
    std::vector<Anchor> anchors;

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
    // The zones the run has to keep clear — the cover page's block, and nothing else so far.
    // Handed to BOTH engines through the one Bin, because that is what the arbitration below
    // compares: two plans measured over ONE obstacle set.
    std::vector<Bin::Zone> zones;
    if (!params.coverZonePx.isEmpty())
    {
        zones.push_back({params.coverZonePx.x(), params.coverZonePx.y(),
                         params.coverZonePx.width(), params.coverZonePx.height()});
    }

    // **The explicit-list constructor, and not the corner one.** An empty list is the absence of
    // a zone; the corner constructor appends one of its own the moment a dimension is non-zero,
    // which is how the strip engine's A0 emulation works and is not what a run without a cover
    // page asks for.
    const Bin bin(params.binWidthPx, params.marginPx, std::move(zones));
    Engine::Config config;
    config.gap = params.gapPx;

    // The grid, and only on a tiled run. It is what lets the engine adjust the
    // plan once it is placed — keeping the footprint borders off the cut lines,
    // and spending the dead band the last artboard row keeps for free. Left at
    // zero, the engine places exactly as it did before tiling existed.
    config.artboardCount  = params.artboardCount;
    config.artboardHeight = params.artboardHeightPx;

    // **The run has to be able to be given up.** It is the whole reason the dialog in front of it
    // can be closed: the engine has no abort hook of its own, so one is handed to it, and it reads
    // it between its iterations. Null — a caller with nobody watching — leaves it as it was.
    config.abandoned = params.abandoned;

    if (params.cuttingPlan)
    {
        allowedAngles.reserve(footprint.keys.size());
        anchors.reserve(footprint.keys.size());
        for (const QString &key : footprint.keys)
        {
            const QVector<double> angles = anglesByKey.value(key);
            allowedAngles.emplace_back(angles.cbegin(), angles.cend());
            anchors.push_back(anchorByKey.value(key, Anchor::Free));
        }
    }

    const Engine engine{config};
    bool abandoned = false;
    std::vector<Placement> placements =
        engine.place(bin, footprint.polygons, allowedAngles, anchors, &abandoned);

    if (abandoned)
    {
        // **What it holds is a half-built plan, and it is not read.** Every check below would be
        // asking a question about a result that was never produced.
        result.abandoned = true;
        return result;
    }

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

    // The engine marks an unplaced piece with a negative rotation and reports
    // nothing else — no status, no exception.
    if (!AllPlaced(placements))
    {
        result.error = QObject::tr("The document is too narrow: some pieces could not be placed.");
        return result;
    }

    // ── L'arbitrage : deux moteurs, le moins de feuilles gagne ────────────
    //
    // Sur un run TUILÉ, un second moteur entre en jeu. Le moteur de bande
    // minimise une HAUTEUR : la page n'existe pas pour lui, et la grille est une
    // grille posée après coup sur le résultat — mesuré sur un patron réel,
    // 1088,050 mm pour une frontière de rangée à 1036, payés d'une rangée
    // entière. Le moteur Tiled paie en PAGES : les grosses pièces d'abord, puis
    // les petites glissées dans les trous au VRAI contour.
    //
    // Les deux plans sont calculés et c'est celui qui imprime le MOINS de
    // feuilles qui est gardé. Le moteur de bande garde donc son mot à dire, et
    // le résultat ne peut pas être pire qu'avant.
    //
    // ⚠️ Le prédicat est `artboardHeightPx > 0.0` — celui d'Engine.cpp — et
    // surtout PAS `artboardCount == 0`, qui n'arrive jamais : `Columns()` rend
    // 1 sur un roulé, jamais 0. Un moteur qui testerait ça se tromperait sur
    // TOUS les runs actuels.
    // ⚠️ **Et jamais quand une pièce est collée au bord.** Le moteur Tiled ne connaît ni les angles
    // par pièce ni l'ancrage : il tourne au quart de tour et pose librement. Lui laisser gagner
    // l'arbitrage jetterait un plan juste pour un plan qui imprime moins de feuilles — et rien ne
    // le dirait. Aujourd'hui il est déjà hors jeu (un plan de coupe ne pose pas d'artboard, donc
    // `artboardHeightPx` vaut zéro), mais c'est une dépendance implicite : le jour où un plan de
    // coupe recevra un cadre, elle tomberait sans bruit.
    if (params.artboardHeightPx > 0.0 && params.artboardCount > 0 && anchors.empty())
    {
        TiledEngine::Spec spec;
        spec.sheetWidth    = params.binWidthPx / params.artboardCount;
        spec.sheetHeight   = params.artboardHeightPx;
        spec.gap           = params.gapPx;
        spec.allowRotation = true;
        // The second engine is asked the same question as the first, and for the same reason: a
        // run given up during the tiled pass has to stop there too.
        spec.abandoned     = params.abandoned;

        // ── TROIS ORDRES, TROIS PLANS, ET C'EST ICI QU'ON CHOISIT ─────────────
        //
        // Le moteur est glouton : il pose les pièces dans un ordre, et ne revient jamais. La
        // première prend la meilleure place, la dernière se contente du reste — donc **l'ordre
        // décide de tout le plan**. Un seul ordre, c'est un seul plan, et rien ne peut rattraper
        // une page qu'il a laissée.
        //
        // On demande donc trois plans au même moteur, à partir des MÊMES pièces, et on garde
        // celui qui imprime le moins de feuilles — compté ici, avec `OExportSheets::Compute`,
        // qui reste le seul endroit où cette règle est écrite.
        //
        // **Le plan historique reste la référence** : un autre ne le remplace que s'il imprime
        // STRICTEMENT moins. Si aucun ne fait mieux, rien ne change — au plan près.
        const TiledEngine::Spec::Order orders[] = {TiledEngine::Spec::Order::HolderArea,
                                             TiledEngine::Spec::Order::HolderHeight,
                                             TiledEngine::Spec::Order::HolderWidth};

        const QVector<OExportPlacement> stripPlan =
            ToExportPlacements(placements, footprint.polygons);
        int bestSheets = PrintedSheets(
            stripPlan, Engine::UsedHeight(placements, footprint.polygons, bin), params);

        for (const TiledEngine::Spec::Order order : orders)
        {
            spec.order = order;

            bool tiledAbandoned = false;
            const std::vector<Placement> tiled =
                TiledEngine(spec).place(bin, footprint.polygons, &tiledAbandoned);

            if (tiledAbandoned)
            {
                result.abandoned = true;
                return result;
            }

            if (tiled.size() != footprint.polygons.size() || !AllPlaced(tiled))
            {
                continue;   // ce départ-là n'a pas su tout poser : il ne concourt pas
            }

            const int tiledSheets = PrintedSheets(
                ToExportPlacements(tiled, footprint.polygons),
                Engine::UsedHeight(tiled, footprint.polygons, bin), params);

            if (tiledSheets < bestSheets)
            {
                bestSheets  = tiledSheets;
                placements  = tiled;
            }
        }

    }

    result.placements = ToExportPlacements(placements, footprint.polygons);
    result.usedHeightPx = Engine::UsedHeight(placements, footprint.polygons, bin);

    result.ok = true;
    return result;
}
