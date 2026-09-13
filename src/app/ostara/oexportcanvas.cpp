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

#include "core/ogratedsizestyles.h"
#include "mainwindow_graded_helpers.h"
#include "ogradedgeometry.h"
#include "ogradedsizetrace.h"
#include "oexportgeometry.h"
#include "oexportnesting.h"
#include "widgets/InfoBlock/OInfoBlockResolver.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../vlayout/vgraphicsfillitem.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/ipieceinfo.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/oinfoblockcontent.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/ocartouchepainter.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vpiecegrainline.h"

#include <QBrush>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QHash>
#include <QPen>
#include <QScopedPointer>
#include <QTransform>
#include <QtMath>
#include <QtNumeric>

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
/// The range's override for one information block of one owner, or an invalid one. Same
/// rule as the grainline: an override exists only for a block the user actually moved in
/// Graded, and this mode reads it without ever writing one.
auto BlockOverrideOf(const OSizerange *range, quint32 ownerId, bool isGroup, int block)
    -> OSizerange::BlockOverride
{
    if (range == nullptr)
    {
        return {};
    }

    if (isGroup)
    {
        const auto it = range->groupContent.constFind(ownerId);
        if (it == range->groupContent.cend())
        {
            return {};
        }
        const auto ov = it.value().blocks.constFind(block);
        return ov == it.value().blocks.cend() ? OSizerange::BlockOverride{} : ov.value();
    }

    const auto it = range->pieceContent.constFind(ownerId);
    if (it == range->pieceContent.cend())
    {
        return {};
    }
    const auto ov = it.value().blocks.constFind(block);
    return ov == it.value().blocks.cend() ? OSizerange::BlockOverride{} : ov.value();
}

//---------------------------------------------------------------------------------------------------------------------
/// The range's override for one annotation of one owner, or an invalid one.
auto AnnotationOverrideOf(const OSizerange *range, quint32 ownerId, bool isGroup, int index)
    -> OSizerange::AnnotationOverride
{
    if (range == nullptr)
    {
        return {};
    }

    if (isGroup)
    {
        const auto it = range->groupContent.constFind(ownerId);
        if (it == range->groupContent.cend())
        {
            return {};
        }
        const auto ov = it.value().annotations.constFind(index);
        return ov == it.value().annotations.cend() ? OSizerange::AnnotationOverride{} : ov.value();
    }

    const auto it = range->pieceContent.constFind(ownerId);
    if (it == range->pieceContent.cend())
    {
        return {};
    }
    const auto ov = it.value().annotations.constFind(index);
    return ov == it.value().annotations.cend() ? OSizerange::AnnotationOverride{} : ov.value();
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
/// The extent a piece is drawn with, measured before the placement is applied.
///
/// Read from an item built exactly like the one the canvas draws — same pen, same
/// notches — because that is the rectangle these frames have always been measured on.
/// Taking the contour points instead would drop the pen, and move every child of a group
/// by half its width.
auto UnplacedBounds(const VLayoutPiece &piece) -> QRectF
{
    QScopedPointer<QGraphicsItem> probe(piece.GetItem(false, true, false));
    return probe.isNull() ? QRectF() : probe->boundingRect();
}

/// @brief The frame an owner's children are stored in, and the placement that brings it to
///        the scene.
struct DecorFrame
{
    QTransform placement; ///< the owner's frame -> the scene
    qreal rotation{0.0};  ///< the angle inside @c placement, in degrees
    QPointF origin{};     ///< the point of the owner's frame that is its local (0, 0)
    QRectF bounds{};      ///< the owner's extent, in its own frame
};

//---------------------------------------------------------------------------------------------------------------------
/// The invisible carrier an owner's children hang under.
///
/// It carries the placement the owner itself received, so a child hung at the coordinates
/// the owner stores lands where the owner is: @c host.map(p) equals
/// @c frame.placement.map(frame.origin + p). Without it the carrier would stay
/// axis-aligned on the bounding box of the placed members, which stops being the owner's
/// frame the moment the nesting turns it — and a group is a footprint like any other, so
/// the engine may turn it by any multiple of two degrees.
auto MakeDecorHost(VMainGraphicsScene *scene, const DecorFrame &frame) -> QGraphicsItem *
{
    auto *host = new QGraphicsRectItem(0.0, 0.0, frame.bounds.width(), frame.bounds.height());
    host->setPos(frame.placement.map(frame.origin));
    host->setRotation(frame.rotation);
    host->setPen(Qt::NoPen);
    host->setBrush(Qt::NoBrush);
    host->setFlag(QGraphicsItem::ItemIsSelectable, false);
    host->setFlag(QGraphicsItem::ItemIsMovable, false);
    host->setAcceptedMouseButtons(Qt::NoButton);
    host->setZValue(1.0);
    scene->addItem(host);
    return host;
}

//---------------------------------------------------------------------------------------------------------------------
/// The mode shows the decor, it never edits it. Both item classes draw their eight resize
/// handles under `m_active` alone — SetReadOnly only drops the movable flag — so an item
/// left active would still paint them and answer the mouse. Manual correction is a subject
/// of its own, with its own spec.
template <typename DecorItem> void MakeDecorReadOnly(DecorItem *item)
{
    item->SetActive(false);
    item->SetReadOnly(true);
    item->setAcceptedMouseButtons(Qt::NoButton);
    item->show();
}

//---------------------------------------------------------------------------------------------------------------------
/// Add an owner's information blocks and annotations under @p host, in @p host's frame.
///
/// The owner is the group when the piece belongs to one — a grouped member carries none of
/// these, exactly as it carries no grainline of its own. Details, the Graded canvas and its
/// SVG export all draw that same line.
///
/// @param centerLocal the point a child that was never placed falls back on, in that frame
auto BuildOwnerDecor(QGraphicsItem *host, const IPieceInfo &owner, bool isGroup, quint32 ownerId,
                     const QPointF &centerLocal, const OSizerange *range, const VContainer *pattern,
                     VAbstractPattern *doc)
{
    if (pattern == nullptr)
    {
        return;
    }

    // A block exists when the template dispatches a section to it and that section has
    // content: the resolver leaves nothing else in blockContent. Without a document there
    // is no template to resolve, so the blocks are skipped and the annotations are not.
    if (doc != nullptr)
    {
        const OInfoBlockEffectiveState state = OInfoBlockResolver::ResolveEffectiveState(owner, doc, pattern);
        const OInfoBlockData &info = owner.GetInfoBlockData();

        for (int block : state.blockContent.keys())
        {
            const QVector<OCartoucheRenderLine> lines = state.blockContent.value(block).toVector();
            if (lines.isEmpty())
            {
                continue;
            }

            const OSizerange::BlockOverride ov = BlockOverrideOf(range, ownerId, isGroup, block);
            const OGraded::InfoBlockPlacement placement =
                OGraded::ResolveInfoBlock(info, ov, lines, block, centerLocal);

            OInfoBlockGeom geom;
            geom.pos = placement.topLeft;
            geom.width = placement.size.width();
            geom.height = placement.size.height();
            geom.rotation = placement.rotation;
            geom.fontScale = placement.fontScale;

            auto *item = new OInfoBlockItem(block, host);
            item->SetBlockGeom(geom);
            item->SetContent(lines, placement.fontScale);
            MakeDecorReadOnly(item);
        }
    }

    const QList<OAnnotation> &annotations = owner.GetAnnotationData().Annotations();
    const OAnnotationStyleResolver &styleOf = VToolSeamAllowance::AnnotationStyleResolver();

    for (int i = 0; i < annotations.size(); ++i)
    {
        const OAnnotation &annotation = annotations.at(i);
        if (annotation.text.trimmed().isEmpty())
        {
            continue;
        }

        OAnnotationGeom geom = annotation.geom;
        const OSizerange::AnnotationOverride ov = AnnotationOverrideOf(range, ownerId, isGroup, i);
        if (ov.valid)
        {
            geom = ov.geom;
        }
        // (0, 0) is the sentinel for an annotation that was never moved: it sits on its
        // owner. The rotation never comes from the range — a range does not store one.
        if (qFuzzyIsNull(geom.pos.x()) && qFuzzyIsNull(geom.pos.y()))
        {
            geom.pos = centerLocal;
        }

        const OAnnotationRenderStyle style = styleOf ? styleOf(annotation.templateName)
                                                     : OAnnotationRenderStyle();
        const qreal rotation = OGraded::AnnotationRotation(annotation.rotationFormula, isGroup, pattern);

        auto *item = new OAnnotationItem(i, host);
        item->SetIndex(i);
        // SetContent before SetAnnotationGeom: an auto-sized box takes its size from the
        // natural size SetContent measured. Details and Graded both call them in this order.
        item->SetContent(annotation.text, style);
        item->SetAnnotationGeom(geom, rotation);
        MakeDecorReadOnly(item);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// The decor of one isolated piece, on the reference layer.
///
/// A piece's children are stored in the document's own coordinates, so its carrier takes
/// the placement as-is and its local origin is the document origin. A piece of a group of
/// one keeps its own decor: below kMinGroupMembers a group is not a block.
void BuildPieceDecor(VMainGraphicsScene *scene, const OExportPieceGeometry &geometry,
                     const QTransform &placement, qreal rotation, const OSizerange *range,
                     const VContainer *pattern, VAbstractPattern *doc)
{
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

    DecorFrame frame;
    frame.placement = placement;
    frame.rotation = rotation;
    frame.origin = QPointF(0.0, 0.0);
    frame.bounds = UnplacedBounds(geometry.piece);

    auto *host = MakeDecorHost(scene, frame);
    BuildOwnerDecor(host, piece.value(), false, geometry.id, frame.bounds.center() - frame.origin, range,
                    pattern, doc);

    if (host->childItems().isEmpty())
    {
        delete host;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// The decor of one group, on the reference layer: its grainline, its cartouche and its
/// annotations.
///
/// A group has no item of its own in this scene: the nesting places its members. Graded
/// builds the same invisible carrier — its origin is the top-left corner of the members,
/// which is the frame the group stores all of its children in.
void BuildGroupDecor(VMainGraphicsScene *scene, quint32 groupId, const DecorFrame &frame,
                     const OSizerange *range, const Unit &unit, const VContainer *pattern,
                     VAbstractPattern *doc)
{
    const auto *groups = pattern != nullptr ? pattern->DataPieceGroups() : nullptr;
    if (groups == nullptr || frame.bounds.isNull())
    {
        return;
    }

    const auto group = groups->constFind(groupId);
    if (group == groups->cend() || group.value().pieceIds.size() < OGraded::kMinGroupMembers)
    {
        return;
    }

    auto *host = MakeDecorHost(scene, frame);
    const QPointF centerLocal = frame.bounds.center() - frame.origin;

    const VGrainlineData &geom = group.value().m_glGrainline;
    if (geom.IsEnabled() && geom.IsVisible())
    {
        const OSizerange::GrainlineOverride ov = GrainlineOverrideOf(range, groupId, true);
        const OGraded::GrainlinePlacement placement =
            OGraded::ResolveGrainline(geom, ov, true, pattern, unit, centerLocal);
        const qreal rotation = OGraded::GrainlineRotation(geom.GetRotation(), true, pattern);

        OGraded::MakeGrainlineItem(host, geom, placement, rotation);
    }

    BuildOwnerDecor(host, group.value(), true, groupId, centerLocal, range, pattern, doc);

    if (host->childItems().isEmpty())
    {
        delete host;
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OExportCanvas::Build(VMainGraphicsScene *scene,
                          const QVector<OExportSizeLayer> &layers,
                          const OExportNestingResult &result,
                          const OSizerange *range,
                          const VContainer *pattern,
                          VAbstractPattern *doc)
{
    if (scene == nullptr || !result.ok)
    {
        return;
    }

    scene->clear();

    const Unit *patternUnit = pattern != nullptr ? pattern->GetPatternUnit() : nullptr;
    const Unit unit = patternUnit != nullptr ? *patternUnit : Unit::Mm;

    // The frame of each group, on the reference layer only: the group's decor is drawn
    // once, above all of its members, once the loop has placed them.
    QHash<quint32, DecorFrame> groupFrames;

    // The per-size trace styles, read on the very key the Graded canvas reads them on.
    // They are authored against a measurements table, and a table outside the library has
    // no key to read: the canvas styles nothing there, and an export that disagreed with
    // the screen would be a trap — the mode delivers a document built from what is shown.
    // The "Show in canvas" toggle is deliberately not read here: it is a canvas
    // preference, and the Graded SVG export ignores it for the same reason.
    QHash<QString, OGradedSizeStyle> styles;
    if (auto *settings = VAbstractApplication::VApp()->Settings(); settings != nullptr)
    {
        const OGradedSizeStyles::StyleAxis axis = OGradedSizeStyles::ResolveStyleAxis(
            doc, pattern, range != nullptr ? range->name : QString());
        if (axis.ready && axis.inLibrary)
        {
            styles = OGradedSizeStyles::StylesForAxis(settings, axis.canonical, axis.axisName);
        }
    }

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

                // Each layer carries the label of the size it is, and takes that size's
                // style — the same pass the Graded canvas runs, so a size reads the same
                // way on screen and in the document. The items are fresh out of GetItem
                // and thrown away with the scene, so the pass owns nothing to restore.
                item->setData(kGradedSizeRole, layer.label);
                OGradedSizeTrace::ApplyToSubtree(item, styles, OGradedSizeTrace::Scope::FreshItems);

                if (!isReference)
                {
                    HidePerSizeDecor(item);
                }
                else if (IsGroupedMember(pattern, geometry.groupId))
                {
                    // A grouped member carries nothing of its own: the group hosts the
                    // single set, grainline included. Its frame is measured here, at the
                    // one moment its unplaced extent is still at hand.
                    DecorFrame &frame = groupFrames[geometry.groupId];
                    frame.placement = placementMatrix;
                    frame.rotation = placement.rotation;
                    frame.bounds = frame.bounds.united(UnplacedBounds(geometry.piece));
                }

                scene->addItem(item);

                if (isReference && !IsGroupedMember(pattern, geometry.groupId))
                {
                    BuildPieceDecor(scene, geometry, placementMatrix, placement.rotation, range, pattern, doc);
                }
            }
        }
    }

    for (auto it = groupFrames.begin(); it != groupFrames.end(); ++it)
    {
        // The children of a group are stored relative to the top-left corner of its
        // members, and that corner is only known once every member has contributed its
        // extent — hence the second pass rather than a frame built on the fly.
        DecorFrame &frame = it.value();
        frame.origin = frame.bounds.topLeft();
        BuildGroupDecor(scene, it.key(), frame, range, unit, pattern, doc);
    }
}
