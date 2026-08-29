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

#include <QList>
#include <QString>
#include <QStringList>

namespace OPieceCartoucheStorage
{
/// One logo of the library: the SVG file (stable id) plus its display name
/// and free-form notes. The name/notes live in a `logos.xml` metadata file
/// next to the SVGs — the SVG files themselves are never modified.
struct LogoEntry
{
    QString fileName;  ///< the .svg file name in the logos folder
    QString name;      ///< display name shown in the template dropdown
    QString notes;     ///< free-form notes
};

/// One client of the library: a stable id plus contact fields. All the data
/// lives in a `clients.xml` metadata file (no per-client file).
struct ClientEntry
{
    QString id;     ///< stable id, never changes (uuid)
    QString name;   ///< display name
    QString mail;   ///< e-mail
    QString phone;  ///< phone number
    QString info;   ///< free-form information
};

/// Default folder from settings (GetPathCartoucheTemplates).
auto DefaultDirectory() -> QString;

/// Default logo library folder from settings (GetPathLogos).
auto LogosDirectory() -> QString;

/// List the logo library: every `.svg` file merged with its metadata.
/// A file without metadata gets a default name (file base name).
auto LoadLogos() -> QList<LogoEntry>;

/// Persist the logo library metadata (`logos.xml`, atomic write).
/// Returns false and fills `error` on failure.
auto SaveLogos(const QList<LogoEntry> &logos, QString *error = nullptr) -> bool;

/// Copy an SVG file into the logo library (overwrites a same-named entry;
/// a file already in the library is a no-op) and ensure it has a metadata
/// entry. Returns false and fills `error` on failure.
auto AddLogo(const QString &srcPath, QString *error = nullptr) -> bool;

/// Remove a logo from the library by file name. Returns false and fills
/// `error` on failure.
auto RemoveLogo(const QString &fileName, QString *error = nullptr) -> bool;

/// Set the display name and notes of one logo (creates the entry if needed).
/// Returns false and fills `error` on failure.
auto UpdateLogoMeta(const QString &fileName, const QString &name, const QString &notes,
                    QString *error = nullptr) -> bool;

/// Default client library folder from settings (GetPathClients).
auto ClientsDirectory() -> QString;

/// List the client library, sorted by name.
auto LoadClients() -> QList<ClientEntry>;

/// Persist the client library (`clients.xml`, atomic write). Returns false and
/// fills `error` on failure.
auto SaveClients(const QList<ClientEntry> &clients, QString *error = nullptr) -> bool;

/// Create a new empty client in the library. Returns its id, or an empty string
/// on failure (with `error` filled).
auto AddClient(QString *error = nullptr) -> QString;

/// Remove a client from the library by id. Returns false and fills `error`
/// on failure.
auto RemoveClient(const QString &id, QString *error = nullptr) -> bool;

/// Set the fields of one client (creates the entry if needed). Returns false and
/// fills `error` on failure.
auto UpdateClientMeta(const QString &id, const QString &name, const QString &mail,
                      const QString &phone, const QString &info, QString *error = nullptr) -> bool;

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
