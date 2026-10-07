/************************************************************************
 **
 **  @file   opleatcaprule.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/10/2026
 **
 **  @brief  The cap of a pleat: the V drawn at an end of the fold line, so that the piece's edge stays
 **          continuous once the pleat is folded.
 **
 **          Its apex is where the edge crosses the fold line, and its two sides are that edge and the same edge
 **          turned over by the fold — the fold *is* a reflection, so the two sides have to be one another's image,
 **          or they would not come together on the fabric.
 **
 **          A straight edge is its own image by that reflection: the two straight tracings the figure has always
 **          drawn are exact there, and an edge lying along the move is carried onto itself and has no cap at all.
 **          A **curved** edge is not a line, and a tangent only meets it where it is taken: the V of the figure
 **          followed the right axis and the wrong shape, and the fold left the difference — small, but visible on
 **          a printed pattern, and all the more so as the edge is bowed and the cap long.
 **
 **          So where a curve carries the edge, the curve itself is taken: the piece that runs from the point of
 **          the departure line to where the edge crosses the fold line, and that piece flipped about the fold
 **          line for the other side. The crossing is the apex, the fold line then runs from edge to edge, and the
 **          two sides superpose exactly when the fabric is folded.
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

#ifndef OPLEATCAPRULE_H
#define OPLEATCAPRULE_H

#include <QLineF>
#include <QPointF>
#include <QSharedPointer>
#include <QtGlobal>

#include "../../vgeometry/vabstractcurve.h"
#include "opleatrule.h"

class VContainer;

/**
 * @brief The apex of a cap read on the curve that carries the edge, and what the figure needs to cut the sides.
 *
 * An empty answer — a null @a point — means no curve carries the cap: either a line carries the edge, or the curve
 * never meets the fold line, and the two straight tracings then stand, as they always did.
 *
 * @a edgeId is the curve the edge was read on, @a length where the apex falls on it, measured from its start the
 * way the cut tools measure a cut, and @a fromStart whether the point of the departure line is that start: together
 * the last two say which of the two halves the cap follows.
 */
struct OPleatCapPlacement
{
    QPointF point{};
    quint32 edgeId{NULL_ID};
    qreal length{0.};
    bool fromStart{false};
};

/**
 * @brief The two sides of a cap: the piece of the edge the fold turns over, and its image by the fold.
 *
 * @a edge runs from the point of the departure line to the apex, @a image from the apex to the point the move
 * carried it to — and the second is the first flipped about the fold line, so folding the pleat lays one exactly
 * on the other and the edge reads whole.
 */
struct OPleatCapSides
{
    QSharedPointer<VAbstractCurve> edge{};
    QSharedPointer<VAbstractCurve> image{};
};

/**
 * @brief The rule of a cap.
 *
 * No state: every method is a reading or a computation, and none of them touches the pattern.
 */
class OPleatCapRule
{
public:
    /** @brief The apex of a cap, read on the curve that carries the edge where it crosses the fold line.
     *
     *  @a end is the end of the departure line the cap stands at, @a side the direction the edge is read on — the
     *  side of the region's fabric, so that the curve taken is the edge the fold turns over (the notch reads the
     *  other side, being counted on the side the fold leaves free) — and @a foldLine the fold line **in the frame of
     *  the edge as the pattern drew it**: the edge is never moved here, so the line is what the pleats carry back to
     *  it. @a offset is what carries the figure from that frame to the place the cap belongs to, and the apex comes
     *  back carried.
     *
     *  The first crossing walking away from the point of the departure line is the one read: an edge crosses the
     *  fold line once near each cap, and the ones further along — the hem, the other side of the piece — belong to
     *  other caps. A point the user designated to carry the edge describes a line and no object, and nothing is
     *  asked of the pattern then. */
    static auto Curved(const OPleatEnd &end, const QPointF &side, const QLineF &foldLine, const QPointF &offset,
                       const VContainer *data) -> OPleatCapPlacement;

    /** @brief The two sides of a cap the plan has read on a curve, ready to be placed as objects of the pattern.
     *
     *  @a mark is the resolved mark, @a prise the end of the departure line it stands at, and @a origin where that
     *  point is at the place of this pleat — the side of the edge starts there. @a foldLine is the fold line **at
     *  that place**, which is what the flipped side is the image of. @a apexName names the junction of the cut, the
     *  point the two sides share.
     *
     *  Both are null when the curve is of a kind no cut can split, or when the container no longer holds the edge:
     *  the caller then has nothing to place, exactly as it has nothing for a turned-off cap. */
    static auto Sides(const OPleatMark &mark, const OPleatEnd &prise, const QPointF &origin, const QLineF &foldLine,
                      const QString &apexName, const VContainer *data) -> OPleatCapSides;

private:
    Q_DISABLE_COPY_MOVE(OPleatCapRule) // NOLINT
    OPleatCapRule() = default;
};

#endif // OPLEATCAPRULE_H
