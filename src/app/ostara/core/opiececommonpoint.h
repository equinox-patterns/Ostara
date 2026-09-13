/************************************************************************
 **
 **  @file   opiececommonpoint.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/09/2026
 **
 **  @brief  Resolve a piece node to its construction source, and back.
 **
 **          Two pieces sharing a construction point each carry their own modeling copy of it, under
 **          a different node id. A group's common point must therefore be matched on the source id
 **          (idObject), never on the node id itself. These helpers are shared by the Details mode
 **          (where the common point is picked) and by the graded alignment (where it is anchored on).
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

#ifndef OPIECECOMMONPOINT_H
#define OPIECECOMMONPOINT_H

#include <QtGlobal>

class VContainer;
class VPiece;

/**
 * @brief Helpers around the common point shared by the members of a piece group.
 */
namespace OPieceCommonPoint
{
/// Construction source (idObject) of a node, or the node id itself when it has no distinct source.
auto SourceNodeId(quint32 nodeId, const VContainer *data) -> quint32;

/// True when the piece carries a node built from sourceId.
auto PieceHasNodeWithSource(const VPiece &piece, quint32 sourceId, const VContainer *data) -> bool;

/// The piece's own node id built from sourceId, or NULL_ID when the piece has no such node.
auto PieceNodeWithSource(const VPiece &piece, quint32 sourceId, const VContainer *data) -> quint32;
} // namespace OPieceCommonPoint

#endif // OPIECECOMMONPOINT_H
