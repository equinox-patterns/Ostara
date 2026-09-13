/************************************************************************
 **
 **  @file   oexportcanvas.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — draw the bin and the nested content.
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

#include "oexportcanvas.h"

#include "ogradedgeometry.h"
#include "oexportgeometry.h"
#include "oexportnesting.h"

#include "../vlayout/vgraphicsfillitem.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/def.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vpiecegrainline.h"

#include <QBrush>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QHash>
#include <QPen>
#include <QTransform>
#include <QtMath>

namespace
{
/// Grainline, fold mark and its label are drawn by GetItem on every size, and only the
/// smallest one may show them — that is the rule the Graded canvas applies to its ghost
/// layers, and the layers of a range are no different. Same three types, same hiding.
void HidePerSizeDecor(QGraphicsItem *item)
{
    if (item == nullptr)
    {
        return;
    }

    const QList<QGraphicsItem *> children = item->childItems();
    for (QGraphicsItem *child : children)
    {
        if (child->type() == VGraphicsFillItem::Type || child->type() == VGraphicsFoldLineItem::Type
            || child->type() == QGraphicsSimpleTextItem::Type)
        {
            child->hide();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// The range's grainline override for one owner, or an invalid one when the range holds
/// none. The mode reads the range and never writes to it: an override exists only for an
/// owner the user actually moved in Graded, and without one the owner's own geometry stands.
auto GrainlineOverrideOf(const OSizerange *range, quint32 ownerId, bool isGroup)
    -> OSizerange::GrainlineOverride
{
    if (range == nullptr)
    {
        return {};
    }

    if (isGroup)
    {
        const auto it = range->groupContent.constFind(ownerId);
        return it == range->groupContent.cend() ? OSizerange::GrainlineOverride{} : it.value().grainline;
    }

    const auto it = range->pieceContent.constFind(ownerId);
    return it == range->pieceContent.cend() ? OSizerange::GrainlineOverride{} : it.value().grainline;
}

//---------------------------------------------------------------------------------------------------------------------
/// Whether a piece belongs to a group that is nested as one rigid block. Below two
/// members a group is not a block — the canvas, the export and the container all draw
/// that line in the same place (OGraded::kMinGroupMembers).
auto IsGroupedMember(const VContainer *pattern, quint32 groupId) -> bool
{
    if (groupId == 0 || pattern == nullptr)
    {
        return false;
    }

    const auto *groups = pattern->DataPieceGroups();
    if (groups == nullptr)
    {
        return false;
    }

    const auto it = groups->constFind(groupId);
    return it != groups->cend() && it.value().pieceIds.size() >= OGraded::kMinGroupMembers;
}

//---------------------------------------------------------------------------------------------------------------------
/// Give the reference layer's piece the grainline the range asks for.
///
/// GetItem builds that grainline from the geometry the piece carries, which is the
/// Details-mode placement — clamped to the piece, and blind to the range. The range is
/// the authority here: its override carries the origin and the length, and the piece's
/// own rotation formula still supplies the angle, since a range stores none.
void ApplyReferenceGrainline(VLayoutPiece &placed, const OExportPieceGeometry &geometry,
                             const OSizerange *range, const Unit &unit, const VContainer *pattern)
{
    VPieceGrainline &grainline = placed.GetGrainline();

    if (IsGroupedMember(pattern, geometry.groupId))
    {
        // A grouped member carries no grainline of its own: the group hosts one for all
        // of them, exactly as Details and Graded do it.
        grainline.SetEnabled(false);
        return;
    }

    // Nothing is drawn for this piece — it carries no grainline, or the one it carries
    // was already refused when the piece was built. A range stores neither an angle nor
    // an arrow type, so it cannot put one back: leaving it alone is the whole answer.
    if (!grainline.IsEnabled() || !grainline.IsVisible())
    {
        return;
    }

    const auto *pieces = pattern != nullptr ? pattern->DataPieces() : nullptr;
    if (pieces == nullptr)
    {
        return;
    }

    const auto piece = pieces->constFind(geometry.id);
    if (piece == pieces->cend())
    {
        return;
    }

    const VGrainlineData &geom = piece.value().GetGrainlineGeometry();

    // The line GetItem built ignores the range, but its start point is the Details
    // placement — the fallback for a grainline that was never placed, so it does not land
    // on the document origin.
    const QPointF fallback = grainline.GetMainLine().p1();

    const OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, geometry.id, false);
    const OGraded::GrainlinePlacement placement =
        OGraded::ResolveGrainline(geom, ov, false, pattern, unit, fallback);
    const qreal rotation = OGraded::GrainlineRotation(geom.GetRotation(), false, pattern);

    // The piece hands its grainline out, and the range only ever moves the origin and the
    // length: the angle stays the piece's own formula, which this same call evaluated.
    const VPieceGrainline moved(placement.pos, placement.lengthPx, qDegreesToRadians(rotation),
                                geom.GetArrowType());
    grainline.SetMainLine(moved.GetMainLine());
}

//---------------------------------------------------------------------------------------------------------------------
/// The host of one group and the group's shared grainline, on the reference layer.
///
/// A group has no item of its own in this scene: the nesting places its members, and the
/// grainline is the group's, not theirs. The host is the invisible carrier Graded also
/// builds — its origin is the top-left corner of the placed members, which is the frame
/// the group's own grainline is stored in.
void BuildGroupGrainline(VMainGraphicsScene *scene, quint32 groupId,
                         const QList<QGraphicsItem *> &members, const OSizerange *range, const Unit &unit,
                         const VContainer *pattern)
{
    const auto *groups = pattern != nullptr ? pattern->DataPieceGroups() : nullptr;
    if (members.isEmpty() || groups == nullptr)
    {
        return;
    }

    const auto group = groups->constFind(groupId);
    if (group == groups->cend() || group.value().pieceIds.size() < OGraded::kMinGroupMembers)
    {
        return;
    }

    QRectF box;
    for (QGraphicsItem *member : members)
    {
        box = box.united(member->sceneBoundingRect());
    }
    if (box.isEmpty())
    {
        return;
    }

    auto *host = new QGraphicsRectItem(0.0, 0.0, box.width(), box.height());
    host->setPos(box.topLeft());
    host->setPen(Qt::NoPen);
    host->setBrush(Qt::NoBrush);
    host->setFlag(QGraphicsItem::ItemIsSelectable, false);
    host->setFlag(QGraphicsItem::ItemIsMovable, false);
    host->setAcceptedMouseButtons(Qt::NoButton);
    host->setZValue(1.0);
    scene->addItem(host);

    const VGrainlineData &geom = group.value().m_glGrainline;
    if (!geom.IsEnabled() || !geom.IsVisible())
    {
        return;
    }

    const QPointF center(box.width() / 2.0, box.height() / 2.0);
    const OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, groupId, true);
    const OGraded::GrainlinePlacement placement = OGraded::ResolveGrainline(geom, ov, true, pattern, unit, center);
    const qreal rotation = OGraded::GrainlineRotation(geom.GetRotation(), true, pattern);

    OGraded::MakeGrainlineItem(host, geom, placement, rotation);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OExportCanvas::Build(VMainGraphicsScene *scene,
                          const QVector<OExportSizeLayer> &layers,
                          const OExportNestingResult &result,
                          const OSizerange *range,
                          const VContainer *pattern)
{
    if (scene == nullptr || !result.ok)
    {
        return;
    }

    scene->clear();

    const Unit *patternUnit = pattern != nullptr ? pattern->GetPatternUnit() : nullptr;
    const Unit unit = patternUnit != nullptr ? *patternUnit : Unit::Mm;

    // Members of each group, on the reference layer only: the group's grainline is drawn
    // once, above all of them, once the loop has placed them.
    QHash<quint32, QList<QGraphicsItem *>> groupMembers;

    // The bin: fixed width, height given by the nesting.
    auto *bin = new QGraphicsRectItem(0, 0, result.binWidthPx, result.usedHeightPx);
    bin->setPen(QPen(Qt::black, 1));
    bin->setBrush(QBrush(Qt::white));
    bin->setZValue(-1);
    scene->addItem(bin);

    for (int i = 0; i < result.placements.size() && i < result.keys.size(); ++i)
    {
        const OExportPlacement &placement = result.placements.at(i);
        const QString &key = result.keys.at(i);

        for (int l = 0; l < layers.size(); ++l)
        {
            const OExportSizeLayer &layer = layers.at(l);
            // The layers come in the order of the range's sizes, smallest first: the
            // first one is the reference, the only one allowed to carry per-piece decor.
            const bool isReference = (l == 0);

            for (const OExportPieceGeometry &geometry : layer.pieces)
            {
                if (geometry.key != key)
                {
                    continue;
                }

                // The engine places a footprint by the min corner of its unrotated polygon,
                // so the piece has to be brought back to that origin before being moved —
                // exactly what the nested SVG writes as translate(x, y) rotate(r)
                // translate(-ox, -oy). The piece's alignment shift belongs to that origin,
                // not after the rotation: it is part of where the piece sits in the
                // document, and the engine measured the footprint with it already applied.
                QTransform placementMatrix;
                placementMatrix.translate(placement.x, placement.y);
                placementMatrix.rotate(placement.rotation);
                placementMatrix.translate(-placement.originX + geometry.alignDelta.x(),
                                          -placement.originY + geometry.alignDelta.y());

                // The placement is baked into the piece's own matrix rather than into a
                // transform on the item: an item transform would drag the bounding rect,
                // the selection and the rubber band along with it.
                VLayoutPiece placed = geometry.piece;
                placed.SetMatrix(placementMatrix);

                // The decor is set on the piece before GetItem runs, because GetItem is
                // what builds it — and it is set on the reference layer only.
                if (isReference)
                {
                    ApplyReferenceGrainline(placed, geometry, range, unit, pattern);
                }

                QGraphicsItem *item = placed.GetItem(false, true, false);
                if (item == nullptr)
                {
                    continue;
                }

                if (!isReference)
                {
                    HidePerSizeDecor(item);
                }
                else if (geometry.groupId != 0)
                {
                    groupMembers[geometry.groupId].append(item);
                }

                scene->addItem(item);
            }
        }
    }

    for (auto it = groupMembers.cbegin(); it != groupMembers.cend(); ++it)
    {
        BuildGroupGrainline(scene, it.key(), it.value(), range, unit, pattern);
    }
}
