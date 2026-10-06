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

#ifndef VABSTRACTOPERATION_H
#define VABSTRACTOPERATION_H

#include <QGraphicsLineItem>
#include <QMap>
#include <QMetaObject>
#include <QSet>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../toolsdef.h"
#include "../vdrawtool.h"
#include "../vmisc/vabstractapplication.h"
#include "../vwidgets/vsimplecurve.h"
#include "ooperationlines.h"

class QUndoCommand;

struct VAbstractOperationInitData : VDrawToolInitData
{
    using VDrawToolInitData::VDrawToolInitData;

    QString suffix{};
    QVector<SourceItem> source{};
    QVector<DestinationItem> destination{};
    QVector<OOperationLine> lines{};

    /** Objects of the source list the layer must leave where they are.
     *
     *  A tool that folds the fabric, or turns it, may build itself ON some of the objects it was given — the
     *  departure line of a pleat is the fold the whole pleat is taken on. Hiding those with the originals would take
     *  them away from the figure the tool draws, so they are never moved into the new layer. */
    QSet<quint32> keptInPlace{};

    QString visibilityGroupName{};
    QStringList visibilityGroupTags{};
    bool hasLinkedVisibilityGroup{false};
};

// FIXME. I don't know how to use QGraphicsItem properly, so just took first available finished class.
// QGraphicsItem itself produce case where clicking on empty space produce call to QGraphicsItem.
// And i don't know how to fix it.
class VAbstractOperation : public VDrawTool, public QGraphicsLineItem
{
    Q_OBJECT // NOLINT
    // Fix warning "Class implements the interface QGraphicsItem but does not list it
    // in Q_INTERFACES. qobject_cast to QGraphicsItem will not work!"
    Q_INTERFACES(QGraphicsItem)

public:
    ~VAbstractOperation() override = default;

    static const QString TagItem;
    static const QString TagSource;
    static const QString TagDestination;

    auto getTagName() const -> QString override;

    auto IsRemovable() const -> RemoveStatus override;

    void SetNotes(const QString &notes) override;

    auto SourceItems() const -> QVector<SourceItem>;

    /** @brief The pen and the colour an operation gives ONE of its own copies.
     *
     * A copy has no element of its own, so what it carries as its own pen is kept where the operation already
     * keeps it: a copied curve on the `<source>` item it comes from, a copied line on its `<lines>` entry. Both
     * are empty for a copied point — a point carries no pen — and @a exists is false for an object this
     * operation did not produce. @a isLine says which of the two lists answered, so a caller offers the line
     * styles rather than the curve pen styles. */
    struct CopyStyle
    {
        QString penStyle{};
        QString color{};
        /// The kind of the copy, so a caller can say WHAT it is styling — "Style de la courbe", "de l'arc" —
        /// rather than printing a technical name. Unknown for a copied line, which is named by `isLine`.
        GOType type{GOType::Unknown};
        bool isLine{false};
        bool exists{false};
    };

    auto CopyStyleOf(quint32 objectId) const -> CopyStyle;

    /** @brief Write that pen back where the file keeps it, as one undoable step, the way every tool option is
     *  saved. Does nothing for an object this operation did not produce. */
    void SetCopyStyle(quint32 objectId, const QString &penStyle, const QString &color);

    void GroupVisibility(quint32 object, bool visible) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void ChangeLabelPosition(quint32 id, const QPointF &pos) override;

    auto IsLabelVisible(quint32 id) const -> bool override;
    void SetLabelVisible(quint32 id, bool visible) override;

    static void ExtractData(const QDomElement &domElement, VAbstractOperationInitData &initData);
    static auto ExtractSourceData(const QDomElement &domElement) -> QVector<SourceItem>;
    static auto ExtractDestinationData(const QDomElement &domElement) -> QVector<DestinationItem>;

    static auto OperationColorsList() -> QMap<QString, QString>;

public slots:
    void FullUpdateFromFile() override;

    void AllowHover(bool enabled) override;
    void AllowSelecting(bool enabled) override;
    void EnableToolMove(bool move) override;

    void AllowPointHover(bool enabled);
    void AllowPointSelecting(bool enabled);

    void AllowPointLabelHover(bool enabled);
    void AllowPointLabelSelecting(bool enabled);

    void AllowSplineHover(bool enabled);
    void AllowSplineSelecting(bool enabled);

    void AllowSplinePathHover(bool enabled);
    void AllowSplinePathSelecting(bool enabled);

    void AllowArcHover(bool enabled);
    void AllowArcSelecting(bool enabled);

    void AllowElArcHover(bool enabled);
    void AllowElArcSelecting(bool enabled);

    void ToolSelectionType(const SelectionType &selectionType) override;
    void Enable() override;
    void ObjectSelected(bool selected, quint32 objId);
    void DeleteFromLabel();
    void LabelChangePosition(const QPointF &pos, quint32 labelId);

protected:
    QVector<SourceItem> m_source;
    QVector<DestinationItem> m_destination;

    /** @brief Lines attached to the operation, each one with the id of the line it produced. A line is
     *  not a geometric object: it is carried by two point ids and rebuilt on every parse. */
    QVector<OOperationLine> m_lines{};

    QMap<quint32, VAbstractSimple *> operatedObjects{};

    bool m_hasLinkedGroup{false};
    QString m_groupName{};
    QStringList m_groupTags{};

    explicit VAbstractOperation(const VAbstractOperationInitData &initData, QGraphicsItem *parent = nullptr);

    void AddToFile() override;
    void RefreshDataInFile() override;
    void ChangeLabelVisibility(quint32 id, bool visible) override;
    void ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement) override;
    void PerformDelete() override;
    void ReadToolAttributes(const QDomElement &domElement) override;
    void SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj) override;

    void UpdateNamePosition(quint32 id, const QPointF &pos) override;

    /** @brief The file entry that carries one destination object: the <item> of <destination> whose idObject is
     *  @a id. Null when the operation has no such entry. */
    auto DestinationItemElement(quint32 id) const -> QDomElement;

    /** @brief Write one destination object's label position into the pattern file. The destination list is
     *  rebuilt whole when the operation is saved (SaveSourceDestination), so the entry is found here by the object
     *  id it already carries. Without this write the position lives in memory alone, and the next parse — an
     *  object creation, an undo, a redo — reapplies the stored value. */
    void WriteDestinationLabelPosition(quint32 id, const QPointF &pos);

    /** @brief Same for the "show the name" flag of a destination object: the toggle has to reach the file too, or
     *  the next parse brings the hidden name back. */
    void WriteDestinationLabelVisibility(quint32 id, bool visible);
    void SaveSourceDestination(QDomElement &tag,
                               const QVector<SourceItem> &source,
                               const QVector<DestinationItem> &destination);

    /** @brief Create the transformed copy of every attached line whose two endpoints have an image.
     *  Called by each operation tool once its destination objects exist. @a fixedPoints lists the points
     *  the transformation leaves in place (rotation centre, mirror axis points): they are their own
     *  image. @a referencedPoints lists the points an operation references instead of copying — the merge
     *  point of a transfer, whose image is the merge point of the host part. Static because the
     *  destination objects are built before the tool instance exists. */
    static void CreateDestinationLines(const VAbstractOperationInitData &initData,
                                       const QVector<quint32> &fixedPoints,
                                       const QHash<quint32, quint32> &referencedPoints = {});

    auto Lines() const -> const QVector<OOperationLine> & { return m_lines; }

    template <typename T> void ShowToolVisualization(bool show);

    template <typename T> void SetDialogVisibilityGroupData(QPointer<T> dialogTool);

    template <typename T> void SaveVisibilityGroupData(QPointer<T> dialogTool);

    void InitCurve(quint32 id, VContainer *data, GOType curveType, SceneObject sceneType);

    template <typename T> static void InitOperationToolConnections(VMainGraphicsScene *scene, T *tool);

    /** @brief The tail every operation shares: record the tool, build it, put it in the scene, register it — and
     *  create its visibility group in the same undo macro as its creation when the dialog asked for one.
     *
     *  Extracted because six operations had copied the same dozen lines, order included, and the order is the
     *  part that matters: the macro is opened before the tool is built, since building it is what pushes the
     *  command the gesture has to undo; and the group is created after the tool is registered, so undoing the
     *  macro takes both away. @a makeTool builds the tool — the constructors are private, so only the operation
     *  itself can — and is called at the one moment that matters, inside the macro. A transfer has its own tail
     *  — what it does inside the macro runs before the record is written — and says so where it stands. */
    template <class ToolClass, class InitData, class Factory>
    static auto AddToolToScene(InitData &initData, Tool toolType, const QString &macroName, Factory makeTool)
        -> ToolClass *;

    void InitOperatedObjects();

    /** @brief The points an operation draws besides the copies of its sources.
     *
     *  A slash-and-spread draws a display of its own, which the base class knows nothing about: it answers with
     *  the ids of its marks here, and the scene items are built exactly as the copies' are: same wiring, same
     *  tooltip, same place in \a operatedObjects, so the layer that hides a copy hides them too. Empty for every
     *  other operation. Called from InitOperatedObjects, on the instance a constructor is building. */
    virtual auto ExtraPointIds() const -> QVector<quint32> { return {}; }

    /** @brief The lines an operation draws besides the ones its sources carry, wired exactly like them: they
     *  are parented to the operation, so they go with it. Empty for every operation but the slash-and-spread. */
    virtual auto ExtraLineIds() const -> QVector<quint32> { return {}; }

    /** @brief The ids of the objects an operation draws besides its copies, read from ITS OWN element: the display
     *  of a slash-and-spread, the figure of a pleat. Only the operation knows which element carries them.
     *
     *  The test that decides between a lite and a full parse needs them (see ProcessOperationToolOptions): turning
     *  a cap off, or the display of the rotations on, takes objects away or brings them back, and nothing else in
     *  the element says so. Empty for an operation that draws nothing of its own. */
    virtual auto DisplayObjectIds(const QDomElement &domElement) const -> QSet<quint32>
    {
        Q_UNUSED(domElement)
        return {};
    }

    /** @brief Build the scene item of one object this operation draws, reading its shape from the container. */
    void InitOperatedObject(quint32 id);

    /** @brief Parent one line this operation produced to the operation item, and wire its two requests. */
    void InitOperatedLine(quint32 id);

    auto ComplexPointToolTip(quint32 itemId) const -> QString;
    auto ComplexCurveToolTip(quint32 itemId) const -> QString;
    auto VisibilityGroupToolTip() const -> QString;

    static void CreateVisibilityGroup(const VAbstractOperationInitData &initData, QUndoCommand *parent = nullptr);

    static void PrepareNames(VAbstractOperationInitData &initData);

    void ProcessOperationToolOptions(const QDomElement &oldDomElement,
                                     const QDomElement &newDomElement,
                                     const QVector<SourceItem> &newSource);

    /** @brief The dialog of this operation, typed. Every operation keeps its options in a dialog of its own,
     *  and the cast with its two assertions is written the same way in all of them. */
    template <class Dialog> auto DialogOf() const -> QPointer<Dialog>;

    /** @brief Show the tool options menu. Only the dialog it opens changes from one operation to the next. */
    template <class Dialog> void ShowContextMenuOf(QGraphicsSceneContextMenuEvent *event, quint32 id);

    /** @brief Write back what a dialog returned: the sources it holds are the ones to write. Same three
     *  assertions in every operation, only the dialog type differs. */
    template <class Dialog>
    void ApplyDialogOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement);

    /** @brief The destinations the operation already holds, by recordId, and whether the source set is
     *  unchanged — in which case there is nothing to recompute and the caller returns early.
     *  @a unchanged is set accordingly. */
    auto SyncDestinationPreamble(const QVector<SourceItem> &source, bool &unchanged) const
        -> QMap<QUuid, DestinationItem>;

    /** @brief The part of a dialog save that every operation writes the same way: the notes, and the legacy
     *  suffix attribute that must not come back into the file. */
    void SaveCommonDialogData(QDomElement &domElement, const QString &notes) const;

private:
    Q_DISABLE_COPY_MOVE(VAbstractOperation) // NOLINT

    void AllowCurveHover(bool enabled, GOType type);
    void AllowCurveSelecting(bool enabled, GOType type);

    auto NeedUpdateVisibilityGroup() const -> bool;
    void HandleVisibilityGroupChange(QUndoCommand *parent);
};

//---------------------------------------------------------------------------------------------------------------------
template <typename T> void VAbstractOperation::SetDialogVisibilityGroupData(QPointer<T> dialogTool)
{
    SCASSERT(not dialogTool.isNull())

    vidtype const group = doc->GroupLinkedToTool(m_id);
    dialogTool->SetGroupCategories(doc->GetGroupCategories());
    if (group != null_id)
    {
        dialogTool->SetHasLinkedVisibilityGroup(true);
        dialogTool->SetVisibilityGroupName(doc->GetGroupName(group));
        dialogTool->SetVisibilityGroupTags(doc->GetGroupTags(group));
    }
    else
    {
        dialogTool->SetHasLinkedVisibilityGroup(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> void VAbstractOperation::SaveVisibilityGroupData(QPointer<T> dialogTool)
{
    SCASSERT(not dialogTool.isNull())

    // Save for later use.
    m_hasLinkedGroup = dialogTool->HasLinkedVisibilityGroup();
    m_groupName = dialogTool->GetVisibilityGroupName();
    m_groupTags = dialogTool->GetVisibilityGroupTags();
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> void VAbstractOperation::ShowToolVisualization(bool show)
{
    if (show)
    {
        if (vis.isNull())
        {
            AddVisualization<T>();
            SetVisualization();
        }
        else
        {
            if (T *visual = qobject_cast<T *>(vis))
            {
                visual->show();
            }
        }
    }
    else
    {
        delete vis.data();
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> void VAbstractOperation::InitOperationToolConnections(VMainGraphicsScene *scene, T *tool)
{
    SCASSERT(scene != nullptr)
    SCASSERT(tool != nullptr)

    InitDrawToolConnections(scene, tool);

    QObject::connect(scene, &VMainGraphicsScene::EnablePointItemHover, tool, &T::AllowPointHover);
    QObject::connect(scene, &VMainGraphicsScene::EnablePointItemSelection, tool, &T::AllowPointSelecting);
    QObject::connect(scene, &VMainGraphicsScene::EnableLabelItemHover, tool, &T::AllowPointLabelHover);
    QObject::connect(scene, &VMainGraphicsScene::EnableLabelItemSelection, tool, &T::AllowPointLabelSelecting);

    QObject::connect(scene, &VMainGraphicsScene::EnableSplineItemHover, tool, &T::AllowSplineHover);
    QObject::connect(scene, &VMainGraphicsScene::EnableSplineItemSelection, tool, &T::AllowSplineSelecting);

    QObject::connect(scene, &VMainGraphicsScene::EnableSplinePathItemHover, tool, &T::AllowSplinePathHover);
    QObject::connect(scene, &VMainGraphicsScene::EnableSplinePathItemSelection, tool, &T::AllowSplinePathSelecting);

    QObject::connect(scene, &VMainGraphicsScene::EnableArcItemHover, tool, &T::AllowArcHover);
    QObject::connect(scene, &VMainGraphicsScene::EnableArcItemSelection, tool, &T::AllowArcSelecting);

    QObject::connect(scene, &VMainGraphicsScene::EnableElArcItemHover, tool, &T::AllowElArcHover);
    QObject::connect(scene, &VMainGraphicsScene::EnableElArcItemSelection, tool, &T::AllowElArcSelecting);
}

//---------------------------------------------------------------------------------------------------------------------
template <class Dialog> auto VAbstractOperation::DialogOf() const -> QPointer<Dialog>
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<Dialog> dialogTool = qobject_cast<Dialog *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    return dialogTool;
}

//---------------------------------------------------------------------------------------------------------------------
template <class Dialog> void VAbstractOperation::ShowContextMenuOf(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    try
    {
        ContextMenu<Dialog>(event, id);
    }
    catch (const VExceptionToolWasDeleted &)
    {
        return; // Leave this method immediately: the tool was deleted while its menu was open
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Dialog>
void VAbstractOperation::ApplyDialogOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ProcessOperationToolOptions(oldDomElement, newDomElement, DialogOf<Dialog>()->GetSourceObjects());
}

//---------------------------------------------------------------------------------------------------------------------
template <class ToolClass, class InitData, class Factory>
auto VAbstractOperation::AddToolToScene(InitData &initData, Tool toolType, const QString &macroName, Factory makeTool)
    -> ToolClass *
{
    const bool linkedGroup = initData.typeCreation == Source::FromGui && initData.hasLinkedVisibilityGroup;

    if (linkedGroup)
    {
        VAbstractApplication::VApp()->getUndoStack()->beginMacro(macroName);
    }

    VAbstractTool::AddRecord(initData.id, toolType, initData.doc);

    auto *tool = makeTool();
    initData.scene->addItem(tool);
    InitOperationToolConnections(initData.scene, tool);
    VAbstractPattern::AddTool(initData.id, tool);

    if (linkedGroup)
    {
        CreateVisibilityGroup(initData);
        VAbstractApplication::VApp()->getUndoStack()->endMacro();
    }

    return tool;
}

#endif // VABSTRACTOPERATION_H
