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

/// Path of the logo library metadata file (next to the SVG files).
QString LogosMetaPath()
{
    return OPieceCartoucheStorage::LogosDirectory() + QLatin1Char('/') + QStringLiteral("logos.xml");
}

/// Read the logo library metadata (`logos.xml`). Missing or malformed file → empty list.
QList<OPieceCartoucheStorage::LogoEntry> ReadLogosMeta()
{
    QList<OPieceCartoucheStorage::LogoEntry> entries;
    QFile file(LogosMetaPath());
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
    if (root.tagName() != QStringLiteral("logos"))
    {
        return entries;
    }

    for (QDomElement el = root.firstChildElement(QStringLiteral("logo")); !el.isNull();
         el = el.nextSiblingElement(QStringLiteral("logo")))
    {
        OPieceCartoucheStorage::LogoEntry entry;
        entry.fileName = el.attribute(QStringLiteral("file"));
        entry.name = el.firstChildElement(QStringLiteral("name")).text();
        entry.notes = el.firstChildElement(QStringLiteral("notes")).text();
        if (!entry.fileName.isEmpty())
        {
            entries.append(entry);
        }
    }
    return entries;
}

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
}  // namespace

namespace OPieceCartoucheStorage
{
auto DefaultDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathCartoucheTemplates();
}

auto LogosDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathLogos();
}

auto ClientsDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathClients();
}

auto CollectionsDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathCollections();
}

auto LoadLogos() -> QList<LogoEntry>
{
    QDir dir(LogosDirectory());
    const QStringList files =
        dir.exists() ? dir.entryList({QStringLiteral("*.svg")}, QDir::Files, QDir::Name | QDir::IgnoreCase)
                     : QStringList();

    const QList<LogoEntry> meta = ReadLogosMeta();
    QHash<QString, LogoEntry> byFile;
    for (const LogoEntry &entry : meta)
    {
        byFile.insert(entry.fileName, entry);
    }

    QList<LogoEntry> result;
    result.reserve(files.size());
    for (const QString &file : files)
    {
        LogoEntry entry = byFile.value(file);
        entry.fileName = file;
        if (entry.name.isEmpty())
        {
            entry.name = QFileInfo(file).completeBaseName();
        }
        result.append(entry);
    }
    return result;
}

auto SaveLogos(const QList<LogoEntry> &logos, QString *error) -> bool
{
    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("logos"));
    doc.appendChild(root);
    for (const LogoEntry &entry : logos)
    {
        QDomElement el = doc.createElement(QStringLiteral("logo"));
        el.setAttribute(QStringLiteral("file"), entry.fileName);

        QDomElement name = doc.createElement(QStringLiteral("name"));
        name.appendChild(doc.createTextNode(entry.name));
        el.appendChild(name);

        QDomElement notes = doc.createElement(QStringLiteral("notes"));
        notes.appendChild(doc.createTextNode(entry.notes));
        el.appendChild(notes);

        root.appendChild(el);
    }

    QSaveFile file(LogosMetaPath());
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

auto AddLogo(const QString &srcPath, QString *error) -> bool
{
    const QString dir = LogosDirectory();
    if (!QDir().mkpath(dir))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Cannot create the logos folder.");
        }
        return false;
    }

    const QString fileName = QFileInfo(srcPath).fileName();
    const QString dest = dir + QLatin1Char('/') + fileName;
    if (QFileInfo(srcPath).absoluteFilePath() != QFileInfo(dest).absoluteFilePath())
    {
        if (QFile::exists(dest) && !QFile::remove(dest))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("Cannot replace the existing logo file.");
            }
            return false;
        }
        if (!QFile::copy(srcPath, dest))
        {
            if (error != nullptr)
            {
                *error = QStringLiteral("Cannot copy the logo file.");
            }
            return false;
        }
    }

    // Ensure a metadata entry exists; re-adding an existing file keeps its
    // current name/notes.
    QList<LogoEntry> entries = LoadLogos();
    for (const LogoEntry &entry : entries)
    {
        if (entry.fileName == fileName)
        {
            return true;
        }
    }
    LogoEntry entry;
    entry.fileName = fileName;
    entry.name = QFileInfo(fileName).completeBaseName();
    entries.append(entry);
    return SaveLogos(entries, error);
}

auto RemoveLogo(const QString &fileName, QString *error) -> bool
{
    const QString safe = QFileInfo(fileName).fileName();  // strip any directory part
    const QString path = LogosDirectory() + QLatin1Char('/') + safe;
    if (QFile::exists(path) && !QFile::remove(path))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Cannot remove the logo file.");
        }
        return false;
    }

    QList<LogoEntry> entries = LoadLogos();
    const qsizetype oldCount = entries.size();
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [&safe](const LogoEntry &entry) { return entry.fileName == safe; }),
                  entries.end());
    if (entries.size() != oldCount)
    {
        return SaveLogos(entries, error);
    }
    return true;
}

auto UpdateLogoMeta(const QString &fileName, const QString &name, const QString &notes, QString *error) -> bool
{
    QList<LogoEntry> entries = LoadLogos();
    bool updated = false;
    for (LogoEntry &entry : entries)
    {
        if (entry.fileName == fileName)
        {
            entry.name = name;
            entry.notes = notes;
            updated = true;
            break;
        }
    }
    if (!updated)
    {
        LogoEntry entry;
        entry.fileName = fileName;
        entry.name = name;
        entry.notes = notes;
        entries.append(entry);
    }
    return SaveLogos(entries, error);
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
}  // namespace OPieceCartoucheStorage
