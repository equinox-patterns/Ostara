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

    m_capLine1 = InitMark<VScaledLine>(VColorRole::VisSupportColor2, this);
    m_capLine2 = InitMark<VScaledLine>(VColorRole::VisSupportColor4, this);

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

    // The axis of the dart, as soon as the dart is known: it runs from the apex of the dart to the point C — the
    // point the tool made, when it exists — and to the middle of the opening while the cap is only being placed,
    // since there is no point C yet. The point C stands on that line either way.
    const bool hasDart = hasApex && hasFirst && hasSecond;
    if (hasDart)
    {
        const bool made = m_capPointId > NULL_ID && GetData() != nullptr && GetData()->HasObject(m_capPointId);
        const QPointF end = made ? static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(m_capPointId))
                                 : ODartCapRule::Midpoint(firstPos, secondPos);
        DrawLine(m_axis, QLineF(apexPos, end), Qt::DashLine);
    }
    m_axis->setVisible(hasDart);

    // The line of the pattern the cap is read on: the edge the piece carries at the end of the opening the
    // sense designates, which the caller read for us — or, when the user had to designate a point, the line
    // through it and the closer of the two points of the opening, exactly as before.
    QPointF capPos;
    bool hasCap = false;

    if (hasDart && hasExtend)
    {
        const int nearest = ODartCapRule::NearestBasePoint(extendPos, firstPos, secondPos);
        DrawLine(m_extendLine, QLineF(extendPos, nearest == 0 ? firstPos : secondPos), Qt::DotLine);
        m_extendLine->setVisible(true);

        hasCap = not ODartCapRule::IsFailure(
            ODartCapRule::CapPoint(apexPos, firstPos, secondPos, extendPos, &capPos));
    }
    else
    {
        const int baseIndex = m_secondSense ? 1 : 0;
        const QPointF basePos = m_secondSense ? secondPos : firstPos;
        const QPointF carrier = ODartCapRule::CarrierPoint(
            basePos, m_edgeDirection, QLineF(firstPos, secondPos).length());

        if (hasDart && not carrier.isNull())
        {
            DrawLine(m_extendLine, QLineF(carrier, basePos), Qt::DotLine);
            m_extendLine->setVisible(true);
        }
        else
        {
            m_extendLine->setVisible(false);
        }

        // The cap itself, and nothing when the construction does not exist: same rule as the tool, so the two
        // cannot show different things.
        hasCap = hasDart
                 && not ODartCapRule::IsFailure(ODartCapRule::CapPointFromEdge(
                     apexPos, firstPos, secondPos, baseIndex, m_edgeDirection, &capPos));
    }

    // The cap is NOT drawn here — neither its point nor its two sides. It is what the pattern holds once the tool
    // is applied, and the canvas draws it where it is built: with the colours and the pen styles the user chose, and
    // following the curve of the edge. Drawn a second time here it would be the straight one, because a preview
    // reads no curve, and the user would see two different caps. The pleat's own preview shows no cap either, for
    // the very same reason (see OVisToolPleat).
    m_capPoint->setVisible(false);
    m_capLine1->setVisible(false);
    m_capLine2->setVisible(false);
    Q_UNUSED(capPos)
    Q_UNUSED(hasCap)
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
