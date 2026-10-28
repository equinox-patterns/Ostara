/************************************************************************
 **
 **  @file   olibrarytree.cpp
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

#include "olibrarytree.h"

#include "core/olibraryscanner.h"

#include <QFileInfo>
#include <QList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

namespace
{
/// One level under @p parent, then each of those in turn — the order the tree reads, top to bottom.
void AddLevel(QTreeWidgetItem *parent, const QString &path, const QIcon &folderIcon)
{
    const QStringList folders = OLibraryScanner::Directories(path);
    for (const QString &folder : folders)
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, QFileInfo(folder).fileName());
        item->setIcon(0, folderIcon);
        item->setData(0, Qt::UserRole, folder);
        parent->addChild(item);

        AddLevel(item, folder, folderIcon);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OLibraryTree::Fill(QTreeWidget *tree, const QString &rootPath, const QString &rootLabel,
                        const QIcon &folderIcon)
{
    tree->clear();

    // The library's own folder, as a line of its own. It is where the window stands when no folder is chosen,
    // and it is the only line a reader can click to come back up from a subfolder.
    auto *root = new QTreeWidgetItem();
    root->setText(0, rootLabel);
    root->setIcon(0, folderIcon);
    root->setData(0, Qt::UserRole, rootPath);
    tree->addTopLevelItem(root);

    AddLevel(root, rootPath, folderIcon);

    // Open, so that the folders underneath stay in sight: closing them is the reader's gesture, never the
    // window's.
    root->setExpanded(true);
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryTree::Select(QTreeWidget *tree, const QString &path) -> bool
{
    if (tree == nullptr || path.isEmpty())
    {
        return false;
    }

    for (int top = 0; top < tree->topLevelItemCount(); ++top)
    {
        QList<QTreeWidgetItem *> stack{tree->topLevelItem(top)};

        while (not stack.isEmpty())
        {
            QTreeWidgetItem *current = stack.takeLast();
            if (current->data(0, Qt::UserRole).toString() == path)
            {
                // A folder chosen after a search can be under a closed branch: selecting an item the reader
                // cannot see would leave the tree pointing at nothing, while the list shows that folder.
                for (QTreeWidgetItem *parent = current->parent(); parent != nullptr; parent = parent->parent())
                {
                    parent->setExpanded(true);
                }

                tree->setCurrentItem(current);
                tree->scrollToItem(current);
                return true;
            }

            for (int child = 0; child < current->childCount(); ++child)
            {
                stack.append(current->child(child));
            }
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryTree::SelectedPath(const QTreeWidget *tree) -> QString
{
    const QTreeWidgetItem *item = tree != nullptr ? tree->currentItem() : nullptr;
    return item != nullptr ? item->data(0, Qt::UserRole).toString() : QString();
}
