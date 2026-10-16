/************************************************************************
 **
 **  @file   ocurveaxis.cpp
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

#include "ocurveaxis.h"

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
OCurveAxis::OCurveAxis(quint32 p1, quint32 p2, CurveAxisSense sense)
  : m_p1(p1),
    m_p2(p2),
    m_sense(sense)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxis::P1() const -> quint32
{
    return m_p1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxis::P2() const -> quint32
{
    return m_p2;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxis::Sense() const -> CurveAxisSense
{
    return m_sense;
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxis::SetP1(quint32 id)
{
    m_p1 = id;
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxis::SetP2(quint32 id)
{
    m_p2 = id;
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxis::SetSense(CurveAxisSense sense)
{
    m_sense = sense;
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxis::Set(quint32 p1, quint32 p2, CurveAxisSense sense)
{
    m_p1 = p1;
    m_p2 = p2;
    m_sense = sense;
}

//---------------------------------------------------------------------------------------------------------------------
void OCurveAxis::Clear()
{
    m_p1 = NULL_ID;
    m_p2 = NULL_ID;
    m_sense = CurveAxisSense::Line;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxis::IsValid() const -> bool
{
    return m_p1 != NULL_ID && m_p2 != NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxis::HandleAngle(const QPointF &p1, const QPointF &p2) const -> qreal
{
    QLineF line(p1, p2);

    switch (m_sense)
    {
        case CurveAxisSense::Line:
            break;
        case CurveAxisSense::LineReverse:
            line.setAngle(line.angle() + 180);
            break;
        case CurveAxisSense::Perpendicular:
            line.setAngle(line.angle() + 90);
            break;
        case CurveAxisSense::PerpendicularReverse:
            line.setAngle(line.angle() + 270);
            break;
    }

    // setAngle does not wrap above 360, angle() always does: the round trip normalises the sum, and
    // NormalizeAngle() takes the noise of the trigonometry out of it.
    return NormalizeAngle(line.angle());
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxis::OppositeAngle(qreal angle) -> qreal
{
    QLineF line(0, 0, 100, 0);
    line.setAngle(angle + 180);
    return NormalizeAngle(line.angle());
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxis::SenseName() const -> QString
{
    switch (m_sense)
    {
        case CurveAxisSense::Line:
            return CurveAxisSenseLine;
        case CurveAxisSense::LineReverse:
            return CurveAxisSenseLineReverse;
        case CurveAxisSense::Perpendicular:
            return CurveAxisSensePerpendicular;
        case CurveAxisSense::PerpendicularReverse:
            return CurveAxisSensePerpendicularReverse;
    }
    return CurveAxisSenseLine;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCurveAxis::SenseFromName(const QString &name) -> CurveAxisSense
{
    if (name == CurveAxisSenseLineReverse)
    {
        return CurveAxisSense::LineReverse;
    }
    if (name == CurveAxisSensePerpendicular)
    {
        return CurveAxisSense::Perpendicular;
    }
    if (name == CurveAxisSensePerpendicularReverse)
    {
        return CurveAxisSense::PerpendicularReverse;
    }
    return CurveAxisSense::Line;
}
