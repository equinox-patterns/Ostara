/************************************************************************
 **
 **  @file   oexporttemplatestorage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  Where templates are saved, how they are read and written, and the one the
 **          installation starts from.
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

#ifndef OEXPORTTEMPLATESTORAGE_H
#define OEXPORTTEMPLATESTORAGE_H

#include "oexporttemplate.h"

#include <QString>
#include <QStringList>

/**
 * @brief The library of saved templates: one `.oamt` file per template, in a per-user folder.
 *
 * The cartouche's arrangement, one format further on: files under a folder of the user's home,
 * enumerated by scanning it, **the file's base name being the identity** — the list shows it, the
 * favourite names it, and nothing else identifies a template.
 *
 * A file is XML validated against its schema and committed atomically, so a template is either
 * the one that was saved or the one that was there before, never half of each.
 */
namespace OExportTemplateStorage
{
/** @brief The extension of a saved template, without its dot. */
auto Extension() -> QString;

/** @brief The folder templates are saved in, from settings (GetPathPdfTemplates). */
auto DefaultDirectory() -> QString;

/** @brief Every template in @p dirPath, as absolute paths, sorted by name — empty if there is none. */
auto ScanDirectory(const QString &dirPath) -> QStringList;

/**
 * @brief The path a template called @p name is saved at, in the default folder.
 *
 * The name is made into a file name rather than refused: a user who types a slash is naming a
 * template, not filling a form, and an error message about illegal characters would be a
 * technical problem handed to someone who has no way to solve it. What cannot be kept is dropped
 * silently.
 */
auto PathForName(const QString &name) -> QString;


/**
 * @brief Writes @p tpl to @p filePath, folder and all.
 *
 * Serialised, validated against `://schema/assemblymark_template/v1.0.0.xsd`, then committed with
 * a QSaveFile — the target is never left truncated by a failure halfway through.
 */
auto Save(const OExportTemplate &tpl, const QString &filePath, QString *error = nullptr) -> bool;

/** @brief Reads @p filePath into @p tpl. @p tpl is untouched unless the file is a template. */
auto Load(const QString &filePath, OExportTemplate *tpl, QString *error = nullptr) -> bool;

/**
 * @brief Reads the template @p name designates, or « Default » when that name resolves to nothing.
 *
 * The library's one fallback, and it is a chain of exactly two. A plan keeps the NAME of the
 * template it was nested with, and the file behind a name can be deleted or renamed long after
 * the run — the plan then draws the installation's own template rather than nothing, which is a
 * visible answer to a question the user did not ask.
 *
 * False only when neither is there: a folder the user has emptied leaves the plan drawn alone, as
 * it was before there were templates at all.
 */
auto Resolve(const QString &name, OExportTemplate *tpl) -> bool;

/** @brief The name the shipped template carries, and the key of the favourite that points at it. */
auto DefaultTemplateName() -> QString;

/** @brief The template the installation starts from — Sophie's list of 17-09-2026. */
auto ShippedTemplate() -> OExportTemplate;

/**
 * @brief Writes the shipped template, and only the first time.
 *
 * Called on startup. Does nothing the moment the folder holds one template of any name, so it
 * seeds a library and never overwrites one — deleting every template brings this one back, which
 * is the behaviour the cartouche has and the reason its dropdown is never empty.
 */
void EnsureDefaultTemplate();
} // namespace OExportTemplateStorage

#endif // OEXPORTTEMPLATESTORAGE_H
