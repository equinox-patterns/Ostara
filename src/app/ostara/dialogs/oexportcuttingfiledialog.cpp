/************************************************************************
 **
 **  @file   oexportcuttingfiledialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/10/2026
 **
 **  @brief  Export mode — where a cutting plan is written, and in what.
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

#include "oexportcuttingfiledialog.h"

#include "../vlayout/vlayoutexporter.h"
#include "oexportcuttingfileoptions.h"

//---------------------------------------------------------------------------------------------------------------------
OExportCuttingFileDialog::OExportCuttingFileDialog(const QString &outputName, const QString &directory,
                                                   QWidget *parent)
  : OExportFileDialog(outputName, directory,
                      VLayoutExporter::ExportFormatSuffix(LayoutExportFormats::DXF_AAMA), parent)
{
    m_options = new OExportCuttingFileOptions(this);
    AddOption(m_options);

    // L'extension suit le format : un fichier livré sous un nom qui annonce l'autre format est un
    // nom qui ment sur son contenu.
    connect(m_options, &OExportCuttingFileOptions::FormatChanged, this,
            [this] { SetSuffix(m_options->Suffix()); });

    // Retranslate est rappelée : la passe du squelette a posé ses textes et SON titre, alors que la
    // partie dérivée n'existait pas encore pour dire le sien.
    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingFileDialog::Options() const -> OExportCuttingFile::Options
{
    return m_options->Options();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileDialog::Retranslate()
{
    OExportFileDialog::Retranslate();

    setWindowTitle(tr("Export the cutting plan"));
}
