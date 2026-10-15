/************************************************************************
 **
 **  @file   oexportcuttingfile.cpp
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

#include "oexportcuttingfile.h"

#include "core/oexportoutput.h"
#include "oexportfootprint.h"
#include "oexportgeometry.h"
#include "oexportnesting.h"

#include "../vdxf/libdxfrw/drw_base.h"
#include "../vlayout/vlayoutexporter.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"

#include <QFileInfo>
#include <QGraphicsScene>
#include <QtMath>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// The plan's rectangle, in pixels: what the engines take for the size of their file. The plan
/// starts at the bin's origin and runs to the height the nesting used — the very rectangle the
/// canvas draws.
auto PlanRect(const OExportNestingResult &result) -> QRectF
{
    return {0.0, 0.0, result.binWidthPx, result.usedHeightPx};
}

//---------------------------------------------------------------------------------------------------------------------
/// The plan's pieces, PLACED — the canvas's own composition, and the only place it is written.
///
/// ⚠️ The placement goes into the piece's OWN matrix, composed with the one it already carries —
/// never into an item's transform, and never by overwriting: the Details export learned that a
/// transform on the item drags the bounding rect along with it, and that overwriting throws away
/// the mirror a symmetric copy is made of.
///
/// ⚠️ Et c'est la même boucle que `OExportCanvas::Build`, sur les mêmes deux listes (`result.keys` et
/// `result.placements` sont index-alignées) : le fichier est ce que l'écran montre, ou il est faux.
///
/// Les deux listes arrivent en paramètres — et non la génération — parce qu'une génération en porte
/// DEUX jeux : celui du tissu et celui de la doublure. L'appelant a déjà choisi sa nappe, et cette
/// fonction n'a pas à savoir qu'il y en a une autre.
auto PlacedPieces(const QVector<OExportSizeLayer> &layers, const OExportNestingResult &result)
    -> QVector<VLayoutPiece>
{
    QVector<VLayoutPiece> placed;

    for (int i = 0; i < result.placements.size() && i < result.keys.size(); ++i)
    {
        const OExportPlacement &placement = result.placements.at(i);
        const QString &key = result.keys.at(i);

        for (const OExportSizeLayer &layer : layers)
        {
            for (const OExportPieceGeometry &geometry : layer.pieces)
            {
                if (geometry.key != key)
                {
                    continue;
                }

                VLayoutPiece piece = geometry.piece;
                piece.SetMatrix(geometry.piece.GetMatrix() *
                                OExportFootprint::PieceMatrix(placement, geometry.alignDelta));

                // « Coupé au pli » is a caption, and a cutting plan carries none: the canvas drops
                // it too, for the same reason and in the same words.
                piece.SetShowFoldLineLabel(false);

                placed.append(piece);
            }
        }
    }

    return placed;
}

//---------------------------------------------------------------------------------------------------------------------
/// The nine flat DXF versions, and only them: the engine needs one by name, and nothing here may
/// guess that an enum value and a version number happen to meet.
auto FlatDxfVersion(LayoutExportFormats format) -> DRW::Version
{
    switch (format)
    {
        case LayoutExportFormats::DXF_AC1006_Flat:
            return DRW::AC1006;
        case LayoutExportFormats::DXF_AC1009_Flat:
            return DRW::AC1009;
        case LayoutExportFormats::DXF_AC1012_Flat:
            return DRW::AC1012;
        case LayoutExportFormats::DXF_AC1014_Flat:
            return DRW::AC1014;
        case LayoutExportFormats::DXF_AC1015_Flat:
            return DRW::AC1015;
        case LayoutExportFormats::DXF_AC1018_Flat:
            return DRW::AC1018;
        case LayoutExportFormats::DXF_AC1021_Flat:
            return DRW::AC1021;
        case LayoutExportFormats::DXF_AC1024_Flat:
            return DRW::AC1024;
        case LayoutExportFormats::DXF_AC1027_Flat:
            return DRW::AC1027;
        default:
            return DRW::AC1009;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// True when the format is one of the nine flat DXF versions — the ones the Details export writes
/// through a scene and a `QPainter` rather than through a list of pieces.
auto IsFlatDxf(LayoutExportFormats format) -> bool
{
    switch (format)
    {
        case LayoutExportFormats::DXF_AC1006_Flat:
        case LayoutExportFormats::DXF_AC1009_Flat:
        case LayoutExportFormats::DXF_AC1012_Flat:
        case LayoutExportFormats::DXF_AC1014_Flat:
        case LayoutExportFormats::DXF_AC1015_Flat:
        case LayoutExportFormats::DXF_AC1018_Flat:
        case LayoutExportFormats::DXF_AC1021_Flat:
        case LayoutExportFormats::DXF_AC1024_Flat:
        case LayoutExportFormats::DXF_AC1027_Flat:
            return true;
        default:
            return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/// The flat DXF path: the pieces become items in a throwaway scene, exactly as the Details export
/// builds them, and the engine renders that scene. The pieces carry their placement in their own
/// matrix, so nothing has to be moved here — an item built by `GetItem` is already mapped.
auto WriteFlatDxf(VLayoutExporter &exporter, const QVector<VLayoutPiece> &pieces,
                  const OExportCuttingFile::Options &options) -> bool
{
    QGraphicsScene scene;
    QList<QGraphicsItem *> items;
    items.reserve(pieces.size());

    for (const VLayoutPiece &piece : pieces)
    {
        QGraphicsItem *item = piece.GetItem(options.textAsPaths, options.togetherWithNotches, false);
        if (item == nullptr)
        {
            continue;
        }

        scene.addItem(item);
        items.append(item);
    }

    if (items.isEmpty())
    {
        qWarning() << "OExportCuttingFile: no piece could be drawn";
        return false;
    }

    exporter.SetDxfVersion(FlatDxfVersion(options.format));
    exporter.ExportToFlatDXF(&scene, items);
    return true;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingFile::PlansOf(const OExportOutput &output) -> QVector<OExportCuttingGuard::Material>
{
    QVector<OExportCuttingGuard::Material> plans;

    // Ce qui n'est pas un plan de coupe ne porte aucune nappe — un run d'impression n'a pas de « plan
    // de tissu », et lui en prêter un ferait écrire un fichier de découpe pour un PDF.
    if (not output.cuttingPlan)
    {
        return plans;
    }

    plans.append(OExportCuttingGuard::Material::Fabric);
    if (output.liningPlan)
    {
        plans.append(OExportCuttingGuard::Material::Lining);
    }

    return plans;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingFile::PlanName(const OExportOutput &output, OExportCuttingGuard::Material material) -> QString
{
    if (output.name.isEmpty())
    {
        return {};
    }

    // **Un seul plan : le nom de la sortie, tel quel.** C'est ce qu'un plan de coupe s'appelait avant
    // la doublure, et rien ne justifie de le renommer aujourd'hui.
    if (not output.liningPlan)
    {
        return output.name;
    }

    // ⚠️ **Traduit**, et le contexte est nommé à la main : un espace de noms n'a pas de `tr()` — voir
    // l'en-tête. C'est le mot que l'utilisatrice lit sur son disque.
    //
    // ⚠️ **Deux appels, jamais un littéral passé par une variable** : c'est le texte EN CLAIR dans
    // l'appel que l'extracteur relève. Un `const char *` choisi avant, ou un littéral dans un
    // opérateur ternaire à l'intérieur de l'appel, et les deux mots ne seraient plus extractibles —
    // ils resteraient anglais en silence, ce qui est exactement ce qu'on répare ici.
    const QString cloth = material == OExportCuttingGuard::Material::Lining
                              ? QCoreApplication::translate("OExportCuttingFile", "Lining")
                              : QCoreApplication::translate("OExportCuttingFile", "Fabric");

    return QStringLiteral("%1-%2").arg(output.name, cloth);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingFile::Write(const QString &path, const OExportOutput &output, const Options &options,
                               OExportCuttingGuard::Material material) -> bool
{
    // **La nappe demandée, et rien d'autre.** Le plan de tissu est là où il a toujours été ; celui de
    // doublure vit à côté de lui, et une génération qui n'en porte pas n'a rien à écrire pour cette
    // matière-là : un fichier sans pièce serait pire que pas de fichier du tout.
    const bool lining = material == OExportCuttingGuard::Material::Lining;
    const QVector<OExportSizeLayer> &layers = lining ? output.liningLayers : output.layers;
    const OExportNestingResult &result = lining ? output.liningResult : output.result;

    const bool carried = lining ? output.liningPlan : output.cuttingPlan;
    if (path.isEmpty() || not carried || not result.ok || layers.isEmpty())
    {
        qWarning() << "OExportCuttingFile: the generation cannot be reproduced";
        return false;
    }

    const QVector<VLayoutPiece> pieces = PlacedPieces(layers, result);
    if (pieces.isEmpty())
    {
        qWarning() << "OExportCuttingFile: the plan holds no piece to write";
        return false;
    }

    VLayoutExporter exporter;
    exporter.SetFileName(path);
    exporter.SetImageRect(PlanRect(result));

    // ── 100 %, et rien d'autre ───────────────────────────────────────────────────────────────────
    //
    // CuttingPlan.md §24.2 — the one difference with the Details export: a cutting plan leaves at
    // 100 %, always, because it is a file a machine cuts. The engines multiply by these two, so 1.0
    // is the whole of the rule.
    exporter.SetXScale(1.0);
    exporter.SetYScale(1.0);

    exporter.SetShowGrainline(options.showGrainline);
    exporter.SetBoundaryTogetherWithNotches(options.togetherWithNotches);
    exporter.SetBinaryDxfFormat(options.binary);
    exporter.SetDxfApparelCompatibility(options.compatibility);
    exporter.SetShowMainPath(options.showMainPath);

    // The two pen settings are the application's, not the dialog's — the Details export reads them
    // the same way, and HP-GL is the only format they reach.
    VValentinaSettings const *settings = VAbstractValApplication::VApp()->ValentinaSettings();

    switch (options.format)
    {
        case LayoutExportFormats::DXF_AAMA:
            exporter.SetDxfVersion(DRW::AC1009);
            exporter.ExportToAAMADXF(pieces);
            break;
        case LayoutExportFormats::DXF_ASTM:
            exporter.SetDxfVersion(DRW::AC1009);
            exporter.ExportToASTMDXF(pieces);
            break;
        case LayoutExportFormats::HPGL:
        case LayoutExportFormats::HPGL_PLT:
            exporter.SetSingleLineFont(settings->GetSingleLineFonts());
            exporter.SetSingleStrokeOutlineFont(settings->GetSingleStrokeOutlineFont());
            exporter.SetPenWidth(qCeil(settings->WidthHairLine()));
            exporter.ExportToHPGL(pieces);
            break;
        case LayoutExportFormats::HPGL2:
        case LayoutExportFormats::HPGL2_PLT:
            exporter.SetSingleLineFont(settings->GetSingleLineFonts());
            exporter.SetSingleStrokeOutlineFont(settings->GetSingleStrokeOutlineFont());
            exporter.SetPenWidth(qCeil(settings->WidthHairLine()));
            exporter.ExportToHPGL2(pieces);
            break;
        default:
            if (not IsFlatDxf(options.format))
            {
                qWarning() << "OExportCuttingFile: unknown format" << static_cast<int>(options.format);
                return false;
            }
            if (not WriteFlatDxf(exporter, pieces, options))
            {
                return false;
            }
            break;
    }

    // ⚠️ The engines report a failure by logging it and returning nothing — `ExportToAAMADXF` and
    // the two HP-GL calls return void, and a `qCritical` is all the user would never see. What can
    // be told, and is: the file is there and it holds something.
    const QFileInfo written(path);
    if (not written.exists() || written.size() == 0)
    {
        qWarning() << "OExportCuttingFile: nothing was written to" << path;
        return false;
    }

    return true;
}
