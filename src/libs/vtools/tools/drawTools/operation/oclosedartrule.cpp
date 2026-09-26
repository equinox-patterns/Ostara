/************************************************************************
 **
 **  @file   oclosedartrule.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The rule that closes a dart, see oclosedartrule.h.
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

#include "oclosedartrule.h"

#include <QLineF>
#include <QtNumeric>

#include "../vgeometry/vgeometrydef.h"

//---------------------------------------------------------------------------------------------------------------------
auto OCloseDartRule::DefaultDivisorFormula() -> QString
{
    // The whole opening, written the way any value of a tool is held: as a formula.
    return QStringLiteral("1");
}

//---------------------------------------------------------------------------------------------------------------------
auto OCloseDartRule::LineAngle(const QPointF &from, const QPointF &to) -> qreal
{
    // The formula variable AngleLine_<A>_<B> rounds the value it caches to five decimals. The angle is read here
    // without that rounding: the point has to land on the other one, and a dart is closed with the exact angle,
    // not with one that is shown to be the same.
    return QLineF(from, to).angle();
}

//---------------------------------------------------------------------------------------------------------------------
auto OCloseDartRule::IsDefined(const QPointF &pivot, const QPointF &moving, const QPointF &target) -> bool
{
    // Three points that tell each other apart: the pivot cannot sit on a leg, and a leg point cannot land on
    // itself — a point that has nowhere to travel is not a dart, and building one would only duplicate the
    // sources at the very place they already are.
    return QLineF(pivot, moving).length() > 0. && QLineF(pivot, target).length() > 0.
           && QLineF(moving, target).length() > 0.;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCloseDartRule::IsDivisorUsable(qreal divisor) -> bool
{
    return qIsFinite(divisor) && divisor != 0.;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCloseDartRule::ClosingAngle(const QPointF &pivot, const QPointF &moving, const QPointF &target,
                                  qreal divisor) -> qreal
{
    if (!IsDefined(pivot, moving, target) || !IsDivisorUsable(divisor))
    {
        return qQNaN();
    }

    // The opening is the short way round between the two legs, and it has to be read before the divisor is
    // applied: a dart whose legs are 10° apart, seen as 350° from the other side, would be half closed the long
    // way — 175° instead of 5°.
    const qreal opening = NormalizeRotation(LineAngle(pivot, target) - LineAngle(pivot, moving));
    if (qIsNaN(opening))
    {
        return qQNaN();
    }

    // A divisor below one asks for more than the whole opening, which can leave the domain of a rotation; and a
    // rotation reads the same from either side of a half-turn.
    return NormalizeRotation(opening / divisor);
}
