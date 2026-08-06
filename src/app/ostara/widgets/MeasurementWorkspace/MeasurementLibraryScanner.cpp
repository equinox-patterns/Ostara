/************************************************************************
 **
 **  @file   MeasurementLibraryScanner.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/07/2026
 **
 **  @brief  Library scanner implementation.
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

#include "MeasurementLibraryScanner.h"
#include "xml/oomtdocument.h"
#include "omeasurementtypes.h"
#include "ifc/exception/vexceptionobjecterror.h"

#include <QDir>
#include <QFileInfo>

namespace MeasurementLibraryScanner
{

auto ScanLibrary(const QString &libPath, const QStringList &favorites) -> QList<LibraryFileEntry>
{
    QList<LibraryFileEntry> result;

    QDir dir(libPath);
    if (!dir.exists())
    {
        return result;
    }

    const QStringList files = dir.entryList({QStringLiteral("*.omt"), QStringLiteral("*.otu")}, QDir::Files,
                                            QDir::Name | QDir::IgnoreCase);

    QList<LibraryFileEntry> favEntries;
    QList<LibraryFileEntry> normalEntries;

    for (const QString &fileName : files)
    {
        LibraryFileEntry entry;
        entry.displayName = QFileInfo(fileName).completeBaseName();
        entry.filePath = dir.absoluteFilePath(fileName);
        entry.isFavorite = favorites.contains(entry.filePath);

        QFileInfo fi(entry.filePath);
        entry.created = fi.birthTime();
        entry.modified = fi.lastModified();

        if (entry.isFavorite)
        {
            favEntries.append(entry);
        }
        else
        {
            normalEntries.append(entry);
        }
    }

    result.append(favEntries);
    result.append(normalEntries);
    return result;
}

auto ExtractStructure(const QString &filePath) -> QVector<MeasurementRecord>
{
    QVector<MeasurementRecord> records;

    auto doc = std::make_unique<OOmtDocument>();
    try
    {
        doc->setXMLContent(filePath);
    }
    catch (const VExceptionObjectError &)
    {
        return records;
    }

    const QStringList names = doc->GetAllMNames();
    for (const QString &name : names)
    {
        MeasurementRecord rec;
        rec.name = name;
        rec.fullName = doc->GetMFullName(name);
        rec.description = doc->GetMDescription(name);
        rec.anchorValue = 0.0;
        rec.increments = {};

        const QString type = doc->GetMType(name);
        if (type == QStringLiteral("separator"))
        {
            rec.isSeparator = true;
            rec.isCollapsed = doc->GetMCollapsed(name);
        }

        records.append(rec);
    }

    return records;
}

} // namespace MeasurementLibraryScanner
