/************************************************************************
 **
 **  @file   olibrarystorage.cpp
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

#include "olibrarystorage.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryStorage::ValidateChildName(const QString &parentPath, const QString &name, QString *error) -> bool
{
    const QString trimmed = name.trimmed();

    QString reason;
    if (trimmed.isEmpty())
    {
        reason = QCoreApplication::translate("OLibraryStorage", "A folder's name cannot be empty.");
    }
    else if (trimmed.contains(QLatin1Char('/')) || trimmed.contains(QLatin1Char('\\')))
    {
        reason = QCoreApplication::translate("OLibraryStorage", "A folder's name cannot hold a slash.");
    }
    else if (QDir(parentPath).exists(trimmed))
    {
        reason = QCoreApplication::translate("OLibraryStorage", "\"%1\" already exists here.").arg(trimmed);
    }

    if (reason.isEmpty())
    {
        return true;
    }

    if (error != nullptr)
    {
        *error = reason;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryStorage::CreateFolder(const QString &parentPath, const QString &name, QString *error) -> bool
{
    if (not ValidateChildName(parentPath, name, error))
    {
        return false;
    }

    const QString path = QDir(parentPath).absoluteFilePath(name.trimmed());
    if (not QDir().mkpath(path))
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OLibraryStorage",
                                                 "Ostara could not create this folder:\n%1\n\nCheck that you are "
                                                 "allowed to write in the folder that holds it.")
                         .arg(QDir::toNativeSeparators(path));
        }
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryStorage::TargetPath(const QString &sourcePath, const QString &targetDirectory) -> QString
{
    return QDir(targetDirectory).absoluteFilePath(QFileInfo(sourcePath).fileName());
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryStorage::Transfer(const QString &sourcePath, const QString &targetDirectory, bool overwrite,
                               QString *error) -> bool
{
    const QString target = TargetPath(sourcePath, targetDirectory);
    if (target == sourcePath)
    {
        return true; // Moved to where it already is: nothing to do, and nothing to explain.
    }

    if (QFileInfo::exists(target))
    {
        if (not overwrite)
        {
            if (error != nullptr)
            {
                *error = QCoreApplication::translate("OLibraryStorage", "\"%1\" already exists there.")
                             .arg(QFileInfo(target).fileName());
            }
            return false;
        }

        // The reader said yes: the destination goes first, so the rename has a free name.
        if (not QFile::remove(target))
        {
            if (error != nullptr)
            {
                *error = QCoreApplication::translate("OLibraryStorage",
                                                     "The file it replaces could not be removed.");
            }
            return false;
        }
    }

    if (QFile::rename(sourcePath, target))
    {
        return true;
    }

    // Another volume: the move is a copy that must remove what it copied, or the library would hold two of it.
    if (QFile::copy(sourcePath, target) && QFile::remove(sourcePath))
    {
        return true;
    }

    if (error != nullptr)
    {
        *error = QCoreApplication::translate("OLibraryStorage",
                                             "Ostara could not move this file to:\n%1\n\nCheck that the folder "
                                             "allows writing, and that the file is not open elsewhere.")
                     .arg(QDir::toNativeSeparators(target));
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryStorage::Export(const QString &sourcePath, const QString &targetPath, QString *error) -> bool
{
    if (QDir::cleanPath(targetPath) == QDir::cleanPath(sourcePath))
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OLibraryStorage", "The chosen destination is the file itself.");
        }
        return false;
    }

    if (QFileInfo::exists(targetPath) && not QFile::remove(targetPath))
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OLibraryStorage", "The file it replaces could not be removed.");
        }
        return false;
    }

    if (not QFile::copy(sourcePath, targetPath))
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OLibraryStorage",
                                                 "Could not copy the file to the selected destination.");
        }
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryStorage::Remove(const QString &filePath, QString *error) -> bool
{
    if (not QFile::remove(filePath))
    {
        if (error != nullptr)
        {
            *error = QCoreApplication::translate("OLibraryStorage", "Could not delete the file.");
        }
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// @brief The path as Qt compares it: `'/'` separators, canonical when the file is there, cleaned otherwise.
auto ComparablePath(const QString &path) -> QString
{
    const QString qtPath = QDir::fromNativeSeparators(path);
    const QString canonical = QDir::fromNativeSeparators(QFileInfo(qtPath).canonicalFilePath());
    return canonical.isEmpty() ? QDir::cleanPath(qtPath) : canonical;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryStorage::IsUnder(const QString &root, const QString &path) -> bool
{
    if (root.isEmpty() || path.isEmpty())
    {
        return false;
    }

#if defined(Q_OS_WIN)
    const Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif

    const QString comparableRoot = ComparablePath(root);
    const QString comparablePath = ComparablePath(path);
    if (comparableRoot.isEmpty() || comparablePath.isEmpty())
    {
        return false;
    }

    if (comparablePath.compare(comparableRoot, pathCase) == 0)
    {
        return true;
    }

    QString prefix = comparableRoot;
    if (!prefix.endsWith(QLatin1Char('/')))
    {
        prefix += QLatin1Char('/');
    }
    return comparablePath.startsWith(prefix, pathCase);
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// @brief Every folder under @p path, deepest first, so a folder is empty by the time it is removed.
auto SubDirectories(const QString &path) -> QStringList
{
    QStringList dirs;
    QDirIterator it(path, QDir::Dirs | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        dirs << it.next();
    }
    std::sort(dirs.begin(), dirs.end(),
              [](const QString &first, const QString &second) { return first.size() > second.size(); });
    return dirs;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryStorage::IsNested(const QString &first, const QString &second) -> bool
{
    if (first.isEmpty() || second.isEmpty())
    {
        return false;
    }

#if defined(Q_OS_WIN)
    const Qt::CaseSensitivity pathCase = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity pathCase = Qt::CaseSensitive;
#endif

    QString left = QDir::cleanPath(first);
    QString right = QDir::cleanPath(second);
    if (not left.endsWith(QLatin1Char('/')))
    {
        left += QLatin1Char('/');
    }
    if (not right.endsWith(QLatin1Char('/')))
    {
        right += QLatin1Char('/');
    }

    return right.startsWith(left, pathCase) || left.startsWith(right, pathCase);
}

//---------------------------------------------------------------------------------------------------------------------
auto OLibraryStorage::MoveFolderContents(const QString &source, const QString &target, QStringList *blocked) -> bool
{
    // The caller's list is an output, never an input: what it may already hold cannot decide this move.
    QStringList problems;

    if (not QDir(source).exists())
    {
        return true; // nothing to move: a folder that does not exist already "lives" nowhere else
    }

    // The same folder, possibly spelled differently ("Lib" and "LIB" are one folder on Windows): there is
    // nothing to move, and refusing would scold the reader for a gesture that changes nothing.
    const QString canonicalSource = QFileInfo(source).canonicalFilePath();
    const QString canonicalTarget = QFileInfo(target).canonicalFilePath();
    if (not canonicalSource.isEmpty() && canonicalSource == canonicalTarget)
    {
        return true;
    }

    // Moving a folder into its own subtree would copy files into the very tree it then deletes; moving it
    // over its own parent would empty the parent. The page refuses these two cases before asking, but the
    // helper has to be safe on its own — it is the only thing standing between a reader and their files.
    if (IsNested(source, target))
    {
        problems << QDir::toNativeSeparators(target);
        if (blocked != nullptr)
        {
            *blocked = problems;
        }
        return false;
    }

    const bool targetExisted = QDir(target).exists();

    if (not QDir().mkpath(target))
    {
        problems << QDir::toNativeSeparators(target);
        if (blocked != nullptr)
        {
            *blocked = problems;
        }
        return false;
    }

    // What the destination already held. A rollback may remove what this attempt created, never what was
    // there before: an empty sub-folder can be a deliberate state, and this helper does not own it.
    QSet<QString> existed;
    existed.insert(QDir::cleanPath(target));
    for (const QString &dir : SubDirectories(target))
    {
        existed.insert(QDir::cleanPath(dir));
    }

    QStringList files;
    QDirIterator it(source, QDir::Files | QDir::Hidden | QDir::System, QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        files << it.next();
    }

    QDir const sourceDir(source);
    QDir const targetDir(target);
    QStringList copied;

    for (const QString &file : files)
    {
        const QString relative = sourceDir.relativeFilePath(file);
        const QString destination = targetDir.filePath(relative);

        if (QFile::exists(destination) || not QDir().mkpath(QFileInfo(destination).absolutePath())
            || not QFile::copy(file, destination))
        {
            problems << relative;
            continue;
        }

        copied << file;
    }

    if (not problems.isEmpty())
    {
        // Every copy made so far goes, and the source keeps all its files: both sides are left as they were.
        // Only the folders this attempt created are removed — the ones that were there stay, empty or not.
        for (const QString &file : copied)
        {
            QFile::remove(targetDir.filePath(sourceDir.relativeFilePath(file)));
        }

        for (const QString &dir : SubDirectories(target))
        {
            if (not existed.contains(QDir::cleanPath(dir)))
            {
                QDir().rmdir(dir); // Only when empty.
            }
        }
        if (not targetExisted)
        {
            QDir().rmdir(target); // Only when empty.
        }

        if (blocked != nullptr)
        {
            *blocked = problems;
        }
        return false;
    }

    // The copies are all in place — that is what this function promises. Removing the sources is best
    // effort: a file locked by another program (OneDrive, an antivirus, an open viewer) keeps its copy
    // behind, and the reader deletes it herself. Nothing is lost either way.
    for (const QString &file : copied)
    {
        QFile::remove(file);
    }

    const QStringList dirs = SubDirectories(source);
    for (const QString &dir : dirs)
    {
        QDir().rmdir(dir); // Only when empty.
    }
    QDir().rmdir(source);

    if (blocked != nullptr)
    {
        blocked->clear();
    }
    return true;
}
