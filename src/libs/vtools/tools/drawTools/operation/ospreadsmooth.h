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
#include <QVector>
#include <QtGlobal>

#include <cmath>

#include "../vgeometry/oc2chain.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "ospreadrule.h"

/** @brief One curve of the ordered selection, as the rotations give it, with the steps it went through. */
struct OSpreadSmoothCurve
{
    quint32 id{NULL_ID};
    QPointF start{};         ///< first point of the image
    QPointF end{};           ///< last point of the image
    QPointF startTangent{};  ///< unit direction of travel at @a start
    QPointF endTangent{};    ///< unit direction of travel at @a end

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
 * @brief The smoothing of a slash-and-spread: one C2 chain through the pivot points.
 *
 * The run is read from the ordered selection: consecutive curves whose ends meet belong to the same run, and
 * a run of one curve has nothing to smooth. The chain is built by OC2Chain, the engine of the graduated
 * curve, which puts the same curvature on both sides of every knot: that is what removes the crease a
 * rotation leaves, and what keeps it from coming back as a bump the way a patched handle does.
 */
class OSpreadSmooth
{
public:
    /** @brief The spline that replaces the plain image of every curve taking part in a run, by source id.
     *  A curve that belongs to no run of two is left out, and keeps the image the rotations gave it. */
    static auto Rebuild(const QVector<OSpreadSmoothCurve> &curves) -> QHash<quint32, OSpreadSmoothImage>
    {
        QHash<quint32, OSpreadSmoothImage> smoothed;
        if (curves.size() < 2)
        {
            return smoothed;
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
            QPointF head = curves.at(first).start;
            QPointF tail = curves.at(first).end;

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
                    if (samePoint(next.start, tail))
                    {
                        links.append(j);
                        reversed.append(false);
                        tail = next.end;
                    }
                    else if (samePoint(next.end, tail))
                    {
                        links.append(j);
                        reversed.append(true);
                        tail = next.start;
                    }
                    else if (samePoint(next.end, head))
                    {
                        // The curve sits before the chain and joins it by its end: it is read the way it is
                        // drawn, from its start to that end, and its start becomes the new head.
                        links.prepend(j);
                        reversed.prepend(false);
                        head = next.start;
                    }
                    else if (samePoint(next.start, head))
                    {
                        // It joins by its start, so it has to be read backwards to end where the chain begins.
                        links.prepend(j);
                        reversed.prepend(true);
                        head = next.end;
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
                continue; // A single curve has no junction: nothing to smooth.
            }

            // The knots of the chain: the two ends of the run and every junction in between, in order.
            QVector<QPointF> points;
            points.reserve(links.size() + 1);
            for (int k = 0; k < links.size(); ++k)
            {
                const OSpreadSmoothCurve &curve = curves.at(links.at(k));
                points.append(reversed.at(k) ? curve.end : curve.start);
            }
            const OSpreadSmoothCurve &last = curves.at(links.constLast());
            points.append(reversed.constLast() ? last.start : last.end);

            // The direction the run takes at each of its two ends. At a pivot, the direction to keep is the
            // one the curve had before the step that turned it around that very pivot: the piece that stayed
            // is still there, and it is that direction it carries.
            const auto directionAt = [&](const OSpreadSmoothCurve &curve, const QPointF &point,
                                         const QPointF &direction)
            {
                for (const OSpreadStep &step : curve.steps)
                {
                    if (samePoint(step.pivot, point))
                    {
                        return turned(direction, -step.angle);
                    }
                }
                return direction;
            };

            const OSpreadSmoothCurve &startCurve = curves.at(links.constFirst());
            const QPointF startDirection = reversed.constFirst() ? QPointF(-startCurve.endTangent.x(),
                                                                          -startCurve.endTangent.y())
                                                                 : startCurve.startTangent;
            const QPointF endDirection = reversed.constLast() ? QPointF(-last.startTangent.x(), -last.startTangent.y())
                                                              : last.endTangent;

            const QPointF startTangent = unitOf(directionAt(startCurve, points.constFirst(), startDirection));
            const QPointF endTangent = unitOf(directionAt(last, points.constLast(), endDirection));

            const VSplinePath chain = OC2Chain::Chain(points, startTangent, endTangent, QStringLiteral("X1"),
                                                      QStringLiteral("X4"));
            if (chain.CountSubSpl() != links.size())
            {
                continue; // A chain that could not be built leaves the copies as the rotations made them.
            }

            for (int k = 0; k < links.size(); ++k)
            {
                // GetSpline is 1-based: index 0 is the one it refuses, and it says so with an exception.
                const VSpline segment = chain.GetSpline(k + 1);
                OSpreadSmoothImage image;

                if (reversed.at(k))
                {
                    image.p1 = segment.GetP4().toQPointF();
                    image.p2 = segment.GetP3().toQPointF();
                    image.p3 = segment.GetP2().toQPointF();
                    image.p4 = segment.GetP1().toQPointF();
                }
                else
                {
                    image.p1 = segment.GetP1().toQPointF();
                    image.p2 = segment.GetP2().toQPointF();
                    image.p3 = segment.GetP3().toQPointF();
                    image.p4 = segment.GetP4().toQPointF();
                }

                smoothed.insert(curves.at(links.at(k)).id, image);
            }
        }

        return smoothed;
    }
};

#endif // OSPREADSMOOTH_H
