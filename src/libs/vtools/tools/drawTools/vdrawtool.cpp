/************************************************************************
 **
 **  @file   vdrawtool.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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

#include "vdrawtool.h"
#include "../vtheme/omenuglyph.h"
#include "../vtheme/vtheme.h"

#include <QDialog>
#include <QDomNode>
#include <QMessageLogger>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QUndoStack>
#include <QtDebug>

#include "../../undocommands/addtocalc.h"
#include "../../undocommands/savetooloptions.h"
#include "../../undocommands/undogroup.h"
#include "../ifc/exception/vexceptionwrongid.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vdomdocument.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../vabstracttool.h"
#include "../vgeometry/vpointf.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vabstractmainwindow.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

template <class T> class QSharedPointer;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VDrawTool constructor.
 * @param doc dom document container.
 * @param data container with variables.
 * @param id object id in container.
 */
VDrawTool::VDrawTool(VAbstractPattern *doc, VContainer *data, quint32 id, QString notes, QObject *parent)
  : VInteractiveTool(doc, data, id, parent),
    m_indexActivePatternBlock(doc->PatternBlockMapper()->GetActiveId()),
    m_lineType(TypeLineLine),
    m_notes(std::move(notes))
{
    connect(this->doc->PatternBlockMapper(),
            QOverload<int>::of(&VPatternBlockMapper::ChangedActivePatternBlock),
            this,
            &VDrawTool::Enable);
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::SaveDialogChange(const QString &undoText)
{
    Q_UNUSED(undoText)
    qCDebug(vTool, "Saving tool options after using dialog");
    QDomElement const oldDomElement = doc->FindElementById(m_id, getTagName());
    if (!oldDomElement.isElement())
    {
        qCDebug(vTool, "Can't find tool with id = %u", m_id);
        return;
    }

    QDomElement newDomElement = oldDomElement.cloneNode().toElement();
    SaveDialog(newDomElement);
    ApplyToolOptions(oldDomElement, newDomElement);
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id);
    connect(saveOptions, &SaveToolOptions::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    VAbstractApplication::VApp()->getUndoStack()->push(saveOptions);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddToFile add tag with informations about tool into file.
 */
void VDrawTool::AddToFile()
{
    QDomElement domElement = doc->createElement(getTagName());
    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOptions(domElement, obj);
    AddToCalculation(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::SaveOption(QSharedPointer<VGObject> &obj)
{
    qCDebug(vTool, "Saving tool options");
    QDomElement const oldDomElement = doc->FindElementById(m_id, getTagName());
    if (!oldDomElement.isElement())
    {
        qCDebug(vTool, "Can't find tool with id = %u", m_id);
        return;
    }

    QDomElement newDomElement = oldDomElement.cloneNode().toElement();

    SaveOptions(newDomElement, obj);

    auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id);
    connect(saveOptions, &SaveToolOptions::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    VAbstractApplication::VApp()->getUndoStack()->push(saveOptions);
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    Q_UNUSED(obj)

    doc->SetAttribute(tag, VDomDocument::AttrId, m_id);
    doc->SetAttributeOrRemoveIf<QString>(tag,
                                         AttrNotes,
                                         m_notes,
                                         [](const QString &notes) noexcept -> bool { return notes.isEmpty(); });
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::MakeToolTip() const -> QString
{
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::UpdateNamePosition(quint32 id, const QPointF &pos)
{
    Q_UNUSED(id)
    Q_UNUSED(pos)
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::WriteLabelPosition(const QString &attrMx, const QString &attrMy, const QPointF &pos)
{
    QDomElement element = doc->FindElementById(m_id, getTagName());
    if (not element.isElement())
    {
        return;
    }

    doc->SetAttribute(element, attrMx, VAbstractValApplication::VApp()->fromPixel(pos.x()));
    doc->SetAttribute(element, attrMy, VAbstractValApplication::VApp()->fromPixel(pos.y()));
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::ReadAttributes()
{
    const QDomElement domElement = doc->FindElementById(m_id, getTagName());
    if (domElement.isElement())
    {
        ReadToolAttributes(domElement);
    }
    else
    {
        qCDebug(vTool, "Can't find tool with id = %u", m_id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::ReadToolAttributes(const QDomElement &domElement)
{
    m_notes = VDomDocument::GetParametrEmptyString(domElement, AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::EnableToolMove(bool move)
{
    Q_UNUSED(move)
    // Do nothing.
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::SetDetailsMode(bool mode)
{
    Q_UNUSED(mode)
    // Do nothing.
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::ChangeLabelVisibility(quint32 id, bool visible)
{
    Q_UNUSED(id)
    Q_UNUSED(visible)
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Retrieves the geometric object type for the given item ID.
 * 
 * @param itemId The ID of the item to query
 * @return The GOType of the item, or GOType::Unknown if the item doesn't exist or was deleted
 */
auto VDrawTool::GetItemType(quint32 itemId) const -> GOType
{
    if (itemId == NULL_ID)
    {
        return GOType::Unknown;
    }

    try
    {
        return data.GetGObject(itemId)->getType();
    }
    catch (const VExceptionBadId &e)
    {
        // Possible cases: a parent was deleted and the node object still exists, or the id is a line carried by
        // an operation — a Line tool, which is not an object of the container at all. Asking for its type is
        // normal, and this method runs on every right click and every selection.
        //
        // It must not warn: the message handler turns every warning into a modal error box, and a box raised
        // from inside the event stack re-enters the event loop there — one box per interaction leaves the
        // application unusable (see the QPainter:: and QPen:: cases in noisyFailureMsgHandler). The trace
        // stays available in the log.
        qCDebug(vTool) << qUtf8Printable(e.ErrorMessage());
        return GOType::Unknown;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Builds the tool context menu, runs it and dispatches the selection.
 *
 * The "Options" entry is added only when @a withOptions is true; selecting it calls
 * @a onOptionsSelected, so the dialog type — known only to the templated entry points — does not
 * have to appear here.
 *
 * @param event The context menu event containing position and triggering information
 * @param itemId The ID of the specific item being clicked, or 0 for the tool itself
 * @param withOptions Whether to offer the "Options" entry
 * @param onOptionsSelected Handler invoked when the "Options" entry is selected
 */
void VDrawTool::RunContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 itemId, bool withOptions,
                               const std::function<void()> &onOptionsSelected)
{
    SCASSERT(event != nullptr)

    if (m_suppressContextMenu)
    {
        return;
    }

    GOType const itemType = GetItemType(itemId);

    qCDebug(vTool, "Creating tool context menu.");
    QMenu menu;

    // Build menu structure
    QAction *actionOption = nullptr;
    if (withOptions)
    {
        actionOption = menu.addAction(VDrawTool::tr("Options"));
        OMenuGlyph::SetGlyph(actionOption, QStringLiteral("gear-six"));
    }

    // add the menu "add to group" to the context menu
    auto *actionsAddToGroup = CreateAddToGroupMenu(menu, itemId);

    // add the menu "remove from group" to the context menu
    auto *actionsRemoveFromGroup = CreateRemoveFromGroupMenu(menu, itemId);

    QAction *actionShowLabel = CreateShowLabelAction(menu, itemType, itemId);
    QAction *actionRemove = CreateRemoveAction(menu);

    // A row of a menu under the cursor has an accent background in both themes, and the glyph of the active
    // theme (#2b2b2b in light) dies on it: under the cursor every entry takes the dark set of the same name
    // (Icons.md §2, "règle ligne foncée"). Installed once for the whole menu, submenu titles included.
    OMenuGlyph::InstallHover(&menu, &menu);

    // Execute menu and handle selection
    QAction const *selectedAction = menu.exec(event->screenPos());

    if (selectedAction == nullptr)
    {
        return;
    }

    if (selectedAction == actionOption && onOptionsSelected)
    {
        onOptionsSelected();
        return;
    }

    DispatchMenuSelection(selectedAction,
                          {.option = actionOption,
                           .remove = actionRemove,
                           .showLabel = actionShowLabel,
                           .addToGroup = actionsAddToGroup,
                           .removeFromGroup = actionsRemoveFromGroup},
                          itemId);
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::ContextMenuWithoutOptions(QGraphicsSceneContextMenuEvent *event, quint32 itemId)
{
    RunContextMenu(event, itemId, /*withOptions*/ false, {});
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Handles the user's menu selection and executes the corresponding action.
 *
 * The "Options" entry is not handled here: it is dispatched by RunContextMenu, which owns the
 * dialog type.
 *
 * @param selectedAction The action selected by the user
 * @param actions Struct containing all menu actions and action groups
 * @param itemId The ID of the item being operated on
 */
void VDrawTool::DispatchMenuSelection(const QAction *selectedAction, const MenuActions &actions, quint32 itemId)
{
    if (selectedAction == actions.remove)
    {
        HandleRemoveAction();
    }
    else if (selectedAction == actions.showLabel)
    {
        ChangeLabelVisibility(itemId, selectedAction->isChecked());
    }
    else if (selectedAction->actionGroup() == actions.addToGroup)
    {
        HandleAddToGroup(selectedAction, itemId);
    }
    else if (selectedAction->actionGroup() == actions.removeFromGroup)
    {
        HandleRemoveFromGroup(selectedAction, itemId);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Creates the "Add to group" submenu with available groups.
 * 
 * @param menu The parent menu to add the submenu to
 * @param itemId The ID of the item that will be added to a group
 * @return Action group containing all "add to group" actions, or nullptr if no groups available
 */
auto VDrawTool::CreateAddToGroupMenu(QMenu &menu, quint32 itemId) -> QActionGroup *
{
    const QPair<quint32, quint32> target = LayerMenuPair(itemId);
    QMap<quint32, QString> groupsNotContainingItem
        = doc->GetGroupsContainingItem(target.second, target.first, false);

    auto *actionsAddToGroup = new QActionGroup(this);

    if (groupsNotContainingItem.empty())
    {
        return actionsAddToGroup;
    }

    QMenu *menuAddToGroup = menu.addMenu(tr("Add to layer"));
    OMenuGlyph::SetGlyph(menuAddToGroup->menuAction(), QStringLiteral("plus"));

    auto list = QStringList(groupsNotContainingItem.values());
    list.sort(Qt::CaseInsensitive);

    for (const auto &name : list)
    {
        QAction *actionAddToGroup = menuAddToGroup->addAction(name);
        actionsAddToGroup->addAction(actionAddToGroup);
        const quint32 groupId = groupsNotContainingItem.key(name);
        actionAddToGroup->setData(groupId);

        // Remove the processed group to handle duplicate names correctly
        groupsNotContainingItem.remove(groupId);
    }

    return actionsAddToGroup;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::CreateRemoveFromGroupMenu(QMenu &menu, quint32 itemId) -> QActionGroup *
{
    const QPair<quint32, quint32> target = LayerMenuPair(itemId);
    QMap<quint32, QString> groupsContainingItem = doc->GetGroupsContainingItem(target.second, target.first, true);

    auto *actionsRemoveFromGroup = new QActionGroup(this);

    if (groupsContainingItem.empty())
    {
        return actionsRemoveFromGroup;
    }

    QMenu *menuRemoveFromGroup = menu.addMenu(tr("Remove from layer"));
    OMenuGlyph::SetGlyph(menuRemoveFromGroup->menuAction(), QStringLiteral("minus"));

    auto list = QStringList(groupsContainingItem.values());
    list.sort(Qt::CaseInsensitive);

    // An object must always belong to at least one layer, and taking it out of the layer that holds it ALONE would
    // leave it belonging to none — the very state the layer system exists to prevent. The Layers dock refuses that
    // removal (VWidgetGroups::RemovableObjects); the canvas had no such guard, and this menu was a way to strand an
    // object (the next load would silently re-home it in "Unnamed"). Removing here takes the WHOLE figure out of
    // the layer — HandleRemoveFromGroup walks every object GroupObjects() returns — so a layer may only be offered
    // when each object of the figure it holds is also held by another layer. What is refused stays VISIBLE, greyed:
    // the layer's name is then the one thing the entry still tells the user, namely where the object is.
    const QVector<QPair<quint32, quint32>> figure = GroupObjects(itemId);
    const auto mayLeave = [this, &figure](quint32 groupId) -> bool
    {
        for (const auto &[objectId, toolId] : figure)
        {
            const QMap<quint32, QString> holders = doc->GetGroupsContainingItem(toolId, objectId, true);
            if (holders.contains(groupId) && holders.size() < 2)
            {
                return false;
            }
        }
        return true;
    };

    for (const auto &name : list)
    {
        QAction *actionRemoveFromGroup = menuRemoveFromGroup->addAction(name);
        actionsRemoveFromGroup->addAction(actionRemoveFromGroup);
        const quint32 groupId = groupsContainingItem.key(name);
        actionRemoveFromGroup->setData(groupId);
        actionRemoveFromGroup->setEnabled(mayLeave(groupId));

        // Remove the processed group to handle duplicate names correctly
        groupsContainingItem.remove(groupId);
    }

    return actionsRemoveFromGroup;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Creates the "Show Point name" toggle action for point items.
 * 
 * @param menu The parent menu to add the action to
 * @param itemType The type of the geometric object
 * @param itemId The ID of the item
 * @return Checkable action for showing/hiding the label, or nullptr if not applicable
 */
auto VDrawTool::CreateShowLabelAction(QMenu &menu, GOType itemType, quint32 itemId) -> QAction *
{
    QAction *actionShowLabel = menu.addAction(tr("Show Point name"));
    actionShowLabel->setCheckable(true);

    if (itemType == GOType::Point)
    {
        const bool visible = IsLabelVisible(itemId);
        actionShowLabel->setChecked(visible);

        // Same convention as the node menu of the Details mode: the glyph marks the entry that is ON, and an
        // entry that is off carries none — this is a toggle, not an icon with two states.
        if (visible)
        {
            OMenuGlyph::SetGlyph(actionShowLabel, QStringLiteral("check"));
        }
    }
    else
    {
        actionShowLabel->setVisible(false);
    }

    return actionShowLabel;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Creates the "Delete" action with appropriate status indication.
 * 
 * The action is enabled if the item is removable or blocked, and shows
 * a "Pending" indicator if deletion status is pending.
 * 
 * @param menu The parent menu to add the action to
 * @return Delete action with enabled state based on removal status
 */
auto VDrawTool::CreateRemoveAction(QMenu &menu) -> QAction *
{
    using namespace Qt::Literals::StringLiterals;

    QAction *actionRemove = menu.addAction(VDrawTool::tr("Delete"));
    OMenuGlyph::SetGlyph(actionRemove, QStringLiteral("trash"));
    RemoveStatus const status = IsRemovable();
    actionRemove->setEnabled(status == RemoveStatus::Removable || status == RemoveStatus::Blocked);

    if (status == RemoveStatus::Pending)
    {
        actionRemove->setText(actionRemove->text() + " ("_L1 + tr("Pending") + ')');
    }

    return actionRemove;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Handles the remove/delete action, either deleting the tool or showing an error.
 * 
 * If the tool is removable, it proceeds with deletion. If the tool is blocked
 * due to dependencies, it displays a warning message explaining why deletion
 * cannot proceed.
 */
void VDrawTool::HandleRemoveAction()
{
    // Une seule route pour supprimer un objet, que le geste vienne du clic droit ou de la touche Suppr : c'est
    // l'outil qui sait s'il est supprimable, et **pourquoi** il ne l'est pas — un point de base, lui, demande en
    // nommant la pièce et explique quelle pièce en dépend. Ce chemin, lui, se taisait derrière une phrase
    // générique qui ne disait ni la pièce ni la raison.
    DeleteToolWithConfirm(); // do not catch exception here
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Adds the item to the selected group and updates the UI.
 * 
 * @param selectedAction The action containing the group ID as data
 * @param itemId The ID of the item to add to the group
 */
void VDrawTool::HandleAddToGroup(const QAction *selectedAction, quint32 itemId)
{
    quint32 const groupId = selectedAction->data().toUInt();
    const QVector<QPair<quint32, quint32>> objects = GroupObjects(itemId);

    auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
    SCASSERT(scene != nullptr)
    scene->clearSelection();

    auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
    SCASSERT(window != nullptr)

    // One command per object, under a single macro: one Ctrl+Z takes the whole write out again. The list is the
    // one GroupObjects settles — the clicked copy alone for an operation, the whole figure for a tool that draws
    // one indivisible figure — so the canvas and the Layers dock write the same thing.
    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(tr("add objects to a layer"));

    for (const auto &[objectId, toolId] : objects)
    {
        if (const QDomElement item = doc->AddItemToGroup(toolId, objectId, groupId); not item.isNull())
        {
            auto *addItemToGroup = new AddItemToGroup(item, doc, groupId);
            connect(addItemToGroup, &AddItemToGroup::UpdateGroups, window,
                    &VAbstractMainWindow::UpdateVisibilityGroups);
            undoStack->push(addItemToGroup);
        }
    }

    undoStack->endMacro();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Removes the item from the selected group and updates the UI.
 * 
 * @param selectedAction The action containing the group ID as data
 * @param itemId The ID of the item to remove from the group
 */
void VDrawTool::HandleRemoveFromGroup(const QAction *selectedAction, quint32 itemId)
{
    quint32 const groupId = selectedAction->data().toUInt();
    const QVector<QPair<quint32, quint32>> objects = GroupObjects(itemId);

    auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
    SCASSERT(window != nullptr)

    // Symmetric with adding: the same list leaves the layer, in one undoable step.
    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(tr("remove objects from a layer"));

    for (const auto &[objectId, toolId] : objects)
    {
        if (const QDomElement item = doc->RemoveItemFromGroup(toolId, objectId, groupId); not item.isNull())
        {
            auto *removeItemFromGroup = new RemoveItemFromGroup(item, doc, groupId);
            connect(removeItemFromGroup,
                    &RemoveItemFromGroup::UpdateGroups,
                    window,
                    &VAbstractMainWindow::UpdateVisibilityGroups);
            undoStack->push(removeItemFromGroup);
        }
    }

    undoStack->endMacro();
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
/** @brief The (object, tool) pair the "add to layer" and "remove from layer" entries speak about.
 *
 *  The one the action will write, so a menu can never offer a layer on one pair and write another. For an
 *  operation that is the copy the click landed on — and for a produced line the pair is (line, line), not
 *  (operation, line), which is exactly why this asks the working set instead of building the pair from this
 *  tool's own id. A tool that draws one indivisible figure keeps the pair of the object that was clicked: its
 *  figure goes in and out as one, and the entries list it where the click landed.
 */
auto VDrawTool::LayerMenuPair(quint32 itemId) const -> QPair<quint32, quint32>
{
    if (const QVector<QPair<quint32, quint32>> working = GroupObjects(itemId); working.size() == 1)
    {
        return working.first();
    }

    return {itemId, getId()};
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::GroupObjects(quint32 itemId) const -> QVector<QPair<quint32, quint32>>
{
    // The objects a layer holds for one click. A tool the pattern cannot enumerate — a piece, a node — falls back
    // to the object the menu was opened on, which is what this path has always written.
    if (const QDomElement element = doc->FindElementById(getId(), getTagName()); not element.isNull())
    {
        const QVector<QPair<quint32, quint32>> objects = VAbstractPattern::LayerableObjectsOfTool(element, getId());
        if (not objects.isEmpty())
        {
            // An operation produces a SET of copies, each an object of its own, and a click works on the one it
            // landed on — never on the whole set. That is also what the layer of an operation already does: it
            // holds the sources that were picked, never all of them, so the copies can be hidden and filed one by
            // one (asked for on 01-10-2026: "je ne peux pas ajouter la courbe spécifiquement dans un
            // calque"). A cut is the other case: the point and its two halves are three objects, each filed on
            // its own, so the click speaks for the one it landed on. Every other tool draws ONE figure whose
            // parts travel together — a rectangle, a double point, a dart cap — and the whole figure is what
            // goes in or out (§5quinquies).
            const bool cutPoint = VAbstractPattern::IsCutPoint(element);
            if (element.tagName() == VAbstractPattern::TagOperation || cutPoint)
            {
                for (const auto &object : objects)
                {
                    if (object.first == itemId)
                    {
                        return {object};
                    }
                }
            }

            return objects;
        }
    }

    return {QPair<quint32, quint32>(itemId, getId())};
}







//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddToCalculation add tool to calculation tag in pattern file.
 * @param domElement tag in xml tree.
 */
void VDrawTool::AddToCalculation(const QDomElement &domElement)
{
    const QDomElement duplicate = doc->FindElementById(m_id);
    if (not duplicate.isNull())
    {
        throw VExceptionWrongId(tr("This id (%1) is not unique.").arg(m_id), duplicate);
    }

    auto *addToCal = new AddToCalc(domElement, doc);
    connect(addToCal, &AddToCalc::NeedFullParsing, doc, &VAbstractPattern::NeedFullParsing);
    VAbstractApplication::VApp()->getUndoStack()->push(addToCal);
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::getLineType() const -> QString
{
    return m_lineType;
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::SetLineType(const QString &value)
{
    m_lineType = value;

    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::IsLabelVisible(quint32 id) const -> bool
{
    Q_UNUSED(id)
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::GetNotes() const -> QString
{
    return m_notes;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::AxisAngle(const OAxis &axis, const VContainer *data) -> qreal
{
    SCASSERT(data != nullptr)
    SCASSERT(axis.IsValid())

    const QSharedPointer<VPointF> p1 = data->GeometricObject<VPointF>(axis.P1());
    const QSharedPointer<VPointF> p2 = data->GeometricObject<VPointF>(axis.P2());

    return axis.HandleAngle(static_cast<QPointF>(*p1), static_cast<QPointF>(*p2));
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::SaveAxis(VAbstractPattern *doc,
                         QDomElement &element,
                         const OAxis &axis,
                         const QString &attrP1,
                         const QString &attrP2,
                         const QString &attrSense)
{
    SCASSERT(doc != nullptr)

    if (not axis.IsValid())
    {
        element.removeAttribute(attrP1);
        element.removeAttribute(attrP2);
        element.removeAttribute(attrSense);
        return;
    }

    doc->SetAttribute(element, attrP1, axis.P1());
    doc->SetAttribute(element, attrP2, axis.P2());
    doc->SetAttribute(element, attrSense, axis.SenseName());
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::ReadAxis(const QDomElement &element,
                         const QString &attrP1,
                         const QString &attrP2,
                         const QString &attrSense) -> OAxis
{
    const quint32 p1 = VDomDocument::GetParametrUInt(element, attrP1, NULL_ID_STR);
    const quint32 p2 = VDomDocument::GetParametrUInt(element, attrP2, NULL_ID_STR);
    const QString sense = VDomDocument::GetParametrEmptyString(element, attrSense);

    return {p1, p2, OAxis::SenseFromName(sense)};
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::AxisSlotCount() const -> int
{
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto VDrawTool::AxisSlot(int slot) const -> OAxis
{
    Q_UNUSED(slot)
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::PreviewAxis(int slot, const OAxis &axis)
{
    Q_UNUSED(slot)
    Q_UNUSED(axis)
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::CommitAxis()
{
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::RevertAxis()
{
}

//---------------------------------------------------------------------------------------------------------------------
void VDrawTool::SetNotes(const QString &notes)
{
    m_notes = notes;

    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOption(obj);
}
