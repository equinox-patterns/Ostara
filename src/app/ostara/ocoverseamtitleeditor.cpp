/************************************************************************
 **
 **  @file   ocoverseamtitleeditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  Renaming the heading of the seam-allowances information.
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

#include "ocoverseamtitleeditor.h"

#include "core/ocoverseamallowances.h"
#include "core/otemplateaxisoptions.h"

#include "widgets/Template/oaxisrenameeditor.h"

#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------

OCoverSeamTitleEditor::OCoverSeamTitleEditor(const OTemplateOptionContext &context, QWidget *parent)
  : OTemplateOptionEditor(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_editor = new OAxisRenameEditor(this);
    m_editor->SetTitle(context.label);
    // **One title, and the user may want each language on its own line.** These headings are
    // sentences — « Marges non comprises » — and two of them on one line is a line a reader has to
    // take apart to find where one language ends. The legend's title, one word, is offered neither.
    m_editor->SetOneLineOffered(true);

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

    // The one-line answer belongs to the same block of settings, and is written like any other of
    // its options — which is also what tells the resolver how to lay the heading out.
    connect(m_editor, &OAxisRenameEditor::OneLineChanged, this, [context](bool oneLine) {
        if (context.apply)
        {
            context.apply(OTemplateAxisOptions::kOneLine, oneLine);
        }
    });

    layout->addWidget(m_editor);

    Refresh(context);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverSeamTitleEditor::Refresh(const OTemplateOptionContext &context)
{
    // **Which heading it is showing is the answer's to say.** Reading it here rather than being told
    // means the control is right whenever it is refreshed, including the refresh the answer itself
    // asks for — see the Choice control, which asks for one.
    //
    // An answer not given yet shows the first of the two: the control stands greyed until the
    // question is answered, and something has to be written in it.
    const QString seamCase = context.options.value(OCoverSeamAllowances::kCaseKey).toString();
    const QString shown =
        seamCase == OCoverSeamAllowances::kNotIncluded ? seamCase : OCoverSeamAllowances::kIncluded;

    // **A token for the file, a word for the field.** What is renamed is keyed by the case, so a
    // rename survives a change of the interface's language; what the field offers to be replaced is
    // the sentence the application would print, so the user is not reading a token.
    const QList<OAxisChoice> available = {
        {OCoverSeamAllowances::TitleKey(shown), QStringList(),
         OCoverSeamAllowances::TitleWord(shown, QString())}};
    const QStringList selected = {OCoverSeamAllowances::TitleKey(shown)};

    const QMap<QString, OAxisRenames> renames =
        ParseAxisRenamesByAxis(context.options.value(OTemplateAxisOptions::kRenames), QString());
    const bool oneLine = context.options.value(OTemplateAxisOptions::kOneLine, false).toBool();

    m_editor->SetData(available, selected, renames, oneLine, context.languages, false);
}
