/************************************************************************
 **
 **  @file   onormalizedanchor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Position of a piece or group child, normalized against a
 **          reference rect. This is what makes a child follow a change of
 **          the active size.
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

#ifndef ONORMALIZEDANCHOR_H
#define ONORMALIZEDANCHOR_H

#include <QPointF>

/**
 * @brief Where a child of a piece or of a group sits, in units of a reference rect.
 *
 * A child keeps an absolute position too (mx/my), and that one stays the truth for
 * every reader that knows nothing about sizes: the Graded layers, the SVG and
 * layout exporters, the undo snapshots. This anchor is what makes the child follow
 * a change of the active size — the position is re-derived from the reference rect
 * at every geometry refresh.
 *
 * uv is the child's own origin (its item's pos(), not its visual centre), measured
 * in reference-rect widths and heights, so (0, 0) is the top-left corner of the
 * rect and (1, 1) its bottom-right. Values outside [0, 1] are legitimate: a
 * cartouche placed beside or below the piece is a real placement, and nothing
 * clamps it.
 *
 * valid == false means no deliberate placement is recorded. Two cases, both
 * resolved by OChildAnchor::Materialize: a file written before the anchor existed
 * (one is adopted from the absolute position), or a child that was never moved and
 * still auto-centres.
 */
struct ONormalizedAnchor
{
    bool valid{false};
    QPointF uv{0.5, 0.5};

    friend auto operator==(const ONormalizedAnchor &lhs, const ONormalizedAnchor &rhs) -> bool
    {
        // An invalid anchor carries no placement: its uv is meaningless and must
        // not make two equally unplaced children compare apart.
        return lhs.valid == rhs.valid && (!lhs.valid || lhs.uv == rhs.uv);
    }

    friend auto operator!=(const ONormalizedAnchor &lhs, const ONormalizedAnchor &rhs) -> bool
    {
        return !(lhs == rhs);
    }
};

#endif // ONORMALIZEDANCHOR_H
