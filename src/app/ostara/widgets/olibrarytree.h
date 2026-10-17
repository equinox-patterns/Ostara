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
 * The callers keep their own idea of what "nothing chosen" means — the window reads the library's own folder,
 * which is why it does not show it as a line of its own; the save dialog needs a destination that is always
 * visible, so it asks for that line.
 */
namespace OLibraryTree
{
/**
 * @brief Empties @p tree and fills it with the folders found under @p rootPath.
 *
 * @param includeRoot adds the library's own folder as the first line, wearing @p rootLabel.
 * @param folderIcon the icon every line wears — each caller builds it with its own theme helper, the way the
 *        rest of the application does.
 *
 * The caller cuts the signals around this: rebuilding a tree is not the reader choosing anything.
 */
void Fill(QTreeWidget *tree, const QString &rootPath, bool includeRoot, const QString &rootLabel,
          const QIcon &folderIcon);

/// @brief Points the selection at the folder @p path. @return false when the tree does not hold it.
auto Select(QTreeWidget *tree, const QString &path) -> bool;

/// @brief The folder the tree points at — an empty string when it points at nothing.
auto SelectedPath(const QTreeWidget *tree) -> QString;
} // namespace OLibraryTree

#endif // OLIBRARYTREE_H
