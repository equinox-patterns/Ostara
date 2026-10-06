/************************************************************************
 **
 **  @file   oedgereader.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/10/2026
 **
 **  @brief  The line of the pattern carried by a point: the edge of the piece the point sits on.
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

#ifndef OEDGEREADER_H
#define OEDGEREADER_H

#include <QPointF>
#include <QtGlobal>

#include "../../toolsdef.h"

class VContainer;

/**
 * @brief The edge of the piece at one point, as the object that carries it and the way it runs from the point.
 *
 * @a objectId is the curve that ends at the point — the object a notch has to be cut into — or NULL_ID when only a
 * line carries the edge there, the contour of a piece included. @a inward is a vector running from the point into
 * that edge: a cap reads a line and no more, but a notch is walked along the edge, so it is the *way* that
 * matters, and it is the reason this answer is not the one @a Direction gives.
 */
struct OEdgeCarrier
{
    quint32 objectId{NULL_ID};
    QPointF inward{};
};

/**
 * @brief Which way the piece's edge runs at one point of the pattern.
 *
 * A cap — the V a pleat builds where the fold meets the edge of the piece — has to follow that edge. The tool
 * is given nothing but its points, so the edge has to be read, and there is more than one place to read it.
 * The reader asks, in this order, and stops at the first answer:
 *
 * 1. **the contour of the piece** — when the point sits on the outline of a piece, that outline is the truth,
 *    and nothing else can know it: the reader walks the united path of every piece, finds the edge carrying the
 *    point and answers its tangent. It is the only source that can decide between two edges meeting at a point;
 * 2. **the segment the point's own tool draws through it** — the base segment of a point on a line (alongLine,
 *    endLine, normal, bisector, shoulder point), exactly one segment, never two. This is what "the point draws a
 *    line, follow it back to the other end" means: the line a tool draws *from* the point is the line the point
 *    sits on. The pairs a container records through the point are deliberately **not** read: any tool of the
 *    pattern can register one — the departure line of a pleat drawn with the Line tool is registered on both of
 *    its prise points — and such a line is not the edge;
 * 3. **the curves the tool produced at that point** — a cut leaves the two sub-curves it made of its parent, both
 *    carrying the tool's id and both ending at the cut point: the tangent there is the edge. The two tangents
 *    have to describe the same line, which is what tells a cut from a crossing;
 * 4. **any curve of the pattern that ends at that point** — a curve names nobody: the point it starts from was
 *    drawn by another tool, and no id links the two. The position does, and this last source is what answers for
 *    the end of an arc or a spline, and for the copy an operation made of a curve.
 *
 * Only a *direction* is ever read, never an orientation: a fold cares about the line, and a vector and its
 * opposite describe the same edge. A null answer is the signal that nothing could be read and the caller has to
 * ask the user to designate a point — it is not a failure.
 *
 * No state, no pattern: the question is asked of the objects a container already holds.
 */
class OEdgeReader
{
public:
    /** @brief A vector along the edge of the piece at @a pointId, or a null vector when that point carries no
     *  edge the reader can name.
     *
     *  The answer is never normalized, and its orientation carries no meaning: the caller reads a line. */
    static auto Direction(quint32 pointId, const VContainer *data) -> QPointF;

    /** @brief The edge at @a pointId, taken on the side @a side points to.
     *
     *  Where @a Direction answers a line, this answers *which* object carries it and which of the two ways along
     *  that line leaves the point — a notch is walked from the point, and at a cut the two halves both end there
     *  and leave it in opposite directions, so the side is what chooses between them. The answer falls back on a
     *  plain direction — no object — when the edge is a line, a filler of the contour, or a segment a tool drew:
     *  the contour is read first, exactly as @a Direction reads it, so a curve that merely passes through the
     *  point is never taken for the edge. */
    static auto Carrier(quint32 pointId, const QPointF &side, const VContainer *data) -> OEdgeCarrier;

    /** @brief The position of a point of the container, or the origin when the id names no point. */
    static auto Position(quint32 pointId, const VContainer *data) -> QPointF;

private:
    Q_DISABLE_COPY_MOVE(OEdgeReader) // NOLINT
    OEdgeReader() = default;
};

#endif // OEDGEREADER_H
