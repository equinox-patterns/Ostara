/************************************************************************
 **
 **  @file   olibraryscanner.cpp
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

#include "olibraryscanner.h"

#include <QDir>
#include <QFileInfo>

#include <algorithm>

namespace
{
/// The files of one level that match @p nameFilter, sorted by name, hidden ones left out — the filter `QDir`
/// applies without being asked, and the reason a library never shows a `.git`.
auto LevelFiles(const QDir &dir, const QString &nameFilter) -> QStringList
{
    const QStringList names = dir.entryList({nameFilter}, QDir::Files, QDir::Name | QDir::IgnoreCase);

    QStringList paths;
    paths.reserve(names.size());
    for (const QString &name : names)
    {
        paths.append(dir.absoluteFilePath(name));
    }

    return paths;
}

/// @brief The sub-folders of @p dir, absolute paths, sorted by name — the symbolic links left out.
///
/// A library is a folder a reader fills by hand: a link pointing at one of its own ancestors is a loop, and a
/// walk that follows it never comes back. Skipping the links costs a reader who sorted her library with one
/// nothing but a level she cannot see here — and the folder itself is still there, in the Finder.
auto SubFolders(const QDir &dir) -> QStringList
{
    const QStringList folders = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);

    QStringList paths;
    paths.reserve(folders.size());
    for (const QString &folder : folders)
    {
        const QString path = dir.absoluteFilePath(folder);
        if (QFileInfo(path).isSymLink())
        {
            continue;
        }

        paths.append(path);
    }

    return paths;
}

/// This level, then each sub-folder in turn: the order the tree shows, so a list built from it never jumps.
void Walk(const QDir &dir, const QString &nameFilter, bool recursive, QStringList *paths)
{
    paths->append(LevelFiles(dir, nameFilter));

    if (not recursive)
    {
        return;
    }

    for (const QString &folder : SubFolders(dir))
    {
        Walk(QDir(folder), nameFilter, recursive, paths);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryScanner::Directories(const QString &dirPath) -> QStringList
{
    const QDir dir(dirPath);
    if (not dir.exists())
    {
        return {};
    }

    return SubFolders(dir);
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryScanner::Files(const QString &dirPath, const QString &nameFilter, bool recursive) -> QStringList
{
    if (nameFilter.isEmpty())
    {
        return {};
    }

    const QDir dir(dirPath);
    if (not dir.exists())
    {
        return {};
    }

    QStringList paths;
    Walk(dir, nameFilter, recursive, &paths);
    return paths;
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
void CollectFolders(const QString &root, const QString &prefix, QList<OLibraryScanner::Folder> *folders)
{
    const QStringList subFolders = OLibraryScanner::Directories(root);
    for (const QString &folder : subFolders)
    {
        const QString name = QFileInfo(folder).fileName();
        const QString label = prefix.isEmpty() ? name : prefix + QLatin1Char('/') + name;

        folders->append({label, folder});
        CollectFolders(folder, label, folders);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryScanner::Folders(const QString &rootPath) -> QList<OLibraryScanner::Folder>
{
    QList<Folder> folders;
    CollectFolders(rootPath, QString(), &folders);

    std::sort(folders.begin(), folders.end(), [](const Folder &left, const Folder &right) {
        return left.label.compare(right.label, Qt::CaseInsensitive) < 0;
    });

    return folders;
}
