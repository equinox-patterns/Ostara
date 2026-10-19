/************************************************************************
 **
 **  @file   olibrarystorage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/10/2026
 **
 **  @brief  What a library does to its files: make a folder, move one in, copy one out, remove it.
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

#ifndef OLIBRARYSTORAGE_H
#define OLIBRARYSTORAGE_H

#include <QString>

/**
 * @brief The gestures a library makes on the files it holds — the same for every library that has folders.
 *
 * A library of patterns and a library of pieces are two lists of files under a root folder; both sort them into
 * sub-folders, and both offer the same three gestures on a line: move it to another folder, copy it out of the
 * library, delete it. Written once here, so the two libraries cannot disagree on what "already exists" means or on
 * what a refused name says.
 *
 * The library itself keeps its own namespace, its own entry, its own reading; only the file system part is here.
 */
namespace OLibraryStorage
{
/**
 * @brief Says why @p name cannot be a child of @p parentPath: empty, holding a separator, or already taken.
 *
 * @param error the reason, translated — never looked at when the answer is true.
 * @return true when the folder can be created under that name.
 */
auto ValidateChildName(const QString &parentPath, const QString &name, QString *error) -> bool;

/// @brief Validates @p name the way ValidateChildName does, then makes the folder and returns its absolute path.
auto CreateDirectory(const QString &parentPath, const QString &name, QString *error) -> bool;

/// @brief Where @p sourcePath lands when moved into @p targetDirectory: its own file name, there.
auto TargetPath(const QString &sourcePath, const QString &targetDirectory) -> QString;

/**
 * @brief Moves the file of @p sourcePath into @p targetDirectory.
 *
 * @param overwrite the reader answered yes to replacing what is already there; the destination is then removed
 *        first, because a rename over an existing file is not something the file system promises.
 */
auto Transfer(const QString &sourcePath, const QString &targetDirectory, bool overwrite, QString *error) -> bool;

/**
 * @brief Copies the file of @p sourcePath to @p targetPath, replacing what is there.
 *
 * `QFile::copy` refuses to overwrite while the save dialog offers to: the destination is removed first, so a
 * reader who answered "replace it" is not told afterwards that the copy failed.
 */
auto Export(const QString &sourcePath, const QString &targetPath, QString *error) -> bool;

/// @brief Removes the file, and says so when it refuses.
auto Remove(const QString &filePath, QString *error) -> bool;
} // namespace OLibraryStorage

#endif // OLIBRARYSTORAGE_H
