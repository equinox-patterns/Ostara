/************************************************************************
 **
 **  @file   OPieceCartoucheAxisRenameEditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  What there is to rename in a cartouche, handed to the shared control.
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

#include "widgets/Template/oaxisrenameeditor.h"

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

    connect(m_editor, &OAxisRenameEditor::SelectionChanged, this, [context](const QStringList &axes) {
        context.apply(OTemplateAxisOptions::kAxes, axes);
    });
    connect(m_editor, &OAxisRenameEditor::RenamesChanged, this,
            [context](const QMap<QString, OAxisRenames> &renames) {
                context.apply(OTemplateAxisOptions::kRenames, MakeAxisRenamesByAxis(renames));
            });
    connect(m_editor, &OAxisRenameEditor::OneLineChanged, this, [context](bool on) {
        context.apply(OTemplateAxisOptions::kOneLine, on);
    });

    layout->addWidget(m_editor);

    Refresh(context);
}

//---------------------------------------------------------------------------------------------------------------------

void OPieceCartoucheAxisRenameEditor::Refresh(const OTemplateOptionContext &context)
{
    // **A measurement, and only a measurement.** The size axes are shown by the shared composite
    // now — the two families ask the table the same question, so they ask it with the same control.
    // What is left here is what only a cartouche has: a measurement, which is one thing there is
    // nothing to choose between, so it is shown without a box.
    QString name;
    if (context.sectionTypeId == QStringLiteral("measure"))
    {
        name = context.options.value(QStringLiteral("measureName")).toString();
    }

    QList<OAxisChoice> available;
    QStringList selected;
    if (!name.isEmpty())
    {
        available.append({name, QStringList(), QString()});
        selected.append(name);
    }

    QMap<QString, OAxisRenames> renames =
        ParseAxisRenamesByAxis(context.options.value(OTemplateAxisOptions::kRenames), QString());
    const bool oneLine = context.options.value(OTemplateAxisOptions::kOneLine, false).toBool();

    // **The aliases are aliases of a name.** A measurement that was renamed and is then swapped for
    // another describes something that is no longer on the page, so they are dropped rather than
    // left to rename the wrong thing. The first refresh has nothing to compare with.
    if (m_refreshed && m_origin != name)
    {
        if (context.apply)
        {
            context.apply(OTemplateAxisOptions::kRenames, QVariantMap{});
        }
        renames.clear();
    }
    m_origin = name;
    m_refreshed = true;

    m_editor->SetData(available, selected, renames, oneLine, context.languages, false);
}
