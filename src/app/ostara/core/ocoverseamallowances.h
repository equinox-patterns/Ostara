/************************************************************************
 **
 **  @file   ocoverseamallowances.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  What a cover page says about a pattern's margins: the two
 **          cases it is asked about, and the lines each of them prints.
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

#ifndef OCOVERSEAMALLOWANCES_H
#define OCOVERSEAMALLOWANCES_H

#include <QString>
#include <QStringList>
#include <QVector>

#include "oseamallowancelibrary.h"
#include "xml/vcoverpagedata.h"

/**
 * @brief The margins a pattern is cut with, said the way a cover page says them.
 *
 * **Two cases, and the page is one of them.** A pattern is either drawn with its seam allowances or
 * drawn without them, and which one it is decides what the reader has to be told: without them, the
 * page states it and stops; with them, the page has to name each margin the pattern actually
 * carries, because a reader measuring the outline needs to know which widths they are looking at.
 *
 * **The names come from the library, the widths come from the pattern** — and the two meet here.
 * This is the one place that decides which name answers which width, so a page and a future export
 * cannot come to name the same pattern differently. Nothing here reads a file or a pattern: both
 * are handed over, as the legend's lines are.
 */
namespace OCoverSeamAllowances
{
/// The pattern is drawn with its seam allowances, so the page names them.
inline const QString kIncluded = QStringLiteral("included");
/// The pattern is drawn without them, so the page says so and names nothing.
inline const QString kNotIncluded = QStringLiteral("notIncluded");

/// The option the case is answered under. Written by the catalogue, read by the resolver and the
/// control that renames a title: one key, so the three cannot come to look at different answers.
inline const QString kCaseKey = QStringLiteral("seamCase");
/// The option holding the units a width is stated in, as the application names them.
inline const QString kUnitsKey = QStringLiteral("seamUnits");
/// The option holding what separates those units from one another.
inline const QString kUnitSeparatorKey = QStringLiteral("seamUnitSeparator");
/// The option holding what separates the languages of a margin's name.
inline const QString kValuesLanguageSeparatorKey = QStringLiteral("seamValuesLanguageSeparator");

/**
 * @brief The key the title of @p seamCase is renamed under, in the template's shared rename map.
 *
 * **One title per case, and they are independent**: a page that renames the title it prints when the
 * margins are included says nothing about the title it prints when they are not. Both live in the
 * same map, under keys of their own, because a second map would be a second format.
 */
auto TitleKey(const QString &seamCase) -> QString;

/// @brief What the application calls @p seamCase by, before anyone renames it.
auto TitleWord(const QString &seamCase, const QString &locale) -> QString;

/** @brief One margin the page prints: what it is called, and how wide it is. */
struct Line
{
    QStringList texts; ///< the name in each language of the page, in block order
    qreal mm = 0.0;    ///< the width, in millimetres — the unit every page states a length in
};

/**
 * @brief The lines @p library has to say about the widths @p values.
 *
 * **One line per width an entry answers, in the library's order** — which is the order the user
 * arranged the names in, and the order a reader reads them. An entry that answers no width says
 * nothing: a name the pattern does not use is a name the page has no business printing.
 *
 * **An entry may answer several widths, and then it says several lines.** A library rule written as
 * "from 2 cm up is a hem" answers both the 2 cm and the 3 cm a pattern may hold, and those are two
 * different margins a reader has to be able to tell apart — so each is stated, in the pattern's own
 * order, smallest first. The library names them the same, which is honest: it says what a margin is
 * *called*, and it cannot know what this pattern calls each one. @p patternNames is where a pattern
 * says so — see @ref CoverPageMarginNameFor — and a width it has named is printed under that name.
 *
 * **The entry marked as the default answers @p defaultMm**, the width the application is set to —
 * and it prints it, like any other entry, only if the pattern really carries it.
 */
auto Lines(const OSeamAllowanceLibrary::Library &library, const QStringList &languages,
           const QVector<qreal> &values, qreal defaultMm,
           const QVector<VCoverPageMarginName> &patternNames) -> QVector<Line>;

/**
 * @brief The widths of @p values more than one line would have to name the same way.
 *
 * **The only widths a pattern has any reason to rename.** A library rule may be written as a
 * threshold — "from 2 cm up is a hem" — and then a pattern holding both a 2 cm and a 3 cm gets two
 * lines carrying the same word, which no reader can tell apart. Every other width is named by its
 * own rule and named once, and nothing about it needs deciding.
 *
 * Smallest first, in the pattern's own order. Empty is the ordinary answer.
 */
auto Ambiguous(const OSeamAllowanceLibrary::Library &library, const QVector<qreal> &values,
               qreal defaultMm) -> QVector<qreal>;

/**
 * @brief What @p library calls a margin of @p mm in @p locale, or nothing when no entry answers it.
 *
 * The rule of @ref Lines asked of one width and one language — which is what a field offering the
 * name to be replaced has to show: **the name that would be printed if nobody renamed anything.**
 */
auto LibraryNameFor(const OSeamAllowanceLibrary::Library &library, qreal mm, qreal defaultMm,
                    const QString &locale) -> QString;
} // namespace OCoverSeamAllowances

#endif // OCOVERSEAMALLOWANCES_H
