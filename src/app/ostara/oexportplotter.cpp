/************************************************************************
 **
 **  @file   oexportplotter.cpp
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

#include "oexportplotter.h"

#include <algorithm>
#include <cassert>
#include <cmath>

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlotter::CellCount(double lengthPx, double cellPx) -> int
{
    assert(cellPx > 0.0);

    // A length that is a whole number of cells must give exactly that number: a box 1104 px wide on
    // a 1104 px mat is ONE cell, and only a float's last bit says otherwise. The tolerance is a
    // millionth of a pixel — far under anything a pattern can mean, and far over what one division
    // can drift by. It is what keeps a piece that fits its mat from costing a second mat.
    constexpr double kTolerance = 1e-6;

    const double cells = lengthPx / cellPx;
    return std::max(1, static_cast<int>(std::ceil(cells - kTolerance)));
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlotter::Plan(const QRectF &box, const Mat &mat) -> PiecePlan
{
    PiecePlan plan;

    if (!mat.IsValid() || box.isEmpty())
    {
        plan.box = box;
        plan.cuts.append(box);
        return plan;
    }

    const int columnsAsDrawn = CellCount(box.width(), mat.widthPx);
    const int rowsAsDrawn = CellCount(box.height(), mat.heightPx);

    const int columnsTurned = CellCount(box.height(), mat.widthPx);
    const int rowsTurned = CellCount(box.width(), mat.heightPx);

    // The turn is taken when it lowers the NUMBER OF PARTS. On a tie the piece is left as drawn:
    // turning a piece that gains nothing would only make the plan harder to read against the model,
    // and a pattern is read against its model.
    const bool turned = columnsTurned * rowsTurned < columnsAsDrawn * rowsAsDrawn;

    // A quarter turn about the box's own top-left, so that the turned box shares that corner and the
    // cuts below need no translation of their own. Which way round the turn lands — and it lands up
    // and to the left of that corner — is nobody's business but this function's: everything
    // downstream reads `box`, and the placement normalises the rest.
    const QPointF corner = box.topLeft();
    plan.turn = turned ? QTransform().translate(corner.x(), corner.y())
                             .rotate(90)
                             .translate(-corner.x(), -corner.y())
                       : QTransform();

    plan.columns = turned ? columnsTurned : columnsAsDrawn;
    plan.rows = turned ? rowsTurned : rowsAsDrawn;
    plan.box = plan.turn.mapRect(box);

    plan.cuts.reserve(plan.columns * plan.rows);
    for (int row = 0; row < plan.rows; ++row)
    {
        for (int column = 0; column < plan.columns; ++column)
        {
            const double x0 = plan.box.left() + column * mat.widthPx;
            const double y0 = plan.box.top() + row * mat.heightPx;
            const double x1 = std::min(x0 + mat.widthPx, plan.box.right());
            const double y1 = std::min(y0 + mat.heightPx, plan.box.bottom());

            // The last column and the last row fall short of a whole mat whenever the piece is not
            // an exact multiple of it — the sliver. It is kept as it is: the goal is the fewest
            // parts, not the prettiest ones, and a slice is a part like any other.
            plan.cuts.append(QRectF(x0, y0, x1 - x0, y1 - y0));
        }
    }

    return plan;
}
