/************************************************************************
 **
 **  @file   osymbollibrary.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The symbols of the installation: the marks a legend is written
 **          with, and the words each is written as.
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

#ifndef OSYMBOLLIBRARY_H
#define OSYMBOLLIBRARY_H

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include "osymbolshapes.h"

/**
 * @brief The symbols a cover page's legend is written with, and what each one says.
 *
 * **A symbol is a mark and a word, and the word is written in every language the page is.** A legend
 * stands on a page that may be read in two languages, so a symbol carries one text per language
 * rather than one text that would have to be rewritten for each: the languages are declared once,
 * for the whole library, and every entry is then written in all of them.
 *
 * **A symbol may stand for several marks.** A pattern notched three different ways calls all three
 * of them "notches", and a legend that named them separately would be explaining a distinction the
 * reader does not have to make — so an entry carries a list of marks, read left to right on its
 * line, and one word underneath them.
 *
 * **A symbol is permanent, or it is chosen.** The grain line is on a piece every time, and so is
 * what the page's own marks mean: a permanent symbol is on every cover page that uses the template
 * without anyone asking for it. The others wait in the library until a page switches them on — a
 * buttonhole is worth explaining on the pattern that has one and nowhere else.
 *
 * **Nothing here is a file.** The mark is drawn by Ostara (@ref OSymbolRender), so a symbol has no
 * artwork to keep — what is kept is which mark it is and what it says. That is why the library is a
 * single index and not a folder of files, and, like the logo library's index, it is deliberately
 * **not** validated against a schema: it holds nothing that cannot be rewritten by hand, and a
 * malformed one costs the library, not the installation.
 *
 * **The order is the user's.** A legend is read from top to bottom and its order is an argument, so
 * entries are kept in the order they were put in and moved by hand.
 */
namespace OSymbolLibrary
{
/** @brief One symbol: the marks it stands for, and what it says in each language. */
struct Entry
{
    /**
     * @brief What a pattern calls this symbol by — drawn once, and never changed afterwards.
     *
     * **The entry has no other name.** What the list shows is the marks it draws, and its place in
     * the file is the user's to reorder, so neither can be referred to from outside: a pattern that
     * chose a symbol by its rank would be pointing at whatever took that rank next. See @ref GiveIds.
     */
    QString id;

    QVector<OSymbolShapes::Shape> shapes; ///< the marks, in the order they are read; never empty
    QMap<QString, QString> texts;         ///< one text per locale, keyed by the locale's code

    /**
     * @brief Whether the symbol belongs to every page that uses the template, or waits to be asked.
     *
     * **Permanent is the default**, because a symbol is written in the library for the things a
     * pattern always says — and a library that had to be switched on page by page would make the
     * user do the same work again on every page.
     */
    bool permanent = true;
};

/** @brief The library: the languages it is written in, and the symbols in the order they are read. */
struct Library
{
    QStringList languages; ///< locale codes, in display order; never empty
    QList<Entry> entries;  ///< in the order the user put them
};

/**
 * @brief Gives an id to every entry that has none, and says whether it filled any in.
 *
 * **A pattern refers to a symbol by its id**, so the id is drawn when the entry is created and never
 * touched again: moving, renaming or re-describing a symbol then leaves every pattern that chose it
 * still pointing at it, which a place in the list could not promise — the list is the user's to
 * reorder, and a removal would shift every rank below it.
 *
 * Two callers, and they are the two moments an entry can appear without one: @ref Load, reading a
 * library written before ids existed, and the page, before it writes an entry it has just created.
 * Both hand the result to @ref Save, so an id given here is on disk before anything can refer to it.
 */
auto GiveIds(Library &library) -> bool;

/** @brief The folder the library is kept in, from settings (GetPathSymbols). */
auto Directory() -> QString;

/** @brief The path of the index file, which is the whole library. */
auto MetaPath() -> QString;

/**
 * @brief The library, seeding it on first use.
 *
 * **The first launch is the only time anything is written for the user.** The grain line and the
 * fold line are in every pattern ever drawn, so a legend that had to be built before it could be
 * used would ask for work nobody has to do — they are written once, already said in every language
 * the application knows, and never written again. From then on the library is the user's: an entry
 * they delete does not come back, and one they rename keeps the name they gave it.
 *
 * A library that is there is read as it is, whatever it holds — including nothing, which is what a
 * user who deleted everything asked for.
 */
auto Load() -> Library;

/** @brief Writes the library, atomically, creating its folder if it has gone. */
auto Save(const Library &library, QString *error = nullptr) -> bool;
} // namespace OSymbolLibrary

#endif // OSYMBOLLIBRARY_H
