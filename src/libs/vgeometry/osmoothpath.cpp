/************************************************************************
 **
 **  @file   osmoothpath.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/10/2026
 **
 **  @brief  Draw a contour held as points as the smooth curve those points describe.
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

#include "osmoothpath.h"

#include <QLineF>
#include <QPainterPath>

#include <algorithm>
#include <cmath>

#include "oc2chain.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Angle the contour turns at a knot, in degrees, between the segment arriving and the segment leaving.
 */
auto TurnAngle(const QPointF &previous, const QPointF &knot, const QPointF &next) -> qreal
{
    const QLineF incoming(knot, previous);
    const QLineF outgoing(knot, next);

    if (qFuzzyIsNull(incoming.length()) || qFuzzyIsNull(outgoing.length()))
    {
        return 0.0;
    }

    const qreal angle = incoming.angleTo(outgoing); // [0, 360)

    return angle > 180.0 ? 360.0 - angle : angle;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Angle above which a knot is a corner of the contour, not a kink left by the cutting.
 *
 * Measured on real contours: cutting a curve into a polyline leaves knots turning by 5° to 14°, a tight one
 * a little over 20°, while the corners the model placed on purpose (dart points, prongs, the corner of a
 * piece) turn by far more. A knot kept as a corner costs nothing where the line was nearly straight
 * anyway, and smoothing a real corner would round it, so the bar sits under the smallest real corner and
 * over the widest kink. At 20° the smoothed line moves from the polyline by at most a fifth of the
 * distance the polyline is already allowed to sit from the curve it cuts (0.5 / approximationScale): the
 * drawing gets smoother, nothing moves.
 */
constexpr qreal kCornerAngle = 20.0; // degrees

//---------------------------------------------------------------------------------------------------------------------
auto Normalized(const QPointF &vector) -> QPointF
{
    const qreal length = std::hypot(vector.x(), vector.y());

    return qFuzzyIsNull(length) ? QPointF() : vector / length;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Distance from a point to a segment.
 */
auto DistanceToSegment(const QPointF &point, const QPointF &from, const QPointF &to) -> qreal
{
    const QPointF direction = to - from;
    const qreal lengthSquared = QPointF::dotProduct(direction, direction);

    if (qFuzzyIsNull(lengthSquared))
    {
        return QLineF(from, point).length();
    }

    const qreal position = std::clamp(QPointF::dotProduct(point - from, direction) / lengthSquared, 0.0, 1.0);

    return QLineF(from + direction * position, point).length();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Distance the chain is allowed to take on one segment before it is drawn straight instead.
 *
 * A chain clamped on the directions of its end chords can overshoot where two knots sit very close
 * together, and the corner heuristics of the seam allowance do insert such pairs. The cutting itself is
 * allowed to sit 0.5 / approximationScale — one pattern unit by default — away from the curve it cuts; a
 * segment that would move further than that is not a smoother drawing of the same line, so it stays a line.
 */
constexpr qreal kMaxChordDeviation = 1.0; // pattern units

//---------------------------------------------------------------------------------------------------------------------
auto LeavesItsChord(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4) -> bool
{
    const QVector<QPointF> control{p1, p2, p3, p4};

    for (int i = 1; i < 8; ++i)
    {
        const QPointF point = OC2Chain::BezierPoint(control, static_cast<qreal>(i) / 8.0);

        if (DistanceToSegment(point, p1, p4) > kMaxChordDeviation)
        {
            return true;
        }
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Distance under which two knots are the same knot.
 *
 * The tangent solver divides by the square of the chord lengths, so a repeated knot would put an infinity
 * into the handles. The geometry never means anything by a repeated knot: it is dropped.
 */
constexpr qreal kSameKnot = 1e-6; // pattern units

//---------------------------------------------------------------------------------------------------------------------
auto WithoutRepeatedKnots(const QVector<QPointF> &points) -> QVector<QPointF>
{
    QVector<QPointF> knots;
    knots.reserve(points.size());

    for (const QPointF &point : points)
    {
        if (knots.isEmpty() || QLineF(knots.constLast(), point).length() > kSameKnot)
        {
            knots.append(point);
        }
    }

    return knots;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Append one run of knots as a cubic chain, clamped on the directions of its two end chords.
 */
void AppendRun(QPainterPath &path, const QVector<QPointF> &run)
{
    const QVector<QPointF> knots = WithoutRepeatedKnots(run);

    if (knots.size() < 2)
    {
        return;
    }

    if (knots.size() == 2)
    {
        // Two knots and nothing in between: a straight line, and a line is what it is.
        path.lineTo(knots.constLast());
        return;
    }

    const QVector<qreal> chords = OC2Chain::Chords(knots);
    const QPointF startTangent = Normalized(knots.at(1) - knots.at(0));
    const QPointF endTangent = Normalized(knots.constLast() - knots.at(knots.size() - 2));
    const QVector<QPointF> tangents = OC2Chain::Tangents(knots, chords, startTangent, endTangent);

    for (int i = 0; i + 1 < knots.size(); ++i)
    {
        const qreal chord = chords.at(i);
        const QPointF firstHandle = knots.at(i) + tangents.at(i) * (chord / 3.0);
        const QPointF secondHandle = knots.at(i + 1) - tangents.at(i + 1) * (chord / 3.0);

        if (LeavesItsChord(knots.at(i), firstHandle, secondHandle, knots.at(i + 1)))
        {
            path.lineTo(knots.at(i + 1));
            continue;
        }

        path.cubicTo(firstHandle, secondHandle, knots.at(i + 1));
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OSmoothPath::Path(const QVector<QPointF> &points, bool close) -> QPainterPath
{
    QPainterPath path;

    if (points.size() < 2)
    {
        return path;
    }

    path.moveTo(points.constFirst());

    // Cut the contour at its corners: every run in between is smooth, every junction stays a corner.
    int start = 0;
    for (int i = 1; i + 1 < points.size(); ++i)
    {
        if (TurnAngle(points.at(i - 1), points.at(i), points.at(i + 1)) > kCornerAngle)
        {
            AppendRun(path, points.mid(start, i - start + 1));
            start = i;
        }
    }
    AppendRun(path, points.mid(start));

    if (close && points.constLast() != points.constFirst())
    {
        path.lineTo(points.constFirst());
    }

    return path;
}
