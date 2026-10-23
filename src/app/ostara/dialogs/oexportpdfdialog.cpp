/************************************************************************
 **
 **  @file   oexportpdfdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the PDF destination dialog.
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

#include "oexportpdfdialog.h"

#include <QCheckBox>

namespace
{
/// What the file is called when the user leaves the name alone.
constexpr auto kPdfSuffix = ".pdf";
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportPdfDialog::OExportPdfDialog(const QString &outputName, const QString &directory, QWidget *parent)
  : OExportFileDialog(outputName, directory, QString::fromLatin1(kPdfSuffix), parent)
{
    // Unchecked, and deliberately so: an unticked box is what a decor layer needs to be safe,
    // because a plan printed without its cartouche and its annotations looks perfectly normal.
    // The box is there for the user who wants the layer, not for the one who does not know.
    m_layoutLayerCheck = new QCheckBox(this);
    m_layoutLayerCheck->setChecked(false);
    AddOption(m_layoutLayerCheck);

    // Checked, and the opposite of the box above for the same reason: an unticked box here
    // produces a PDF that prints perfectly and that Rabnest cannot reopen, and nothing in
    // the file says so. The client would be the one to discover it.
    m_payloadCheck = new QCheckBox(this);
    m_payloadCheck->setChecked(true);
    AddOption(m_payloadCheck);

    // Retranslate est rappelée : la passe du squelette a posé ses textes et SON titre, alors que la
    // partie dérivée n'existait pas encore pour dire le sien ni nommer sa case.
    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPdfDialog::AddLayoutLayer() const -> bool
{
    return m_layoutLayerCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPdfDialog::CarriesPayload() const -> bool
{
    return m_payloadCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPdfDialog::Retranslate()
{
    OExportFileDialog::Retranslate();

    setWindowTitle(tr("Export the plan as PDF"));
    m_layoutLayerCheck->setText(tr("Add Layout layer"));
    m_layoutLayerCheck->setToolTip(
        tr("Write the layout as one of the PDF's layers, so a printer can switch it off.\n"
           "Left unchecked, the layout is printed with the plan and cannot be switched off -\n"
           "which is the safer setting: a plan printed without its information block, its\n"
           "folds and its annotations looks perfectly normal."));

    m_payloadCheck->setText(tr("Compatible with Rabnest"));
    m_payloadCheck->setToolTip(
        tr("Write what the plan was drawn from inside the file, so it can be reopened in\n"
           "Rabnest and delivered again in another format. Nothing in the PDF looks different\n"
           "and no reader shows it; the file is only heavier.\n"
           "Left unchecked, the PDF prints and nothing reopens it."));
}
