/************************************************************************
 **
 **  @file   oexportplotterlayout.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Laying a plotter run out: the parts packed onto mats, and the
 **          mats laid out to be read.
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

#ifndef OEXPORTPLOTTERLAYOUT_H
#define OEXPORTPLOTTERLAYOUT_H

#include "oexportnesting.h"
#include "oexportplotter.h"

#include <QVector>

/**
 * @brief Where a plotter run puts what it has.
 *
 * This is the whole of what the mode calls « nesting » for a domestic plotter, and it is not the
 * nesting engine. **The part is what gets laid, not the piece**: a piece too big for a mat was cut
 * into parts, and those parts go onto mats wherever they fit. What is minimised is the number of
 * MATS — the number of times the user loads the machine — and OExportMatPacking is where that is
 * decided, on the parts' boxes.
 *
 * Two things are this file's own, and neither is arithmetic:
 *
 *   1. <b>the mats on the canvas.</b> The mats have no reading order — see OExportMatPacking's
 *      Arrangement — so they are laid as near square as they go, which keeps a twelve-mat run from
 *      being a strip seven metres tall.
 *   2. <b>the quarter turn.</b> A part laid turned is expressed as the placement's `rotation`, not
 *      baked into its geometry: the geometry is built before anything is packed, so the orientation
 *      cannot be known there. ⚠️ And Qt turns about the anchor and lands the shape to its LEFT,
 *      which is a whole box's width of offset to pay — the one line in here that a sign error would
 *      hide, since a part laid outside its mat looks like a part laid anywhere.
 *
 * The result is the one the canvas already knows how to draw: `keys` and `placements`, index
 * aligned, each key naming the geometry that goes there.
 */
namespace OExportPlotterLayout
{
    /// @brief Pack @p layers onto mats of @p mat, and lay the mats out.
    ///
    /// Only the reference layer is laid — a plotter run is a single size.
    ///
    /// Never fails on the arithmetic: the packing has no opinion to give. It does refuse a run with
    /// no mat, or with nothing in it, because a plan that cannot be laid out must be refused rather
    /// than drawn empty — a file without a single object says nothing to anyone.
    auto Run(const QVector<OExportSizeLayer> &layers, const OExportPlotter::Mat &mat)
        -> OExportNestingResult;
} // namespace OExportPlotterLayout

#endif // OEXPORTPLOTTERLAYOUT_H
