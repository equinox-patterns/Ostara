/************************************************************************
 **
 **  @file   OPieceCartoucheAxisRenameEditor.cpp
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

#include "OPieceCartoucheAxisRenameEditor.h"

#include "OAxisRenameEditor.h"
#include "OPieceCartoucheOptions.h"
#include "core/osizetable.h"

#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------

OPieceCartoucheAxisRenameEditor::OPieceCartoucheAxisRenameEditor(const OTemplateOptionContext &context,
                                                                 QWidget *parent)
  : OTemplateOptionEditor(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_editor = new OAxisRenameEditor(this);
    m_editor->SetTitle(context.label);

    connect(m_editor, &OAxisRenameEditor::RenamesChanged, this,
            [context](const QMap<QString, OAxisRename> &renames) {
                context.apply(OPieceCartoucheAxisRename::kRenames, MakeAxisRenames(renames));
            });
    connect(m_editor, &OAxisRenameEditor::OneLineChanged, this, [context](bool on) {
        context.apply(OPieceCartoucheAxisRename::kOneLine, on);
    });

    layout->addWidget(m_editor);

    Refresh(context);
}

//---------------------------------------------------------------------------------------------------------------------

void OPieceCartoucheAxisRenameEditor::Refresh(const OTemplateOptionContext &context)
{
    // The "original" the aliases override: the axis name and the values it holds (Sizes), or the
    // measurement's name (Measurement — its value is computed, so there are none to list).
    QString originalName;
    QStringList values;
    if (context.sectionTypeId == QStringLiteral("axis"))
    {
        originalName = context.options.value(QStringLiteral("axis")).toString();
        values = context.table != nullptr ? context.table->AxisValues(originalName) : QStringList();
    }
    else if (context.sectionTypeId == QStringLiteral("measure"))
    {
        originalName = context.options.value(QStringLiteral("measureName")).toString();
    }

    QMap<QString, OAxisRename> renames =
        ParseAxisRenames(context.options.value(OPieceCartoucheAxisRename::kRenames));
    const bool oneLine = context.options.value(OPieceCartoucheAxisRename::kOneLine, false).toBool();

    // **The aliases are aliases of a name.** When that name changes, they describe something that
    // is no longer on the page, and are dropped rather than left to rename the wrong thing. That is
    // this control's business — which is why the shared editor, which used to do it, no longer
    // knows these keys at all. The first refresh has nothing to compare with: the section has just
    // been built and its aliases are whatever the file held.
    if (m_refreshed && m_origin != originalName)
    {
        if (context.apply)
        {
            context.apply(OPieceCartoucheAxisRename::kRenames, QVariantMap{});
        }
        renames.clear();
    }
    m_origin = originalName;
    m_refreshed = true;

    m_editor->SetData(renames, oneLine, context.languages, originalName, values);
}
