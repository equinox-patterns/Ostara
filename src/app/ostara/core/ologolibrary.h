/************************************************************************
 **
 **  @file   ologolibrary.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The installation's logo library — one folder of SVG files, and their names.
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

#ifndef OLOGOLIBRARY_H
#define OLOGOLIBRARY_H

#include <QList>
#include <QString>

/**
 * @brief The logos of the installation: a folder of SVG files, and the names they are shown under.
 *
 * **Not a template family's library.** A logo belongs to whoever is drawing — a brand, a
 * workshop — and not to the shape it happens to be placed on: the same set is offered to the
 * cartouche and to the cover page, and adding one in either makes it available in both. Which
 * is why this lives here rather than beside any one editor's storage.
 *
 * **A file is the logo.** What lives in the folder is the truth, and the metadata file beside it
 * carries only a display name and notes — so a file dropped in by hand is a logo like any other,
 * and the list is built by reading the folder rather than the index. `fileName` is therefore the
 * identity, and it is what a template stores.
 *
 * **SVG only.** Nothing here checks that: a file the folder holds is a logo as far as this is
 * concerned, and whether it draws is answered when something tries to render it.
 *
 * The index is XML but is deliberately **not** validated against a schema, unlike the template
 * files: it is a cache of names for files that already exist, and a malformed one costs the names
 * and nothing else — reading it falls back to the file names themselves.
 */
namespace OLogoLibrary
{
/** @brief One logo: the file that is it, and what it is shown as. */
struct Entry
{
    QString fileName; ///< the file's name, bare — the identity, and what a template stores
    QString name;     ///< the name shown in a list; falls back to the file's base name
    QString notes;    ///< free text, shown nowhere but the library page
};

/** @brief The folder logos are kept in, from settings (GetPathLogos). */
auto Directory() -> QString;

/** @brief The path of the metadata file, which sits beside the files it names. */
auto MetaPath() -> QString;

/**
 * @brief Every logo in the folder, in name order, with its display name — empty if there is none.
 *
 * The folder is what is read, so a file whose metadata entry has gone is still a logo; an entry
 * whose file has gone is not, and is dropped — which is what makes the index follow the folder
 * rather than the other way round.
 */
auto Load() -> QList<Entry>;

/** @brief Writes the display names and notes, atomically. The folder must exist. */
auto Save(const QList<Entry> &logos, QString *error = nullptr) -> bool;

/**
 * @brief Copies @p srcPath into the library under its own name.
 *
 * **The file is copied as it is**, never renamed and never converted: it belongs to the user, and
 * a logo library that rewrote what it was given would be deciding what a brand's file is called.
 * A name already in the library is replaced without asking, which the library page is expected
 * to have asked about.
 */
auto Add(const QString &srcPath, QString *error = nullptr) -> bool;

/** @brief Removes a logo and its entry. A name with no file behind it is not an error. */
auto Remove(const QString &fileName, QString *error = nullptr) -> bool;
} // namespace OLogoLibrary

#endif // OLOGOLIBRARY_H
