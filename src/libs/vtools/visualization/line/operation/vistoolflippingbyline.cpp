/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 9, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 10/10/2026.
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

#include "vistoolflippingbyline.h"
#include "../vgeometry/vpointf.h"

//---------------------------------------------------------------------------------------------------------------------
VisToolFlippingByLine::VisToolFlippingByLine(const VContainer *data, QGraphicsItem *parent)
  : VisFlippingOperation(data, parent)
{
    m_point1 = InitPoint(VColorRole::VisSupportColor2, this, kMarkZ);
    m_point2 = InitPoint(VColorRole::VisSupportColor2, this, kMarkZ);
}

//---------------------------------------------------------------------------------------------------------------------
auto VisToolFlippingByLine::IsFixedPoint(quint32 id) const -> bool
{
    return (m_lineP1Id != NULL_ID && id == m_lineP1Id) || (m_lineP2Id != NULL_ID && id == m_lineP2Id);
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolFlippingByLine::RefreshGeometry()
{
    // An operation can be started from a line alone: the operated-object list is empty then, but the
    // attached line still has a source side to draw.
    if (Objects().isEmpty() && !HasPreviewLines())
    {
        return;
    }

    int iPoint = -1;
    int iCurve = -1;

    if (const QVector<QGraphicsItem *> originObjects = CreateOriginObjects(iPoint, iCurve);
        originObjects.isEmpty() || (GetMode() == Mode::Creation && !ObjectSelected()))
    {
        // Stage 1: the transformation is not known yet, so only the source side of the lines is drawn.
        RefreshPreviewLines(false);
        return;
    }

    QPointF firstPoint;
    QPointF secondPoint;

    if (m_lineP1Id != NULL_ID)
    {
        firstPoint = static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(m_lineP1Id));
        DrawPoint(m_point1, firstPoint);

        if (m_lineP2Id == NULL_ID)
        {
            secondPoint = ScenePos();
        }
        else
        {
            secondPoint = static_cast<QPointF>(*GetData()->GeometricObject<VPointF>(m_lineP2Id));
            DrawPoint(m_point2, secondPoint);
        }

        DrawLine(this, QLineF(firstPoint, secondPoint), Qt::DashLine);
    }

    CreateFlippedObjects(iPoint, iCurve, m_lineP1Id, firstPoint, secondPoint);
    RefreshPreviewLines(true);
}
