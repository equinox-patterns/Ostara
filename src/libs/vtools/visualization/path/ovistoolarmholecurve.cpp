/************************************************************************
 **
 **  @file   ovistoolarmholecurve.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/08/2026
 **
 **  @brief  Visualization for the ArmholeCurve tool.
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

#include "ovistoolarmholecurve.h"

#include <QGraphicsLineItem>
#include <QLineF>
#include <QPainterPath>
#include <QPointF>
#include <QSharedPointer>
#include <new>

#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vpointf.h"
#include "../../tools/drawTools/toolcurve/otoolarmholecurve.h"
#include "../visualization.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/scalesceneitems.h"
#include "vispath.h"

//---------------------------------------------------------------------------------------------------------------------
OVisToolArmholeCurve::OVisToolArmholeCurve(const VContainer *data, QGraphicsItem *parent)
  : VisPath(data, parent)
{
    m_helpLine1 = InitItem<VScaledLine>(VColorRole::VisMainColor, this);
    m_helpLine2 = InitItem<VScaledLine>(VColorRole::VisMainColor, this);

    m_point1 = InitPoint(VColorRole::VisSupportColor, this); // armhole
    m_point2 = InitPoint(VColorRole::VisSupportColor, this); // chest/back width
    m_point3 = InitPoint(VColorRole::VisSupportColor, this); // perpendicular
    m_point4 = InitPoint(VColorRole::VisSupportColor, this); // shoulder
}

//---------------------------------------------------------------------------------------------------------------------
OVisToolArmholeCurve::~OVisToolArmholeCurve() = default;

//---------------------------------------------------------------------------------------------------------------------
void OVisToolArmholeCurve::RefreshGeometry()
{
    if (GetMode() == Mode::Show)
    {
        // Show mode: no construction lines, no support points — only the final curve.
        m_helpLine1->setVisible(false);
        m_helpLine2->setVisible(false);
        m_point1->setVisible(false);
        m_point2->setVisible(false);
        m_point3->setVisible(false);
        m_point4->setVisible(false);

        if (m_point1Id <= NULL_ID || m_point2Id <= NULL_ID || m_point3Id <= NULL_ID || m_point4Id <= NULL_ID)
        {
            return;
        }

        const auto em = GetData()->GeometricObject<VPointF>(m_point1Id);
        const auto carrure = GetData()->GeometricObject<VPointF>(m_point2Id);
        const auto perp = GetData()->GeometricObject<VPointF>(m_point3Id);
        const auto epaule = GetData()->GeometricObject<VPointF>(m_point4Id);
        const ArmholeHandles handles = OToolArmholeCurve::SolveHandles(*em, *carrure, 0.30);

        QVector<VPointF> pts;
        pts.append(*em);
        pts.append(handles.h1a);
        pts.append(handles.h1b);
        pts.append(*carrure);
        pts.append(handles.h2a);
        pts.append(*perp);
        pts.append(*epaule);
        VCubicBezierPath path(pts);
        path.SetApproximationScale(ApproximationScale());
        DrawPath(this, path.GetPath(), path.DirectionArrows(), LineStyle(), Qt::RoundCap);
        return;
    }

    // --- Creation mode: progressive preview ---
    if (m_point1Id <= NULL_ID)
    {
        return;
    }

    const auto em = GetData()->GeometricObject<VPointF>(m_point1Id);
    DrawPoint(m_point1, static_cast<QPointF>(*em), Qt::DashLine);

    if (m_point2Id <= NULL_ID)
    {
        // Only armhole selected: dashed line to cursor
        DrawLine(m_helpLine1, QLineF(static_cast<QPointF>(*em), ScenePos()), Qt::DashLine);
        m_helpLine2->setVisible(false);
        return;
    }

    const auto carrure = GetData()->GeometricObject<VPointF>(m_point2Id);
    DrawPoint(m_point2, static_cast<QPointF>(*carrure));
    DrawLine(m_helpLine1, QLineF(static_cast<QPointF>(*em), static_cast<QPointF>(*carrure)), Qt::DashLine);

    const ArmholeHandles handles = OToolArmholeCurve::SolveHandles(*em, *carrure, 0.30);

    if (m_point3Id <= NULL_ID)
    {
        // Armhole + chest/back width: preview segment 1, dashed line chest/back width → cursor
        DrawLine(m_helpLine2, QLineF(static_cast<QPointF>(*carrure), ScenePos()), Qt::DashLine);

        QVector<VPointF> pts;
        pts.append(*em);
        pts.append(handles.h1a);
        pts.append(handles.h1b);
        pts.append(*carrure);
        VCubicBezierPath path(pts);
        path.SetApproximationScale(ApproximationScale());
        DrawPath(this, path.GetPath(), path.DirectionArrows(), LineStyle(), Qt::RoundCap);
        return;
    }

    const auto perp = GetData()->GeometricObject<VPointF>(m_point3Id);
    DrawPoint(m_point3, static_cast<QPointF>(*perp));
    DrawLine(m_helpLine2, QLineF(static_cast<QPointF>(*carrure), static_cast<QPointF>(*perp)), Qt::DashLine);

    if (m_point4Id <= NULL_ID)
    {
        // Armhole + chest/back width + perpendicular: preview with cursor as shoulder
        const VPointF cursorPoint(ScenePos());
        QVector<VPointF> pts;
        pts.append(*em);
        pts.append(handles.h1a);
        pts.append(handles.h1b);
        pts.append(*carrure);
        pts.append(handles.h2a);
        pts.append(*perp);
        pts.append(cursorPoint);
        VCubicBezierPath path(pts);
        path.SetApproximationScale(ApproximationScale());
        DrawPath(this, path.GetPath(), path.DirectionArrows(), LineStyle(), Qt::RoundCap);
        return;
    }

    const auto epaule = GetData()->GeometricObject<VPointF>(m_point4Id);
    DrawPoint(m_point4, static_cast<QPointF>(*epaule));

    // All 4 points: final curve
    QVector<VPointF> pts;
    pts.append(*em);
    pts.append(handles.h1a);
    pts.append(handles.h1b);
    pts.append(*carrure);
    pts.append(handles.h2a);
    pts.append(*perp);
    pts.append(*epaule);
    VCubicBezierPath path(pts);
    path.SetApproximationScale(ApproximationScale());
    DrawPath(this, path.GetPath(), path.DirectionArrows(), LineStyle(), Qt::RoundCap);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolArmholeCurve::VisualMode(quint32 id)
{
    m_point1Id = id;
    StartVisualMode();
}
