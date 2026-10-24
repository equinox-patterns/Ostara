/************************************************************************
 **
 **  @file   mainwindow_graded_grainline.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: interactive grainlines on the reference layer (lone pieces + groups).
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
#include "../vlayout/vgraphicsfillitem.h"
#include "../vmisc/def.h"
#include "../vwidgets/vgrainlineitem.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"

#include <QGraphicsItem>
#include <QHash>
#include <QObject>
#include <QtGlobal>
#include <QtMath>
#include <QUndoStack>

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// Marker of an interactive grainline item (carries the owner piece/group id).
constexpr int kGradedGrainlineOwnerRole = Qt::UserRole + 904;

} // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreGradedGrainline(quint32 ownerId, bool isGroup, const OSizerange::GrainlineOverride &grainline)
{
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange oldRange = ranges->value(m_activeGradedRangeName);
    OSizerange newRange = oldRange;
    if (isGroup)
    {
        newRange.groupContent[ownerId].grainline = grainline;
    }
    else
    {
        newRange.pieceContent[ownerId].grainline = grainline;
    }
    VAbstractApplication::VApp()->getUndoStack()->push(
        new SaveGradedRangeContent(doc, pattern, oldRange, newRange));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGradedReferenceGrainlines(quint32 onlyPieceId, quint32 onlyGroupId)
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

    // 1) Drop previous interactive grainline items (children of reference pieces / group hosts).
    //    When a targeted owner is given, only that owner's children are dropped.
    auto items = m_sceneGraded->items();
    for (auto *owner : items)
    {
        const quint32 ownerPiece = GradedDecorOwnerPiece(owner); // a piece, or its own decor deck
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
            if (!child->data(kGradedGrainlineOwnerRole).isNull())
            {
                delete child;
            }
        }
    }
    items = m_sceneGraded->items(); // fresh — the deleted children above must not be re-walked

    const auto *piecesMap = pattern->DataPieces();
    const auto *groups = pattern->DataPieceGroups();
    const auto rotationDeg = [](const QString &formula) -> qreal {
        bool ok = false;
        const qreal value = formula.toDouble(&ok);
        return ok ? value : 0.0;
    };

    // 2) Reference pieces with an enabled grainline → interactive item. A group member keeps its
    //    own here, like in Details: the group's shared grainline is a second, separate item, hosted
    //    on the group (step 3), and each of the two is re-placed per size range on its own.
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
            continue; // targeted rebuild of another piece
        }

        const VPiece &piece = piecesMap->value(pieceId);
        const VGrainlineData geom = piece.GetGrainlineGeometry();
        if (!geom.IsEnabled() || !geom.IsVisible())
        {
            continue;
        }

        const auto pit = range.pieceContent.constFind(pieceId);
        const bool ov = (pit != range.pieceContent.cend() && pit->grainline.valid);
        QPointF pos = ov ? pit->grainline.pos : geom.GetPos();
        if (qFuzzyIsNull(pos.x()) && qFuzzyIsNull(pos.y()))
        {
            pos = item->boundingRect().center(); // parent-local default
        }
        const qreal rot = rotationDeg(geom.GetRotation());
        const qreal baseLenPx =
            ToPixel((geom.GetLength().isEmpty() ? 10.0 : geom.GetLength().toDouble()),
                    *pattern->GetPatternUnit());
        const qreal lenPx = (ov && pit->grainline.lengthPx > 0.0) ? pit->grainline.lengthPx : baseLenPx;

        // Hide the static grainline GetItem drew — the interactive item replaces it. It is hidden
        // where the interactive one is hung: on the piece's own item, or on a member's view (the
        // group's layer keeps none — the group's shared grainline is hosted on the group).
        QGraphicsItem *owner = GradedPieceDecorOwner(m_sceneGraded, pieceId, item);
        const auto children = owner->childItems();
        for (auto *child : children)
        {
            if (child->type() == VGraphicsFillItem::Type)
            {
                child->hide();
            }
        }

        auto *gl = new VGrainlineItem(VColorRole::PieceColor, GradedDecorHost(owner));
        gl->setData(kGradedGrainlineOwnerRole, pieceId);
        gl->setFlag(QGraphicsItem::ItemIsMovable, true);
        gl->SetMoveType(VPieceItem::MoveTypes(VPieceItem::IsMovable | VPieceItem::IsResizable));
        gl->UpdateGeometry(pos, rot, lenPx, geom.GetArrowType());

        connect(gl, &VGrainlineItem::SignalMoved, this, [this, pieceId](const QPointF &p) {
            const auto *r = pattern->SizeRanges();
            if (r == nullptr || !r->contains(m_activeGradedRangeName))
            {
                return;
            }
            const OSizerange oldR = r->value(m_activeGradedRangeName);
            OSizerange newR = oldR; // carry over any existing length
            newR.pieceContent[pieceId].grainline.valid = true;
            newR.pieceContent[pieceId].grainline.pos = p;
            VAbstractApplication::VApp()->getUndoStack()->push(
                new SaveGradedRangeContent(doc, pattern, oldR, newR));
        });
        connect(gl, &VGrainlineItem::SignalResized, this, [this, gl, pieceId](qreal lengthPx) {
            const auto *r = pattern->SizeRanges();
            if (r == nullptr || !r->contains(m_activeGradedRangeName))
            {
                return;
            }
            const OSizerange oldR = r->value(m_activeGradedRangeName);
            OSizerange newR = oldR; // carry over any existing position
            newR.pieceContent[pieceId].grainline.valid = true;
            newR.pieceContent[pieceId].grainline.pos = gl->pos();
            newR.pieceContent[pieceId].grainline.lengthPx = lengthPx;
            VAbstractApplication::VApp()->getUndoStack()->push(
                new SaveGradedRangeContent(doc, pattern, oldR, newR));
        });
    }

    // 3) Groups: interactive grainline on the invisible host (its bounding rect = union box).
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
        const VGrainlineData geom = group.m_glGrainline;
        if (!geom.IsEnabled() || !geom.IsVisible())
        {
            continue;
        }

        const auto git = range.groupContent.constFind(groupId);
        const bool ov = (git != range.groupContent.cend() && git->grainline.valid);
        QPointF pos = ov ? git->grainline.pos : geom.GetPos();
        if (qFuzzyIsNull(pos.x()) && qFuzzyIsNull(pos.y()))
        {
            pos = item->boundingRect().center(); // host-local default (union centre)
        }
        const qreal rot = rotationDeg(geom.GetRotation());
        const qreal baseLenPx =
            ToPixel((geom.GetLength().isEmpty() ? 10.0 : geom.GetLength().toDouble()),
                    *pattern->GetPatternUnit());
        const qreal lenPx = (ov && git->grainline.lengthPx > 0.0) ? git->grainline.lengthPx : baseLenPx;

        auto *gl = new VGrainlineItem(VColorRole::PieceColor, item); // the group's own, on its host
        gl->setData(kGradedGrainlineOwnerRole, groupId);
        gl->setFlag(QGraphicsItem::ItemIsMovable, true);
        gl->SetMoveType(VPieceItem::MoveTypes(VPieceItem::IsMovable | VPieceItem::IsResizable));
        gl->UpdateGeometry(pos, rot, lenPx, geom.GetArrowType());

        connect(gl, &VGrainlineItem::SignalMoved, this, [this, groupId](const QPointF &p) {
            const auto *r = pattern->SizeRanges();
            if (r == nullptr || !r->contains(m_activeGradedRangeName))
            {
                return;
            }
            const OSizerange oldR = r->value(m_activeGradedRangeName);
            OSizerange newR = oldR;
            newR.groupContent[groupId].grainline.valid = true;
            newR.groupContent[groupId].grainline.pos = p;
            VAbstractApplication::VApp()->getUndoStack()->push(
                new SaveGradedRangeContent(doc, pattern, oldR, newR));
        });
        connect(gl, &VGrainlineItem::SignalResized, this, [this, gl, groupId](qreal lengthPx) {
            const auto *r = pattern->SizeRanges();
            if (r == nullptr || !r->contains(m_activeGradedRangeName))
            {
                return;
            }
            const OSizerange oldR = r->value(m_activeGradedRangeName);
            OSizerange newR = oldR;
            newR.groupContent[groupId].grainline.valid = true;
            newR.groupContent[groupId].grainline.pos = gl->pos();
            newR.groupContent[groupId].grainline.lengthPx = lengthPx;
            VAbstractApplication::VApp()->getUndoStack()->push(
                new SaveGradedRangeContent(doc, pattern, oldR, newR));
        });
    }
}
