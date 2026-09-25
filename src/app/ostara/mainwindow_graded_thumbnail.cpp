/************************************************************************
 **
 **  @file   mainwindow_graded_thumbnail.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26 September, 2026
 **
 **  @brief  Graded mode: the miniatures of the reference layer.
 **
 **          A size range never carries another size's drawing. A miniature always shows the
 **          piece at the reference size of the pattern — the very drawing Details shows, taken
 **          from the same cache — so moving or resizing a block in Graded records the frame of
 **          that range and of no other.
 **
 **          Where a miniature lives follows the mode's own layering: a lone piece carries its own
 **          on its reference item, and a **member of a group** has its miniature carried by the
 **          group — the group stores one per member, and the group's host draws them. Those are the
 **          group's data, not the members': linking a piece never turns its own miniature on, and
 **          unlinking finds that one exactly as it was.
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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "core/savegradedrangecontent.h"
#include "mainwindow_graded_helpers.h"
#include "xml/vpattern.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/othumbnaildata.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/tools/othumbnailitem.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/vmaingraphicsscene.h"

#include <QGraphicsItem>
#include <QHash>
#include <QPainterPath>
#include <QtGlobal>
#include <QUndoStack>

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// Marker of an interactive miniature item (carries the owner piece id).
constexpr int kGradedThumbnailRole = Qt::UserRole + 910;

} // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreGradedThumbnail(quint32 groupId, quint32 pieceId, const QPointF &pos, qreal width,
                                      qreal height)
{
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange oldRange = ranges->value(m_activeGradedRangeName);
    OSizerange newRange = oldRange;
    OSizerange::ThumbnailOverride override;
    override.valid = true;
    override.pos = pos;
    override.width = width;
    override.height = height;
    if (groupId == NULL_ID)
    {
        newRange.pieceContent[pieceId].thumbnail = override; // a lone piece's miniature
    }
    else
    {
        // A member's miniature is the group's, per member — never the piece's own.
        newRange.groupContent[groupId].memberThumbnails.insert(pieceId, override);
    }
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SaveGradedRangeContent(doc, pattern, oldRange, newRange));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGradedReferenceThumbnails(quint32 onlyPieceId, quint32 onlyGroupId)
{
    if (m_sceneGraded == nullptr || pattern == nullptr || m_activeGradedRangeName.isEmpty())
    {
        return;
    }
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const auto *pieces = pattern->DataPieces();
    const auto *groups = pattern->DataPieceGroups();
    if (pieces == nullptr || groups == nullptr)
    {
        return;
    }
    const OSizerange &range = ranges->value(m_activeGradedRangeName);

    // The reference item of every piece, by id: a member's miniature is drawn on its group's host,
    // and the box it is measured against when nobody chose one is the member's own.
    QHash<quint32, QGraphicsItem *> referenceItems;
    const auto sceneItems = m_sceneGraded->items();
    for (auto *item : sceneItems)
    {
        if (GradedIsReference(item))
        {
            const quint32 pieceId = GradedPieceId(item);
            if (pieceId != 0)
            {
                referenceItems.insert(pieceId, item);
            }
        }
    }

    // Drop previous interactive miniature items, then rebuild (the old list must not be reused).
    for (auto *owner : sceneItems)
    {
        const quint32 ownerPiece = GradedPieceId(owner);
        const quint32 ownerGroup = ownerPiece == 0 ? owner->data(kGradedHostGroupIdRole).toUInt() : 0;
        if (ownerPiece == 0 && ownerGroup == 0)
        {
            continue;
        }
        if (onlyPieceId != 0 && ownerPiece != onlyPieceId)
        {
            continue;
        }
        if (onlyGroupId != 0 && ownerGroup != onlyGroupId)
        {
            continue;
        }
        const auto children = owner->childItems();
        for (auto *child : children)
        {
            if (!child->data(kGradedThumbnailRole).isNull())
            {
                delete child;
            }
        }
    }

    // One miniature on an owner: its drawing is the member's own (they can never disagree on what a
    // miniature shows), its frame comes from the range, and the box it was measured against is the
    // piece's — never the group's union.
    auto BuildThumbnail = [this](QGraphicsItem *owner, quint32 pieceId, quint32 groupId,
                                         const OThumbnailData &stored,
                                         const OSizerange::ThumbnailOverride &override, const QRectF &pieceBox)
    {
        if (!stored.IsActive())
        {
            return; // nothing asked for, in Details or in the group
        }
        // **`getTool()` is not a guard: it throws when the tool is gone** (VExceptionBadId — see the
        // cmd+W crash family in Bugs.md). A stale piece id must draw nothing, not abort the app.
        VToolSeamAllowance *tool = nullptr;
        try
        {
            tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
        }
        catch (const VExceptionBadId &)
        {
            return;
        }
        if (tool == nullptr)
        {
            return;
        }
        // The reference drawing, not the one of the layer under it: the miniature names the seam
        // allowances of the piece, and those are shown at one size only — the pattern's reference.
        const QPainterPath content = tool->ThumbnailReferenceContent();
        const QRectF contentBounds = content.boundingRect();
        if (content.isEmpty() || contentBounds.isEmpty())
        {
            return; // not captured yet (a capture is on its way) or the piece has no outline
        }

        OThumbnailGeom geom;
        if (override.valid)
        {
            geom.pos = override.pos;
            geom.width = override.width;
            geom.height = override.height;
        }
        else
        {
            geom = stored.Geom();
        }
        geom = OThumbnailItem::ResolveBlockGeom(geom, pieceBox, override.valid || stored.HasGeometry());

        // The wording follows @p stored — the piece's own miniature for a lone piece, the group's
        // miniature of that member for a member: what is drawn is what is printed.
        const VToolSeamAllowance::ThumbnailWording wording = tool->ResolveThumbnailWording(
            pattern->DataPieces()->value(pieceId), stored);

        auto *thumbnail = new OThumbnailItem(owner);
        thumbnail->setData(kGradedThumbnailRole, pieceId);
        // The values and the legend have no placement of their own inside a size range, so the
        // sub-mode stays closed — and the double-click stays the view's zoom, as for every other child.
        thumbnail->SetSubModeEnabled(false);
        thumbnail->SetLabels(wording.labels, wording.legend, stored.LegendOffset(), stored.HasLegendOffset());
        thumbnail->SetContent(content, contentBounds);
        thumbnail->SetBlockGeom(geom);
        thumbnail->show();

        connect(thumbnail, &OThumbnailItem::GeometryChanged, this,
                [this, groupId, pieceId](const QPointF &pos, qreal width, qreal height) {
                    StoreGradedThumbnail(groupId, pieceId, pos, width, height);
                });
    };

    // Lone pieces: their miniature sits on their own reference item, in the piece's own frame — the
    // very frame Details stores it in, so a range needs no conversion to apply it.
    for (auto it = referenceItems.cbegin(); it != referenceItems.cend(); ++it)
    {
        const quint32 pieceId = it.key();
        if (onlyPieceId != 0 && pieceId != onlyPieceId)
        {
            continue;
        }
        if (pattern->GetPieceGroupId(pieceId) != NULL_ID)
        {
            continue; // a member's miniature is carried by its group, on the group's host
        }
        const auto pieceIt = pieces->constFind(pieceId);
        if (pieceIt == pieces->constEnd())
        {
            continue;
        }
        const OThumbnailData stored = pieceIt.value().GetThumbnailData();
        OSizerange::ThumbnailOverride override;
        const auto contentIt = range.pieceContent.constFind(pieceId);
        if (contentIt != range.pieceContent.constEnd())
        {
            override = contentIt->thumbnail;
        }
        BuildThumbnail(it.value(), pieceId, NULL_ID, stored, override, it.value()->boundingRect());
    }

    // Groups: one miniature per member, on the group's host — the group carries them, the members'
    // own stay masked, and each frame is stored per range like the group's other children.
    if (onlyPieceId != 0)
    {
        return; // a piece-targeted refresh never concerns a group host
    }
    for (auto *host : sceneItems)
    {
        const quint32 groupId = host->data(kGradedHostGroupIdRole).toUInt();
        if (groupId == 0 || (onlyGroupId != 0 && groupId != onlyGroupId))
        {
            continue;
        }
        const auto groupIt = groups->constFind(groupId);
        if (groupIt == groups->constEnd())
        {
            continue;
        }
        const OPieceGroup &group = groupIt.value();
        const auto contentIt = range.groupContent.constFind(groupId);
        const OSizerange::GroupContent *content =
            contentIt == range.groupContent.constEnd() ? nullptr : &contentIt.value();
        for (quint32 pieceId : group.pieceIds)
        {
            QGraphicsItem *memberItem = referenceItems.value(pieceId, nullptr);
            if (memberItem == nullptr)
            {
                continue; // a member with no reference layer (not in layout): nothing to measure on
            }
            OSizerange::ThumbnailOverride override;
            if (content != nullptr)
            {
                override = content->memberThumbnails.value(pieceId);
            }
            BuildThumbnail(host, pieceId, groupId, group.MemberThumbnail(pieceId), override,
                           host->mapRectFromScene(memberItem->sceneBoundingRect()));
        }
    }
}
