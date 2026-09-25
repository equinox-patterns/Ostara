/************************************************************************
 **
 **  @file   mainwindow_graded_layout.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: apply alignment + per-range content offsets and end a reference drag.
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
#include "mainwindowsnogui.h"
#include "mainwindow_graded_helpers.h"
#include "ogradedgeometry.h"
#include "xml/vpattern.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"

#include <QGraphicsItem>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QtGlobal>
#include <QtMath>
#include <QUndoStack>
#include <QVector>

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// Classification used by the deferred apply. Independent of any undo command: the scope is
/// recomputed from the range-content diff, which stays reliable across push, redo and undo.
enum class GradedChangeKind
{
    None,
    PieceOffset,
    GroupOffset,
    PieceChild,
    GroupChild
};

/// Describes the single-owner difference between two size-range contents — the scope a deferred
/// graded apply must refresh. Anything not reducible to exactly one owner whose change is purely
/// an offset (dx/dy) or purely child overrides stays "ambiguous" → full refresh.
struct GradedContentDiff
{
    quint32 ownerId = 0; // 0 → no single usable owner
    bool isGroup = false;
    bool offsetOnly = false;
    bool ambiguous = true;
};

GradedContentDiff DiffGradedRangeContent(const OSizerange &applied, const OSizerange &current)
{
    GradedContentDiff result;
    if (applied.name != current.name)
    {
        return result;
    }

    quint32 pieceDiff = 0;
    bool pieceMulti = false;
    bool pieceMoved = false;
    bool pieceChild = false;
    const OSizerange::PieceContent emptyPiece;
    for (auto it = current.pieceContent.cbegin(); it != current.pieceContent.cend(); ++it)
    {
        const auto at = applied.pieceContent.constFind(it.key());
        const OSizerange::PieceContent &a = at == applied.pieceContent.cend() ? emptyPiece : at.value();
        if (a == it.value())
        {
            continue;
        }
        if (pieceDiff != 0)
        {
            pieceMulti = true;
        }
        pieceDiff = it.key();
        const bool moved =
            !qFuzzyCompare(a.dx, it.value().dx) || !qFuzzyCompare(a.dy, it.value().dy);
        const bool child =
            !(a.grainline == it.value().grainline && a.blocks == it.value().blocks
              && a.annotations == it.value().annotations && a.thumbnail == it.value().thumbnail
              && a.annotationsFp == it.value().annotationsFp);
        pieceMoved = pieceMoved || moved;
        pieceChild = pieceChild || child;
    }
    for (auto it = applied.pieceContent.cbegin(); it != applied.pieceContent.cend(); ++it)
    {
        if (!current.pieceContent.contains(it.key()))
        {
            pieceMulti = true;
            pieceDiff = pieceDiff != 0 ? pieceDiff : it.key();
        }
    }

    quint32 groupDiff = 0;
    bool groupMulti = false;
    bool groupMoved = false;
    bool groupChild = false;
    const OSizerange::GroupContent emptyGroup;
    for (auto it = current.groupContent.cbegin(); it != current.groupContent.cend(); ++it)
    {
        const auto at = applied.groupContent.constFind(it.key());
        const OSizerange::GroupContent &a = at == applied.groupContent.cend() ? emptyGroup : at.value();
        if (a == it.value())
        {
            continue;
        }
        if (groupDiff != 0)
        {
            groupMulti = true;
        }
        groupDiff = it.key();
        const bool moved =
            !qFuzzyCompare(a.dx, it.value().dx) || !qFuzzyCompare(a.dy, it.value().dy);
        const bool child =
            !(a.grainline == it.value().grainline && a.blocks == it.value().blocks
              && a.annotations == it.value().annotations
              && a.memberThumbnails == it.value().memberThumbnails
              && a.annotationsFp == it.value().annotationsFp);
        groupMoved = groupMoved || moved;
        groupChild = groupChild || child;
    }
    for (auto it = applied.groupContent.cbegin(); it != applied.groupContent.cend(); ++it)
    {
        if (!current.groupContent.contains(it.key()))
        {
            groupMulti = true;
            groupDiff = groupDiff != 0 ? groupDiff : it.key();
        }
    }

    if (pieceDiff != 0 && groupDiff == 0 && !pieceMulti && pieceMoved != pieceChild)
    {
        result.ownerId = pieceDiff;
        result.isGroup = false;
        result.offsetOnly = pieceMoved;
        result.ambiguous = false;
    }
    else if (groupDiff != 0 && pieceDiff == 0 && !groupMulti && groupMoved != groupChild)
    {
        result.ownerId = groupDiff;
        result.isGroup = true;
        result.offsetOnly = groupMoved;
        result.ambiguous = false;
    }
    return result;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ApplyGradedLayoutOffsets(bool forceFull)
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
    const OSizerange &range = ranges->value(m_activeGradedRangeName);

    // Reusable positions pass, shared with the headless SVG export
    // (OGraded::ApplyOffsets): idempotent reset to the frozen base, per-piece
    // alignment, rigid-group alignment, then the per-range content offsets.
    const QList<QGraphicsItem *> allItems = m_sceneGraded->items();
    OGraded::ApplyOffsets(allItems, range, pattern);

    // Refresh the reference drag baselines.
    m_gradedBasePositions.clear();
    for (auto *baseItem : allItems)
    {
        if (GradedIsReference(baseItem))
        {
            m_gradedBasePositions.insert(GradedPieceId(baseItem), baseItem->pos());
        }
    }

    // Positions are final. Refresh the decor + children according to the change scope. forceFull
    // (a fresh capture) or any change that cannot be attributed to a single owner → full refresh
    // (always-correct default). A lone-piece drag touches nothing (its children are parented to
    // the piece and follow it); a child override refreshes only the affected piece or group — the
    // expensive content resolution (info blocks / annotations) never runs for the other owners.
    using ChangeKind = GradedChangeKind;
    ChangeKind kind = ChangeKind::None;
    quint32 ownerId = 0;
    if (!forceFull)
    {
        const GradedContentDiff diff = DiffGradedRangeContent(m_gradedAppliedRange, range);
        if (!diff.ambiguous)
        {
            kind = diff.isGroup ? (diff.offsetOnly ? ChangeKind::GroupOffset : ChangeKind::GroupChild)
                                : (diff.offsetOnly ? ChangeKind::PieceOffset : ChangeKind::PieceChild);
            ownerId = diff.ownerId;
        }
    }

    const bool pieceIsGrouped = ownerId != 0 && kind != ChangeKind::None
                                && kind != ChangeKind::GroupOffset && kind != ChangeKind::GroupChild
                                && pattern->GetPieceGroupId(ownerId) != NULL_ID;

    if (forceFull || kind == ChangeKind::None || kind == ChangeKind::GroupOffset || pieceIsGrouped)
    {
        // Full decor + children: entry/structural change, group drag (hosts are re-anchored and
        // every group child re-hosted) or a change not reducible to a single owner.
        RefreshGradedReferenceGroupDecor(pattern, m_sceneGraded);
        RefreshGradedReferenceGrainlines();
        RefreshGradedReferenceInfoBlocks();
        RefreshGradedReferenceAnnotations();
        RefreshGradedReferenceThumbnails();
    }
    else if (kind == ChangeKind::PieceOffset)
    {
        // Lone-piece drag: the children are parented to the reference item and follow it, so the
        // group hosts and every other owner's children are untouched — nothing to rebuild.
    }
    else if (kind == ChangeKind::PieceChild)
    {
        RefreshGradedReferenceGrainlines(ownerId, 0);
        RefreshGradedReferenceInfoBlocks(ownerId, 0);
        RefreshGradedReferenceAnnotations(ownerId, 0);
        RefreshGradedReferenceThumbnails(ownerId, 0);
    }
    else // GroupChild
    {
        RefreshGradedReferenceGrainlines(0, ownerId);
        RefreshGradedReferenceInfoBlocks(0, ownerId);
        RefreshGradedReferenceAnnotations(0, ownerId);
        RefreshGradedReferenceThumbnails(0, ownerId);
    }

    // Remember the applied content so the next deferred apply can diff against it.
    if (ranges != nullptr && ranges->contains(m_activeGradedRangeName))
    {
        m_gradedAppliedRange = ranges->value(m_activeGradedRangeName);
    }
    else
    {
        m_gradedAppliedRange = OSizerange{};
    }
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::GradedMouseReleased()
{
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Graded || doc == nullptr
        || m_activeGradedRangeName.isEmpty() || m_sceneGraded == nullptr)
    {
        return;
    }

    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }

    const OSizerange oldRange = ranges->value(m_activeGradedRangeName);
    OSizerange newRange = oldRange;
    bool changed = false;

    const auto items = m_sceneGraded->items();
    const auto *groups = pattern->DataPieceGroups();
    const auto GroupOfPiece = [groups](quint32 pieceId) -> quint32 {
        if (groups == nullptr)
        {
            return 0;
        }
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            if (git.value().pieceIds.contains(pieceId))
            {
                return git.key();
            }
        }
        return 0;
    };

    for (auto *baseItem : items)
    {
        if (!GradedIsReference(baseItem))
        {
            continue;
        }

        const quint32 pieceId = GradedPieceId(baseItem);
        const auto baseIt = m_gradedBasePositions.constFind(pieceId);
        if (baseIt == m_gradedBasePositions.cend())
        {
            continue;
        }

        const QPointF delta = baseItem->pos() - baseIt.value();
        if (qAbs(delta.x()) < 0.5 && qAbs(delta.y()) < 0.5)
        {
            continue;
        }

        // The dragged piece belongs to a group → move the whole group as one rigid unit:
        // fan the same delta to every member × every layer, persisted as a group offset
        // (a member never drags alone, like in Details).
        if (const quint32 groupId = GroupOfPiece(pieceId); groupId != 0)
        {
            const QList<quint32> memberIds = groups->value(groupId).pieceIds;
            OSizerange::GroupContent &content = newRange.groupContent[groupId];
            content.dx += delta.x();
            content.dy += delta.y();

            for (auto *otherItem : items)
            {
                if (otherItem != baseItem)
                {
                    const quint32 otherId = GradedPieceId(otherItem);
                    if (otherId != 0 && memberIds.contains(otherId))
                    {
                        otherItem->setPos(otherItem->pos() + delta);
                    }
                }
            }
            // Refresh the drag base of every reference member of the group.
            for (auto *refItem : items)
            {
                if (GradedIsReference(refItem))
                {
                    const quint32 refId = GradedPieceId(refItem);
                    if (refId != 0 && memberIds.contains(refId))
                    {
                        m_gradedBasePositions.insert(refId, refItem->pos());
                    }
                }
            }
            changed = true;
            continue;
        }

        // Lone piece: existing per-piece path.
        OSizerange::PieceContent &content = newRange.pieceContent[pieceId];
        content.dx += delta.x();
        content.dy += delta.y();
        changed = true;
        m_gradedBasePositions.insert(pieceId, baseItem->pos());

        // Fan the offset out to every other layer (size) of the same piece.
        for (auto *otherItem : items)
        {
            if (otherItem != baseItem && GradedPieceId(otherItem) == pieceId)
            {
                otherItem->setPos(otherItem->pos() + delta);
            }
        }
    }

    if (changed)
    {
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SaveGradedRangeContent(doc, pattern, oldRange, newRange));
    }
}
