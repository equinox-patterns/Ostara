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
#include <QFile>
#include <QFileInfo>

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
            *error = QCoreApplication::translate("OLibraryStorage", "The folder could not be created.");
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
        *error = QCoreApplication::translate("OLibraryStorage", "The file could not be moved there.");
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
