/************************************************************************
 **
 **  @file   opatternversions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  The pattern's versions: what they are called, and who wears which.
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

#ifndef OPATTERN_VERSIONS_H
#define OPATTERN_VERSIONS_H

#include <QList>
#include <QString>
#include <QStringList>

class IPieceInfo;
class VAbstractPattern;
class VContainer;

/**
 * @brief What a pattern's versions are called, and who wears which.
 *
 * The document holds the list (`<patternVersions>`), a piece holds one id, and a group holds none: it
 * shows the versions of its members. These questions are asked from the details dock, from the
 * information block and from the version dialog, and they must answer the same way everywhere — hence
 * one place, not three.
 */
namespace OPatternVersions
{
/// @brief The name of @p id, falling back to the "all versions" entry for an id the document no
/// longer knows (a version deleted by hand, or a file written by another reader).
auto Name(const VAbstractPattern &doc, quint32 id) -> QString;

/// @brief The ids @p object wears: its own for a piece (0 reads as the "all versions" entry), the ids
/// of its members for a group — without repetition.
auto Ids(const VAbstractPattern &doc, const VContainer *data, const IPieceInfo &object) -> QList<quint32>;

/// @brief @p ids written out, in the order the pattern lists its versions, without repetition.
auto Names(const VAbstractPattern &doc, const QList<quint32> &ids) -> QStringList;

/// @brief "All versions + Version A" — how several versions are shown on one line.
auto Join(const QStringList &names) -> QString;

/// @brief The pieces of @p doc that wear @p versionId.
///
/// Read from the document itself, not from a live scene: this is what the version dialog counts before
/// it deletes a version or turns the mode off, and it must count the pieces the file holds.
auto PieceIdsUsing(const VAbstractPattern &doc, quint32 versionId) -> QList<quint32>;
} // namespace OPatternVersions

#endif // OPATTERN_VERSIONS_H
