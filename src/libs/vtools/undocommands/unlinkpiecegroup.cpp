/************************************************************************
 **
 **  @file   unlinkpiecegroup.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Undo command that unlinks a piece group (restores members).
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

#include "unlinkpiecegroup.h"

#include <QDomElement>

UnlinkPieceGroup::UnlinkPieceGroup(quint32 groupId, VAbstractPattern *doc, VContainer *data,
                                   QUndoCommand *parent)
  : VUndoCommand(doc, parent), m_data(data), m_groupId(groupId)
{
    setText(tr("unlink pieces"));
    SetElementId(groupId);

    // Snapshot the group before it is removed so undo can restore it verbatim.
    const auto *groups = data->DataPieceGroups();
    if (groups && groups->contains(groupId))
    {
        m_group = groups->value(groupId);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void UnlinkPieceGroup::undo()
{
    if (m_groupId == NULL_ID)
    {
        return;
    }

    m_group.id = m_groupId;
    m_data->AddPieceGroup(m_group);
    QDomElement rootElement = Doc()->documentElement();
    VToolSeamAllowance::AddPieceGroups(Doc(), rootElement, m_data);

    // Re-mask the members' own children (the group is back).
    for (quint32 pid : m_group.pieceIds)
    {
        try
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
            if (tool != nullptr)
            {
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
void UnlinkPieceGroup::redo()
{
    if (m_groupId == NULL_ID)
    {
        return;
    }

    m_data->RemovePieceGroup(m_groupId);
    QDomElement rootElement = Doc()->documentElement();
    VToolSeamAllowance::AddPieceGroups(Doc(), rootElement, m_data);
    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
auto UnlinkPieceGroup::id() const -> int
{
    return static_cast<int>(UndoCommand::UnlinkPieceGroup);
}
