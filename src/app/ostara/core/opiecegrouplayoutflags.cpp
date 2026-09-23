/************************************************************************
 **
 **  @file   opiecegrouplayoutflags.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Details mode — the layout flags a group shares with its members.
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

#include "opiecegrouplayoutflags.h"

#include <QCoreApplication>
#include <QUndoStack>

#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vtools/undocommands/savepieceoptions.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// The members of the group that still exist, in the order the group lists them.
auto ExistingMembers(const QVector<quint32> &pieceIds, const VContainer *data) -> QVector<VPiece>
{
    QVector<VPiece> members;
    if (data == nullptr || data->DataPieces() == nullptr)
    {
        return members;
    }

    members.reserve(pieceIds.size());
    for (quint32 pieceId : pieceIds)
    {
        if (data->DataPieces()->contains(pieceId))
        {
            members.append(data->GetPiece(pieceId));
        }
    }
    return members;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPieceGroupLayoutFlags::ForceFlipping(const QVector<quint32> &pieceIds, const VContainer *data) -> bool
{
    const QVector<VPiece> members = ExistingMembers(pieceIds, data);
    if (members.isEmpty())
    {
        return false;
    }

    for (const auto &member : members)
    {
        if (not member.IsForceFlipping())
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceGroupLayoutFlags::SymmetricalCopy(const QVector<quint32> &pieceIds, const VContainer *data) -> bool
{
    const QVector<VPiece> members = ExistingMembers(pieceIds, data);
    if (members.isEmpty())
    {
        return false;
    }

    for (const auto &member : members)
    {
        if (not member.IsSymmetricalCopy())
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupLayoutFlags::Apply(const QVector<quint32> &pieceIds, bool forceFlipping, bool symmetricalCopy,
                                   VContainer *data, VAbstractPattern *doc)
{
    if (data == nullptr || doc == nullptr || data->DataPieces() == nullptr)
    {
        return;
    }

    QVector<quint32> ids;
    QVector<VPiece> oldPieces;
    QVector<VPiece> newPieces;
    ids.reserve(pieceIds.size());
    oldPieces.reserve(pieceIds.size());
    newPieces.reserve(pieceIds.size());

    for (quint32 pieceId : pieceIds)
    {
        if (not data->DataPieces()->contains(pieceId))
        {
            continue;
        }

        const VPiece oldDet = data->GetPiece(pieceId);
        if (oldDet.IsForceFlipping() == forceFlipping && oldDet.IsSymmetricalCopy() == symmetricalCopy)
        {
            continue; // the member already carries the values — no undo step for it
        }

        VPiece newDet = oldDet;
        newDet.SetForceFlipping(forceFlipping);
        newDet.SetSymmetricalCopy(symmetricalCopy);

        ids.append(pieceId);
        oldPieces.append(oldDet);
        newPieces.append(newDet);
    }

    if (ids.isEmpty())
    {
        return;
    }

    QUndoStack *stack = VAbstractApplication::VApp()->getUndoStack();
    if (ids.size() == 1)
    {
        // A macro of one command collapses to that command anyway; skip the wrapper.
        stack->push(new SavePieceOptions(oldPieces.at(0), newPieces.at(0), doc, ids.at(0)));
        return;
    }

    stack->beginMacro(QCoreApplication::translate("OPieceGroupLayoutFlags", "Layout flags of the group"));
    for (int i = 0; i < ids.size(); ++i)
    {
        stack->push(new SavePieceOptions(oldPieces.at(i), newPieces.at(i), doc, ids.at(i)));
    }
    stack->endMacro();
}
