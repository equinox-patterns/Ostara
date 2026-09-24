/************************************************************************
 **
 **  @file   oexportplotter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  The mat a domestic plotter cuts on, and what one piece makes
 **          of it — how many parts, and where the cuts fall.
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

#ifndef OEXPORTPLOTTER_H
#define OEXPORTPLOTTER_H

#include <QRectF>
#include <QTransform>
#include <QVector>

/**
 * @brief The mat a domestic plotter cuts on, and what one piece makes of it.
 *
 * A domestic plotter — Cricut, Silhouette and their like — does not work on a continuous length.
 * Its working surface is a MAT, a hard cell: nothing longer than it can be cut, and the software
 * refuses a shape that overflows. A piece bigger than a mat is therefore **split at the mat
 * borders**, and its parts are put back together like a puzzle.
 *
 * Two things follow, and they are the whole of this class:
 *
 *   1. <b>how many parts</b> — the piece's box divided by the mat, rounded up in each direction.
 *      This is not a packing problem: nothing is searched, and the count is
 *      `ceil(w / matW) × ceil(h / matH)`. What is minimised is the number of PARTS, not the
 *      surface used.
 *   2. <b>where the cuts fall</b> — at multiples of the mat measured **from the piece's own box
 *      corner**, never from the canvas. That is what makes the count above exact with no placement
 *      search at all: the absolute position is free, the cut frame is anchored to the piece.
 *
 * A quarter turn is allowed, and it is the PIECE that turns, not the mat — a 55 × 25 piece does not
 * fit a 29,21 wide mat, while the same piece turned does. It is taken when, and only when, it
 * lowers the number of parts.
 *
 * The arithmetic here is deliberately readable on its own: a count that can be wrong is a plan that
 * cuts wrong, and none of it needs a scene, a document or a machine to be checked.
 */
class OExportPlotter
{
public:
    /// The mat's usable area, in pixels. What the user reads in her plotter's own software.
    struct Mat
    {
        double widthPx{0.0};
        double heightPx{0.0};

        auto IsValid() const -> bool { return widthPx > 0.0 && heightPx > 0.0; }
    };

    /// What one piece makes of one mat.
    struct PiecePlan
    {
        /// The quarter turn to apply to the piece's points, about @ref box's own top-left. Identity
        /// when the piece is left as drawn.
        ///
        /// Handed back as a transform rather than as a flag on purpose: the contour is turned *and*
        /// the cuts are laid in the turned box, and these two halves must not be able to disagree.
        QTransform turn{};

        /// The piece's box **after** @ref turn — the frame @ref cuts live in.
        QRectF box{};

        int columns{1};
        int rows{1};

        /// The `columns × rows` rectangles the piece is cut by, in @ref box. They tile the box
        /// exactly — side by side, no gap and no overlap — because the cut line is SHARED: a piece
        /// put back together from two parts that did not share it would not have its own size.
        ///
        /// A rectangle can hold no geometry of the piece at all, if the outline does not reach it.
        /// It is still laid, and it is the caller that finds nothing to cut there.
        QVector<QRectF> cuts{};
    };

    /// @brief Lay @p box on @p mat — how many parts, which way round, and where the cuts fall.
    ///
    /// @p box is the piece's own bounding box, in the piece's own pixels. Its POSITION is what the
    /// cuts are measured from, never the canvas's: two pieces of the same size are cut the same way
    /// wherever they are laid, and that is the whole point.
    ///
    /// @p mat invalid, or @p box empty, gives one part covering @p box — no split. A caller that
    /// reaches this with no mat has a run to refuse, not a plan to draw.
    static auto Plan(const QRectF &box, const Mat &mat) -> PiecePlan;

private:
    /// How many cells of @p cellPx fit in @p lengthPx — and never a cell more for a length that only
    /// looks longer because a float said so.
    static auto CellCount(double lengthPx, double cellPx) -> int;
};

#endif // OEXPORTPLOTTER_H
