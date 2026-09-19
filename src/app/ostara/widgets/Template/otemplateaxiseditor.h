/************************************************************************
 **
 **  @file   otemplateaxiseditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   19/09/2026
 **
 **  @brief  The axes of a size table, read from the .omt for whoever shows them.
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

#ifndef OTEMPLATEAXISEDITOR_H
#define OTEMPLATEAXISEDITOR_H

#include "core/otemplateaxisoptions.h"
#include "core/otemplateoptioneditor.h"

class OAxisRenameEditor;

/**
 * @brief The composite that shows the axes of a pattern's size table, and names them.
 *
 * **It is the same control in both families, because it is the same question.** A cartouche states
 * the sizes of the piece it decorates, a cover page the sizes of the sheet it heads — and neither
 * invents what an axis is: the .omt declares them, and a .omt declares three at most. So the whole
 * of this composite's knowledge is « ask the table », which is why it is shared and the families
 * declare it rather than each writing their own.
 *
 * What is left to a family is only what it alone knows: the cartouche renames a *measurement* too,
 * which is one thing there is nothing to choose between, and that stays its own control.
 */
class OTemplateAxisEditor : public OTemplateOptionEditor
{
    Q_OBJECT

public:
    explicit OTemplateAxisEditor(const OTemplateOptionContext &context, QWidget *parent = nullptr);

    void Refresh(const OTemplateOptionContext &context) override;

private:
    OAxisRenameEditor *m_editor{nullptr};
};

#endif // OTEMPLATEAXISEDITOR_H
