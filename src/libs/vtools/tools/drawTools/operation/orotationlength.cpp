/************************************************************************
 **
 **  @file   orotationlength.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The rule that ties the angle of a rotation to the length it draws (see the header).
 **          The geometry itself is in vgeometrydef (RotationChord, RotationAngleForChord); what is
 **          decided here is the unit, the rounding, and where a length stops being possible.
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

#include "orotationlength.h"

#include "../vgeometry/vgeometrydef.h"
#include "../vmisc/def.h"
#include "../vpatterndb/vcontainer.h"

//---------------------------------------------------------------------------------------------------------------------
auto ORotationLength::ReferencePoint(const VContainer *data, const QVector<quint32> &sourceIds) -> quint32
{
    return data != nullptr ? data->FirstPointId(sourceIds) : NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto ORotationLength::Radius(const VContainer *data, quint32 pivotId, quint32 referenceId) -> qreal
{
    return data != nullptr ? data->PointsDistance(pivotId, referenceId) : 0.;
}

//---------------------------------------------------------------------------------------------------------------------
auto ORotationLength::HasRadius(qreal radius) -> bool
{
    return qIsFinite(radius) && !qFuzzyIsNull(radius);
}

//---------------------------------------------------------------------------------------------------------------------
auto ORotationLength::LengthOfAngle(qreal radius, qreal angle, const Unit &unit) -> qreal
{
    // The chord carries the sign of the rotation, which is what lets a length say the direction.
    return RoundForDisplay(FromPixel(RotationChord(radius, angle), unit));
}

//---------------------------------------------------------------------------------------------------------------------
auto ORotationLength::AngleOfLength(qreal radius, qreal length, const Unit &unit) -> qreal
{
    const qreal diameter = 2. * radius;
    qreal chord = ToPixel(length, unit);

    // The field shows three decimals, so a length typed as the diameter can pass the exact one by up to half
    // a step of that display once turned back into pixels.
    const qreal halfStep = ToPixel(DisplayTolerance, unit);

    if (!HasRadius(radius) || qAbs(chord) > diameter + halfStep)
    {
        return qQNaN(); // further from the pivot than the circle it draws around it
    }

    chord = qBound(-diameter, chord, diameter);

    return RoundForDisplay(RotationAngleForChord(radius, chord));
}

//---------------------------------------------------------------------------------------------------------------------
auto ORotationLength::DiameterText(qreal radius, const Unit &unit) -> QString
{
    return FormatDisplayNumber(FromPixel(2. * radius, unit));
}
