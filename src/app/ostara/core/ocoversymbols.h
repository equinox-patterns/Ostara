/************************************************************************
 **
 **  @file   ocoversymbols.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The symbols a cover page's legend is made of — the marks, and
 **          the words, ready to be put into lines.
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

#ifndef OCOVERSYMBOLS_H
#define OCOVERSYMBOLS_H

#include <QStringList>
#include <QVector>

#include "osymbollibrary.h"

#include "vwidgets/ocartouchepainter.h"

/**
 * @brief Which symbols a page shows, and what each one comes to.
 *
 * **The library is not the page's.** A legend is written on a page that is being laid out, and what
 * it holds is a matter of which symbols are permanent — a question about the library — and of which
 * languages the page is printed in. This is where the two meet, so that the resolver never has to
 * know a library exists: it is handed lines, exactly as it is handed a list of sizes
 * (@ref OCoverPageSizes).
 *
 * Nothing here draws: @ref OSymbolRender composes the marks, and the painter puts them on the page.
 */
namespace OCoverSymbols
{
/**
 * @brief The key a legend's own title is renamed under.
 *
 * **Stable, and not the word it shows.** A title is renamed per language and the file keys those
 * renames by this token; keying them by the word itself would lose every one of them the day the
 * interface changes language.
 */
inline const QString kTitleKey = QStringLiteral("symbols");

/**
 * @brief What a legend's title says in one language, before anyone renames it.
 *
 * An empty @p locale answers in the interface's own language, which is what a field offering the
 * word to be changed needs; a named one answers in that language, which is what the page prints.
 */
auto TitleWord(const QString &locale) -> QString;

/** @brief One line of the legend: the marks a symbol stands for, and what it says. */
struct Line
{
    OCartoucheSymbolMarks marks; ///< the marks, side by side, at the size they will print at
    QStringList texts;           ///< one per language of the page, in that order
};

/**
 * @brief The lines the given languages call for, in the library's own order.
 *
 * **The permanent symbols, and the ones the pattern chose.** A permanent symbol is on every page
 * that uses the template without anyone asking. The others wait in the library, and the pattern
 * asks for one by its id — so the legend says what the pattern carries, and not everything the
 * library happens to hold.
 *
 * **One pass, in the library's order.** A legend is read top to bottom and its order is an argument
 * the user makes by arranging the library, so a chosen symbol takes its place among the permanent
 * ones rather than being appended after them: a legend whose order depended on how a symbol got
 * there would be one nobody could lay out.
 */
auto Lines(const OSymbolLibrary::Library &library, const QStringList &languages,
           const QStringList &chosenIds) -> QVector<Line>;
} // namespace OCoverSymbols

#endif // OCOVERSYMBOLS_H
