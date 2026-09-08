/************************************************************************
 **
 **  @file   otoolcurvepoint.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/08/2026
 **
 **  @brief  Point at the start (FirstPoint) or end (LastPoint) of an
 **          offset curve (parallel or graduated), without cutting the
 **          curve into sub-curves.
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

#include "otoolcurvepoint.h"

#include <QSharedPointer>
#include <new>

#include "../../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vabstractcubicbezierpath.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "vtoolsinglepoint.h"

const QString OToolCurvePoint::ToolTypeStart = QStringLiteral("curveStartPoint");
const QString OToolCurvePoint::ToolTypeEnd = QStringLiteral("curveEndPoint");

//---------------------------------------------------------------------------------------------------------------------
OToolCurvePoint::OToolCurvePoint(const OToolCurvePointInitData &initData, QGraphicsItem *parent)
  : VToolSinglePoint(initData.doc, initData.data, initData.id, initData.notes, parent),
    splineId(initData.splineId),
    position(initData.position)
{
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCurvePoint::Create(OToolCurvePointInitData initData) -> OToolCurvePoint *
{
    const QSharedPointer<VAbstractCubicBezierPath> curve =
        initData.data->GeometricObject<VAbstractCubicBezierPath>(initData.splineId);
    const VPointF point = (initData.position == CurvePointPosition::Start) ? curve->FirstPoint() : curve->LastPoint();

    auto *p = new VPointF(point.toQPointF(), initData.name, initData.mx, initData.my);
    p->SetShowLabel(initData.showLabel);

    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->AddGObject(p);
    }
    else
    {
        initData.data->UpdateGObject(initData.id, p);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());
    patternGraph->AddEdge(initData.splineId, initData.id);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::CurvePoint, initData.doc);
        auto *pointTool = new OToolCurvePoint(initData);
        initData.scene->addItem(pointTool);
        InitPointToolConnections(initData.scene, pointTool);
        VAbstractPattern::AddTool(initData.id, pointTool);
        return pointTool;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCurvePoint::SplineName() const -> QString
{
    return VAbstractTool::data.GetGObject(splineId)->ObjectName();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCurvePoint::TypeString() const -> QString
{
    return position == CurvePointPosition::Start ? ToolTypeStart : ToolTypeEnd;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCurvePoint::GetPosition() const -> CurvePointPosition
{
    return position;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurvePoint::ShowVisualization(bool show)
{
    Q_UNUSED(show) // No visualization for this point.
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurvePoint::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    Q_UNUSED(event)
    Q_UNUSED(id)
    // No context menu for now — the point is edited via the curve dialog and the property browser.
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurvePoint::SaveDialog(QDomElement &domElement)
{
    doc->SetAttribute(domElement, AttrSpline, QString().setNum(splineId));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurvePoint::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolSinglePoint::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, position == CurvePointPosition::Start ? ToolTypeStart : ToolTypeEnd);
    doc->SetAttribute(tag, AttrSpline, splineId);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurvePoint::ReadToolAttributes(const QDomElement &domElement)
{
    VToolSinglePoint::ReadToolAttributes(domElement);

    splineId = VAbstractPattern::GetParametrUInt(domElement, AttrSpline, NULL_ID_STR);
}
