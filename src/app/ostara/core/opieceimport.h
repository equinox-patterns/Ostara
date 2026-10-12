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

#include "../vmisc/def.h"

#include <QDomElement>
#include <QHash>
#include <QString>
#include <QVector>

class OPieceDocument;
class VAbstractPattern;
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

/**
 * @brief Whether @p piece may enter @p host at all, judging by the unit its lengths are written in.
 *
 * **The unit belongs to the file, not to the piece.** A pattern holds one unit from end to end, and a piece cut
 * in centimetres and read into a pattern in inches would give lengths that are wrong by a factor of two and a
 * half, silently and everywhere. Nothing is ever converted: the piece is **refused**, and told why.
 *
 * The unit of the piece is read as a **word of the schema** (`mm`, `cm`, `inch`), never through
 * `StrToUnits` — that one falls back to centimetres on anything it does not know, which would make an unreadable
 * unit pass for a piece in centimetres. A piece that says nothing, or says a unit a piece may not carry, is
 * refused for that reason rather than compared.
 *
 * @param piece the library piece about to be read; it is not modified.
 * @param host the pattern it would enter, asked for the unit of the **file** — and, when the file has none yet,
 *             for the unit the application is working in.
 * @param error written with what to tell the reader. Never a code, always a sentence: the one who reads it does
 *              not solve the problem, and has to know what to do instead.
 * @return true when the piece may enter. False when it is refused, @p error saying why.
 */
auto CheckUnit(const OPieceDocument &piece, const VAbstractPattern *host, QString &error) -> bool;

/**
 * @brief The unit a piece declares, read from the word alone.
 *
 * `Unit::LAST_UNIT_DO_NOT_USE` when the word is missing or names something a piece may not carry — read as a word
 * of the schema, never through `StrToUnits`, which answers centimetres to anything it does not know.
 *
 * The same reading `CheckUnit` refuses on, and it is public so that a list can be **filtered** by it: a dialog
 * that offers a piece of another unit would offer a piece the very next click refuses.
 */
auto PieceUnit(const QString &unit) -> Unit;

/**
 * @brief The unit @p host works in: the one its file declares, and failing that the one the work is in.
 *
 * Public for the same reason as @ref PieceUnit — the list of pieces to offer is built from it.
 */
auto HostUnit(const VAbstractPattern *host) -> Unit;

/**
 * @brief Whether @p host holds the pieces @p piece reads elsewhere.
 *
 * A piece that reads a value it does not declare needs another piece — that is what a dependency *is* — and a
 * pattern that holds none of them cannot give the value back: the piece would enter with formulas pointing at
 * nothing. The answer is a **refusal that names what is missing**, never a silent entry.
 *
 * A candidate is looked for in the order that survives renaming: the piece's `libraryId` first — it is what stays
 * true when a piece is renamed in the library — then its `category`, the name it wore, and finally the **name of
 * the block**, because a block drawn by hand carries neither identity nor category and refusing to see it would
 * be refusing the ordinary case.
 *
 * @param error written with what to tell the reader: what the piece reads, and what to do about it.
 * @return true when every value the piece reads is declared by some block of @p host.
 */
auto CheckDependencies(const OPieceDocument &piece, const VAbstractPattern *host, QString &error) -> bool;

/**
 * @brief Whether @p host holds the measurement table a piece is read against.
 *
 * A piece does not carry its measurements: the table is shared by every pattern of a file, and the formulas the
 * piece brings are evaluated *with* it. A pattern that holds no table therefore has nothing to read a piece with
 * — and the failure would come too late to be repaired: the block would already be in the document when the parse
 * refuses it, and the parse only logs "the formula uses a name the pattern does not define" before disabling the
 * editor. So the refusal is asked **before** anything is written, and it is the caller's to answer: the canvas is
 * where a measurement table is loaded.
 *
 * @param host the pattern the piece would enter, asked whether it holds a measurement table at all.
 * @param error written with what to tell the reader — the refusal, then what to do about it.
 * @return true when the pattern holds a measurement table.
 */
auto CheckMeasurements(const OPieceDocument &piece, const VAbstractPattern *host, QString &error) -> bool;

/**
 * @brief Writes the origin of @p piece on @p block, as the block enters the pattern.
 *
 * Two attributes, answering two different questions. `libraryId` is the identity of the library piece the block
 * comes from, **kept exactly as the piece carries it**: renaming a piece in the library renames its file, and a
 * block already imported has to go on knowing which piece it came from. `category` is the readable name of that
 * same origin — the name the piece was saved under — because the instance is about to take a name of its own
 * (`Dos`) and `category` is what keeps the origin readable afterwards: it is what will let the dialog offer only
 * the plausible candidates.
 *
 * @param block the block about to enter, **before** it is renamed — the origin's name is read from @p piece while
 *              the block still carries it, and `category` is what remembers it.
 * @param piece the library piece the block was read from; it is not modified.
 */
void SetOrigin(QDomElement &block, const OPieceDocument &piece);
/**
 * @brief A name the pattern already gives to a line, and the line it gives it to.
 *
 * A role is remembered per **pair of points**, and its name has to be unique — the pattern refuses a second
 * line under a name already taken, and drops the entry when it reads the file back. So a piece can meet two
 * walls: the pattern may already name that same pair, and it may already answer to that same name.
 */
struct LineNameClash
{
    QString points{};      ///< The pair, as the piece writes it.
    QString name{};        ///< The name the piece gives that line.
    bool nameTaken{false}; ///< True when the name belongs to **another** line; what is kept is then in @p held.
    QString held{};        ///< What the pattern keeps — its name for that pair, or the pair owning that name.
};

struct LineNameReport
{
    int added{0};
    int kept{0};
    QVector<LineNameClash> clashes{};
};

/**
 * @brief Writes the names @p piece gives to its lines into the pattern, their pair of points renamed.
 *
 * The lines' names live at the **root** of a pattern — outside every block — so they do not travel with the
 * block and have to be carried and put back deliberately. They go into @p data and not into the DOM: the
 * section is **rebuilt from the container at every save** (`VPattern::SyncLineNamesToDom`), so a row written
 * by hand would be swept away by the next save.
 *
 * The pair of a row is renamed by @p renamed, which is the same table the import uses for the piece's names —
 * empty when nothing collides, in which case a row travels as it stands. @p renamed also applies to the role
 * itself: it is a name of use, and it becomes a token of a formula (`AngleLine_Angle180`).
 *
 * The **first import wins**, as for the increments: a pair the pattern already names is left as it is, and the
 * reader is told when the piece would have said something else.
 */
auto ImportLineNames(const OPieceDocument &piece, VContainer *data, const QHash<QString, QString> &renamed)
    -> LineNameReport;
} // namespace OPieceImport

#endif // OPIECEIMPORT_H
