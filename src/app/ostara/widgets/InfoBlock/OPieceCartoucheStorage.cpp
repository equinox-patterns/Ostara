/************************************************************************
 **
 **  @file   OPieceCartoucheStorage.cpp
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

#include "OPieceCartoucheStorage.h"

#include "core/ofontfaces.h"

#include "vabstractapplication.h"
#include "xml/vdomdocument.h"

#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QList>
#include <QDate>
#include <QSaveFile>
#include <QUuid>

#include <algorithm>

namespace
{
quint64 g_annotationStylesVersion{0};  // bumped by SaveAnnotationStyles (resolver cache)

/**
 * @brief VDomDocument subclass bound to the cartouche template XSD, so that
 * SaveDocument validates the serialized content before the atomic commit.
 */
class CartoucheTemplateDocument : public VDomDocument
{
public:
    explicit CartoucheTemplateDocument(QObject *parent = nullptr) : VDomDocument(parent)
    {
    }

    auto SaveSchema() const -> QString override
    {
        return QStringLiteral("://schema/cartouche_template/v1.0.0.xsd");
    }
};

/// Path of the client library metadata file.
QString ClientsMetaPath()
{
    return OPieceCartoucheStorage::ClientsDirectory() + QLatin1Char('/') + QStringLiteral("clients.xml");
}

/// Read the client library metadata (`clients.xml`). Missing or malformed file → empty list.
QList<OPieceCartoucheStorage::ClientEntry> ReadClientsMeta()
{
    QList<OPieceCartoucheStorage::ClientEntry> entries;
    QFile file(ClientsMetaPath());
    if (!file.open(QIODevice::ReadOnly))
    {
        return entries;
    }

    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        return entries;
    }

    const QDomElement root = doc.documentElement();
    if (root.tagName() != QStringLiteral("clients"))
    {
        return entries;
    }

    for (QDomElement el = root.firstChildElement(QStringLiteral("client")); !el.isNull();
         el = el.nextSiblingElement(QStringLiteral("client")))
    {
        OPieceCartoucheStorage::ClientEntry entry;
        entry.id = el.attribute(QStringLiteral("id"));
        entry.name = el.firstChildElement(QStringLiteral("name")).text();
        entry.mail = el.firstChildElement(QStringLiteral("mail")).text();
        entry.phone = el.firstChildElement(QStringLiteral("phone")).text();
        entry.info = el.firstChildElement(QStringLiteral("info")).text();
        if (!entry.id.isEmpty())
        {
            entries.append(entry);
        }
    }
    return entries;
}

/// Path of the collection library metadata file.
QString CollectionsMetaPath()
{
    return OPieceCartoucheStorage::CollectionsDirectory() + QLatin1Char('/') + QStringLiteral("collections.xml");
}

/// Path of the annotation styles metadata file.
QString AnnotationStylesMetaPath()
{
    return OPieceCartoucheStorage::AnnotationStylesDirectory() + QLatin1Char('/') + QStringLiteral("annotationstyles.xml");
}

/// Read the collection library metadata (`collections.xml`). Missing or
/// malformed file → empty list.
QList<OPieceCartoucheStorage::CollectionEntry> ReadCollectionsMeta()
{
    QList<OPieceCartoucheStorage::CollectionEntry> entries;
    QFile file(CollectionsMetaPath());
    if (!file.open(QIODevice::ReadOnly))
    {
        return entries;
    }

    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        return entries;
    }

    const QDomElement root = doc.documentElement();
    if (root.tagName() != QStringLiteral("collections"))
    {
        return entries;
    }

    for (QDomElement el = root.firstChildElement(QStringLiteral("collection")); !el.isNull();
         el = el.nextSiblingElement(QStringLiteral("collection")))
    {
        OPieceCartoucheStorage::CollectionEntry entry;
        entry.id = el.attribute(QStringLiteral("id"));
        entry.name = el.firstChildElement(QStringLiteral("name")).text();
        entry.year = el.firstChildElement(QStringLiteral("year")).text().toInt();
        entry.period = el.firstChildElement(QStringLiteral("period")).text();
        entry.description = el.firstChildElement(QStringLiteral("description")).text();
        if (!entry.id.isEmpty())
        {
            entries.append(entry);
        }
    }
    return entries;
}

/// Read the annotation styles metadata (`annotationstyles.xml`). Missing or
/// malformed file → empty list.
QList<OPieceCartoucheStorage::AnnotationStyleEntry> ReadAnnotationStylesMeta()
{
    QList<OPieceCartoucheStorage::AnnotationStyleEntry> entries;
    QFile file(AnnotationStylesMetaPath());
    if (!file.open(QIODevice::ReadOnly))
    {
        return entries;
    }

    QDomDocument doc;
    if (!doc.setContent(&file))
    {
        return entries;
    }

    const QDomElement root = doc.documentElement();
    if (root.tagName() != QStringLiteral("styles"))
    {
        return entries;
    }

    for (QDomElement el = root.firstChildElement(QStringLiteral("style")); !el.isNull();
         el = el.nextSiblingElement(QStringLiteral("style")))
    {
        OPieceCartoucheStorage::AnnotationStyleEntry entry;
        entry.name = el.attribute(QStringLiteral("name"));
        entry.fontFamily = el.firstChildElement(QStringLiteral("fontFamily")).text();
        entry.fontFace = el.firstChildElement(QStringLiteral("fontFace")).text();
        if (entry.fontFace.isEmpty())
        {
            // ⚠️ **The one migration, here and nowhere else.** A library written before the faces were
            // named holds a weight; resolving it once, at read time, leaves every reader and every
            // writer with a single representation — a face name — and loses nothing of the old number's
            // meaning, since it is the very face that number drew (see @ref FontFaceFromWeight).
            entry.fontFace = FontFaceFromWeight(
                entry.fontFamily, el.firstChildElement(QStringLiteral("fontWeight")).text().toInt());
        }
        entry.fontSize = el.firstChildElement(QStringLiteral("fontSize")).text().toInt();
        entry.backgroundColor = el.firstChildElement(QStringLiteral("backgroundColor")).text();
        entry.textColor = el.firstChildElement(QStringLiteral("textColor")).text();
        if (!entry.name.isEmpty())
        {
            entries.append(entry);
        }
    }
    return entries;
}
}  // namespace

namespace OPieceCartoucheStorage
{
auto DefaultDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathCartoucheTemplates();
}

auto ClientsDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathClients();
}

auto CollectionsDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathCollections();
}

auto AnnotationStylesDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathAnnotationStyles();
}

auto AnnotationStylesVersion() -> quint64
{
    return g_annotationStylesVersion;
}


auto LoadClients() -> QList<ClientEntry>
{
    QList<ClientEntry> entries = ReadClientsMeta();
    std::sort(entries.begin(), entries.end(),
              [](const ClientEntry &a, const ClientEntry &b) {
                  return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
              });
    return entries;
}

auto SaveClients(const QList<ClientEntry> &clients, QString *error) -> bool
{
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("clients"));
    doc.appendChild(root);
    for (const ClientEntry &entry : clients)
    {
        QDomElement el = doc.createElement(QStringLiteral("client"));
        el.setAttribute(QStringLiteral("id"), entry.id);

        QDomElement name = doc.createElement(QStringLiteral("name"));
        name.appendChild(doc.createTextNode(entry.name));
        el.appendChild(name);

        QDomElement mail = doc.createElement(QStringLiteral("mail"));
        mail.appendChild(doc.createTextNode(entry.mail));
        el.appendChild(mail);

        QDomElement phone = doc.createElement(QStringLiteral("phone"));
        phone.appendChild(doc.createTextNode(entry.phone));
        el.appendChild(phone);

        QDomElement info = doc.createElement(QStringLiteral("info"));
        info.appendChild(doc.createTextNode(entry.info));
        el.appendChild(info);

        root.appendChild(el);
    }

    QSaveFile file(ClientsMetaPath());
    if (!file.open(QIODevice::WriteOnly))
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }
    file.write(doc.toByteArray(2));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

auto AddClient(QString *error) -> QString
{
    const QString dir = ClientsDirectory();
    if (!QDir().mkpath(dir))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Cannot create the clients folder.");
        }
        return {};
    }

    ClientEntry entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QList<ClientEntry> entries = LoadClients();
    entries.append(entry);
    if (!SaveClients(entries, error))
    {
        return {};
    }
    return entry.id;
}

auto RemoveClient(const QString &id, QString *error) -> bool
{
    QList<ClientEntry> entries = LoadClients();
    const qsizetype oldCount = entries.size();
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [&id](const ClientEntry &entry) { return entry.id == id; }),
                  entries.end());
    if (entries.size() != oldCount)
    {
        return SaveClients(entries, error);
    }
    return true;
}

auto UpdateClientMeta(const QString &id, const QString &name, const QString &mail, const QString &phone,
                      const QString &info, QString *error) -> bool
{
    QList<ClientEntry> entries = LoadClients();
    bool updated = false;
    for (ClientEntry &entry : entries)
    {
        if (entry.id == id)
        {
            entry.name = name;
            entry.mail = mail;
            entry.phone = phone;
            entry.info = info;
            updated = true;
            break;
        }
    }
    if (!updated)
    {
        ClientEntry entry;
        entry.id = id;
        entry.name = name;
        entry.mail = mail;
        entry.phone = phone;
        entry.info = info;
        entries.append(entry);
    }
    return SaveClients(entries, error);
}

auto LoadCollections(bool ascending) -> QList<CollectionEntry>
{
    QList<CollectionEntry> entries = ReadCollectionsMeta();
    std::sort(entries.begin(), entries.end(),
              [ascending](const CollectionEntry &a, const CollectionEntry &b) {
                  if (a.year != b.year)
                  {
                      return ascending ? a.year < b.year : a.year > b.year;
                  }
                  return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
              });
    return entries;
}

auto SaveCollections(const QList<CollectionEntry> &collections, QString *error) -> bool
{
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("collections"));
    doc.appendChild(root);
    for (const CollectionEntry &entry : collections)
    {
        QDomElement el = doc.createElement(QStringLiteral("collection"));
        el.setAttribute(QStringLiteral("id"), entry.id);

        QDomElement name = doc.createElement(QStringLiteral("name"));
        name.appendChild(doc.createTextNode(entry.name));
        el.appendChild(name);

        QDomElement year = doc.createElement(QStringLiteral("year"));
        year.appendChild(doc.createTextNode(QString::number(entry.year)));
        el.appendChild(year);

        QDomElement period = doc.createElement(QStringLiteral("period"));
        period.appendChild(doc.createTextNode(entry.period));
        el.appendChild(period);

        QDomElement description = doc.createElement(QStringLiteral("description"));
        description.appendChild(doc.createTextNode(entry.description));
        el.appendChild(description);

        root.appendChild(el);
    }

    QSaveFile file(CollectionsMetaPath());
    if (!file.open(QIODevice::WriteOnly))
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }
    file.write(doc.toByteArray(2));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

auto AddCollection(QString *error) -> QString
{
    const QString dir = CollectionsDirectory();
    if (!QDir().mkpath(dir))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Cannot create the collections folder.");
        }
        return {};
    }

    CollectionEntry entry;
    entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry.year = QDate::currentDate().year();
    QList<CollectionEntry> entries = LoadCollections(false);
    entries.append(entry);
    if (!SaveCollections(entries, error))
    {
        return {};
    }
    return entry.id;
}

auto RemoveCollection(const QString &id, QString *error) -> bool
{
    QList<CollectionEntry> entries = LoadCollections(false);
    const qsizetype oldCount = entries.size();
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [&id](const CollectionEntry &entry) { return entry.id == id; }),
                  entries.end());
    if (entries.size() != oldCount)
    {
        return SaveCollections(entries, error);
    }
    return true;
}

auto UpdateCollectionMeta(const QString &id, const QString &name, int year, const QString &period,
                          const QString &description, QString *error) -> bool
{
    QList<CollectionEntry> entries = LoadCollections(false);
    bool updated = false;
    for (CollectionEntry &entry : entries)
    {
        if (entry.id == id)
        {
            entry.name = name;
            entry.year = year;
            entry.period = period;
            entry.description = description;
            updated = true;
            break;
        }
    }
    if (!updated)
    {
        CollectionEntry entry;
        entry.id = id;
        entry.name = name;
        entry.year = year;
        entry.period = period;
        entry.description = description;
        entries.append(entry);
    }
    return SaveCollections(entries, error);
}

auto LoadAnnotationStyles() -> QList<AnnotationStyleEntry>
{
    QList<AnnotationStyleEntry> entries = ReadAnnotationStylesMeta();
    if (entries.isEmpty() && !QFile::exists(AnnotationStylesMetaPath()))
    {
        // First launch: the library starts with the built-in Default style so the
        // combo and the render always have their fallback key.
        AnnotationStyleEntry def;
        def.name = QStringLiteral("Default");
        entries.append(def);
        QString err;
        SaveAnnotationStyles(entries, &err);
    }
    std::sort(entries.begin(), entries.end(),
              [](const AnnotationStyleEntry &a, const AnnotationStyleEntry &b) {
                  return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
              });
    return entries;
}

auto SaveAnnotationStyles(const QList<AnnotationStyleEntry> &styles, QString *error) -> bool
{
    QList<AnnotationStyleEntry> out = styles;

    // First save (missing file): pre-install the built-in Default style so the
    // library is never written without its fallback key.
    const bool missingFile = !QFile::exists(AnnotationStylesMetaPath());
    const bool hasDefault =
        std::any_of(out.cbegin(), out.cend(),
                    [](const AnnotationStyleEntry &entry) { return entry.name == QStringLiteral("Default"); });
    if (missingFile && !hasDefault)
    {
        AnnotationStyleEntry def;
        def.name = QStringLiteral("Default");
        out.prepend(def);
    }

    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("styles"));
    doc.appendChild(root);
    for (const AnnotationStyleEntry &entry : out)
    {
        QDomElement el = doc.createElement(QStringLiteral("style"));
        el.setAttribute(QStringLiteral("name"), entry.name);

        // Omission of the null fields (default font / transparent background /
        // black text) — the read defaults match these predicates.
        if (!entry.fontFamily.isEmpty())
        {
            QDomElement fontFamily = doc.createElement(QStringLiteral("fontFamily"));
            fontFamily.appendChild(doc.createTextNode(entry.fontFamily));
            el.appendChild(fontFamily);
        }
        if (!entry.fontFace.isEmpty())
        {
            QDomElement fontFace = doc.createElement(QStringLiteral("fontFace"));
            fontFace.appendChild(doc.createTextNode(entry.fontFace));
            el.appendChild(fontFace);
        }
        if (entry.fontSize != 0)
        {
            QDomElement fontSize = doc.createElement(QStringLiteral("fontSize"));
            fontSize.appendChild(doc.createTextNode(QString::number(entry.fontSize)));
            el.appendChild(fontSize);
        }
        if (!entry.backgroundColor.isEmpty())
        {
            QDomElement backgroundColor = doc.createElement(QStringLiteral("backgroundColor"));
            backgroundColor.appendChild(doc.createTextNode(entry.backgroundColor));
            el.appendChild(backgroundColor);
        }
        // textColor is omitted when black (#000000) — the render default.
        const QString textColor = entry.textColor.isEmpty() ? QStringLiteral("#000000") : entry.textColor;
        if (textColor != QStringLiteral("#000000"))
        {
            QDomElement text = doc.createElement(QStringLiteral("textColor"));
            text.appendChild(doc.createTextNode(textColor));
            el.appendChild(text);
        }

        root.appendChild(el);
    }

    QSaveFile file(AnnotationStylesMetaPath());
    if (!file.open(QIODevice::WriteOnly))
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }
    file.write(doc.toByteArray(2));
    if (!file.commit())
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }
    ++g_annotationStylesVersion;  // invalidate resolver caches
    return true;
}

auto AddAnnotationStyle(QString *error) -> QString
{
    const QString dir = AnnotationStylesDirectory();
    if (!QDir().mkpath(dir))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Cannot create the annotation styles folder.");
        }
        return {};
    }

    QList<AnnotationStyleEntry> entries = LoadAnnotationStyles();
    const QString base = QStringLiteral("New style");
    QString name = base;
    int suffix = 2;
    const auto taken = [&entries](const QString &n) {
        return std::any_of(entries.cbegin(), entries.cend(),
                           [&n](const AnnotationStyleEntry &entry) { return entry.name == n; });
    };
    while (taken(name))
    {
        name = base + QLatin1Char(' ') + QString::number(suffix++);
    }

    AnnotationStyleEntry entry;
    entry.name = name;
    entries.append(entry);
    if (!SaveAnnotationStyles(entries, error))
    {
        return {};
    }
    return name;
}

auto RemoveAnnotationStyle(const QString &name, QString *error) -> bool
{
    QList<AnnotationStyleEntry> entries = LoadAnnotationStyles();
    const qsizetype oldCount = entries.size();
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [&name](const AnnotationStyleEntry &entry) { return entry.name == name; }),
                  entries.end());
    if (entries.size() != oldCount)
    {
        return SaveAnnotationStyles(entries, error);
    }
    return true;
}

auto UpdateAnnotationStyleMeta(const QString &oldName, const QString &newName, const QString &fontFamily,
                               const QString &fontFace, int fontSize, const QString &backgroundColor,
                               const QString &textColor, QString *error) -> bool
{
    QList<AnnotationStyleEntry> entries = LoadAnnotationStyles();
    if (oldName != newName)
    {
        // A rename must not collide with an existing key.
        entries.erase(std::remove_if(entries.begin(), entries.end(),
                                     [&newName](const AnnotationStyleEntry &entry) { return entry.name == newName; }),
                      entries.end());
    }

    bool updated = false;
    for (AnnotationStyleEntry &entry : entries)
    {
        if (entry.name == oldName)
        {
            entry.name = newName;
            entry.fontFamily = fontFamily;
            entry.fontFace = fontFace;
            entry.fontSize = fontSize;
            entry.backgroundColor = backgroundColor;
            entry.textColor = textColor;
            updated = true;
            break;
        }
    }
    if (!updated)
    {
        AnnotationStyleEntry entry;
        entry.name = newName;
        entry.fontFamily = fontFamily;
        entry.fontFace = fontFace;
        entry.fontSize = fontSize;
        entry.backgroundColor = backgroundColor;
        entry.textColor = textColor;
        entries.append(entry);
    }
    return SaveAnnotationStyles(entries, error);
}

auto ScanDirectory(const QString &dirPath) -> QStringList
{
    QDir dir(dirPath);
    if (!dir.exists())
    {
        return {};
    }

    const QStringList files =
        dir.entryList({QStringLiteral("*.oct")}, QDir::Files, QDir::Name | QDir::IgnoreCase);

    QStringList paths;
    paths.reserve(files.size());
    for (const QString &file : files)
    {
        paths.append(dir.absoluteFilePath(file));
    }
    return paths;
}

auto PreferredTemplateIndex(const QStringList &baseNames, const QString &currentTemplate,
                            const QString &defaultName) -> int
{
    const qsizetype current = baseNames.indexOf(currentTemplate);
    if (current >= 0)
    {
        return static_cast<int>(current);
    }
    const qsizetype fallback = baseNames.indexOf(defaultName);
    return fallback >= 0 ? static_cast<int>(fallback) : 0;
}

auto Load(const QString &filePath, OPieceCartoucheTemplate *tpl, QString *error) -> bool
{
    if (tpl == nullptr)
    {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        if (error != nullptr)
        {
            *error = file.errorString();
        }
        return false;
    }

    CartoucheTemplateDocument doc;
    const QDomDocument::ParseResult result = doc.setContent(&file);
    if (!result)
    {
        if (error != nullptr)
        {
            *error = result.errorMessage;
        }
        return false;
    }

    if (!tpl->LoadFromXml(doc.documentElement()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Not a cartouche template file.");
        }
        return false;
    }
    return true;
}

auto Save(const OPieceCartoucheTemplate &tpl, const QString &filePath, QString *error) -> bool
{
    // Ensure the destination folder exists (a typed path or a deleted folder
    // must not fail with a raw "No such file or directory").
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    CartoucheTemplateDocument doc;
    doc.appendChild(tpl.ToXml(doc));

    QString err;
    const bool ok = doc.SaveDocument(filePath, err);
    if (!ok && error != nullptr)
    {
        *error = err;
    }
    return ok;
}

void EnsureDefaultTemplate()
{
    const QString dir = DefaultDirectory();
    if (!ScanDirectory(dir).isEmpty())
    {
        return;
    }

    if (!QDir().mkpath(dir))
    {
        qWarning() << "Cannot create cartouche templates folder:" << dir;
        return;
    }

    OPieceCartoucheTemplate tpl(QStringLiteral("Default"));
    tpl.AddSection(QStringLiteral("patternName"));
    tpl.AddSection(QStringLiteral("pieceNumber"));
    tpl.AddSection(QStringLiteral("pieceName"));
    tpl.AddSection(QStringLiteral("cut"));

    const QString path = dir + QLatin1Char('/') + QStringLiteral("Default.oct");
    QString error;
    if (!Save(tpl, path, &error))
    {
        qWarning() << "Cannot create default cartouche template:" << error;
    }
}

void InvalidateAnnotationStyles()
{
    ++g_annotationStylesVersion;
}
}  // namespace OPieceCartoucheStorage
