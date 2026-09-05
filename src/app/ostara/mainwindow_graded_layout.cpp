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
              && a.annotations == it.value().annotations && a.annotationsFp == it.value().annotationsFp);
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
              && a.annotations == it.value().annotations && a.annotationsFp == it.value().annotationsFp);
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

    // Idempotent re-apply: alignment + stored offsets below are computed from the item's
    // frozen base (mx/my). Reset every item to that base first, so a re-apply (drag end,
    // undo/redo, rebuild) never stacks offsets on top of already-offset positions.
    const auto resetItems = m_sceneGraded->items();
    for (auto *item : resetItems)
    {
        if (GradedPieceId(item) != 0 && !item->data(kGradedBasePosRole).isNull())
        {
            item->setPos(item->data(kGradedBasePosRole).toPointF());
        }
    }

    // Re-align the size layers of a piece in SCENE coordinates, as a delta added to the
    // item's mx/my base (Val Fork BuildGradedDetailsScene) — never re-base to the local
    // origin, which would discard the piece's Details placement. Then add the per-range
    // content offsets. The model is the single source of truth.
    const auto AnchorPoint = [](const QRectF &rect, int h, int v) -> QPointF {
        const qreal hFrac = h == 1 ? 0.0 : (h == 3 ? 1.0 : 0.5);
        const qreal vFrac = v == 1 ? 0.0 : (v == 3 ? 1.0 : 0.5);
        return QPointF(rect.left() + rect.width() * hFrac, rect.top() + rect.height() * vFrac);
    };

    QHash<quint32, QList<QGraphicsItem *>> byPiece;
    const auto allItems = m_sceneGraded->items();
    for (auto *baseItem : allItems)
    {
        if (GradedPieceId(baseItem) != 0)
        {
            byPiece[GradedPieceId(baseItem)].append(baseItem);
        }
    }

    // Group membership: a member piece is never aligned/offset as an individual — the group
    // is the unit (same rule as Details). Ostara groups have no "no-align" sentinel
    // (alignH/V default 2 = center), so every group with ≥ 2 members is aligned as a unit.
    QHash<quint32, quint32> pieceGroupOf; // member piece id -> group id
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

    // Pass 1 — align each NON-grouped piece's size layers (additive delta, mx/my preserved).
    const auto *piecesMap = pattern->DataPieces();
    for (auto it = byPiece.cbegin(); it != byPiece.cend(); ++it)
    {
        if (pieceGroupOf.contains(it.key()))
        {
            continue; // group members are aligned as their group (pass 2)
        }

        const QList<QGraphicsItem *> &items = it.value();
        if (items.size() < 2)
        {
            continue; // a single layer keeps its Details (mx/my) position
        }

        int alignH = 2;
        int alignV = 2;
        if (piecesMap != nullptr && piecesMap->contains(it.key()))
        {
            const VPiece &piece = piecesMap->value(it.key());
            alignH = piece.GetGradedAlignH();
            alignV = piece.GetGradedAlignV();
        }

        // Capture the scene rects BEFORE any move (they include the mx/my base), then shift
        // each layer by the anchor delta only — position additive, never absolute re-base.
        QVector<QRectF> rects;
        rects.reserve(items.size());
        QRectF unionRect;
        for (const auto *item : items)
        {
            const QRectF r = item->sceneBoundingRect();
            rects.append(r);
            unionRect = unionRect.isNull() ? r : unionRect.united(r);
        }

        const QPointF target = AnchorPoint(unionRect, alignH, alignV);
        for (int i = 0; i < items.size(); ++i)
        {
            const QPointF delta = target - AnchorPoint(rects.at(i), alignH, alignV);
            if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
            {
                items.at(i)->setPos(items.at(i)->pos() + delta);
            }
        }
    }

    // Pass 2 — align each GROUP as a rigid unit (mirror of Val Fork BuildGradedDetailsScene's
    // group pass): per layer the union of the members, cross-layer overall union, then shift
    // every member of a layer by the same anchor delta.
    if (groups != nullptr)
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            const OPieceGroup &group = git.value();
            if (group.pieceIds.size() < 2)
            {
                continue;
            }

            QMap<qreal, QList<QGraphicsItem *>> itemsByLayer; // z -> items
            QMap<qreal, QRectF> rectByLayer;                  // z -> pre-move member union
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

            for (auto it = rectByLayer.cbegin(); it != rectByLayer.cend(); ++it)
            {
                const QPointF delta = AnchorPoint(overallRect, group.alignH, group.alignV)
                                      - AnchorPoint(it.value(), group.alignH, group.alignV);
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

    // Per-range content offsets (drag deltas persisted in the size range). A group member
    // only ever moves through its group's offset — never through a per-piece one.
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
    }
    else // GroupChild
    {
        RefreshGradedReferenceGrainlines(0, ownerId);
        RefreshGradedReferenceInfoBlocks(0, ownerId);
        RefreshGradedReferenceAnnotations(0, ownerId);
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
