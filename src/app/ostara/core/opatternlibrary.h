/************************************************************************
 **
 **  @file   opatternlibrary.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The library of patterns: the `.ostara` files of a folder tree, read for what they say of themselves.
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

#ifndef OPATTERNLIBRARY_H
#define OPATTERNLIBRARY_H

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

/**
 * @brief Where patterns are kept, and what each of them says about itself.
 *
 * The library **does not open a pattern**: it reads the head of each file — the six things a pattern writes
 * about itself before its geometry begins — and that is enough for a list, a search and a detail panel. A
 * pattern is a whole document, with its drawing, its pieces and its sizes; none of that is read here.
 *
 * A folder is a place: the library walks a tree, one level for the list it shows, every level for the search.
 * Nothing is indexed and nothing is cached — the file is the truth, so a pattern renamed in the pattern's own
 * Property Browser is renamed here the next time the folder is read.
 */
namespace OPatternLibrary
{
/**
 * @brief What a pattern says about itself, read from the head of its file — and nothing of its geometry.
 *
 * The five free texts are the ones the two Property Browser pages write: `description`, `notes` and the
 * pattern's own name in Draw mode, `fabricInfo`, `customer` and `collection` in Details mode. The library shows
 * them; it never writes them.
 */
struct Entry
{
    QString path{};          ///< the file, absolute.
    QString fileName{};      ///< the file's own name, extension included — `bustedebase.ostara`.
    QString patternName{};   ///< the name the pattern wears; empty when its reader gave it none.
    QString collection{};    ///< the collection it belongs to, as the pattern states it.
    QString customer{};      ///< the client it is made for, as the pattern states it.
    QString description{};   ///< what its reader wrote about it, in Draw mode.
    QString notes{};         ///< the rest of what she wrote, in Draw mode.
    QString fabricInfo{};    ///< the fabric it is meant for, in Details mode.
    QDateTime modified{};    ///< when the file was last written.
    QString error{};         ///< why its head could not be read; empty when it could.
};

/// @brief The name a list shows: the pattern's own name, and its file's name when it has none.
auto DisplayName(const Entry &entry) -> QString;

/// @brief The extension of a pattern, without its dot — `ostara`.
auto Extension() -> QString;

/// @brief The filter a folder is read with — `*.ostara`.
auto FilePattern() -> QString;

/// @brief The folder the library lives in, from settings (GetPathPatterns).
auto DefaultDirectory() -> QString;

/**
 * @brief Makes sure the library's folder exists, and does nothing when it already does.
 *
 * Called once at the first launch, like the two template libraries: the window is opened **before** anything
 * is ever saved, so the folder has to be there to be looked at — and creating it is not a gesture the reader
 * should have to make.
 */
void EnsureDirectory();

/**
 * @brief Reads the head of @p filePath into @p entry, and stops before the geometry.
 *
 * The file is read as a stream and abandoned as soon as `<draw>` begins: the six elements sit at the top of the
 * document, in the order the schema imposes, and everything below them is a drawing the library has no use for.
 * A file that is not a pattern, or that cannot be opened, is refused with its reason rather than shown as a
 * nameless line.
 */
auto ReadHeader(const QString &filePath, Entry *entry, QString *error) -> bool;

/// @brief The patterns held directly in @p dirPath — this folder, not the ones under it — sorted by shown name.
auto Entries(const QString &dirPath) -> QList<Entry>;

/// @brief What a pattern names — the client it is made for, or the collection it belongs to.
enum class Owner
{
    Customer,   ///< the client, as the Property Browser writes it (`customer`).
    Collection, ///< the collection, as the Property Browser writes it (`collection`).
};

/**
 * @brief The patterns of the tree under @p rootPath that name @p name as their @p owner.
 *
 * Compared trimmed and ignoring the case: the link between a pattern and a client is a **name**, and a name typed
 * twice with one capital more is the same client. An empty name matches nothing — a pattern that names nobody
 * belongs to nobody, and a page showing every such pattern would be showing the whole library.
 */
auto PatternsOf(const QString &rootPath, const QString &name, Owner owner) -> QList<Entry>;

/// @brief A folder of the library, as a reader reads it and where it is on disk.
struct Folder
{
    QString label; ///< the relative path she reads — « Dupont », « Dupont/Hiver ».
    QString path;  ///< the absolute path.
};

/**
 * @brief Every folder under @p rootPath, sorted by the label she reads.
 *
 * The library's own folder is **not** one of them: each caller names it in its own words (the transfer dialog
 * writes « The library's own folder »), and a core that invented that word would decide what a window says.
 */
auto Folders(const QString &rootPath) -> QList<Folder>;

/// @brief The sub-folders of @p dirPath, absolute paths, sorted by name.
auto SubDirectories(const QString &dirPath) -> QStringList;

/**
 * @brief Every pattern of the tree under @p rootPath whose shown name contains @p needle, case-insensitively.
 *
 * The search is the one gesture that walks the whole library: a pattern can be in a folder the reader is not
 * looking at, and that is exactly what she is asking for. An empty @p needle returns nothing — the folder's own
 * list is what answers "everything", not the search.
 */
auto Search(const QString &rootPath, const QString &needle) -> QList<Entry>;

/**
 * @brief Whether @p name can be made a folder inside @p parentPath, and why not when it cannot.
 *
 * Empty, a path separator, or a name already taken: three refusals, and the reader is told which one she met
 * rather than left with a folder that did not appear.
 */
auto ValidateChildName(const QString &parentPath, const QString &name, QString *error) -> bool;

/// @brief Creates the folder @p name inside @p parentPath, after ValidateChildName.
auto CreateDirectory(const QString &parentPath, const QString &name, QString *error) -> bool;

/// @brief Where @p sourcePath would land in @p targetDirectory — the name it already has, there.
auto TargetPath(const QString &sourcePath, const QString &targetDirectory) -> QString;

/**
 * @brief Moves @p sourcePath into @p targetDirectory.
 *
 * With @p overwrite false, an existing file of the same name is a refusal — the caller asked the reader first,
 * and this is where her answer is honoured. With @p overwrite true the destination is removed before the move:
 * a rename over an existing file is not something the file system promises.
 */
auto Transfer(const QString &sourcePath, const QString &targetDirectory, bool overwrite, QString *error) -> bool;

/**
 * @brief Copies @p sourcePath to @p targetPath, replacing what is there.
 *
 * The destination is removed first, and this is the whole reason the gesture lives here: `QFile::copy` refuses
 * to overwrite, while the save dialog offers to — the piece library exports with that defect today, and a
 * reader who says "yes, replace it" must not then be told the copy failed.
 */
auto Export(const QString &sourcePath, const QString &targetPath, QString *error) -> bool;

/// @brief Removes the pattern at @p filePath. The caller has warned; nothing else is decided here.
auto Remove(const QString &filePath, QString *error) -> bool;
} // namespace OPatternLibrary

#endif // OPATTERNLIBRARY_H
