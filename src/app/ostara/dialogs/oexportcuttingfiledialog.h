/************************************************************************
 **
 **  @file   oexportcuttingfiledialog.h
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

#ifndef OEXPORTCUTTINGFILEDIALOG_H
#define OEXPORTCUTTINGFILEDIALOG_H

#include "../core/oexportcuttingguard.h"
#include "../oexportcuttingfile.h"
#include "oexportfiledialog.h"

class OExportCuttingFileOptions;
class QLabel;
class QLineEdit;

/**
 * @brief Where a cutting plan is written, and in what: DXF, PLT or HPGL.
 *
 * The name and the folder come from @ref OExportFileDialog, and with them the overwrite question and
 * the folder that is remembered between two exports. What this one adds is the setting — the format,
 * its sub-type and the options — which lives in @ref OExportCuttingFileOptions because the batch's
 * cutting page needs the very same one, and two copies of seven fields would be two truths.
 *
 * **And it does not offer the scale.** A cutting plan leaves at 100 %, always: it is a file a machine
 * cuts, and a reduced scale would produce a plan that looks right and cuts wrong (`CuttingPlan.md`
 * §24.2).
 *
 * **Une génération qui porte la PAIRE — tissu et doublure — demande DEUX noms.** Un plan de coupe est
 * un fichier par nappe : les deux se livrent ensemble, dans le même dossier, et chacun dit sa matière
 * (`-Fabric`, `-Lining`) — sans quoi le second écraserait le premier. La ligne unique d'hier reste la
 * ligne unique dès qu'il n'y a qu'un plan.
 *
 * ⚠️ The suffix follows the format. The base class takes its own in the constructor and appends it to
 * the name; with three formats to choose from, that one has to change, which is what `SetSuffix` is
 * for — and it changes it on **chaque** ligne de nom, la seconde comprise.
 */
class OExportCuttingFileDialog : public OExportFileDialog
{
    Q_OBJECT // NOLINT

public:
    /// @param output the generation: its name is offered as the file name, and a second line is
    ///               opened when it carries a lining plan as well as the fabric one.
    /// @param directory where it opens; the folder last used, or Documents.
    explicit OExportCuttingFileDialog(const OExportOutput &output, const QString &directory,
                                      QWidget *parent = nullptr);

    /// @brief Everything the writer needs: the format, its sub-type, and the options.
    auto Options() const -> OExportCuttingFile::Options;

    /// @brief Les fichiers à écrire : un par plan, chacun avec sa matière et son chemin.
    ///
    /// **L'appelant n'a pas à recompter les nappes.** C'est ce dialogue qui sait quelle ligne est
    /// laquelle — il les a construites — et il le dit ici une fois, dans l'ordre de ses lignes.
    auto Targets() const -> QVector<OExportCuttingFile::Target>;

protected:
    void Retranslate() override;

private:
    OExportCuttingFileOptions *m_options{nullptr};

    /// La seconde ligne de nom, quand la génération porte deux plans. Nulle autrement : un plan seul
    /// garde la ligne — et le libellé — du squelette.
    QLabel *m_liningLabel{nullptr};
    QLineEdit *m_liningEdit{nullptr};

    /// Les plans de la génération, lus UNE fois à la construction : c'est cette liste qui dit combien
    /// de lignes de nom ont été ouvertes, et à quelle matière chacune appartient.
    QVector<OExportCuttingGuard::Material> m_plans{};
};

#endif // OEXPORTCUTTINGFILEDIALOG_H
