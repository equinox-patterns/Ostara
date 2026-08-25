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
#include <QSaveFile>

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
