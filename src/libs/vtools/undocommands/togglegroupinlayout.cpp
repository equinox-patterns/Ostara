/************************************************************************
 **
 **  @file   togglegroupinlayout.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   02/09/2026
 **
 **  @brief  Undo command that toggles a piece group's in-layout (check/x) state.
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

#include "togglegroupinlayout.h"

#include <QDomElement>
#include <QMessageLogger>
#include <QtDebug>

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "vundocommand.h"

//---------------------------------------------------------------------------------------------------------------------
ToggleGroupInLayout::ToggleGroupInLayout(
    quint32 groupId, bool state, VContainer *data, VAbstractPattern *doc, QUndoCommand *parent)
  : VUndoCommand(doc, parent),
    m_groupId(groupId),
    m_data(data),
    m_oldState(not state),
    m_newState(state)
{
    setText(tr("group in layout list"));
    m_oldState = m_data->DataPieceGroups()->value(m_groupId).IsInLayout();
}

//---------------------------------------------------------------------------------------------------------------------
void ToggleGroupInLayout::undo()
{
    qCDebug(vUndo, "ToggleGroupInLayout::undo().");
    Do(m_oldState);
}

//---------------------------------------------------------------------------------------------------------------------
void ToggleGroupInLayout::redo()
{
    qCDebug(vUndo, "ToggleGroupInLayout::redo().");
    Do(m_newState);
}

//---------------------------------------------------------------------------------------------------------------------
void ToggleGroupInLayout::Do(bool state)
{
    QDomElement groupElement = Doc()->FindElementById(m_groupId, VAbstractPattern::TagPieceGroup);
    if (groupElement.isElement())
    {
        Doc()->SetAttributeOrRemoveIf<bool>(groupElement, AttrInLayout, state, [](bool state) noexcept { return state; });

        OPieceGroup group = m_data->DataPieceGroups()->value(m_groupId);
        group.SetInLayout(state);
        m_data->UpdatePieceGroup(m_groupId, group);
        emit Toggled(m_groupId);
    }
    else
    {
        qDebug("Can't get pieceGroup by id = %u.", m_groupId);
    }
}
