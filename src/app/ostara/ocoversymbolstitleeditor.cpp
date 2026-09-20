/************************************************************************
 **
 **  @file   ocoversymbolstitleeditor.cpp
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

#include "ocoversymbolstitleeditor.h"

#include "core/ocoversymbols.h"
#include "core/otemplateaxisoptions.h"

#include "widgets/Template/oaxisrenameeditor.h"

#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------

OCoverSymbolsTitleEditor::OCoverSymbolsTitleEditor(const OTemplateOptionContext &context, QWidget *parent)
  : OTemplateOptionEditor(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_editor = new OAxisRenameEditor(this);
    m_editor->SetTitle(context.label);
    m_editor->SetOneLineOffered(false);

    // The context is held by the connection rather than kept as a member: the editor refreshes a
    // composite with the options alone and no way to write them back, so the one that can still
    // write is the one the factory handed over — see OTemplateOptionContext::apply.
    connect(m_editor, &OAxisRenameEditor::RenamesChanged, this,
            [context](const QMap<QString, OAxisRenames> &renames) {
                if (context.apply)
                {
                    context.apply(OTemplateAxisOptions::kRenames, MakeAxisRenamesByAxis(renames));
                }
            });

    layout->addWidget(m_editor);

    Refresh(context);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverSymbolsTitleEditor::Refresh(const OTemplateOptionContext &context)
{
    // **A token for the file, a word for the field.** What is renamed is keyed by the token, so a
    // rename survives a change of the interface's language; what the field offers to be replaced is
    // the word the application calls a legend by, so the user is not reading jargon.
    const QList<OAxisChoice> available = {
        {OCoverSymbols::kTitleKey, QStringList(), OCoverSymbols::TitleWord(QString())}};
    const QStringList selected = {OCoverSymbols::kTitleKey};

    const QMap<QString, OAxisRenames> renames =
        ParseAxisRenamesByAxis(context.options.value(OTemplateAxisOptions::kRenames), QString());

    m_editor->SetData(available, selected, renames, false, context.languages, false);
}
