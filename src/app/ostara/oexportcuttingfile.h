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

#include <QString>

#include "../vdxf/dxfdef.h"
#include "../vlayout/vlayoutdef.h"

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

/// @brief Writes the placed plan to @p path. False when nothing could be written — the reason has
/// been logged, and the caller says so to the user.
auto Write(const QString &path, const OExportOutput &output, const Options &options) -> bool;
} // namespace OExportCuttingFile

#endif // OEXPORTCUTTINGFILE_H
