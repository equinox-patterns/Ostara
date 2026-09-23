/************************************************************************
 **
 **  @file   oexportcuttingdirection.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Export mode — which rotations a piece may take, from its direction.
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

#ifndef OEXPORTCUTTINGDIRECTION_H
#define OEXPORTCUTTINGDIRECTION_H

#include "../vpatterndb/floatItemData/floatitemdef.h"

#include <QVector>
#include <QtGlobal>

struct OPieceGroup;
class VLayoutPiece;

/**
 * @brief The rotations a piece may take, read from its direction.
 *
 * Fabric has a grain, and a piece laid on it follows that grain. The nesting cannot invent the
 * direction — the guard refuses to start without one — and it must not be free to lay the piece
 * however it likes either: the piece's direction says which rotations keep it on the grain.
 *
 * This is a business rule, so it lives on its own: no widget, no scene, nothing but the arithmetic
 * that decides an angle. It is also the one place in this feature where a sign can be wrong in a
 * way nothing on screen would show — a piece laid across the grain looks like a piece laid right.
 */
namespace OExportCuttingDirection
{
/// A piece's direction, and what it allows.
struct Direction
{
    bool known{false};

    /// The angle of the direction in the pattern's frame, in degrees, in **Qt's convention**:
    /// counter-clockwise as seen on screen, so 90° points up. See AllowedAngles for the sign.
    qreal angle{0.0};

    GrainlineArrowDirection arrows{GrainlineArrowDirection::twoWaysUpDown};
};

/// @brief The fold line of a piece, in the piece's own frame, or a null line when it has none.
///
/// **The ONE reading of the fold**, and this feature has paid for the lesson twice: what says a
/// piece has a fold is the LINE and never its marking's type (`FoldLineType` is born at
/// `TwoArrowsTextAbove`, so "the type is not None" is true of every piece ever made), and the two
/// readers of that line must read the SAME one — a guard that tested the seam-allowance line while
/// the nesting read the seam line let a piece through with no constraint at all, and the plan laid
/// it at random without saying so.
auto FoldLine(const VLayoutPiece &piece) -> QLineF;

/// @brief The same line, in the frame the contour is drawn and nested in.
///
/// ⚠️ **Not the same line when the piece carries a mirror.** `Force flipping` and the symmetrical
/// copy put a reflection in the piece's MATRIX, and the matrix does not touch the stored fold line:
/// reading the line unmapped while the contour is mapped gives a fold that no longer falls on its
/// own piece's edge.
auto MappedFoldLine(const VLayoutPiece &piece) -> QLineF;

/// @brief The one rotation that lays a piece cut ON THE FOLD on folded fabric, when there is one.
///
/// A folded plan lays the half, and its fold line has to sit exactly on the fabric's fold — so the
/// only rotation that will do is the one that brings that line **vertical** with the piece's body on
/// the fabric's side. Of the two rotations a fold axis allows (`{base, base + 180}`), **one** does:
/// the other lays the body outside the cloth.
///
/// The fold wins over the grainline here, which is the opposite of @ref PieceDirection — and it is
/// not a contradiction: a piece cut on the fold is cut ON THE FOLD, so it is the fold of the fabric
/// that carries it. Its grainline, if it has one, follows.
///
/// @param angle set to the rotation, in the engine's convention, when the answer is true.
/// @return false when the piece has no fold, or when its fold line is not an edge of its own
///         outline — in which case there is no rotation to find, and the caller has to say so
///         rather than lay the piece somewhere.
auto FoldAngle(const VLayoutPiece &piece, double *angle) -> bool;

/// @brief The direction of a piece: its own grainline when it is switched on, its fold axis otherwise.
///
/// ⚠️ **Never call this on a member of a piece group.** A grouped member carries no direction of its
/// own — the group hosts one for all of them — but `VLayoutPiece::Create` does not mask the member's
/// grainline: it reads `IsEnabled()` and nothing else. The masking is done on the ITEM, not on the
/// data, so the flag can be true while the direction belongs to the group. Use @ref GroupDirection.
auto PieceDirection(const VLayoutPiece &piece) -> Direction;

/// @brief The direction of a group — its grainline, whose rotation is a literal and not a formula.
auto GroupDirection(const OPieceGroup &group) -> Direction;

/// @brief The direction of a piece mirrored about a **vertical** axis.
///
/// ⚠️ **Mirroring a piece changes the angle of its direction, and nothing said so.** The mirror is
/// put on the geometry — in the matrix — while the angle is read on the piece's stored grainline,
/// which the matrix does not touch. So a mirrored copy kept the original's angle: a 45° bias became
/// 135° on the copy, and the copy was turned as though it were still at 45° — its grain read
/// **horizontal** where it had to read vertical. The plan looked right; only the grainline said
/// otherwise. Found by Sophie, not by me.
///
/// A reflection about a vertical axis sends an angle @c a to @c 180 - a, whichever axis the
/// direction came from — a piece's own grainline or its group's.
auto Mirrored(const Direction &direction) -> Direction;

/// @brief The rotations that lay @p direction along the fabric's length, in the engine's convention.
///
/// The fabric is sold on a bolt: its grain runs the length of it, which is the bin's unbounded
/// direction. So the piece's direction has to come out **vertical**.
///
/// ⚠️ **The sign is the whole of it.** `QLineF::angle()` is counter-clockwise as seen on screen —
/// `VPieceGrainline` builds its line with `pt2.y = p1.y - len * sin(a)`. The nesting engine's
/// `rotatePath()` is the other way round (`y' = x * sin + y * cos`, so a positive angle turns
/// clockwise on screen). The two conventions are opposite, which is why this reads `g - 90` and not
/// `g + 90`: laying a direction at @p g along the vertical takes `g - 90`, and `g + 90` is the same
/// orientation turned a half-turn.
///
/// An empty result means the piece has no direction at all — the nesting then keeps its freedom,
/// which is what the guard already refused to allow for a cutting plan.
/// @param oneWay the run's own option: no piece is turned a half-turn. **It removes a SENSE, not an
///        axis** — the fabric's grain has a direction, and on a napped or printed cloth every piece
///        has to take it the same way. So a direction that allowed two senses keeps one, and a
///        direction that allowed two AXES keeps both, one sense each: a "Double" piece may still be
///        cut droit-fil or contre-fil, it simply may not be laid upside down.
auto AllowedAngles(const Direction &direction, bool oneWay = false) -> QVector<double>;
} // namespace OExportCuttingDirection

#endif // OEXPORTCUTTINGDIRECTION_H
