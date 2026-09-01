/************************************************************************
 **
 **  @file   savepiecegroupoptions.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Undo command for editing a piece group's options.
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

#include "savepiecegroupoptions.h"

#include <QDomElement>

SavePieceGroupOptions::SavePieceGroupOptions(OPieceGroup oldGroup, OPieceGroup newGroup, VAbstractPattern *doc,
                                             VContainer *data, quint32 groupId, QUndoCommand *parent)
  : VUndoCommand(doc, parent)
  , m_oldGroup(std::move(oldGroup))
  , m_newGroup(std::move(newGroup))
  , m_data(data)
  , m_groupId(groupId)
{
    setText(tr("save group options"));
    SetElementId(groupId);
}

//---------------------------------------------------------------------------------------------------------------------
void SavePieceGroupOptions::undo()
{
    if (m_groupId == NULL_ID || !m_data->DataPieceGroups()->contains(m_groupId))
    {
        return;
    }

    m_data->UpdatePieceGroup(m_groupId, m_oldGroup);
    QDomElement rootElement = Doc()->documentElement();
    VToolSeamAllowance::AddPieceGroups(Doc(), rootElement, m_data);
    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
void SavePieceGroupOptions::redo()
{
    if (m_groupId == NULL_ID || !m_data->DataPieceGroups()->contains(m_groupId))
    {
        return;
    }

    m_data->UpdatePieceGroup(m_groupId, m_newGroup);
    QDomElement rootElement = Doc()->documentElement();
    VToolSeamAllowance::AddPieceGroups(Doc(), rootElement, m_data);
    emit UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
auto SavePieceGroupOptions::mergeWith(const QUndoCommand *command) -> bool
{
    const auto *c = dynamic_cast<const SavePieceGroupOptions *>(command);
    if (c == nullptr || c->m_groupId != m_groupId)
    {
        return false;
    }

    m_newGroup = c->m_newGroup;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto SavePieceGroupOptions::id() const -> int
{
    return static_cast<int>(UndoCommand::SavePieceGroupOptions);
}
