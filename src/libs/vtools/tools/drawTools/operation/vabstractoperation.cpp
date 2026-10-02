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

#include "vabstractoperation.h"

#include <QScopeGuard>

#include "../../../undocommands/deltool.h"
#include "../../../undocommands/savetooloptions.h"
#include "../../../undocommands/undogroup.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/compatibility.h"
#include "../vwidgets/vsimplepoint.h"
#include "../vtoolline.h"
#include "vtools/undocommands/renameobject.h"

using namespace Qt::Literals::StringLiterals;

const QString VAbstractOperation::TagItem = QStringLiteral("item");
const QString VAbstractOperation::TagSource = QStringLiteral("source");
const QString VAbstractOperation::TagDestination = QStringLiteral("destination");

namespace
{
/**
 * @brief VisibilityGroupDataFromSource converts source list to visibility group list.
 *
 * A layer may only hold pairs (object, tool) the model can ADDRESS: the pair a tool enumerates for that object,
 * or — when the tool cannot name that object alone — what the tool does enumerate. A cut point is the case that
 * matters: its two segments are not addressable (`VToolSinglePoint::GroupVisibility` ignores the object it is
 * handed, a cut hides as a whole), and writing the pair anyway is far from harmless — the loading pass judges it
 * stale (`CleanStaleLayerItems`) and deletes it from the file at the next open, in silence. Measured: a rotation
 * built on one segment of a cut put `(14, 13)` in its layer, and the entry was gone after a re-open.
 *
 * @param doc pattern document
 * @param data container with pattern data
 * @param source list with source objects
 * @return visibility group data
 */
auto VisibilityGroupDataFromSource(VAbstractPattern *doc, const VContainer *data, const QVector<SourceItem> &source)
    -> QMap<quint32, quint32>
{
    QMap<quint32, quint32> groupData;

    for (const auto &sItem : source)
    {
        try
        {
            const quint32 toolId = data->GetGObject(sItem.id)->getIdTool();
            const QVector<QPair<quint32, quint32>> addressable = doc->LayerableObjectsOfTool(toolId);

            if (addressable.contains({sItem.id, toolId}))
            {
                groupData.insert(sItem.id, toolId);
                continue;
            }

            // The object itself cannot be named in a layer: what goes in is the tool that carries it, exactly
            // as the tool creation path writes it.
            for (const auto &[objectId, objectTool] : addressable)
            {
                groupData.insert(objectId, objectTool);
            }
        }
        catch (const VExceptionBadId &) // NOLINT(bugprone-empty-catch)
        {
            // ignore
        }
    }

    return groupData;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddLineCarriersToGroupData adds the objects the lines of an operation bring into its layer.
 *
 * A line has no object of its own: it is carried by two point ids and rebuilt on every parse, so the source list
 * never holds it — it travels in initData.lines. The layer still has to hold it: that is what makes the eye of a
 * layer hide a line, and what the Layers dock counts. VAbstractPattern::LayerableObjectsOfTool is the one place
 * that knows how the tool carrying a line is named in a layer — by id, since the line an earlier operation
 * produced has no element of its own to read.
 *
 * @param doc pattern document
 * @param lines the lines attached to the operation
 * @param groupData visibility group data, completed in place
 */
void AddLineCarriersToGroupData(VAbstractPattern *doc, const QVector<OOperationLine> &lines,
                                QMap<quint32, quint32> &groupData)
{
    for (const auto &line : lines)
    {
        if (line.source == NULL_ID)
        {
            continue;
        }

        const QVector<QPair<quint32, quint32>> objects = doc->LayerableObjectsOfTool(line.source);

        for (const auto &[objectId, toolId] : objects)
        {
            groupData.insert(objectId, toolId);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RemoveOriginalsFromOtherLayers takes every object this layer is built from out of the layers that held it.
 *
 * The layer an operation creates is meant to be the only layer its originals are in: the user asked for one layer
 * holding them, not for a copy of them in each. Written after the layer exists — AddGroup has already put it in
 * the document — so that at no moment does an object belong to no layer, the order the Layers dock's transfer uses.
 *
 * @param initData operation data
 * @param groupData the objects the new layer holds, by object id, with the tool that carries them
 * @param groupId the layer just created
 * @param parent the command the removals belong to, nullptr when each of them goes on the undo stack
 */
void RemoveOriginalsFromOtherLayers(const VAbstractOperationInitData &initData,
                                    const QMap<quint32, quint32> &groupData, vidtype groupId, QUndoCommand *parent)
{
    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();

    for (auto item = groupData.constBegin(); item != groupData.constEnd(); ++item)
    {
        // Every layer of the piece that holds the object, the new one included: it is the one skipped below,
        // since what is asked here is where the object used to be.
        const QMap<quint32, QString> holders = initData.doc->GetGroupsContainingItem(item.value(), item.key(), true);

        for (auto holder = holders.constBegin(); holder != holders.constEnd(); ++holder)
        {
            if (holder.key() == groupId)
            {
                continue;
            }

            if (const QDomElement removed = initData.doc->RemoveItemFromGroup(item.value(), item.key(), holder.key());
                not removed.isNull())
            {
                auto *removeItemFromGroup = new RemoveItemFromGroup(removed, initData.doc, holder.key(), parent);
                QObject::connect(removeItemFromGroup,
                                 &RemoveItemFromGroup::UpdateGroups,
                                 initData.doc,
                                 &VAbstractPattern::UpdateVisiblityGroups);
                if (parent == nullptr)
                {
                    undoStack->push(removeItemFromGroup);
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto BuildNameMap(const QVector<SourceItem> &items) -> QMap<quint32, QString>
{
    QMap<quint32, QString> names;
    for (const auto &item : items)
    {
        if (item.id != NULL_ID)
        {
            names.insert(item.id, item.name);
        }
    }
    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto BuildDestIdByRecord(const QVector<DestinationItem> &destination) -> QMap<QUuid, quint32>
{
    QMap<QUuid, quint32> destIdByRecord;
    for (const auto &dest : destination)
    {
        if (dest.id != NULL_ID && !dest.recordId.isNull())
        {
            destIdByRecord.insert(dest.recordId, dest.id);
        }
    }
    return destIdByRecord;
}

//---------------------------------------------------------------------------------------------------------------------
auto CollectRenames(const QVector<SourceItem> &newSource,
                    const QMap<quint32, QString> &oldNames,
                    const QMap<QUuid, quint32> &destIdByRecord)
    -> QVector<std::tuple<quint32, quint32, QString, QString>>
{
    QVector<std::tuple<quint32, quint32, QString, QString>> renames;

    for (const auto &newItem : newSource)
    {
        if (newItem.id == NULL_ID || newItem.recordId.isNull())
        {
            continue;
        }

        if (!oldNames.contains(newItem.id))
        {
            continue; // new record, skip
        }

        const QString &oldName = oldNames.value(newItem.id);
        if (oldName == newItem.name)
        {
            continue;
        }

        if (!destIdByRecord.contains(newItem.recordId))
        {
            continue; // no matching destination, skip
        }

        renames.append({newItem.id, destIdByRecord.value(newItem.recordId), oldName, newItem.name});
    }

    return renames;
}

//---------------------------------------------------------------------------------------------------------------------
template<typename T>
void SetupRenameCommand(T *command, bool fullParse, bool isLast, VAbstractPattern *doc)
{
    command->SetFullParse(fullParse);
    if (!isLast)
    {
        return;
    }

    if (fullParse)
    {
        QObject::connect(command, &T::NeedFullParsing, doc, &VAbstractPattern::NeedFullParsing, Qt::QueuedConnection);
    }
    else
    {
        QObject::connect(command, &T::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::getTagName() const -> QString
{
    return VAbstractPattern::TagOperation;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::IsRemovable() const -> RemoveStatus
{
    if (!doc->IsPatternGraphComplete())
    {
        return RemoveStatus::Pending; // Data not ready yet
    }

    VPatternGraph const *patternGraph = doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    // The lines this operation produced depend on its destination points. They must not make the
    // operation itself impossible to delete.
    QVector<quint32> ownLines;
    ownLines.reserve(m_lines.size());
    for (const auto &line : std::as_const(m_lines))
    {
        ownLines.append(line.result);
    }

    auto Filter = [&ownLines](const auto &node) -> auto
    {
        return node.type != VNodeType::MODELING_TOOL && node.type != VNodeType::MODELING_OBJECT
               && !ownLines.contains(node.id);
    };

    for (const auto &item : m_destination)
    {
        if (item.id == NULL_ID)
        {
            // The merge point the operation references instead of copying: it keeps its place in the list with
            // no object of its own, so it has no dependents to ask about. Asking anyway cost the whole answer:
            // a single failed attempt on this entry — the first one, and the one that means nothing — turned
            // the status into "still collecting", and the Delete action stayed greyed out with "(Pending)" on
            // a pattern whose dependencies had long settled.
            continue;
        }

        // 500 ms, like the other tools that ask the same question.
        auto const dependecies = patternGraph->TryGetDependentNodes(item.id, 500, Filter);

        if (!dependecies)
        {
            return RemoveStatus::Pending; // Lock timeout
        }

        if (!dependecies->isEmpty())
        {
            return RemoveStatus::Blocked;
        }
    }

    return RemoveStatus::Removable;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::SetNotes(const QString &notes)
{
    m_notes = notes;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::SourceItems() const -> QVector<SourceItem>
{
    return m_source;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::CopyStyleOf(quint32 objectId) const -> CopyStyle
{
    if (objectId == NULL_ID)
    {
        return {};
    }

    // A copied curve: the pen it is DRAWN with, read on the object the container holds — a copy inherits the pen
    // of the object it comes from, so the raw attributes of its `<source>` item say `default` where the user sees
    // a style and a colour. This is also where the "Pen style:" property of a normal curve reads it, so the two
    // cannot answer differently. The two lists are read positionally — destination[i] is the image of source[i].
    for (int i = 0; i < m_destination.size() && i < m_source.size(); ++i)
    {
        if (m_destination.at(i).id != objectId)
        {
            continue;
        }

        GOType type = GOType::Unknown;
        try
        {
            type = VAbstractTool::data.GetGObject(objectId)->getType();
        }
        catch (const VExceptionBadId &)
        {
            return {};
        }

        // A copied point carries no pen at all (`CreatePoint` and `UpdatePoint` have nothing to give one to), so
        // nothing is offered for it: an entry that would be written and never read is worse than none.
        if (type == GOType::Point)
        {
            return {};
        }

        try
        {
            const QSharedPointer<VAbstractCurve> curve =
                VAbstractTool::data.GeometricObject<VAbstractCurve>(objectId);

            return {.penStyle = curve->GetPenStyle(),
                    .color = curve->GetColor(),
                    .type = type,
                    .isLine = false,
                    .exists = true};
        }
        catch (const VExceptionBadId &)
        {
            return {};
        }
    }

    // A copied line: a tool of its own, whose type and colour the operation keeps beside its id.
    for (const auto &line : m_lines)
    {
        if (line.result != objectId)
        {
            continue;
        }

        return {.penStyle = line.typeLine, .color = line.lineColor, .isLine = true, .exists = true};
    }

    // A copied point carries no pen, and an object this operation did not produce is none of its business.
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::SetCopyStyle(quint32 objectId, const QString &penStyle, const QString &color)
{
    // The very question the reader answers — does this operation keep a pen for that object, and on which list?
    // Asked here too, so a value nobody would ever read is never written.
    const CopyStyle current = CopyStyleOf(objectId);
    if (!current.exists || (current.penStyle == penStyle && current.color == color))
    {
        return;
    }

    if (current.isLine)
    {
        for (auto &line : m_lines)
        {
            if (line.result == objectId)
            {
                line.typeLine = penStyle;
                line.lineColor = color;
                break;
            }
        }
    }
    else
    {
        for (int i = 0; i < m_destination.size() && i < m_source.size(); ++i)
        {
            if (m_destination.at(i).id == objectId)
            {
                m_source[i].penStyle = penStyle;
                m_source[i].color = color;
                break;
            }
        }
    }

    // Saved the way every tool option is: the element is cloned, the write is a command of its own, and the line
    // parse that follows rebuilds the copies from what the element now says.
    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::GroupVisibility(quint32 object, bool visible)
{
    if (!operatedObjects.contains(object))
    {
        return;
    }

    if (VAbstractSimple *obj = operatedObjects.value(object); (obj != nullptr) && obj->GetType() == GOType::Point)
    {
        auto *item = qobject_cast<VSimplePoint *>(obj);
        SCASSERT(item != nullptr)
        item->setVisible(visible);
    }
    else
    {
        auto *item = qobject_cast<VSimpleCurve *>(obj);
        SCASSERT(item != nullptr)
        item->setVisible(visible);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(painter)
    Q_UNUSED(option)
    Q_UNUSED(widget)
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::ChangeLabelPosition(quint32 id, const QPointF &pos)
{
    if (!operatedObjects.contains(id))
    {
        return;
    }

    if (VAbstractSimple *obj = operatedObjects.value(id); (obj != nullptr) && obj->GetType() == GOType::Point)
    {
        auto *item = qobject_cast<VSimplePoint *>(obj);
        SCASSERT(item != nullptr)
        QSharedPointer<VPointF> const point = VAbstractTool::data.GeometricObject<VPointF>(id);
        point->setMx(pos.x());
        point->setMy(pos.y());
        item->RefreshPointGeometry(*(point.data()));

        if (QGraphicsScene *sc = item->scene())
        {
            VMainGraphicsView::NewSceneRect(sc, VAbstractValApplication::VApp()->getSceneView(), item);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::IsLabelVisible(quint32 id) const -> bool
{
    if (!operatedObjects.contains(id))
    {
        return false;
    }

    if (const VAbstractSimple *obj = operatedObjects.value(id); (obj != nullptr) && obj->GetType() == GOType::Point)
    {
        return VAbstractTool::data.GeometricObject<VPointF>(id)->IsShowLabel();
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::SetLabelVisible(quint32 id, bool visible)
{
    if (!operatedObjects.contains(id))
    {
        return;
    }

    if (VAbstractSimple *obj = operatedObjects.value(id); (obj != nullptr) && obj->GetType() == GOType::Point)
    {
        auto *item = qobject_cast<VSimplePoint *>(obj);
        SCASSERT(item != nullptr)
        const QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(id);
        point->SetShowLabel(visible);
        item->RefreshPointGeometry(*point);
        if (QGraphicsScene *sc = item->scene())
        {
            VMainGraphicsView::NewSceneRect(sc, VAbstractValApplication::VApp()->getSceneView(), item);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::ExtractData(const QDomElement &domElement, VAbstractOperationInitData &initData)
{
    initData.source = ExtractSourceData(domElement);
    initData.destination = ExtractDestinationData(domElement);
    initData.lines = OOperationLines::ExtractLines(domElement);

    if (initData.source.size() != initData.destination.size())
    {
        // The mismatch is handled below by ignoring the extra ones; it must not warn, or the message would be
        // raised as a modal box from inside an undo/redo parse (see VDrawTool::GetItemType).
        qCDebug(vTool) << "Source and destination object count mismatch:" << initData.source.size() << "vs"
                       << initData.destination.size();
    }

    for (int i = 0; i < initData.source.size() && i < initData.destination.size(); ++i)
    {
        initData.destination[i].recordId = initData.source.at(i).recordId;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::ExtractSourceData(const QDomElement &domElement) -> QVector<SourceItem>
{
    QVector<SourceItem> source;
    const QDomNodeList nodeList = domElement.childNodes();
    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement dataElement = QDOM_ELEMENT(nodeList, i).toElement();
            not dataElement.isNull() && dataElement.tagName() == TagSource)
        {
            const QDomNodeList srcList = dataElement.childNodes();
            QDOM_LOOP(srcList, j)
            {
                if (const QDomElement element = QDOM_ELEMENT(srcList, j).toElement(); not element.isNull())
                {
                    // We no longer need to handle alias attribute here. The code can be removed.
                    // Name must be mandatory
                    Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
                    const QString alias = VDomDocument::GetParametrEmptyString(element, AttrAlias);
                    QString name = VDomDocument::GetParametrEmptyString(element, AttrName);
                    if (name.isEmpty() && !alias.isEmpty())
                    {
                        name = alias;
                    }

                    source.append({.id = VDomDocument::GetParametrUInt(element, AttrIdObject, NULL_ID_STR),
                                   .name = name,
                                   .penStyle = VDomDocument::GetParametrString(element, AttrPenStyle, TypeLineDefault),
                                   .color = VDomDocument::GetParametrString(element, AttrColor, ColorDefault)});
                }
            }
            return source;
        }
    }

    return source;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::ExtractDestinationData(const QDomElement &domElement) -> QVector<DestinationItem>
{
    QVector<DestinationItem> destination;
    const QDomNodeList nodeList = domElement.childNodes();
    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement dataElement = QDOM_ELEMENT(nodeList, i).toElement();
            not dataElement.isNull() && dataElement.tagName() == TagDestination)
        {
            const QDomNodeList srcList = dataElement.childNodes();
            QDOM_LOOP(srcList, j)
            {
                if (const QDomElement element = QDOM_ELEMENT(srcList, j).toElement(); not element.isNull())
                {
                    destination.append({.id = VDomDocument::GetParametrUInt(element, AttrIdObject, NULL_ID_STR),
                                        .mx = VAbstractValApplication::VApp()->toPixel(
                                            VDomDocument::GetParametrDouble(element, AttrMx, QChar('1'))),
                                        .my = VAbstractValApplication::VApp()->toPixel(
                                            VDomDocument::GetParametrDouble(element, AttrMy, QChar('1'))),
                                        .showLabel = VDomDocument::GetParametrBool(element, AttrShowLabel, falseStr)});
                }
            }

            return destination;
        }
    }

    return destination;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::OperationColorsList() -> QMap<QString, QString>
{
    QMap<QString, QString> list = VAbstractTool::ColorsList();
    list.insert(ColorDefault, QCoreApplication::translate("VAbstractOperation", "default"));
    return list;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::FullUpdateFromFile()
{
    ReadAttributes();
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->setToolTip(ComplexPointToolTip(i.key()));
            item->RefreshPointGeometry(*VAbstractTool::data.GeometricObject<VPointF>(i.key()));
        }
        else
        {
            auto *item = qobject_cast<VSimpleCurve *>(i.value());
            SCASSERT(item != nullptr)
            item->setToolTip(ComplexCurveToolTip(i.key()));
            item->RefreshGeometry(VAbstractTool::data.GeometricObject<VAbstractCurve>(i.key()));
        }
    }
    SetVisualization();
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowHover(bool enabled)
{
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->setAcceptHoverEvents(enabled);
        }
        else
        {
            auto *item = qobject_cast<VSimpleCurve *>(i.value());
            SCASSERT(item != nullptr)
            item->setAcceptHoverEvents(enabled);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowSelecting(bool enabled)
{
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
        }
        else
        {
            auto *item = qobject_cast<VSimpleCurve *>(i.value());
            SCASSERT(item != nullptr)
            item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::EnableToolMove(bool move)
{
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->EnableToolMove(move);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowPointHover(bool enabled)
{
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->setAcceptHoverEvents(enabled);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowPointSelecting(bool enabled)
{
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowPointLabelHover(bool enabled)
{
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->AllowLabelHover(enabled);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowPointLabelSelecting(bool enabled)
{
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->AllowLabelSelecting(enabled);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowSplineHover(bool enabled)
{
    AllowCurveHover(enabled, GOType::Spline);
    AllowCurveHover(enabled, GOType::CubicBezier);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowSplineSelecting(bool enabled)
{
    AllowCurveSelecting(enabled, GOType::Spline);
    AllowCurveSelecting(enabled, GOType::CubicBezier);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowSplinePathHover(bool enabled)
{
    AllowCurveHover(enabled, GOType::SplinePath);
    AllowCurveHover(enabled, GOType::CubicBezierPath);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowSplinePathSelecting(bool enabled)
{
    AllowCurveSelecting(enabled, GOType::SplinePath);
    AllowCurveSelecting(enabled, GOType::CubicBezierPath);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowArcHover(bool enabled)
{
    AllowCurveHover(enabled, GOType::Arc);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowArcSelecting(bool enabled)
{
    AllowCurveSelecting(enabled, GOType::Arc);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowElArcHover(bool enabled)
{
    AllowCurveHover(enabled, GOType::EllipticalArc);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowElArcSelecting(bool enabled)
{
    AllowCurveSelecting(enabled, GOType::EllipticalArc);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::ToolSelectionType(const SelectionType &selectionType)
{
    VAbstractTool::ToolSelectionType(selectionType);
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->ToolSelectionType(selectionType);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::Enable()
{
    const bool enabled = m_indexActivePatternBlock == doc->PatternBlockMapper()->GetActiveId();
    setEnabled(enabled);

    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() == GOType::Point)
        {
            auto *item = qobject_cast<VSimplePoint *>(i.value());
            SCASSERT(item != nullptr)
            item->SetEnabled(enabled);
        }
        else
        {
            auto *item = qobject_cast<VSimpleCurve *>(i.value());
            SCASSERT(item != nullptr)
            item->setEnabled(enabled);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::ObjectSelected(bool selected, quint32 objId)
{
    emit ChangedToolSelection(selected, objId, m_id);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::DeleteFromLabel()
{
    try
    {
        DeleteToolWithConfirm();
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::LabelChangePosition(const QPointF &pos, quint32 labelId)
{
    if (!operatedObjects.contains(labelId))
    {
        return;
    }

    if (VAbstractSimple *obj = operatedObjects.value(labelId); (obj != nullptr) && obj->GetType() == GOType::Point)
    {
        auto *item = qobject_cast<VSimplePoint *>(obj);
        SCASSERT(item != nullptr)
        UpdateNamePosition(labelId, pos - item->pos());
    }
}

//---------------------------------------------------------------------------------------------------------------------
VAbstractOperation::VAbstractOperation(const VAbstractOperationInitData &initData, QGraphicsItem *parent)
  : VDrawTool(initData.doc, initData.data, initData.id, initData.notes),
    QGraphicsLineItem(parent),
    m_source(initData.source),
    m_destination(initData.destination),
    m_lines(initData.lines)
{
    connect(initData.doc,
            &VAbstractPattern::UpdateToolTip,
            this,
            [this]() -> void
            {
                QMapIterator i(operatedObjects);
                while (i.hasNext())
                {
                    i.next();
                    if (i.value()->GetType() == GOType::Point)
                    {
                        auto *item = qobject_cast<VSimplePoint *>(i.value());
                        SCASSERT(item != nullptr)
                        item->setToolTip(ComplexPointToolTip(i.key()));
                    }
                    else
                    {
                        auto *item = qobject_cast<VSimpleCurve *>(i.value());
                        SCASSERT(item != nullptr)
                        item->setToolTip(ComplexCurveToolTip(i.key()));
                    }
                }
            });
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AddToFile()
{
    QDomElement domElement = doc->createElement(getTagName());
    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOptions(domElement, obj);
    AddToCalculation(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::RefreshDataInFile()
{
    // We no longer need to handle suffix and names here. The code can be removed.
    Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
    QDomElement domElement = doc->FindElementById(m_id, getTagName());
    if (domElement.hasAttribute(AttrSuffix))
    {
        domElement.removeAttribute(AttrSuffix);
        SaveSourceDestination(domElement, m_source, m_destination);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::ChangeLabelVisibility(quint32 id, bool visible)
{
    if (!operatedObjects.contains(id))
    {
        return;
    }

    if (const VAbstractSimple *obj = operatedObjects.value(id); (obj != nullptr) && obj->GetType() == GOType::Point)
    {
        if (auto dItem = std::find_if(m_destination.begin(),
                                      m_destination.end(),
                                      [id](const DestinationItem &dItem) -> bool { return dItem.id == id; });
            dItem != m_destination.end())
        {
            dItem->showLabel = visible;
        }
        SetLabelVisible(id, visible);
    }
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::SaveCommonDialogData(QDomElement &domElement, const QString &notes) const
{
    doc->SetAttributeOrRemoveIf<QString>(domElement,
                                         AttrNotes,
                                         notes,
                                         [](const QString &value) noexcept -> bool { return value.isEmpty(); });

    // We no longer need to handle suffix attribute here. The code can be removed.
    Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
    if (domElement.hasAttribute(AttrSuffix))
    {
        domElement.removeAttribute(AttrSuffix);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::SyncDestinationPreamble(const QVector<SourceItem> &source, bool &unchanged) const
    -> QMap<QUuid, DestinationItem>
{
    // Build lookup of existing destination items by recordId
    QMap<QUuid, DestinationItem> existingByRecord;
    for (const auto &dest : std::as_const(m_destination))
    {
        if (!dest.recordId.isNull())
        {
            existingByRecord.insert(dest.recordId, dest);
        }
    }

    // Check if source set changed (additions or removals)
    const QSet<QUuid> oldRecords = ConvertToSet<QUuid>(existingByRecord.keys());
    QSet<QUuid> newRecords;
    for (const auto &item : std::as_const(source))
    {
        if (!item.recordId.isNull())
        {
            newRecords.insert(item.recordId);
        }
    }

    unchanged = (oldRecords == newRecords);
    return existingByRecord;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    auto *newGroup = new QUndoCommand(); // an empty command
    newGroup->setText(tr("save tool options"));

    const auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id, newGroup);
    connect(saveOptions, &SaveToolOptions::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);

    HandleVisibilityGroupChange(newGroup);

    VAbstractApplication::VApp()->getUndoStack()->push(newGroup);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::CreateDestinationLines(const VAbstractOperationInitData &initData,
                                               const QVector<quint32> &fixedPoints,
                                               const QHash<quint32, quint32> &referencedPoints)
{
    if (initData.lines.isEmpty())
    {
        return;
    }

    // Every parse, not only the full one. A transformed line has no <line> element, so the parse never
    // re-reads it: without this call the Line_ and AngleLine_ variables it owns — which every parse clears —
    // would stay missing until the file is reopened. CreateLines builds it on the full parse and refreshes
    // it on the others, the same two cases a normal line goes through (see VToolLine::Create).
    OOperationLines::CreateLines(initData.doc, initData.data, initData.scene, initData.lines, initData.source,
                                 initData.destination, fixedPoints, initData.parse, referencedPoints);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::PerformDelete()
{
    vidtype const group = doc->GroupLinkedToTool(m_id);
    bool const deleteGroup = group != null_id;

    qCDebug(vTool, "Begin deleting.");
    if (deleteGroup)
    {
        VAbstractApplication::VApp()->getUndoStack()->beginMacro(tr("delete operation"));

        // The layer the operation created is let go, not deleted: it holds the objects the operation was built
        // from, which existed before the tool and outlive it. Deleting them with the tool would leave them in no
        // layer at all — the state the layers exist to prevent. Only the link goes, so the layer becomes an
        // ordinary layer of its piece, holding what the user put in it.
        qCDebug(vTool, "Unlinking the group of the operation.");
        auto *changeGroupTool = new ChangeGroupTool(doc, group, null_id);
        connect(changeGroupTool, &ChangeGroupTool::UpdateGroups, doc, &VAbstractPattern::UpdateVisiblityGroups);
        VAbstractApplication::VApp()->getUndoStack()->push(changeGroupTool);
    }

    qCDebug(vTool, "Deleting the tool.");
    auto *delTool = new DelTool(doc, m_id);
    connect(delTool, &DelTool::NeedFullParsing, doc, &VAbstractPattern::NeedFullParsing);
    VAbstractApplication::VApp()->getUndoStack()->push(delTool);

    if (deleteGroup)
    {
        VAbstractApplication::VApp()->getUndoStack()->endMacro();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::ReadToolAttributes(const QDomElement &domElement)
{
    VDrawTool::ReadToolAttributes(domElement);

    QVector<SourceItem> source = ExtractSourceData(domElement);
    for (auto &item : source)
    {
        auto it = std::find_if(m_source.cbegin(),
                               m_source.cend(),
                               [&item](const SourceItem &s) { return s.id == item.id; });
        if (it != m_source.cend())
        {
            item.recordId = it->recordId;
        }
    }
    m_source = source;

    m_destination = ExtractDestinationData(domElement);

    if (m_source.size() != m_destination.size())
    {
        // Same as above: the extra objects are ignored, and a warning here would open a modal box in the middle
        // of a parse.
        qCDebug(vTool) << "Source and destination object count mismatch:" << m_source.size() << "vs"
                       << m_destination.size();
    }

    for (int i = 0; i < m_source.size() && i < m_destination.size(); ++i)
    {
        m_destination[i].recordId = m_source.at(i).recordId;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VDrawTool::SaveOptions(tag, obj);

    // We no longer need to handle suffix attribute here. The code can be removed.
    Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
    if (tag.hasAttribute(AttrSuffix))
    {
        tag.removeAttribute(AttrSuffix);
    }

    SaveSourceDestination(tag, m_source, m_destination);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::UpdateNamePosition(quint32 id, const QPointF &pos)
{
    if (!operatedObjects.contains(id))
    {
        return;
    }

    if (const VAbstractSimple *obj = operatedObjects.value(id); (obj != nullptr) && obj->GetType() == GOType::Point)
    {
        if (auto dItem = std::find_if(m_destination.begin(),
                                      m_destination.end(),
                                      [id](const DestinationItem &dItem) -> bool { return dItem.id == id; });
            dItem != m_destination.end())
        {
            dItem->mx = pos.x();
            dItem->my = pos.y();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::SaveSourceDestination(QDomElement &tag,
                                               const QVector<SourceItem> &source,
                                               const QVector<DestinationItem> &destination)
{
    VAbstractPattern::RemoveAllChildren(tag);

    QDomElement tagObjects = doc->createElement(TagSource);
    for (const auto &sItem : std::as_const(source))
    {
        QDomElement item = doc->createElement(TagItem);
        doc->SetAttribute(item, AttrIdObject, sItem.id);
        doc->SetAttribute(item, AttrName, sItem.name);
        doc->SetAttributeOrRemoveIf<QString>(item,
                                             AttrPenStyle,
                                             sItem.penStyle,
                                             [](const QString &penStyle) noexcept -> bool
                                             { return penStyle == TypeLineDefault; });
        doc->SetAttributeOrRemoveIf<QString>(item,
                                             AttrColor,
                                             sItem.color,
                                             [](const QString &color) noexcept -> bool
                                             { return color == ColorDefault; });
        tagObjects.appendChild(item);
    }
    tag.appendChild(tagObjects);

    tagObjects = doc->createElement(TagDestination);
    for (const auto &dItem : std::as_const(destination))
    {
        QDomElement item = doc->createElement(TagItem);
        doc->SetAttribute(item, AttrIdObject, dItem.id);

        if (dItem.id == NULL_ID)
        {
            // The source is referenced, not copied — the merge point of a transfer. The entry is written all
            // the same, with no object: the two lists are read positionally, so dropping it would pair every
            // following source with the wrong image on the next open. It carries no label either.
            tagObjects.appendChild(item);
            continue;
        }

        VAbstractSimple const *obj = operatedObjects.value(dItem.id, nullptr);

        doc->SetAttributeOrRemoveIf<double>(item,
                                            AttrMx,
                                            VAbstractValApplication::VApp()->fromPixel(dItem.mx),
                                            [obj](double value) noexcept -> bool
                                            {
                                                Q_UNUSED(value)
                                                return obj && obj->GetType() != GOType::Point;
                                            });

        doc->SetAttributeOrRemoveIf<double>(item,
                                            AttrMy,
                                            VAbstractValApplication::VApp()->fromPixel(dItem.my),
                                            [obj](double value) noexcept -> bool
                                            {
                                                Q_UNUSED(value)
                                                return obj && obj->GetType() != GOType::Point;
                                            });

        doc->SetAttributeOrRemoveIf<bool>(item,
                                          AttrShowLabel,
                                          dItem.showLabel,
                                          [](bool showLabel) noexcept -> bool { return not showLabel; });

        tagObjects.appendChild(item);
    }
    tag.appendChild(tagObjects);

    // The transformed lines belong to the same element as the objects they are built from. Written here
    // rather than in SaveOptions because SaveDialog calls this method too, and it drops every child of the
    // element: writing them only in SaveOptions would lose them on any dialog edit.
    OOperationLines::SaveLines(doc, tag, m_lines);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::InitCurve(quint32 id, VContainer *data, GOType curveType, SceneObject sceneType)
{
    const QSharedPointer<VAbstractCurve> initCurve = data->GeometricObject<VAbstractCurve>(id);
    auto *curve = new VSimpleCurve(id, initCurve);
    curve->setParentItem(this);
    curve->SetType(curveType);
    curve->setToolTip(ComplexCurveToolTip(id));
    connect(curve, &VSimpleCurve::Selected, this, &VAbstractOperation::ObjectSelected);
    connect(curve,
            &VSimpleCurve::ShowContextMenu,
            this,
            [this](QGraphicsSceneContextMenuEvent *event, quint32 id) -> void { ShowContextMenu(event, id); });
    connect(curve,
            &VSimpleCurve::Choosed,
            this,
            [this, sceneType](quint32 id) -> void { emit ChoosedTool(id, sceneType); });
    connect(curve, &VSimpleCurve::Delete, this, &VAbstractOperation::DeleteFromLabel);
    curve->RefreshGeometry(VAbstractTool::data.GeometricObject<VAbstractCurve>(id));
    operatedObjects.insert(id, curve);
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowCurveHover(bool enabled, GOType type)
{
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() != GOType::Point)
        {
            auto *item = qobject_cast<VSimpleCurve *>(i.value());
            SCASSERT(item != nullptr)
            if (item->GetType() == type)
            {
                item->setAcceptHoverEvents(enabled);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::AllowCurveSelecting(bool enabled, GOType type)
{
    QMapIterator i(operatedObjects);
    while (i.hasNext())
    {
        i.next();
        if (i.value()->GetType() != GOType::Point)
        {
            auto *item = qobject_cast<VSimpleCurve *>(i.value());
            SCASSERT(item != nullptr)
            if (item->GetType() == type)
            {
                item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::NeedUpdateVisibilityGroup() const -> bool
{
    vidtype const group = doc->GroupLinkedToTool(m_id);

    if (m_hasLinkedGroup)
    {
        if (group != null_id)
        {
            if (m_groupName != doc->GetGroupName(group) || m_groupTags != doc->GetGroupTags(group))
            {
                return true; // rename group
            }
        }
        else
        {
            return true; // create group
        }
    }
    else
    {
        return group != null_id; // remove group
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::HandleVisibilityGroupChange(QUndoCommand *parent)
{
    if (!NeedUpdateVisibilityGroup())
    {
        return;
    }

    SCASSERT(parent != nullptr)

    vidtype const group = doc->GroupLinkedToTool(m_id);

    if (m_hasLinkedGroup)
    {
        if (group != null_id)
        {
            // The layer's pen is edited from the Layers dock, not here: the values are read back so that this
            // command leaves them untouched.
            const auto *groupOptions = new ChangeGroupOptions(doc, group, m_groupName, m_groupTags,
                                                              doc->GetGroupColor(group), doc->GetGroupPenStyle(group),
                                                              parent);
            connect(groupOptions, &ChangeGroupOptions::UpdateGroups, doc, &VAbstractPattern::UpdateVisiblityGroups);
        }
        else
        {
            VAbstractOperationInitData initData;
            initData.id = m_id;
            initData.hasLinkedVisibilityGroup = m_hasLinkedGroup;
            initData.visibilityGroupName = m_groupName;
            initData.visibilityGroupTags = m_groupTags;
            initData.data = &(VDataTool::data);
            initData.doc = doc;
            initData.source = m_source;
            initData.lines = m_lines;

            VAbstractOperation::CreateVisibilityGroup(initData, parent);
        }
    }
    else
    {
        // The layer is let go, not deleted: it holds the objects the operation was built from, which existed
        // before the tool and outlive it. Deleting them with the layer would leave them in no layer at all.
        const auto *changeGroupTool = new ChangeGroupTool(doc, group, null_id, parent);
        connect(changeGroupTool, &ChangeGroupTool::UpdateGroups, doc, &VAbstractPattern::UpdateVisiblityGroups);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::InitOperatedObjects()
{
    for (const auto &object : std::as_const(m_destination))
    {
        if (object.id == NULL_ID)
        {
            // A source the operation references instead of copying — the merge point of a transfer — has no
            // image of its own: there is nothing to show, select or move for it.
            continue;
        }

        const QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(object.id);

        // This check helps to find missed objects in the switch
        Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects were handled.");

        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wswitch-default")
        QT_WARNING_DISABLE_CLANG("-Wswitch-default")

        switch (obj->getType())
        {
            case GOType::Point:
            {
                auto *point = new VSimplePoint(object.id, VColorRole::PatternColor);
                point->setParentItem(this);
                point->SetType(GOType::Point);
                point->setToolTip(ComplexPointToolTip(object.id));
                connect(point,
                        &VSimplePoint::Choosed,
                        this,
                        [this](quint32 id) -> void { emit ChoosedTool(id, SceneObject::Point); });
                connect(point, &VSimplePoint::Selected, this, &VAbstractOperation::ObjectSelected);
                connect(point,
                        &VSimplePoint::ShowContextMenu,
                        this,
                        [this](QGraphicsSceneContextMenuEvent *event, quint32 id) -> void
                        { ShowContextMenu(event, id); });
                connect(point, &VSimplePoint::Delete, this, &VAbstractOperation::DeleteFromLabel);
                connect(point, &VSimplePoint::NameChangedPosition, this, &VAbstractOperation::LabelChangePosition);
                point->RefreshPointGeometry(*VAbstractTool::data.GeometricObject<VPointF>(object.id));
                operatedObjects.insert(object.id, point);
                break;
            }
            case GOType::Arc:
                InitCurve(object.id, &(VAbstractTool::data), obj->getType(), SceneObject::Arc);
                break;
            case GOType::EllipticalArc:
                InitCurve(object.id, &(VAbstractTool::data), obj->getType(), SceneObject::ElArc);
                break;
            case GOType::Spline:
            case GOType::CubicBezier:
                InitCurve(object.id, &(VAbstractTool::data), obj->getType(), SceneObject::Spline);
                break;
            case GOType::SplinePath:
            case GOType::CubicBezierPath:
                InitCurve(object.id, &(VAbstractTool::data), obj->getType(), SceneObject::SplinePath);
                break;
            case GOType::PlaceLabel:
            case GOType::Unknown:
                Q_UNREACHABLE();
                break;
        }

        QT_WARNING_POP
    }

    // A transformed line is a result of this operation, not an object of its own: it is drawn as a child
    // of the operation item, exactly like a destination point. Clicking it therefore opens the operation's
    // options and shows the whole result (see VToolOptionsPropertyBrowser::ShowItemOptions).
    for (const auto &line : std::as_const(m_lines))
    {
        if (auto *lineItem = qobject_cast<VToolLine *>(VAbstractPattern::FindTool(line.result)))
        {
            lineItem->setParentItem(this);
            // Asking to delete a result line means deleting this operation, exactly like a destination point
            // (see DeleteFromLabel).
            connect(lineItem, &VToolLine::DeleteRequested, this, &VAbstractOperation::DeleteFromLabel);
            // A result line has no context menu of its own: the right click opens this operation's menu,
            // the same way a destination point does above.
            connect(lineItem,
                    &VToolLine::ContextMenuRequested,
                    this,
                    [this](QGraphicsSceneContextMenuEvent *event, quint32 id) -> void
                    { ShowContextMenu(event, id); });
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::ComplexPointToolTip(quint32 itemId) const -> QString
{
    return QStringLiteral("<table>"
                          "<tr> <td><b>%1:</b> %2</td> </tr>"
                          "<tr> <td colspan=\"100%\"><hr/></td> </tr>"
                          "%3"
                          "</table>")
        .arg(tr("Label"), VAbstractTool::data.GetGObject(itemId)->name(), MakeToolTip());
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::ComplexCurveToolTip(quint32 itemId) const -> QString
{
    const QSharedPointer<VAbstractCurve> curve = VAbstractTool::data.GeometricObject<VAbstractCurve>(itemId);

    const QString toolTip = u"<table>"
                            u"<tr> <td><b>%1:</b> %2</td> </tr>"
                            u"<tr> <td><b>%3:</b> %4 %5</td> </tr>"
                            u"<tr> <td colspan=\"100%\"><hr/></td> </tr>"
                            u"%6"
                            u"</table>"_s.arg(tr("Label"), curve->ObjectName(), tr("Length"))
                                .arg(RoundForDisplay(VAbstractValApplication::VApp()->fromPixel(curve->GetLength())))
                                .arg(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true), MakeToolTip());
    return toolTip;
}

//---------------------------------------------------------------------------------------------------------------------
auto VAbstractOperation::VisibilityGroupToolTip() const -> QString
{
    vidtype const group = doc->GroupLinkedToTool(m_id);
    if (group != null_id)
    {
        return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>")
            .arg(tr("Layer"), doc->GetGroupName(group)); // 1, 2
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::CreateVisibilityGroup(const VAbstractOperationInitData &initData, QUndoCommand *parent)
{
    if (not initData.hasLinkedVisibilityGroup && not initData.visibilityGroupName.isEmpty())
    {
        return;
    }

    QMap<quint32, quint32> groupData = VisibilityGroupDataFromSource(initData.doc, initData.data, initData.source);
    AddLineCarriersToGroupData(initData.doc, initData.lines, groupData);

    vidtype const groupId = initData.data->getNextId();

    if (const QDomElement group = initData.doc->CreateGroup(groupId, initData.visibilityGroupName,
                                                            initData.visibilityGroupTags, groupData, initData.id);
        not group.isNull())
    {
        auto *addGroup = new AddGroup(group, initData.doc, parent);
        connect(addGroup, &AddGroup::UpdateGroups, initData.doc, &VAbstractPattern::UpdateVisiblityGroups);
        if (parent == nullptr)
        {
            VAbstractApplication::VApp()->getUndoStack()->push(addGroup);
        }

        RemoveOriginalsFromOtherLayers(initData, groupData, groupId, parent);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::PrepareNames(VAbstractOperationInitData &initData)
{
    // We no longer need to handle suffix attribute here. The code can be removed.
    Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
    if (initData.suffix.isEmpty())
    {
        return;
    }

    for (int i = 0; i < initData.source.size(); ++i)
    {
        SourceItem &object = initData.source[i];

        if (!object.name.isEmpty())
        {
            continue;
        }

        if (const QSharedPointer<VGObject> obj = initData.data->GetGObject(object.id); obj->getType() == GOType::Point)
        {
            object.name = obj->name() + initData.suffix;
        }
        else
        {
            QSharedPointer<VAbstractCurve> const curve = initData.data->GeometricObject<VAbstractCurve>(object.id);
            object.name = curve->HeadlessName() + initData.suffix;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VAbstractOperation::ProcessOperationToolOptions(const QDomElement &oldDomElement,
                                                     const QDomElement &newDomElement,
                                                     const QVector<SourceItem> &newSource)
{
    // Build maps for efficient lookup by id
    const QMap<quint32, QString> oldNames = BuildNameMap(m_source);
    const QMap<quint32, QString> newNames = BuildNameMap(newSource);

    // Detect additions or removals of sources by comparing id sets.
    const QSet<quint32> oldIds = ConvertToSet<quint32>(oldNames.keys());
    const QSet<quint32> newIds = ConvertToSet<quint32>(newNames.keys());
    const bool sourceSetChanged = (oldIds != newIds);

    // Detect additions or removals of destinations the same way: a slash-and-spread toggling its copies keeps
    // the same sources and names, yet its destination objects must be created or deleted — which only a full
    // parse does. The id set says it even when the names do not change.
    const auto destinationIds = [](const QDomElement &element) -> QSet<quint32>
    {
        QSet<quint32> ids;
        for (const auto &item : VAbstractOperation::ExtractDestinationData(element))
        {
            ids.insert(item.id);
        }
        return ids;
    };
    const bool destinationSetChanged = (destinationIds(oldDomElement) != destinationIds(newDomElement));
    const bool needsFullParse = sourceSetChanged || destinationSetChanged;

    // Check if there are any name changes
    if (oldNames == newNames && !destinationSetChanged)
    {
        VAbstractOperation::ApplyToolOptions(oldDomElement, newDomElement);
        return;
    }

    const QMap<QUuid, quint32> destIdByRecord = BuildDestIdByRecord(m_destination);
    const auto renames = CollectRenames(newSource, oldNames, destIdByRecord);

    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    auto *newGroup = new QUndoCommand(); // an empty command
    newGroup->setText(tr("save tool options"));

    auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id, newGroup);
    saveOptions->SetInGroup(!renames.isEmpty());
    SetupRenameCommand(saveOptions, needsFullParse, /*isLast=*/true, doc);

    HandleVisibilityGroupChange(newGroup);

    // Process rename operations, connecting signal only for the last one
    for (int i = 0; i < renames.size(); ++i)
    {
        const auto &[sourceId, destId, oldName, newName] = renames.at(i);
        const bool isLast = (i == renames.size() - 1);

        if (const QSharedPointer<VGObject> obj = VDataTool::data.GetGObject(sourceId); obj->getType() == GOType::Point)
        {
            SetupRenameCommand(new RenameLabel(oldName, newName, doc, destId, newGroup), needsFullParse, isLast, doc);
        }
        else
        {
            const CurveAliasType type = RenameAlias::CurveType(obj->getType());
            SetupRenameCommand(new RenameAlias(type, oldName, newName, doc, destId, newGroup),
                               needsFullParse,
                               isLast,
                               doc);
        }
    }

    undoStack->push(newGroup);
}
