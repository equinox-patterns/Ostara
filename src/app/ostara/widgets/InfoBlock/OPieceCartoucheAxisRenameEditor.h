/************************************************************************
 **
 **  @file   OPieceCartoucheAxisRenameEditor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The aliases a cartouche gives to the names a size table holds.
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

#ifndef OPIECECARTOUCHTAXISRENAMEEDITOR_H
#define OPIECECARTOUCHTAXISRENAMEEDITOR_H

#include "core/otemplateoptioneditor.h"

class OAxisRenameEditor;

/**
 * @brief The cartouche's control for renaming size axes, or a measurement, per language.
 *
 * A wrapper, and only a wrapper: `OAxisRenameEditor` is the control and does the work, and this
 * puts it where the shared section editor can build it — what there is to rename is knowledge of
 * this family's, and the shared editor is not given it. The control itself is shared with the cover
 * page, which shows the same axes of the same table and renames them the same way.
 *
 * It is the composite that has to be *refreshed*: the table can change under it, and a renamed
 * measurement is dropped when another takes its place. That is what the composite interface is for.
 */
class OPieceCartoucheAxisRenameEditor : public OTemplateOptionEditor
{
    Q_OBJECT

public:
    explicit OPieceCartoucheAxisRenameEditor(const OTemplateOptionContext &context,
                                             QWidget *parent = nullptr);

    void Refresh(const OTemplateOptionContext &context) override;

private:
    OAxisRenameEditor *m_editor{nullptr};

    /// What the aliases currently rename. The first refresh has nothing to compare with.
    QString m_origin;
    bool m_refreshed{false};
};

#endif // OPIECECARTOUCHTAXISRENAMEEDITOR_H
