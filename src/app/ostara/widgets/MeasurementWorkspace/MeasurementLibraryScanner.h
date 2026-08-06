/************************************************************************
 **
 **  @file   MeasurementLibraryScanner.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/07/2026
 **
 **  @brief  Utility to scan the measurement library and extract structure.
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

#ifndef MEASUREMENTLIBRARYSCANNER_H
#define MEASUREMENTLIBRARYSCANNER_H

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

struct MeasurementRecord;

struct LibraryFileEntry
{
    QString displayName;  // file name without extension
    QString filePath;     // absolute path
    bool isFavorite = false;
    QDateTime created;    // QFileInfo::birthTime()
    QDateTime modified;   // QFileInfo::lastModified()
};

namespace MeasurementLibraryScanner
{

/// Scan the library folder for .omt files.
/// Favorites are placed first with a star prefix, then alphabetical.
auto ScanLibrary(const QString &libPath, const QStringList &favorites) -> QList<LibraryFileEntry>;

/// Open an .omt file and extract only the structural fields
/// (name, fullName, description, type, collapsed).
/// Values and increments are NOT copied.
auto ExtractStructure(const QString &filePath) -> QVector<MeasurementRecord>;

} // namespace MeasurementLibraryScanner

#endif // MEASUREMENTLIBRARYSCANNER_H
