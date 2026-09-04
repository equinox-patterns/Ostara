/************************************************************************
 **
 **  @file   mainwindow_graded_group.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: mask grouped members' own decor and host the shared group grainline.
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

#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QHash>
#include <QObject>
#include <QtGlobal>


namespace
{
/// A grouped member masks its own decor: grainline hidden everywhere, fold line kept only on
/// the member that carries the group's fold (mirror of Details + the SVG export path).
void HideGroupedReferenceMemberDecor(QGraphicsItem *memberItem, quint32 pieceId, const OPieceGroup &group)
{
    const bool foldCarrier = (group.m_foldLineMemberId == pieceId);
    const QList<QGraphicsItem *> children = memberItem->childItems();
    for (QGraphicsItem *child : children)
    {
        if (child->type() == VGraphicsFillItem::Type)
        {
            child->hide(); // member's grainline → the group host shows the shared one
        }
        else if (!foldCarrier && (child->type() == VGraphicsFoldLineItem::Type
                                  || child->type() == QGraphicsSimpleTextItem::Type))
        {
            child->hide(); // fold mark/text, unless this member is the group fold carrier
        }
    }
}
} // namespace

/// Rebuild the per-group reference decoration (run once positions are final): mask the grouped
/// members' own grainline/fold and host the GROUP's shared grainline on an invisible root whose
/// anchor is the union of the graded reference members — like the Details group holder, but
/// anchored to the graded items (mirror of the SVG export, mainwindowsnogui.cpp:657-747).
void RefreshGradedReferenceGroupDecor(VContainer *pattern, QGraphicsScene *scene)
{
    if (pattern == nullptr || scene == nullptr)
    {
        return;
    }

    // Drop previous hosts (and, with them, their interactive children) — they are re-created
    // from scratch now that positions are final. Collect the hosts to delete FIRST, then
    // delete, so the code never dereferences an already-freed child inside the snapshot
    // (ordering-independent, robust to future reparenting).
    const auto stale = scene->items();
    QList<QGraphicsItem *> hostsToDelete;
    for (auto *item : stale)
    {
        if (!item->data(kGradedHostGroupIdRole).isNull())
        {
            hostsToDelete.append(item);
        }
    }
    for (auto *host : hostsToDelete)
    {
        scene->removeItem(host);
        delete host;
    }
    const auto items = scene->items();

    // 1) Mask the decor that must NOT appear per piece:
    //    - ghost layers (any piece): no grainline/fold — only the reference size carries decor;
    //    - reference grouped members: own grainline hidden, own fold kept only on the fold
    //      carrier (the group host shows the shared grainline instead, like Details).
    QHash<quint32, QRectF> groupBounds;
    for (auto *item : items)
    {
        const quint32 pieceId = GradedPieceId(item);
        if (pieceId == 0)
        {
            continue;
        }

        if (not GradedIsReference(item))
        {
            // Ghost layer: the sizes under the reference carry no per-piece decor.
            const QList<QGraphicsItem *> children = item->childItems();
            for (QGraphicsItem *child : children)
            {
                if (child->type() == VGraphicsFillItem::Type
                    || child->type() == VGraphicsFoldLineItem::Type
                    || child->type() == QGraphicsSimpleTextItem::Type)
                {
                    child->hide();
                }
            }
            continue;
        }

        // Reference layer.
        const quint32 gid = pattern->GetPieceGroupId(pieceId);
        if (gid == NULL_ID)
        {
            continue; // a lone piece keeps its own grainline/fold on the reference
        }
        const OPieceGroup &group = pattern->DataPieceGroups()->value(gid);
        HideGroupedReferenceMemberDecor(item, pieceId, group);

        const QRectF r = item->sceneBoundingRect();
        QRectF &b = groupBounds[gid];
        b = b.isNull() ? r : b.united(r);
    }
    if (groupBounds.isEmpty())
    {
        return; // no group on the reference → nothing to host
    }

    // Highest layer z, so the host draws above the reference pieces without intercepting clicks.
    qreal topZ = 0.0;
    for (auto *item : items)
    {
        if (GradedPieceId(item) != 0)
        {
            topZ = qMax(topZ, item->zValue());
        }
    }

    // 2) per-group invisible host sized to the union box: it gives the interactive group
    //    grainline (built by RefreshGradedReferenceGrainlines) a valid local frame to clamp
    //    into, without intercepting any mouse event itself.
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
