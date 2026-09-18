/************************************************************************
 **
 **  @file   ocoverpagecatalogue.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The catalogue of what a cover page can show.
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

#ifndef OCOVERPAGECATALOGUE_H
#define OCOVERPAGECATALOGUE_H

#include "otemplatesection.h"

#include <QList>
#include <QString>

/**
 * @brief The informations a cover page can be made of — the menu behind « + Add information ».
 *
 * **A catalogue, like the cartouche's, and a different one.** What the two share is the
 * mechanism: an entry says what an information is called and which fields it has, and an editor
 * builds a control per field without knowing either family. What they do not share is the list
 * itself — a cover page has no fabric to cut and no piece to number, and a cartouche has no
 * stature to state. So the two catalogues grow apart, and each is read only by its own editor.
 *
 * **It holds one entry for now**, and it grows one information at a time. Nothing else changes
 * when an entry is added here: the menu, the list, the form and the file all read this list
 * rather than naming what is in it.
 *
 * The entries are built once and cached; `InvalidateCache()` is what a language change calls,
 * because a label is resolved when the entry is built and not when it is shown.
 */
class OCoverPageCatalogue
{
public:
    /** @brief Every information a cover page can show, in the order the menu offers them. */
    static auto Sections() -> const QList<OTemplateSection> &;

    /** @brief The entry an id designates — an invalid one when the id is unknown. */
    static auto Section(const QString &id) -> const OTemplateSection &;

    /** @brief Drops the cache, so the next call rebuilds the entries in the current language. */
    static void InvalidateCache();

private:
    static auto Build() -> QList<OTemplateSection>;
};

#endif // OCOVERPAGECATALOGUE_H
