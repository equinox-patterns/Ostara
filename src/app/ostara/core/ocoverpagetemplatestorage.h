/************************************************************************
 **
 **  @file   ocoverpagetemplatestorage.h
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

#ifndef OCOVERPAGETEMPLATESTORAGE_H
#define OCOVERPAGETEMPLATESTORAGE_H

#include "ocoverpagetemplate.h"

#include <QString>
#include <QStringList>

/**
 * @brief The library of saved cover page templates: one `.ocpt` file per template, per-user.
 *
 * The assembly marks' arrangement, one format further on: files under a folder of the user's home,
 * enumerated by scanning it, **the file's base name being the identity** — the list shows it, and
 * nothing else identifies a template.
 *
 * A file is XML validated against its schema and committed atomically, so a template is either
 * the one that was saved or the one that was there before, never half of each.
 *
 * **Its own folder, and its own schema.** A cover page is not a plan's decor: it is written for
 * the PDF export of any pattern, tiled or not, where the assembly marks only exist on a tiled
 * plan. Sharing one file would have meant one version, one schema and one migration for two
 * things that neither apply together nor change together.
 *
 * **Nothing is seeded.** The assembly marks ship a written-out default so a plan always has
 * something to draw with; a cover page carries nothing but its name yet, and a template with no
 * informations in it would be an empty page handed to the user as if it were a starting point.
 * The folder begins empty and the first template is the user's.
 */
namespace OCoverPageTemplateStorage
{
/** @brief The extension of a saved template, without its dot. */
auto Extension() -> QString;

/** @brief The folder templates are saved in, from settings (GetPathCoverPageTemplates). */
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
 * @brief How a save ended.
 *
 * Three outcomes rather than a yes or a no, because the two ways of not being written ask for two
 * different things to be said: a name already taken is the user's to change, a write that failed is
 * not. The storage says which one happened; the editor words it.
 */
enum class SaveOutcome
{
    Saved,     ///< written, and the file it replaced, if any, is gone
    NameTaken, ///< another template answers to that name — nothing was written
    Failed     ///< the write failed — nothing was replaced, and @c error says why
};

/** @brief What a save produced, and everything the caller has to be told about it. */
struct SaveResult
{
    SaveOutcome outcome{SaveOutcome::Failed};

    /// The file written — set when the outcome is Saved.
    QString filePath;

    /// The name the file actually carries: what was typed, once made into a file name.
    QString name;

    /// Why the write failed — set when the outcome is Failed.
    QString error;
};

/**
 * @brief Writes @p tpl under the name it was given, taking the place of the file it had.
 *
 * **A template's name is its identity**, so saving under a new name *moves* it rather than leaving
 * a second copy behind: the new file is written first and the old one removed afterwards, so a
 * write that fails leaves what was there rather than losing both.
 *
 * A name another template already answers to is refused, and nothing is written — writing over it
 * would destroy something the user never asked to touch. A name that is already this template's own
 * file is not a collision: saving twice under one name is what saving is.
 *
 * Serialised, validated against `://schema/coverpage_template/v1.0.0.xsd`, then committed with a
 * QSaveFile — the target is never left truncated by a failure halfway through.
 *
 * @param name         what the user called the template
 * @param previousPath the file the template was in, empty for one that had none
 */
auto SaveAs(const OCoverPageTemplate &tpl, const QString &name, const QString &previousPath) -> SaveResult;

/** @brief Reads @p filePath into @p tpl. @p tpl is untouched unless the file is a template. */
auto Load(const QString &filePath, OCoverPageTemplate *tpl, QString *error = nullptr) -> bool;
} // namespace OCoverPageTemplateStorage

#endif // OCOVERPAGETEMPLATESTORAGE_H
