/************************************************************************
 **
 **  @file   ovisooldartcap.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  preview of the cap of a dart
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

#include "ovistooldartcap.h"

#include <QLineF>
#include <QPointF>
#include <QSharedPointer>

#include "../../tools/drawTools/toolpoint/toolsinglepoint/odartcaprule.h"
#include "../vgeometry/vpointf.h"
#include "../vpatterndb/vcontainer.h"
#include "../vmisc/compatibility.h"

//---------------------------------------------------------------------------------------------------------------------
OVisToolDartCap::OVisToolDartCap(const VContainer *data, QGraphicsItem *parent)
  : VisLine(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor);

    m_apex = InitPoint(VColorRole::VisSupportColor, this);
    m_firstPoint = InitPoint(VColorRole::VisSupportColor, this);
    m_secondPoint = InitPoint(VColorRole::VisSupportColor, this);
    m_extendPoint = InitPoint(VColorRole::VisSupportColor, this);

    m_axis = InitItem<VScaledLine>(VColorRole::VisSupportColor, this);
    m_extendLine = InitItem<VScaledLine>(VColorRole::VisSupportColor, this);

    m_capLine1 = InitItem<VScaledLine>(VColorRole::VisSupportColor2, this);
    m_capLine2 = InitItem<VScaledLine>(VColorRole::VisSupportColor4, this);

    m_capPoint = InitPoint(VColorRole::VisMainColor, this);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolDartCap::RefreshGeometry()
{
    const bool hasApex = m_apexId > NULL_ID;
    const bool hasFirst = m_firstPointId > NULL_ID;
    const bool hasSecond = m_secondPointId > NULL_ID;
    const bool hasExtend = m_extendPointId > NULL_ID;

    QPointF apexPos;
    QPointF firstPos;
    QPointF secondPos;
    QPointF extendPos;

    if (hasApex)
    {
        apexPos = static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(m_apexId));
        DrawPoint(m_apex, apexPos);
    }
    m_apex->setVisible(hasApex);

    if (hasFirst)
    {
        firstPos = static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(m_firstPointId));
        DrawPoint(m_firstPoint, firstPos);
    }
    m_firstPoint->setVisible(hasFirst);

    if (hasSecond)
    {
        secondPos = static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(m_secondPointId));
        DrawPoint(m_secondPoint, secondPos);
    }
    m_secondPoint->setVisible(hasSecond);

    if (hasExtend)
    {
        extendPos = static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(m_extendPointId));
        DrawPoint(m_extendPoint, extendPos);
    }
    m_extendPoint->setVisible(hasExtend);

    // The axis of the dart, as soon as the dart is known: the cap point is on it.
    const bool hasDart = hasApex && hasFirst && hasSecond;
    if (hasDart)
    {
        DrawLine(m_axis, QLineF(apexPos, ODartCapRule::Midpoint(firstPos, secondPos)), Qt::DashLine);
    }
    m_axis->setVisible(hasDart);

    // The line of the pattern to extend goes through the point the user picked and the closer of the two
    // points of the opening — the one the pattern carries.
    const bool hasLine = hasDart && hasExtend;
    if (hasLine)
    {
        const int nearest = ODartCapRule::NearestBasePoint(extendPos, firstPos, secondPos);
        DrawLine(m_extendLine, QLineF(extendPos, nearest == 0 ? firstPos : secondPos), Qt::DotLine);
    }
    m_extendLine->setVisible(hasLine);

    // The cap itself, and nothing when the construction does not exist: same rule as the tool, so the two
    // cannot show different things.
    QPointF capPos;
    const bool hasCap =
        hasLine && not ODartCapRule::IsFailure(
                        ODartCapRule::CapPoint(apexPos, firstPos, secondPos, extendPos, &capPos));

    if (hasCap)
    {
        DrawPoint(m_capPoint, capPos);
        DrawLine(m_capLine1, QLineF(capPos, firstPos));
        DrawLine(m_capLine2, QLineF(capPos, secondPos));
    }
    m_capPoint->setVisible(hasCap);
    m_capLine1->setVisible(hasCap);
    m_capLine2->setVisible(hasCap);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolDartCap::VisualMode(quint32 id)
{
    m_apexId = id;
    StartVisualMode();
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolDartCap::Clear()
{
    m_apexId = NULL_ID;
    m_firstPointId = NULL_ID;
    m_secondPointId = NULL_ID;
    m_extendPointId = NULL_ID;
    RefreshGeometry();
}
