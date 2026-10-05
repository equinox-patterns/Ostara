/************************************************************************
 **
 **  @file   otoolspread.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The slash-and-spread tool: a batch of rotations that opens a piece along N cut lines.
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

#include "otoolspread.h"

#include <QDomNode>
#include <QDomNodeList>
#include <QMap>
#include <QSharedPointer>

#include "../../../dialogs/tools/dialogtool.h"
#include "../../../dialogs/tools/odialogspread.h"
#include "../../../visualization/line/operation/ovistoolspread.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vgobject.h"
#include "../vpatterndb/vcontainer.h"
#include "ooperationdestination.h"
#include "ospreadtransform.h"

class QDomElement;
class QGraphicsSceneContextMenuEvent;

using namespace Qt::Literals::StringLiterals;

const QString OToolSpread::ToolType = QStringLiteral("spread");
const QString OToolSpread::TagRotations = QStringLiteral("rotations");
const QString OToolSpread::AttrStart = QStringLiteral("start");
const QString OToolSpread::AttrPivot = QStringLiteral("pivot");

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto MakeDestination(const QVector<SourceItem> &source, const QVector<OSpreadRotation> &rotations,
                     const QVector<qreal> &angles, VContainer *data) -> OOperationDestination<OSpreadTransform>
{
    return OOperationDestination<OSpreadTransform>(OSpreadTransform(source, rotations, angles, data));
}

//---------------------------------------------------------------------------------------------------------------------
auto EvaluateAngles(quint32 idTool, const QVector<OSpreadRotation> &rotations, VContainer *data) -> QVector<qreal>
{
    QVector<qreal> angles;
    angles.reserve(rotations.size());
    for (const auto &rotation : std::as_const(rotations))
    {
        QString formula = rotation.angle; // CheckFormula may rewrite the formula it reads
        angles.append(VAbstractTool::CheckFormula(idTool, formula, data));
    }
    return angles;
}

//---------------------------------------------------------------------------------------------------------------------
void CreateDestination(OToolSpreadInitData &initData)
{
    const QVector<qreal> angles = EvaluateAngles(initData.id, initData.rotations, initData.data);
    const OOperationDestination<OSpreadTransform> destination =
        MakeDestination(initData.source, initData.rotations, angles, initData.data);

    if (initData.typeCreation == Source::FromGui)
    {
        destination.Create(initData.id, initData.source, initData.data, initData.destination);
    }
    else
    {
        destination.Update(initData.id, initData.source, initData.data, initData.destination);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OToolSpread::OToolSpread(const OToolSpreadInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_rotations(initData.rotations)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpread::SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>
{
    bool unchanged = false;
    const QMap<QUuid, DestinationItem> existingByRecord = SyncDestinationPreamble(source, unchanged);
    if (unchanged)
    {
        return m_destination;
    }

    const QVector<qreal> angles = EvaluateAngles(m_id, m_rotations, getData());
    const OOperationDestination<OSpreadTransform> destination =
        MakeDestination(source, m_rotations, angles, getData());

    QVector<DestinationItem> newDestination;
    newDestination.reserve(source.size());

    for (const auto &srcItem : std::as_const(source))
    {
        if (existingByRecord.contains(srcItem.recordId))
        {
            newDestination.append(existingByRecord.value(srcItem.recordId));
        }
        else
        {
            newDestination.append(destination.CreateObject(m_id, srcItem, getData()));
        }
    }

    return newDestination;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogSpread> dialogTool = qobject_cast<ODialogSpread *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetRotations(m_rotations);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);
    dialogTool->SetLineSources(Lines());

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpread::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                         VContainer *data) -> OToolSpread *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogSpread> dialogTool = qobject_cast<ODialogSpread *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolSpreadInitData initData;
    initData.rotations = dialogTool->GetRotations();
    initData.source = dialogTool->GetSourceObjects();
    initData.lines = OOperationLines::Collect(dialogTool->GetLineSources());
    initData.hasLinkedVisibilityGroup = dialogTool->HasLinkedVisibilityGroup();
    initData.visibilityGroupName = dialogTool->GetVisibilityGroupName();
    initData.visibilityGroupTags = dialogTool->GetVisibilityGroupTags();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();

    OToolSpread *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpread::Create(OToolSpreadInitData &initData) -> OToolSpread *
{
    if (initData.typeCreation == Source::FromGui)
    {
        initData.destination.clear(); // Try to avoid mistake, value must be empty

        initData.id = initData.data->getNextId(); // Just reserve id for tool

        OOperationLines::AllocateResultIds(initData.data, initData.lines);
    }
    else
    {
        PrepareNames(initData);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();
    for (const auto &rotation : std::as_const(initData.rotations))
    {
        initData.doc->FindFormulaDependencies(rotation.angle, initData.id, varData);
    }

    CreateDestination(initData);

    // The two points of every rotation are read by the copies, so the graph knows what the operation depends on.
    for (const auto &rotation : std::as_const(initData.rotations))
    {
        if (rotation.startId != NULL_ID)
        {
            patternGraph->AddEdge(rotation.startId, initData.id);
        }
        if (rotation.pivotId != NULL_ID)
        {
            patternGraph->AddEdge(rotation.pivotId, initData.id);
        }
    }

    for (const auto &object : std::as_const(initData.source))
    {
        patternGraph->AddEdge(object.id, initData.id);
    }

    for (const auto &object : std::as_const(initData.destination))
    {
        patternGraph->AddVertex(object.id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, object.id);
    }

    // A slash-and-spread has no fixed point: every pivot turns in the rotations that precede it.
    CreateDestinationLines(initData, {});

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        return AddToolToScene<OToolSpread>(initData, Tool::Spread, tr("slash and spread"),
                                           [&initData]() { return new OToolSpread(initData); });
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpread::Rotations() const -> const QVector<OSpreadRotation> &
{
    return m_rotations;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SetRotations(const QVector<OSpreadRotation> &rotations)
{
    m_rotations = rotations;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolSpread>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::OpenOptionsDialog()
{
    ShowOptionsDialog<ODialogSpread>();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    ShowContextMenuOf<ODialogSpread>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<OVisToolSpread *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetObjects(SourceToObjects(m_source));

        QVector<QPair<quint32, quint32>> lines;
        lines.reserve(m_lines.size());
        for (const auto &line : std::as_const(m_lines))
        {
            lines.append(qMakePair(line.point1, line.point2));
        }
        visual->SetPreviewLines(lines);

        visual->SetRotations(SourceToObjects(m_source), m_rotations);
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SaveDialog(QDomElement &domElement)
{
    const QPointer<ODialogSpread> dialogTool = DialogOf<ODialogSpread>();

    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();

    m_rotations = dialogTool->GetRotations();

    const QVector<DestinationItem> destination = SyncDestination(source);

    m_lines = OOperationLines::SyncLines(getData(), m_lines, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);

    SaveRotations(doc, domElement, m_rotations);

    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractOperation::ReadToolAttributes(domElement);

    m_rotations = ExtractRotations(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractOperation::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    SaveRotations(doc, tag, m_rotations);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SaveRotations(VAbstractPattern *doc, QDomElement &tag, const QVector<OSpreadRotation> &rotations)
{
    if (rotations.isEmpty())
    {
        return;
    }

    QDomElement tagRotations = doc->createElement(TagRotations);
    for (const auto &rotation : std::as_const(rotations))
    {
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        doc->SetAttribute(item, AttrStart, rotation.startId);
        doc->SetAttribute(item, AttrPivot, rotation.pivotId);
        doc->SetAttribute(item, AttrAngle, rotation.angle);
        doc->SetAttribute(item, AttrName, rotation.name);
        tagRotations.appendChild(item);
    }
    tag.appendChild(tagRotations);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpread::ExtractRotations(const QDomElement &domElement) -> QVector<OSpreadRotation>
{
    QVector<OSpreadRotation> rotations;

    const QDomNodeList nodeList = domElement.childNodes();
    for (int i = 0; i < nodeList.size(); ++i)
    {
        const QDomElement element = nodeList.at(i).toElement();
        if (element.isNull() || element.tagName() != TagRotations)
        {
            continue;
        }

        const QDomNodeList items = element.childNodes();
        for (int j = 0; j < items.size(); ++j)
        {
            const QDomElement item = items.at(j).toElement();
            if (item.isNull() || item.tagName() != VAbstractOperation::TagItem)
            {
                continue;
            }

            rotations.append({.startId = VAbstractPattern::GetParametrUInt(item, AttrStart, NULL_ID_STR),
                              .pivotId = VAbstractPattern::GetParametrUInt(item, AttrPivot, NULL_ID_STR),
                              .name = VDomDocument::GetParametrEmptyString(item, AttrName),
                              .angle = VDomDocument::GetParametrString(item, AttrAngle, OSpreadRule::DefaultAngle)});
        }

        break;
    }

    return rotations;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpread::MakeToolTip() const -> QString
{
    return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>"
                          "%3")
        .arg(tr("Rotations"), QString::number(m_rotations.size())) // 1, 2
        .arg(VisibilityGroupToolTip());                            // 3
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<ODialogSpread>(oldDomElement, newDomElement);
}
