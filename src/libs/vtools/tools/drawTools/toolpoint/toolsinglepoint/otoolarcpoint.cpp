/************************************************************************
 **
 **  @file   otoolarcpoint.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/08/2026
 **
 **  @brief  Point at the start (P1) or end (P2) of an arc, without
 **          cutting the arc into sub-arcs.
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

#include "otoolarcpoint.h"

#include <QSharedPointer>
#include <new>

#include "../../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "vtoolsinglepoint.h"

const QString OToolArcPoint::ToolTypeStart = QStringLiteral("arcStartPoint");
const QString OToolArcPoint::ToolTypeEnd = QStringLiteral("arcEndPoint");

//---------------------------------------------------------------------------------------------------------------------
OToolArcPoint::OToolArcPoint(const OToolArcPointInitData &initData, QGraphicsItem *parent)
  : VToolSinglePoint(initData.doc, initData.data, initData.id, initData.notes, parent),
    arcId(initData.arcId),
    position(initData.position)
{
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArcPoint::Create(OToolArcPointInitData initData) -> OToolArcPoint *
{
    const QSharedPointer<VAbstractArc> arc = initData.data->GeometricObject<VAbstractArc>(initData.arcId);
    const QPointF point = (initData.position == ArcPointPosition::Start) ? arc->GetP1() : arc->GetP2();

    const QSharedPointer<VPointF> p = QSharedPointer<VPointF>::create(point, initData.name, initData.mx, initData.my);
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
    patternGraph->AddEdge(initData.arcId, initData.id);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::ArcPoint, initData.doc);
        auto *pointTool = new OToolArcPoint(initData);
        initData.scene->addItem(pointTool);
        InitPointToolConnections(initData.scene, pointTool);
        VAbstractPattern::AddTool(initData.id, pointTool);
        return pointTool;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArcPoint::ArcName() const -> QString
{
    return VAbstractTool::data.GetGObject(arcId)->ObjectName();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArcPoint::TypeString() const -> QString
{
    return position == ArcPointPosition::Start ? ToolTypeStart : ToolTypeEnd;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArcPoint::GetPosition() const -> ArcPointPosition
{
    return position;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArcPoint::ShowVisualization(bool show)
{
    Q_UNUSED(show) // No visualization for this point.
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArcPoint::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    Q_UNUSED(event)
    Q_UNUSED(id)
    // No context menu for now — the point is edited via the arc dialog and the property browser.
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArcPoint::SaveDialog(QDomElement &domElement)
{
    doc->SetAttribute(domElement, AttrArc, QString().setNum(arcId));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArcPoint::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolSinglePoint::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, position == ArcPointPosition::Start ? ToolTypeStart : ToolTypeEnd);
    doc->SetAttribute(tag, AttrArc, arcId);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArcPoint::ReadToolAttributes(const QDomElement &domElement)
{
    VToolSinglePoint::ReadToolAttributes(domElement);

    arcId = VAbstractPattern::GetParametrUInt(domElement, AttrArc, NULL_ID_STR);
}
