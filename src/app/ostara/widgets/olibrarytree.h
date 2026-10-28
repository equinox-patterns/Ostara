/************************************************************************
 **
 **  @file   olibrarytree.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The folder tree of a library, filled and read the same way everywhere it is shown.
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

#ifndef OLIBRARYTREE_H
#define OLIBRARYTREE_H

#include <QIcon>
#include <QString>

class QTreeWidget;

/**
 * @brief The folder tree of a library — the folders under its root, and which one is being pointed at.
 *
 * The window that shows the library and the dialog that saves into it show **the same tree**, and read the same
 * choice out of it. Written once here, so the two cannot drift apart: a folder the window shows and the dialog
 * does not would be a folder the reader could never save into.
 *
 * The library's own folder is a line of the tree, always, for every caller: it is the only way back up once a
 * folder has been chosen, and "nothing chosen" is not something a reader can see — let alone click.
 */
namespace OLibraryTree
{
/**
 * @brief Empties @p tree and fills it with the library's own folder, then every folder found under @p rootPath.
 *
 * @param rootLabel the name the library's own folder wears — each caller says it in the language the window is
 *        speaking, which is also why this is called again on a language change.
 * @param folderIcon the icon every line wears — each caller builds it with its own theme helper, the way the
 *        rest of the application does.
 *
 * The first line is the library's own folder, already open: a tree that opened on one closed line would hide
 * the folders the window is about.
 *
 * The caller cuts the signals around this: rebuilding a tree is not the reader choosing anything.
 */
void Fill(QTreeWidget *tree, const QString &rootPath, const QString &rootLabel, const QIcon &folderIcon);

/// @brief Points the selection at the folder @p path, opening its parents so that it is seen.
/// @return false when the tree does not hold it.
auto Select(QTreeWidget *tree, const QString &path) -> bool;

/// @brief The folder the tree points at — an empty string when it points at nothing.
auto SelectedPath(const QTreeWidget *tree) -> QString;
} // namespace OLibraryTree

#endif // OLIBRARYTREE_H
