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
#include "core/oexportoutput.h"
#include "oexportcuttingfileoptions.h"

#include <QLabel>
#include <QLineEdit>

//---------------------------------------------------------------------------------------------------------------------
OExportCuttingFileDialog::OExportCuttingFileDialog(const OExportOutput &output, const QString &directory,
                                                   QWidget *parent)
  : OExportFileDialog(OExportCuttingFile::PlanName(output, OExportCuttingGuard::Material::Fabric),
                      directory, VLayoutExporter::ExportFormatSuffix(LayoutExportFormats::DXF_AAMA),
                      parent),
    m_plans(OExportCuttingFile::PlansOf(output))
{
    // ── Une ligne de nom par plan ──────────────────────────────────────────────────────────────────
    //
    // **Deux nappes, deux fichiers, deux noms.** Le dossier reste commun : les deux se livrent
    // ensemble. Le squelette porte la seconde ligne — il est le seul à savoir où vivent ses lignes, et
    // c'est lui qui reporte l'extension du format sur chaque champ.
    //
    // Le libellé de la première est reposé par `Retranslate`, pas ici : un dialogue qui écrit deux
    // fichiers ne demande plus « un nom », il demande celui de chaque matière — et il le redit à
    // chaque changement de langue, comme son titre.
    if (m_plans.size() > 1)
    {
        m_liningLabel = new QLabel(this);
        m_liningEdit = AddNameRow(
            m_liningLabel, OExportCuttingFile::PlanName(output, OExportCuttingGuard::Material::Lining));
    }

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
auto OExportCuttingFileDialog::Targets() const -> QVector<OExportCuttingFile::Target>
{
    QVector<OExportCuttingFile::Target> targets;

    // Les chemins sont dans l'ordre des lignes, et `m_plans` dans celui des plans : les deux listes
    // ont été construites ensemble, dans le même constructeur, et c'est ce qui les fait correspondre
    // sans qu'aucune des deux n'ait à recompter.
    const QStringList paths = FullPaths();
    targets.reserve(m_plans.size());
    for (int i = 0; i < m_plans.size() && i < paths.size(); ++i)
    {
        targets.append(OExportCuttingFile::Target{m_plans.at(i), paths.at(i)});
    }

    return targets;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingFileDialog::Retranslate()
{
    OExportFileDialog::Retranslate();

    setWindowTitle(tr("Export the cutting plan"));

    // **Deux plans : chaque ligne dit la sienne.** Le squelette vient de nommer la première « File
    // name: » — le mot d'un dialogue qui n'écrit qu'un fichier — et il faut le remplacer ici, une fois
    // par traduction, comme le titre.
    if (m_liningLabel != nullptr)
    {
        NameLabel()->setText(tr("Fabric:"));
        m_liningLabel->setText(tr("Lining:"));
    }
}
