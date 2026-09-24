/************************************************************************
 **
 **  @file   oexportmatpacking.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Fitting the parts of a plotter run onto mats — as few mats
 **          as their boxes allow.
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

#ifndef OEXPORTMATPACKING_H
#define OEXPORTMATPACKING_H

#include "oexportplotter.h"

#include <QRectF>
#include <QSize>
#include <QVector>

/**
 * @brief Fitting the parts of a plotter run onto mats — as few mats as possible.
 *
 * A mat is a hard cell, and the plotter's own software cannot be handed a shape that overflows one.
 * Every part of a run is therefore smaller than a mat — that is what cutting the pieces bought —
 * and the question left is **how many mats they need**. One mat per part, or one block of mats per
 * piece, leaves whole mats empty: a sliver of 6,58 cm holds a 29,21 cm board to itself.
 *
 * So the parts are packed, and what is packed is their **box** — not their outline. Two curved
 * parts that would dovetail are still counted as their two rectangles, and that is a decision
 * rather than an oversight: packing outlines is the nesting engine's trade, and a plotter run
 * deliberately does not do it. The bound it leaves is honest and worth knowing — only parts that
 * are small in a direction can share a mat, so the gain is real without being the last mat there
 * is to win.
 *
 * The method is the oldest shelf there is: the tallest box first, laid left to right, a new shelf
 * when the row is full, a new mat when the shelves are. A quarter turn is allowed and taken only
 * when it lowers the box, because a lower shelf carries more.
 *
 * Nothing here knows a piece, a file or a scene: it takes boxes and gives seats back, so the
 * arithmetic can be read — and checked — on its own.
 */
namespace OExportMatPacking
{
    /// One box to fit, in pixels. Two doubles rather than a QRectF: a box here has no position yet,
    /// and where it goes is the whole of what this is asked.
    struct Item
    {
        double widthPx{0.0};
        double heightPx{0.0};
    };

    /// Where a box was laid: which mat, and the corner of the rectangle it OCCUPIES on it.
    ///
    /// ⚠️ **The rectangle occupied, never the corner of the shape's own box.** With @ref turned
    /// set, the two are not the same point — the occupied rectangle is then the box's height by its
    /// width — and only the caller knows what to add. See OExportPlotterLayout, where the turn
    /// becomes the placement's rotation and Qt's own direction has to be paid for.
    struct Seat
    {
        int mat{0};           ///< 0-based, in the order the mats were opened
        double x{0.0};        ///< the occupied rectangle's left edge, inside the mat
        double y{0.0};        ///< its top edge
        bool turned{false};   ///< laid a quarter turn: the rectangle is the box turned over
    };

    /// One seat per item, **in the order the items came in** — and how many mats were opened.
    struct Packing
    {
        QVector<Seat> seats{};
        int mats{0};
    };

    /// @brief Fit @p items on mats of @p mat, opening as few as the boxes allow.
    ///
    /// @pre Every item fits a mat as drawn: the geometry guarantees it, a part being cut on a cell
    /// the size of the mat. An item that did not would be seated anyway, overflowing its mat — this
    /// has no opinion to give, and refusing a plan is the caller's business.
    ///
    /// The answer is the same every time: the sort order is total, and nothing is searched.
    auto Pack(const QVector<Item> &items, const OExportPlotter::Mat &mat) -> Packing;

    /// @brief How many mats to lay, and in how many columns, to be as near square as they go.
    ///
    /// The mats have **no reading order**: one is loaded at a time, and the plotter's software reads
    /// the objects on the file, never the tiles they were laid in. The arrangement is therefore free
    /// and the only thing to get right is not to draw a strip — twelve mats in a single column is a
    /// canvas seven metres tall.
    ///
    /// Zero mats give an empty size, and the caller has nothing to draw.
    auto Arrangement(int mats) -> QSize;

    /// @brief The rectangle mat @p index covers once the mats are laid in @p arrangement.
    ///
    /// ⚠️ **One definition, two readers** — the layout places a part on its mat with it, and the
    /// canvas draws the mats with it. Written twice, the two would drift the day the arrangement
    /// changes, and a plan would show its parts on mats that are not where they were put.
    auto TileRect(int index, const QSize &arrangement, double matWidthPx, double matHeightPx) -> QRectF;
} // namespace OExportMatPacking

#endif // OEXPORTMATPACKING_H
