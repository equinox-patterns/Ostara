/************************************************************************
 **
 **  @file   opleatsupport.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The piece's edge at one point of a pleat's prise: the direction the fold has to follow.
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

#include "opleatsupport.h"

#include <QLineF>
#include <QPointF>
#include <QSharedPointer>

#include "../../vabstracttool.h"
#include "../../vgeometry/vabstractarc.h"
#include "../../vgeometry/vabstractcubicbezier.h"
#include "../../vgeometry/vabstractcurve.h"
#include "../../vgeometry/varc.h"
#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vgeometrydef.h"
#include "../../vgeometry/vpointf.h"
#include "../../vpatterndb/vcontainer.h"
#include "../ifc/xml/vabstractpattern.h"

namespace
{
/** Below this length a vector carries no direction at all: a degenerate segment, or two points that the pattern
 *  put on top of each other, answer as little as a point that has no edge. */
constexpr qreal kDirectionTolerance = 1e-7;

//---------------------------------------------------------------------------------------------------------------------
/** @brief The position of a point of the container, or a null vector when the id names no point. */
auto PositionOf(quint32 pointId, const VContainer *data) -> QPointF
{
    if (data == nullptr || not data->HasObject(pointId))
    {
        return {};
    }

    const QSharedPointer<VPointF> point = qSharedPointerDynamicCast<VPointF>(data->GetGObject(pointId));
    return point.isNull() ? QPointF() : static_cast<QPointF>(*point);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The one segment a tool draws through a point, as a direction: the line that point sits on.
 *
 *  A tool that draws two of them through the same point means a corner — the vertex of a triangle, the meeting
 *  of a side and a diagonal — and nothing here may choose between two edges: the answer is then empty, exactly
 *  as for a point that carries none. */
auto DirectionFromSegments(quint32 pointId, quint32 toolId, const VContainer *data) -> QPointF
{
    auto *tool = qobject_cast<VAbstractTool *>(VAbstractPattern::FindTool(toolId));
    if (tool == nullptr)
    {
        return {};
    }

    quint32 other = NULL_ID;
    for (const VDrawnSegment &segment : tool->DrawnSegments())
    {
        quint32 candidate = NULL_ID;
        if (segment.point1 == pointId)
        {
            candidate = segment.point2;
        }
        else if (segment.point2 == pointId)
        {
            candidate = segment.point1;
        }
        else
        {
            continue;
        }

        if (other != NULL_ID && other != candidate)
        {
            return {}; // two edges through one point: a corner, and no way to tell which one the fold follows
        }
        other = candidate;
    }

    if (other == NULL_ID)
    {
        return {};
    }

    return PositionOf(pointId, data) - PositionOf(other, data);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The tangent of a curve at one of its two ends, or a null vector when @a point is not one of them.
 *
 *  A bezier curve stores the direction of each end as the handle leaving it, so the tangent is read exactly.
 *  A circular arc is tangent to the radius at every point, so the perpendicular of the radius is its tangent
 *  line — in either direction, since only the line is read. An elliptical arc is tangent to the perpendicular
 *  of no such radius, and its tangent would have to be derived from the ellipse itself: it is left unanswered,
 *  and the user designates a point there instead. */
auto TangentAt(const QSharedPointer<VAbstractCurve> &curve, const QPointF &point) -> QPointF
{
    if (const auto bezier = qSharedPointerDynamicCast<VAbstractCubicBezier>(curve); not bezier.isNull())
    {
        const QPointF start = static_cast<QPointF>(bezier->GetP1());
        if (VFuzzyComparePoints(start, point))
        {
            return static_cast<QPointF>(bezier->GetP2()) - start; // the first handle lies along the tangent
        }

        const QPointF end = static_cast<QPointF>(bezier->GetP4());
        if (VFuzzyComparePoints(end, point))
        {
            return end - static_cast<QPointF>(bezier->GetP3()); // and the second one, from the other end
        }

        return {};
    }

    if (const auto arc = qSharedPointerDynamicCast<VArc>(curve); not arc.isNull())
    {
        const QPointF radius = point - static_cast<QPointF>(arc->GetCenter());
        return {-(radius.y()), radius.x()};
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The tangent, at the prise point, of a curve that point was cut out of.
 *
 *  A cut leaves two sub-curves in the container, both of them carrying the id of the tool that cut them, and the
 *  cut point is an end of each. Looking for them there is what makes the tangent readable at any point of a
 *  curve, and not only at the two ends of the curve the user drew. */
auto DirectionFromCurves(quint32 pointId, quint32 toolId, const VContainer *data) -> QPointF
{
    const QPointF position = PositionOf(pointId, data);
    const auto *objects = data->CalculationGObjects();
    if (objects == nullptr)
    {
        return {};
    }

    for (auto it = objects->constBegin(); it != objects->constEnd(); ++it)
    {
        const QSharedPointer<VGObject> &object = it.value();
        if (object.isNull() || object->getIdObject() != toolId)
        {
            continue; // only what that very tool produced: a cut's own two sub-curves, not the whole pattern
        }

        const QPointF direction = TangentAt(qSharedPointerDynamicCast<VAbstractCurve>(object), position);
        if (QLineF(QPointF(), direction).length() > kDirectionTolerance)
        {
            return direction;
        }
    }

    return {};
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OPleatSupport::Resolve(OPleatEnd &end, const VContainer *data)
{
    end.position = PositionOf(end.pointId, data);

    if (end.edgePointId != NULL_ID)
    {
        const QPointF designated = PositionOf(end.edgePointId, data);
        if (QLineF(QPointF(), designated).length() > kDirectionTolerance)
        {
            end.edgeDirection = designated - end.position;
            return;
        }
    }

    end.edgeDirection = EdgeDirection(end.pointId, data);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatSupport::EdgeDirection(quint32 pointId, const VContainer *data) -> QPointF
{
    if (data == nullptr || not data->HasObject(pointId))
    {
        return {};
    }

    const QSharedPointer<VGObject> object = data->GetGObject(pointId);
    if (object.isNull())
    {
        return {};
    }

    // The tool that drew the point is the one that knows its edge. An object an operation produced says which
    // operation made it — what draws the edge there is that operation, not the point. A point drawn by a tool of its
    // own carries no such name, because the tool and its point ARE one element of the file: the point's own id is
    // then the id of the tool to ask.
    const quint32 toolId = (object->getIdObject() != NULL_ID) ? object->getIdObject() : pointId;

    const QPointF direction = DirectionFromSegments(pointId, toolId, data);
    if (QLineF(QPointF(), direction).length() > kDirectionTolerance)
    {
        return direction;
    }

    return DirectionFromCurves(pointId, toolId, data);
}
