/************************************************************************
 **
 **  @file   ogradedsizeranges.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/09/2026
 **
 **  @brief  The (measurements table, declared grading axis) context that decides
 **          which of a pattern's stored size ranges are usable — the rule the Graded
 **          dock lists by, and the one every other enumeration of the ranges follows.
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

#ifndef OGRADEDSIZERANGES_H
#define OGRADEDSIZERANGES_H

#include <QString>

class OSizerange;
class VAbstractPattern;

namespace OGradedSizeRanges
{
/**
 * @brief The (measurements table, declared grading axis) pair a range has to match.
 *
 * Both halves matter, and each one is lifted when it is empty — an empty canonical
 * constrains nothing, an empty axis keeps every axis of the table. That is not a detail:
 * a table outside the library never persists its axis, so the empty axis is the normal
 * state there, and reading it as "match the empty axis" would empty the list.
 */
struct Context
{
    QString canonical;    ///< absolute path of the measurements table
    QString axis;         ///< declared grading axis; empty → every axis of the table is kept
    bool inLibrary{false}; ///< how it was resolved: outside the library the axis is never declared
};

/// @brief Resolve the context of a measurements table, by its absolute path.
auto ContextFor(const QString &canonicalPath) -> Context;

/// @brief Resolve the context of a pattern. An empty context constrains nothing.
auto CurrentContext(VAbstractPattern *doc) -> Context;

/**
 * @brief Whether a range belongs to a context.
 *
 * The rule the dock has always listed by. Everything that enumerates the stored ranges —
 * the dock, the batch export, the range list of the Export mode, the range activated
 * after a deletion — has to ask it, or it will offer what the dock hides: a range built
 * against another table, or against another axis of the same table, is not what the user
 * is working on.
 */
auto Matches(const OSizerange &range, const Context &context) -> bool;
}  // namespace OGradedSizeRanges

#endif  // OGRADEDSIZERANGES_H
