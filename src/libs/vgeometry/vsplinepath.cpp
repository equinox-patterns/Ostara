/************************************************************************
 **
 **  @file   vsplinepath.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "vsplinepath.h"

#include "oc2chain.h"

#include <algorithm>
#include <cmath>
#include <QJsonArray>
#include <QJsonObject>
#include <QPoint>

#include "../ifc/ifcdef.h"
#include "../vmisc/defglobal.h"
#include "vexception.h"
#include "vabstractcurve.h"
#include "vsplinepath_p.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VSplinePath constructor.
 * @param idObject parent id.
 * @param mode mode creation spline path.
 */
VSplinePath::VSplinePath(quint32 idObject, Draw mode)
  : VAbstractCubicBezierPath(GOType::SplinePath, idObject, mode),
    d(new VSplinePathData())
{
}

//---------------------------------------------------------------------------------------------------------------------
VSplinePath::VSplinePath(const QVector<VFSplinePoint> &points, qreal kCurve, quint32 idObject, Draw mode)
  : VAbstractCubicBezierPath(GOType::SplinePath, idObject, mode),
    d(new VSplinePathData())
{
    if (points.size() < 3)
    {
        return;
    }

    QVector<VSplinePoint> newPoints(points.size());
    for (qint32 i = 1; i <= points.size() - 1; ++i)
    {
        const VFSplinePoint &p1 = points.at(i - 1);
        const VFSplinePoint &p2 = points.at(i);
        VSpline const spl(p1.P(), p2.P(), p1.Angle2(), p2.Angle1(), p1.KAsm2(), p2.KAsm1(), kCurve);

        newPoints[i - 1].SetP(p1.P());
        newPoints[i - 1].SetAngle2(p1.Angle2(), spl.GetStartAngleFormula());
        newPoints[i - 1].SetLength2(spl.GetC1Length(), spl.GetC1LengthFormula());

        newPoints[i].SetP(p2.P());
        newPoints[i].SetAngle1(p2.Angle1(), spl.GetEndAngleFormula());
        newPoints[i].SetLength1(spl.GetC2Length(), spl.GetC2LengthFormula());
    }

    d->path = newPoints;
    VSplinePath::CreateName();
}

//---------------------------------------------------------------------------------------------------------------------
VSplinePath::VSplinePath(const QVector<VSplinePoint> &points, quint32 idObject, Draw mode)
  : VAbstractCubicBezierPath(GOType::SplinePath, idObject, mode),
    d(new VSplinePathData())
{
    if (points.isEmpty())
    {
        return;
    }

    d->path = points;
    VSplinePath::CreateName();
}

//---------------------------------------------------------------------------------------------------------------------
VSplinePath::VSplinePath(const QVector<VSpline> &path, quint32 idObject, Draw mode)
  : VAbstractCubicBezierPath(GOType::SplinePath, idObject, mode),
    d(new VSplinePathData())
{
    if (path.size() < 2)
    {
        return;
    }

    QVector<VSplinePoint> nodes;
    nodes.reserve(path.size() + 1);

    for (int i = 0; i < path.size(); ++i)
    {
        const VSpline &spl = path.at(i);

        if (i == 0)
        {
            VSplinePoint node;
            node.SetStrict(false);
            node.SetP(spl.GetP1());
            node.SetAngle2(spl.GetStartAngle(), spl.GetStartAngleFormula());
            node.SetLength2(spl.GetC1Length(), spl.GetC1LengthFormula());
            nodes.append(node);
        }
        else
        {
            // Update previous node with this spline's start control info
            VSplinePoint &prevNode = nodes.last();
            prevNode.SetAngle2(spl.GetStartAngle(), spl.GetStartAngleFormula());
            prevNode.SetLength2(spl.GetC1Length(), spl.GetC1LengthFormula());
        }

        VSplinePoint node;
        node.SetStrict(false);
        node.SetP(spl.GetP4());
        node.SetAngle1(spl.GetEndAngle(), spl.GetEndAngleFormula());
        node.SetLength1(spl.GetC2Length(), spl.GetC2LengthFormula());
        nodes.append(node);
    }

    if (nodes.isEmpty())
    {
        return;
    }

    d->path = nodes;
    VSplinePath::CreateName();
}

//---------------------------------------------------------------------------------------------------------------------
COPY_CONSTRUCTOR_IMPL_2(VSplinePath, VAbstractCubicBezierPath)

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::Rotate(const QPointF &originPoint, qreal degrees, const QString &name) const -> VSplinePath
{
    QVector<VSplinePoint> newPoints(CountPoints());
    for (qint32 i = 1; i <= CountSubSpl(); ++i)
    {
        const VSpline spl = GetSpline(i).Rotate(originPoint, degrees);

        newPoints[i - 1].SetP(spl.GetP1());
        newPoints[i - 1].SetAngle2(spl.GetStartAngle(), spl.GetStartAngleFormula());
        newPoints[i - 1].SetLength2(spl.GetC1Length(), spl.GetC1LengthFormula());

        newPoints[i].SetP(spl.GetP4());
        newPoints[i].SetAngle1(spl.GetEndAngle(), spl.GetEndAngleFormula());
        newPoints[i].SetLength1(spl.GetC2Length(), spl.GetC2LengthFormula());
    }

    VSplinePath splPath(newPoints);
    if (!name.isEmpty())
    {
        splPath.SetNameSuffix(name);
        splPath.SetMainNameForHistory(name);
    }

    splPath.SetColor(GetColor());
    splPath.SetPenStyle(GetPenStyle());
    splPath.SetApproximationScale(GetApproximationScale());
    splPath.SetDerivative(true);
    return splPath;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::Flip(const QLineF &axis, const QString &name) const -> VSplinePath
{
    QVector<VSplinePoint> newPoints(CountPoints());
    for (qint32 i = 1; i <= CountSubSpl(); ++i)
    {
        const VSpline spl = GetSpline(i).Flip(axis);

        newPoints[i - 1].SetP(spl.GetP1());
        newPoints[i - 1].SetAngle2(spl.GetStartAngle(), spl.GetStartAngleFormula());
        newPoints[i - 1].SetLength2(spl.GetC1Length(), spl.GetC1LengthFormula());

        newPoints[i].SetP(spl.GetP4());
        newPoints[i].SetAngle1(spl.GetEndAngle(), spl.GetEndAngleFormula());
        newPoints[i].SetLength1(spl.GetC2Length(), spl.GetC2LengthFormula());
    }

    VSplinePath splPath(newPoints);
    if (!name.isEmpty())
    {
        splPath.SetNameSuffix(name);
        splPath.SetMainNameForHistory(name);
    }

    splPath.SetColor(GetColor());
    splPath.SetPenStyle(GetPenStyle());
    splPath.SetApproximationScale(GetApproximationScale());
    splPath.SetDerivative(true);
    return splPath;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::Move(qreal length, qreal angle, const QString &name) const -> VSplinePath
{
    QVector<VSplinePoint> newPoints(CountPoints());
    for (qint32 i = 1; i <= CountSubSpl(); ++i)
    {
        const VSpline spl = GetSpline(i).Move(length, angle);

        newPoints[i - 1].SetP(spl.GetP1());
        newPoints[i - 1].SetAngle2(spl.GetStartAngle(), spl.GetStartAngleFormula());
        newPoints[i - 1].SetLength2(spl.GetC1Length(), spl.GetC1LengthFormula());

        newPoints[i].SetP(spl.GetP4());
        newPoints[i].SetAngle1(spl.GetEndAngle(), spl.GetEndAngleFormula());
        newPoints[i].SetLength1(spl.GetC2Length(), spl.GetC2LengthFormula());
    }

    VSplinePath splPath(newPoints);
    if (!name.isEmpty())
    {
        splPath.SetNameSuffix(name);
        splPath.SetMainNameForHistory(name);
    }

    splPath.SetColor(GetColor());
    splPath.SetPenStyle(GetPenStyle());
    splPath.SetApproximationScale(GetApproximationScale());
    splPath.SetDerivative(true);
    return splPath;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::Offset(qreal distance, const QString &name) const -> VSplinePath
{
    if (qFuzzyIsNull(distance))
    {
        VSplinePath tmp = *this;
        if (!name.isEmpty())
        {
            tmp.SetNameSuffix(name);
            tmp.SetMainNameForHistory(name);
        }
        tmp.SetDerivative(true);
        return tmp;
    }

    QVector<VSpline> offsetPath;
    for (qint32 i = 1; i <= CountSubSpl(); ++i)
    {
        const QVector<VSpline> subSplines = GetSpline(i).OffsetPath(distance);
        offsetPath.append(subSplines);
    }

    if (offsetPath.size() == 1)
    {
        const VSpline spl = offsetPath.constFirst();

        VSpline left;
        VSpline right;
        spl.CutSplineAtParam(0.5, left, right, "X"_L1);

        offsetPath.clear();
        offsetPath = {left, right};
    }

    VSplinePath splPath(offsetPath);
    if (!name.isEmpty())
    {
        splPath.SetNameSuffix(name);
        splPath.SetMainNameForHistory(name);
    }

    splPath.SetColor(GetColor());
    splPath.SetPenStyle(GetPenStyle());
    splPath.SetApproximationScale(GetApproximationScale());
    splPath.SetDerivative(true);
    return splPath;
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Width of the outline along the base curve, as a monotone cubic through the stations.
 *
 * Stations are spread evenly on the arc length: station i sits at t = i / (n - 1). The stations used to be
 * interpolated linearly, which broke the slope of the width at every one of them, and a broken slope is a corner on
 * the widened edge: measured on a half circle widened 6 / 10 / 6, the edge turned by 11 degrees at the middle station
 * (36 degrees when the offset goes inward). A monotone cubic keeps the width exact at the stations, keeps the
 * variation monotone between them, and has a continuous slope, so the corner becomes a curve.
 */
class MonotoneWidthLaw
{
public:
    explicit MonotoneWidthLaw(const QVector<qreal> &stations)
      : m_stations(stations)
    {
        const int count = static_cast<int>(m_stations.size());
        m_slopes.fill(0.0, count);
        if (count < 3)
        {
            // One or two stations: a straight segment is already the exact answer, no slope needed.
            return;
        }

        QVector<qreal> secants(count - 1);
        for (int i = 0; i < count - 1; ++i)
        {
            secants[i] = m_stations.at(i + 1) - m_stations.at(i);
        }

        m_slopes[0] = secants.constFirst();
        m_slopes[count - 1] = secants.constLast();
        for (int i = 1; i < count - 1; ++i)
        {
            m_slopes[i] = (secants.at(i - 1) + secants.at(i)) / 2.0;
        }

        // Fritsch-Carlson limiter: no overshoot between the stations, and a zero slope at every extremum — the top of
        // a flounce is exactly that.
        for (int i = 0; i < count - 1; ++i)
        {
            if (qFuzzyIsNull(secants.at(i)))
            {
                m_slopes[i] = 0.0;
                m_slopes[i + 1] = 0.0;
                continue;
            }

            const qreal alpha = m_slopes.at(i) / secants.at(i);
            const qreal beta = m_slopes.at(i + 1) / secants.at(i);
            const qreal square = alpha * alpha + beta * beta;
            if (square > 9.0)
            {
                const qreal tau = 3.0 / std::sqrt(square);
                m_slopes[i] = tau * alpha * secants.at(i);
                m_slopes[i + 1] = tau * beta * secants.at(i);
            }
        }
    }

    /**
     * @brief Width at the arc length fraction t, in [0, 1].
     */
    auto Width(qreal t) const -> qreal
    {
        const int count = static_cast<int>(m_stations.size());
        if (count == 0)
        {
            return 0.0;
        }

        const qreal x = qBound(0.0, t, 1.0) * (count - 1);
        if (count < 3)
        {
            const int index = qMin(static_cast<int>(std::floor(x)), count - 2);
            return m_stations.at(index) + (m_stations.at(index + 1) - m_stations.at(index)) * (x - index);
        }

        const int index = qMin(static_cast<int>(std::floor(x)), count - 2);
        const qreal h = x - index;

        const qreal h00 = (2.0 * h - 3.0) * h * h + 1.0;
        const qreal h10 = ((h - 2.0) * h + 1.0) * h;
        const qreal h01 = (3.0 - 2.0 * h) * h * h;
        const qreal h11 = (h - 1.0) * h * h;

        return h00 * m_stations.at(index) + h10 * m_slopes.at(index) + h01 * m_stations.at(index + 1) +
               h11 * m_slopes.at(index + 1);
    }

private:
    QVector<qreal> m_stations;
    QVector<qreal> m_slopes; // derivative against the station index
};

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Outline of a base curve at a width that varies along its arc length.
 *
 * The base is sampled along its length, with a sample sitting exactly on every station, and one Catmull-Rom chain is
 * built through the offset points. The code used to offset every base segment on its own and then average the handles
 * to hide the seams: the tangents ended up continuous — SmoothJoints does exactly that, so measuring them says
 * nothing — but the curvature jumped at every seam (measured: 0.22 against a local curvature of 0.10 on a half
 * circle, with the curve flattened to 0.003 just before the seam), which reads as a crease. Sampling removes the
 * seams themselves instead of patching them.
 */
auto OutlineByWidth(const QVector<VSpline> &base, const QVector<qreal> &distances) -> VSplinePath
{
    if (base.isEmpty() || distances.isEmpty())
    {
        return VSplinePath();
    }

    QVector<OC2Chain::SegmentArcLength> tables;
    tables.reserve(base.size());
    qreal totalLength = 0.0;
    for (const VSpline &segment : base)
    {
        tables.append(OC2Chain::SegmentArcLength(OC2Chain::ControlPoints(segment)));
        totalLength += tables.constLast().Length();
    }
    if (qFuzzyIsNull(totalLength))
    {
        return VSplinePath();
    }

    constexpr qreal kSampleStep = 2.0; // cm between two samples of the base curve
    constexpr int kMinSamples = 8;     // per base segment, whatever its length
    constexpr qreal kEpsilon = 1e-6;

    const int stationCount = static_cast<int>(distances.size());
    const MonotoneWidthLaw widthLaw(distances);

    // Samples of the base curve, in arc length, plus one exactly on each interior station: a station rarely falls on a
    // sample, and the widest point of a flounce must not be shaved off.
    QVector<QPointF> points;
    QVector<QPointF> tangents;
    QVector<qreal> lengths;

    qreal accumulated = 0.0;
    for (int segment = 0; segment < base.size(); ++segment)
    {
        const QVector<QPointF> control = OC2Chain::ControlPoints(base.at(segment));
        const OC2Chain::SegmentArcLength &table = tables.at(segment);
        const qreal segmentLength = table.Length();
        const int steps = std::max(kMinSamples, static_cast<int>(std::ceil(segmentLength / kSampleStep)));

        QVector<qreal> positions;
        positions.reserve(steps + stationCount);
        for (int step = 0; step < steps; ++step)
        {
            positions.append(segmentLength * step / steps);
        }

        for (int station = 1; station < stationCount - 1; ++station)
        {
            const qreal stationLength = totalLength * station / (stationCount - 1);
            if (stationLength > accumulated + kEpsilon && stationLength < accumulated + segmentLength - kEpsilon)
            {
                positions.append(stationLength - accumulated);
            }
        }
        std::sort(positions.begin(), positions.end());

        qreal previous = -1.0;
        for (const qreal position : std::as_const(positions))
        {
            if (position - previous < kEpsilon)
            {
                continue;
            }
            previous = position;

            const qreal t = table.Parameter(position);
            const QPointF derivative = OC2Chain::BezierDerivative(control, t);
            const qreal norm = std::hypot(derivative.x(), derivative.y());
            if (qFuzzyIsNull(norm))
            {
                continue; // stationary point: no usable normal here
            }

            points.append(OC2Chain::BezierPoint(control, t));
            tangents.append(derivative / norm);
            lengths.append(accumulated + position);
        }

        accumulated += segmentLength;
    }

    // Last point of the base: the loop above stops at the start of the last segment.
    {
        const QVector<QPointF> control = OC2Chain::ControlPoints(base.constLast());
        const QPointF derivative = OC2Chain::BezierDerivative(control, 1.0);
        const qreal norm = std::hypot(derivative.x(), derivative.y());
        if (!qFuzzyIsNull(norm))
        {
            points.append(OC2Chain::BezierPoint(control, 1.0));
            tangents.append(derivative / norm);
            lengths.append(totalLength);
        }
    }

    if (points.size() < 3)
    {
        return VSplinePath();
    }

    // Offset points. The normal is the one the previous code used for a positive width.
    QVector<QPointF> outlinePoints;
    outlinePoints.reserve(points.size());
    for (int i = 0; i < points.size(); ++i)
    {
        const qreal width = widthLaw.Width(lengths.at(i) / totalLength);
        const QPointF tangent = tangents.at(i);
        outlinePoints.append(points.at(i) + width * QPointF(-tangent.y(), tangent.x()));
    }

    // One C2 chain through the offset points: the tangents are solved so the curvature is continuous at every sample.
    // A chain that is only C1 (Catmull-Rom, or handles averaged at the seams) keeps the tangent smooth but lets the
    // curvature jump at every knot, and a curvature jump reads as a crease on a hem.
    const QVector<qreal> chords = OC2Chain::Chords(outlinePoints);

    const qreal firstChord = chords.constFirst();
    const qreal lastChord = chords.constLast();
    const QPointF startTangent =
        qFuzzyIsNull(firstChord) ? QPointF() : (outlinePoints.at(1) - outlinePoints.constFirst()) / firstChord;
    const QPointF endTangent = qFuzzyIsNull(lastChord)
                                   ? QPointF()
                                   : (outlinePoints.constLast() - outlinePoints.at(outlinePoints.size() - 2)) /
                                         lastChord;

    return OC2Chain::Chain(outlinePoints, startTangent, endTangent, "X1"_L1, "X4"_L1);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::Outline(const QVector<qreal> &distances, const QString &name) const -> VSplinePath
{
    // A width that varies along the curve is not a set of independent offsets: the whole curve is sampled at once, so
    // that neither the curvature (seams) nor the width law (stations) can break. See OutlineByWidth().
    if (distances.size() > 1)
    {
        QVector<VSpline> base;
        base.reserve(CountSubSpl());
        for (vsizetype i = 1; i <= CountSubSpl(); ++i)
        {
            base.append(GetSpline(i));
        }

        VSplinePath widthPath = OutlineByWidth(base, distances);
        if (widthPath.CountSubSpl() > 0)
        {
            if (!name.isEmpty())
            {
                widthPath.SetNameSuffix(name);
                widthPath.SetMainNameForHistory(name);
            }
            widthPath.SetColor(GetColor());
            widthPath.SetPenStyle(GetPenStyle());
            widthPath.SetApproximationScale(GetApproximationScale());
            widthPath.SetDerivative(true);
            return widthPath;
        }
    }

    qreal const totalLen = GetLength();
    qreal accumulated = 0.0;
    const auto maxIndex = static_cast<int>(distances.size() - 1);

    // 2. Helper: Linear Interpolator
    // Returns the exact width at a precise floating-point index
    auto GetInterpolatedWidth = [&](double preciseIndex) -> qreal
    {
        if (preciseIndex <= 0.0)
        {
            return distances.first();
        }
        if (preciseIndex >= maxIndex)
        {
            return distances.last();
        }

        const int idx = static_cast<int>(preciseIndex);
        const double fraction = preciseIndex - idx;
        return distances.at(idx) * (1.0 - fraction) + distances.at(idx + 1) * fraction;
    };

    QVector<VSpline> outlinePath;
    for (qint32 i = 1; i <= CountSubSpl(); ++i)
    {
        VSpline const seg = GetSpline(i);
        if (distances.size() > 1)
        {
            qreal const segLen = seg.GetLength();

            // Calculate Global 't' (0.0 to 1.0)
            double const tStart = accumulated / totalLen;
            double const tEnd = (accumulated + segLen) / totalLen;

            // Map Global 't' to 'distances' array indices (e.g., 0.0 to 99.0)
            double const indexStart = tStart * maxIndex;
            double const indexEnd = tEnd * maxIndex;

            QVector<qreal> localDistances;

            // A. ALWAYS add the exact interpolated width at the start of this segment
            localDistances.append(GetInterpolatedWidth(indexStart));

            // B. Capture any ORIGINAL values that fall strictly between start and end
            // Using ceil/floor to find the integer indices inside the range
            auto firstWholeIdx = static_cast<int>(std::ceil(indexStart));
            auto lastWholeIdx = static_cast<int>(std::floor(indexEnd));

            // Only consider indices that are strictly inside (avoid duplicating start/end if they align exactly)
            // Small epsilon might be needed if floating point precision is an issue,
            // but generally strict comparison works for the "between" logic.
            if (firstWholeIdx <= indexStart)
            {
                firstWholeIdx++;
            }
            if (lastWholeIdx >= indexEnd)
            {
                lastWholeIdx--;
            }

            for (int k = firstWholeIdx; k <= lastWholeIdx; ++k)
            {
                // Clamp k just in case logic slips out of bounds
                if (k >= 0 && k < distances.size())
                {
                    localDistances.append(distances[k]);
                }
            }

            // C. ALWAYS add the exact interpolated width at the end
            localDistances.append(GetInterpolatedWidth(indexEnd));

            const QVector<VSpline> subSplines = seg.OutlinePath(localDistances);
            outlinePath.append(subSplines);

            accumulated += segLen;
        }
        else
        {
            const QVector<VSpline> subSplines = seg.OutlinePath(distances);
            outlinePath.append(subSplines);
        }
    }

    if (outlinePath.size() == 1)
    {
        const VSpline spl = outlinePath.constFirst();

        VSpline left;
        VSpline right;
        spl.CutSplineAtParam(0.5, left, right, "X"_L1);

        outlinePath.clear();
        outlinePath = {left, right};
    }

    VSplinePath splPath(outlinePath);
    if (!name.isEmpty())
    {
        splPath.SetNameSuffix(name);
        splPath.SetMainNameForHistory(name);
    }
    splPath.SetColor(GetColor());
    splPath.SetPenStyle(GetPenStyle());
    splPath.SetApproximationScale(GetApproximationScale());
    splPath.SetDerivative(true);
    return splPath;
}

//---------------------------------------------------------------------------------------------------------------------
VSplinePath::~VSplinePath() = default;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief append add point in the end of list points.
 * @param point new point.
 */
void VSplinePath::append(const VSplinePoint &point)
{
    d->path.append(point);
    CreateName();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CountSubSpl return count of simple splines.
 * @return count.
 */
auto VSplinePath::CountSubSpl() const -> vsizetype
{
    if (d->path.isEmpty())
    {
        return 0;
    }

    return d->path.size() - 1;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSpline return spline by index.
 * @param index index spline in spline path.
 * @return spline
 */
auto VSplinePath::GetSpline(vsizetype index) const -> VSpline
{
    if (CountPoints() < 1)
    {
        throw VException(tr("Not enough points to create the spline."));
    }

    if (index < 1 || index > CountSubSpl())
    {
        throw VException(tr("This spline does not exist."));
    }

    const VSplinePoint &p1 = d->path.at(index - 1);
    const VSplinePoint &p2 = d->path.at(index);
    VSpline spl(p1.P(), p2.P(), p1.Angle2(), p1.Angle2Formula(), p2.Angle1(), p2.Angle1Formula(), p1.Length2(),
                p1.Length2Formula(), p2.Length1(), p2.Length1Formula(), 1);
    spl.SetApproximationScale(GetApproximationScale());
    return spl;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdatePoint update spline point in list.
 * @param indexSpline spline index in list.
 * @param pos position point in spline.
 * @param point point.
 */
void VSplinePath::UpdatePoint(qint32 indexSpline, const SplinePointPosition &pos, const VSplinePoint &point)
{
    if (indexSpline < 1 || indexSpline > CountSubSpl())
    {
        throw VException(tr("This spline does not exist."));
    }
    if (pos == SplinePointPosition::FirstPoint)
    {
        d->path[indexSpline - 1] = point;
    }
    else
    {
        d->path[indexSpline] = point;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSplinePoint return spline point from list.
 * @param indexSpline spline index in list.
 * @param pos position point in spline.
 * @return spline point.
 */
auto VSplinePath::GetSplinePoint(qint32 indexSpline, SplinePointPosition pos) const -> VSplinePoint
{
    if (indexSpline < 1 || indexSpline > CountSubSpl())
    {
        throw VException(tr("This spline does not exist."));
    }
    if (pos == SplinePointPosition::FirstPoint)
    {
        return d->path.at(indexSpline - 1);
    }
    else
    {
        return d->path.at(indexSpline);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief operator = assignment operator.
 * @param path spline path.
 * @return spline path.
 */
auto VSplinePath::operator=(const VSplinePath &path) -> VSplinePath &
{
    if (&path == this)
    {
        return *this;
    }
    VAbstractCubicBezierPath::operator=(path);
    d = path.d;
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
VSplinePath::VSplinePath(VSplinePath &&splPath) noexcept
  : VAbstractCubicBezierPath(std::move(splPath)),
    d(std::move(splPath.d)) // NOLINT(bugprone-use-after-move)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::operator=(VSplinePath &&path) noexcept -> VSplinePath &
{
    VAbstractCubicBezierPath::operator=(path);
    std::swap(d, path.d);
    return *this;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief operator [] return spline point by index.
 * @param indx index in list.
 * @return spline point.
 */
auto VSplinePath::operator[](vsizetype indx) -> VSplinePoint &
{
    return d->path[indx];
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief at return spline point by index.
 * @param indx index in list.
 * @return spline point.
 */
auto VSplinePath::at(vsizetype indx) const -> const VSplinePoint &
{
    return d->path[indx];
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::ToJson() const -> QJsonObject
{
    QJsonObject object = VAbstractCubicBezierPath::ToJson();
    object["aScale"_L1] = GetApproximationScale();
    object["mainName"_L1] = GetMainNameForHistory();

    QJsonArray nodesArray;
    for (const auto &node : d->path)
    {
        nodesArray.append(node.ToJson());
    }
    object["nodes"_L1] = nodesArray;

    return object;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::DirectionArrows() const -> QVector<DirectionArrow>
{
    if (IsRelaxed())
    {
        return VAbstractCurve::DirectionArrows(); // NOLINT(bugprone-parent-virtual-call)
    }

    return VAbstractCubicBezierPath::DirectionArrows();
}

//---------------------------------------------------------------------------------------------------------------------
void VSplinePath::SetStrict(bool strict)
{
    for (auto &node : d->path)
    {
        node.SetStrict(strict);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::NameForHistory(const QString &toolName) const -> QString
{
    QString const name = toolName + GetMainNameForHistory();

    QString alias;

    if (not GetAliasSuffix().isEmpty())
    {
        alias = u"%1 %2"_s.arg(toolName, GetAliasSuffix());
    }

    return not alias.isEmpty() ? u"%1 (%2)"_s.arg(alias, name) : name;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::GetStartAngle() const -> qreal
{
    return CountPoints() > 0 ? GetSplinePath().constFirst().Angle2() : 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::GetEndAngle() const -> qreal
{
    if (CountPoints() > 0)
    {
        return GetSplinePath().constLast().Angle1();
    }

    return 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::GetC1Length() const -> qreal
{
    return CountPoints() > 0 ? GetSplinePath().constFirst().Length2() : 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::GetC2Length() const -> qreal
{
    return CountPoints() > 0 ? GetSplinePath().constFirst().Length1() : 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::FirstPoint() const -> VPointF
{
    return not d->path.isEmpty() ? d->path.constFirst().P() : VPointF();
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::LastPoint() const -> VPointF
{
    const vsizetype count = CountSubSpl();
    return count >= 1 ? d->path.at(count).P() : // Take last point of the last real spline
               VPointF();
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::GetMainNameForHistory() const -> QString
{
    if (!d->mainNameForHistory.isEmpty())
    {
        return d->mainNameForHistory;
    }

    QString name;
    if (CountPoints() > 0)
    {
        name += u" %1"_s.arg(FirstPoint().name());
        if (CountSubSpl() >= 1)
        {
            name += u"_%1"_s.arg(LastPoint().name());
        }

        if (GetDuplicate() > 0)
        {
            name += u"_%1"_s.arg(GetDuplicate());
        }
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
void VSplinePath::SetMainNameForHistory(const QString &name)
{
    d->mainNameForHistory = name;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CountPoints return count of points.
 * @return count.
 */
auto VSplinePath::CountPoints() const -> vsizetype
{
    return d->path.size();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSplinePath return list with spline points.
 * @return list.
 */
auto VSplinePath::GetSplinePath() const -> QVector<VSplinePoint>
{
    return d->path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VSplinePath::GetFSplinePath() const -> QVector<VFSplinePoint>
{
    if (d->path.isEmpty())
    {
        return {};
    }

    QVector<VFSplinePoint> points;
    points.resize(d->path.size());

    for (qint32 i = 1; i <= CountSubSpl(); ++i)
    {
        const VSplinePoint &p1 = d->path.at(i - 1);
        const VSplinePoint &p2 = d->path.at(i);
        VSpline const spl(p1.P(), p2.P(), p1.Angle2(), p1.Angle2Formula(), p2.Angle1(), p2.Angle1Formula(),
                          p1.Length2(), p1.Length2Formula(), p2.Length1(), p2.Length1Formula(), 1);

        points[i - 1].SetP(p1.P());
        points[i - 1].SetAngle2(p1.Angle2());
        points[i - 1].SetKAsm2(spl.GetKasm1());

        points[i].SetP(p2.P());
        points[i].SetAngle1(p2.Angle1());
        points[i].SetKAsm1(spl.GetKasm2());
    }

    return points;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Clear clear list of points.
 */
void VSplinePath::Clear()
{
    d->path.clear();
    SetDuplicate(0);
}
