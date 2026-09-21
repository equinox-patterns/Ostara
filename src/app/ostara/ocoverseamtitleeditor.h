/************************************************************************
 **
 **  @file   ocoverseamtitleeditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  Renaming the heading of the seam-allowances information —
 **          whichever of the two it is currently saying.
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

#ifndef OCOVERSEAMTITLEEDITOR_H
#define OCOVERSEAMTITLEEDITOR_H

#include "core/otemplateoptioneditor.h"

class OAxisRenameEditor;

/**
 * @brief Renaming the heading the information prints, in each language of the page.
 *
 * **Two headings, and the user is renaming one of them at a time.** A page that says the allowances
 * are included says something else than a page that says they are not, so the two titles are their
 * own: what the user renames here is the heading of the case the answer below has chosen, and the
 * other one is left exactly as it was.
 *
 * **Both live in the one map the shared control writes.** A second map would be a second format for
 * the same thing, and two controls writing the same key would overwrite each other — so there is one
 * control, showing one title, writing back the map whole. Which is why the shared editor is told
 * again whenever the answer changes: it is not the same title any more.
 *
 * One line per language *is* offered here, unlike the legend's: these headings are sentences, and a
 * sentence written in two languages is a long thing to keep on one line.
 */
class OCoverSeamTitleEditor : public OTemplateOptionEditor
{
    Q_OBJECT

public:
    explicit OCoverSeamTitleEditor(const OTemplateOptionContext &context, QWidget *parent = nullptr);

    void Refresh(const OTemplateOptionContext &context) override;

private:
    OAxisRenameEditor *m_editor{nullptr};
};

#endif // OCOVERSEAMTITLEEDITOR_H
