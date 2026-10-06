/************************************************************************
 **
 **  @file   otoolpleat.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The pleat tool: one fold, taken on a prise, and held by its own figure.
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

#include "otoolpleat.h"

#include <QDomNode>
#include <QDomNodeList>
#include <QMap>
#include <QSharedPointer>
#include <QUndoCommand>

#include "../../../dialogs/tools/dialogtool.h"
#include "../../../dialogs/tools/odialogpleat.h"
#include "../../../undocommands/savetooloptions.h"
#include "../../../visualization/line/operation/ovistoolpleat.h"
#include "../../vabstracttool.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/literals.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "ooperationdestination.h"
#include "ooperationlines.h"
#include "opleatsupport.h"
#include "opleattransform.h"

class QDomElement;
class QGraphicsSceneContextMenuEvent;

using namespace Qt::Literals::StringLiterals;

const QString OToolPleat::ToolType = QStringLiteral("pleat");
const QString OToolPleat::AttrValue = QStringLiteral("value");
const QString OToolPleat::AttrKind = QStringLiteral("kind");
const QString OToolPleat::AttrKindSimple = QStringLiteral("simple");
const QString OToolPleat::AttrKindDouble = QStringLiteral("double");
const QString OToolPleat::AttrCap1 = QStringLiteral("cap1");
const QString OToolPleat::AttrCap2 = QStringLiteral("cap2");
const QString OToolPleat::AttrCap1Sense = QStringLiteral("cap1Sense");
const QString OToolPleat::AttrCap2Sense = QStringLiteral("cap2Sense");
const QString OToolPleat::AttrCap1Ref = QStringLiteral("cap1Ref");
const QString OToolPleat::AttrCap2Ref = QStringLiteral("cap2Ref");
const QString OToolPleat::AttrFoldLine = QStringLiteral("foldLine");

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/** @brief The value of the pleat as it will show once folded, in pixels: the formula the user typed, read in the
 *  pattern's unit and turned into the unit the geometry is drawn in. */
auto PleatValue(quint32 idTool, const QString &value, const VContainer *data) -> qreal
{
    QString formula = value; // CheckFormula may rewrite the formula it reads
    return VAbstractValApplication::VApp()->toPixel(VAbstractTool::CheckFormula(idTool, formula, data));
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The side of the prise the objects that travel are on, as the rule reads it. */
auto PleatSide(const OPleatEnd &first, const OPleatEnd &second, const QVector<SourceItem> &source,
               const VContainer *data) -> OPleatSide
{
    return OPleatRule::SideOf(source, first.pointId, second.pointId, data);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The whole plan of the pleat: the move, and the mark of each end of each pleat. */
auto MakePlan(quint32 idTool, const OPleatEnd &first, const OPleatEnd &second, const QString &value, OPleatKind kind,
              const QVector<SourceItem> &source, const VContainer *data) -> OPleatPlan
{
    return OPleatRule::Compose(first, second, PleatValue(idTool, value, data), kind,
                               PleatSide(first, second, source, data));
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The destination factory of a pleat: the objects that travel, translated. */
auto MakeDestination(quint32 idTool, const OPleatEnd &first, const OPleatEnd &second, const QString &value,
                     OPleatKind kind, const QVector<SourceItem> &source, const VContainer *data)
    -> OOperationDestination<OPleatTransform>
{
    const OPleatTransform transform(first, second, PleatValue(idTool, value, data), kind,
                                    PleatSide(first, second, source, data));
    return OOperationDestination<OPleatTransform>(transform);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The point that carries the cap of one end of one pleat, or NULL_ID when that cap is not drawn. */
auto MarkPoint(const OPleatDisplay &display, int pleat, int end) -> quint32
{
    for (const OPleatMarkDisplay &mark : display.marks)
    {
        if (mark.pleat == pleat && mark.end == end)
        {
            return mark.point;
        }
    }

    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Create every object of the figure, and the lines that join them.
 *
 *  The points come first: a line is carried by two point ids, and the operation's own copy of a prise point only
 *  exists once the destination has been built. */
void CreateDisplayObjects(OToolPleatInitData &initData, const OPleatPlan &plan)
{
    OPleatDisplay::EnsureIds(initData.display, initData.first, initData.second, initData.kind, initData.foldLine, plan,
                             initData.source, initData.destination, initData.data);
    OPleatDisplay::Create(initData.id, initData.display, plan, initData.kind, initData.first, initData.second,
                          initData.data);

    if (plan.failure != OPleatFailure::None || not initData.display.show)
    {
        return; // no figure to draw: nothing was placed, and no line may be built on a point that is not there
    }

    const int positions = OPleatDisplay::PositionCount(initData.kind);
    const auto endpoint = [&initData](int position, int end) -> quint32
    {
        return OPleatDisplay::Endpoint(initData.display, position, end, initData.first, initData.second, initData.source,
                                       initData.destination);
    };

    // The prise, and every place it went to: the same line, translated by one pleat at a time. The last one is the
    // copy line — where the fabric lands — and the one between the two pleats of a double is the line they share.
    OOperationLine line;
    line.typeLine = initData.display.lineTypeLine;
    line.lineColor = initData.display.lineColor;

    for (int position = 0; position < positions && position < initData.display.lines.size(); ++position)
    {
        line.result = initData.display.lines.at(position);
        OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, endpoint(position, 0),
                                    endpoint(position, 1), initData.parse);
    }

    // The fold line of each pleat: what the pleat is pressed along, drawn from one cap to the other.
    OOperationLine fold;
    fold.typeLine = initData.display.foldTypeLine;
    fold.lineColor = initData.display.foldColor;

    for (int pleat = 0; pleat < OPleatDisplay::PleatCount(initData.kind) && pleat < initData.display.foldLines.size();
         ++pleat)
    {
        fold.result = initData.display.foldLines.at(pleat);
        OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, fold, MarkPoint(initData.display, pleat, 0),
                                    MarkPoint(initData.display, pleat, 1), initData.parse);
    }

    // The two sides of every cap: its apex, the prise point it was taken on, and the same point carried by the move.
    for (const OPleatMarkDisplay &mark : std::as_const(initData.display.marks))
    {
        if (mark.pleat < 0 || mark.pleat + 1 >= positions)
        {
            continue;
        }

        if (mark.side1 != NULL_ID)
        {
            line.result = mark.side1;
            OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, mark.point,
                                        endpoint(mark.pleat, mark.end), initData.parse);
        }

        if (mark.side2 != NULL_ID)
        {
            line.result = mark.side2;
            OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, mark.point,
                                        endpoint(mark.pleat + 1, mark.end), initData.parse);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief True when the file already carries exactly this figure: compared id by id, so a pattern written before an
 *  object existed gets its ids written once, and no parse writes the file again. */
auto SameDisplay(const OPleatDisplay &left, const OPleatDisplay &right) -> bool
{
    if (left.show != right.show || left.lineTypeLine != right.lineTypeLine || left.lineColor != right.lineColor
        || left.foldTypeLine != right.foldTypeLine || left.foldColor != right.foldColor
        || left.lines != right.lines || left.foldLines != right.foldLines || left.copies.size() != right.copies.size()
        || left.marks.size() != right.marks.size())
    {
        return false;
    }

    for (int i = 0; i < left.copies.size(); ++i)
    {
        if (left.copies.at(i).point != right.copies.at(i).point || left.copies.at(i).name != right.copies.at(i).name)
        {
            return false;
        }
    }

    for (int i = 0; i < left.marks.size(); ++i)
    {
        const OPleatMarkDisplay &one = left.marks.at(i);
        const OPleatMarkDisplay &other = right.marks.at(i);
        if (one.point != other.point || one.side1 != other.side1 || one.side2 != other.side2 || one.name != other.name)
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void CreateDestination(OToolPleatInitData &initData)
{
    OPleatSupport::Resolve(initData.first, initData.data);
    OPleatSupport::Resolve(initData.second, initData.data);

    const OPleatPlan plan = MakePlan(initData.id, initData.first, initData.second, initData.value, initData.kind,
                                     initData.source, initData.data);
    const OOperationDestination<OPleatTransform> destination
        = MakeDestination(initData.id, initData.first, initData.second, initData.value, initData.kind, initData.source,
                          initData.data);

    if (initData.typeCreation == Source::FromGui)
    {
        destination.Create(initData.id, initData.source, initData.data, initData.destination);
    }
    else
    {
        destination.Update(initData.id, initData.source, initData.data, initData.destination);
    }

    if (plan.failure != OPleatFailure::None)
    {
        // The dialog refuses to build such a pleat, and this only happens on a file edited by hand: the objects that
        // travel are still copied, the figure is not drawn, and the reason reaches the log.
        qWarning() << "The pleat" << initData.id << "cannot be drawn, failure" << static_cast<int>(plan.failure);
    }

    CreateDisplayObjects(initData, plan);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OToolPleat::OToolPleat(const OToolPleatInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_first(initData.first),
    m_second(initData.second),
    m_value(initData.value),
    m_kind(initData.kind),
    m_foldLine(initData.foldLine),
    m_display(initData.display)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::End(int rank) const -> const OPleatEnd &
{
    return (rank == 1) ? m_second : m_first;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetEnd(int rank, const OPleatEnd &end)
{
    (rank == 1) ? m_second = end : m_first = end;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::SyncDestination(const QVector<SourceItem> &source) -> QVector<DestinationItem>
{
    bool unchanged = false;
    const QMap<QUuid, DestinationItem> existingByRecord = SyncDestinationPreamble(source, unchanged);
    if (unchanged)
    {
        return m_destination;
    }

    const OOperationDestination<OPleatTransform> destination
        = MakeDestination(m_id, m_first, m_second, m_value, m_kind, source, getData());

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
void OToolPleat::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogPleat> dialogTool = qobject_cast<ODialogPleat *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetPleat(m_first, m_second, m_value, m_kind, m_foldLine);
    dialogTool->SetDisplay(m_display);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);
    dialogTool->SetLineSources(Lines());

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                        VContainer *data) -> OToolPleat *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogPleat> dialogTool = qobject_cast<ODialogPleat *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolPleatInitData initData;
    initData.first = dialogTool->GetFirst();
    initData.second = dialogTool->GetSecond();
    initData.value = dialogTool->GetValue();
    initData.kind = dialogTool->GetKind();
    initData.foldLine = dialogTool->IsFoldLineShown();
    initData.display = dialogTool->GetDisplay();
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

    OToolPleat *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::Create(OToolPleatInitData &initData) -> OToolPleat *
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

    // The value is a formula like any other: what it reads has to be known before the copies are built.
    initData.doc->FindFormulaDependencies(initData.value, initData.id, initData.data->DataDependencyVariables());

    CreateDestination(initData);

    // What the copies and the caps are read from: the two points of the prise, the points designated to carry an
    // edge, and everything that travels.
    for (const quint32 id : {initData.first.pointId, initData.second.pointId, initData.first.edgePointId,
                             initData.second.edgePointId})
    {
        if (id != NULL_ID)
        {
            patternGraph->AddEdge(id, initData.id);
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

    // The points of the figure belong to the operation exactly as its copies do, and the graph is what makes them go
    // with it: deleting the operation deletes what hangs from it, and nothing else names them.
    for (const quint32 id : OPleatDisplay::PointImages(initData.display, initData.data))
    {
        patternGraph->AddVertex(id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, id);
    }

    // The prise itself is what stays: the pleat has no pivot, and no point of the pattern is left in place by it.
    CreateDestinationLines(initData, {});

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        return AddToolToScene<OToolPleat>(initData, Tool::Pleat, tr("pleat"),
                                          [&initData]() { return new OToolPleat(initData); });
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetValue(const QString &value)
{
    if (m_value == value)
    {
        return;
    }

    m_value = value;
    SaveOptionsAndReparse(true, tr("change the value of the pleat"));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::GetFormulaValue() const -> VFormula
{
    VFormula formula(m_value, getData());
    formula.setToolId(m_id);
    formula.Eval();
    return formula;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetFormulaValue(const VFormula &value)
{
    // A pleat of nothing is no pleat: the rule refuses a value that is not greater than zero, and so does the dock,
    // rather than letting a pattern hold one the tool would refuse to read back.
    if (value.error() || value.getDoubleValue() <= 0.)
    {
        return;
    }

    SetValue(value.GetFormula(FormulaType::FromUser));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetKind(OPleatKind kind)
{
    if (m_kind == kind)
    {
        return;
    }

    m_kind = kind;
    SaveOptionsAndReparse(true, tr("change the kind of the pleat"));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetFoldLineShown(bool show)
{
    if (m_foldLine == show)
    {
        return;
    }

    m_foldLine = show;
    SaveOptionsAndReparse(true, show ? tr("show the fold line") : tr("hide the fold line"));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetCapShown(int rank, bool show)
{
    if (End(rank).showCap == show)
    {
        return;
    }

    OPleatEnd end = End(rank);
    end.showCap = show;
    SetEnd(rank, end);

    SaveOptionsAndReparse(true, show ? tr("show the cap") : tr("hide the cap"));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetCapSecondSense(int rank, bool second)
{
    if (End(rank).secondSense == second)
    {
        return;
    }

    OPleatEnd end = End(rank);
    end.secondSense = second;
    SetEnd(rank, end);

    SaveOptionsAndReparse(true, tr("turn the cap the other way"));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetCapEdgePoint(int rank, quint32 pointId)
{
    if (End(rank).edgePointId == pointId)
    {
        return;
    }

    OPleatEnd end = End(rank);
    end.edgePointId = pointId;
    SetEnd(rank, end);

    SaveOptionsAndReparse(true, tr("change the edge of the cap"));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetDisplay(const OPleatDisplay &display)
{
    m_display = display;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetDisplayLineType(const QString &typeLine)
{
    if (m_display.lineTypeLine == typeLine)
    {
        return;
    }

    m_display.lineTypeLine = typeLine;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetDisplayLineColor(const QString &color)
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
void OToolPleat::SetDisplayFoldLineType(const QString &typeLine)
{
    if (m_display.foldTypeLine == typeLine)
    {
        return;
    }

    m_display.foldTypeLine = typeLine;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetDisplayFoldLineColor(const QString &color)
{
    if (m_display.foldColor == color)
    {
        return;
    }

    m_display.foldColor = color;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SyncDisplay(const QVector<SourceItem> &source)
{
    OPleatSupport::Resolve(m_first, getData());
    OPleatSupport::Resolve(m_second, getData());

    const OPleatPlan plan = MakePlan(m_id, m_first, m_second, m_value, m_kind, source, getData());
    OPleatDisplay::EnsureIds(m_display, m_first, m_second, m_kind, FoldLineDrawn(), plan, source, m_destination,
                             getData());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SaveOptionsAndReparse(bool fullParse, const QString &commandText)
{
    QDomElement const oldDomElement = doc->FindElementById(m_id, getTagName());
    if (!oldDomElement.isElement())
    {
        return;
    }

    // The figure follows the options, so its objects have to be given their ids before the file is written: the parse
    // that rebuilds them reads the ids from here.
    SyncDisplay(m_source);

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    QDomElement newDomElement = oldDomElement.cloneNode().toElement();
    SaveOptions(newDomElement, obj);

    // The command goes in a group of its own so that it can neither merge with the option edit before it nor lose the
    // parse it asks for: SaveToolOptions merges with its predecessor and the merged command keeps the parse mode of
    // the first one (see SaveToolOptions::mergeWith).
    auto *group = new QUndoCommand();
    group->setText(commandText);

    auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id, group);
    if (fullParse)
    {
        saveOptions->SetFullParse(true);
        connect(saveOptions, &SaveToolOptions::NeedFullParsing, doc, &VAbstractPattern::NeedFullParsing,
                Qt::QueuedConnection);
    }
    else
    {
        connect(saveOptions, &SaveToolOptions::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    }

    VAbstractApplication::VApp()->getUndoStack()->push(group);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::ExtraPointIds() const -> QVector<quint32>
{
    return OPleatDisplay::PointImages(m_display, getData());
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::ExtraLineIds() const -> QVector<quint32>
{
    QVector<quint32> ids;
    ids.reserve(m_display.lines.size() + m_display.foldLines.size() + m_display.marks.size() * 2);

    for (const quint32 id : std::as_const(m_display.lines))
    {
        if (id != NULL_ID)
        {
            ids.append(id);
        }
    }

    for (const quint32 id : std::as_const(m_display.foldLines))
    {
        if (id != NULL_ID)
        {
            ids.append(id);
        }
    }

    for (const OPleatMarkDisplay &mark : std::as_const(m_display.marks))
    {
        if (mark.side1 != NULL_ID)
        {
            ids.append(mark.side1);
        }
        if (mark.side2 != NULL_ID)
        {
            ids.append(mark.side2);
        }
    }

    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::DisplayObjectIds(const QDomElement &domElement) const -> QSet<quint32>
{
    // Everything the figure is made of: the lines of the prise and their copies, the fold lines, the copies of the
    // prise points the display places, and the caps. Turning a cap off, or the figure itself, takes some of them
    // away, and only this list says so to the test that chooses between a lite and a full parse.
    QSet<quint32> ids;
    const QDomElement display = domElement.firstChildElement(OPleatDisplay::TagDisplay);
    for (QDomElement item = display.firstChildElement(VAbstractOperation::TagItem); not item.isNull();
         item = item.nextSiblingElement(VAbstractOperation::TagItem))
    {
        for (const QString &attribute :
             {OPleatDisplay::AttrLine, OPleatDisplay::AttrFoldLine, OPleatDisplay::AttrCopy, OPleatDisplay::AttrPoint,
              OPleatDisplay::AttrSide1, OPleatDisplay::AttrSide2})
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
void OToolPleat::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolPleat>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::OpenOptionsDialog()
{
    ShowOptionsDialog<ODialogPleat>();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    ShowContextMenuOf<ODialogPleat>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<OVisToolPleat *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetObjects(SourceToObjects(m_source));

        QVector<QPair<quint32, quint32>> lines;
        lines.reserve(m_lines.size());
        for (const auto &line : std::as_const(m_lines))
        {
            lines.append(qMakePair(line.point1, line.point2));
        }
        visual->SetPreviewLines(lines);

        visual->SetPleat(m_first, m_second, m_value, m_kind, FoldLineDrawn());
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SaveDialog(QDomElement &domElement)
{
    const QPointer<ODialogPleat> dialogTool = DialogOf<ODialogPleat>();

    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();

    m_first = dialogTool->GetFirst();
    m_second = dialogTool->GetSecond();
    m_value = dialogTool->GetValue();
    m_kind = dialogTool->GetKind();
    m_foldLine = dialogTool->IsFoldLineShown();

    const OPleatDisplay dialogDisplay = dialogTool->GetDisplay();
    // The figure is not optional: it is drawn, and the dialog only decides what the fold line looks like.
    m_display.show = true;
    m_display.lineTypeLine = dialogDisplay.lineTypeLine;
    m_display.lineColor = dialogDisplay.lineColor;
    m_display.foldTypeLine = dialogDisplay.foldTypeLine;
    m_display.foldColor = dialogDisplay.foldColor;

    const QVector<DestinationItem> destination = SyncDestination(source);

    SyncDisplay(source);

    m_lines = OOperationLines::SyncLines(getData(), m_lines, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);

    SavePleat(domElement);

    OPleatDisplay::Save(doc, domElement, m_display);

    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractOperation::ReadToolAttributes(domElement);

    OToolPleatInitData initData;
    ExtractPleat(domElement, initData);
    m_first = initData.first;
    m_second = initData.second;
    m_value = initData.value;
    m_kind = initData.kind;
    m_foldLine = initData.foldLine;

    m_display = OPleatDisplay::Extract(domElement);

    // Where the two prise points are, and which way the edge runs there, are never read from the file: they are asked
    // of the container again, which is what makes the figure follow a point the user moves.
    OPleatSupport::Resolve(m_first, getData());
    OPleatSupport::Resolve(m_second, getData());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractOperation::SaveOptions(tag, obj);

    SavePleat(tag);
    OPleatDisplay::Save(doc, tag, m_display);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SavePleat(QDomElement &tag) const
{
    doc->SetAttribute(tag, AttrType, ToolType);

    doc->SetAttribute(tag, AttrFirstPoint, m_first.pointId);
    doc->SetAttribute(tag, AttrSecondPoint, m_second.pointId);
    doc->SetAttribute(tag, AttrValue, m_value);
    doc->SetAttribute(tag, AttrKind, m_kind == OPleatKind::Double ? AttrKindDouble : AttrKindSimple);

    doc->SetAttribute(tag, AttrCap1, m_first.showCap);
    doc->SetAttribute(tag, AttrCap2, m_second.showCap);
    doc->SetAttribute(tag, AttrCap1Sense, m_first.secondSense ? 2 : 1);
    doc->SetAttribute(tag, AttrCap2Sense, m_second.secondSense ? 2 : 1);

    if (m_first.edgePointId != NULL_ID)
    {
        doc->SetAttribute(tag, AttrCap1Ref, m_first.edgePointId);
    }
    if (m_second.edgePointId != NULL_ID)
    {
        doc->SetAttribute(tag, AttrCap2Ref, m_second.edgePointId);
    }

    doc->SetAttribute(tag, AttrFoldLine, m_foldLine);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::ExtractPleat(const QDomElement &domElement, OToolPleatInitData &initData)
{
    initData.first.pointId = VAbstractPattern::GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
    initData.second.pointId = VAbstractPattern::GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);

    initData.value = VDomDocument::GetParametrEmptyString(domElement, AttrValue);

    initData.kind = (VAbstractPattern::GetParametrString(domElement, AttrKind, AttrKindSimple) == AttrKindDouble)
                        ? OPleatKind::Double
                        : OPleatKind::Simple;

    initData.first.showCap = VAbstractPattern::GetParametrBool(domElement, AttrCap1, trueStr);
    initData.second.showCap = VAbstractPattern::GetParametrBool(domElement, AttrCap2, trueStr);
    initData.first.secondSense = VAbstractPattern::GetParametrUInt(domElement, AttrCap1Sense, u"1"_s) == 2;
    initData.second.secondSense = VAbstractPattern::GetParametrUInt(domElement, AttrCap2Sense, u"1"_s) == 2;

    initData.first.edgePointId = VAbstractPattern::GetParametrUInt(domElement, AttrCap1Ref, NULL_ID_STR);
    initData.second.edgePointId = VAbstractPattern::GetParametrUInt(domElement, AttrCap2Ref, NULL_ID_STR);

    initData.foldLine = VAbstractPattern::GetParametrBool(domElement, AttrFoldLine, trueStr);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::MakeToolTip() const -> QString
{
    return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>"
                          "%3")
        .arg(tr("Pleat"), m_value)              // 1, 2
        .arg(VisibilityGroupToolTip());         // 3
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::RefreshDataInFile()
{
    VAbstractOperation::RefreshDataInFile();

    // An object the parse has just created — a cap that became oblique, the copy line of a prise point that stopped
    // travelling — carries an id the file must hold, exactly as the ids of the copies do: a layer names its objects by
    // id, and an id the file does not carry is one the next open would not recognize.
    QDomElement domElement = doc->FindElementById(m_id, getTagName());
    if (domElement.isElement() && not SameDisplay(OPleatDisplay::Extract(domElement), m_display))
    {
        OPleatDisplay::Save(doc, domElement, m_display);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<ODialogPleat>(oldDomElement, newDomElement);
}
