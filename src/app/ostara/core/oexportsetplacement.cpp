/************************************************************************
 **
 **  @file   oexportsetplacement.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Undo command writing one placement of a nesting generation. Keeps
 **          both ends of a hand gesture and rewrites the store on undo and redo.
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

#include "oexportsetplacement.h"

#include "oexportoutput.h"

//---------------------------------------------------------------------------------------------------------------------
OExportSetPlacement::OExportSetPlacement(OExportOutputStore *store, quint32 outputId, int index,
                                         const OExportPlacement &from, const OExportPlacement &to,
                                         bool turned, QUndoCommand *parent)
  : QUndoCommand(parent),
    m_store(store),
    m_outputId(outputId),
    m_index(index),
    m_from(from),
    m_to(to)
{
    // The class is not a QObject — the nearest analogue, SaveGradedRangeContent, is not either —
    // so tr() has to be qualified. QUndoStack appends this to "Undo" and "Redo" in the menu.
    setText(turned ? QObject::tr("Turn a piece on the plan")
                   : QObject::tr("Move a piece on the plan"));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportSetPlacement::undo()
{
    Apply(m_from);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportSetPlacement::redo()
{
    // Also the first write: QUndoStack::push() calls redo() itself, so the caller hands the
    // command over and does not touch the store. The gesture's own write and the command's
    // would be the same write twice, and one of them would eventually be forgotten.
    Apply(m_to);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportSetPlacement::Apply(const OExportPlacement &placement)
{
    if (m_store == nullptr)
    {
        return;
    }

    m_store->SetPlacement(m_outputId, m_index, placement);
}
