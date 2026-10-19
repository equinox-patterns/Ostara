/************************************************************************
 **
 **  @file   opatternlibrary.cpp
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

#include "opatternlibrary.h"

#include "olibraryscanner.h"
#include "olibrarystorage.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

#include <algorithm>

namespace
{
const QString kExtension = QStringLiteral("ostara");
const QString kFilePattern = QStringLiteral("*.ostara");
const QString kRootTag = QStringLiteral("pattern");

// The six things a pattern says about itself. The schema imposes their order, and all of them come before the
// geometry — which is exactly why the reading can stop early.
const QString kTagPatternName = QStringLiteral("patternName");
const QString kTagDescription = QStringLiteral("description");
const QString kTagNotes = QStringLiteral("notes");
const QString kTagFabricInfo = QStringLiteral("fabricInfo");
const QString kTagCollection = QStringLiteral("collection");
const QString kTagCustomer = QStringLiteral("customer");

// Where the drawing begins: below it there is nothing this library reads.
const QString kTagDraw = QStringLiteral("draw");

/// @brief One pattern of the library, read from its file's head — the file name and the date come from the file
/// itself, so they are readable even when the head is not.
auto ReadEntry(const QString &path) -> OPatternLibrary::Entry
{
    OPatternLibrary::Entry entry;
    entry.path = path;

    const QFileInfo info(path);
    entry.fileName = info.fileName();
    entry.modified = info.lastModified();

    QString error;
    if (not OPatternLibrary::ReadHeader(path, &entry, &error))
    {
        entry.error = error;
    }

    return entry;
}

/// @brief Sorts @p entries the way the list reads them — the name shown, ignoring the case.
void SortByDisplayName(QList<OPatternLibrary::Entry> *entries)
{
    std::sort(entries->begin(), entries->end(),
              [](const OPatternLibrary::Entry &left, const OPatternLibrary::Entry &right) {
                  return OPatternLibrary::DisplayName(left).compare(OPatternLibrary::DisplayName(right),
                                                                   Qt::CaseInsensitive)
                         < 0;
              });
}

/// @brief The patterns of @p paths whose shown name holds @p needle — every one of them when @p needle is empty.
/// The list is sorted the way it reads, not the way the disk lists it: the reader looks for a name.
auto Collect(const QStringList &paths, const QString &needle) -> QList<OPatternLibrary::Entry>
{
    QList<OPatternLibrary::Entry> entries;
    entries.reserve(paths.size());

    for (const QString &path : paths)
    {
        OPatternLibrary::Entry entry = ReadEntry(path);
        if (needle.isEmpty() || OPatternLibrary::DisplayName(entry).contains(needle, Qt::CaseInsensitive))
        {
            entries.append(entry);
        }
    }

    SortByDisplayName(&entries);

    return entries;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::Extension() -> QString
{
    return kExtension;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::FilePattern() -> QString
{
    return kFilePattern;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::DefaultDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathPatterns();
}

//---------------------------------------------------------------------------------------------------------------------
void OPatternLibrary::EnsureDirectory()
{
    const QString directory = DefaultDirectory();
    if (directory.isEmpty())
    {
        return;
    }

    // Idempotent, and never fatal: a library that cannot be created will say so the day something is written
    // into it — refusing to start the application over a folder would be the wrong trade.
    QDir().mkpath(directory);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::DisplayName(const Entry &entry) -> QString
{
    if (not entry.patternName.isEmpty())
    {
        return entry.patternName;
    }

    // The fallback is the file's name, and not a lesser name: it is what the reader sees in the Finder, and
    // what the window shows beside the pattern when the pattern says nothing.
    return QFileInfo(entry.path).completeBaseName();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::ReadHeader(const QString &filePath, Entry *entry, QString *error) -> bool
{
    QFile file(filePath);
    if (not file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        *error = QCoreApplication::translate("OPatternLibrary", "The file could not be opened.");
        return false;
    }

    QXmlStreamReader xml(&file);

    if (not xml.readNextStartElement())
    {
        *error = QCoreApplication::translate("OPatternLibrary", "The file is empty.");
        return false;
    }

    if (xml.name() != kRootTag)
    {
        *error = QCoreApplication::translate("OPatternLibrary", "Not a pattern file.");
        return false;
    }

    while (xml.readNextStartElement())
    {
        const QStringView name = xml.name();

        // The drawing: everything the library reads is above it, and a pattern's geometry is the bulk of its
        // file — reading it to show a name and a date would be paying for what nobody looks at.
        if (name == kTagDraw)
        {
            break;
        }

        if (name == kTagPatternName)
        {
            entry->patternName = xml.readElementText();
        }
        else if (name == kTagDescription)
        {
            entry->description = xml.readElementText();
        }
        else if (name == kTagNotes)
        {
            entry->notes = xml.readElementText();
        }
        else if (name == kTagFabricInfo)
        {
            entry->fabricInfo = xml.readElementText();
        }
        else if (name == kTagCollection)
        {
            entry->collection = xml.readElementText();
        }
        else if (name == kTagCustomer)
        {
            entry->customer = xml.readElementText();
        }
        else
        {
            // A section this library has nothing to say about — skipped whole, children and all.
            xml.skipCurrentElement();
        }
    }

    if (xml.hasError())
    {
        *error = QCoreApplication::translate("OPatternLibrary", "The file could not be read: %1")
                     .arg(xml.errorString());
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::Entries(const QString &dirPath) -> QList<Entry>
{
    return Collect(OLibraryScanner::Files(dirPath, kFilePattern, false), QString());
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::SubDirectories(const QString &dirPath) -> QStringList
{
    return OLibraryScanner::Directories(dirPath);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::Folders(const QString &rootPath) -> QList<Folder>
{
    QList<Folder> folders;
    const QList<OLibraryScanner::Folder> scanned = OLibraryScanner::Folders(rootPath);
    folders.reserve(scanned.size());
    for (const OLibraryScanner::Folder &folder : scanned)
    {
        folders.append({folder.label, folder.path});
    }

    return folders;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::PatternsOf(const QString &rootPath, const QString &name, Owner owner) -> QList<Entry>
{
    const QString wanted = name.trimmed();
    if (wanted.isEmpty())
    {
        return {};
    }

    const QStringList paths = OLibraryScanner::Files(rootPath, kFilePattern, true);

    QList<Entry> entries;
    for (const QString &path : paths)
    {
        Entry entry = ReadEntry(path);
        const QString named = (owner == Owner::Customer ? entry.customer : entry.collection).trimmed();
        if (named.compare(wanted, Qt::CaseInsensitive) == 0)
        {
            entries.append(entry);
        }
    }

    SortByDisplayName(&entries);
    return entries;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::Search(const QString &rootPath, const QString &needle) -> QList<Entry>
{
    const QString trimmed = needle.trimmed();
    if (trimmed.isEmpty())
    {
        return {};
    }

    return Collect(OLibraryScanner::Files(rootPath, kFilePattern, true), trimmed);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::ValidateChildName(const QString &parentPath, const QString &name, QString *error) -> bool
{
    return OLibraryStorage::ValidateChildName(parentPath, name, error);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::CreateDirectory(const QString &parentPath, const QString &name, QString *error) -> bool
{
    return OLibraryStorage::CreateDirectory(parentPath, name, error);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::TargetPath(const QString &sourcePath, const QString &targetDirectory) -> QString
{
    return OLibraryStorage::TargetPath(sourcePath, targetDirectory);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::Transfer(const QString &sourcePath, const QString &targetDirectory, bool overwrite,
                               QString *error) -> bool
{
    return OLibraryStorage::Transfer(sourcePath, targetDirectory, overwrite, error);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::Export(const QString &sourcePath, const QString &targetPath, QString *error) -> bool
{
    return OLibraryStorage::Export(sourcePath, targetPath, error);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPatternLibrary::Remove(const QString &filePath, QString *error) -> bool
{
    return OLibraryStorage::Remove(filePath, error);
}
