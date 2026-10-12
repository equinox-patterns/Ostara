/************************************************************************
 **
 **  @file   opieceimport.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  Bringing a piece of the library into a pattern: what it carries, and what the pattern already has.
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

#ifndef OPIECEIMPORT_H
#define OPIECEIMPORT_H

#include <QDomElement>
#include <QString>
#include <QVector>

class OPieceDocument;
class VContainer;
class VPattern;

/**
 * @brief The increments of a piece entering a pattern.
 *
 * The rule of the subject is **the first import wins**. An increment is like a measurement: one definition, shared.
 * A row the pattern already holds is left exactly as it is, and when the piece's copy would have said something
 * else the reader is **told**, never asked — the definition already there is the one that stays.
 *
 * This is also the only thing that ever **creates a group of increments**: one per table, named after the instance,
 * and only where the piece really brought rows — a piece that uses increments in preliminary calculations alone
 * creates no group in the table of increments.
 */
namespace OPieceImport
{
/// An increment the pattern already holds and keeps. @p differentDefinition says the piece's copy would have said
/// something else: nothing is decided, the reader is told and judges.
struct IncrementClash
{
    QString name{};
    bool previewCalculation{false};
    bool differentDefinition{false};
};

struct IncrementReport
{
    int added{0};
    int kept{0};
    int groupsAdded{0};
    QVector<IncrementClash> clashes{};
};

/**
 * @brief Write the increments @p piece carries into @p host, each table under one group named after @p instanceName.
 *
 * Read from the piece, written in the pattern, and in the pattern's own tables only: the block itself is inserted
 * elsewhere, and the caller reparses once everything is in place — the container is not touched here. @p data is the
 * pattern's container, asked for the one thing a container owns: the counter a new group's id comes from.
 */
auto ImportIncrements(const OPieceDocument &piece, VPattern *host, VContainer *data, const QString &instanceName)
    -> IncrementReport;
} // namespace OPieceImport

#endif // OPIECEIMPORT_H
