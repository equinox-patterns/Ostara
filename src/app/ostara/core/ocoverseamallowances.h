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
 * **One line per entry, in the library's order**, which is the order the user arranged the names in
 * and the order a reader reads them. An entry that answers no width says nothing: a name the
 * pattern does not use is a name the page has no business printing.
 *
 * **The entry marked as the default answers @p defaultMm**, the width the application is set to —
 * and it prints it, like any other entry, only if the pattern really carries it.
 *
 * **An entry names one width.** Should the pattern hold two widths that answer the same entry, the
 * first one found is the one printed: the entry says what a margin is called, and a library that
 * has two different widths answering to one name is a library that was written wrong, not a page
 * that has something to say.
 */
auto Lines(const OSeamAllowanceLibrary::Library &library, const QStringList &languages,
           const QVector<qreal> &values, qreal defaultMm) -> QVector<Line>;
} // namespace OCoverSeamAllowances

#endif // OCOVERSEAMALLOWANCES_H
