/************************************************************************
 **
 **  @file   oexportcuttingfile.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/10/2026
 **
 **  @brief  Export mode — the file a cutting plan is written to: DXF, PLT or HPGL.
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

#ifndef OEXPORTCUTTINGFILE_H
#define OEXPORTCUTTINGFILE_H

#include <QCoreApplication>
#include <QString>
#include <QVector>

#include "../vdxf/dxfdef.h"
#include "../vlayout/vlayoutdef.h"
#include "core/oexportcuttingguard.h"

struct OExportOutput;

/**
 * @brief The cutting file of a plan: DXF, PLT or HPGL — a cutting plan, laid on fabric, written the
 * way the plotter or the cutter reads it.
 *
 * **What is written is what the screen shows.** The pieces are placed by the very same composition
 * the canvas uses — `OExportFootprint::PieceMatrix`, applied to the piece's own matrix, never to the
 * item's transform — so the file cannot carry a piece the screen did not draw somewhere else. The
 * engines are the ones the Details mode already writes with: `VLayoutExporter` over `VDxfEngine` and
 * `VHPGLEngine`, the same code, the same options.
 *
 * **One thing is not the same: the scale.** A cutting plan leaves at 100 %, always — it is a file a
 * machine cuts, and a reduced scale would produce a plan that looks right and cuts wrong. The
 * Details export keeps its scale option; this one has none (`CuttingPlan.md` §24.2).
 *
 * What the file does NOT carry: the decor — no information block, no annotations, no miniatures —
 * because a cutting plan's canvas shows none; and no fold-line caption, because that is a caption.
 */
namespace OExportCuttingFile
{
/**
 * @brief What the export dialog asks for, and the whole of it.
 *
 * The options are the Details export's, format by format: an option that means nothing for a format
 * is not offered for it, and its default here is the value that format would have had there.
 */
struct Options
{
    /// The format AND its sub-type, in one value — exactly as the Details export names it. The nine
    /// `DXF_AC*_Flat` take the scene path, everything else the pieces path.
    LayoutExportFormats format{LayoutExportFormats::DXF_AAMA};

    /// `Binary form` — DXF only.
    bool binary{false};

    /// `Text as paths` — flat DXF only: AAMA/ASTM carry no text this engine can vectorise.
    bool textAsPaths{false};

    /// `Show grainline` — the three formats; not offered for AAMA/ASTM, which always write it.
    bool showGrainline{true};

    /// `Boundary together with notches` — the three formats. False keeps the notches separate.
    bool togetherWithNotches{false};

    /// `Show the main path` — the sewing line. AAMA/ASTM only.
    bool showMainPath{false};

    /// The DXF apparel compatibility mode. AAMA/ASTM only.
    DXFApparelCompatibility compatibility{DXFApparelCompatibility::STANDARD};
};

/**
 * @brief Un fichier à écrire : quelle nappe, et où.
 *
 * **La paire se dit par une LISTE**, et l'ordre est celui des plans : le tissu d'abord. Le dialogue qui
 * offre deux noms, le lot qui écrit deux fichiers et l'écrivain qui les écrit tous lisent cette même
 * liste plutôt que de recompter les nappes chacun de son côté — deux comptages seraient deux réponses
 * à une seule question, et le jour où l'une oublierait la doublure est le jour où un fichier manque
 * sans que personne ne l'ait dit.
 */
struct Target
{
    OExportCuttingGuard::Material material{OExportCuttingGuard::Material::Fabric};
    QString path{};
};

/**
 * @brief Les plans qu'une génération porte, dans l'ordre : le tissu, et la doublure quand elle est là.
 *
 * **La seule réponse à « combien de fichiers ? ».** Un run de coupe porte un plan, ou deux — jamais
 * zéro, jamais trois : la doublure n'existe que si le run l'a demandée. Un run qui n'est pas un plan de
 * coupe n'en porte aucun, et c'est ce que la liste vide dit.
 */
auto PlansOf(const OExportOutput &output) -> QVector<OExportCuttingGuard::Material>;

/**
 * @brief Le nom de fichier d'une nappe, **sans extension** : l'appelant ajoute celle du format.
 *
 * **Un seul plan garde le nom de la sortie**, tel quel : c'est ce qu'un plan de coupe s'appelait avant
 * la doublure, et rien ne justifie de le renommer aujourd'hui. **Deux plans disent chacun leur matière**,
 * sans quoi les deux fichiers porteraient le même nom et le second écraserait le premier.
 *
 * ⚠️ **« Fabric » et « Lining » sont TRADUITS** — « Tissu » et « Doublure » — comme les jetons du nom
 * lui-même (« Cut », « Plot ») : le nom d'une sortie se lit dans la langue de celui qui la regarde, et
 * c'est la même règle partout. Un nom créé dans une langue ne se retraduit pas après coup : il est
 * écrit une fois, à la génération.
 *
 * Le contexte est nommé à la main — `QCoreApplication::translate("OExportCuttingFile", …)` — parce
 * qu'un espace de noms n'a pas de `tr()` : `Q_DECLARE_TR_FUNCTIONS` ne vit que dans une classe. C'est
 * l'idiome déjà employé par OPieceImport.
 */
auto PlanName(const OExportOutput &output, OExportCuttingGuard::Material material) -> QString;

/**
 * @brief Writes the placed plan to @p path. False when nothing could be written — the reason has
 * been logged, and the caller says so to the user.
 *
 * @param material which plan of the generation to write. A generation that carries only the fabric
 *                 refuses to be read as a lining one: there is nothing to write, and a file with no
 *                 piece in it is worse than no file at all.
 */
auto Write(const QString &path, const OExportOutput &output, const Options &options,
           OExportCuttingGuard::Material material = OExportCuttingGuard::Material::Fabric) -> bool;
} // namespace OExportCuttingFile

#endif // OEXPORTCUTTINGFILE_H
