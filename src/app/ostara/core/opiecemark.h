/************************************************************************
 **
 **  @file   opiecemark.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  The `__P<n>` a piece's names carry when they would have taken the place of names the pattern
 **          already holds: which names it touches, and what they become.
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

#ifndef OPIECEMARK_H
#define OPIECEMARK_H

#include <QDomElement>
#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

/**
 * @brief The names a piece declares, and the mark that keeps them apart from the ones the pattern already holds.
 *
 * Two blocks cut from two patterns can agree on a name — `Em`, `Pro`, `Line_Em_Pro` — and a container holds one
 * object per name: the second piece would take the place of the first, silently. So a piece entering a pattern
 * whose names are already there is **marked**, and the mark is a suffix on **every** name it declares:
 * `Em__P1`. All of them or none: a piece half marked would have two namespaces and no coherence.
 *
 * `__P<n>` is a **mark, not a name**. It says which file the piece entered, so it is written when the piece
 * enters a file and **removed when the piece is saved back to the library** — a `.opiece` never carries one,
 * and the same piece imported twice into two files is marked from scratch each time. The number is drawn from
 * the file (`HighestNumber`, plus one), never from a counter of the session: once written, it never moves.
 *
 * The names are the ones the schema and the code agree on, and nothing else — a layer's label, a dialog's item,
 * a piece's own name are not names any object answers to. Because a formula cites an object by a **token built
 * on its name** (`Line_Em_Pro`, `AngleLine_Angle180`, `SplPath_devant`, `A14LSubCurve__c1`), renaming a name
 * means rewriting those tokens too, and this is one operation, never two.
 *
 * The caller owes the order: the block is marked **before** it is inserted, and the mark is removed **after**
 * the piece's own line roles have been matched on its names — a role is matched by the pair of points it joins,
 * and those points are marked at that moment.
 */
namespace OPieceMark
{
/// The number a name's mark carries — the **last** one, since a name can carry several — and 0 when the name
/// carries none. A name that merely ends in `__P` and digits is read as marked: the mark has no other shape.
auto Number(const QString &name) -> int;

/// @p name with every mark removed: `Em__P1` is `Em`, and a name that carries none is itself.
auto Unmarked(const QString &name) -> QString;

/// @p name, marked: `Em` becomes `Em__P1`. A number below 1 marks nothing.
auto Marked(const QString &name, int number) -> QString;

/**
 * @brief The slots that declare one name, as `parent/element/@attribute` — read from the pattern schema.
 *
 * The **parent** is part of the slot because the same element name does not mean the same thing everywhere:
 * `item` under `source` is a point the operation places and is named; `item` under `rotations` is a line of a
 * dialog, which no object ever answers to. A slot the schema types `nameSuffix` is an alias — the whole family,
 * read rather than written by hand — and what it types otherwise is written here, because only the code knows
 * that a point's own name (`shortName`) and the technical name of a spline (`string`) are names of objects too.
 */
auto NameSlots() -> const QSet<QString> &;

/// The slots that declare **several** names in one attribute, space separated: the pair of points a line's role
/// joins. The one slot of the schema that is not a name of its own.
auto NameListSlots() -> const QSet<QString> &;

/// Every name @p root declares, in the order they are met, without repetition.
auto DeclaredNames(const QDomElement &root) -> QStringList;

/// The highest mark @p root carries, 0 when it carries none. The file's own answer to "which number next".
auto HighestNumber(const QDomElement &root) -> int;

/// True when the pattern the piece is entering already answers to one of the names @p root declares — the name
/// itself, or the same name under one of the heads the container builds on it (`Spl_devant` for the alias
/// `devant`, the area of a piece for a `<detail>`). Wider than "holds that name", because an alias of the pattern
/// is overwritten in silence by an alias of the piece, and a name is not what the pattern answers to.
auto Collides(const QDomElement &root, const QSet<QString> &taken) -> bool;

/// What a rename did: the names of the table that were really met, and the places rewritten.
struct Report
{
    int names = 0;   ///< Distinct names of the table the piece declares.
    int written = 0; ///< Name slots rewritten, one count per value.
};

/**
 * @brief The table that marks @p root with @p number: every name it declares, and the name it takes.
 *
 * **All or nothing, per piece.** The table is empty when @p root declares none of the names in @p taken: a
 * piece that collides with nothing keeps its names exactly as they are, and part of a fresh file it is the
 * first `Buste` that arrives as `Em`, never as `Em__P1`. It never marks a name per name.
 */
auto TableFor(const QDomElement &root, const QSet<QString> &taken, int number) -> QHash<QString, QString>;

/**
 * @brief Applies @p renamed to every name @p root declares, **and to the formulas that cite them**.
 *
 * A name is cited by tokens built on it, and the token is what the calculator reads: a name renamed without its
 * tokens leaves the piece pointing at objects that no longer exist. The two passes are therefore one, and the
 * same call serves both directions — the mark a piece takes when it enters a file, and the mark it loses when
 * it is saved.
 */
auto Rename(QDomElement &root, const QHash<QString, QString> &renamed) -> Report;

/// Removes the mark of @p root: the names, and the formulas that cite them.
auto Unmark(QDomElement &root) -> Report;
} // namespace OPieceMark

#endif // OPIECEMARK_H
