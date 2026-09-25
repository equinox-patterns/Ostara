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

/// One collection of the library: a stable id plus the year (sort key), a
/// free-form period and a description. All the data lives in a
/// `collections.xml` metadata file (no per-collection file).
struct CollectionEntry
{
    QString id;      ///< stable id, never changes (uuid)
    QString name;    ///< display name
    int year = 0;    ///< sort key — current year at creation
    QString period;  ///< free-form period (e.g. "Autumn Winter") or date
    QString description;  ///< free-form description
};

/// One annotation style of the library: a named style (the name is the stable
/// key the pieces reference) plus font, background and text colors. All the
/// data lives in `annotationstyles.xml`.
struct AnnotationStyleEntry
{
    QString name;            ///< stable key, referenced by the pieces
    QString fontFamily;      ///< font family; empty = default
    QString fontFace;        ///< face name; empty = the family's Regular
    int fontSize = 0;        ///< size in px; 0 = default
    QString backgroundColor; ///< hex "#RRGGBB"; empty = transparent
    QString textColor;       ///< hex; empty = black #000000
};

/// Default folder from settings (GetPathCartoucheTemplates).
auto DefaultDirectory() -> QString;

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

/// Default collection library folder from settings (GetPathCollections).
auto CollectionsDirectory() -> QString;

/// List the collection library, sorted by year (ascending or descending) then
/// by name (case-insensitive) within the same year.
auto LoadCollections(bool ascending) -> QList<CollectionEntry>;

/// Persist the collection library (`collections.xml`, atomic write). Returns
/// false and fills `error` on failure.
auto SaveCollections(const QList<CollectionEntry> &collections, QString *error = nullptr) -> bool;

/// Create a new empty collection in the library (year = current year). Returns
/// its id, or an empty string on failure (with `error` filled).
auto AddCollection(QString *error = nullptr) -> QString;

/// Remove a collection from the library by id. Returns false and fills
/// `error` on failure.
auto RemoveCollection(const QString &id, QString *error = nullptr) -> bool;

/// Set the fields of one collection (creates the entry if needed). Returns
/// false and fills `error` on failure.
auto UpdateCollectionMeta(const QString &id, const QString &name, int year, const QString &period,
                          const QString &description, QString *error = nullptr) -> bool;

/// Default annotation style library folder from settings (GetPathAnnotationStyles).
auto AnnotationStylesDirectory() -> QString;

/// List the annotation styles, sorted by name. On a first launch (missing
/// file) the built-in Default style is pre-installed.
auto LoadAnnotationStyles() -> QList<AnnotationStyleEntry>;

/// Persist the annotation styles (`annotationstyles.xml`, atomic write).
/// Returns false and fills `error` on failure.
auto SaveAnnotationStyles(const QList<AnnotationStyleEntry> &styles, QString *error = nullptr) -> bool;

/// Create a new empty annotation style in the library. Returns its name, or an
/// empty string on failure (with `error` filled).
auto AddAnnotationStyle(QString *error = nullptr) -> QString;

/// Remove an annotation style by name. Returns false and fills `error` on
/// failure.
auto RemoveAnnotationStyle(const QString &name, QString *error = nullptr) -> bool;

/// Set the fields of one annotation style (renames from `oldName` to `newName`
/// when they differ; creates the entry if needed). Returns false and fills
/// `error` on failure.
auto UpdateAnnotationStyleMeta(const QString &oldName, const QString &newName, const QString &fontFamily,
                               const QString &fontFace, int fontSize, const QString &backgroundColor,
                               const QString &textColor, QString *error = nullptr) -> bool;

/// Monotonic version bumped by every successful SaveAnnotationStyles — lets
/// callers cache the library and reload only after a write.
auto AnnotationStylesVersion() -> quint64;

/// List of `.oct` files in the given folder (absolute paths), alphabetical.
auto ScanDirectory(const QString &dirPath) -> QStringList;

/// Index of the template a combo should select, given the scanned base names:
/// the pattern's current template when present, otherwise the favourite from
/// settings, otherwise the first entry. Shared by the dialog step 1 and the
/// Details dock so the two dropdowns never drift apart.
auto PreferredTemplateIndex(const QStringList &baseNames, const QString &currentTemplate,
                            const QString &defaultName) -> int;

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
