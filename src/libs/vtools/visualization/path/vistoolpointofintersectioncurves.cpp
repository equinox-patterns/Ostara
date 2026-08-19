/************************************************************************
 **
 **  @file   vistoolpointofintersectioncurves.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   24 1, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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

#include "vistoolpointofintersectioncurves.h"

#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QPointF>
#include <QScopedPointer>
#include <QSharedPointer>
#include <new>

#include "../../tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../../tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncurves.h"
#include "../../visualization/path/vispath.h"
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
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/scalesceneitems.h"

//---------------------------------------------------------------------------------------------------------------------
VisToolPointOfIntersectionCurves::VisToolPointOfIntersectionCurves(const VContainer *data, QGraphicsItem *parent)
  : VisPath(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor);

    m_visCurve2 = InitItem<VCurvePathItem>(VColorRole::VisSupportColor, this);
    m_visCurve2->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    m_curve1Seg1 = InitItem<VCurvePathItem>(VColorRole::VisSupportColor2, this);
    m_curve1Seg1->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    m_curve1Seg2 = InitItem<VCurvePathItem>(VColorRole::VisSupportColor4, this);
    m_curve1Seg2->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    m_curve2Seg1 = InitItem<VCurvePathItem>(VColorRole::VisSupportColor, this);
    m_curve2Seg1->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    m_curve2Seg2 = InitItem<VCurvePathItem>(VColorRole::VisSupportColor3, this);
    m_curve2Seg2->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    m_point = InitPoint(VColorRole::VisMainColor, this);
    m_point->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
    m_point->setZValue(1);
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolPointOfIntersectionCurves::RefreshGeometry()
{
    if (m_curve1Id > NULL_ID)
    {
        auto curve1 = GetData()->GeometricObject<VAbstractCurve>(m_curve1Id);
        DrawPath(this, curve1->GetPath(), m_curve2Id > NULL_ID ? QVector<DirectionArrow>() : curve1->DirectionArrows(),
                 Qt::SolidLine, Qt::RoundCap);

        if (m_curve2Id > NULL_ID)
        {
            auto curve2 = GetData()->GeometricObject<VAbstractCurve>(m_curve2Id);
            DrawPath(m_visCurve2, curve2->GetPath(), QVector<DirectionArrow>(), Qt::SolidLine, Qt::RoundCap);

            QPointF p;
            const bool success = VToolPointOfIntersectionCurves::FindPoint(curve1->GetPoints(), curve2->GetPoints(),
                                                                            m_vCrossPoint, m_hCrossPoint, &p);
            DrawPoint(m_point, p);

            if (success)
            {
                DrawSegments(m_curve1Id, p, m_curve1Seg1, m_curve1Seg2);
                DrawSegments(m_curve2Id, p, m_curve2Seg1, m_curve2Seg2);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolPointOfIntersectionCurves::DrawSegments(quint32 curveId, const QPointF &p, VCurvePathItem *seg1,
                                                    VCurvePathItem *seg2)
{
    const QSharedPointer<VAbstractCurve> curve = GetData()->GeometricObject<VAbstractCurve>(curveId);
    const qreal segLength = curve->GetLengthByPoint(p);

    switch (curve->getType())
    {
        case GOType::Spline:
        case GOType::CubicBezier:
        {
            const QSharedPointer<VAbstractCubicBezier> spl = GetData()->GeometricObject<VAbstractCubicBezier>(curveId);
            QPointF spl1p2;
            QPointF spl1p3;
            QPointF spl2p2;
            QPointF spl2p3;
            spl->CutSpline(segLength, spl1p2, spl1p3, spl2p2, spl2p3, QString());

            VSpline sp1(spl->GetP1(), spl1p2, spl1p3, VPointF(p));
            VSpline sp2(VPointF(p), spl2p2, spl2p3, spl->GetP4());
            sp1.SetApproximationScale(spl->GetApproximationScale());
            sp2.SetApproximationScale(spl->GetApproximationScale());

            DrawPath(seg1, sp1.GetPath(), sp1.DirectionArrows(), LineStyle(), Qt::RoundCap);
            DrawPath(seg2, sp2.GetPath(), sp2.DirectionArrows(), LineStyle(), Qt::RoundCap);
            break;
        }
        case GOType::SplinePath:
        case GOType::CubicBezierPath:
        {
            const QSharedPointer<VAbstractCubicBezierPath> splPath =
                GetData()->GeometricObject<VAbstractCubicBezierPath>(curveId);
            VSplinePath *spPath1 = nullptr;
            VSplinePath *spPath2 = nullptr;
            QScopedPointer<VPointF> const cutPoint(
                VToolCutSplinePath::CutSplinePath(segLength, splPath, QChar('X'), &spPath1, &spPath2));
            SCASSERT(spPath1 != nullptr)
            SCASSERT(spPath2 != nullptr)

            DrawPath(seg1, spPath1->GetPath(), spPath1->DirectionArrows(), LineStyle(), Qt::RoundCap);
            DrawPath(seg2, spPath2->GetPath(), spPath2->DirectionArrows(), LineStyle(), Qt::RoundCap);

            delete spPath1;
            delete spPath2;
            break;
        }
        case GOType::Arc:
        {
            const QSharedPointer<VAbstractArc> arc = GetData()->GeometricObject<VAbstractArc>(curveId);
            VArc ar1;
            VArc ar2;
            arc->CutArc(segLength, &ar1, &ar2, QString());
            DrawPath(seg1, ar1.GetPath(), ar1.DirectionArrows(), LineStyle(), Qt::RoundCap);
            DrawPath(seg2, ar2.GetPath(), ar2.DirectionArrows(), LineStyle(), Qt::RoundCap);
            break;
        }
        case GOType::EllipticalArc:
        {
            const QSharedPointer<VAbstractArc> arc = GetData()->GeometricObject<VAbstractArc>(curveId);
            VEllipticalArc ar1;
            VEllipticalArc ar2;
            arc->CutArc(segLength, &ar1, &ar2, QString());
            DrawPath(seg1, ar1.GetPath(), ar1.DirectionArrows(), LineStyle(), Qt::RoundCap);
            DrawPath(seg2, ar2.GetPath(), ar2.DirectionArrows(), LineStyle(), Qt::RoundCap);
            break;
        }
        case GOType::Point:
        case GOType::PlaceLabel:
        case GOType::Unknown:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolPointOfIntersectionCurves::VisualMode(quint32 id)
{
    m_curve1Id = id;
    StartVisualMode();
}
