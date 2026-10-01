/************************************************************************
 **
 **  @file   oc2chain.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Cubic chain through sampled points, C2 at every interior knot.
 **          Bezier evaluation, arc length of one cubic segment, clamped
 **          knot tangents and the chain itself — extracted from the
 **          anonymous namespace of vsplinepath.cpp so that every tool
 **          which rebuilds a smooth curve out of points (the offsets, the
 **          spiral) shares one implementation instead of one each.
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

#ifndef OC2CHAIN_H
#define OC2CHAIN_H

#include <QPointF>
#include <QString>
#include <QVector>
#include <QtGlobal>

class VSpline;
class VSplinePath;

namespace OC2Chain
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Control points of a base segment.
 */
auto ControlPoints(const VSpline &spline) -> QVector<QPointF>;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Point of a cubic Bezier given by its four control points.
 */
auto BezierPoint(const QVector<QPointF> &control, qreal t) -> QPointF;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief First derivative of a cubic Bezier given by its four control points.
 */
auto BezierDerivative(const QVector<QPointF> &control, qreal t) -> QPointF;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Arc length of one cubic segment, tabulated once so a length can be turned back into a parameter.
 */
class SegmentArcLength
{
public:
    explicit SegmentArcLength(const QVector<QPointF> &control);

    auto Length() const -> qreal;

    /**
     * @brief Parameter of the point sitting at the given arc length, clamped to the segment.
     */
    auto Parameter(qreal length) const -> qreal;

private:
    static constexpr int kTableSize = 32;
    QVector<qreal> m_cumulated;
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Chord lengths between consecutive points.
 */
auto Chords(const QVector<QPointF> &points) -> QVector<qreal>;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Tangents (against the chord length parameter) that make a cubic chain through the points C2.
 *
 * Solves the classical tridiagonal spline system, the two end tangents being clamped. Curvature is what the eye
 * reads on a hem: a C1 chain can still show a crease at every knot, a C2 chain cannot.
 *
 * @param points        knots of the chain, in order.
 * @param chords        distance between two consecutive knots.
 * @param startTangent  direction taken at the first knot (a unit vector).
 * @param endTangent    direction taken at the last knot (a unit vector).
 */
auto Tangents(const QVector<QPointF> &points, const QVector<qreal> &chords, const QPointF &startTangent,
              const QPointF &endTangent) -> QVector<QPointF>;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Cubic chain passing exactly through @p points.
 *
 * Every interior knot is C2, every handle is @c tangent * chord / 3 — the tangent distribution of the classical
 * clamped spline, which is what makes the curvature continuous rather than only the direction.
 *
 * @param points          knots of the chain, at least two.
 * @param startTangent    direction taken at the first knot (a unit vector).
 * @param endTangent      direction taken at the last knot (a unit vector).
 * @param firstPointName  name carried by the first knot; every other knot but the last is named X1 or X4.
 * @param lastPointName   name carried by the last knot. The two ends are what names the path, so they are the
 *                        only knots whose name means anything.
 * @return an empty VSplinePath when there are fewer than two usable knots.
 */
auto Chain(const QVector<QPointF> &points, const QPointF &startTangent, const QPointF &endTangent,
           const QString &firstPointName, const QString &lastPointName) -> VSplinePath;
} // namespace OC2Chain

#endif // OC2CHAIN_H
