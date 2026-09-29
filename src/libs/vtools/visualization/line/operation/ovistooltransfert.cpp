/************************************************************************
 **
 **  @file   ovistooltransfert.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   29/09/2026
 **
 **  @brief  What a transfer shows on the canvas: the portion highlighted where it comes from, and its ghost
 **          where the translation X → A puts it.
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

#include "ovistooltransfert.h"

#include <QGraphicsPathItem>
#include <QLineF>
#include <QSharedPointer>
#include <QtAlgorithms>

#include "../../../vgeometry/varc.h"
#include "../../../vgeometry/vcubicbezier.h"
#include "../../../vgeometry/vcubicbezierpath.h"
#include "../../../vgeometry/vellipticalarc.h"
#include "../../../vgeometry/vgeometrydef.h"
#include "../../../vgeometry/vgobject.h"
#include "../../../vgeometry/vpointf.h"
#include "../../../vgeometry/vspline.h"
#include "../../../vgeometry/vsplinepath.h"
#include "../../../vpatterndb/vcontainer.h"
#include "../../../vwidgets/global.h"
#include "visoperation.h"

//---------------------------------------------------------------------------------------------------------------------
OVisToolTransfert::OVisToolTransfert(const VContainer *data, QGraphicsItem *parent)
  : VisOperation(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor2);

    m_pointOrigin = InitPoint(VColorRole::VisSupportColor2, this);
    m_pointTarget = InitPoint(VColorRole::VisSupportColor2, this);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolTransfert::SetOriginPointId(quint32 value)
{
    m_originId = value;
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolTransfert::SetTargetPointId(quint32 value)
{
    m_targetId = value;
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolTransfert::PointImage(const QPointF &point) const -> QPointF
{
    return VPointF::MovePF(point, m_displayedLength, m_displayedAngle);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolTransfert::RefreshGeometry()
{
    // A transfer can be made of lines alone: the object list is empty then, but the attached lines still have a
    // source side to draw.
    if (Objects().isEmpty() && !HasPreviewLines())
    {
        return;
    }

    int iPoint = -1;
    int iCurve = -1;

    const QVector<QGraphicsItem *> originObjects = CreateOriginObjects(iPoint, iCurve);

    if (originObjects.isEmpty() || !Anchor())
    {
        // The translation is not known: only the source side is drawn, so the preview never promises a copy
        // that has nowhere to land.
        m_pointOrigin->setVisible(false);
        m_pointTarget->setVisible(false);
        RefreshPreviewLines(false);
        return;
    }

    DrawPoint(m_pointOrigin, m_displayedOrigin);
    DrawPoint(m_pointTarget, m_displayedTarget);

    CreateTransferredObjects(iPoint, iCurve);
    RefreshPreviewLines(true);
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolTransfert::Anchor() -> bool
{
    if (m_originId == NULL_ID || m_targetId == NULL_ID)
    {
        return false;
    }

    try
    {
        const QSharedPointer<VPointF> origin = GetData()->GeometricObject<VPointF>(m_originId);
        const QSharedPointer<VPointF> target = GetData()->GeometricObject<VPointF>(m_targetId);
        m_displayedOrigin = origin->toQPointF();
        m_displayedTarget = target->toQPointF();
    }
    catch (const VExceptionBadId &)
    {
        return false; // A merge point that is no longer there: nothing to translate, nothing to promise.
    }

    const QLineF line(m_displayedOrigin, m_displayedTarget);
    m_displayedLength = line.length();
    m_displayedAngle = line.angle();
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
template <class Item>
auto OVisToolTransfert::AddTransferredCurve(quint32 id, int i) -> int
{
    const QSharedPointer<Item> curve = GetData()->template GeometricObject<Item>(id);

    ++i;
    VCurvePathItem *path = GetCurve(static_cast<quint32>(i), VColorRole::VisSupportColor);
    const Item moved = curve->Move(m_displayedLength, m_displayedAngle);
    DrawPath(path, moved.GetPath(), moved.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);

    return i;
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolTransfert::CreateTransferredObjects(int &iPoint, int &iCurve)
{
    for (auto id : Objects())
    {
        // X is not copied: A is its copy, and a ghost point drawn on top of it would read as a stray object.
        if (id == m_originId)
        {
            continue;
        }

        const QSharedPointer<VGObject> obj = GetData()->GetGObject(id);

        // This check helps to find missed objects in the switch
        Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects was handled.");

        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wswitch-default")
        QT_WARNING_DISABLE_CLANG("-Wswitch-default")

        switch (obj->getType())
        {
            case GOType::Point:
            {
                const QSharedPointer<VPointF> p = GetData()->GeometricObject<VPointF>(id);

                ++iPoint;
                VScaledEllipse *point = GetPoint(static_cast<quint32>(iPoint), VColorRole::VisSupportColor);
                DrawPoint(point, PointImage(static_cast<QPointF>(*p)));
                break;
            }
            case GOType::Arc:
                iCurve = AddTransferredCurve<VArc>(id, iCurve);
                break;
            case GOType::EllipticalArc:
                iCurve = AddTransferredCurve<VEllipticalArc>(id, iCurve);
                break;
            case GOType::Spline:
                iCurve = AddTransferredCurve<VSpline>(id, iCurve);
                break;
            case GOType::SplinePath:
                iCurve = AddTransferredCurve<VSplinePath>(id, iCurve);
                break;
            case GOType::CubicBezier:
                iCurve = AddTransferredCurve<VCubicBezier>(id, iCurve);
                break;
            case GOType::CubicBezierPath:
                iCurve = AddTransferredCurve<VCubicBezierPath>(id, iCurve);
                break;
            case GOType::PlaceLabel:
            case GOType::Unknown:
                Q_UNREACHABLE();
                break;
        }

        QT_WARNING_POP
    }
}
