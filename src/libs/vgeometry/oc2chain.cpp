/************************************************************************
 **
 **  @file   oc2chain.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Cubic chain through sampled points, C2 at every interior knot.
 **          Extracted from the anonymous namespace of vsplinepath.cpp.
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

#include "oc2chain.h"

#include <QLineF>
#include <QPointF>
#include <QString>
#include <QVector>

#include <algorithm>
#include <cmath>

#include "vpointf.h"
#include "vspline.h"
#include "vsplinepath.h"

namespace OC2Chain
{
//---------------------------------------------------------------------------------------------------------------------
auto ControlPoints(const VSpline &spline) -> QVector<QPointF>
{
    return {spline.GetP1().toQPointF(), spline.GetP2().toQPointF(), spline.GetP3().toQPointF(),
            spline.GetP4().toQPointF()};
}

//---------------------------------------------------------------------------------------------------------------------
auto BezierPoint(const QVector<QPointF> &control, qreal t) -> QPointF
{
    const qreal u = 1.0 - t;
    return u * u * u * control.at(0) + 3.0 * u * u * t * control.at(1) + 3.0 * u * t * t * control.at(2) +
           t * t * t * control.at(3);
}

//---------------------------------------------------------------------------------------------------------------------
auto BezierDerivative(const QVector<QPointF> &control, qreal t) -> QPointF
{
    const qreal u = 1.0 - t;
    return 3.0 * u * u * (control.at(1) - control.at(0)) + 6.0 * u * t * (control.at(2) - control.at(1)) +
           3.0 * t * t * (control.at(3) - control.at(2));
}

//---------------------------------------------------------------------------------------------------------------------
SegmentArcLength::SegmentArcLength(const QVector<QPointF> &control)
{
    m_cumulated.reserve(kTableSize + 1);
    m_cumulated.append(0.0);

    QPointF previous = BezierPoint(control, 0.0);
    for (int i = 1; i <= kTableSize; ++i)
    {
        const QPointF current = BezierPoint(control, static_cast<qreal>(i) / kTableSize);
        m_cumulated.append(m_cumulated.constLast() + QLineF(previous, current).length());
        previous = current;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto SegmentArcLength::Length() const -> qreal
{
    return m_cumulated.constLast();
}

//---------------------------------------------------------------------------------------------------------------------
auto SegmentArcLength::Parameter(qreal length) const -> qreal
{
    if (length <= 0.0)
    {
        return 0.0;
    }
    if (length >= m_cumulated.constLast())
    {
        return 1.0;
    }

    const auto upper = std::upper_bound(m_cumulated.cbegin(), m_cumulated.cend(), length);
    const auto index = static_cast<int>(upper - m_cumulated.cbegin());
    const qreal before = m_cumulated.at(index - 1);
    const qreal after = m_cumulated.at(index);
    const qreal fraction = qFuzzyIsNull(after - before) ? 0.0 : (length - before) / (after - before);
    return (index - 1 + fraction) / kTableSize;
}

//---------------------------------------------------------------------------------------------------------------------
auto Chords(const QVector<QPointF> &points) -> QVector<qreal>
{
    QVector<qreal> chords(std::max(0, static_cast<int>(points.size()) - 1));
    for (int i = 0; i + 1 < points.size(); ++i)
    {
        chords[i] = std::hypot(points.at(i + 1).x() - points.at(i).x(), points.at(i + 1).y() - points.at(i).y());
    }
    return chords;
}

//---------------------------------------------------------------------------------------------------------------------
auto Tangents(const QVector<QPointF> &points, const QVector<qreal> &chords, const QPointF &startTangent,
              const QPointF &endTangent) -> QVector<QPointF>
{
    const int count = static_cast<int>(points.size());
    QVector<QPointF> tangents(count);
    if (count < 2)
    {
        return tangents;
    }

    tangents[0] = startTangent;
    tangents[count - 1] = endTangent;
    if (count == 2)
    {
        return tangents;
    }

    const int unknowns = count - 2;
    QVector<qreal> lower(unknowns); // 1 / d(i-1)
    QVector<qreal> diagonal(unknowns);
    QVector<qreal> upper(unknowns); // 1 / d(i)
    QVector<qreal> rightX(unknowns);
    QVector<qreal> rightY(unknowns);

    for (int k = 0; k < unknowns; ++k)
    {
        const int i = k + 1;
        const qreal previous = chords.at(i - 1);
        const qreal next = chords.at(i);

        lower[k] = qFuzzyIsNull(previous) ? 0.0 : 1.0 / previous;
        upper[k] = qFuzzyIsNull(next) ? 0.0 : 1.0 / next;
        diagonal[k] = 2.0 * (lower.at(k) + upper.at(k));

        const QPointF right = 3.0 * ((points.at(i) - points.at(i - 1)) / (previous * previous) +
                                     (points.at(i + 1) - points.at(i)) / (next * next));
        rightX[k] = right.x();
        rightY[k] = right.y();

        if (k == 0)
        {
            rightX[k] -= lower.at(k) * startTangent.x();
            rightY[k] -= lower.at(k) * startTangent.y();
        }
        if (k == unknowns - 1)
        {
            rightX[k] -= upper.at(k) * endTangent.x();
            rightY[k] -= upper.at(k) * endTangent.y();
        }
    }

    // Thomas algorithm, one elimination pass for both coordinates.
    for (int k = 1; k < unknowns; ++k)
    {
        const qreal factor = qFuzzyIsNull(diagonal.at(k - 1)) ? 0.0 : lower.at(k) / diagonal.at(k - 1);
        diagonal[k] -= factor * upper.at(k - 1);
        rightX[k] -= factor * rightX.at(k - 1);
        rightY[k] -= factor * rightY.at(k - 1);
    }

    QVector<qreal> solutionX(unknowns);
    QVector<qreal> solutionY(unknowns);
    for (int k = unknowns - 1; k >= 0; --k)
    {
        const qreal followingX = (k + 1 < unknowns) ? solutionX.at(k + 1) : 0.0;
        const qreal followingY = (k + 1 < unknowns) ? solutionY.at(k + 1) : 0.0;
        const qreal divisor = qFuzzyIsNull(diagonal.at(k)) ? 1.0 : diagonal.at(k);

        solutionX[k] = (rightX.at(k) - upper.at(k) * followingX) / divisor;
        solutionY[k] = (rightY.at(k) - upper.at(k) * followingY) / divisor;
    }

    for (int k = 0; k < unknowns; ++k)
    {
        tangents[k + 1] = QPointF(solutionX.at(k), solutionY.at(k));
    }

    return tangents;
}

//---------------------------------------------------------------------------------------------------------------------
auto Chain(const QVector<QPointF> &points, const QPointF &startTangent, const QPointF &endTangent,
           const QString &firstPointName, const QString &lastPointName) -> VSplinePath
{
    if (points.size() < 2)
    {
        return VSplinePath();
    }

    const QVector<qreal> chords = Chords(points);
    const QVector<QPointF> knotTangents = Tangents(points, chords, startTangent, endTangent);

    QVector<VSpline> chain;
    chain.reserve(points.size() - 1);
    for (int i = 0; i + 1 < points.size(); ++i)
    {
        const qreal chord = chords.at(i);
        if (qFuzzyIsNull(chord))
        {
            continue;
        }

        // Only the two ends carry a name that means something: they are what names the path. The knots in
        // between keep the X1 / X4 labels a derivative curve has always used.
        const QString name1 = i == 0 ? firstPointName : QStringLiteral("X1");
        const QString name4 = (i + 1 == points.size() - 1) ? lastPointName : QStringLiteral("X4");

        chain.append(VSpline(VPointF(points.at(i), name1), points.at(i) + knotTangents.at(i) * (chord / 3.0),
                             points.at(i + 1) - knotTangents.at(i + 1) * (chord / 3.0),
                             VPointF(points.at(i + 1), name4)));
    }

    if (chain.size() < 2)
    {
        return VSplinePath();
    }

    return VSplinePath(chain);
}
} // namespace OC2Chain
