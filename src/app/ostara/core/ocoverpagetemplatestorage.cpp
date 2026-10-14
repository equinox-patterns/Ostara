/************************************************************************
 **
 **  @file   ocoverpagetemplatestorage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The library of cover page templates, one `.ocpt` file each.
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

#include "ocoverpagetemplatestorage.h"

#include "vabstractapplication.h"
#include "vcommonsettings.h"
#include "xml/vdomdocument.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QSet>

namespace
{
/**
 * @brief VDomDocument subclass bound to the cover-page template XSD, so that SaveDocument
 * validates the serialized content before the atomic commit.
 */
class CoverPageTemplateDocument : public VDomDocument
{
public:
    explicit CoverPageTemplateDocument(QObject *parent = nullptr) : VDomDocument(parent)
    {
    }

    auto SaveSchema() const -> QString override
    {
        return QStringLiteral("://schema/coverpage_template/v1.0.0.xsd");
    }
};

/** The characters that either no file system accepts or that would send the file elsewhere. */
const QString kForbidden = QStringLiteral("/\\:*?\"<>|");

/** What a name made only of dropped characters falls back to, so that a file still has one. */
const QString kFallbackName = QStringLiteral("template");

/**
 * @brief @p name as a file name — what cannot be kept is dropped, not refused.
 *
 * The rule for the end user is one click and no technical problem. A message saying a name
 * contains an illegal character names a problem the user cannot see and did not make; taking the
 * character out does what she meant, which was to call a template something.
 */
auto SanitisedFileName(const QString &name) -> QString
{
    QString cleaned;
    cleaned.reserve(name.size());
    for (const QChar &character : name)
    {
        if (kForbidden.contains(character) || character < QChar(0x20))
        {
            continue;
        }
        cleaned.append(character);
    }

    cleaned = cleaned.trimmed();
    return cleaned.isEmpty() ? kFallbackName : cleaned;
}

/**
 * @brief Writes @p tpl to @p filePath, folder and all — the last step of a save, and the only one
 *        that touches the disk.
 */
auto WriteTemplate(const OCoverPageTemplate &tpl, const QString &filePath, QString *error) -> bool
{
    // A typed name, or a folder the user deleted, must not fail with a raw "No such file or
    // directory" — the folder is the application's to keep, not the user's to remember.
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    CoverPageTemplateDocument doc;
    doc.appendChild(tpl.ToXml(doc));

    QString message;
    const bool ok = doc.SaveDocument(filePath, message);
    if (!ok && error != nullptr)
    {
        *error = message;
    }
    return ok;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplateStorage::Extension() -> QString
{
    return QStringLiteral("ocpt");
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplateStorage::DefaultDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathCoverPageTemplates();
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplateStorage::ScanDirectory(const QString &dirPath) -> QStringList
{
    QDir dir(dirPath);
    if (!dir.exists())
    {
        return {};
    }

    const QStringList files =
        dir.entryList({QStringLiteral("*.") + Extension()}, QDir::Files | QDir::Readable, QDir::Name);
    QStringList paths;
    paths.reserve(files.size());
    for (const QString &file : files)
    {
        paths.append(dir.absoluteFilePath(file));
    }
    return paths;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplateStorage::PathForName(const QString &name) -> QString
{
    return DefaultDirectory() + QLatin1Char('/') + SanitisedFileName(name) + QLatin1Char('.')
           + Extension();
}

//---------------------------------------------------------------------------------------------------------------------
auto OCoverPageTemplateStorage::SaveAs(const OCoverPageTemplate &tpl, const QString &name,
                                       const QString &previousPath) -> SaveResult
{
    SaveResult result;

    const QString target = PathForName(name);
    result.name = QFileInfo(target).completeBaseName();

    // Another template already answers to that name, and this is not the file it is already in.
    if (target != previousPath && QFileInfo::exists(target))
    {
        result.outcome = SaveOutcome::NameTaken;
        return result;
    }

    // The name the file will carry, not the one that was typed: the file is the identity, and an
    // editor showing the typed version would show a name no file answers to. What cannot be kept of
    // a name is dropped by PathForName, silently — see the rule there.
    OCoverPageTemplate written = tpl;
    written.name = result.name;

    QString error;
    if (!WriteTemplate(written, target, &error))
    {
        result.outcome = SaveOutcome::Failed;
        result.error = error;
        return result;
    }

    // Written first, moved after: the two are never both missing, whatever happens in between.
    if (!previousPath.isEmpty() && previousPath != target)
    {
        QFile::remove(previousPath);
    }

    result.outcome = SaveOutcome::Saved;
    result.filePath = target;
    return result;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplateStorage::Load(const QString &filePath, OCoverPageTemplate *tpl, QString *error)
    -> bool
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

    CoverPageTemplateDocument doc;
    const QDomDocument::ParseResult result = doc.setContent(&file);
    if (!result)
    {
        if (error != nullptr)
        {
            *error = result.errorMessage;
        }
        return false;
    }

    // Read into a copy: a file that turns out not to be a template leaves the caller's template
    // exactly as it was, rather than half-overwritten by a failed load.
    OCoverPageTemplate loaded;
    if (!loaded.LoadFromXml(doc.documentElement()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Not a cover page template file.");
        }
        return false;
    }

    *tpl = loaded;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCoverPageTemplateStorage::DefaultTemplateName() -> QString
{
    return QStringLiteral("Default");
}

//---------------------------------------------------------------------------------------------------------------------
auto OCoverPageTemplateStorage::Resolve(const QString &name, OCoverPageTemplate *tpl) -> bool
{
    if (tpl == nullptr || name.isEmpty())
    {
        return false;
    }

    // **One file, and nothing behind it.** A cover page the user has renamed or deleted is a
    // cover page that is not there any more, and drawing another in its place would print a page
    // the generation never asked for — with nothing on screen to say so. So there is no second
    // candidate here, and the chain that picks a name (the favourite, then Default) belongs to
    // the dialog, which is where choosing happens.
    //
    // A name that IS `Default` is read like any other: it simply happens to name nothing until
    // somebody creates such a template, since this library seeds none.
    return Load(PathForName(name), tpl);
}
