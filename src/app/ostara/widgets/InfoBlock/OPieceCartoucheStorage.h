/************************************************************************
 **
 **  @file   OPieceCartoucheStorage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Scan / load / save of cartouche template files (.oct).
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

#ifndef OPIECECARTOUCHESTORAGE_H
#define OPIECECARTOUCHESTORAGE_H

#include "OPieceCartoucheTemplate.h"

#include <QString>
#include <QStringList>

namespace OPieceCartoucheStorage
{
/// Default folder from settings (GetPathCartoucheTemplates).
auto DefaultDirectory() -> QString;

/// List of `.oct` files in the given folder (absolute paths), alphabetical.
auto ScanDirectory(const QString &dirPath) -> QStringList;

/// Load a `.oct` file into `tpl`. Returns false and fills `error` on failure.
auto Load(const QString &filePath, OPieceCartoucheTemplate *tpl, QString *error = nullptr) -> bool;

/// Save `tpl` to `filePath`, validated against the cartouche template XSD
/// (atomic write via VDomDocument::SaveDocument). Returns false on failure.
auto Save(const OPieceCartoucheTemplate &tpl, const QString &filePath, QString *error = nullptr) -> bool;

/// Ensure the default template exists — created on first use when the folder is
/// empty, so the cartouche dropdown is never empty (spec §4.1).
void EnsureDefaultTemplate();
}  // namespace OPieceCartoucheStorage

#endif // OPIECECARTOUCHESTORAGE_H
