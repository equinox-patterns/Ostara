/************************************************************************
 **
 **  @file   opiecegrouplayoutflags.h
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

#ifndef OPIECEGROUPLAYOUTFLAGS_H
#define OPIECEGROUPLAYOUTFLAGS_H

#include <QVector>
#include <QtGlobal>

class VAbstractPattern;
class VContainer;

/**
 * @brief Helpers around the layout flags a piece group shows.
 *
 * A group carries no layout flag of its own — the flags are its members'. Checking a flag for a group means
 * checking it for every member, so that the user does not have to unlink the group or tick each member in the tree
 * one by one. These two readings and this one write are the whole of that rule, here rather than in a widget,
 * because writing the same value into K pieces has to be ONE undo step and not K.
 */
namespace OPieceGroupLayoutFlags
{
/// @brief The members of a group that still exist, in the order the group lists them.
///
/// A group's `pieceIds` is not cleaned when a piece is deleted, and `VContainer::GetPiece()` THROWS
/// on an unknown id rather than returning empty. Every reader of a group has to filter first, so the
/// filter lives here, once.
auto ExistingMemberIds(const QVector<quint32> &pieceIds, const VContainer *data) -> QVector<quint32>;

/// True when EVERY member carries the flag. Members that no longer exist are ignored; no member means false.
auto ForceFlipping(const QVector<quint32> &pieceIds, const VContainer *data) -> bool;
auto SymmetricalCopy(const QVector<quint32> &pieceIds, const VContainer *data) -> bool;

/// Write both flags into every member. Members already carrying the values are left untouched, and the whole write
/// is a single undo step.
void Apply(const QVector<quint32> &pieceIds, bool forceFlipping, bool symmetricalCopy, VContainer *data,
           VAbstractPattern *doc);
} // namespace OPieceGroupLayoutFlags

#endif // OPIECEGROUPLAYOUTFLAGS_H
