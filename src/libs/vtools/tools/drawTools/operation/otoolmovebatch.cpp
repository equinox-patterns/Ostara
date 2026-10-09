/************************************************************************
 **
 **  @file   otoolmovebatch.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/10/2026
 **
 **  @brief  The move by batch: N moves, each one carried by the objects of one cut line.
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

#include "otoolmovebatch.h"

#include <QDomNode>
#include <QDomNodeList>
#include <QMap>
#include <QSharedPointer>

#include "../../../dialogs/tools/dialogtool.h"
#include "../../../dialogs/tools/odialogmovebatch.h"
#include "../../../visualization/line/operation/ovistoolmovebatch.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vgobject.h"
#include "../vpatterndb/vcontainer.h"
#include "omovebatchtransform.h"
#include "ooperationdestination.h"

class QDomElement;
class QGraphicsSceneContextMenuEvent;

using namespace Qt::Literals::StringLiterals;

const QString OToolMoveBatch::ToolType = QStringLiteral("movingbatch");
const QString OToolMoveBatch::TagMoves = QStringLiteral("moves");
const QString OToolMoveBatch::AttrStart = QStringLiteral("start");
const QString OToolMoveBatch::AttrPivot = QStringLiteral("pivot");

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto MakeDestination(const QVector<SourceItem> &source, const QVector<OMoveBatchMove> &moves,
                     const QVector<qreal> &lengths, const QVector<qreal> &angles,
                     const QVector<qreal> &rotationAngles, VContainer *data)
    -> OOperationDestination<OMoveBatchTransform>
{
    return OOperationDestination<OMoveBatchTransform>(
        OMoveBatchTransform(source, moves, lengths, angles, rotationAngles, data));
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The three values of every move, evaluated once: pixels for the length, degrees for the two angles. */
struct MoveBatchValues
{
    QVector<qreal> lengths{};
    QVector<qreal> angles{};
    QVector<qreal> rotationAngles{};
};

//---------------------------------------------------------------------------------------------------------------------
auto EvaluateValues(quint32 idTool, const QVector<OMoveBatchMove> &moves, VContainer *data) -> MoveBatchValues
{
    MoveBatchValues values;
    values.lengths.reserve(moves.size());
    values.angles.reserve(moves.size());
    values.rotationAngles.reserve(moves.size());

    for (const auto &move : std::as_const(moves))
    {
        QString length = move.length; // CheckFormula may rewrite the formula it reads
        values.lengths.append(
            VAbstractValApplication::VApp()->toPixel(VAbstractTool::CheckFormula(idTool, length, data)));

        QString angle = move.angle; // CheckFormula may rewrite the formula it reads
        values.angles.append(VAbstractTool::CheckFormula(idTool, angle, data));

        QString rotationAngle = move.rotationAngle; // CheckFormula may rewrite the formula it reads
        values.rotationAngles.append(VAbstractTool::CheckFormula(idTool, rotationAngle, data));
    }

    return values;
}

//---------------------------------------------------------------------------------------------------------------------
void CreateDestination(OToolMoveBatchInitData &initData)
{
    const MoveBatchValues values = EvaluateValues(initData.id, initData.moves, initData.data);
    const OOperationDestination<OMoveBatchTransform> destination = MakeDestination(
        initData.source, initData.moves, values.lengths, values.angles, values.rotationAngles, initData.data);

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
OToolMoveBatch::OToolMoveBatch(const OToolMoveBatchInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_moves(initData.moves)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolMoveBatch::SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>
{
    bool unchanged = false;
    const QMap<QUuid, DestinationItem> existingByRecord = SyncDestinationPreamble(source, unchanged);
    if (unchanged)
    {
        return m_destination;
    }

    const MoveBatchValues values = EvaluateValues(m_id, m_moves, getData());
    const OOperationDestination<OMoveBatchTransform> destination =
        MakeDestination(source, m_moves, values.lengths, values.angles, values.rotationAngles, getData());

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
void OToolMoveBatch::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogMoveBatch> dialogTool = qobject_cast<ODialogMoveBatch *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetMoves(m_moves);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);
    dialogTool->SetLineSources(Lines());

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolMoveBatch::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                            VContainer *data) -> OToolMoveBatch *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogMoveBatch> dialogTool = qobject_cast<ODialogMoveBatch *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolMoveBatchInitData initData;
    initData.moves = dialogTool->GetMoves();
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

    OToolMoveBatch *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolMoveBatch::Create(OToolMoveBatchInitData &initData) -> OToolMoveBatch *
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
    for (const auto &move : std::as_const(initData.moves))
    {
        initData.doc->FindFormulaDependencies(move.length, initData.id, varData);
        initData.doc->FindFormulaDependencies(move.angle, initData.id, varData);
        initData.doc->FindFormulaDependencies(move.rotationAngle, initData.id, varData);
    }

    CreateDestination(initData);

    // The two points of every move are read by the copies, so the graph knows what the operation depends on.
    for (const auto &move : std::as_const(initData.moves))
    {
        if (move.startId != NULL_ID)
        {
            patternGraph->AddEdge(move.startId, initData.id);
        }
        if (move.pivotId != NULL_ID)
        {
            patternGraph->AddEdge(move.pivotId, initData.id);
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

    // A move by batch has no fixed point: every pivot travels with the moves that precede it.
    CreateDestinationLines(initData, {});

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        return AddToolToScene<OToolMoveBatch>(initData, Tool::MoveBatch, tr("move by batch"),
                                             [&initData]() { return new OToolMoveBatch(initData); });
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolMoveBatch::Moves() const -> const QVector<OMoveBatchMove> &
{
    return m_moves;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::SetMoves(const QVector<OMoveBatchMove> &moves)
{
    m_moves = moves;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolMoveBatch>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::OpenOptionsDialog()
{
    ShowOptionsDialog<ODialogMoveBatch>();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    ShowContextMenuOf<ODialogMoveBatch>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<OVisToolMoveBatch *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetObjects(SourceToObjects(m_source));

        QVector<QPair<quint32, quint32>> lines;
        lines.reserve(m_lines.size());
        for (const auto &line : std::as_const(m_lines))
        {
            lines.append(qMakePair(line.point1, line.point2));
        }
        visual->SetPreviewLines(lines);

        visual->SetMoves(SourceToObjects(m_source), m_moves);
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::SaveDialog(QDomElement &domElement)
{
    const QPointer<ODialogMoveBatch> dialogTool = DialogOf<ODialogMoveBatch>();

    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();

    m_moves = dialogTool->GetMoves();

    const QVector<DestinationItem> destination = SyncDestination(source);

    m_lines = OOperationLines::SyncLines(getData(), m_lines, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);
    SaveMoves(doc, domElement, m_moves);

    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractOperation::ReadToolAttributes(domElement);

    m_moves = ExtractMoves(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractOperation::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    SaveMoves(doc, tag, m_moves);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::SaveMoves(VAbstractPattern *doc, QDomElement &tag, const QVector<OMoveBatchMove> &moves)
{
    if (moves.isEmpty())
    {
        return;
    }

    QDomElement tagMoves = doc->createElement(TagMoves);
    for (const auto &move : std::as_const(moves))
    {
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        doc->SetAttribute(item, AttrStart, move.startId);
        doc->SetAttribute(item, AttrPivot, move.pivotId);
        doc->SetAttribute(item, AttrName, move.name);
        doc->SetAttribute(item, AttrLength, move.length);
        doc->SetAttribute(item, AttrAngle, move.angle);
        doc->SetAttribute(item, AttrRotationAngle, move.rotationAngle);
        tagMoves.appendChild(item);
    }
    tag.appendChild(tagMoves);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolMoveBatch::ExtractMoves(const QDomElement &domElement) -> QVector<OMoveBatchMove>
{
    QVector<OMoveBatchMove> moves;

    const QDomNodeList nodeList = domElement.childNodes();
    for (int i = 0; i < nodeList.size(); ++i)
    {
        const QDomElement element = nodeList.at(i).toElement();
        if (element.isNull() || element.tagName() != TagMoves)
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

            moves.append({.startId = VAbstractPattern::GetParametrUInt(item, AttrStart, NULL_ID_STR),
                          .pivotId = VAbstractPattern::GetParametrUInt(item, AttrPivot, NULL_ID_STR),
                          .name = VDomDocument::GetParametrEmptyString(item, AttrName),
                          .length = VDomDocument::GetParametrString(item, AttrLength, OMoveBatchRule::DefaultLength),
                          .angle = VDomDocument::GetParametrString(item, AttrAngle, OMoveBatchRule::DefaultAngle),
                          .rotationAngle = VDomDocument::GetParametrString(item, AttrRotationAngle,
                                                                          OMoveBatchRule::DefaultRotationAngle)});
        }

        break;
    }

    return moves;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolMoveBatch::MakeToolTip() const -> QString
{
    return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>"
                          "%3")
        .arg(tr("Moves"), QString::number(m_moves.size())) // 1, 2
        .arg(VisibilityGroupToolTip());                    // 3
}

//---------------------------------------------------------------------------------------------------------------------
void OToolMoveBatch::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<ODialogMoveBatch>(oldDomElement, newDomElement);
}
