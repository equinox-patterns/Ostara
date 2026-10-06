/************************************************************************
 **
 **  @file   otoolpleat.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The pleat tool: see the header for what it is meant to build.
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
#include <QStringList>
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
const QString OToolPleat::TagPleats = QStringLiteral("pleats");
const QString OToolPleat::AttrFirstPoint = QStringLiteral("firstPoint");
const QString OToolPleat::AttrSecondPoint = QStringLiteral("secondPoint");
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
/** @brief The value of a pleat as it will show once folded, in pixels: the formula the user typed, read in the
 *  pattern's unit and turned into the unit the geometry is drawn in.
 *
 *  A formula that cannot be read is worth nothing rather than a refused pattern: the rule then says the pleat has no
 *  value, and the figure is left out, which is what a file edited by hand deserves. */
auto PleatValue(quint32 idTool, const QString &value, const VContainer *data) -> qreal
{
    try
    {
        QString formula = value; // CheckFormula may rewrite the formula it reads
        return VAbstractValApplication::VApp()->toPixel(VAbstractTool::CheckFormula(idTool, formula, data));
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        return 0.;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The value of every departure line, in pixels, in the order the lines were given. */
auto PleatValues(quint32 idTool, const QVector<OPleatDeparture> &departures, const VContainer *data) -> QVector<qreal>
{
    QVector<qreal> values;
    values.reserve(departures.size());

    for (const OPleatDeparture &departure : departures)
    {
        values.append(PleatValue(idTool, departure.value, data));
    }

    return values;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The whole plan of the pleats: how far each line was carried, and the mark of each end of each pleat. */
auto MakePlan(quint32 idTool, const QVector<OPleatDeparture> &departures, const QVector<SourceItem> &source,
              const VContainer *data) -> OPleatPlan
{
    return OPleatRule::Compose(departures, PleatValues(idTool, departures, data), source, data);
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
/** @brief One departure line, as the file holds it: the very element a recipe step carries. */
auto DepartureOf(const QDomElement &item) -> OPleatDeparture
{
    OPleatDeparture departure;

    departure.first.pointId = VAbstractPattern::GetParametrUInt(item, OToolPleat::AttrFirstPoint, NULL_ID_STR);
    departure.second.pointId = VAbstractPattern::GetParametrUInt(item, OToolPleat::AttrSecondPoint, NULL_ID_STR);

    departure.value = VDomDocument::GetParametrEmptyString(item, OToolPleat::AttrValue);

    departure.kind = (VAbstractPattern::GetParametrString(item, OToolPleat::AttrKind, OToolPleat::AttrKindSimple)
                      == OToolPleat::AttrKindDouble)
                         ? OPleatKind::Double
                         : OPleatKind::Simple;

    departure.first.showCap = VAbstractPattern::GetParametrBool(item, OToolPleat::AttrCap1, trueStr);
    departure.second.showCap = VAbstractPattern::GetParametrBool(item, OToolPleat::AttrCap2, trueStr);
    departure.first.secondSense = VAbstractPattern::GetParametrUInt(item, OToolPleat::AttrCap1Sense, u"1"_s) == 2;
    departure.second.secondSense = VAbstractPattern::GetParametrUInt(item, OToolPleat::AttrCap2Sense, u"1"_s) == 2;

    departure.first.edgePointId = VAbstractPattern::GetParametrUInt(item, OToolPleat::AttrCap1Ref, NULL_ID_STR);
    departure.second.edgePointId = VAbstractPattern::GetParametrUInt(item, OToolPleat::AttrCap2Ref, NULL_ID_STR);

    return departure;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Create every object of the figure, and the lines that join them.
 *
 *  The points come first: a line is carried by two point ids, and the operation's own copy of a point of a departure
 *  line only exists once the destination has been built. */
void CreateDisplayObjects(OToolPleatInitData &initData, const OPleatPlan &plan)
{
    OPleatDisplay::EnsureIds(initData.display, initData.departures, initData.foldLine, plan, initData.source,
                             initData.destination, initData.data);
    OPleatDisplay::Create(initData.id, initData.display, plan, initData.departures, initData.data);

    if (plan.failure != OPleatFailure::None || not initData.display.show)
    {
        return; // no figure to draw: nothing was placed, and no line may be built on a point that is not there
    }

    const auto endpoint = [&initData, &plan](int departure, int line, int end) -> quint32
    {
        return OPleatDisplay::Endpoint(initData.display, plan, initData.departures, departure, line, end,
                                       initData.source, initData.destination);
    };

    // The departure line of every pleat, and every place the folds carried it to: the same line, translated by one
    // pleat at a time. The last place of a line is its copy line — where the fabric lands — and the places between
    // the two folds of a double are the line they share.
    OOperationLine line;
    line.typeLine = initData.display.lineTypeLine;
    line.lineColor = initData.display.lineColor;

    for (int departure = 0; departure < initData.departures.size(); ++departure)
    {
        for (int place = 0; place < OPleatDisplay::LineCount(initData.departures.at(departure)); ++place)
        {
            line.result = initData.display.lines.value(OPleatDisplay::LineIndex(initData.departures, departure, place));
            OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, endpoint(departure, place, 0),
                                        endpoint(departure, place, 1), initData.parse);
        }
    }

    // The fold line of each pleat: what the pleat is pressed along, drawn from one cap to the other, and only where
    // both of them are there to be joined.
    OOperationLine fold;
    fold.typeLine = initData.display.foldTypeLine;
    fold.lineColor = initData.display.foldColor;

    if (initData.foldLine)
    {
        for (int departure = 0; departure < initData.departures.size(); ++departure)
        {
            const OPleatPlace place = plan.places.value(departure);

            for (int rank = 0; rank < place.pleats; ++rank)
            {
                const int pleat = place.firstPleat + rank;
                if (plan.Mark(pleat, 0) == nullptr || plan.Mark(pleat, 1) == nullptr)
                {
                    continue;
                }

                fold.result = initData.display.foldLines.value(pleat);
                OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, fold,
                                            MarkPoint(initData.display, pleat, 0),
                                            MarkPoint(initData.display, pleat, 1), initData.parse);
            }
        }
    }

    // The two sides of every cap: its apex, the point of the departure line it was taken on, and the same point
    // carried by the move.
    for (int i = 0; i < initData.display.marks.size() && i < plan.marks.size(); ++i)
    {
        const OPleatMarkDisplay &mark = initData.display.marks.at(i);
        const OPleatMark &resolved = plan.marks.at(i);
        const int rank = resolved.pleat - plan.places.value(resolved.departure).firstPleat;

        if (mark.side1 != NULL_ID)
        {
            line.result = mark.side1;
            OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, mark.point,
                                        endpoint(resolved.departure, rank, resolved.end), initData.parse);
        }

        if (mark.side2 != NULL_ID)
        {
            line.result = mark.side2;
            OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, mark.point,
                                        endpoint(resolved.departure, rank + 1, resolved.end), initData.parse);
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
        const OPleatCopy &one = left.copies.at(i);
        const OPleatCopy &other = right.copies.at(i);
        if (one.departure != other.departure || one.line != other.line || one.end != other.end
            || one.point != other.point || one.name != other.name)
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
    for (OPleatDeparture &departure : initData.departures)
    {
        OPleatSupport::Resolve(departure.first, initData.data);
        OPleatSupport::Resolve(departure.second, initData.data);
    }

    const OPleatPlan plan = MakePlan(initData.id, initData.departures, initData.source, initData.data);
    const OOperationDestination<OPleatTransform> destination((OPleatTransform(plan)));

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
    m_departures(initData.departures),
    m_foldLine(initData.foldLine),
    m_display(initData.display)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::PleatCount() const -> int
{
    int count = 0;

    for (const OPleatDeparture &departure : m_departures)
    {
        count += (departure.kind == OPleatKind::Double) ? 2 : 1;
    }

    return count;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::FoldLinePossible() const -> bool
{
    for (const OPleatDeparture &departure : m_departures)
    {
        if (departure.first.showCap && departure.second.showCap)
        {
            return true;
        }
    }

    return false;
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

    const OPleatPlan plan = MakePlan(m_id, m_departures, source, getData());
    const OOperationDestination<OPleatTransform> destination((OPleatTransform(plan)));

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
    dialogTool->SetPleat(m_departures, m_foldLine);
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
    initData.departures = dialogTool->GetDepartures();
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

    // The departure line of the first pleat is the fold the whole operation is taken on: it is part of what the tool
    // builds, and the layer must leave the two points where they are. The other lines are given flat, and the fabric
    // they belong to travels: they go with the objects that do.
    if (initData.typeCreation == Source::FromGui && not initData.departures.isEmpty())
    {
        initData.keptInPlace.insert(initData.departures.constFirst().first.pointId);
        initData.keptInPlace.insert(initData.departures.constFirst().second.pointId);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    // The values are formulas like any other: what they read has to be known before the copies are built.
    for (const OPleatDeparture &departure : std::as_const(initData.departures))
    {
        initData.doc->FindFormulaDependencies(departure.value, initData.id, initData.data->DataDependencyVariables());
    }

    CreateDestination(initData);

    // What the copies and the caps are read from: the points of every departure line, the points designated to carry
    // an edge, and everything that travels.
    for (const OPleatDeparture &departure : std::as_const(initData.departures))
    {
        for (const quint32 id : {departure.first.pointId, departure.second.pointId, departure.first.edgePointId,
                                 departure.second.edgePointId})
        {
            if (id != NULL_ID)
            {
                patternGraph->AddEdge(id, initData.id);
            }
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

    // The departure lines themselves are what stays: the pleats have no pivot, and no point of the pattern is left
    // in place by them.
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
void OToolPleat::SetFoldLineShown(bool show)
{
    if (m_foldLine == show)
    {
        return;
    }

    m_foldLine = show;
    SaveOptionsAndReparse(true, show ? tr("show the fold lines") : tr("hide the fold lines"));
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
    for (OPleatDeparture &departure : m_departures)
    {
        OPleatSupport::Resolve(departure.first, getData());
        OPleatSupport::Resolve(departure.second, getData());
    }

    const OPleatPlan plan = MakePlan(m_id, m_departures, source, getData());
    OPleatDisplay::EnsureIds(m_display, m_departures, m_foldLine, plan, source, m_destination, getData());
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
    // Everything the figure is made of: the departure lines and their copies, the fold lines, the copies of the points
    // the display places, and the caps. Turning a cap off, or the figure itself, takes some of them away, and only
    // this list says so to the test that chooses between a lite and a full parse.
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

        // No line is being edited here: the tool's own figure draws them all at the same light.
        visual->SetPleat(m_departures, OPleatEnd{}, m_foldLine, -1);
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

    m_departures = dialogTool->GetDepartures();
    m_foldLine = dialogTool->IsFoldLineShown();

    const OPleatDisplay dialogDisplay = dialogTool->GetDisplay();
    // The figure is not optional: it is drawn, and the dialog only decides what the fold lines look like.
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
    m_departures = initData.departures;
    m_foldLine = initData.foldLine;

    m_display = OPleatDisplay::Extract(domElement);

    // Where the points of the departure lines are, and which way the edge runs there, are never read from the file:
    // they are asked of the container again, which is what makes the figure follow a point the user moves.
    for (OPleatDeparture &departure : m_departures)
    {
        OPleatSupport::Resolve(departure.first, getData());
        OPleatSupport::Resolve(departure.second, getData());
    }
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
    doc->SetAttribute(tag, AttrFoldLine, m_foldLine);

    SaveDepartures(doc, tag, m_departures);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::SaveDepartures(VAbstractPattern *doc, QDomElement &tag,
                                const QVector<OPleatDeparture> &departures)
{
    SCASSERT(doc != nullptr)

    // The element is written whole: a departure line the user took away must not stay behind in the file.
    for (QDomElement old = tag.firstChildElement(TagPleats); not old.isNull();)
    {
        const QDomElement next = old.nextSiblingElement(TagPleats);
        tag.removeChild(old);
        old = next;
    }

    QDomElement element = doc->createElement(TagPleats);

    for (const OPleatDeparture &departure : departures)
    {
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        doc->SetAttribute(item, AttrFirstPoint, departure.first.pointId);
        doc->SetAttribute(item, AttrSecondPoint, departure.second.pointId);
        doc->SetAttribute(item, AttrValue, departure.value);
        doc->SetAttribute(item, AttrKind, departure.kind == OPleatKind::Double ? AttrKindDouble : AttrKindSimple);

        doc->SetAttribute(item, AttrCap1, departure.first.showCap);
        doc->SetAttribute(item, AttrCap2, departure.second.showCap);
        doc->SetAttribute(item, AttrCap1Sense, departure.first.secondSense ? 2 : 1);
        doc->SetAttribute(item, AttrCap2Sense, departure.second.secondSense ? 2 : 1);

        if (departure.first.edgePointId != NULL_ID)
        {
            doc->SetAttribute(item, AttrCap1Ref, departure.first.edgePointId);
        }
        if (departure.second.edgePointId != NULL_ID)
        {
            doc->SetAttribute(item, AttrCap2Ref, departure.second.edgePointId);
        }

        element.appendChild(item);
    }

    tag.appendChild(element);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::ExtractPleat(const QDomElement &domElement, OToolPleatInitData &initData)
{
    initData.foldLine = VAbstractPattern::GetParametrBool(domElement, AttrFoldLine, trueStr);
    initData.departures.clear();

    for (QDomElement element = domElement.firstChildElement(TagPleats); not element.isNull();
         element = element.nextSiblingElement(TagPleats))
    {
        for (QDomElement item = element.firstChildElement(VAbstractOperation::TagItem); not item.isNull();
             item = item.nextSiblingElement(VAbstractOperation::TagItem))
        {
            initData.departures.append(DepartureOf(item));
        }

        break; // Only the first element is read, exactly as the operation lines are: an operation has one.
    }

    if (not initData.departures.isEmpty())
    {
        return;
    }

    // A file written before a pleat could be taken on several departure lines carries one line, flat, on the
    // operation itself. Reading it is what opens those patterns, and the next save writes them the new way.
    const OPleatDeparture departure = DepartureOf(domElement);
    if (departure.first.pointId != NULL_ID || departure.second.pointId != NULL_ID)
    {
        initData.departures.append(departure);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolPleat::MakeToolTip() const -> QString
{
    QStringList values;
    values.reserve(m_departures.size());

    for (const OPleatDeparture &departure : m_departures)
    {
        values.append(departure.value);
    }

    return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>"
                          "%3")
        .arg(tr("Pleats"), values.join(QStringLiteral(", "))) // 1, 2
        .arg(VisibilityGroupToolTip());                       // 3
}

//---------------------------------------------------------------------------------------------------------------------
void OToolPleat::RefreshDataInFile()
{
    VAbstractOperation::RefreshDataInFile();

    // An object the parse has just created — a cap that became oblique, the copy of a point that stopped travelling —
    // carries an id the file must hold, exactly as the ids of the copies do: a layer names its objects by id, and an
    // id the file does not carry is one the next open would not recognize.
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
