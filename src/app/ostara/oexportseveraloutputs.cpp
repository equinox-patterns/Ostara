/************************************************************************
 **
 **  @file   oexportseveraloutputs.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Export mode — l'export en lot : lire ce que le dock liste, faire
 **          décrire le lot, et l'écrire.
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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "dialogs/oexportbatchdialog.h"
#include "dialogs/oexportfiledialog.h"
#include "core/oexporttemplate.h"
#include "core/oexporttemplatestorage.h"
#include "oexportcuttingfile.h"
#include "oexportfolder.h"
#include "oexportpdf.h"
#include "oexportplottersvg.h"

#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QStringList>

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ExportSeveralOutputs le lot du dock : plusieurs générations, un format par catégorie.
 *
 * **Ce que le lot offre est ce que le dock liste** — le contexte courant (table de mesures, axe),
 * résolu par la règle qui vit ici et nulle part ailleurs. Le lot ne la réinvente pas : il demande
 * les mêmes identifiants, puis les range par catégorie.
 *
 * **Un plan de coupe est une troisième catégorie**, à côté de Print et de Plotter : il a un format
 * depuis le 15-10-2026 — DXF, PLT ou HPGL, choisi dans la page du lot (`CuttingPlan.md` §24). Comme
 * la case « mise en page » d'un lot de PDF, **le réglage vaut pour toutes ses lignes** : un lot est un
 * format, et qui veut deux jeux d'options fait deux lots.
 *
 * **Aucun nesting n'est relancé.** Une génération porte son résultat : le lot écrit ce que le dock
 * affiche, et une correction à la main reste ce qu'elle était — c'est la même règle que l'export
 * d'une seule génération, et c'est ce qui permet d'écrire six fichiers sans attendre six fois.
 */
void MainWindow::ExportSeveralOutputs()
{
    // Un clic qui arrive après avoir quitté le mode ne doit pas écrire les plans d'un autre mode —
    // la même garde, sur la même chose, que celle dont `ShowExportOutput` s'ouvre.
    if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Export)
    {
        return;
    }

    QVector<OExportBatchPage::Row> cuttingRows;
    QVector<OExportBatchPage::Row> printRows;
    QVector<OExportBatchPage::Row> plotterRows;

    for (const quint32 id : VisibleExportOutputIds())
    {
        const OExportOutput *output = m_exportOutputs.Find(id);
        if (output == nullptr)
        {
            continue;
        }

        // Le plan de coupe a sa catégorie, à côté des deux autres : il a un format depuis le
        // 15-10-2026 — DXF, PLT ou HPGL (`CuttingPlan.md` §24) — et le lot le prend comme les autres.
        if (output->plotterPlan)
        {
            plotterRows.append({output->id, output->name});
        }
        else if (output->cuttingPlan)
        {
            // **Une ligne par FICHIER, et non par génération.** Une génération qui porte la paire —
            // tissu et doublure — écrit deux fichiers, chacun avec son nom : elle se présente donc en
            // deux lignes, exactement comme le dialogue d'export demande deux noms pour la même
            // raison. Une génération qui n'a qu'un plan garde une ligne, et son nom d'hier.
            for (const OExportCuttingGuard::Material material : OExportCuttingFile::PlansOf(*output))
            {
                cuttingRows.append(
                    {output->id, OExportCuttingFile::PlanName(*output, material), material});
            }
        }
        else
        {
            printRows.append({output->id, output->name});
        }
    }

    if (cuttingRows.isEmpty() && printRows.isEmpty() && plotterRows.isEmpty())
    {
        return; // rien à offrir : le bouton du dock est éteint pour cette raison, et le resterait
    }

    // Le dossier : le dernier utilisé par un export, et non celui d'une gamme — un lot traverse
    // plusieurs gammes, il n'y en a pas une dont le dossier vaudrait pour toutes (`LastExportFolder`).
    OExportBatchDialog dialog(cuttingRows, printRows, plotterRows, LastExportFolder(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    const OExportBatchDialog::OutputCategory category = dialog.Category();
    const QString folder = dialog.Directory();
    const bool addLayoutLayer = dialog.AddLayoutLayer();

    // Le réglage du lot de plans de coupe : format, sous-type et options, **les mêmes pour toutes les
    // lignes** — comme la case « mise en page » d'un lot de PDF. Il n'est lu que si la catégorie est
    // celle-là, et il est lu ici parce que le dialogue meurt avec ce qui suit.
    const OExportCuttingFile::Options cuttingOptions = dialog.CuttingOptions();

    QStringList written;
    QStringList failed;
    QStringList untouched;

    for (const OExportBatchPage::Row &row : dialog.Chosen())
    {
        const OExportOutput *output = m_exportOutputs.Find(row.id);
        if (output == nullptr)
        {
            continue;
        }

        const QString path = QDir(folder).filePath(row.name);

        // La question d'écrasement, posée une fois par fichier et à la définition près — c'est
        // celle de l'export d'une seule génération (`OExportFileDialog::ConfirmOverwrite`). Un
        // « non » laisse ce fichier-là tel qu'il est et ne touche pas aux autres : un lot n'est pas
        // une question dont toutes les réponses seraient la même.
        if (!OExportFileDialog::ConfirmOverwrite(this, path))
        {
            untouched.append(row.name);
            continue;
        }

        bool ok = false;
        QString why; // ce qui a empêché l'écriture, dans les mots de l'utilisatrice
        if (category == OExportBatchDialog::OutputCategory::Cutting)
        {
            // Le plan de coupe s'écrit avec le réglage **commun** de la page — un lot est un format —
            // et l'écrivain du mode Export : DXF, PLT ou HPGL, à 100 %, avec les options du mode
            // Details moins l'échelle (`CuttingPlan.md` §24). **Et c'est la nappe de la LIGNE qui est
            // écrite** : sur une paire, une ligne est le tissu et l'autre la doublure. Une génération
            // dont le nesting a échoué n'a pas de plan à écrire, et l'écrivain le dit lui-même : une
            // nappe qu'elle ne porte pas n'écrit rien plutôt que d'écrire un fichier vide.
            ok = OExportCuttingFile::Write(path, *output, cuttingOptions, row.material, &why);
        }
        else if (category == OExportBatchDialog::OutputCategory::Plotter)
        {
            // Un run plotter dont le nesting a échoué n'a pas de géométrie à écrire. L'export d'une
            // seule génération sort en silence sur ce cas ; ici le lot le dit sur sa ligne, ce qui
            // est la seule façon de distinguer « rien à écrire » de « écrit ».
            ok = output->result.ok && OExportPlotterSvg::Write(path, *output, pattern, doc);
        }
        else
        {
            // Le décor et la page de garde sont résolus **par les mêmes appels** que l'export PDF
            // d'une seule génération : le papier ne peut pas porter des marques que l'écran n'a pas
            // montrées, ni en manquer qu'il montrait. Rien n'est recopié ici, tout est redemandé.
            OExportTemplate marks;
            bool framed = false;
            const bool hasMarks = ExportMarksOf(*output, marks, framed);
            const OExportCoverPage::Drawing cover = OExportCoverPage::Of(*output, doc);

            ok = OExportPdf::Write(*output, path, pattern, doc, hasMarks ? &marks : nullptr, framed,
                                   addLayoutLayer, cover.IsEmpty() ? nullptr : &cover);
        }

        if (ok)
        {
            written.append(row.name);
        }
        else
        {
            // Une génération qui ne s'écrit pas n'arrête pas les autres : elles ont été demandées
            // aussi, et la ligne fautive se dit à la fin avec les autres — **et avec sa raison**,
            // quand l'écrivain en a une : « ça n'a pas marché » n'apprend rien à personne.
            failed.append(why.isEmpty() ? row.name : row.name + QStringLiteral("\n") + why);
        }
    }

    // Le dossier où le lot vient d'écrire — s'il a écrit quelque chose : c'est celui que le
    // prochain export proposera, lot comme génération seule.
    if (!written.isEmpty())
    {
        RememberExportFolder(folder);
    }

    // UN dialogue pour tout le lot, quelle que soit sa longueur : une boîte par fichier serait une
    // boîte à écarter six fois pour apprendre une chose.
    QStringList lines;
    if (!written.isEmpty())
    {
        lines << tr("%n file(s) written to %1", "", static_cast<int>(written.size())).arg(folder);
        lines << QString();
        lines << written;
    }

    // Ce qui a été refusé se dit aussi. Sans cette section, répondre « non » à la question
    // d'écrasement ne laisserait aucune trace — et l'utilisatrice chercherait un fichier qui n'a
    // pas été écrit.
    if (!untouched.isEmpty())
    {
        if (!lines.isEmpty())
        {
            lines << QString();
        }
        lines << tr("%n file(s) left untouched:", "", static_cast<int>(untouched.size()));
        lines << untouched;
    }

    if (failed.isEmpty())
    {
        if (!lines.isEmpty())
        {
            QMessageBox::information(this, tr("Export mode"), lines.join(QLatin1Char('\n')));
        }
        return;
    }

    if (!lines.isEmpty())
    {
        lines << QString();
    }
    lines << tr("%n file(s) could not be written:", "", static_cast<int>(failed.size()));
    lines << failed;

    QMessageBox::warning(this, tr("Export mode"), lines.join(QLatin1Char('\n')));
}
