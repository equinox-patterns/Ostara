/************************************************************************
 **
 **  @file   mainwindow_graded_alignmenu.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Graded mode: right-click alignment menu on a piece/group + single-unit realign.
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

#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vtools/undocommands/savepiecegroupoptions.h"
#include "../vtools/undocommands/savepieceoptions.h"
#include "../vtheme/vtheme.h"

#include <QAction>
#include <QCoreApplication>
#include <QGraphicsItem>
#include <QIcon>
#include <QMenu>
#include <QPoint>
#include <QRectF>
#include <QMap>
#include <QSet>
#include <QUndoStack>
#include <QVariant>
#include <QVector>

//---------------------------------------------------------------------------------------------------------------------
namespace
{

/// The alignment anchor fractions used by the graded geometry passes (mirror of the AnchorPoint
/// lambda in ApplyGradedLayoutOffsets): 1 = left/top, 2 = center, 3 = right/bottom.
auto GradedAnchorPoint(const QRectF &rect, int h, int v) -> QPointF
{
    const qreal hFrac = h == 1 ? 0.0 : (h == 3 ? 1.0 : 0.5);
    const qreal vFrac = v == 1 ? 0.0 : (v == 3 ? 1.0 : 0.5);
    return QPointF(rect.left() + rect.width() * hFrac, rect.top() + rect.height() * vFrac);
}

/// The QRC "check" icon, theme-aware (Light/Dark), marking the current alignment in the menu.
auto GradedCheckIcon() -> QIcon
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");
    return QIcon(QStringLiteral(":/icons/%1/check_%1.svg").arg(theme));
}

/// One selectable alignment value, in the same wording as the Details OGradedAlignWidget.
struct AlignMenuOption
{
    int value;        // 1/2/3
    const char *text; // English source (same context as OGradedAlignWidget)
};

const AlignMenuOption kAlignMenuH[] = {
    {1, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Left")},
    {2, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Center")},
    {3, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Right")},
};

const AlignMenuOption kAlignMenuV[] = {
    {1, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Top")},
    {2, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Center")},
    {3, QT_TRANSLATE_NOOP("OGradedAlignWidget", "Bottom")},
};

} // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::GradedAlignContextMenu(const QPoint &globalPos)
{
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Graded || m_sceneGraded == nullptr
        || pattern == nullptr || doc == nullptr || m_activeGradedRangeName.isEmpty())
    {
        return;
    }

    const QPointF scenePos = ui->view->mapToScene(ui->view->viewport()->mapFromGlobal(globalPos));

    // Resolve the unit under the cursor: the topmost scene item (or its parent chain) carrying a
    // graded piece id or a group-host id. A piece belonging to a group of ≥ 2 members is always
    // edited through its GROUP (same rule as ApplyGradedLayoutOffsets and the Details dock).
    quint32 pieceId = NULL_ID;
    quint32 groupId = NULL_ID;
    const auto itemsAtPos = m_sceneGraded->items(scenePos);
    for (auto *item : itemsAtPos)
    {
        for (QGraphicsItem *p = item; p != nullptr; p = p->parentItem())
        {
            const QVariant hostId = p->data(kGradedHostGroupIdRole);
            if (!hostId.isNull())
            {
                groupId = hostId.toUInt();
                break;
            }
            const quint32 pid = GradedPieceId(p);
            if (pid != NULL_ID)
            {
                pieceId = pid;
                break;
            }
        }
        if (groupId != NULL_ID || pieceId != NULL_ID)
        {
            break;
        }
    }
    if (pieceId == NULL_ID && groupId == NULL_ID)
    {
        return; // right-click on empty graded canvas → nothing
    }

    bool isGroup = (groupId != NULL_ID);
    quint32 unitId = groupId;
    if (!isGroup)
    {
        unitId = pieceId;
        const quint32 gid = pattern->GetPieceGroupId(pieceId);
        if (gid != NULL_ID)
        {
            const auto *groups = pattern->DataPieceGroups();
            if (groups != nullptr && groups->contains(gid)
                && groups->value(gid).pieceIds.size() >= 2)
            {
                isGroup = true;
                unitId = gid;
            }
        }
    }

    // Current alignment of the unit (model — the single source shared with the Details dock).
    int alignH = 2;
    int alignV = 2;
    if (isGroup)
    {
        const auto *groups = pattern->DataPieceGroups();
        if (groups == nullptr || !groups->contains(unitId))
        {
            return;
        }
        const OPieceGroup &group = groups->value(unitId);
        alignH = group.alignH;
        alignV = group.alignV;
    }
    else
    {
        if (const auto *pieces = pattern->DataPieces(); pieces != nullptr && pieces->contains(unitId))
        {
            const VPiece &piece = pieces->value(unitId);
            alignH = piece.GetGradedAlignH();
            alignV = piece.GetGradedAlignV();
        }
    }

    // The user's choice (axis + value), filled by the submenu action that is triggered.
    struct AlignChoice
    {
        bool vertical = false;
        int value = 2;
        bool valid = false;
    };
    AlignChoice choice;

    auto *topMenu = new QMenu();
    QMenu *hSub = topMenu->addMenu(tr("Horizontal alignment"));
    QMenu *vSub = topMenu->addMenu(tr("Vertical alignment"));

    const auto AddOption = [](QMenu *sub, const AlignMenuOption &option, int current) {
        // The option texts are declared with QT_TRANSLATE_NOOP("OGradedAlignWidget", …) —
        // translate them back with that SAME context (a MainWindow tr() here would miss them).
        QAction *action = sub->addAction(QCoreApplication::translate("OGradedAlignWidget", option.text));
        action->setData(option.value);
        if (option.value == current)
        {
            action->setIcon(GradedCheckIcon());
        }
        return action;
    };
    for (const AlignMenuOption &option : kAlignMenuH)
    {
        QAction *action = AddOption(hSub, option, alignH);
        connect(action, &QAction::triggered, topMenu, [&choice, option]() {
            choice.vertical = false;
            choice.value = option.value;
            choice.valid = true;
        });
    }
    for (const AlignMenuOption &option : kAlignMenuV)
    {
        QAction *action = AddOption(vSub, option, alignV);
        connect(action, &QAction::triggered, topMenu, [&choice, option]() {
            choice.vertical = true;
            choice.value = option.value;
            choice.valid = true;
        });
    }

    topMenu->exec(globalPos);
    delete topMenu;

    if (!choice.valid)
    {
        return; // menu dismissed
    }

    // No-op when the user re-picked the current alignment.
    if (choice.value == (choice.vertical ? alignV : alignH))
    {
        return;
    }

    // Flag the single-unit realign BEFORE the push: QUndoStack::push() emits indexChanged()
    // synchronously, and the graded hook consumes the flag to route to RealignGradedUnit instead
    // of the whole-canvas BuildGradedScene.
    m_gradedPendingAlignUnit = unitId;
    m_gradedPendingAlignIsGroup = isGroup;

    // Write the alignment through the SAME undo commands the Details dock uses: the model, the
    // .ostara DOM and the undo stack stay coherent between the two modes (single source of truth).
    if (isGroup)
    {
        const auto *groups = pattern->DataPieceGroups();
        if (groups == nullptr || !groups->contains(unitId))
        {
            m_gradedPendingAlignUnit = 0;
            m_gradedPendingAlignIsGroup = false;
            return;
        }
        OPieceGroup oldGroup = groups->value(unitId);
        OPieceGroup newGroup = oldGroup;
        if (choice.vertical)
        {
            newGroup.alignV = choice.value;
        }
        else
        {
            newGroup.alignH = choice.value;
        }
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SavePieceGroupOptions(oldGroup, newGroup, doc, pattern, unitId));
    }
    else
    {
        VPiece oldPiece = pattern->GetPiece(unitId);
        VPiece newPiece = oldPiece;
        if (choice.vertical)
        {
            newPiece.SetGradedAlignV(choice.value);
        }
        else
        {
            newPiece.SetGradedAlignH(choice.value);
        }
        VAbstractApplication::VApp()->getUndoStack()->push(
            new SavePieceOptions(oldPiece, newPiece, doc, unitId));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RealignGradedUnit(quint32 unitId, bool isGroup)
{
    if (unitId == NULL_ID || m_sceneGraded == nullptr || pattern == nullptr
        || m_activeGradedRangeName.isEmpty())
    {
        return;
    }
    const auto *ranges = pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(m_activeGradedRangeName))
    {
        return;
    }
    const OSizerange &range = ranges->value(m_activeGradedRangeName);

    QSet<quint32> memberIds;
    if (isGroup)
    {
        const auto *groups = pattern->DataPieceGroups();
        if (groups == nullptr || !groups->contains(unitId))
        {
            return;
        }
        const OPieceGroup &group = groups->value(unitId);
        if (group.pieceIds.size() < 2)
        {
            return;
        }
        for (quint32 id : group.pieceIds)
        {
            memberIds.insert(id);
        }
    }
    else
    {
        memberIds.insert(unitId);
    }

    // 1) Collect this unit's scene items and reset them to their frozen base (mx/my), exactly as
    //    ApplyGradedLayoutOffsets resets every item before realigning — here restricted to the
    //    unit, because a unit's alignment delta depends only on its own layers.
    const auto allItems = m_sceneGraded->items();
    QList<QGraphicsItem *> unitItems;
    for (auto *item : allItems)
    {
        const quint32 pid = GradedPieceId(item);
        if (pid == NULL_ID || !memberIds.contains(pid))
        {
            continue;
        }
        unitItems.append(item);
        if (!item->data(kGradedBasePosRole).isNull())
        {
            item->setPos(item->data(kGradedBasePosRole).toPointF());
        }
    }
    if (unitItems.isEmpty())
    {
        return;
    }

    // 2) Realign the unit — piece pass or group pass, mirroring ApplyGradedLayoutOffsets. The
    //    geometry step is conditional (a lone piece with a single layer has nothing to align; a
    //    group with < 2 members is not a unit), but steps 3-5 below always run.
    if (!isGroup)
    {
        if (unitItems.size() >= 2)
        {
            int alignH = 2;
            int alignV = 2;
            if (const auto *pieces = pattern->DataPieces(); pieces != nullptr && pieces->contains(unitId))
            {
                const VPiece &piece = pieces->value(unitId);
                alignH = piece.GetGradedAlignH();
                alignV = piece.GetGradedAlignV();
            }

            QRectF unionRect;
            QVector<QRectF> rects;
            rects.reserve(unitItems.size());
            for (const auto *item : unitItems)
            {
                const QRectF r = item->sceneBoundingRect();
                rects.append(r);
                unionRect = unionRect.isNull() ? r : unionRect.united(r);
            }
            const QPointF target = GradedAnchorPoint(unionRect, alignH, alignV);
            for (int i = 0; i < unitItems.size(); ++i)
            {
                const QPointF delta = target - GradedAnchorPoint(rects.at(i), alignH, alignV);
                if (!qFuzzyIsNull(delta.x()) || !qFuzzyIsNull(delta.y()))
                {
                    unitItems.at(i)->setPos(unitItems.at(i)->pos() + delta);
                }
            }
        }
    }
    else
    {
        // Group pass: union per layer of the members, cross-layer overall union, then shift every
        // member of a layer by the same anchor delta (rigid body per layer). Group presence and
        // member count were already validated at the top of this function.
        const OPieceGroup &group = pattern->DataPieceGroups()->value(unitId);
        if (group.pieceIds.size() >= 2)
        {
            QMap<qreal, QList<QGraphicsItem *>> itemsByLayer;
            QMap<qreal, QRectF> rectByLayer;
            for (quint32 memberId : group.pieceIds)
            {
                for (auto *item : allItems)
                {
                    if (GradedPieceId(item) != memberId)
                    {
                        continue;
                    }
                    const qreal z = item->zValue();
                    itemsByLayer[z].append(item);
                    const QRectF r = item->sceneBoundingRect();
                    QRectF &lr = rectByLayer[z];
                    lr = lr.isNull() ? r : lr.united(r);
                }
            }
            QRectF overallRect;
            for (auto it = rectByLayer.cbegin(); it != rectByLayer.cend(); ++it)
            {
                overallRect = overallRect.isNull() ? it.value() : overallRect.united(it.value());
            }
            if (!overallRect.isNull())
            {
                for (auto it = rectByLayer.cbegin(); it != rectByLayer.cend(); ++it)
                {
                    const QPointF delta = GradedAnchorPoint(overallRect, group.alignH, group.alignV)
                                          - GradedAnchorPoint(it.value(), group.alignH, group.alignV);
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
    }

    // 3) Re-apply the unit's stored per-range content offset (drag delta) — unchanged by the
    //    alignment edit, but the reset above removed it, so it is re-added on top of the new
    //    aligned position. Group members only move through the group offset (never a piece one).
    if (isGroup)
    {
        const auto content = range.groupContent.constFind(unitId);
        if (content != range.groupContent.cend())
        {
            const QPointF delta(content->dx, content->dy);
            for (auto *item : unitItems)
            {
                item->setPos(item->pos() + delta);
            }
        }
    }
    else
    {
        const auto content = range.pieceContent.constFind(unitId);
        if (content != range.pieceContent.cend())
        {
            const QPointF delta(content->dx, content->dy);
            for (auto *item : unitItems)
            {
                item->setPos(item->pos() + delta);
            }
        }
    }

    // 4) Refresh the reference drag baselines of the unit, so the next drag computes its delta
    //    from the new aligned positions (mirror of the global base refresh in ApplyGradedLayoutOffsets).
    for (auto *item : unitItems)
    {
        if (GradedIsReference(item))
        {
            m_gradedBasePositions.insert(GradedPieceId(item), item->pos());
        }
    }

    // 5) Group: re-anchor the invisible host (and its parented children) onto the new union of the
    //    reference members — a pure translation; the reference members moved as one, so the union
    //    box keeps its size and only its top-left follows them.
    if (isGroup)
    {
        QRectF refUnion;
        for (auto *item : allItems)
        {
            const quint32 pid = GradedPieceId(item);
            if (pid == NULL_ID || !memberIds.contains(pid) || !GradedIsReference(item))
            {
                continue;
            }
            const QRectF r = item->sceneBoundingRect();
            refUnion = refUnion.isNull() ? r : refUnion.united(r);
        }
        for (auto *item : allItems)
        {
            if (!item->data(kGradedHostGroupIdRole).isNull()
                && item->data(kGradedHostGroupIdRole).toUInt() == unitId)
            {
                item->setPos(refUnion.topLeft());
                break;
            }
        }
    }
}
