/************************************************************************
 **
 **  @file   otooltransfert.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   29/09/2026
 **
 **  @brief  The transfer tool: a portion of one pattern part grafted onto another one, as a copy recomputed
 **          from its sources at every parse.
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

#include "otooltransfert.h"

#include <QDomElement>
#include <QDomNode>
#include <QHash>
#include <QLineF>
#include <QPointF>
#include <QSharedPointer>
#include <QString>
#include <QUndoStack>
#include <QVector>
#include <QtGlobal>

#include "../../../dialogs/tools/dialogtool.h"
#include "../../../dialogs/tools/odialogtransfert.h"
#include "../../../undocommands/undogroup.h"
#include "../../../visualization/line/operation/ovistooltransfert.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "ooperationdestination.h"
#include "ooperationlines.h"
#include "otransferttransform.h"
#include "vabstractoperation.h"

class QGraphicsSceneContextMenuEvent;

const QString OToolTransfert::ToolType = QStringLiteral("transfert");

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto MakeDestination(const QPointF &origin, const QPointF &target) -> OOperationDestination<OTransfertTransform>
{
    return OOperationDestination<OTransfertTransform>(OTransfertTransform(origin, target));
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The points this operation references instead of copying: X, whose image is A. OOperationLines reads
 *  it first, so a line anchored on X is rebuilt on the merge point of the host part. */
auto ReferencedPoints(quint32 origin, quint32 target) -> QHash<quint32, quint32>
{
    QHash<quint32, quint32> referenced;
    if (origin != NULL_ID && target != NULL_ID)
    {
        referenced.insert(origin, target);
    }
    return referenced;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Build the images of the portion in the container, or rewrite them when the tool is read back.
 *
 * The merge point of the source part is referenced, not copied: it keeps its place in the source list — the two
 * lists are paired positionally everywhere else — with no object of its own, and OOperationLines reads the
 * X → A table to rebuild a line anchored on it on the merge point of the host part. */
void CreateDestination(OToolTransfertInitData &initData)
{
    const QSharedPointer<VPointF> origin = initData.data->GeometricObject<VPointF>(initData.origin);
    const QSharedPointer<VPointF> target = initData.data->GeometricObject<VPointF>(initData.target);

    const QVector<quint32> referenced{initData.origin};
    const OOperationDestination<OTransfertTransform> destination
        = MakeDestination(static_cast<QPointF>(*origin), static_cast<QPointF>(*target));

    if (initData.typeCreation == Source::FromGui)
    {
        destination.Create(initData.id, initData.source, initData.data, initData.destination, referenced);
    }
    else
    {
        destination.Update(initData.id, initData.source, initData.data, initData.destination, referenced);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The layer the copies are written into.
 *
 * A tool attaches the objects it creates to the active layer of the active part, so the layer of the transfer is
 * the one that is made active here: an existing layer of the host part the user picked, or a new one created
 * with the name they typed. Written before the tool instance exists, because that is when the attachment
 * happens. */
void PrepareHostLayer(OToolTransfertInitData &initData)
{
    if (initData.layer == NULL_ID && initData.layerName.isEmpty())
    {
        return; // no layer asked for: the copies follow the host part's active layer, as for any operation
    }

    VAbstractPattern *doc = initData.doc;
    quint32 groupId = initData.layer;

    if (groupId == NULL_ID)
    {
        groupId = initData.data->getNextId();
        const QDomElement group
            = doc->CreateGroup(groupId, initData.layerName, {}, {}, initData.id);
        if (group.isNull())
        {
            return;
        }

        auto *addGroup = new AddGroup(group, doc);
        QObject::connect(addGroup, &AddGroup::UpdateGroups, doc, &VAbstractPattern::UpdateVisiblityGroups);
        VAbstractApplication::VApp()->getUndoStack()->push(addGroup);
    }

    doc->SetActiveGroup(groupId, doc->PatternBlockMapper()->GetActive());
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OToolTransfert::OToolTransfert(const OToolTransfertInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_origPointId(initData.origin),
    m_targetPointId(initData.target)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTransfert::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                            VContainer *data) -> OToolTransfert *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogTransfert> dialogTool = qobject_cast<ODialogTransfert *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolTransfertInitData initData;
    initData.origin = dialogTool->GetOriginPointId();
    initData.target = dialogTool->GetTargetPointId();
    initData.layer = dialogTool->GetHostLayerId();
    initData.layerName = dialogTool->GetHostLayerName();
    initData.source = dialogTool->GetSourceObjects();
    initData.lines = OOperationLines::Collect(data, initData.source, dialogTool->GetLineSources());
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();

    OToolTransfert *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTransfert::Create(OToolTransfertInitData &initData) -> OToolTransfert *
{
    if (initData.typeCreation == Source::FromGui)
    {
        initData.destination.clear(); // Try to avoid mistake, value must be empty
        initData.id = initData.data->getNextId(); // Just reserve id for tool

        // The transformed lines are persisted with the operation, so their ids are reserved now, before the
        // operation element is written.
        OOperationLines::AllocateResultIds(initData.data, initData.lines);
    }
    else
    {
        PrepareNames(initData);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    CreateDestination(initData);

    // Both merge points are read by the copies: X for the objects of the source part, A because the graph of the
    // host part must know what the operation depends on.
    if (initData.origin != NULL_ID)
    {
        patternGraph->AddEdge(initData.origin, initData.id);
    }
    if (initData.target != NULL_ID)
    {
        patternGraph->AddEdge(initData.target, initData.id);
    }

    for (const auto &object : std::as_const(initData.source))
    {
        patternGraph->AddEdge(object.id, initData.id);
    }

    for (const auto &object : std::as_const(initData.destination))
    {
        if (object.id == NULL_ID)
        {
            continue; // The referenced merge point: no object of its own, hence no node in the graph.
        }

        patternGraph->AddVertex(object.id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, object.id);
    }

    // A translation has no fixed point: X is not left in place, its image is A, which the referenced-point table
    // says. Rebuilt once the destination objects and their graph nodes exist.
    CreateDestinationLines(initData, {}, ReferencedPoints(initData.origin, initData.target));

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        const bool fromGui = initData.typeCreation == Source::FromGui;

        if (fromGui)
        {
            // One gesture: the layer that receives the copies, and the operation itself. Ctrl+Z takes the two
            // back together.
            VAbstractApplication::VApp()->getUndoStack()->beginMacro(tr("transfer"));
            PrepareHostLayer(initData);
        }

        VAbstractTool::AddRecord(initData.id, Tool::Transfert, initData.doc);
        auto *tool = new OToolTransfert(initData);
        initData.scene->addItem(tool);
        InitOperationToolConnections(initData.scene, tool);
        VAbstractPattern::AddTool(initData.id, tool);

        if (fromGui)
        {
            VAbstractApplication::VApp()->getUndoStack()->endMacro();
        }

        return tool;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTransfert::OriginPointName() const -> QString
{
    try
    {
        return VAbstractTool::data.GetGObject(m_origPointId)->name();
    }
    catch (const VExceptionBadId &)
    {
        return tr("Merge point");
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTransfert::TargetPointName() const -> QString
{
    try
    {
        return VAbstractTool::data.GetGObject(m_targetPointId)->name();
    }
    catch (const VExceptionBadId &)
    {
        return tr("Merge point");
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolTransfert::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogTransfert> dialogTool = qobject_cast<ODialogTransfert *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetOriginPointId(m_origPointId);
    dialogTool->SetTargetPointId(m_targetPointId);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);
    dialogTool->SetDestinationObjects(m_destination);

    // The part the operation lives in is the active one — a tool is disabled in any other — and that is what the
    // dialog shows as the host part.
    dialogTool->SetHostPartName(doc->PatternBlockMapper()->GetActive());

    // Only the lines explicitly selected by the user come back into the dialog: the implicit ones follow their
    // points.
    QVector<quint32> lineSources;
    for (const auto &line : Lines())
    {
        if (OOperationLines::IsExplicit(line))
        {
            lineSources.append(line.source);
        }
    }
    dialogTool->SetLineSources(lineSources);

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolTransfert::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolTransfert>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolTransfert::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    ShowContextMenuOf<ODialogTransfert>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolTransfert::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<OVisToolTransfert *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetObjects(SourceToObjects(m_source));

        QVector<QPair<quint32, quint32>> previewLines;
        previewLines.reserve(m_lines.size());
        for (const auto &line : std::as_const(m_lines))
        {
            previewLines.append(qMakePair(line.point1, line.point2));
        }
        visual->SetPreviewLines(previewLines);

        visual->SetOriginPointId(m_origPointId);
        visual->SetTargetPointId(m_targetPointId);
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolTransfert::SaveDialog(QDomElement &domElement)
{
    const QPointer<ODialogTransfert> dialogTool = DialogOf<ODialogTransfert>();

    doc->SetAttribute(domElement, AttrOrigin, QString().setNum(dialogTool->GetOriginPointId()));
    doc->SetAttribute(domElement, AttrTarget, QString().setNum(dialogTool->GetTargetPointId()));

    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();
    const QVector<DestinationItem> destination = SyncDestination(source);

    m_lines = OOperationLines::SyncLines(getData(), m_lines, source, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);

    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolTransfert::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractOperation::ReadToolAttributes(domElement);

    m_origPointId = VDomDocument::GetParametrUInt(domElement, AttrOrigin, NULL_ID_STR);
    m_targetPointId = VDomDocument::GetParametrUInt(domElement, AttrTarget, NULL_ID_STR);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolTransfert::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractOperation::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrOrigin, QString().setNum(m_origPointId));
    doc->SetAttribute(tag, AttrTarget, QString().setNum(m_targetPointId));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTransfert::MakeToolTip() const -> QString
{
    return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>"
                          "<tr> <td><b>%3:</b> %4</td> </tr>"
                          "%5")
        .arg(tr("From"),        // 1
             OriginPointName()) // 2
        .arg(tr("To"),          // 3
             TargetPointName(), // 4
             VisibilityGroupToolTip()); // 5
}

//---------------------------------------------------------------------------------------------------------------------
void OToolTransfert::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<ODialogTransfert>(oldDomElement, newDomElement);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolTransfert::SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>
{
    bool unchanged = false;
    const QMap<QUuid, DestinationItem> existingByRecord = SyncDestinationPreamble(source, unchanged);
    if (unchanged)
    {
        return m_destination;
    }

    const QSharedPointer<VPointF> origin = getData()->GeometricObject<VPointF>(m_origPointId);
    const QSharedPointer<VPointF> target = getData()->GeometricObject<VPointF>(m_targetPointId);
    const OOperationDestination<OTransfertTransform> destination
        = MakeDestination(static_cast<QPointF>(*origin), static_cast<QPointF>(*target));

    QVector<DestinationItem> newDestination;
    newDestination.reserve(source.size());

    for (const auto &srcItem : std::as_const(source))
    {
        if (srcItem.id == m_origPointId)
        {
            // The merge point is referenced, not copied: it keeps its place with no object of its own.
            newDestination.append({.recordId = srcItem.recordId});
        }
        else if (existingByRecord.contains(srcItem.recordId))
        {
            // Preserve existing destination item
            newDestination.append(existingByRecord.value(srcItem.recordId));
        }
        else
        {
            // Create destination item for newly added source
            newDestination.append(destination.CreateObject(m_id, srcItem, getData()));
        }
    }

    return newDestination;
}
