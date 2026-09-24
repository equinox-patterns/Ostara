/************************************************************************
 **
 **  @file   oexportfragment.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Splitting a piece on the mat grid — the puzzle, cut by the
 **          cells of a domestic plotter's mat.
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

#ifndef OEXPORTFRAGMENT_H
#define OEXPORTFRAGMENT_H

#include "oexportplotter.h"

#include <QPointF>
#include <QVector>

/**
 * @brief One piece cut into the parts a mat can hold.
 *
 * A piece larger than a mat cannot be cut on a domestic plotter — the machine's software refuses a
 * shape that overflows its mat. It is therefore cut **at the mat borders**, and the parts are put
 * back together like a puzzle. This is where that cut happens.
 *
 * Every part is a closed outline of its own: the stretch of the piece's cut line that falls in one
 * cell, joined along the cell's own edges. Side by side the parts share those edges exactly — the
 * cut is the same line seen from both sides — so a piece put back together has its own size.
 *
 * The piece's own lines — its internal paths, its grainline and its fold line — are cut the same way
 * and handed back with the part that holds them, so that a part carries what the piece carried: its
 * construction lines, the grain direction, and where it lies on the fold. They are LINES and stay
 * lines: the fragment of a grainline is a shorter grainline.
 *
 * ⚠️ **The internal paths are carried, and ALL of them** — including the ones the model flags as cut
 * paths. Whether a line is cut or drawn is not Ostara's to decide: the plotter's own software
 * assigns that to each line of the file, and Ostara's part is to hand it the geometry. Filtering
 * here would take the choice away from the only place entitled to make it.
 *
 * The arithmetic lives in OExportPlotter and this is only its geometry: the turn below is the one
 * the plan decided, applied here so that the cuts and the outline cannot end up in two frames.
 */
namespace OExportFragment
{
    /// One part of a piece: what is left of it inside one cell of the mat grid.
    struct Part
    {
        /// The cut line of this part, closed, in the piece's own turned frame.
        QVector<QPointF> outline{};

        /// The pieces of the piece's own lines that fall inside the SAME cell, each an open
        /// polyline. Empty when none does — most parts of most pieces.
        QVector<QVector<QPointF>> lines{};
    };

    /// @brief Cut @p outline — and the piece's @p lines — on the cells of @p plan.
    ///
    /// @p outline is the piece's cut line, closed, in the piece's own pixels. @p lines are the
    /// piece's own lines, each an open polyline — its internal paths, its grainline, its fold line.
    /// Nothing is returned when the piece holds no geometry at all: an empty part is not a part.
    ///
    /// A piece with a C shape can leave more than one part in the SAME cell. Each is a part in its
    /// own right — the machine would cut them as two shapes — and a line goes to the one that
    /// actually holds it.
    auto Split(const QVector<QPointF> &outline, const QVector<QVector<QPointF>> &lines,
               const OExportPlotter::PiecePlan &plan) -> QVector<Part>;
}

#endif // OEXPORTFRAGMENT_H
