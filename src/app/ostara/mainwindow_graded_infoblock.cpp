/************************************************************************
 **
 **  @file   mainwindow_graded_infoblock.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: interactive info blocks on the reference layer (lone pieces + groups).
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
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vwidgets/ocartouchepainter.h"
#include "widgets/InfoBlock/OInfoBlockResolver.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"

#include <QGraphicsItem>
#include <QHash>
#include <QObject>
#include <QtGlobal>
#include <QUndoStack>

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// Marker of an interactive info-block item (carries the owner piece/group id).
constexpr int kGradedInfoBlockRole = Qt::UserRole + 905;

} // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreGradedInfoBlock(quint32 ownerId, bool isGroup, int block, const OInfoBlockGeom &geom)
{
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange oldRange = ranges->value(m_activeGradedRangeName);
    OSizerange newRange = oldRange;
    OSizerange::BlockOverride override;
    override.valid = true;
    override.geom = geom;
    if (isGroup)
    {
        newRange.groupContent[ownerId].blocks.insert(block, override);
    }
    else
    {
        newRange.pieceContent[ownerId].blocks.insert(block, override);
    }
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SaveGradedRangeContent(doc, pattern, oldRange, newRange));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGradedReferenceInfoBlocks(quint32 onlyPieceId, quint32 onlyGroupId)
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

    // Drop previous interactive block items, then re-fetch (the old list must not be reused).
    // When a targeted owner is given, only that owner's children are dropped.
    auto items = m_sceneGraded->items();
    for (auto *owner : items)
    {
        const quint32 ownerPiece = GradedPieceId(owner);
        const quint32 ownerGroup =
            ownerPiece == 0 ? owner->data(kGradedHostGroupIdRole).toUInt() : 0;
        if (ownerPiece == 0 && ownerGroup == 0)
        {
            continue;
        }
        if (onlyPieceId != 0 && (ownerPiece == 0 || ownerPiece != onlyPieceId))
        {
            continue;
        }
        if (onlyGroupId != 0 && (ownerGroup == 0 || ownerGroup != onlyGroupId))
        {
            continue;
        }
        const auto children = owner->childItems();
        for (auto *child : children)
        {
            if (!child->data(kGradedInfoBlockRole).isNull())
            {
                delete child;
            }
        }
    }
    items = m_sceneGraded->items();

    const auto *piecesMap = pattern->DataPieces();
    const auto *groups = pattern->DataPieceGroups();

    // Shared builder: instantiate one OInfoBlockItem per active block of an owner, from the
    // Details base geometry or the per-range override, and wire move/resize to the range.
    constexpr qreal kBlockStackingGap = 12.0;
    auto BuildOwnerBlocks = [this, &range](QGraphicsItem *owner, quint32 ownerId, bool isGroup,
                                           const OInfoBlockData &baseData,
                                           const OInfoBlockEffectiveState &state) {
        const QPointF center = owner->boundingRect().center();
        const QList<int> blockNumbers = state.blockContent.keys();
        for (int block : blockNumbers)
        {
            const auto lines = state.blockContent.value(block);
            if (lines.isEmpty())
            {
                continue;
            }

            OSizerange::BlockOverride override;
            if (isGroup)
            {
                const auto it = range.groupContent.constFind(ownerId);
                if (it != range.groupContent.cend())
                {
                    override = it->blocks.value(block);
                }
            }
            else
            {
                const auto it = range.pieceContent.constFind(ownerId);
                if (it != range.pieceContent.cend())
                {
                    override = it->blocks.value(block);
                }
            }

            OInfoBlockGeom geom = baseData.BlockGeom(block);
            if (override.valid)
            {
                geom.pos = override.geom.pos;
                if (override.geom.fontScale > 0.0)
                {
                    geom.fontScale = override.geom.fontScale;
                }
            }
            const QSizeF natural = OCartouchePainter::NaturalSize(lines);
            const qreal scale = geom.fontScale > 0.0 ? geom.fontScale : 1.0;
            geom.width = scale * natural.width();
            geom.height = scale * natural.height();
            if (!override.valid && !baseData.HasStoredGeometry(block))
            {
                geom.pos = center + QPointF(0.0, (block - 1) * (geom.height + kBlockStackingGap));
            }

            auto *blk = new OInfoBlockItem(block, owner);
            blk->setData(kGradedInfoBlockRole, ownerId);
            blk->SetBlockGeom(geom);
            blk->SetContent(lines, geom.fontScale);
            blk->show();

            connect(blk, &OInfoBlockItem::Moved, this,
                    [this, ownerId, isGroup, block, baseData](int, const QPointF &p) {
                        const auto *r = pattern->SizeRanges();
                        if (r == nullptr || !r->contains(m_activeGradedRangeName))
                        {
                            return;
                        }
                        qreal fontScale = baseData.BlockGeom(block).fontScale;
                        const OSizerange cur = r->value(m_activeGradedRangeName);
                        const OSizerange::BlockOverride ov =
                            isGroup ? cur.groupContent.value(ownerId).blocks.value(block)
                                    : cur.pieceContent.value(ownerId).blocks.value(block);
                        if (ov.valid && ov.geom.fontScale > 0.0)
                        {
                            fontScale = ov.geom.fontScale;
                        }
                        if (fontScale <= 0.0)
                        {
                            fontScale = 1.0;
                        }
                        OInfoBlockGeom g;
                        g.pos = p;
                        g.fontScale = fontScale;
                        StoreGradedInfoBlock(ownerId, isGroup, block, g);
                    });
            connect(blk, &OInfoBlockItem::Resized, this,
                    [this, blk, ownerId, isGroup, block, baseData](int, qreal newW, qreal newH, qreal origW,
                                                                  qreal) {
                        const auto *r = pattern->SizeRanges();
                        if (r == nullptr || !r->contains(m_activeGradedRangeName))
                        {
                            return;
                        }
                        qreal fontScale = baseData.BlockGeom(block).fontScale;
                        const OSizerange cur = r->value(m_activeGradedRangeName);
                        const OSizerange::BlockOverride ov =
                            isGroup ? cur.groupContent.value(ownerId).blocks.value(block)
                                    : cur.pieceContent.value(ownerId).blocks.value(block);
                        if (ov.valid && ov.geom.fontScale > 0.0)
                        {
                            fontScale = ov.geom.fontScale;
                        }
                        if (fontScale <= 0.0)
                        {
                            fontScale = 1.0;
                        }
                        if (origW > 0.0)
                        {
                            fontScale *= newW / origW;
                        }
                        OInfoBlockGeom g;
                        g.pos = blk->pos();
                        g.width = newW;
                        g.height = newH;
                        g.fontScale = fontScale;
                        StoreGradedInfoBlock(ownerId, isGroup, block, g);
                    });
        }
    };

    // Lone (non-grouped) reference pieces.
    for (auto *item : items)
    {
        if (!GradedIsReference(item))
        {
            continue;
        }
        const quint32 pieceId = GradedPieceId(item);
        if (pieceId == 0 || piecesMap == nullptr || !piecesMap->contains(pieceId))
        {
            continue;
        }
        if (onlyGroupId != 0)
        {
            continue; // targeted rebuild of a group only
        }
        if (onlyPieceId != 0 && pieceId != onlyPieceId)
        {
            continue; // targeted rebuild of another lone piece
        }
        if (pattern->GetPieceGroupId(pieceId) != NULL_ID)
        {
            continue; // grouped → the group hosts the (single) set of blocks
        }
        const VPiece &piece = piecesMap->value(pieceId);
        const OInfoBlockEffectiveState state = OInfoBlockResolver::ResolveEffectiveState(piece, doc, pattern);
        if (!state.active)
        {
            continue;
        }
        BuildOwnerBlocks(item, pieceId, false, piece.GetInfoBlockData(), state);
    }

    // Groups.
    for (auto *item : items)
    {
        if (item->data(kGradedHostGroupIdRole).isNull() || groups == nullptr)
        {
            continue;
        }
        const quint32 groupId = item->data(kGradedHostGroupIdRole).toUInt();
        if (!groups->contains(groupId))
        {
            continue;
        }
        if (onlyPieceId != 0)
        {
            continue; // targeted rebuild of a lone piece only
        }
        if (onlyGroupId != 0 && groupId != onlyGroupId)
        {
            continue; // targeted rebuild of another group
        }
        const OPieceGroup &group = groups->value(groupId);
        const OInfoBlockEffectiveState state = OInfoBlockResolver::ResolveEffectiveState(group, doc, pattern);
        if (!state.active)
        {
            continue;
        }
        BuildOwnerBlocks(item, groupId, true, group.GetInfoBlockData(), state);
    }
}
