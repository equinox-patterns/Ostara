/************************************************************************
 **
 **  @file   ologolibrary.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The installation's logo library — one folder of SVG files, and their names.
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

#include "ologolibrary.h"

#include "vabstractapplication.h"
#include "vcommonsettings.h"

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QSaveFile>

#include <algorithm>

namespace
{
const QString kTagRoot = QStringLiteral("logos");
const QString kTagLogo = QStringLiteral("logo");
const QString kTagName = QStringLiteral("name");
const QString kTagNotes = QStringLiteral("notes");
const QString kAttrFile = QStringLiteral("file");

/// Read the metadata. A missing or malformed file costs the names, never the logos.
auto ReadMeta() -> QList<OLogoLibrary::Entry>
{
    QList<OLogoLibrary::Entry> entries;

    QFile file(OLogoLibrary::MetaPath());
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
    if (root.tagName() != kTagRoot)
    {
        return entries;
    }

    for (QDomElement el = root.firstChildElement(kTagLogo); !el.isNull();
         el = el.nextSiblingElement(kTagLogo))
    {
        OLogoLibrary::Entry entry;
        entry.fileName = el.attribute(kAttrFile);
        entry.name = el.firstChildElement(kTagName).text();
        entry.notes = el.firstChildElement(kTagNotes).text();
        if (!entry.fileName.isEmpty())
        {
            entries.append(entry);
        }
    }
    return entries;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OLogoLibrary::Directory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathLogos();
}

//---------------------------------------------------------------------------------------------------------------------

auto OLogoLibrary::MetaPath() -> QString
{
    return Directory() + QLatin1Char('/') + QStringLiteral("logos.xml");
}

//---------------------------------------------------------------------------------------------------------------------

auto OLogoLibrary::Load() -> QList<Entry>
{
    QDir dir(Directory());
    const QStringList files =
        dir.exists() ? dir.entryList({QStringLiteral("*.svg")}, QDir::Files, QDir::Name | QDir::IgnoreCase)
                     : QStringList();

    // Indexed by file name, because that is what the index is keyed on and what a template
    // stores. The folder leads: an entry whose file has gone is simply not reached.
    const QList<Entry> meta = ReadMeta();
    QHash<QString, Entry> byFile;
    for (const Entry &entry : meta)
    {
        byFile.insert(entry.fileName, entry);
    }

    QList<Entry> result;
    result.reserve(files.size());
    for (const QString &file : files)
    {
        Entry entry = byFile.value(file);
        entry.fileName = file;
        if (entry.name.isEmpty())
        {
            entry.name = QFileInfo(file).completeBaseName();
        }
        result.append(entry);
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------

auto OLogoLibrary::Save(const QList<Entry> &logos, QString *error) -> bool
{
    QDomDocument doc;
    QDomElement root = doc.createElement(kTagRoot);
    doc.appendChild(root);
    for (const Entry &entry : logos)
    {
        QDomElement el = doc.createElement(kTagLogo);
        el.setAttribute(kAttrFile, entry.fileName);

        QDomElement name = doc.createElement(kTagName);
        name.appendChild(doc.createTextNode(entry.name));
        el.appendChild(name);

        QDomElement notes = doc.createElement(kTagNotes);
        notes.appendChild(doc.createTextNode(entry.notes));
        el.appendChild(notes);

        root.appendChild(el);
    }

    QSaveFile file(MetaPath());
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

//---------------------------------------------------------------------------------------------------------------------

auto OLogoLibrary::Add(const QString &srcPath, QString *error) -> bool
{
    // The folder is the application's to keep, not the user's to remember — a library that
    // refused the first logo because its folder had been deleted would be a dead end.
    const QString dir = Directory();
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
    QList<Entry> entries = Load();
    for (const Entry &entry : entries)
    {
        if (entry.fileName == fileName)
        {
            return true;
        }
    }
    Entry entry;
    entry.fileName = fileName;
    entry.name = QFileInfo(fileName).completeBaseName();
    entries.append(entry);
    return Save(entries, error);
}

//---------------------------------------------------------------------------------------------------------------------

auto OLogoLibrary::Remove(const QString &fileName, QString *error) -> bool
{
    const QString safe = QFileInfo(fileName).fileName(); // strip any directory part
    const QString path = Directory() + QLatin1Char('/') + safe;
    if (QFile::exists(path) && !QFile::remove(path))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Cannot remove the logo file.");
        }
        return false;
    }

    QList<Entry> entries = Load();
    const qsizetype oldCount = entries.size();
    entries.erase(std::remove_if(entries.begin(), entries.end(),
                                 [&safe](const Entry &entry) { return entry.fileName == safe; }),
                  entries.end());
    if (entries.size() != oldCount)
    {
        return Save(entries, error);
    }
    return true;
}
