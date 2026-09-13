/************************************************************************
 **
 **  @file   ogradedgeometry.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Implementation of the reusable graded alignment/offsets pass (see
 **          header). The body mirrors the "positions" part of
 **          MainWindow::ApplyGradedLayoutOffsets so canvas and export never
 **          drift apart.
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

#include "ogradedgeometry.h"

#include "mainwindow_graded_helpers.h"
#include "core/opiececommonpoint.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vlayout/vgraphicsfillitem.h"
#include "../vgeometry/vpointf.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vwidgets/vpiecegrainline.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QHash>
#include <QList>
#include <QMap>
#include <QPen>
#include <QRectF>
#include <QtMath>
#include <QtNumeric>

namespace OGraded
{
//---------------------------------------------------------------------------------------------------------------------
auto AnchorPoint(const QRectF &rect, int h, int v) -> QPointF
{
    const qreal hFrac = h == 1 ? 0.0 : (h == 3 ? 1.0 : 0.5);
    const qreal vFrac = v == 1 ? 0.0 : (v == 3 ? 1.0 : 0.5);
    return QPointF(rect.left() + rect.width() * hFrac, rect.top() + rect.height() * vFrac);
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// X of a node in pattern coordinates for the current size, or NaN when it cannot be resolved.
auto NodeX(quint32 nodeId, const VContainer *pattern) -> qreal
{
    try
    {
        const QSharedPointer<VPointF> point = pattern->GeometricObject<VPointF>(nodeId);
        return point.isNull() ? qQNaN() : point->x();
    }
    catch (const VExceptionBadId &)
    {
        return qQNaN();
    }
}

/// The member whose copy of the common point every size is brought onto.
auto ReferenceMemberId(const OPieceGroup &group) -> quint32
{
    if (group.alignPointMemberId != NULL_ID)
    {
        return group.alignPointMemberId;
    }
    // A group written before the common point was stored carries no member: pick deterministically.
    quint32 lowest = NULL_ID;
    for (quint32 memberId : group.pieceIds)
    {
        if (lowest == NULL_ID || memberId < lowest)
        {
            lowest = memberId;
        }
    }
    return lowest;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
auto UnitAlignPointX(quint32 pieceId, const VPiece &piece, const VContainer *pattern) -> qreal
{
    if (pattern == nullptr || pieceId == NULL_ID)
    {
        return qQNaN();
    }

    // A member of a real group is aligned through its group: its own point stays stored but inert.
    const quint32 groupId = pattern->GetPieceGroupId(pieceId);
    const auto *groups = pattern->DataPieceGroups();
    if (groupId != NULL_ID && groups != nullptr)
    {
        const auto found = groups->constFind(groupId);
        if (found != groups->cend() && found.value().pieceIds.size() >= kMinGroupMembers)
        {
            const OPieceGroup &group = found.value();
            if (!group.alignOnPoint || group.alignPointSourceId == NULL_ID)
            {
                return qQNaN();
            }
            if (pieceId != ReferenceMemberId(group))
            {
                return qQNaN();  // only the reference member carries the X of the whole layer
            }
            const quint32 nodeId =
                OPieceCommonPoint::PieceNodeWithSource(piece, group.alignPointSourceId, pattern);
            return nodeId == NULL_ID ? qQNaN() : NodeX(nodeId, pattern);
        }
    }

    const quint32 nodeId = piece.GetGradedAlignPointId();
    if (nodeId == NULL_ID || piece.GetPath().indexOfNode(nodeId) == -1)
    {
        return qQNaN();  // no point, or it left the main path: silent fallback
    }
    return NodeX(nodeId, pattern);
}

//---------------------------------------------------------------------------------------------------------------------
auto UnitLayerDelta(const AlignRule &rule, const QRectF &unitRect, const QRectF &layerRect,
                    qreal layerPointX) -> QPointF
{
    const QPointF unitAnchor = AnchorPoint(unitRect, rule.alignH, rule.alignV);
    const QPointF layerAnchor = AnchorPoint(layerRect, rule.alignH, rule.alignV);

    // The forced point only ever takes over the horizontal component; the vertical anchor stands.
    const bool forced = !qIsNaN(rule.refPointX) && !qIsNaN(layerPointX);
    const qreal x = forced ? rule.refPointX - layerPointX : unitAnchor.x() - layerAnchor.x();
    return QPointF(x, unitAnchor.y() - layerAnchor.y());
}

//---------------------------------------------------------------------------------------------------------------------
void ApplyOffsets(const QList<QGraphicsItem *> &items, const OSizerange &range, const VContainer *pattern)
{
    if (items.isEmpty() || pattern == nullptr)
    {
        return;
    }

    // Idempotent re-apply: reset every piece item to its frozen base first, so the alignment
    // deltas and the stored offsets never stack on already-offset positions.
    for (auto *item : items)
    {
        if (GradedPieceId(item) != 0 && !item->data(kGradedBasePosRole).isNull())
        {
            item->setPos(item->data(kGradedBasePosRole).toPointF());
        }
    }

    QHash<quint32, QList<QGraphicsItem *>> byPiece;
    for (auto *baseItem : items)
    {
        if (GradedPieceId(baseItem) != 0)
        {
            byPiece[GradedPieceId(baseItem)].append(baseItem);
        }
    }

    // Group membership: a member piece is never aligned/offset as an individual — the group is
    // the unit (same rule as Details). Groups with < 2 members are ignored (like the canvas).
    QHash<quint32, quint32> pieceGroupOf;  // member piece id -> group id
    const auto *groups = pattern->DataPieceGroups();
    if (groups != nullptr)
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            if (git.value().pieceIds.size() < 2)
            {
                continue;
            }
            for (quint32 memberId : git.value().pieceIds)
            {
                pieceGroupOf.insert(memberId, git.key());
            }
        }
    }

    // Pass 1 — align each non-grouped piece's size layers (additive delta, base preserved).
    const auto *piecesMap = pattern->DataPieces();
    for (auto it = byPiece.cbegin(); it != byPiece.cend(); ++it)
    {
        if (pieceGroupOf.contains(it.key()))
        {
            continue;
        }
        const QList<QGraphicsItem *> &pieceItems = it.value();
        if (pieceItems.size() < 2)
        {
            continue;  // a single layer keeps its Details (mx/my) position
        }

        int alignH = 2;
        int alignV = 2;
        if (piecesMap != nullptr && piecesMap->contains(it.key()))
        {
            const VPiece &piece = piecesMap->value(it.key());
            alignH = piece.GetGradedAlignH();
            alignV = piece.GetGradedAlignV();
        }

        // Forced point: the reference layer carries the X every other size is brought onto.
        AlignRule rule;
        rule.alignH = alignH;
        rule.alignV = alignV;
        for (const auto *item : pieceItems)
        {
            if (GradedIsReference(item))
            {
                rule.refPointX = GradedAlignPointX(item);
                break;
            }
        }

        QVector<QRectF> rects;
        rects.reserve(pieceItems.size());
        QRectF unionRect;
        for (const auto *item : pieceItems)
        {
            const QRectF r = item->sceneBoundingRect();
            rects.append(r);
            unionRect = unionRect.isNull() ? r : unionRect.united(r);
        }

        for (int i = 0; i < pieceItems.size(); ++i)
        {
            const QPointF delta =
                UnitLayerDelta(rule, unionRect, rects.at(i), GradedAlignPointX(pieceItems.at(i)));
            if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
            {
                pieceItems.at(i)->setPos(pieceItems.at(i)->pos() + delta);
            }
        }
    }

    // Pass 2 — align each GROUP as a rigid unit: per layer the union of the members, cross-layer
    // overall union, then shift every member of a layer by the same anchor delta.
    if (groups != nullptr)
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            const OPieceGroup &group = git.value();
            if (group.pieceIds.size() < 2)
            {
                continue;
            }

            QMap<qreal, QList<QGraphicsItem *>> itemsByLayer;  // z -> items
            QMap<qreal, QRectF> rectByLayer;                   // z -> pre-move member union
            QMap<qreal, qreal> pointXByLayer;                  // z -> forced point X (first one wins)
            qreal refLayerZ = qQNaN();                         // z of the reference (smallest) size
            int totalItems = 0;
            for (quint32 memberId : group.pieceIds)
            {
                const auto found = byPiece.constFind(memberId);
                if (found == byPiece.cend())
                {
                    continue;
                }
                for (auto *item : *found)
                {
                    const qreal z = item->zValue();
                    itemsByLayer[z].append(item);
                    ++totalItems;
                    const QRectF r = item->sceneBoundingRect();
                    QRectF &lr = rectByLayer[z];
                    lr = lr.isNull() ? r : lr.united(r);

                    const qreal px = GradedAlignPointX(item);
                    if (!qIsNaN(px) && !pointXByLayer.contains(z))
                    {
                        pointXByLayer.insert(z, px);
                    }
                    if (GradedIsReference(item))
                    {
                        refLayerZ = z;
                    }
                }
            }
            if (totalItems < 2)
            {
                continue;
            }

            QRectF overallRect;
            for (auto it = rectByLayer.cbegin(); it != rectByLayer.cend(); ++it)
            {
                overallRect = overallRect.isNull() ? it.value() : overallRect.united(it.value());
            }

            AlignRule rule;
            rule.alignH = group.alignH;
            rule.alignV = group.alignV;
            if (!qIsNaN(refLayerZ))
            {
                rule.refPointX = pointXByLayer.value(refLayerZ, qQNaN());
            }

            for (auto it = rectByLayer.cbegin(); it != rectByLayer.cend(); ++it)
            {
                const QPointF delta = UnitLayerDelta(rule, overallRect, it.value(),
                                                     pointXByLayer.value(it.key(), qQNaN()));
                if (qFuzzyIsNull(delta.x()) && qFuzzyIsNull(delta.y()))
                {
                    continue;
                }
                for (auto *item : itemsByLayer[it.key()])
                {
                    item->setPos(item->pos() + delta);
                }
            }
        }
    }

    // Per-range content offsets (drag deltas persisted in the size range). A group member only
    // ever moves through its group's offset — never through a per-piece one.
    for (auto pc = range.pieceContent.cbegin(); pc != range.pieceContent.cend(); ++pc)
    {
        if (pieceGroupOf.contains(pc.key()))
        {
            continue;
        }
        const auto found = byPiece.constFind(pc.key());
        if (found == byPiece.cend())
        {
            continue;
        }
        const QPointF delta(pc.value().dx, pc.value().dy);
        for (auto *item : *found)
        {
            item->setPos(item->pos() + delta);
        }
    }

    if (groups != nullptr)
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            const auto content = range.groupContent.constFind(git.key());
            if (content == range.groupContent.cend())
            {
                continue;
            }
            const QPointF delta(content->dx, content->dy);
            for (quint32 memberId : git.value().pieceIds)
            {
                const auto found = byPiece.constFind(memberId);
                if (found == byPiece.cend())
                {
                    continue;
                }
                for (auto *item : *found)
                {
                    item->setPos(item->pos() + delta);
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// The design length of a grainline, in pixels. A piece's length is a formula — the one
/// VAbstractPiece::FindGrainlineGeometry evaluates — a group's is a literal. An empty or
/// unusable value falls back on the 10 cm the Details holder also uses.
auto GrainlineLengthPx(const QString &formula, bool isGroup, const VContainer *pattern, const Unit &unit)
    -> qreal
{
    qreal value = 0.0;
    if (isGroup || pattern == nullptr)
    {
        bool ok = false;
        value = formula.toDouble(&ok);
        if (!ok)
        {
            value = 0.0;
        }
    }
    else
    {
        try
        {
            Calculator cal;
            value = cal.EvalFormula(pattern->DataVariables(), formula);
        }
        catch (qmu::QmuParserError &e)
        {
            Q_UNUSED(e)
            value = 0.0;
        }
    }

    if (value <= 0.0)
    {
        value = 10.0;
    }
    return ToPixel(value, unit);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto ResolveGrainline(const VGrainlineData &geom, const OSizerange::GrainlineOverride &ov, bool isGroup,
                      const VContainer *pattern, const Unit &unit, const QPointF &fallbackCenter)
    -> GrainlinePlacement
{
    GrainlinePlacement placement;

    placement.pos = ov.valid ? ov.pos : geom.GetPos();
    if (qFuzzyIsNull(placement.pos.x()) && qFuzzyIsNull(placement.pos.y()))
    {
        placement.pos = fallbackCenter; // the grainline was never placed: sit it on the owner
    }

    placement.lengthPx = (ov.valid && ov.lengthPx > 0.0) ? ov.lengthPx
                                                         : GrainlineLengthPx(geom.GetLength(), isGroup,
                                                                             pattern, unit);
    return placement;
}

//---------------------------------------------------------------------------------------------------------------------
auto GrainlineRotation(const QString &formula, bool isGroup, const VContainer *pattern) -> qreal
{
    if (formula.isEmpty())
    {
        return 0.0;
    }

    // A group's grainline carries a literal, a piece's carries a formula: the group has no
    // variables of its own to evaluate one against.
    if (isGroup || pattern == nullptr)
    {
        bool ok = false;
        const qreal value = formula.toDouble(&ok);
        return ok ? value : 0.0;
    }

    Calculator cal;
    const qreal value = cal.EvalFormula(pattern->DataVariables(), formula);
    return (qIsNaN(value) || qIsInf(value)) ? 0.0 : value;
}

//---------------------------------------------------------------------------------------------------------------------
auto MakeGrainlineItem(QGraphicsItem *parent, const VGrainlineData &geom, const GrainlinePlacement &placement,
                       qreal rotationDeg) -> QGraphicsItem *
{
    const VPieceGrainline grainline(placement.pos, placement.lengthPx, qDegreesToRadians(rotationDeg),
                                    geom.GetArrowType());
    const GrainlineShape shape = grainline.Shape();
    if (shape.isEmpty())
    {
        return nullptr;
    }

    auto *item = new VGraphicsFillItem(VLayoutPiece::GrainlinePath(shape), parent);
    item->SetCustomPen(true);
    QPen pen = item->pen();
    pen.setWidthF(VAbstractApplication::VApp()->Settings()->WidthHairLine());
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    item->setPen(pen);
    return item;
}
}  // namespace OGraded
