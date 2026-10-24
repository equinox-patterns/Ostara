/************************************************************************
 **
 **  @file   mainwindow_graded_group.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: hang the group's shared children on a host, and mark every member.
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

#include "mainwindowsnogui.h"
#include "mainwindow_graded_helpers.h"
#include "xml/vpattern.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vlayout/vgraphicsfillitem.h"
#include "../vmisc/def.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/ogroupmembershipmark.h"

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QHash>
#include <QObject>
#include <QtGlobal>

namespace
{
/**
 * @brief The invisible deck a reference piece hangs its own children on.
 *
 * It is not a rect item: its shape() is empty, so no click and no PickItem ever lands on the deck
 * itself — the pieces and the children keep every gesture, whichever deck is on top. Only its
 * boundingRect() matters, because the decor builders use the owner's rect as the frame a child that
 * was never placed is stacked in, and that frame must be the piece's.
 */
class DeckItem : public QGraphicsItem
{
public:
    DeckItem(const QRectF &frame, QGraphicsItem *parent)
        : QGraphicsItem(parent)
        , m_frame(frame)
    {
        setFlag(QGraphicsItem::ItemIsSelectable, false);
        setFlag(QGraphicsItem::ItemIsMovable, false);
        setFlag(QGraphicsItem::ItemIsFocusable, false);
        setAcceptedMouseButtons(Qt::NoButton);
    }

    auto boundingRect() const -> QRectF override { return m_frame; }
    auto shape() const -> QPainterPath override { return {}; } // never picked, never in the way
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override {}

private:
    QRectF m_frame{};
};

/**
 * What a group's stack item shows of a member: its geometry, and nothing the group does not print.
 *
 * The stack is the group — what it draws, what it prints. The group's shared grainline is hosted on
 * the group itself, and the group's fold is the carrier member's, so a member's own static grainline
 * has no place here, and neither has its fold unless it carries the group's. The member shows both
 * on its own view, where it is edited.
 */
void HideStackMemberDecor(QGraphicsItem *stackItem, quint32 pieceId, const OPieceGroup &group)
{
    const bool foldCarrier = (group.m_foldLineMemberId == pieceId);
    const QList<QGraphicsItem *> children = stackItem->childItems();
    for (QGraphicsItem *child : children)
    {
        if (child->type() == VGraphicsFillItem::Type)
        {
            child->hide(); // the group's shared grainline is hosted on the group
        }
        else if (!foldCarrier && (child->type() == VGraphicsFoldLineItem::Type
                                  || child->type() == QGraphicsSimpleTextItem::Type))
        {
            child->hide(); // only the carrier's fold is the group's
        }
    }
}

/// A ghost layer carries no per-piece decor: only the reference size shows a grainline or a fold.
void HideGhostLayerDecor(QGraphicsItem *item)
{
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
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto GradedMemberView(QGraphicsScene *scene, quint32 pieceId) -> QGraphicsItem *
{
    if (scene == nullptr || pieceId == 0)
    {
        return nullptr;
    }
    const auto items = scene->items();
    for (auto *item : items)
    {
        if (GradedMemberViewPiece(item) == pieceId && GradedIsReference(item))
        {
            return item;
        }
    }
    return nullptr;
}

/// The deck a reference piece hangs its own children on: an invisible frame in the piece's own
/// coordinates, one step above the group's host, so a piece's child and a group's child are both
/// reachable (see kGradedDecorDeckZ). Created on first use — the decor passes share it, the piece
/// deletes it with itself, and an empty shape keeps it out of every gesture.
auto GradedDecorHost(QGraphicsItem *pieceItem) -> QGraphicsItem *
{
    if (pieceItem == nullptr)
    {
        return nullptr;
    }
    const auto children = pieceItem->childItems();
    for (auto *child : children)
    {
        if (!child->data(kGradedDecorHostRole).isNull())
        {
            return child;
        }
    }

    auto *deck = new DeckItem(pieceItem->boundingRect(), pieceItem);
    deck->setZValue(kGradedDecorDeckZ);
    deck->setData(kGradedDecorHostRole, true);
    return deck;
}

/// Rebuild the per-group reference decoration (run once positions are final): mask what a ghost
/// layer must not show, hang the "Member of <group>" mark above each grouped reference member, and
/// host the GROUP's shared children on an invisible root whose anchor is the union of the graded
/// reference members — like the Details group holder, but anchored to the graded items (mirror of
/// the SVG export, mainwindowsnogui.cpp:657-747).
///
/// A grouped member keeps its OWN children in Graded, exactly as in Details: they are what a size
/// range re-places (pieceContent), and masking them is what kept a member from being edited here.
/// The group's own children (shared grainline, blocks, annotations, per-member miniatures) stay on
/// the host. The host is placed UNDER the reference pieces: a member's own children are the ones a
/// size range re-places, so they must be the ones a click lands on where the two coincide.
void RefreshGradedReferenceGroupDecor(VContainer *pattern, QGraphicsScene *scene)
{
    if (pattern == nullptr || scene == nullptr)
    {
        return;
    }

    // Drop previous hosts and marks (and, with them, their interactive children) — they are
    // re-created from scratch now that positions are final. Collect what to delete FIRST, then
    // delete, so the code never dereferences an already-freed child inside the snapshot
    // (ordering-independent, robust to future reparenting).
    const auto stale = scene->items();
    QList<QGraphicsItem *> itemsToDelete;
    for (auto *item : stale)
    {
        if (!item->data(kGradedHostGroupIdRole).isNull() || !item->data(kGradedMemberMarkRole).isNull())
        {
            itemsToDelete.append(item);
        }
    }
    for (auto *item : itemsToDelete)
    {
        scene->removeItem(item);
        delete item;
    }
    const auto items = scene->items();

    // 1) Mask the decor a ghost layer must not carry, then collect what the reference layer needs:
    //    the bounds of every group's reference members (the host's frame) and one mark per member.
    struct MarkRequest
    {
        QGraphicsItem *item;
        quint32 pieceId;
        QString caption;
    };
    QHash<quint32, QRectF> groupBounds;
    QVector<MarkRequest> marks;
    for (auto *item : items)
    {
        const quint32 pieceId = GradedPieceId(item);
        if (pieceId == 0 && GradedMemberViewPiece(item) == 0)
        {
            continue; // not a piece layer (group host, decor deck, interactive child)
        }

        // Only the smallest size carries a grainline or a fold — whichever stack the layer belongs
        // to: a group's layer or a member's own. The member's stack is graded, but its children are
        // not: they are edited on the reference layer and read there alone.
        if (not GradedIsReference(item))
        {
            HideGhostLayerDecor(item);
            continue;
        }

        if (pieceId == 0)
        {
            continue; // the reference member view: it is the edited one, its own decor stays
        }

        // Reference layer: a member of a group is marked and feeds the group's bounds; a lone piece
        // keeps everything it has and needs neither.
        const quint32 gid = pattern->GetPieceGroupId(pieceId);
        if (gid == NULL_ID || pattern->DataPieceGroups() == nullptr)
        {
            continue;
        }
        const OPieceGroup &group = pattern->DataPieceGroups()->value(gid);
        HideStackMemberDecor(item, pieceId, group);
        marks.append({item, pieceId, OGroupMark::Caption(group.name)});

        const QRectF r = item->sceneBoundingRect();
        QRectF &b = groupBounds[gid];
        b = b.isNull() ? r : b.united(r);
    }
    if (groupBounds.isEmpty())
    {
        return; // no group on the reference → nothing to mark, nothing to host
    }

    // 2) The mark: a dashed frame and "Member of <group>" — the same drawing Details paints on the
    //    piece itself, here carried by an item because a graded piece is a VLayoutPiece, not a tool.
    //    It follows the member's editing view: a member pulled aside carries its mark with it.
    for (const MarkRequest &request : marks)
    {
        QGraphicsItem *owner = GradedPieceDecorOwner(scene, request.pieceId, request.item);
        auto *mark = new OGroupMembershipMarkItem(GradedDecorHost(owner));
        mark->setData(kGradedMemberMarkRole, request.pieceId);
        mark->SetMark(owner->boundingRect(), request.caption, *pattern->GetPatternUnit());
    }

    // Highest layer z, so the host draws above the reference pieces without intercepting clicks.
    // A piece's own children do not lose to it: they hang one step higher, on their own deck
    // (GradedDecorHost / kGradedDecorDeckZ).
    qreal topZ = 0.0;
    for (auto *item : items)
    {
        if (GradedPieceId(item) != 0)
        {
            topZ = qMax(topZ, item->zValue());
        }
    }

    // 3) per-group invisible host sized to the union box: it gives the group's own children (the
    //    shared grainline, the group's blocks and annotations, the per-member miniatures) a valid
    //    local frame to sit in, without intercepting any mouse event itself.
    for (auto it = groupBounds.cbegin(); it != groupBounds.cend(); ++it)
    {
        const QRectF &box = it.value();
        auto *host = new QGraphicsPathItem();
        host->setPos(box.topLeft());
        QPainterPath hostPath;
        hostPath.addRect(QRectF(QPointF(0.0, 0.0), box.size()));
        host->setPath(hostPath);
        host->setPen(Qt::NoPen);
        host->setBrush(Qt::NoBrush);
        host->setZValue(topZ + 1.0);
        host->setFlag(QGraphicsItem::ItemIsSelectable, false);
        host->setFlag(QGraphicsItem::ItemIsMovable, false);
        host->setAcceptedMouseButtons(Qt::NoButton);
        host->setData(kGradedHostGroupIdRole, it.key());
        scene->addItem(host);
    }
}
