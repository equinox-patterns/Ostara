/************************************************************************
 **
 **  @file   opiececommonpoint.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/09/2026
 **
 **  @brief  Resolve a piece node to its construction source, and back.
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

#include "opiececommonpoint.h"

#include "../ifc/exception/vexceptionbadid.h"
#include "../vgeometry/vgobject.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"

//---------------------------------------------------------------------------------------------------------------------
auto OPieceCommonPoint::SourceNodeId(quint32 nodeId, const VContainer *data) -> quint32
{
    if (nodeId == NULL_ID || data == nullptr)
    {
        return nodeId;
    }
    try
    {
        const QSharedPointer<VGObject> gobj = data->GetGObject(nodeId);
        const quint32 source = gobj->getIdObject();
        return source == NULL_ID ? nodeId : source;
    }
    catch (const VExceptionBadId &)
    {
        return nodeId;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceCommonPoint::PieceHasNodeWithSource(const VPiece &piece, quint32 sourceId,
                                               const VContainer *data) -> bool
{
    const QVector<VPieceNode> nodes = piece.GetPath().GetNodes();
    for (const VPieceNode &node : nodes)
    {
        if (SourceNodeId(node.GetId(), data) == sourceId)
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceCommonPoint::PieceNodeWithSource(const VPiece &piece, quint32 sourceId,
                                            const VContainer *data) -> quint32
{
    const QVector<VPieceNode> nodes = piece.GetPath().GetNodes();
    for (const VPieceNode &node : nodes)
    {
        if (SourceNodeId(node.GetId(), data) == sourceId)
        {
            return node.GetId();
        }
    }
    return NULL_ID;
}
