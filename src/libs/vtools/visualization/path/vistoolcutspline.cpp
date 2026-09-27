/************************************************************************
 **
 **  @file   vistoolcutspline.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   6 9, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
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

#include "vistoolcutspline.h"

#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPointF>
#include <QSharedPointer>
#include <new>

#include "../../tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../vgeometry/vabstractcubicbezier.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../visualization.h"
#include "../vmisc/vmodifierkey.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/scalesceneitems.h"
#include "vispath.h"

//---------------------------------------------------------------------------------------------------------------------
VisToolCutSpline::VisToolCutSpline(const VContainer *data, QGraphicsItem *parent)
  : VisPath(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor);

    m_spl1 = InitItem<VCurvePathItem>(VColorRole::VisSupportColor2, this);
    m_spl1->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    m_spl2 = InitItem<VCurvePathItem>(VColorRole::VisSupportColor4, this);
    m_spl2->setFlag(QGraphicsItem::ItemStacksBehindParent, false);

    m_point = InitPoint(VColorRole::VisMainColor, this);
    m_point->setZValue(2);
    m_point->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolCutSpline::RefreshGeometry()
{
    if (m_splineId > NULL_ID)
    {
        const auto spl = GetData()->GeometricObject<VAbstractCubicBezier>(m_splineId);
        // Hide the parent's magenta direction arrows once the split (green/red) is shown.
        DrawPath(this, spl->GetPath(), qIsInf(m_length) ? spl->DirectionArrows() : QVector<DirectionArrow>(),
                 LineStyle(), Qt::RoundCap);

        if (!qIsInf(m_length))
        {
            const VToolCutSpline::SubCurves curves = VToolCutSpline::SplitCurve(m_length, spl);

            VSpline sp1;
            VSpline sp2;
            VToolCutSpline::BuildSubCurves(curves, spl, VPointF(curves.cutPoint), sp1, sp2);

            sp1.SetApproximationScale(spl->GetApproximationScale());
            sp2.SetApproximationScale(spl->GetApproximationScale());

            DrawPoint(m_point, curves.cutPoint);

            DrawPath(m_spl1, sp1.GetPath(), sp1.DirectionArrows(), LineStyle(), Qt::RoundCap);
            DrawPath(m_spl2, sp2.GetPath(), sp2.DirectionArrows(), LineStyle(), Qt::RoundCap);
        }
        else if (GetMode() == Mode::Creation)
        {
            QPointF const p = spl->ClosestPoint(ScenePos());
            qreal const length = spl->GetLengthByPoint(p);

            DrawPoint(m_point, p);

            const QString prefix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);
            SetToolTip(tr("Length = %1%2; "
                          "<b>Mouse click</b> - finish selecting the length, "
                          "<b>%3</b> - skip")
                           .arg(LengthToUser(length), prefix, VModifierKey::EnterKey()));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolCutSpline::VisualMode(quint32 id)
{
    m_splineId = id;
    StartVisualMode();
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolCutSpline::SetLength(const QString &expression)
{
    m_length = FindLengthFromUser(expression, GetData()->DataVariables());
}
