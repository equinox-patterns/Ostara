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
auto PlanRect(const OExportOutput &output) -> QRectF
{
    return {0.0, 0.0, output.result.binWidthPx, output.result.usedHeightPx};
}

//---------------------------------------------------------------------------------------------------------------------
/// The plan's pieces, PLACED — the canvas's own composition, and the only place it is written.
///
/// ⚠️ The placement goes into the piece's OWN matrix, composed with the one it already carries —
/// never into an item's transform, and never by overwriting: the Details export learned that a
/// transform on the item drags the bounding rect along with it, and that overwriting throws away
/// the mirror a symmetric copy is made of.
///
/// ⚠️ And it is the same loop as `OExportCanvas::Build`, on the same two lists (`result.keys` and
/// `result.placements` are index-aligned): the file is what the screen shows, or it is wrong.
auto PlacedPieces(const OExportOutput &output) -> QVector<VLayoutPiece>
{
    QVector<VLayoutPiece> placed;
    const OExportNestingResult &result = output.result;

    for (int i = 0; i < result.placements.size() && i < result.keys.size(); ++i)
    {
        const OExportPlacement &placement = result.placements.at(i);
        const QString &key = result.keys.at(i);

        for (const OExportSizeLayer &layer : output.layers)
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
auto OExportCuttingFile::Write(const QString &path, const OExportOutput &output, const Options &options) -> bool
{
    if (path.isEmpty() || not output.result.ok || output.layers.isEmpty())
    {
        qWarning() << "OExportCuttingFile: the generation cannot be reproduced";
        return false;
    }

    const QVector<VLayoutPiece> pieces = PlacedPieces(output);
    if (pieces.isEmpty())
    {
        qWarning() << "OExportCuttingFile: the plan holds no piece to write";
        return false;
    }

    VLayoutExporter exporter;
    exporter.SetFileName(path);
    exporter.SetImageRect(PlanRect(output));

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
