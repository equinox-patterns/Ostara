/************************************************************************
 **
 **  @file   ocoversymbolstitleeditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The title of a legend, renamed in each language of the page.
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

#ifndef OCOVERSYMBOLSTITLEEDITOR_H
#define OCOVERSYMBOLSTITLEEDITOR_H

#include "core/otemplateoptioneditor.h"

class OAxisRenameEditor;

/**
 * @brief Renaming a legend's title, in each language of the page.
 *
 * **One thing, and nothing to choose between**, which is what the shared control offers when it is
 * asked not to be selectable: a legend has one title, so it is shown without a box — the way a
 * cartouche shows a measurement, and for the same reason.
 *
 * **What the title is keyed by is not what it says.** The renames are stored under a stable token,
 * so that changing the interface's language does not throw them away, and the field offers the
 * *word* — the application's own name for a legend — so that the user is not shown a token. The two
 * travel together in the choice the control is fed (@ref OAxisChoice::displayName).
 *
 * The choice of one line per language is not offered here: a title is one line whatever happens, and
 * a box that changed nothing when it was ticked would be a box lying about what it does.
 */
class OCoverSymbolsTitleEditor : public OTemplateOptionEditor
{
    Q_OBJECT

public:
    explicit OCoverSymbolsTitleEditor(const OTemplateOptionContext &context, QWidget *parent = nullptr);

    void Refresh(const OTemplateOptionContext &context) override;

private:
    OAxisRenameEditor *m_editor{nullptr};
};

#endif // OCOVERSYMBOLSTITLEEDITOR_H
