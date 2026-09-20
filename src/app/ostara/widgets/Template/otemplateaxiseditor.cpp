/************************************************************************
 **
 **  @file   otemplateaxiseditor.cpp
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

#include "otemplateaxiseditor.h"

#include "core/osizetable.h"
#include "oaxisrenameeditor.h"

#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------

OTemplateAxisEditor::OTemplateAxisEditor(const OTemplateOptionContext &context, QWidget *parent)
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

void OTemplateAxisEditor::Refresh(const OTemplateOptionContext &context)
{
    // **Every axis the table declares, and nothing else.** This control has no opinion about which
    // axes exist; it asks, and offers what it is told. With no table there is nothing to offer, and
    // the boxes are empty rather than wrong.
    QList<OAxisChoice> available;
    if (context.table != nullptr)
    {
        for (const QString &name : context.table->AxisNames())
        {
            available.append({name, context.table->AxisValues(name)});
        }
    }

    // A file written when an information could show a single axis names it under a key of its own.
    // Read as the one-element selection it means, so a template saved before today opens with its
    // axis ticked rather than with none — and its aliases land on the right axis rather than nowhere.
    const QString legacyAxis = context.options.value(OTemplateAxisOptions::kLegacyAxis).toString();
    QStringList selected = context.options.value(OTemplateAxisOptions::kAxes).toStringList();
    if (selected.isEmpty() && !legacyAxis.isEmpty())
    {
        selected.append(legacyAxis);
    }

    // **An axis spoken about elsewhere is not a value stated beside the others.** A range says
    // everything about the axis it grades along — one line per size — so a box offering to also
    // state it as a single value would offer the same information twice. It is dropped from the
    // selection here as well as greyed below: the resolver reads the selection, and a page that
    // kept it would draw the axis it lists a second time, once.
    if (!m_reservedAxis.isEmpty())
    {
        selected.removeAll(m_reservedAxis);
    }

    const QMap<QString, OAxisRenames> renames =
        ParseAxisRenamesByAxis(context.options.value(OTemplateAxisOptions::kRenames), legacyAxis);
    const bool oneLine = context.options.value(OTemplateAxisOptions::kOneLine, false).toBool();

    // **Unticking a box keeps that axis's aliases.** Hiding an axis is not changing which axis it
    // is, so the work the user did on it is kept for the moment it comes back — dropping it would
    // make ticking a box again lose what had already been written.
    m_editor->SetReservedAxis(m_reservedAxis);
    m_editor->SetData(available, selected, renames, oneLine, context.languages);
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateAxisEditor::SetReservedAxis(const QString &axisName)
{
    m_reservedAxis = axisName;
}
