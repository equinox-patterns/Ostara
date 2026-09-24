/************************************************************************
 **
 **  @file   ochildanchor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Re-derives the position of a piece's or a group's children
 **          (grainline, information blocks, annotations) against a
 **          reference rect, so that they follow a change of active size.
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

#ifndef OCHILDANCHOR_H
#define OCHILDANCHOR_H

#include <QPointF>
#include <QRectF>

#include "onormalizedanchor.h"

class OAnnotationData;
class OInfoBlockData;
class OThumbnailData;
struct OPieceGroup; // a struct, and -Wmismatched-tags is an error here
class VContainer;
class VGrainlineData;
class VPiece;

/**
 * @brief The children of a piece or of a group, placed once and following the size.
 *
 * Every function here is pure: no GUI, no scene, no rendering. The reference rect
 * comes from the pattern's node points, which is why the result is the same in
 * Details, in a headless export and in the save pass.
 */
namespace OChildAnchor
{
/**
 * @brief Reference rect of a piece: the bounding box of its NodePoint nodes.
 *
 * Deliberately neither the seam allowance nor the rendered outline: both change
 * with display preferences — "Seam allowance" and "Boundary together with
 * notches" — and a recorded placement must not move because a preference was
 * toggled. Arcs and splines are ignored, like for the default grainline centre.
 *
 * @return An empty rect when the piece has no usable node, in which case nothing
 *         is anchored and the absolute positions are left alone.
 */
auto RefRect(const VPiece &piece, const VContainer *data) -> QRectF;

/**
 * @brief Reference rect of a group: the union of its members' node boxes.
 *
 * @param origin Origin of the holder's local frame, which is the frame the
 *               group's children are stored in.
 */
auto RefRect(const OPieceGroup &group, const VContainer *data, const QPointF &origin) -> QRectF;

/**
 * @brief Normalized position of @p absolute against @p ref.
 * @return An invalid anchor when @p ref cannot carry a ratio.
 */
auto FromAbsolute(const QPointF &absolute, const QRectF &ref) -> ONormalizedAnchor;

/**
 * @brief Absolute position an anchor resolves to.
 * @return @p fallback when the anchor is invalid or @p ref cannot carry a ratio —
 *         the position is then left exactly where it is.
 */
auto Resolve(const ONormalizedAnchor &anchor, const QRectF &ref, const QPointF &fallback) -> QPointF;

/**
 * @brief Adopt an anchor for a child that has none, then resolve its position.
 *
 * Adoption never moves the child: the anchor is derived from the position the
 * child already has, against the rect in hand, so the first refresh after opening
 * a file is invisible.
 *
 * @return true when the child changed — a new anchor, or a resolved position that
 *         differs. False lets the caller skip writing the model back entirely,
 *         which is what keeps a refresh over a large pattern cheap.
 */
auto Materialize(VGrainlineData &grainline, const QRectF &ref) -> bool;
auto Materialize(OInfoBlockData &blocks, const QRectF &ref) -> bool;
auto Materialize(OAnnotationData &annotations, const QRectF &ref) -> bool;
auto Materialize(OThumbnailData &thumbnail, const QRectF &ref) -> bool;

/// Every child of @p piece / @p group, in one call. @return true when something changed.
auto Materialize(VPiece &piece, const QRectF &ref) -> bool;
auto Materialize(OPieceGroup &group, const QRectF &ref) -> bool;

/**
 * @brief Record an anchor for every child the user actually moved.
 *
 * Compares @p before and @p after child by child: the ones whose position changed
 * get the anchor of their new position, the others keep theirs. A child that moved
 * always has its anchor overwritten — left stale, it would pull the child back at
 * the next refresh and undo the drag.
 *
 * @return true when something changed.
 */
auto SyncFollowedChildren(const VPiece &before, VPiece &after, const QRectF &ref) -> bool;
auto SyncFollowedChildren(const OPieceGroup &before, OPieceGroup &after, const QRectF &ref) -> bool;
} // namespace OChildAnchor

#endif // OCHILDANCHOR_H
