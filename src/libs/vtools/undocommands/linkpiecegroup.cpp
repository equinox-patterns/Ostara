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

    // Restore the members' original position (Mx/My) and their own children.
    for (quint32 pid : m_group.pieceIds)
    {
        try
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
            if (tool != nullptr)
            {
                const VPiece piece = m_data->GetPiece(pid);
                tool->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
                tool->setPos(QPointF(piece.GetMx(), piece.GetMy()));
                tool->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
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

    m_data->AddPieceGroup(m_group);

    // Val Fork alignment (vwidgetdetails.cpp:1234-1238): superimpose the common point by
    // translating each member whose node is not already at the reference position. The raw
    // setPos keeps Mx/My untouched — persistence goes through memberOffsets +
    // ApplyGroupNodeAlignments on load.
    for (quint32 pid : m_group.pieceIds)
    {
        const QPointF offset = m_group.memberOffsets.value(pid, QPointF());
        if (qFuzzyIsNull(offset.x()) && qFuzzyIsNull(offset.y()))
        {
            continue;
        }
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
        if (tool == nullptr)
        {
            continue;
        }
        tool->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
        tool->setPos(tool->pos() + offset);
        tool->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
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
