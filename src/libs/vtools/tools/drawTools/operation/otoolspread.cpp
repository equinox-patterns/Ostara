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
#include <QUndoCommand>

#include "../../../dialogs/tools/dialogtool.h"
#include "../../../dialogs/tools/odialogspread.h"
#include "../../../visualization/line/operation/ovistoolspread.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../../../undocommands/savetooloptions.h"
#include "../vgeometry/vgobject.h"
#include "../vpatterndb/vcontainer.h"
#include "ooperationdestination.h"
#include "ospreaddisplay.h"
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
/** @brief Give the display what it needs to be built: an id for the objects it is still missing, and the
 *  placement of the points it holds, which follows the rotations and changes with every edit. */
void CreateDisplay(OToolSpreadInitData &initData, const OSpreadPlan &plan)
{
    if (not initData.display.show)
    {
        // Nothing is kept while the representation is off: switching it on allocates the marks again, which is
        // what "the objects are created again" means (see SlashAndSpread.md).
        initData.display.marks.clear();
        return;
    }

    OSpreadDisplay::EnsureIds(initData.display, initData.rotations, plan, initData.data);
    OSpreadDisplay::Create(initData.id, initData.display, plan, initData.data);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Create or refresh the two lines of every mark of the display.
 *
 * Their endpoints are objects, not source ids: the apex is the copy of the pivot, the base the copy of the start
 * point and the third vertex of the mark. That vertex is the start point itself for the first rotation, the one
 * place where a mark ends on a source object instead of an object of its own: a source object PointImage would
 * answer with the copy of, which is why the ends are resolved here. */
void CreateDisplayLines(const OToolSpreadInitData &initData)
{
    if (not initData.display.show)
    {
        return;
    }

    for (int k = 0; k < initData.rotations.size() && k < initData.display.marks.size(); ++k)
    {
        const OSpreadRotation &rotation = initData.rotations.at(k);
        const OSpreadMark &mark = initData.display.marks.at(k);

        if (mark.line1 == NULL_ID && mark.line2 == NULL_ID)
        {
            continue; // The pair describes no region: this rotation draws nothing.
        }

        const quint32 apex =
            OOperationLines::PointImage(rotation.pivotId, initData.source, initData.destination, {});
        const quint32 start =
            OOperationLines::PointImage(rotation.startId, initData.source, initData.destination, {});

        OOperationLine line;
        line.typeLine = initData.display.typeLine;
        line.lineColor = initData.display.lineColor;

        if (mark.line1 != NULL_ID)
        {
            line.result = mark.line1;
            OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, apex, start,
                                        initData.parse);
        }

        if (mark.line2 != NULL_ID)
        {
            line.result = mark.line2;
            const quint32 vertex = (mark.point != NULL_ID) ? mark.point : rotation.startId;
            OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, apex, vertex,
                                        initData.parse);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief True when the file already carries exactly this display: the two are compared id by id, so a pattern
 *  written before the display existed gets its ids written once, and no parse writes the file again. */
auto SameDisplay(const OSpreadDisplay &left, const OSpreadDisplay &right) -> bool
{
    if (left.show != right.show || left.typeLine != right.typeLine || left.lineColor != right.lineColor
        || left.marks.size() != right.marks.size())
    {
        return false;
    }

    for (int k = 0; k < left.marks.size(); ++k)
    {
        const OSpreadMark &one = left.marks.at(k);
        const OSpreadMark &other = right.marks.at(k);
        if (one.point != other.point || one.line1 != other.line1 || one.line2 != other.line2
            || one.name != other.name)
        {
            return false;
        }
    }

    return true;
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

    CreateDisplay(initData, OSpreadRule::Compose(initData.source, initData.rotations, angles, initData.data));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OToolSpread::OToolSpread(const OToolSpreadInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_rotations(initData.rotations),
    m_display(initData.display)
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
    dialogTool->SetDisplay(m_display);
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
    initData.display = dialogTool->GetDisplay();
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

    // The points of the rotation display belong to the operation exactly as its copies do, and the graph is what
    // makes them go with it: deleting the operation deletes what hangs from it, and nothing else names them.
    for (const quint32 id : OSpreadDisplay::PointImages(initData.display, initData.data))
    {
        patternGraph->AddVertex(id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, id);
    }

    // A slash-and-spread has no fixed point: every pivot turns in the rotations that precede it.
    CreateDestinationLines(initData, {});

    // The lines of the rotation display, whose ends are objects rather than source ids.
    CreateDisplayLines(initData);

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
auto OToolSpread::Display() const -> const OSpreadDisplay &
{
    return m_display;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SetDisplayShown(bool show)
{
    if (m_display.show == show)
    {
        return;
    }

    m_display.show = show;

    // The representation creates objects, so switching it on has to give their ids to the file before the parse
    // that builds them reads it, and switching it off has to take them away, or the objects would still be
    // declared. SaveOptionsAndReparse writes both and asks for the full parse such a change needs.
    if (m_display.show)
    {
        SyncDisplay(m_source);
    }
    else
    {
        m_display.marks.clear();
    }

    SaveOptionsAndReparse();

    // The objects the display creates join the layer the copies go to, the active one, and nothing else gives them
    // one: a layer names its objects by id, and only the creation of a tool attaches what it produces. This is the
    // call that creation makes, and it skips what the layer already holds, so a display turned on twice, or one
    // whose ids did not change, writes nothing.
    if (m_display.show && doc->GetActiveGroup() != NULL_ID)
    {
        QDomElement const element = doc->FindElementById(m_id, getTagName());
        if (element.isElement())
        {
            doc->AddItemsToGroup(VAbstractPattern::LayerableObjectsOfTool(element, m_id), doc->GetActiveGroup());
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SetDisplayTypeLine(const QString &typeLine)
{
    if (m_display.typeLine == typeLine)
    {
        return;
    }

    m_display.typeLine = typeLine;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SetDisplayColor(const QString &color)
{
    if (m_display.lineColor == color)
    {
        return;
    }

    m_display.lineColor = color;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SyncDisplay(const QVector<SourceItem> &source)
{
    if (not m_display.show)
    {
        m_display.marks.clear();
        return;
    }

    const QVector<qreal> angles = EvaluateAngles(m_id, m_rotations, getData());
    OSpreadDisplay::EnsureIds(m_display, m_rotations,
                              OSpreadRule::Compose(source, m_rotations, angles, getData()), getData());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SaveOptionsAndReparse()
{
    QDomElement const oldDomElement = doc->FindElementById(m_id, getTagName());
    if (!oldDomElement.isElement())
    {
        return;
    }

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    QDomElement newDomElement = oldDomElement.cloneNode().toElement();
    SaveOptions(newDomElement, obj);

    // The command goes in a group of its own so that it can neither merge with the option edit before it nor lose
    // the full parse it asks for: SaveToolOptions merges with its predecessor and the merged command keeps the
    // parse mode of the first one (see SaveToolOptions::mergeWith).
    auto *group = new QUndoCommand();
    group->setText(m_display.show ? tr("show the rotation display") : tr("hide the rotation display"));

    auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id, group);
    saveOptions->SetFullParse(true);
    connect(saveOptions, &SaveToolOptions::NeedFullParsing, doc, &VAbstractPattern::NeedFullParsing,
            Qt::QueuedConnection);
    VAbstractApplication::VApp()->getUndoStack()->push(group);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpread::ExtraPointIds() const -> QVector<quint32>
{
    return OSpreadDisplay::PointImages(m_display, getData());
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpread::ExtraLineIds() const -> QVector<quint32>
{
    QVector<quint32> ids;
    ids.reserve(m_display.marks.size() * 2);
    for (const OSpreadMark &mark : std::as_const(m_display.marks))
    {
        if (mark.line1 != NULL_ID)
        {
            ids.append(mark.line1);
        }
        if (mark.line2 != NULL_ID)
        {
            ids.append(mark.line2);
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpread::DisplayObjectIds(const QDomElement &domElement) const -> QSet<quint32>
{
    // The mark of a rotation: its third vertex, and the two lines that make the region readable.
    QSet<quint32> ids;
    const QDomElement display = domElement.firstChildElement(OSpreadDisplay::TagDisplay);
    for (QDomElement item = display.firstChildElement(VAbstractOperation::TagItem); not item.isNull();
         item = item.nextSiblingElement(VAbstractOperation::TagItem))
    {
        for (const QString &attribute :
             {OSpreadDisplay::AttrPoint, OSpreadDisplay::AttrLine1, OSpreadDisplay::AttrLine2})
        {
            if (const quint32 id = item.attribute(attribute).toUInt(); id != NULL_ID)
            {
                ids.insert(id);
            }
        }
    }
    return ids;
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

        // The spread is in the pattern: the objects it spread are drawn by the pattern itself, and the preview keeps
        // their images only (see OVisToolSpread::SetInPattern).
        visual->SetInPattern(true);
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

    const OSpreadDisplay dialogDisplay = dialogTool->GetDisplay();
    m_display.show = dialogDisplay.show;
    m_display.typeLine = dialogDisplay.typeLine;
    m_display.lineColor = dialogDisplay.lineColor;

    const QVector<DestinationItem> destination = SyncDestination(source);

    SyncDisplay(source);

    m_lines = OOperationLines::SyncLines(getData(), m_lines, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);

    SaveRotations(doc, domElement, m_rotations);

    OSpreadDisplay::Save(doc, domElement, m_display);

    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractOperation::ReadToolAttributes(domElement);

    m_rotations = ExtractRotations(domElement);
    m_display = OSpreadDisplay::Extract(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractOperation::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    SaveRotations(doc, tag, m_rotations);
    OSpreadDisplay::Save(doc, tag, m_display);
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
void OToolSpread::RefreshDataInFile()
{
    VAbstractOperation::RefreshDataInFile();

    // A pattern written before the display existed carries no id for it, and the parse has just given them to the
    // marks. They have to reach the file, exactly as the ids of the copies do: a layer names its objects by id,
    // and an id the file does not carry is one the next open would not recognize.
    QDomElement domElement = doc->FindElementById(m_id, getTagName());
    if (domElement.isElement() && not SameDisplay(OSpreadDisplay::Extract(domElement), m_display))
    {
        OSpreadDisplay::Save(doc, domElement, m_display);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpread::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<ODialogSpread>(oldDomElement, newDomElement);
}
