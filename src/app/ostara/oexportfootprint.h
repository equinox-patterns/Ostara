/************************************************************************
 **
 **  @file   oexportfootprint.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Export mode — where a footprint sits, and where a hand may put it.
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

#ifndef OEXPORTFOOTPRINT_H
#define OEXPORTFOOTPRINT_H

#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QTransform>

#include "oexportnesting.h"

class QGraphicsItem;

/// @brief The placement formula, and the rigid moves a hand is allowed to make on it.
///
/// `translate(x, y) · rotate(rotation) · translate(-originX, -originY)` is written once, in
/// Matrix() and PieceMatrix(), and nowhere else. The canvas that draws the plan and the
/// correction that moves a piece both come here, so a corrected piece cannot land somewhere
/// the engine's own reading of the same placement would not put it — the two would have to
/// disagree with themselves to disagree with each other. `/ kScale` was already this kind of
/// shared quantity, and it went wrong exactly once, silently.
///
/// A placement is a frame, not a point. Its local (0, 0) is `origin` — the min-bbox corner of
/// the footprint BEFORE rotation — and its local rectangle is `(0, 0, extentX, extentY)`. So
/// the footprint's four corners, its centre and its bounding box all follow from those two
/// fields, and the centre is emphatically NOT `(x, y)`.
///
/// Like OExportArtboards, this file holds no scene and no widget: a quantity that can be
/// wrong arithmetically has to be readable on its own.
namespace OExportFootprint
{
/// @brief The footprint's own frame, mapping a point of the UNROTATED footprint to the scene.
auto Matrix(const OExportPlacement &placement) -> QTransform;

/// @brief The same frame with the piece's alignment shift folded into the inner translate.
///
/// This is what the canvas bakes into a piece before drawing it. The shift belongs to the
/// origin and not after the rotation: it is part of where the piece sits in the document,
/// and the engine measured the footprint with it already applied.
auto PieceMatrix(const OExportPlacement &placement, const QPointF &alignDelta) -> QTransform;

/// @brief The placed footprint's centre, in scene coordinates.
///
/// NOT `(x, y)`, which is a corner. Rotating about `(x, y)` would swing the piece around its
/// own corner instead of turning it on the spot — and at a rotation of zero the two answer
/// the same point, so the mistake survives a first look.
auto Centre(const OExportPlacement &placement) -> QPointF;

/// @brief The placed footprint's four corners, in scene coordinates — a rotated rectangle.
auto Corners(const OExportPlacement &placement) -> QPolygonF;

/// @brief The axis-aligned box of the footprint's OWN outline, placed — what a clamp is
///        measured against.
///
/// Deliberately not the box of the holder's rectangle. That rectangle is the footprint's
/// bounding box, whose corners are empty, so a piece clamped by it stops short of an edge its
/// own shape would have reached — and the engine, which placed the polygon and never reasoned
/// on the box, had left it that room. Only a hand correcting the plan is held to the shape.
auto Rect(const OExportPlacement &placement) -> QRectF;

/// @brief translate by @p delta, then hold inside @p plan.
auto Moved(const OExportPlacement &placement, const QPointF &delta, const QRectF &plan)
    -> OExportPlacement;

/// @brief rotate by @p deltaDegrees about the footprint's own centre, then hold inside @p plan.
///
/// The clamp runs after the rotation, so a piece turned near an edge slides back in rather
/// than leaving the paper: the mode's promise is that everything stays printable, and a
/// corner that pokes out would break it where the PDF is written.
auto Rotated(const OExportPlacement &placement, qreal deltaDegrees, const QRectF &plan)
    -> OExportPlacement;

/// @brief The angle of @p scenePos seen from @p centre, in degrees, in the frame
///        QTransform::rotate() turns in — the scene is y-down, so there is no sign to flip.
auto AngleOf(const QPointF &centre, const QPointF &scenePos) -> qreal;

/// @brief Put an item where a gesture has taken it, from where it was before the gesture.
///
/// @p baseScene is the item's scene mapping captured ONCE when the gesture began; @p delta is
/// the rigid motion, in scene coordinates, from the placement the gesture started at to the one
/// being previewed — that is OExportFootprint::Matrix(start).inverted() *
/// Matrix(preview). That order is not cosmetic; see the note in the .cpp.
///
/// It is applied absolutely, never as an increment. Reading the item's current transform would
/// compound: the second movement would re-apply the whole motion on top of the first, and the
/// piece would travel twice as far as the mouse while the holder, placed straight from the
/// placement, stayed where it should be.
///
/// The item's position and rotation are folded into its transform rather than left alone,
/// because that is the one composition Qt's item pipeline has been MEASURED to obey: with both
/// at zero, the scene mapping IS the transform. Composing a scene delta through a live pos and
/// rotation is not something this code tries to model — it was tried, four ways, and none of
/// the four matched. Nothing reads those two properties back: Build recreates every item.
///
/// Only root items may be passed. A child's scene mapping carries its parent chain, which this
/// formula does not undo — and every item the canvas stamps with a footprint role is a root.
void ApplySceneDelta(QGraphicsItem *item, const QTransform &baseScene, const QTransform &delta);
} // namespace OExportFootprint

#endif // OEXPORTFOOTPRINT_H
