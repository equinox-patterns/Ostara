/************************************************************************
 **
 **  @file   oexporttemplatestorage.cpp
 **
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  The template library: where it lives, and how a file is written and read back.
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

#include "oexporttemplatestorage.h"

#include "vabstractapplication.h"
#include "vcommonsettings.h"
#include "xml/vdomdocument.h"

#include <QDebug>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QSet>

namespace
{
/**
 * @brief VDomDocument subclass bound to the assembly-mark template XSD, so that SaveDocument
 * validates the serialized content before the atomic commit.
 */
class AssemblyMarkTemplateDocument : public VDomDocument
{
public:
    explicit AssemblyMarkTemplateDocument(QObject *parent = nullptr) : VDomDocument(parent)
    {
    }

    auto SaveSchema() const -> QString override
    {
        return QStringLiteral("://schema/assemblymark_template/v1.0.0.xsd");
    }
};

/** The characters that either no file system accepts or that would send the file elsewhere. */
const QString kForbidden = QStringLiteral("/\\:*?\"<>|");

/** What a name made only of dropped characters falls back to, so that a file still has one. */
const QString kFallbackName = QStringLiteral("template");

/** The opacity the shipped template gives every one of its lines and marks. */
constexpr int kShippedOpacity = 50;

/**
 * @brief @p name as a file name — what cannot be kept is dropped, not refused.
 *
 * Sophie's rule for the end user is one click and no technical problem. A message saying a name
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
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateStorage::Extension() -> QString
{
    return QStringLiteral("oamt");
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateStorage::DefaultDirectory() -> QString
{
    return VAbstractApplication::VApp()->Settings()->GetPathPdfTemplates();
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateStorage::ScanDirectory(const QString &dirPath) -> QStringList
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

auto OExportTemplateStorage::PathForName(const QString &name) -> QString
{
    return DefaultDirectory() + QLatin1Char('/') + SanitisedFileName(name) + QLatin1Char('.')
           + Extension();
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateStorage::UniqueName(const QString &base) -> QString
{
    QSet<QString> taken;
    for (const QString &path : ScanDirectory(DefaultDirectory()))
    {
        taken.insert(QFileInfo(path).completeBaseName());
    }

    if (!taken.contains(base))
    {
        return base;
    }

    // Suffixed from two, as the annotation styles are: the first is the bare name.
    int suffix = 2;
    QString candidate = base;
    do
    {
        candidate = base + QLatin1Char(' ') + QString::number(suffix++);
    } while (taken.contains(candidate));

    return candidate;
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateStorage::Save(const OExportTemplate &tpl, const QString &filePath,
                                  QString *error) -> bool
{
    // A typed name, or a folder the user deleted, must not fail with a raw "No such file or
    // directory" — the folder is the application's to keep, not the user's to remember.
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    AssemblyMarkTemplateDocument doc;
    doc.appendChild(tpl.ToXml(doc));

    QString message;
    const bool ok = doc.SaveDocument(filePath, message);
    if (!ok && error != nullptr)
    {
        *error = message;
    }
    return ok;
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateStorage::Load(const QString &filePath, OExportTemplate *tpl, QString *error)
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

    AssemblyMarkTemplateDocument doc;
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
    OExportTemplate loaded;
    if (!loaded.LoadFromXml(doc.documentElement()))
    {
        if (error != nullptr)
        {
            *error = QStringLiteral("Not an assembly mark template file.");
        }
        return false;
    }

    *tpl = loaded;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateStorage::DefaultTemplateName() -> QString
{
    // Never translated: it is a file's base name, and the key the favourite points at. A name
    // that changed with the interface language would lose the file it names.
    return QStringLiteral("Default");
}

//---------------------------------------------------------------------------------------------------------------------

auto OExportTemplateStorage::ShippedTemplate() -> OExportTemplate
{
    OExportTemplate tpl;
    tpl.name = DefaultTemplateName();

    // Everything below is pinned rather than left to the blocks' own defaults: this is a
    // decision about what the software starts with, and it must not move the day one of those
    // defaults is revised.

    tpl.frame.dashed = true;
    tpl.frame.frameOpacity = kShippedOpacity;
    tpl.frame.scissorsHorizontal = true;
    tpl.frame.scissorsVertical = true;
    tpl.frame.scissorsOpacity = kShippedOpacity;

    tpl.marks.shape = OExportMarkShape::Circle;
    tpl.marks.sizeMm = 40.0;
    tpl.marks.opacity = kShippedOpacity;
    tpl.marks.onCorner = true;

    tpl.pagination.form = OExportPaginationForm::NumberAndLetter;
    tpl.pagination.centre.enabled = true;
    tpl.pagination.centre.style.opacity = kShippedOpacity;
    tpl.pagination.corner.enabled = true;
    tpl.pagination.corner.style.opacity = kShippedOpacity;

    return tpl;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateStorage::EnsureDefaultTemplate()
{
    const QString directory = DefaultDirectory();

    // Any template at all means the library is the user's, and nothing here is owed to it.
    if (!ScanDirectory(directory).isEmpty())
    {
        return;
    }

    if (!QDir().mkpath(directory))
    {
        qWarning() << "Could not create the template folder" << directory;
        return;
    }

    QString error;
    if (!Save(ShippedTemplate(), PathForName(DefaultTemplateName()), &error))
    {
        qWarning() << "Could not write the shipped template:" << error;
    }
}
