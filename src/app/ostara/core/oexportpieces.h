/************************************************************************
 **
 **  @file   oexportpieces.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/10/2026
 **
 **  @brief  Export mode — what a cutting plan needs to know about a piece,
 **          stated once, next to the recipe.
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

#ifndef OEXPORTPIECES_H
#define OEXPORTPIECES_H

#include <QDomElement>
#include <QList>
#include <QStringList>
#include <QVector>

class QDomDocument;
class VContainer;

struct OExportSizeLayer;

namespace OGradedSvg
{
struct LayoutObject;
}  // namespace OGradedSvg

/**
 * @brief The pieces, as a cutting plan has to read them — a `<pieces>` element for the payload.
 *
 * **The drawing says where a piece is; this says what may be DONE with it.** A cutting plan is not
 * a picture of the pattern: it decides which rotations a piece may take (its grain), which edge of
 * it is a fold and against which edge of the cloth that fold must sit, how many times it is cut,
 * and how much cloth that is. None of it can be recovered from a contour:
 *
 *   • the grain is drawn as a line, so its angle survives as a drawing and not as a number, and the
 *     ARROW TYPE — one axis or two — lives nowhere in the ink at all. A « Double » piece accepts the
 *     contre-fil and a plain one does not, and the two drawings differ by a pair of arrow heads.
 *   • the fold is drawn as a pair of arrows, never as the axis, and the axis is exactly what a plan
 *     needs: it is the edge that goes on the fold of the cloth.
 *   • the cut quantity belongs to the cartouche, and never reaches the SVG.
 *   • and a piece cut on the fold is DRAWN IN HALF. Its contour is closed and valid, so nothing
 *     distinguishes it from a whole piece: a reader that is not told will lay half a garment on the
 *     cloth and see nothing wrong. The half that is missing is stated here, once, so that a reader
 *     never has to mirror and unite anything to find it.
 *
 * **The complementary form is written, never the form that is already there.** The drawing carries
 * one of the two, and `stored` says which; `outlineAlt` carries the other, and only for a piece
 * that has a fold at all. A whole piece has no complement to speak of.
 *
 * **Two contours, and they are not the same one.** `outline` is the plain outer form, without the
 * notches, and that is the one the nesting unions: a notch is a cut, so unioning a notched contour
 * gives a footprint SMALLER than the piece that gets cut. `outlineInk` is the contour as the canvas
 * DRAWS it — sewing line, seam allowance and notches together — because a notch is not an ornament:
 * it is the assembly mark, and a piece drawn without its notches cannot be mounted. A reader draws
 * the second and nests the first; neither can stand in for the other.
 *
 * **The sizes are already aligned, and the offset is stated.** `OExportAlignment` shifts every size
 * of a piece so that they superimpose — each size keeps its own place in the pattern, and a piece
 * pinned at its fold does not drift like a piece growing from its centre. The contours above are
 * written SHIFTED BY NOTHING, in the pattern's own frame, and `alignDelta` carries that shift in
 * pixels. A reader that ignores it does not get a slightly wrong footprint: it gets one that is too
 * large, because it unions shapes that do not lie on top of each other.
 *
 * The frame is the pattern's own — the same pixels as the drawing's viewBox, before any placement —
 * because a plan places the pieces again: coordinates of the sheet would be a plan already made.
 */
namespace OExportPieces
{
/**
 * @brief The pieces of every size layer, as a `<pieces>` element.
 *
 * @param doc     the document the element is created in; the caller owns it
 * @param layers  the generation's own geometry, one layer per size
 * @param pattern the pattern, source of the cut quantities; may be nullptr, and the quantities are
 *                then left out rather than guessed — a missing figure is a fact, a wrong one is not
 * @param labels  the layer names AS THE DRAWING WRITES THEM, index-aligned with @p layers. They are
 *                not `OExportSizeLayer::label`: the drawing names a layer after the size style and
 *                falls back on the axis value (« Taille 46 » against « 46 »), and a reader finds a
 *                piece through its layer's written name. Empty falls back on the layer's own label.
 * @param decor   the decor the drawing resolved, ONE OBJECT PER OWNER AND PER SIZE — matched here by
 *                its owner's id and its layer index. Each `<piece>` carries the decor of ITS size
 *                (`LayoutObject::layerIndex`), which is what Graded built for that size. The
 *                reference layer's object is the fallback, so a size with no entry of its own still
 *                says something rather than nothing. Empty writes no decor at all, which is what a
 *                caller with no drawing wants.
 *
 * ⚠️ **A decor belongs to a size — never one block copied everywhere.** The block used to be
 * resolved once, on the reference layer, and copied byte for byte into every size's record: a reader
 * drawing any other size then placed another size's children on it. Measured on Rabnest's side, up
 * to 227 px of drift in x on the pieces without a fold, and the drift followed the grading, so no
 * translation could cancel it.
 */
auto Of(QDomDocument &doc, const QVector<OExportSizeLayer> &layers, const QStringList &labels,
        const VContainer *pattern, const QList<OGradedSvg::LayoutObject> &decor) -> QDomElement;
}  // namespace OExportPieces

#endif  // OEXPORTPIECES_H
