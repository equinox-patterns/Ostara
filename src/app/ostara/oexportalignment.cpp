/************************************************************************
 **
 **  @file   oexportalignment.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — line the sizes of a piece up before they are unioned
 **          into one footprint.
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

#include "oexportalignment.h"

#include "oexportgeometry.h"
#include "ogradedgeometry.h"

#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QHash>
#include <QMap>
#include <QRectF>
#include <QtNumeric>

namespace
{
/// Where a size sits in the layers: which layer, and which piece inside it.
struct SizeSlot
{
    int layer{0};
    int piece{0};
};

/// The outline a size occupies, taken from the very points that will be unioned.
/// Measuring the footprint against anything else would align the wrong thing.
auto ContourRect(const OExportPieceGeometry &geometry) -> QRectF
{
    const QVector<VLayoutPoint> points = geometry.piece.GetExternalContourPoints();
    if (points.isEmpty())
    {
        return {};
    }

    qreal xMin = points.first().x();
    qreal xMax = xMin;
    qreal yMin = points.first().y();
    qreal yMax = yMin;
    for (const VLayoutPoint &point : points)
    {
        xMin = qMin(xMin, point.x());
        xMax = qMax(xMax, point.x());
        yMin = qMin(yMin, point.y());
        yMax = qMax(yMax, point.y());
    }

    return QRectF(QPointF(xMin, yMin), QPointF(xMax, yMax));
}

/// Piece id → where that size sits in every layer. "slots" is a Qt macro, hence
/// the name: a variable called `slots` does not compile.
void IndexLayers(const QVector<OExportSizeLayer> &layers, QHash<quint32, QVector<SizeSlot>> &slotsByPiece)
{
    for (int l = 0; l < layers.size(); ++l)
    {
        const OExportSizeLayer &layer = layers.at(l);
        for (int p = 0; p < layer.pieces.size(); ++p)
        {
            // ⚠️ **Un id nul n'est pas une pièce du modèle**, et n'a donc rien à aligner.
            //
            // C'est la marque d'une géométrie DÉRIVÉE — le morceau d'une pièce, découpé aux bords
            // d'un tapis de plotter (voir `FragmentsOf`, dans oexportgeometry.cpp). Tous les
            // morceaux d'un run portent cet id, et les indexer ici les faisait passer pour **une
            // seule pièce présente N fois** : la garde « moins de deux positions » de chaque passe
            // laissait alors passer l'alignement, et un écart était écrit sur chacun d'eux.
            //
            // Or la mise en page d'un run plotter les place **sans** cet écart, et le canvas
            // l'ajoute par-dessus : les morceaux partaient de travers — hors du canevas, et les uns
            // sur les autres — pendant que la taille du canevas, elle, se calculait sans l'écart.
            //
            // L'alignement sert à faire correspondre **une même pièce d'une taille à l'autre**.
            // Un morceau n'existe qu'à une taille.
            const quint32 id = layer.pieces.at(p).id;
            if (id == 0)
            {
                continue;
            }

            slotsByPiece[id].append({l, p});
        }
    }
}

/// The group of every member, for groups that are actually a block (two members up).
auto GroupMembership(const VContainer *container) -> QHash<quint32, quint32>
{
    QHash<quint32, quint32> pieceGroupOf;

    const QHash<quint32, OPieceGroup> *groups = container->DataPieceGroups();
    if (groups == nullptr)
    {
        return pieceGroupOf;
    }

    for (auto it = groups->cbegin(); it != groups->cend(); ++it)
    {
        if (it.value().pieceIds.size() < OGraded::kMinGroupMembers)
        {
            continue;
        }
        for (const quint32 memberId : it.value().pieceIds)
        {
            pieceGroupOf.insert(memberId, it.key());
        }
    }

    return pieceGroupOf;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OExportAlignment::Apply(QVector<OExportSizeLayer> &layers, const VContainer *container)
{
    if (container == nullptr || layers.isEmpty())
    {
        return;
    }

    // Apply() may be called on freshly collected layers, but never rely on it: start
    // from an unshifted geometry so a second call cannot stack on the first.
    for (OExportSizeLayer &layer : layers)
    {
        for (OExportPieceGeometry &geometry : layer.pieces)
        {
            geometry.alignDelta = QPointF(0.0, 0.0);
        }
    }

    const QHash<quint32, quint32> pieceGroupOf = GroupMembership(container);
    const QHash<quint32, VPiece> *pieces = container->DataPieces();

    // ── Pass 1 — every piece outside a group, on its own alignment rule ──────────
    QHash<quint32, QVector<SizeSlot>> slotsByPiece;
    IndexLayers(layers, slotsByPiece);

    for (auto it = slotsByPiece.cbegin(); it != slotsByPiece.cend(); ++it)
    {
        if (pieceGroupOf.contains(it.key()) || it.value().size() < 2)
        {
            continue;
        }

        OGraded::AlignRule rule;
        if (pieces != nullptr && pieces->contains(it.key()))
        {
            const VPiece &piece = pieces->value(it.key());
            rule.alignH = piece.GetGradedAlignH();
            rule.alignV = piece.GetGradedAlignV();
        }

        QVector<QRectF> rects;
        rects.reserve(it.value().size());
        QRectF unionRect;
        for (const SizeSlot &slot : it.value())
        {
            const QRectF rect = ContourRect(layers.at(slot.layer).pieces.at(slot.piece));
            rects.append(rect);
            unionRect = unionRect.isNull() ? rect : unionRect.united(rect);
        }

        // Slots are indexed in increasing layer order, so the first one is the smallest size —
        // the same reference the canvas uses. It carries the X every other size is brought onto.
        rule.refPointX = layers.at(it.value().first().layer).pieces.at(it.value().first().piece).alignPointX;

        for (int i = 0; i < it.value().size(); ++i)
        {
            const SizeSlot &slot = it.value().at(i);
            const qreal layerPointX = layers.at(slot.layer).pieces.at(slot.piece).alignPointX;
            layers[slot.layer].pieces[slot.piece].alignDelta +=
                OGraded::UnitLayerDelta(rule, unionRect, rects.at(i), layerPointX);
        }
    }

    // ── Pass 2 — a group moves as one rigid block, on the group's rule ───────────
    const QHash<quint32, OPieceGroup> *groups = container->DataPieceGroups();
    if (groups == nullptr)
    {
        return;
    }

    for (auto git = groups->cbegin(); git != groups->cend(); ++git)
    {
        const OPieceGroup &group = git.value();
        if (group.pieceIds.size() < OGraded::kMinGroupMembers)
        {
            continue;
        }

        // Per layer, the union of the members present there — the group's extent for
        // that size.
        QMap<int, QVector<SizeSlot>> membersByLayer;
        QMap<int, QRectF> rectByLayer;
        QMap<int, qreal> pointXByLayer;  // layer -> forced point X (first one wins)
        int memberCount = 0;

        for (int l = 0; l < layers.size(); ++l)
        {
            const OExportSizeLayer &layer = layers.at(l);
            for (int p = 0; p < layer.pieces.size(); ++p)
            {
                if (!group.pieceIds.contains(layer.pieces.at(p).id))
                {
                    continue;
                }

                const QRectF rect = ContourRect(layer.pieces.at(p));
                rectByLayer[l] = rectByLayer[l].isNull() ? rect : rectByLayer[l].united(rect);
                membersByLayer[l].append({l, p});
                ++memberCount;

                const qreal px = layer.pieces.at(p).alignPointX;
                if (!qIsNaN(px) && !pointXByLayer.contains(l))
                {
                    pointXByLayer.insert(l, px);
                }
            }
        }

        if (memberCount < OGraded::kMinGroupMembers)
        {
            continue;
        }

        QRectF overallRect;
        for (auto it = rectByLayer.cbegin(); it != rectByLayer.cend(); ++it)
        {
            overallRect = overallRect.isNull() ? it.value() : overallRect.united(it.value());
        }

        OGraded::AlignRule rule;
        rule.alignH = group.alignH;
        rule.alignV = group.alignV;
        // rectByLayer is keyed by layer index: its first entry is the smallest size present.
        if (!rectByLayer.isEmpty())
        {
            rule.refPointX = pointXByLayer.value(rectByLayer.firstKey(), qQNaN());
        }

        for (auto it = rectByLayer.cbegin(); it != rectByLayer.cend(); ++it)
        {
            const QPointF delta = OGraded::UnitLayerDelta(rule, overallRect, it.value(),
                                                          pointXByLayer.value(it.key(), qQNaN()));
            for (const SizeSlot &slot : membersByLayer.value(it.key()))
            {
                layers[slot.layer].pieces[slot.piece].alignDelta += delta;
            }
        }
    }
}
