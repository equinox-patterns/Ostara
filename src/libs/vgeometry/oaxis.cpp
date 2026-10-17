/************************************************************************
 **
 **  @file   oaxis.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The axis a curve handle takes its angle from.
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

#include "oaxis.h"

#include <QLineF>
#include <QtMath>

#include "../ifc/ifcdef.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/** @brief The angle as QLineF hands it back, put on a 1e-5 grid.
 *
 *  QLineF::angle() ends in 6.7563e-15 style noise, and this value is written into the file and shown in the
 *  dock: it has to read as a real angle. Rounded, never truncated — the round trip through setAngle() leaves
 *  89.99999999999999 where the answer is 90, and truncating would write 89.99999 into the pattern — and 360
 *  comes back as 0, so no attribute ever carries a full turn.
 */
auto NormalizeAngle(qreal angle) -> qreal
{
    qreal result = qRound(angle * 100000.) / 100000.;
    if (result >= 360.)
    {
        result = 0.;
    }

    // Le zero negatif est le meme angle que zero, mais il s'ecrit « -0 » dans un export JSON ou une trace.
    return result == 0. ? 0. : result;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OAxis::OAxis(quint32 p1, quint32 p2, AxisSense sense)
  : m_p1(p1),
    m_p2(p2),
    m_sense(sense)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto OAxis::P1() const -> quint32
{
    return m_p1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAxis::P2() const -> quint32
{
    return m_p2;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAxis::Sense() const -> AxisSense
{
    return m_sense;
}

//---------------------------------------------------------------------------------------------------------------------
void OAxis::SetP1(quint32 id)
{
    m_p1 = id;
}

//---------------------------------------------------------------------------------------------------------------------
void OAxis::SetP2(quint32 id)
{
    m_p2 = id;
}

//---------------------------------------------------------------------------------------------------------------------
void OAxis::SetSense(AxisSense sense)
{
    m_sense = sense;
}

//---------------------------------------------------------------------------------------------------------------------
void OAxis::Set(quint32 p1, quint32 p2, AxisSense sense)
{
    m_p1 = p1;
    m_p2 = p2;
    m_sense = sense;
}

//---------------------------------------------------------------------------------------------------------------------
void OAxis::Clear()
{
    m_p1 = NULL_ID;
    m_p2 = NULL_ID;
    m_sense = AxisSense::Line;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAxis::IsValid() const -> bool
{
    return m_p1 != NULL_ID && m_p2 != NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAxis::HandleAngle(const QPointF &p1, const QPointF &p2) const -> qreal
{
    QLineF line(p1, p2);

    switch (m_sense)
    {
        case AxisSense::Line:
            break;
        case AxisSense::LineReverse:
            line.setAngle(line.angle() + 180);
            break;
        case AxisSense::Perpendicular:
            line.setAngle(line.angle() + 90);
            break;
        case AxisSense::PerpendicularReverse:
            line.setAngle(line.angle() + 270);
            break;
    }

    // setAngle does not wrap above 360, angle() always does: the round trip normalises the sum, and
    // NormalizeAngle() takes the noise of the trigonometry out of it.
    return NormalizeAngle(line.angle());
}

//---------------------------------------------------------------------------------------------------------------------
auto OAxis::OppositeAngle(qreal angle) -> qreal
{
    QLineF line(0, 0, 100, 0);
    line.setAngle(angle + 180);
    return NormalizeAngle(line.angle());
}

//---------------------------------------------------------------------------------------------------------------------
auto OAxis::SenseName() const -> QString
{
    switch (m_sense)
    {
        case AxisSense::Line:
            return AxisSenseLine;
        case AxisSense::LineReverse:
            return AxisSenseLineReverse;
        case AxisSense::Perpendicular:
            return AxisSensePerpendicular;
        case AxisSense::PerpendicularReverse:
            return AxisSensePerpendicularReverse;
    }
    return AxisSenseLine;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAxis::SenseFromName(const QString &name) -> AxisSense
{
    if (name == AxisSenseLineReverse)
    {
        return AxisSense::LineReverse;
    }
    if (name == AxisSensePerpendicular)
    {
        return AxisSense::Perpendicular;
    }
    if (name == AxisSensePerpendicularReverse)
    {
        return AxisSense::PerpendicularReverse;
    }
    return AxisSense::Line;
}
