/************************************************************************
 **
 **  @file   oseamallowancelibrary.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  The names a pattern's margins are written with on a cover
 **          page, and what tells one of them from another.
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

#ifndef OSEAMALLOWANCELIBRARY_H
#define OSEAMALLOWANCELIBRARY_H

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

/**
 * @brief The names a page gives the margins a pattern actually carries.
 *
 * **A pattern carries widths; it does not carry names.** The seam allowances of a piece are numbers
 * measured off its outline, and what a width *means* — a plain seam, a French seam, a hem — is
 * something the person reading the pattern knows and the file does not say. So the page is told
 * here, once, and every pattern that uses the template is read with the same vocabulary.
 *
 * **An entry is recognised by its width, not by where it sits on the piece.** A hem is
 * "everything from two centimetres upward" whatever edge it runs along; a French seam is
 * "exactly a centimetre and a half". The pattern is scanned for the widths it holds, and each one
 * is given the name whose condition it answers — which is why a user who gives two different
 * widths the same rule gets two lines saying the same thing, and why that is a mistake in the
 * library rather than something the page can repair.
 *
 * **The default margin is the one everybody has.** A pattern whose pieces all keep the width the
 * application was set to has exactly one margin to declare, and naming it is a condition nobody
 * should have to write — so an entry may be marked as *the* default instead of carrying one, and
 * its name follows the preference rather than a number frozen into a file.
 *
 * **Nothing here is a file either.** Like the symbol library, this is an index: a name per
 * language, a condition, and nothing that cannot be rewritten by hand. It is deliberately **not**
 * validated against a schema — a malformed one costs the library, not the installation.
 */
namespace OSeamAllowanceLibrary
{
/// @brief How the width of a margin is compared with the width an entry declares.
enum class Comparison
{
    Equal,  ///< the margin is exactly this wide — a French seam
    AtLeast ///< the margin is this wide or more — a hem
};

/**
 * @brief What makes a width this entry's.
 *
 * **Millimetres, always.** The library is shared by every pattern, and a pattern's unit is its own:
 * a condition kept in the unit of the pattern it was written for would change meaning the day it
 * was read beside another one. The page converts, and the file stays in one unit.
 */
struct Condition
{
    Comparison comparison = Comparison::Equal;
    qreal mm = 0.0;
};

/** @brief One margin: what it is called in each language, and how it is recognised. */
struct Entry
{
    QMap<QString, QString> texts; ///< one name per locale, keyed by the locale's code

    /**
     * @brief Whether this is the margin a pattern keeps when nobody has changed anything.
     *
     * Such an entry carries no condition: what it is worth is whatever the application's default
     * seam allowance is set to, which is a preference and not a number this file may hold. A
     * library has at most one of them.
     */
    bool byDefault = false;
    Condition condition{};
};

/** @brief The library: the languages it is written in, and the margins in reading order. */
struct Library
{
    QStringList languages; ///< locale codes, in display order; never empty
    QList<Entry> entries;  ///< in the order the user put them
};

/// @brief The hundredths of a millimetre a comparison is made on.
///
/// **A width arrives from arithmetic, not from a keyboard.** It has been through the pattern's unit
/// and back into millimetres, so "one and a half centimetres" comes out as something that is only
/// nearly fifteen. Comparing the two as they fall would have an entry match on one pattern and miss
/// on the next, so both sides are brought to the same step first — a hundredth of a millimetre,
/// which is far finer than any seam is cut.
inline constexpr qreal kComparisonSteps = 100.0;

/// @brief @p mm counted in those steps, as a whole number — what comparisons are actually made on.
auto Steps(qreal mm) -> qint64;

/// @brief Whether @p mm is the margin @p entry names, both brought to @ref kComparisonSteps.
auto Matches(const Entry &entry, qreal mm) -> bool;

/// @brief @p mm brought to @ref kComparisonSteps, for comparing widths with one another.
auto Rounded(qreal mm) -> qreal;

/** @brief The entry marked as the default, or -1 when the library holds none. */
auto DefaultIndex(const Library &library) -> int;

/** @brief The folder the library is kept in, from settings (GetPathSeamAllowances). */
auto Directory() -> QString;

/** @brief The path of the index file, which is the whole library. */
auto MetaPath() -> QString;

/**
 * @brief The library, seeding it on first use.
 *
 * **The default margin is written once, for the same reason the grain line is.** Every pattern has
 * one, and a library that had to be built before a page could say anything would ask for work
 * nobody has to do. It is written in every language the application knows, in the application's own
 * words, and never written again: an entry the user deletes does not come back, and one they rename
 * keeps the name they gave it.
 */
auto Load() -> Library;

/** @brief Writes the library, atomically, creating its folder if it has gone. */
auto Save(const Library &library, QString *error = nullptr) -> bool;
} // namespace OSeamAllowanceLibrary

#endif // OSEAMALLOWANCELIBRARY_H
