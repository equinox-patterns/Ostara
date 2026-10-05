/************************************************************************
 **
 **  @file   vtoolline.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#include "vtoolline.h"

#include <QKeyEvent>
#include <QLineF>
#include <QMessageLogger>
#include <QPen>
#include <QPointF>
#include <QSharedPointer>
#include <QUndoStack>
#include <QtDebug>

#include "../../dialogs/tools/dialogline.h"
#include "../../dialogs/tools/dialogtool.h"
#include "../../undocommands/renameobject.h"
#include "../../undocommands/savetooloptions.h"
#include "../../undocommands/vchangelinerolename.h"
#include "../../visualization/line/vistoolline.h"
#include "../../visualization/visualization.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vabstracttool.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "vexception.h"
#include "vscenestylesheet.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "vdrawtool.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

template <class T> class QSharedPointer;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VToolLine constructor.
 * @param initData init data.
 * @param parent parent object.
 */
VToolLine::VToolLine(const VToolLineInitData &initData, QGraphicsItem *parent)
  : VDrawTool(initData.doc, initData.data, initData.id, initData.notes),
    VScaledLine(VColorRole::CustomColor, parent),
    firstPoint(initData.firstPoint),
    secondPoint(initData.secondPoint),
    lineColor(initData.lineColor),
    m_acceptHoverEvents(true),
    m_derived(initData.derived)
{
    SetBoldLine(false);
    SetShowArrows(true);
    this->m_lineType = initData.typeLine;
    // Line
    RefreshGeometry();
    // A derived line is part of its operation: it can not be deleted on its own, but it must still take the
    // keyboard focus, otherwise the Delete key would never reach it. keyReleaseEvent then forwards the
    // request to the operation instead of deleting the line itself.
    this->setFlag(QGraphicsItem::ItemIsFocusable, true); // For keyboard input focus
    this->setAcceptHoverEvents(m_acceptHoverEvents);

    // A derived line belongs to the operation tool that created it: the operation persists its id and
    // rebuilds it on parse, so the line must not write an element of its own nor push an undo command.
    if (!initData.derived)
    {
        ToolCreation(initData.typeCreation);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setDialog set dialog when user want change tool option.
 */
void VToolLine::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogLine> dialogTool = qobject_cast<DialogLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetFirstPoint(firstPoint);
    dialogTool->SetSecondPoint(secondPoint);
    dialogTool->SetTypeLine(m_lineType);
    dialogTool->SetLineColor(lineColor);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetLineRoleName(GetLineRoleName());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create help create tool form GUI.
 * @param dialog dialog.
 * @param scene pointer to scene.
 * @param doc dom document container.
 * @param data container with variables.
 */
auto VToolLine::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> VToolLine *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogLine> dialogTool = qobject_cast<DialogLine *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolLineInitData initData;
    initData.firstPoint = dialogTool->GetFirstPoint();
    initData.secondPoint = dialogTool->GetSecondPoint();
    initData.typeLine = dialogTool->GetTypeLine();
    initData.lineColor = dialogTool->GetLineColor();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();
    initData.lineRoleName = dialogTool->GetLineRoleName();

    VToolLine *line = Create(initData);
    if (line != nullptr)
    {
        line->m_dialog = dialog;
    }
    return line;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create help create tool.
 * @param initData init data.
 */
auto VToolLine::Create(VToolLineInitData initData) -> VToolLine *
{
    SCASSERT(initData.scene != nullptr)
    SCASSERT(initData.doc != nullptr)
    SCASSERT(initData.data != nullptr)
    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->getNextId();
    }
    else
    {
        initData.data->UpdateId(initData.id);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    if (!initData.lineRoleName.isEmpty())
    {
        const QString firstPointName = initData.data->GeometricObject<VPointF>(initData.firstPoint)->name();
        const QString secondPointName = initData.data->GeometricObject<VPointF>(initData.secondPoint)->name();
        initData.data->SetLineRoleName(firstPointName + '_' + secondPointName, initData.lineRoleName);
    }

    // A transformed line has no name of its own — nothing names it, it is built by the operation — and the
    // pair of its transformed endpoints can carry the very names of the pair it comes from. The role name of
    // that pair belongs to the line the user named: taking it would make Line_<name> designate the copy.
    initData.data->AddLine(initData.firstPoint, initData.secondPoint, initData.id, !initData.derived);

    patternGraph->AddEdge(initData.firstPoint, initData.id);
    patternGraph->AddEdge(initData.secondPoint, initData.id);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        // A derived line must NOT enter the tool history. It has no element of its own, and the history is
        // what lets a deletion find the tool that precedes it in the document: DelTool::undo() resolves that
        // sibling through VDomDocument::NodeById(), which throws VExceptionBadId on an entry that has no
        // element - the undo of a deletion then aborts. The history also drives the Dependencies dock,
        // where a line that is only the result of an operation has nothing to do.
        if (!initData.derived)
        {
            VAbstractTool::AddRecord(initData.id, Tool::Line, initData.doc);
        }
        auto *line = new VToolLine(initData);
        initData.scene->addItem(line);
        InitDrawToolConnections(initData.scene, line);
        connect(initData.scene, &VMainGraphicsScene::EnableLineItemSelection, line, &VToolLine::AllowSelecting);
        connect(initData.scene, &VMainGraphicsScene::EnableLineItemHover, line, &VToolLine::AllowHover);
        VAbstractPattern::AddTool(initData.id, line);
        return line;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLine::getTagName() const -> QString
{
    return VDomDocument::TagLine;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    PaintWithFixItemHighlightSelected<VScaledLine>(this, painter, option, widget);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLine::shape() const -> QPainterPath
{
    if (m_derived)
    {
        // A derived line belongs to the operation that draws it and is rebuilt by it at every parse: it is not an
        // object of its own, it has no element to delete, and the operation is what a click has to reach. It keeps
        // the base's answer, empty while nothing is drawn.
        return VScaledLine::shape();
    }

    // A line drawn with the Line tool IS an object of its own, and its hit area must not depend on what it looks
    // like: with the style "none" the base answers an empty path, so the line could be neither hovered, nor
    // clicked, nor right-clicked, nor framed by the Dependencies dock — while it still counted as a user of its
    // two endpoints and blocked their deletion. Measured on Sophie's BustedeBase: C4 could not be deleted because
    // of a "none" line C4–C2 that nothing on the canvas allowed to pick. Its own geometry answers instead.
    return LineHitPath();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::RefreshPen()
{
    VScaledLine::RefreshPen();

    QPen lPen = pen();
    // The line owns its colour — this override is what replaces the base's — so it is also the one that has to
    // answer for the layer marks, both of them: selected (orange, what Enter will act on) and in the layer
    // (magenta, the layer's content). The selection is tested first, since a line can be both at once.
    if (IsLayerSelected())
    {
        lPen.setColor(VSceneStylesheet::CorrectToolColor(this, VSceneStylesheet::Color(VColorRole::ActivePointColor)));
    }
    else if (IsInLayer())
    {
        lPen.setColor(VSceneStylesheet::CorrectToolColor(this, VSceneStylesheet::Color(VColorRole::VisSupportColor)));
    }
    else
    {
        lPen.setColor(
            VSceneStylesheet::CorrectToolColor(this, VSceneStylesheet::CorrectToolColorForDarkTheme(lineColor)));
    }

    // The style the tool asked for, except that a "none" line has to show itself while it is emphasized — hovered,
    // picked for a layer, or in the layer being worked on. It is the same rule the base applies to the line a point
    // draws through it (VScaledLine::RefreshPen), read here because this override is what writes the pen. Without
    // it a "none" line stays invisible for good, and being invisible is not being absent: it still blocks the
    // deletion of its endpoints.
    const Qt::PenStyle authored = LineStyleToPenStyle(m_lineType);
    const bool emphasized = IsBoldLine() || IsLayerSelected() || IsInLayer() || isSelected();
    lPen.setStyle(authored == Qt::NoPen && emphasized ? Qt::SolidLine : authored);
    setPen(lPen);
}

//---------------------------------------------------------------------------------------------------------------------
// An endpoint that cannot be resolved — a line carried by an operation whose points are gone — must not throw:
// these accessors are called on hover and on every refresh, and an exception there freezes the whole interface.
auto VToolLine::FirstPointName() const -> QString
{
    try
    {
        return VAbstractTool::data.GetGObject(firstPoint)->name();
    }
    catch (const VExceptionBadId &)
    {
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLine::SecondPointName() const -> QString
{
    try
    {
        return VAbstractTool::data.GetGObject(secondPoint)->name();
    }
    catch (const VExceptionBadId &)
    {
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLine::GetLineRoleName() const -> QString
{
    return VAbstractTool::data.LineRoleName(FirstPointName() + '_' + SecondPointName());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::SetLineRoleName(const QString &suffix)
{
    const QString pairKey = FirstPointName() + '_' + SecondPointName();
    const QString oldSuffix = VAbstractTool::data.LineRoleName(pairKey);
    // Uniqueness: same check as curves (IsUnique)
    if (!suffix.isEmpty() && !VAbstractTool::data.IsUnique(line_ + suffix))
    {
        return;
    }

        if (oldSuffix == suffix)
    {
        return;
    }

    // Update the local data copy (VAbstractTool::data is a copy-on-write snapshot;
    // undo commands only update the DOM — the reparse rebuilds the parse-time
    // VContainer, but this tool's local copy is never touched by the reparse.)
    VAbstractTool::data.SetLineRoleName(pairKey, suffix);

    auto *command = new VChangeLineRoleName(doc, pairKey, oldSuffix, suffix);
    connect(command, &VChangeLineRoleName::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    VAbstractApplication::VApp()->getUndoStack()->push(command);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FullUpdateFromFile update tool data form file.
 */
void VToolLine::FullUpdateFromFile()
{
    if (!m_derived)
    {
        // A derived line belongs to the operation that created it and has no element of its own — see
        // ToolCreation: looking for one would find nothing and say so on every refresh, which is the normal
        // state of a transformed line, not a surprise.
        ReadAttributes();
    }
    RefreshGeometry();
    SetVisualization();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::Enable()
{
    setEnabled(m_indexActivePatternBlock == doc->PatternBlockMapper()->GetActiveId());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::AllowHover(bool enabled)
{
    // Manually handle hover events. Need for setting cursor for not selectable paths.
    m_acceptHoverEvents = enabled;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::AllowSelecting(bool enabled)
{
    // A line created by an operation is part of that operation's result: it is never selected on its own.
    // Clicking it opens the operation (see VToolOptionsPropertyBrowser::ShowItemOptions).
    setFlag(QGraphicsItem::ItemIsSelectable, enabled && !m_derived);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    Q_UNUSED(id)
    try
    {
        ContextMenu<DialogLine>(event);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief contextMenuEvent handle context menu events.
 * @param event context menu event.
 */
void VToolLine::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    // A line created by an operation belongs to that operation: it is rebuilt from it at every parse, so
    // editing it on its own would have no effect. Its operation carries its options, and the right click is
    // forwarded to it — doing nothing here would swallow the event, since the scene accepts it before the
    // item is asked (QGraphicsScene::contextMenuEvent) and the operation item below would never see it.
    if (m_derived)
    {
        emit ContextMenuRequested(event, m_id);
        return;
    }

    ShowContextMenu(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // A line is not selectable while an operation picks its source objects (MainWindow::ToolSelectOperationLines),
    // and the default handler ignores the press of a not selectable item. The press must be accepted explicitly:
    // otherwise the scene never hands the mouse to this item and the release — where the line announces itself —
    // is never delivered. Same idiom as VToolAbstractCurve and VSimplePoint.
    QGraphicsLineItem::mousePressEvent(event);

    // Somehow clicking on a not selectable object does not clean previous selections.
    if (not(flags() & ItemIsSelectable) && scene())
    {
        scene()->clearSelection();
    }

    // A derived line is not selectable, so nothing would give it the focus on a click: take it, otherwise
    // the Delete key would never reach this item (see keyReleaseEvent).
    if (m_derived)
    {
        setFocus(Qt::MouseFocusReason);
    }

    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // A line used to be silent on click: nothing announced it, so an operation tool could not select it.
    // It reports itself here, exactly like a point or a curve does.
    if (IsSelectedByReleaseEvent(this, event))
    {
        TriggerChoosed();
    }
    QGraphicsLineItem::mouseReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddToFile add tag with informations about tool into file.
 */
void VToolLine::AddToFile()
{
    QDomElement domElement = doc->createElement(getTagName());
    auto obj = QSharedPointer<VGObject>();
    SaveOptions(domElement, obj);
    AddToCalculation(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief hoverEnterEvent handle hover enter events.
 * @param event hover enter event.
 */
void VToolLine::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (m_acceptHoverEvents)
    {
        SetBoldLine(true);
        setToolTip(MakeToolTip());
        VScaledLine::hoverEnterEvent(event);
    }
    else
    {
        setCursor(VAbstractValApplication::VApp()->getSceneView()->viewport()->cursor());
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief hoverLeaveEvent handle hover leave events.
 * @param event hover leave event.
 */
void VToolLine::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (m_acceptHoverEvents && vis.isNull())
    {
        SetBoldLine(false);
        VScaledLine::hoverLeaveEvent(event);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief itemChange handle item change.
 * @param change change.
 * @param value value.
 * @return value.
 */
auto VToolLine::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) -> QVariant
{
    if (change == QGraphicsItem::ItemSceneHasChanged && value.value<QGraphicsScene *>() != nullptr)
    {
        RefreshPen();
    }
    else if (change == QGraphicsItem::ItemEnabledHasChanged)
    {
        RefreshPen();
    }
    else if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        if (value.toBool())
        {
            setFocus();
        }

        emit ChangedToolSelection(value.toBool(), m_id, m_id);
    }

    return QGraphicsItem::itemChange(change, value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief keyReleaseEvent handle key realse events.
 * @param event key realse event.
 */
void VToolLine::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Delete:
            if (m_derived)
            {
                // A derived line is part of the operation that draws it: asking to delete the line means
                // deleting that operation, exactly like a destination point. It must never delete itself -
                // it has no element of its own for DelTool to remove.
                emit DeleteRequested();
                return; // Leave this method immediately after call!!!
            }

            try
            {
                DeleteToolWithConfirm();
            }
            catch (const VExceptionToolWasDeleted &e)
            {
                Q_UNUSED(e)
                return; // Leave this method immediately!!!
            }
            break;
        default:
            break;
    }
    QGraphicsItem::keyReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SaveDialog save options into file after change in dialog.
 */
void VToolLine::SaveDialog(QDomElement &domElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogLine> dialogTool = qobject_cast<DialogLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    doc->SetAttribute(domElement, AttrFirstPoint, QString().setNum(dialogTool->GetFirstPoint()));
    doc->SetAttribute(domElement, AttrSecondPoint, QString().setNum(dialogTool->GetSecondPoint()));
    doc->SetAttribute(domElement, AttrTypeLine, dialogTool->GetTypeLine());
    doc->SetAttribute(domElement, AttrLineColor, dialogTool->GetLineColor());
    doc->SetAttributeOrRemoveIf<QString>(domElement,
                                         AttrNotes,
                                         dialogTool->GetNotes(),
                                         [](const QString &notes) noexcept -> bool { return notes.isEmpty(); });

    // Persist the role name via the dedicated undo command (lineNames DOM section)
    SetLineRoleName(dialogTool->GetLineRoleName());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VDrawTool::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrFirstPoint, firstPoint);
    doc->SetAttribute(tag, AttrSecondPoint, secondPoint);
    doc->SetAttribute(tag, AttrTypeLine, m_lineType);
    doc->SetAttribute(tag, AttrLineColor, lineColor);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::ReadToolAttributes(const QDomElement &domElement)
{
    VDrawTool::ReadToolAttributes(domElement);

    firstPoint = VAbstractPattern::GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
    secondPoint = VAbstractPattern::GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);
    m_lineType = VDomDocument::GetParametrString(domElement, AttrTypeLine, TypeLineLine);
    lineColor = VDomDocument::GetParametrString(domElement, AttrLineColor, ColorBlack);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<VisToolLine *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetPoint1Id(firstPoint);
        visual->SetPoint2Id(secondPoint);
        visual->SetLineStyle(LineStyleToPenStyle(m_lineType));
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLine::MakeToolTip() const -> QString
{
    // The tooltip is built on every hover: an endpoint that cannot be resolved must not throw here, or the
    // event loop dies with it (no more hover, no more tool, no more mode).
    try
    {
        const QSharedPointer<VPointF> first = VAbstractTool::data.GeometricObject<VPointF>(firstPoint);
        const QSharedPointer<VPointF> second = VAbstractTool::data.GeometricObject<VPointF>(secondPoint);

        const QLineF line(static_cast<QPointF>(*first), static_cast<QPointF>(*second));

        const QString toolTip = u"<table>"
                                u"<tr> <td><b>%1:</b> %2 %3</td> </tr>"
                                u"<tr> <td><b>%4:</b> %5°</td> </tr>"
                                u"</table>"_s.arg(tr("Length"))
                                    .arg(RoundForDisplay(VAbstractValApplication::VApp()->fromPixel(line.length())))
                                    .arg(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true), tr("Angle"))
                                    .arg(RoundForDisplay(line.angle()));
        return toolTip;
    }
    catch (const VExceptionBadId &)
    {
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogLine> dialogTool = qobject_cast<DialogLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    const QString oldFirstPointLabel = FirstPointName();
    const QString newFirstPointLabel = VAbstractTool::data.GetGObject(dialogTool->GetFirstPoint())->name();

    const QString oldSecondPointLabel = SecondPointName();
    const QString newSecondPointLabel = VAbstractTool::data.GetGObject(dialogTool->GetSecondPoint())->name();

    if (oldFirstPointLabel == newFirstPointLabel && oldSecondPointLabel == newSecondPointLabel)
    {
        VDrawTool::ApplyToolOptions(oldDomElement, newDomElement);
        return;
    }

    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    auto *newGroup = new QUndoCommand(); // an empty command
    newGroup->setText(tr("save tool options"));

    auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id, newGroup);
    saveOptions->SetInGroup(true);
    connect(saveOptions, &SaveToolOptions::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);

    ObjectPair_t newPair;
    if (oldFirstPointLabel != newFirstPointLabel && oldSecondPointLabel != newSecondPointLabel)
    {
        newPair = std::make_pair(newFirstPointLabel, newSecondPointLabel);
    }
    else if (oldFirstPointLabel != newFirstPointLabel)
    {
        newPair = std::make_pair(newFirstPointLabel, oldSecondPointLabel);
    }
    else if (oldSecondPointLabel != newSecondPointLabel)
    {
        newPair = std::make_pair(oldFirstPointLabel, newSecondPointLabel);
    }

    auto *renamePair = RenamePair::CreateForLine(std::make_pair(oldFirstPointLabel, oldSecondPointLabel),
                                                 newPair,
                                                 doc,
                                                 m_id,
                                                 newGroup);
    connect(renamePair, &RenamePair::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);

    undoStack->push(newGroup);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::ShowVisualization(bool show)
{
    if (show)
    {
        if (vis.isNull())
        {
            AddVisualization<VisToolLine>();
            SetVisualization();
        }
        else
        {
            if (auto *visual = qobject_cast<VisToolLine *>(vis))
            {
                visual->show();
            }
        }
    }
    else
    {
        delete vis.data();
        hoverLeaveEvent(nullptr);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::SetLineType(const QString &value)
{
    m_lineType = value;
    RefreshPen();

    QSharedPointer<VGObject> obj; // We don't have object for line in data container. Just will send an empty object.
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLine::GetLineColor() const -> QString
{
    return lineColor;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::SetLineColor(const QString &value)
{
    lineColor = value;
    RefreshPen();

    QSharedPointer<VGObject> obj; // We don't have object for line in data container. Just will send an empty object.
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::RefreshPenFromParse(const QString &typeLine, const QString &lineColor)
{
    if (m_lineType == typeLine && this->lineColor == lineColor)
    {
        return;
    }

    m_lineType = typeLine;
    this->lineColor = lineColor;
    RefreshPen();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::SetNotes(const QString &notes)
{
    m_notes = notes;

    QSharedPointer<VGObject> obj; // We don't have object for line in data container. Just will send an empty object.
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLine::GroupVisibility(quint32 object, bool visible)
{
    Q_UNUSED(object)
    setVisible(visible);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RefreshGeometry refresh item on scene.
 */
void VToolLine::RefreshGeometry()
{
    // Called on every scene refresh: an endpoint that is gone leaves the line where it is rather than
    // throwing into the event loop.
    try
    {
        const QSharedPointer<VPointF> first = VAbstractTool::data.GeometricObject<VPointF>(firstPoint);
        const QSharedPointer<VPointF> second = VAbstractTool::data.GeometricObject<VPointF>(secondPoint);
        this->setLine(QLineF(static_cast<QPointF>(*first), static_cast<QPointF>(*second)));
    }
    catch (const VExceptionBadId &)
    {
        return;
    }
}
