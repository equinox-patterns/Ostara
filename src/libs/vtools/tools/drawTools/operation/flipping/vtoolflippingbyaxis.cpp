/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   16 9, 2016
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

#include "vtoolflippingbyaxis.h"

#include <QColor>
#include <QDomNode>
#include <QDomNodeList>
#include <QMapIterator>
#include <QPoint>
#include <QSharedPointer>
#include <QUndoStack>
#include <climits>
#include <qiterator.h>

#include "../../../../dialogs/tools/dialogflippingbyaxis.h"
#include "../../../../dialogs/tools/dialogtool.h"
#include "../../../../visualization/line/operation/vistoolflippingbyaxis.h"
#include "../../../../visualization/visualization.h"
#include "../../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vpointf.h"
#include "vexception.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vwidgets/vabstractsimple.h"
#include "../vwidgets/vmaingraphicsscene.h"

template <class T> class QSharedPointer;

const QString VToolFlippingByAxis::ToolType = QStringLiteral("flippingByAxis");

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByAxis::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogFlippingByAxis> dialogTool = qobject_cast<DialogFlippingByAxis *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetOriginPointId(m_originPointId);
    dialogTool->SetAxisType(m_axisType);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);

    // Only the lines explicitly selected by the user come back into the dialog: the implicit ones
    // follow their points.
    QVector<quint32> lineSources;
    for (const auto &line : Lines())
    {
        if (OOperationLines::IsExplicit(line))
        {
            lineSources.append(line.source);
        }
    }
    dialogTool->SetLineSources(lineSources);
    dialogTool->SetDestinationObjects(m_destination);

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByAxis::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                                 VContainer *data) -> VToolFlippingByAxis *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogFlippingByAxis> dialogTool = qobject_cast<DialogFlippingByAxis *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolFlippingByAxisInitData initData;
    initData.originPointId = dialogTool->GetOriginPointId();
    initData.axisType = dialogTool->GetAxisType();
    initData.source = dialogTool->GetSourceObjects();
    initData.lines = OOperationLines::Collect(data, initData.source, dialogTool->GetLineSources());
    initData.hasLinkedVisibilityGroup = dialogTool->HasLinkedVisibilityGroup();
    initData.visibilityGroupName = dialogTool->GetVisibilityGroupName();
    initData.visibilityGroupTags = dialogTool->GetVisibilityGroupTags();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();

    VToolFlippingByAxis *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByAxis::Create(VToolFlippingByAxisInitData initData) -> VToolFlippingByAxis *
{
    const auto originPoint = *initData.data->GeometricObject<VPointF>(initData.originPointId);
    const auto fPoint = static_cast<QPointF>(originPoint);

    QPointF const sPoint = initData.axisType == AxisType::VerticalAxis ? QPointF(fPoint.x(), fPoint.y() + 100)
                                                                       : QPointF(fPoint.x() + 100, fPoint.y());

    if (initData.typeCreation == Source::FromGui)
    {
        initData.destination.clear(); // Try to avoid mistake, value must be empty

        initData.id = initData.data->getNextId(); // Just reserve id for tool
    }
    else
    {
        PrepareNames(initData);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    CreateDestination(initData, fPoint, sPoint);

    patternGraph->AddEdge(initData.originPointId, initData.id);

    for (const auto &object : std::as_const(initData.source))
    {
        patternGraph->AddEdge(object.id, initData.id);
    }

    for (const auto &object : std::as_const(initData.destination))
    {
        patternGraph->AddVertex(object.id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, object.id);
    }

    // Rebuilt once the destination objects and their graph nodes exist. The point the axis is built from
    // lies on the axis, so it is its own image.
    CreateDestinationLines(initData, {initData.originPointId});

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        return AddToolToScene<VToolFlippingByAxis>(
            initData, Tool::FlippingByAxis, tr("Flipping by axis"),
            [&initData]() { return new VToolFlippingByAxis(initData); });
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByAxis::GetAxisType() const -> AxisType
{
    return m_axisType;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByAxis::SetAxisType(AxisType value)
{
    m_axisType = value;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByAxis::OriginPointName() const -> QString
{
    return VAbstractTool::data.GetGObject(m_originPointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByAxis::ShowVisualization(bool show)
{
    ShowToolVisualization<VisToolFlippingByAxis>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByAxis::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    ShowContextMenuOf<DialogFlippingByAxis>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByAxis::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<VisToolFlippingByAxis *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetObjects(SourceToObjects(m_source));

        QVector<QPair<quint32, quint32>> previewLines;
        previewLines.reserve(m_lines.size());
        for (const auto &line : std::as_const(m_lines))
        {
            previewLines.append(qMakePair(line.point1, line.point2));
        }
        visual->SetPreviewLines(previewLines);
        visual->SetOriginPointId(m_originPointId);
        visual->SetAxisType(m_axisType);
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByAxis::SaveDialog(QDomElement &domElement)
{
    const QPointer<DialogFlippingByAxis> dialogTool = DialogOf<DialogFlippingByAxis>();

    doc->SetAttribute(domElement, AttrCenter, QString().setNum(dialogTool->GetOriginPointId()));
    doc->SetAttribute(domElement, AttrAxisType, QString().setNum(static_cast<int>(dialogTool->GetAxisType())));
    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();
    const QVector<DestinationItem> destination = SyncDestination(source);
    // Persist the dialog line selection: a line added here must be created, a line dropped must go, and
    // the transformed copies that already exist keep their id.
    m_lines = OOperationLines::SyncLines(getData(), m_lines, source, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);

    // Save visibility data for later use
    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByAxis::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractFlipping::ReadToolAttributes(domElement);

    m_originPointId = VAbstractPattern::GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
    m_axisType = static_cast<AxisType>(VAbstractPattern::GetParametrUInt(domElement, AttrAxisType, QChar('1')));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByAxis::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractFlipping::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrCenter, QString().setNum(m_originPointId));
    doc->SetAttribute(tag, AttrAxisType, QString().setNum(static_cast<int>(m_axisType)));
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByAxis::MakeToolTip() const -> QString
{
    return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>"
                          "%3")
        .arg(tr("Origin point"),        // 1
             OriginPointName(),         // 2
             VisibilityGroupToolTip()); // 3
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByAxis::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<DialogFlippingByAxis>(oldDomElement, newDomElement);
}

//---------------------------------------------------------------------------------------------------------------------
VToolFlippingByAxis::VToolFlippingByAxis(const VToolFlippingByAxisInitData &initData, QGraphicsItem *parent)
  : VAbstractFlipping(initData, parent),
    m_originPointId(initData.originPointId),
    m_axisType(initData.axisType)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByAxis::SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>
{
    bool unchanged = false;
    const QMap<QUuid, DestinationItem> existingByRecord = SyncDestinationPreamble(source, unchanged);
    if (unchanged)
    {
        return m_destination;
    }

    const auto originPoint = *getData()->GeometricObject<VPointF>(m_originPointId);
    const auto fPoint = static_cast<QPointF>(originPoint);

    QPointF const sPoint = m_axisType == AxisType::VerticalAxis ? QPointF(fPoint.x(), fPoint.y() + 100)
                                                                : QPointF(fPoint.x() + 100, fPoint.y());

    QVector<DestinationItem> newDestination;
    newDestination.reserve(source.size());

    for (const auto &srcItem : std::as_const(source))
    {
        if (existingByRecord.contains(srcItem.recordId))
        {
            // Preserve existing destination item
            newDestination.append(existingByRecord.value(srcItem.recordId));
        }
        else
        {
            // Create destination item for newly added source
            newDestination.append(MakeDestination(fPoint, sPoint).CreateObject(m_id, srcItem, getData()));
        }
    }

    return newDestination;
}
