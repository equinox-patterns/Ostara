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

#include "vtoolflippingbyline.h"

#include <QColor>
#include <QDomNode>
#include <QDomNodeList>
#include <QMapIterator>
#include <QPoint>
#include <QSharedPointer>
#include <QUndoStack>
#include <climits>
#include <qiterator.h>

#include "../../../../dialogs/tools/dialogflippingbyline.h"
#include "../../../../dialogs/tools/dialogtool.h"
#include "../../../../visualization/line/operation/vistoolflippingbyline.h"
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

const QString VToolFlippingByLine::ToolType = QStringLiteral("flippingByLine");

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByLine::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogFlippingByLine> dialogTool = qobject_cast<DialogFlippingByLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetFirstLinePointId(m_firstLinePointId);
    dialogTool->SetSecondLinePointId(m_secondLinePointId);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);

    // Every line the operation carries comes back into the dialog: each one was picked by the user, one click
    // per line, and there is no longer a line that follows its points on its own.
    dialogTool->SetLineSources(Lines());

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByLine::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                                 VContainer *data) -> VToolFlippingByLine *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogFlippingByLine> dialogTool = qobject_cast<DialogFlippingByLine *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolFlippingByLineInitData initData;
    initData.firstLinePointId = dialogTool->GetFirstLinePointId();
    initData.secondLinePointId = dialogTool->GetSecondLinePointId();
    initData.source = dialogTool->GetSourceObjects();
    initData.lines = OOperationLines::Collect(dialogTool->GetLineSources());
    initData.hasLinkedVisibilityGroup = dialogTool->HasLinkedVisibilityGroup();
    initData.visibilityGroupName = dialogTool->GetVisibilityGroupName();
    initData.layerGroup = dialogTool->GetLayerGroup();
    initData.newLayerGroupName = dialogTool->GetNewLayerGroupName();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();

    VToolFlippingByLine *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByLine::Create(VToolFlippingByLineInitData initData) -> VToolFlippingByLine *
{
    const auto firstPoint = *initData.data->GeometricObject<VPointF>(initData.firstLinePointId);
    const auto fPoint = static_cast<QPointF>(firstPoint);

    const auto secondPoint = *initData.data->GeometricObject<VPointF>(initData.secondLinePointId);
    const auto sPoint = static_cast<QPointF>(secondPoint);

    if (initData.typeCreation == Source::FromGui)
    {
        initData.destination.clear(); // Try to avoid mistake, value must be empty

        initData.id = initData.data->getNextId(); // Just reserve id for tool

        // The transformed lines are persisted with the operation, so their ids are reserved now, before
        // the operation element is written.
        OOperationLines::AllocateResultIds(initData.data, initData.lines);
    }
    else
    {
        PrepareNames(initData);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    CreateDestination(initData, fPoint, sPoint);

    patternGraph->AddEdge(initData.firstLinePointId, initData.id);
    patternGraph->AddEdge(initData.secondLinePointId, initData.id);

    for (const auto &object : std::as_const(initData.source))
    {
        patternGraph->AddEdge(object.id, initData.id);
    }

    for (const auto &object : std::as_const(initData.destination))
    {
        patternGraph->AddVertex(object.id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, object.id);
    }

    // Rebuilt once the destination objects and their graph nodes exist. Both points of the mirror line lie
    // on it, so they are their own image.
    CreateDestinationLines(initData, {initData.firstLinePointId, initData.secondLinePointId});

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        return AddToolToScene<VToolFlippingByLine>(
            initData, Tool::FlippingByLine, tr("flipping by line"),
            [&initData]() { return new VToolFlippingByLine(initData); });
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByLine::FirstLinePointName() const -> QString
{
    return VAbstractTool::data.GetGObject(m_firstLinePointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByLine::SecondLinePointName() const -> QString
{
    return VAbstractTool::data.GetGObject(m_secondLinePointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByLine::ShowVisualization(bool show)
{
    ShowToolVisualization<VisToolFlippingByLine>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByLine::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    ShowContextMenuOf<DialogFlippingByLine>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByLine::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<VisToolFlippingByLine *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetObjects(SourceToObjects(m_source));

        QVector<QPair<quint32, quint32>> previewLines;
        previewLines.reserve(m_lines.size());
        for (const auto &line : std::as_const(m_lines))
        {
            previewLines.append(qMakePair(line.point1, line.point2));
        }
        visual->SetPreviewLines(previewLines);
        visual->SetFirstLinePointId(m_firstLinePointId);
        visual->SetSecondLinePointId(m_secondLinePointId);
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByLine::SaveDialog(QDomElement &domElement)
{
    const QPointer<DialogFlippingByLine> dialogTool = DialogOf<DialogFlippingByLine>();

    doc->SetAttribute(domElement, AttrP1Line, QString().setNum(dialogTool->GetFirstLinePointId()));
    doc->SetAttribute(domElement, AttrP2Line, QString().setNum(dialogTool->GetSecondLinePointId()));
    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();
    const QVector<DestinationItem> destination = SyncDestination(source);
    // Persist the dialog line selection: a line added here must be created, a line dropped must go, and
    // the transformed copies that already exist keep their id.
    m_lines = OOperationLines::SyncLines(getData(), m_lines, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);

    // Save visibility data for later use
    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByLine::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractFlipping::ReadToolAttributes(domElement);

    m_firstLinePointId = VAbstractPattern::GetParametrUInt(domElement, AttrP1Line, NULL_ID_STR);
    m_secondLinePointId = VAbstractPattern::GetParametrUInt(domElement, AttrP2Line, NULL_ID_STR);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByLine::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractFlipping::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrP1Line, QString().setNum(m_firstLinePointId));
    doc->SetAttribute(tag, AttrP2Line, QString().setNum(m_secondLinePointId));
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByLine::MakeToolTip() const -> QString
{
    // The axis is told by two points, and a point name is never written (see OToolTip): what is left is the
    // layer the result belongs to.
    return VisibilityGroupToolTip();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolFlippingByLine::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<DialogFlippingByLine>(oldDomElement, newDomElement);
}

//---------------------------------------------------------------------------------------------------------------------
VToolFlippingByLine::VToolFlippingByLine(const VToolFlippingByLineInitData &initData, QGraphicsItem *parent)
  : VAbstractFlipping(initData, parent),
    m_firstLinePointId(initData.firstLinePointId),
    m_secondLinePointId(initData.secondLinePointId)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolFlippingByLine::SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>
{
    bool unchanged = false;
    const QMap<QUuid, DestinationItem> existingByRecord = SyncDestinationPreamble(source, unchanged);
    if (unchanged)
    {
        return m_destination;
    }

    const auto firstPoint = *getData()->GeometricObject<VPointF>(m_firstLinePointId);
    const auto fPoint = static_cast<QPointF>(firstPoint);

    const auto secondPoint = *getData()->GeometricObject<VPointF>(m_secondLinePointId);
    const auto sPoint = static_cast<QPointF>(secondPoint);

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
