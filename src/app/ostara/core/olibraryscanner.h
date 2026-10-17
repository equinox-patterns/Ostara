/************************************************************************
 **
 **  @file   olibraryscanner.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  Walk a library folder: its sub-folders, and the files it holds — one level, or down the tree.
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

#ifndef OLIBRARYSCANNER_H
#define OLIBRARYSCANNER_H

#include <QString>
#include <QStringList>

/**
 * @brief How a library reads the folder it lives in — and, when asked, everything under it.
 *
 * The older libraries each carry their own flat scan, and that is deliberate: they list a folder and nothing
 * else, and none of them is touched here. This helper exists for the library that needs to look **inside** a
 * folder rather than beside it — a folder is then a place, and not just a list.
 *
 * Hidden entries are never returned: a library that showed a working tree's own folders, or a `.git`, would be
 * showing what is not its own.
 */
namespace OLibraryScanner
{
/// @brief The immediate sub-folders of @p dirPath, absolute paths, sorted by name — empty when there is none.
auto Directories(const QString &dirPath) -> QStringList;

/**
 * @brief The files of @p dirPath whose name matches @p nameFilter (`*.ostara`), absolute paths, sorted by name.
 *
 * With @p recursive, every sub-folder is walked as well, depth first, each level sorted — the order a reader
 * would have read in the tree.
 */
auto Files(const QString &dirPath, const QString &nameFilter, bool recursive) -> QStringList;
} // namespace OLibraryScanner

#endif // OLIBRARYSCANNER_H
