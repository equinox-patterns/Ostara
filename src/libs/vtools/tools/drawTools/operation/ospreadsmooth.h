/************************************************************************
 **
 **  @file   ospreadsmooth.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31/10/2026
 **
 **  @brief  The smoothing of a slash-and-spread: one C2 chain through the pivots, cut back into one
 **          segment per curve.
 **
 **          A rotation carries every object rigidly, so two pieces of the same curve that turn around
 **          different pivots meet again with a crease: the tangent of the piece that turned has turned
 **          with it. Patching a handle would redraw the whole portion — a cubic has four control points,
 **          and moving one moves the curve. The chain does the opposite: it rebuilds the whole run of
 **          consecutive curves as a single C2 curve passing exactly through their junctions, which is what
 **          the graduated curve does with OC2Chain::Chain, and gives every curve back its own segment.
 **
 **          The two ends of the run keep the directions the operation gave them: the direction of the
 **          piece that did not move where the run starts on a pivot, the rotated direction at its far end.
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

#ifndef OSPREADSMOOTH_H
#define OSPREADSMOOTH_H

#include <QHash>
#include <QLineF>
#include <QPointF>
#include <QSet>
#include <QVector>
#include <QtDebug>
#include <QtGlobal>

#include <cmath>

#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "ospreadrule.h"

/** @brief One cubic of the ordered selection, as the rotations give it, with the steps it went through.
 *
 *  A curve is one cubic; a path enters the smoothing through each of its sub-curves, since a path is a chain of
 *  cubics. @a id is always the object of the selection — the path, not its piece — and @a index is where the
 *  cubic sits inside it, so the pieces of one object go back to it in their own order. */
struct OSpreadSmoothCurve
{
    quint32 id{NULL_ID};
    int index{0};
    QPointF p1{};  ///< first point of the image
    QPointF p2{};  ///< control point at @a p1
    QPointF p3{};  ///< control point at @a p4
    QPointF p4{};  ///< last point of the image

    /** @brief The rotations this curve went through: a step whose pivot is one of the two points is what
     *  separated it from the piece that did not move, and the direction it had before that step is the one
     *  the piece next to it still has. */
    QVector<OSpreadStep> steps{};
};

/** @brief The four control points a smoothed copy carries. Points rather than a spline: the copy keeps the
 *  name, the pen and the colour the operation gave it, and only its geometry is replaced. */
struct OSpreadSmoothImage
{
    QPointF p1{};
    QPointF p2{};
    QPointF p3{};
    QPointF p4{};
};

/**
 * @brief The smoothing of a slash-and-spread: nothing moves but the handles that meet at a junction.
 *
 * A rotation turns every piece of a region but not the piece on the other side of its start point, so the two
 * sides of that point take two different directions and the outline shows a crease. What the eye reads there is
 * the break in direction, not a break in curvature: the two handles that meet at the junction are given the
 * direction halfway between the two they had, each keeping its own length. The crease disappears, the outline
 * follows the same points, and everything that was not broken — the inside of a path, the ends of a run — is
 * left exactly as it was.
 *
 * Redrawing the whole run as one C2 chain was tried first, and it is what the graded curve does. It reads well
 * on short copies, but a piece of path is a curve someone drew: rebuilt through the same knots the chain came
 * back with handles a quarter shorter, and the outline left the drawn shape by half a centimetre.
 *
 * A run is read from the ordered selection: cubics whose ends meet belong to it. A run that stays inside one
 * object is not a junction at all: the pieces of a path were already smooth where they were cut.
 */
class OSpreadSmooth
{
public:
    /** @brief The cubics that replace the plain image of every object taking part in a run, by source id and in
     *  the order the object draws them. An object that belongs to no run of two is left out, and keeps the
     *  image the rotations gave it. */
    static auto Rebuild(const QVector<OSpreadSmoothCurve> &curves,
                        const QVector<QPair<QPointF, QPointF>> &outline)
        -> QHash<quint32, QVector<OSpreadSmoothImage>>
    {
        QHash<quint32, QVector<OSpreadSmoothImage>> smoothed;

        // How many cubics each object brings, so the images can be handed back in its own order.
        QHash<quint32, int> countBySource;
        for (const OSpreadSmoothCurve &curve : curves)
        {
            countBySource[curve.id] = qMax(countBySource.value(curve.id), curve.index + 1);
        }

        if (curves.size() < 2)
        {
            return smoothed;
        }

        // One slot per cubic of each object, and a mark for the ones a run gave an image to. An object is
        // smoothed only when every one of its cubics took part: half a path redrawn would be worse than none.
        QHash<quint32, QVector<OSpreadSmoothImage>> pending;
        QHash<quint32, QVector<bool>> filled;
        for (auto it = countBySource.cbegin(); it != countBySource.cend(); ++it)
        {
            pending.insert(it.key(), QVector<OSpreadSmoothImage>(it.value()));
            filled.insert(it.key(), QVector<bool>(it.value(), false));
        }

        // The two copies of a cut share the point the parse gave them, so the test is on the geometry, not on
        // an id: a piece of curve is an object of its own, and the junction is only a place they both reach.
        const auto samePoint = [](const QPointF &left, const QPointF &right)
        { return QLineF(left, right).length() <= 1e-4; };

        const auto unitOf = [](const QPointF &v) -> QPointF
        {
            const qreal length = std::hypot(v.x(), v.y());
            return length > 1e-12 ? QPointF(v.x() / length, v.y() / length) : QPointF(0., 0.);
        };

        // A direction turned by an angle, exactly as VPointF::Rotate turns a point: the two must agree, the
        // direction being read off a point the same way.
        const auto turned = [](const QPointF &v, qreal degrees) -> QPointF
        {
            QLineF line(QPointF(0., 0.), v);
            line.setAngle(line.angle() + degrees);
            return line.p2();
        };

        // The angle of a direction, read the same way @a turned reads one.
        const auto angleOf = [](const QPointF &v) -> qreal { return QLineF(QPointF(0., 0.), v).angle(); };

        // The same cubic read the other way round, which is how a run that arrives backwards is handled.
        const auto flipped = [](const OSpreadSmoothImage &piece) -> OSpreadSmoothImage
        { return {.p1 = piece.p4, .p2 = piece.p3, .p3 = piece.p2, .p4 = piece.p1}; };

        QVector<bool> used(curves.size(), false);

        for (int first = 0; first < curves.size(); ++first)
        {
            if (used.at(first))
            {
                continue;
            }

            // One run, from one free end to the other. The selection follows the contour, so a curve is
            // normally reached from the tail; the head is read too, because a run read backwards is a run all
            // the same.
            QVector<int> links{first};
            QVector<bool> reversed{false};
            used[first] = true;
            QPointF head = curves.at(first).p1;
            QPointF tail = curves.at(first).p4;

            bool grew = true;
            while (grew)
            {
                grew = false;
                for (int j = 0; j < curves.size(); ++j)
                {
                    if (used.at(j))
                    {
                        continue;
                    }

                    const OSpreadSmoothCurve &next = curves.at(j);
                    if (samePoint(next.p1, tail))
                    {
                        links.append(j);
                        reversed.append(false);
                        tail = next.p4;
                    }
                    else if (samePoint(next.p4, tail))
                    {
                        links.append(j);
                        reversed.append(true);
                        tail = next.p1;
                    }
                    else if (samePoint(next.p4, head))
                    {
                        // The curve sits before the run and joins it by its end: it is read the way it is
                        // drawn, from its start to that end, and its start becomes the new head.
                        links.prepend(j);
                        reversed.prepend(false);
                        head = next.p1;
                    }
                    else if (samePoint(next.p1, head))
                    {
                        // It joins by its start, so it has to be read backwards to end where the run begins.
                        links.prepend(j);
                        reversed.prepend(true);
                        head = next.p4;
                    }
                    else
                    {
                        continue;
                    }

                    used[j] = true;
                    grew = true;
                    break;
                }
            }

            if (links.size() < 2)
            {
                continue; // A single cubic has no junction: nothing to smooth.
            }

            // A run that stays inside one object is not a junction either: the pieces of a path meet along the
            // shape they were drawn with, and touching them would move a curve nobody broke.
            QSet<quint32> owners;
            for (int k = 0; k < links.size(); ++k)
            {
                owners.insert(curves.at(links.at(k)).id);
            }

            if (owners.size() < 2)
            {
                continue;
            }

            // The cubics of the run, all read in the direction of travel.
            QVector<OSpreadSmoothImage> pieces;
            pieces.reserve(links.size());
            for (int k = 0; k < links.size(); ++k)
            {
                const OSpreadSmoothCurve &curve = curves.at(links.at(k));
                const OSpreadSmoothImage piece{.p1 = curve.p1, .p2 = curve.p2, .p3 = curve.p3, .p4 = curve.p4};
                pieces.append(reversed.at(k) ? flipped(piece) : piece);
            }

            // The junctions between two objects, read on the geometry the rotations gave, before anything
            // moves: the direction the run arrives with, the one it leaves with, and how far the two are apart.
            struct Junction
            {
                int index;    // the piece that arrives at the knot
                qreal from;   // angle of the direction arriving
                qreal to;     // angle of the direction leaving
            };

            QVector<Junction> junctions;
            for (int k = 1; k < pieces.size(); ++k)
            {
                if (curves.at(links.at(k - 1)).id == curves.at(links.at(k)).id)
                {
                    continue; // A knot inside one object: it was already smooth where it was cut.
                }

                const QPointF knot = pieces.at(k - 1).p4;
                const qreal arriving = angleOf(knot - pieces.at(k - 1).p3);
                qreal leaving = angleOf(pieces.at(k).p2 - knot);

                while (leaving - arriving > 180.)
                {
                    leaving -= 360.;
                }
                while (leaving - arriving < -180.)
                {
                    leaving += 360.;
                }

                if (qAbs(leaving - arriving) < 1e-6)
                {
                    continue; // The two pieces already meet head to tail.
                }

                if (qAbs(leaving - arriving) > 90.)
                {
                    continue; // A corner, not the crease of a rotation: it is part of the drawing.
                }

                junctions.append({.index = k, .from = arriving, .to = leaving});
            }

            // The two ends of the run. A run ends against the outline that is already there: the piece on the
            // other side of that point did not take the rotation, so it still carries the direction the run had
            // before the operation. The handle there takes that direction — otherwise the crease is only pushed
            // out to the edge of the run. The neighbour is only followed when the run really was tangent with
            // it before the operation: a corner that was drawn is not a crease to repair.
            const OSpreadSmoothCurve &startCurve = curves.at(links.constFirst());
            const OSpreadSmoothCurve &endCurve = curves.at(links.constLast());

            const auto angleBetween = [&](const QPointF &left, const QPointF &right) -> qreal
            {
                qreal gap = angleOf(left) - angleOf(right);
                while (gap > 180.)
                {
                    gap -= 360.;
                }
                while (gap < -180.)
                {
                    gap += 360.;
                }
                return qAbs(gap);
            };

            // @a direction is the one the run takes at @a knot now; the direction it had before the operation is
            // that one turned back by the whole of the rotations it went through.
            const auto follows = [&](const QPointF &knot, const QPointF &direction,
                                     const QVector<OSpreadStep> &steps) -> QPointF
            {
                qreal total = 0.;
                for (const OSpreadStep &step : steps)
                {
                    total += step.angle;
                }

                const QPointF before = turned(direction, -total);

                for (const QPair<QPointF, QPointF> &neighbour : outline)
                {
                    if (samePoint(neighbour.first, knot) && angleBetween(neighbour.second, before) <= 0.5)
                    {
                        return neighbour.second;
                    }
                }

                return QPointF();
            };

            // A run that ends on a pivot, with no curve drawn beyond it, is read the same way: the piece that
            // stayed there still carries the direction of before the step.
            const auto merged = [&](const OSpreadSmoothCurve &curve, const QPointF &knot, const QPointF &direction)
                -> QPointF
            {
                for (const OSpreadStep &step : curve.steps)
                {
                    if (samePoint(step.pivot, knot))
                    {
                        return turned(direction, -step.angle);
                    }
                }
                return QPointF();
            };

            const QPointF startDirection
                = follows(pieces.first().p1, unitOf(pieces.first().p2 - pieces.first().p1), startCurve.steps);
            const QPointF startFallback
                = merged(startCurve, pieces.first().p1, unitOf(pieces.first().p2 - pieces.first().p1));
            const QPointF startTaken = startDirection.isNull() ? startFallback : startDirection;
            if (not startTaken.isNull())
            {
                pieces.first().p2
                    = pieces.first().p1 + startTaken * QLineF(pieces.first().p1, pieces.first().p2).length();
            }

            const QPointF endAway = unitOf(pieces.constLast().p4 - pieces.constLast().p3);
            const QPointF endDirection = follows(pieces.constLast().p4, endAway, endCurve.steps);
            const QPointF endFallback = merged(endCurve, pieces.constLast().p4, endAway);
            const QPointF endTaken = endDirection.isNull() ? endFallback : endDirection;
            if (not endTaken.isNull())
            {
                pieces.last().p3
                    = pieces.last().p4 - endTaken * QLineF(pieces.last().p4, pieces.last().p3).length();
            }

            // Curvature at a knot: |handle x far| / |handle|^3, with the handle taken in the direction of
            // travel and @a far the control point the piece bends against.
            const auto curvature = [](const QPointF &handle, const QPointF &far) -> qreal
            {
                const qreal length = std::hypot(handle.x(), handle.y());
                if (length < 1e-12)
                {
                    return 0.;
                }
                return (handle.x() * far.y() - handle.y() * far.x()) / (length * length * length);
            };

            // Each junction in turn: the turn the rotation left is shared until both sides end with the same
            // curvature. Half the turn on each side would already hide the crease from the eye, but the two
            // pieces do not bend alike, so one of them would take a curvature the other does not have — that is
            // the bump of every half-hearted blend. The lengths never change, so nothing moves but the two
            // handles that meet. The far control point of a piece is the handle of the junction next to it, so
            // the junctions are solved again on their neighbours' last state until the handles settle.
            for (int pass = 0; pass < 4 && not junctions.isEmpty(); ++pass)
            {
                for (const Junction &junction : std::as_const(junctions))
                {
                    OSpreadSmoothImage &before = pieces[junction.index - 1];
                    OSpreadSmoothImage &after = pieces[junction.index];

                    const QPointF knot = before.p4;

                    // A handle never longer than a third of the chord it serves. A rotation turns the piece, and the
                    // handle it brought was measured for the place it came from: at a junction the rotation opened
                    // wide, that handle is long compared with the segment, the curve overshoots the straight line
                    // and comes back — a bump at the junction, worse as the angle grows. The third is the classic
                    // limit past which a cubic bulges; below it the curve stays between its ends and the crease is
                    // still the only thing the solve has to erase.
                    const qreal arrivingLength =
                        qMin(QLineF(knot, before.p3).length(), QLineF(before.p1, knot).length() / 3.);
                    const qreal leavingLength =
                        qMin(QLineF(knot, after.p2).length(), QLineF(knot, after.p4).length() / 3.);
                    const QPointF arrivingFar = before.p2 - knot;
                    const QPointF leavingFar = after.p3 - knot;

                    const auto residual = [&](qreal direction) -> qreal
                    {
                        return curvature(turned(QPointF(arrivingLength, 0.), direction), arrivingFar)
                               - curvature(turned(QPointF(leavingLength, 0.), direction), leavingFar);
                    };

                    qreal direction = (junction.from + junction.to) / 2.;

                    if (residual(junction.from) * residual(junction.to) <= 0.)
                    {
                        qreal from = junction.from;
                        qreal to = junction.to;
                        for (int step = 0; step < 60; ++step)
                        {
                            const qreal middle = (from + to) / 2.;
                            if (residual(from) * residual(middle) <= 0.)
                            {
                                to = middle;
                            }
                            else
                            {
                                from = middle;
                            }
                        }
                        direction = (from + to) / 2.;
                    }

                    before.p3 = knot - turned(QPointF(arrivingLength, 0.), direction);
                    after.p2 = knot + turned(QPointF(leavingLength, 0.), direction);
                }
            }

            for (int k = 0; k < links.size(); ++k)
            {
                const OSpreadSmoothCurve &curve = curves.at(links.at(k));
                pending[curve.id][curve.index] = reversed.at(k) ? flipped(pieces.at(k)) : pieces.at(k);
                filled[curve.id][curve.index] = true;
            }
        }

        for (auto it = filled.cbegin(); it != filled.cend(); ++it)
        {
            if (not it->contains(false))
            {
                smoothed.insert(it.key(), pending.value(it.key()));
            }
        }

        return smoothed;
    }
};

#endif // OSPREADSMOOTH_H
