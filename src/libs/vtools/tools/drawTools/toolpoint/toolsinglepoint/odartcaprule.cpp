/************************************************************************
 **
 **  @file   odartcaprule.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  the cap of a dart: the rule, written once
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

#include "odartcaprule.h"

#include <QLineF>

#include "../vgeometry/vgobject.h"

//---------------------------------------------------------------------------------------------------------------------
auto ODartCapRule::IsFailure(Failure failure) -> bool
{
    return failure != Failure::None;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODartCapRule::Midpoint(const QPointF &firstPoint, const QPointF &secondPoint) -> QPointF
{
    return QPointF((firstPoint.x() + secondPoint.x()) / 2., (firstPoint.y() + secondPoint.y()) / 2.);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODartCapRule::NearestBasePoint(const QPointF &extendPoint, const QPointF &firstPoint,
                                    const QPointF &secondPoint) -> int
{
    return QLineF(extendPoint, secondPoint).length() < QLineF(extendPoint, firstPoint).length() ? 1 : 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODartCapRule::CapPoint(const QPointF &apex, const QPointF &firstPoint, const QPointF &secondPoint,
                            const QPointF &extendPoint, QPointF *capPoint) -> Failure
{
    if (QLineF(firstPoint, secondPoint).isNull())
    {
        return Failure::DegenerateBase; // no opening to cap
    }

    const QPointF midpoint = Midpoint(firstPoint, secondPoint);
    const QLineF axis(apex, midpoint);
    if (axis.isNull())
    {
        return Failure::DegenerateApex; // the dart has no direction to build an axis on
    }

    const QPointF basePoint =
        NearestBasePoint(extendPoint, firstPoint, secondPoint) == 0 ? firstPoint : secondPoint;
    const QLineF extendLine(extendPoint, basePoint);
    if (extendLine.isNull())
    {
        return Failure::DegenerateExtend; // the line to extend is carried by a single point
    }

    QPointF intersection;
    if (VGObject::LinesIntersect(extendLine, axis, &intersection) == QLineF::NoIntersection)
    {
        return Failure::NoIntersection; // parallel or confounded: no cap point exists
    }

    *capPoint = intersection;
    return Failure::None;
}
