/************************************************************************
 **
 **  @file   ochildanchor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Re-derives the position of a piece's or a group's children
 **          (grainline, information blocks, annotations) against a
 **          reference rect, so that they follow a change of active size.
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

#include "ochildanchor.h"

#include "../ifc/exception/vexceptionbadid.h"
#include "floatItemData/vgrainlinedata.h"
#include "ipieceinfo.h"
#include "oannotationdata.h"
#include "oinfoblockdata.h"
#include "opiecegroup.h"
#include "othumbnaildata.h"
#include "vcontainer.h"
#include "vpiece.h"

#include <QtGlobal>

#include <algorithm>
#include <utility>

namespace
{
/**
 * @brief Bounding box of a set of pattern points.
 * @return An empty rect when there is no point.
 */
auto PointsBox(const QVector<VPointF> &points) -> QRectF
{
    if (points.isEmpty())
    {
        return {};
    }

    qreal left = 0.0;
    qreal top = 0.0;
    qreal right = 0.0;
    qreal bottom = 0.0;

    bool first = true;
    for (const VPointF &point : points)
    {
        const QPointF pt = point.toQPointF();
        if (first)
        {
            left = right = pt.x();
            top = bottom = pt.y();
            first = false;
            continue;
        }
        left = (std::min)(left, pt.x());
        right = (std::max)(right, pt.x());
        top = (std::min)(top, pt.y());
        bottom = (std::max)(bottom, pt.y());
    }

    return {QPointF(left, top), QPointF(right, bottom)};
}

/// A rect can carry a ratio only if it has a surface: a piece reduced to a
/// vertical or horizontal line has no meaningful width or height to divide by.
auto IsUsable(const QRectF &ref) -> bool
{
    return !ref.isNull() && ref.width() > 0.0 && ref.height() > 0.0;
}

/**
 * @brief Follow one child: resolve its anchor, or adopt one.
 *
 * @param adopt The child carries a deliberate placement that predates the anchor
 *              (a moved block, a placed annotation or grainline) — adopt it. When
 *              false the child was never placed and keeps its historical
 *              auto-centring, untouched.
 * @return true when something changed.
 */
auto Follow(ONormalizedAnchor &anchor, QPointF &pos, const QRectF &ref, bool adopt) -> bool
{
    if (anchor.valid)
    {
        const QPointF resolved = OChildAnchor::Resolve(anchor, ref, pos);
        if (resolved == pos)
        {
            return false;
        }
        pos = resolved;
        return true;
    }

    if (!adopt)
    {
        return false;
    }

    const ONormalizedAnchor adopted = OChildAnchor::FromAbsolute(pos, ref);
    if (!adopted.valid)
    {
        return false;
    }

    // Adopting must not move the child: the position stays exactly as it was, and
    // only the anchor is recorded. Resolving it now would be a needless detour
    // through two divisions.
    anchor = adopted;
    return true;
}

/**
 * @brief Record an anchor for every child the user moved between two states.
 *
 * A child that moved always has its anchor overwritten, even when the reference
 * rect cannot carry a ratio (the anchor is then invalid): left stale, an old
 * anchor would pull the child back at the next refresh and undo the gesture.
 */
auto SyncChildren(const VGrainlineData &grainlineBefore, VGrainlineData &grainlineAfter,
                  const IPieceInfo &before, IPieceInfo &after, const QRectF &ref) -> bool
{
    bool changed = false;

    if (grainlineBefore.GetPos() != grainlineAfter.GetPos())
    {
        grainlineAfter.SetAnchor(OChildAnchor::FromAbsolute(grainlineAfter.GetPos(), ref));
        changed = true;
    }

    const OInfoBlockData &blocksBefore = before.GetInfoBlockData();
    OInfoBlockData &blocksAfter = after.GetInfoBlockData();
    for (int index : blocksAfter.StoredBlockIndexes())
    {
        const OInfoBlockGeom geom = blocksAfter.BlockGeom(index);
        if (blocksBefore.BlockGeom(index).pos == geom.pos)
        {
            continue;
        }
        OInfoBlockGeom moved = geom;
        moved.anchor = OChildAnchor::FromAbsolute(geom.pos, ref);
        blocksAfter.SetBlockGeom(index, moved);
        changed = true;
    }

    const QList<OAnnotation> annotationsBefore = before.GetAnnotationData().Annotations();
    QList<OAnnotation> annotationsAfter = after.GetAnnotationData().Annotations();
    bool annotationsMoved = false;
    for (int i = 0; i < annotationsAfter.size(); ++i)
    {
        const QPointF pos = annotationsAfter.at(i).geom.pos;
        if (i < annotationsBefore.size() && annotationsBefore.at(i).geom.pos == pos)
        {
            continue;
        }
        annotationsAfter[i].geom.anchor = OChildAnchor::FromAbsolute(pos, ref);
        annotationsMoved = true;
    }
    if (annotationsMoved)
    {
        OAnnotationData data = after.GetAnnotationData();
        data.SetAnnotations(annotationsAfter);
        after.SetAnnotationData(data);
        changed = true;
    }

    // The miniature records its anchor the same way: a block the gesture moved must carry the
    // anchor of where it landed, or the next refresh would pull it back and undo the drag.
    const OThumbnailData &thumbnailBefore = before.GetThumbnailData();
    OThumbnailData &thumbnailAfter = after.GetThumbnailData();
    if (thumbnailAfter.HasGeometry() && thumbnailBefore.Geom().pos != thumbnailAfter.Geom().pos)
    {
        OThumbnailGeom geom = thumbnailAfter.Geom();
        geom.anchor = OChildAnchor::FromAbsolute(geom.pos, ref);
        thumbnailAfter.SetGeom(geom);
        changed = true;
    }

    return changed;
}

/// Every child of an owner, resolved in one call.
auto MaterializeChildren(VGrainlineData &grainline, IPieceInfo &owner, const QRectF &ref) -> bool
{
    bool changed = OChildAnchor::Materialize(grainline, ref);
    changed = OChildAnchor::Materialize(owner.GetInfoBlockData(), ref) || changed;
    changed = OChildAnchor::Materialize(owner.GetAnnotationData(), ref) || changed;
    changed = OChildAnchor::Materialize(owner.GetThumbnailData(), ref) || changed;
    return changed;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::RefRect(const VPiece &piece, const VContainer *data) -> QRectF
{
    if (data == nullptr)
    {
        return {};
    }

    try
    {
        return PointsBox(piece.MainPathNodePoints(data, false));
    }
    catch (const VExceptionBadId &)
    {
        return {}; // a node left the container — nothing to anchor against
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::RefRect(const OPieceGroup &group, const VContainer *data, const QPointF &origin) -> QRectF
{
    if (data == nullptr)
    {
        return {};
    }

    QRectF box;
    for (quint32 pieceId : group.pieceIds)
    {
        try
        {
            const VPiece member = data->GetPiece(pieceId);
            // A piece's path is built from the pattern points, and an origin is what translates it
            // into place. For a group member that origin is its position in the **stack** — where
            // the group draws, prints and exports it — not where its tool stands on the canvas: the
            // group's children are anchored to the group, and a member's editing workshop is not
            // part of it. A group with no stored stack falls back to the piece's own position,
            // which is exactly what such a file says.
            const QPointF memberPos = group.MemberStackPosition(pieceId,
                                                                QPointF(member.GetMx(), member.GetMy()));
            const QRectF memberBox = PointsBox(member.MainPathNodePoints(data, false)).translated(memberPos);
            if (memberBox.isNull())
            {
                continue;
            }
            box = box.isNull() ? memberBox : box.united(memberBox);
        }
        catch (const VExceptionBadId &)
        {
            continue; // stale member id (hand-edited file) — skip it
        }
    }

    if (box.isNull())
    {
        return {};
    }

    // The group's children are stored in the holder's local frame, whose origin is
    // the top-left of the members' bounds.
    return box.translated(-origin);
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::FromAbsolute(const QPointF &absolute, const QRectF &ref) -> ONormalizedAnchor
{
    if (!IsUsable(ref))
    {
        return {};
    }

    ONormalizedAnchor anchor;
    anchor.valid = true;
    anchor.uv = QPointF((absolute.x() - ref.left()) / ref.width(), (absolute.y() - ref.top()) / ref.height());
    return anchor;
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::Resolve(const ONormalizedAnchor &anchor, const QRectF &ref, const QPointF &fallback) -> QPointF
{
    if (!anchor.valid || !IsUsable(ref))
    {
        return fallback;
    }

    return {ref.left() + anchor.uv.x() * ref.width(), ref.top() + anchor.uv.y() * ref.height()};
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::Materialize(VGrainlineData &grainline, const QRectF &ref) -> bool
{
    // A disabled or hidden grainline draws nothing: there is no placement to keep.
    // Re-enabling one goes through the dock preset, which invalidates the anchor.
    if (!grainline.IsEnabled() || !grainline.IsVisible())
    {
        return false;
    }

    ONormalizedAnchor anchor = grainline.GetAnchor();
    QPointF pos = grainline.GetPos();

    // (0, 0) is the reading the Graded layers already make: never placed.
    const bool adopt = !(qFuzzyIsNull(pos.x()) && qFuzzyIsNull(pos.y()));
    if (!Follow(anchor, pos, ref, adopt))
    {
        return false;
    }

    grainline.SetPos(pos);
    grainline.SetAnchor(anchor);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::Materialize(OInfoBlockData &blocks, const QRectF &ref) -> bool
{
    bool changed = false;

    // StoredBlockIndexes() hands out a copy, so writing back while iterating is safe.
    for (int index : blocks.StoredBlockIndexes())
    {
        OInfoBlockGeom geom = blocks.BlockGeom(index);
        ONormalizedAnchor anchor = geom.anchor;
        QPointF pos = geom.pos;

        // A stored block is a deliberate placement even at (0, 0) — HasStoredGeometry
        // is the marker, not the value.
        if (!Follow(anchor, pos, ref, true))
        {
            continue;
        }

        geom.anchor = anchor;
        geom.pos = pos;
        blocks.SetBlockGeom(index, geom);
        changed = true;
    }

    return changed;
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::Materialize(OAnnotationData &annotations, const QRectF &ref) -> bool
{
    QList<OAnnotation> list = annotations.Annotations();
    bool changed = false;

    for (OAnnotation &annotation : list)
    {
        if (annotation.text.isEmpty())
        {
            // Same existence test UpdateAnnotations makes: an empty text is no annotation.
            continue;
        }

        ONormalizedAnchor anchor = annotation.geom.anchor;
        QPointF pos = annotation.geom.pos;
        const bool adopt = !(qFuzzyIsNull(pos.x()) && qFuzzyIsNull(pos.y()));
        if (!Follow(anchor, pos, ref, adopt))
        {
            continue;
        }

        annotation.geom.anchor = anchor;
        annotation.geom.pos = pos;
        changed = true;
    }

    if (changed)
    {
        annotations.SetAnnotations(list);
    }

    return changed;
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::Materialize(OThumbnailData &thumbnail, const QRectF &ref) -> bool
{
    // A miniature that was never placed is auto-centred at every refresh: there is no deliberate
    // position to keep, and adopting the derived one would freeze the auto-placement on a spot the
    // user never chose. HasGeometry is the marker, exactly like HasStoredGeometry for a block.
    if (!thumbnail.HasGeometry())
    {
        return false;
    }

    OThumbnailGeom geom = thumbnail.Geom();
    ONormalizedAnchor anchor = geom.anchor;
    QPointF pos = geom.pos;

    // A stored placement is deliberate even at (0, 0).
    if (!Follow(anchor, pos, ref, true))
    {
        return false;
    }

    geom.anchor = anchor;
    geom.pos = pos;
    thumbnail.SetGeom(geom);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::Materialize(VPiece &piece, const QRectF &ref) -> bool
{
    return MaterializeChildren(piece.GetGrainlineGeometry(), piece, ref);
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::Materialize(OPieceGroup &group, const QRectF &ref) -> bool
{
    return MaterializeChildren(group.m_glGrainline, group, ref);
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::SyncFollowedChildren(const VPiece &before, VPiece &after, const QRectF &ref) -> bool
{
    return SyncChildren(std::as_const(before).GetGrainlineGeometry(), after.GetGrainlineGeometry(), before, after, ref);
}

//---------------------------------------------------------------------------------------------------------------------
auto OChildAnchor::SyncFollowedChildren(const OPieceGroup &before, OPieceGroup &after, const QRectF &ref) -> bool
{
    return SyncChildren(std::as_const(before).m_glGrainline, after.m_glGrainline, before, after, ref);
}
