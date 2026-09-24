/************************************************************************
 **
 **  @file   oexportpuzzlereference.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  The reassembly marks: one shape straddling every cut, so
 **          that the parts of a piece can be put back together.
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

#ifndef OEXPORTPUZZLEREFERENCE_H
#define OEXPORTPUZZLEREFERENCE_H

#include "oexportplotter.h"

#include <QPointF>
#include <QTransform>
#include <QVector>

/**
 * @brief The marks that make a cut piece reassemblable — one shape across every cut.
 *
 * A piece too big for a mat is cut into parts, and **once cut, nothing on the material says which
 * part goes beside which**. The name travels in the file's group id, not on the fabric. So a mark is
 * drawn astride each cut: the plotter cuts the line, each part keeps **half** the mark, and two
 * halves make a whole shape at one position only — the right neighbour, at the right place.
 *
 * **A shape, and not a stroke.** A straight tick would slide: pushed up or down along the cut, a
 * tick still faces its other half, and nothing would say where it belongs. A shape reforms at a
 * single offset, which is the whole of what these marks are for.
 *
 * **A repertoire, and not one shape.** A piece cut three ways has three cuts, and three identical
 * circles would say "a cut is here" three times without ever saying which of the three. So the
 * shapes come in an order — circle first, then square, then triangle — and a part showing half a
 * circle on one edge and half a square on another knows which of its edges goes where.
 *
 * A **cut** is a straight line of the mat grid, not one segment of it: the parts meeting along the
 * same line carry the same shape, whether that line spans one cell or three. The order is fixed —
 * vertical cuts left to right, then horizontal cuts top to bottom — so that two exports of the same
 * pattern carry the same marks and can be read against each other.
 *
 * The marks are ordinary LINES and travel with the piece's other lines — the cut itself divides
 * them, so each part receives its half with no extra work, and they reach the screen and the file
 * through the one channel everything else already uses.
 *
 * Nothing here decides whether a mark is cut or drawn: that belongs to the plotter's own software,
 * and the file carries the geometry either way.
 */
namespace OExportPuzzleReference
{
    /// @brief One mark across each interior cut of @p plan, in the piece's own pixels.
    ///
    /// @p outline is the piece's cut line, closed, **in the piece's own frame** — the frame @p turn
    /// leaves, and the one OExportFragment::Split expects its lines in. @p sizePx is the size of a
    /// mark: the box every shape of the repertoire is drawn in, the circle as much as the square.
    ///
    /// A mark is placed only where it falls **entirely inside the piece**, which is exactly where the
    /// piece exists on BOTH sides of the cut. Anywhere else it would be whole on one part and absent
    /// from the other, and a mark that comes back in one piece has nothing to say.
    ///
    /// A piece held in a single cell has no cut and gets nothing.
    auto Marks(const QVector<QPointF> &outline, const QTransform &turn,
               const OExportPlotter::PiecePlan &plan, double sizePx) -> QVector<QVector<QPointF>>;
} // namespace OExportPuzzleReference

#endif // OEXPORTPUZZLEREFERENCE_H
