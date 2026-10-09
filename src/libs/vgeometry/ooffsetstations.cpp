/************************************************************************
 **
 **  @file   ooffsetstations.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  Where a variable-width offset acts: for every width, the point
 **          of the base curve it is measured from and the point it lands on.
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
#include "ooffsetstations.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

#include "oc2chain.h"
#include "vabstractarc.h"
#include "vabstractcubicbezierpath.h"
#include "vabstractcurve.h"
#include "vspline.h"
#include "vsplinepath.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The cubic segments of a base curve, whatever kind it is.
 *
 * These are the four curves a variable width is defined on — the ones the graduated curve dialog accepts: an arc, an
 * elliptical arc, a spline and a spline path. A cubic Bézier is none of them (no tool offers one as the base of a
 * variable-width offset), and neither is anything else, so the result is empty there rather than wrong.
 */
auto BaseSegments(const VAbstractCurve &curve) -> QVector<VSpline>
{
    const auto PathSegments = [](const VAbstractCubicBezierPath &path) -> QVector<VSpline>
    {
        QVector<VSpline> segments;
        segments.reserve(path.CountSubSpl());
        for (vsizetype i = 1; i <= path.CountSubSpl(); ++i)
        {
            segments.append(path.GetSpline(i));
        }
        return segments;
    };

    if (const auto *path = dynamic_cast<const VAbstractCubicBezierPath *>(&curve))
    {
        return PathSegments(*path);
    }

    if (const auto *spline = dynamic_cast<const VSpline *>(&curve))
    {
        return {*spline};
    }

    if (const auto *arc = dynamic_cast<const VAbstractArc *>(&curve))
    {
        return PathSegments(arc->ToSplinePath());
    }

    return {};
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OOffsetStations::Stations(const VAbstractCurve &curve, const QVector<qreal> &widths) -> QVector<OOffsetStation>
{
    QVector<OOffsetStation> stations;

    const QVector<VSpline> segments = BaseSegments(curve);
    if (segments.isEmpty() || widths.isEmpty())
    {
        return stations;
    }

    // The very tables OutlineByWidth() samples the base with: a marker reading its position from another measure
    // would name a point the result curve does not go through.
    QVector<OC2Chain::SegmentArcLength> tables;
    tables.reserve(segments.size());
    qreal totalLength = 0.0;
    for (const VSpline &segment : segments)
    {
        tables.append(OC2Chain::SegmentArcLength(OC2Chain::ControlPoints(segment)));
        totalLength += tables.constLast().Length();
    }
    if (qFuzzyIsNull(totalLength))
    {
        return stations;
    }

    // An arc taken the other way round offsets by the opposite of the widths it is given — VAbstractArc::Outline()
    // negates them all — and a marker has to land where the curve does, not where the number says.
    const auto *arc = dynamic_cast<const VAbstractArc *>(&curve);
    const qreal orientation = (arc != nullptr && arc->IsNegative()) ? -1.0 : 1.0;

    // One width is not a set of stations: the offset is the same all along the curve, and its two ends bound it.
    // From two widths on, station i sits at i/(n-1) of the arc length, exactly as in OutlineByWidth().
    const int widthCount = static_cast<int>(widths.size());
    const int stationCount = std::max(widthCount, 2);
    const int lastStation = stationCount - 1;

    stations.reserve(stationCount);
    for (int i = 0; i < stationCount; ++i)
    {
        const int index = qMin(i, widthCount - 1);
        const qreal wanted = totalLength * i / lastStation;

        int segment = 0;
        qreal accumulated = 0.0;
        while (segment < segments.size() - 1 && wanted > accumulated + tables.at(segment).Length())
        {
            accumulated += tables.at(segment).Length();
            ++segment;
        }

        const QVector<QPointF> control = OC2Chain::ControlPoints(segments.at(segment));
        const qreal local = qBound(0.0, wanted - accumulated, tables.at(segment).Length());
        const qreal t = tables.at(segment).Parameter(local);
        const QPointF derivative = OC2Chain::BezierDerivative(control, t);
        const qreal norm = std::hypot(derivative.x(), derivative.y());
        if (qFuzzyIsNull(norm))
        {
            continue; // a stationary point has no normal to offset along, and the engine skips it too
        }

        const QPointF tangent = derivative / norm;
        const QPointF base = OC2Chain::BezierPoint(control, t);
        const qreal width = widths.at(index) * orientation;

        stations.append({.index = index,
                         .base = base,
                         .target = base + width * QPointF(-tangent.y(), tangent.x()),
                         .width = width});
    }

    return stations;
}
