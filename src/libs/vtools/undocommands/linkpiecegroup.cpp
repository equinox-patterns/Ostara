/************************************************************************
 **
 **  @file   linkpiecegroup.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Undo command that links pieces into a piece group.
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

#include "linkpiecegroup.h"

#include <QDomElement>

#include "../ifc/ifcdef.h"
#include "../vmisc/vabstractvalapplication.h"

LinkPieceGroup::LinkPieceGroup(const OPieceGroup &group, VAbstractPattern *doc, VContainer *data,
                               QUndoCommand *parent)
  : VUndoCommand(doc, parent), m_group(group), m_data(data)
{
    setText(tr("link pieces"));
}

//---------------------------------------------------------------------------------------------------------------------
void LinkPieceGroup::undo()
{
    if (m_groupId == NULL_ID)
    {
        return;
    }

    m_data->RemovePieceGroup(m_groupId);
    QDomElement rootElement = Doc()->documentElement();
    VToolSeamAllowance::AddPieceGroups(Doc(), rootElement, m_data);

    // Back where they were: redo moved each member out of the stack into the workshop, so undo puts
    // it back at the very place the stack still claims — the group is gone, the pieces are whole.
    for (quint32 pid : m_group.pieceIds)
    {
        try
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
            if (tool != nullptr)
            {
                const QPointF original = m_preLinkPositions.value(pid, tool->pos());
                QDomElement domElement = Doc()->FindElementById(pid, VAbstractPattern::TagDetail);
                if (domElement.isElement())
                {
                    Doc()->SetAttribute(domElement, AttrMx,
                                        QString().setNum(VAbstractValApplication::VApp()->fromPixel(original.x())));
                    Doc()->SetAttribute(domElement, AttrMy,
                                        QString().setNum(VAbstractValApplication::VApp()->fromPixel(original.y())));
                }
                tool->Move(original.x(), original.y());
                tool->RefreshGroupedChildren();
            }
        }
        catch (const VExceptionBadId &)
        {
            continue;
        }
    }
    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
void LinkPieceGroup::redo()
{
    // Assign a stable id on the first redo so undo/redo keep the same group id.
    if (m_groupId == NULL_ID)
    {
        m_groupId = m_data->getNextId();
        m_data->UpdateId(m_groupId);
        m_group.id = m_groupId;
        SetElementId(m_groupId);
    }

    m_preLinkPositions.clear();

    // **The stack is where the members stand now.** Each member's stack position is the place it
    // occupies once the common point coincides — its own position, moved by the offset the linking
    // dialog worked out. Recorded before anything moves, so the group shows and exports exactly the
    // arrangement the user built.
    QRectF stackBounds;
    for (quint32 pid : m_group.pieceIds)
    {
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
        if (tool == nullptr)
        {
            continue;
        }
        m_preLinkPositions.insert(pid, tool->pos());
        const QPointF offset = m_group.memberOffsets.value(pid, QPointF());
        const QPointF stackPos = tool->pos() + offset;
        m_group.SetMemberStackPosition(pid, stackPos);
        stackBounds = stackBounds.united(tool->sceneBoundingRect().translated(offset));
    }

    // **The group's fold line comes from a member.** When exactly one member already has one there
    // is nothing to choose, so the group takes it right away; with several, the choice belongs to
    // the group page. The group never defines a fold line of its own.
    if (m_group.m_foldLineMemberId == NULL_ID)
    {
        quint32 candidate = NULL_ID;
        int candidates = 0;
        for (quint32 pid : m_group.pieceIds)
        {
            try
            {
                if (m_data->GetPiece(pid).GetFoldLineType() != FoldLineType::None)
                {
                    candidate = pid;
                    ++candidates;
                }
            }
            catch (const VExceptionBadId &)
            {
                continue;
            }
        }
        if (candidates == 1)
        {
            m_group.m_foldLineMemberId = candidate;
        }
    }

    // **A member that already shows a miniature brings it along.** The group copies it here, once —
    // switches, values and where they were placed — and from then on the two live their own lives:
    // nothing is linked, so editing the piece later never moves the group's copy, nor the reverse.
    for (quint32 pid : m_group.pieceIds)
    {
        try
        {
            const OThumbnailData memberMiniature = m_data->GetPiece(pid).GetThumbnailData();
            if (memberMiniature.IsActive() && not m_group.MemberThumbnail(pid).IsActive())
            {
                m_group.SetMemberThumbnail(pid, memberMiniature);
            }
        }
        catch (const VExceptionBadId &)
        {
            continue;
        }
    }

    m_data->AddPieceGroup(m_group);

    // **The members leave the stack for the workshop.** A member stays a real piece — its own
    // points, its margins, its notches, its children — and that is what makes them editable: a
    // drawing cannot carry a point. The group keeps the stack, drawn from the positions above, and
    // it keeps its own children, so a group still counts, exports and prints as one piece.
    const qreal gap = VAbstractValApplication::VApp()->toPixel(10.0); // 10 cm between the members
    QPointF nextWorkshop(stackBounds.right() + gap, stackBounds.top());
    for (quint32 pid : m_group.pieceIds)
    {
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
        if (tool == nullptr)
        {
            continue;
        }
        const QRectF live = tool->sceneBoundingRect();
        const QPointF target = tool->pos() + (nextWorkshop - live.topLeft());
        nextWorkshop.rx() += live.width() + gap;

        QDomElement domElement = Doc()->FindElementById(pid, VAbstractPattern::TagDetail);
        if (domElement.isElement())
        {
            Doc()->SetAttribute(domElement, AttrMx,
                                QString().setNum(VAbstractValApplication::VApp()->fromPixel(target.x())));
            Doc()->SetAttribute(domElement, AttrMy,
                                QString().setNum(VAbstractValApplication::VApp()->fromPixel(target.y())));
        }
        tool->Move(target.x(), target.y());
    }

    QDomElement rootElement = Doc()->documentElement();
    VToolSeamAllowance::AddPieceGroups(Doc(), rootElement, m_data);
    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
auto LinkPieceGroup::id() const -> int
{
    return static_cast<int>(UndoCommand::LinkPieceGroup);
}
