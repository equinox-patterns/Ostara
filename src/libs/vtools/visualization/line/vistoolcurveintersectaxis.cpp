/************************************************************************
 **
 **  @file   vistoolcurveintersectaxis.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   21 10, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 28/08/2026.
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

// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#include "vistoolcurveintersectaxis.h"

#include <QColor>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QLineF>
#include <QPainterPath>
#include <QPointF>
#include <QScopedPointer>
#include <QSharedPointer>
#include <new>

#include "../../tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../../tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolcurveintersectaxis.h"
#include "../vgeometry/vabstractarc.h"
#include "../vgeometry/vabstractcubicbezier.h"
#include "../vgeometry/vabstractcubicbezierpath.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../visualization.h"
#include "../vmisc/vmodifierkey.h"
#include "../vpatterndb/vcontainer.h"
#include "visline.h"

//---------------------------------------------------------------------------------------------------------------------
VisToolCurveIntersectAxis::VisToolCurveIntersectAxis(const VContainer *data, QGraphicsItem *parent)
  : VisLine(data, parent)
{
    m_visCurve = InitItem<VCurvePathItem>(VColorRole::VisSupportColor, this);
    m_curve1 = InitItem<VCurvePathItem>(VColorRole::VisSupportColor2, this);
    m_curve1->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    m_curve2 = InitItem<VCurvePathItem>(VColorRole::VisSupportColor4, this);
    m_curve2->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    m_basePoint = InitPoint(VColorRole::VisSupportColor, this);
    m_baseLine = InitItem<VScaledLine>(VColorRole::VisSupportColor, this);
    m_axisLine = InitItem<VScaledLine>(VColorRole::VisSupportColor, this); //-V656
    m_point = InitPoint(VColorRole::VisMainColor, this);
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolCurveIntersectAxis::RefreshGeometry()
{
    if (m_curveId > NULL_ID)
    {
        const QSharedPointer<VAbstractCurve> curve = GetData()->GeometricObject<VAbstractCurve>(m_curveId);
        // Hide the full curve's direction arrows once the cut (green/red segments) is shown.
        DrawPath(m_visCurve, curve->GetPath(),
                 m_axisPointId > NULL_ID ? QVector<DirectionArrow>() : curve->DirectionArrows(),
                 Qt::SolidLine, Qt::RoundCap);

        if (m_axisPointId > NULL_ID)
        {
            QLineF axis;
            const QSharedPointer<VPointF> first = GetData()->GeometricObject<VPointF>(m_axisPointId);
            if (VFuzzyComparePossibleNulls(m_angle, -1))
            {
                axis = Axis(static_cast<QPointF>(*first), ScenePos());
            }
            else
            {
                axis = Axis(static_cast<QPointF>(*first), m_angle);
            }
            DrawPoint(m_basePoint, static_cast<QPointF>(*first));
            DrawLine(m_axisLine, axis, Qt::DashLine);

            QPointF p;
            const bool success =
                VToolCurveIntersectAxis::FindPoint(static_cast<QPointF>(*first), axis.angle(), curve->GetPoints(), &p);
            QLineF const axis_line(static_cast<QPointF>(*first), p);
            DrawLine(this, axis_line, LineStyle());

            DrawPoint(m_point, p);

            if (success)
            {
                DrawSegments(curve.data(), p);
            }

            SetToolTip(tr("<b>Intersection curve and axis</b>: angle = %1°; <b>%2</b> - "
                          "sticking angle, <b>%3</b> - finish creation")
                           .arg(AngleToUser(this->line().angle()), VModifierKey::Shift(), VModifierKey::EnterKey()));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolCurveIntersectAxis::DrawSegments(const VAbstractCurve *curve, const QPointF &p)
{
    const qreal segLength = curve->GetLengthByPoint(p);

    // The two cut segments keep their own colors (green/red) and stay solid:
    // the line style belongs to the created axis only, not to the segments.
    switch (curve->getType())
    {
        case GOType::Spline:
        case GOType::CubicBezier:
        {
            const QSharedPointer<VAbstractCubicBezier> spl = GetData()->GeometricObject<VAbstractCubicBezier>(m_curveId);
            QPointF spl1p2;
            QPointF spl1p3;
            QPointF spl2p2;
            QPointF spl2p3;
            spl->CutSpline(segLength, spl1p2, spl1p3, spl2p2, spl2p3, QString());

            VSpline sp1(spl->GetP1(), spl1p2, spl1p3, VPointF(p));
            VSpline sp2(VPointF(p), spl2p2, spl2p3, spl->GetP4());
            sp1.SetApproximationScale(spl->GetApproximationScale());
            sp2.SetApproximationScale(spl->GetApproximationScale());

            DrawPath(m_curve1, sp1.GetPath(), sp1.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);
            DrawPath(m_curve2, sp2.GetPath(), sp2.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);
            break;
        }
        case GOType::SplinePath:
        case GOType::CubicBezierPath:
        {
            const QSharedPointer<VAbstractCubicBezierPath> splPath =
                GetData()->GeometricObject<VAbstractCubicBezierPath>(m_curveId);
            VSplinePath *spPath1 = nullptr;
            VSplinePath *spPath2 = nullptr;
            QScopedPointer<VPointF> const cutPoint(
                VToolCutSplinePath::CutSplinePath(segLength, splPath, QChar('X'), &spPath1, &spPath2));
            SCASSERT(spPath1 != nullptr)
            SCASSERT(spPath2 != nullptr)

            DrawPath(m_curve1, spPath1->GetPath(), spPath1->DirectionArrows(), Qt::SolidLine, Qt::RoundCap);
            DrawPath(m_curve2, spPath2->GetPath(), spPath2->DirectionArrows(), Qt::SolidLine, Qt::RoundCap);

            delete spPath1;
            delete spPath2;
            break;
        }
        case GOType::Arc:
        {
            const QSharedPointer<VAbstractArc> arc = GetData()->GeometricObject<VAbstractArc>(m_curveId);
            VArc ar1;
            VArc ar2;
            arc->CutArc(segLength, &ar1, &ar2, QString());
            DrawPath(m_curve1, ar1.GetPath(), ar1.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);
            DrawPath(m_curve2, ar2.GetPath(), ar2.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);
            break;
        }
        case GOType::EllipticalArc:
        {
            const QSharedPointer<VAbstractArc> arc = GetData()->GeometricObject<VAbstractArc>(m_curveId);
            VEllipticalArc ar1;
            VEllipticalArc ar2;
            arc->CutArc(segLength, &ar1, &ar2, QString());
            DrawPath(m_curve1, ar1.GetPath(), ar1.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);
            DrawPath(m_curve2, ar2.GetPath(), ar2.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);
            break;
        }
        case GOType::Point:
        case GOType::PlaceLabel:
        case GOType::Unknown:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolCurveIntersectAxis::VisualMode(quint32 id)
{
    m_curveId = id;
    StartVisualMode();
}

//---------------------------------------------------------------------------------------------------------------------
auto VisToolCurveIntersectAxis::Angle() const -> QString
{
    return QString::number(this->line().angle());
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolCurveIntersectAxis::SetAngle(const QString &expression)
{
    m_angle = FindValFromUser(expression, GetData()->DataVariables());
}
