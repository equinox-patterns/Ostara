/************************************************************************
 **
 **  @file   oexportcorrection.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Export mode — correcting a nesting plan by hand.
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

#include "oexportcorrection.h"

#include <QGraphicsItem>
#include <QGraphicsScene>

#include "core/oexportoutput.h"
#include "core/oexportsetplacement.h"
#include "oexportcanvas.h"
#include "oexportfootprint.h"
#include "oexportholder.h"

#include "../vwidgets/vmaingraphicsscene.h"

namespace
{
/// Whether two placements put the footprint in the same place — the test that decides if a
/// gesture did anything at all. A gesture that ends where it began must leave no history and
/// no rebuild, exactly as the annotation items of the other modes decide it.
auto SamePlacement(const OExportPlacement &lhs, const OExportPlacement &rhs) -> bool
{
    return qFuzzyCompare(lhs.x, rhs.x) && qFuzzyCompare(lhs.y, rhs.y)
           && qFuzzyCompare(lhs.rotation, rhs.rotation);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportCorrection::OExportCorrection(VMainGraphicsScene *scene, OExportOutputStore *store,
                                     std::function<void(quint32)> repaint, QObject *parent)
  : QObject(parent), m_scene(scene), m_store(store), m_repaint(std::move(repaint))
{
    Q_ASSERT(m_scene != nullptr);
    Q_ASSERT(m_store != nullptr);

    // The scene reports the last position it saw on every movement, and the item under the
    // cursor on a press. Both are needed and neither is enough: itemAt answers "the topmost
    // item", which on a tiled run is the grid — it covers the whole plan — so the footprint
    // has to be found by walking the stack at a position, and the press signal is the only
    // place the scene offers a position at press time.
    connect(m_scene, &VMainGraphicsScene::mouseMove, this, &OExportCorrection::OnSceneMouseMove);
    connect(m_scene, &VMainGraphicsScene::ItemByMousePress, this, &OExportCorrection::OnItemPressed);

    // The history drives the repaint, never the gesture. A hand correction, an undo and a redo
    // are three different reasons for the store to have changed, and the plan has to be redrawn
    // for all three — so the stack is asked once, here, rather than each path remembering to
    // ask. This is also how every other stack in the application is watched.
    connect(&m_undoStack, &QUndoStack::indexChanged, this, &OExportCorrection::OnStackIndexChanged);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCorrection::UndoStack() -> QUndoStack *
{
    return &m_undoStack;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::SetPlan(const QRectF &plan)
{
    m_plan = plan;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::SetEnabled(bool enabled)
{
    // **Une paire tissu + doublure n'a pas de correction.** Le repère d'un item est l'index de son
    // placement dans le résultat du plan affiché, et les deux nappes ont chacune le leur : le même
    // index désigne une pièce dans LES DEUX, donc un clic en sélectionnerait deux et un geste les
    // déplacerait ensemble. La paire se lit, elle ne se corrige pas — et c'est dit ici, une fois,
    // plutôt que deviné par chaque lecteur du rôle.
    m_enabled = enabled;

    if (not enabled)
    {
        ClearSelection();
        m_undoStack.clear();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCorrection::IsEnabled() const -> bool
{
    return m_enabled;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::SetViewScale(qreal scale)
{
    m_viewScale = scale;
    if (m_holder != nullptr)
    {
        m_holder->SetViewScale(scale);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::OnSceneMouseMove(const QPointF &scenePos)
{
    m_lastScenePos = scenePos;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::OnItemPressed(QGraphicsItem *item)
{
    Q_UNUSED(item)

    // Correction éteinte : le plan est en lecture seule, et une pression ne fait rien du tout —
    // pas même tomber une sélection, qui ne peut pas exister. Voir SetEnabled.
    if (not m_enabled)
    {
        return;
    }

    // A press that lands while a gesture is running belongs to that gesture, not to a new
    // selection: Qt routes it to the item holding the grab, and reaching here would mean the
    // grab was lost.
    if (m_gesturing)
    {
        return;
    }

    // Found from the position rather than taken from the signal's argument. The argument is
    // whatever itemAt returned, and on a tiled run that is the grid — the mode must answer
    // about what is UNDER the cut lines, not about the lines.
    const QList<QGraphicsItem *> under = m_scene->items(m_lastScenePos);
    for (QGraphicsItem *candidate : under)
    {
        // A press on the holder itself keeps the selection. The four handles sit at the corners
        // of the FOOTPRINT, where no piece outline has to reach — the footprint is the union of
        // every size and its rectangle is wider than any one contour — so looking for a role
        // under a handle finds nothing, and answering that with ClearSelection() would destroy
        // the holder in the middle of the very press that was aimed at it.
        if (candidate == m_holder)
        {
            return;
        }

        const QVariant role = candidate->data(kExportFootprintRole);
        if (role.isValid())
        {
            Select(role.toInt());
            return;
        }
    }

    ClearSelection();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCorrection::FollowSet(int index) const -> QList<FollowItem>
{
    QList<FollowItem> items;
    for (QGraphicsItem *item : m_scene->items())
    {
        // The validity test is the whole filter, and it is not defensive: an item with no role
        // answers 0 to toInt(), so an unfiltered read would hand the footprint of index 0 the
        // grid, the paper and the bin as well — and dragging a piece would take the whole
        // drawing with it, leaving an empty viewport until the next rebuild.
        const QVariant role = item->data(kExportFootprintRole);
        if (not role.isValid() || role.toInt() != index)
        {
            continue;
        }

        FollowItem follow;
        follow.item = item;
        follow.baseScene = item->sceneTransform();
        items.append(follow);
    }
    return items;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::Select(int index)
{
    // Rien à sélectionner quand la correction est éteinte — voir SetEnabled : un index ne désigne
    // pas une pièce, mais une pièce par plan.
    if (not m_enabled)
    {
        return;
    }

    const OExportOutput *output = m_store->Find(m_store->ActiveId());
    if (output == nullptr || index < 0 || index >= output->result.placements.size())
    {
        ClearSelection();
        return;
    }

    m_outputId = output->id;
    m_index = index;

    if (m_holder == nullptr)
    {
        m_holder = new OExportHolderItem();
        m_scene->addItem(m_holder);
        m_holder->SetViewScale(m_viewScale);

        connect(m_holder, &OExportHolderItem::MoveStarted, this, &OExportCorrection::OnMoveStarted);
        connect(m_holder, &OExportHolderItem::RotateStarted, this, &OExportCorrection::OnRotateStarted);
        connect(m_holder, &OExportHolderItem::GestureMoved, this, &OExportCorrection::OnGestureMoved);
        connect(m_holder, &OExportHolderItem::GestureFinished, this, &OExportCorrection::OnGestureFinished);
    }

    DressHolder(output->result.placements.at(index));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::ClearSelection()
{
    if (m_holder != nullptr)
    {
        m_scene->removeItem(m_holder);
        delete m_holder;
        m_holder = nullptr;
    }

    m_outputId = 0;
    m_index = -1;
    m_gesturing = false;
    m_rotating = false;
    m_follow.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::DressHolder(const OExportPlacement &placement)
{
    if (m_holder == nullptr)
    {
        return;
    }

    m_holder->SetPlacement(placement);
    m_holder->show();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::OnSceneAboutToBeRebuilt(quint32 outputId)
{
    // The rebuild clears the scene, so anything holding the mouse grab is about to be deleted
    // under it. Abandoning the gesture here is not a nicety: the preview was never written,
    // so dropping it and letting Build draw the stored placement puts the plan back exactly
    // where the model says it is.
    m_gesturing = false;
    m_rotating = false;
    m_follow.clear();

    if (m_outputId != outputId)
    {
        // Another generation is about to be shown. The selection named a footprint of the one
        // leaving, and an index means nothing in a result that has not been computed yet.
        ClearSelection();
        return;
    }

    if (m_holder != nullptr)
    {
        m_scene->removeItem(m_holder);
        delete m_holder;
        m_holder = nullptr;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::OnSceneRebuilt(const OExportOutput &output)
{
    if (m_index < 0 || m_outputId != output.id)
    {
        return;
    }

    if (m_index >= output.result.placements.size())
    {
        ClearSelection();
        return;
    }

    if (m_holder == nullptr)
    {
        m_holder = new OExportHolderItem();
        m_scene->addItem(m_holder);
        m_holder->SetViewScale(m_viewScale);

        connect(m_holder, &OExportHolderItem::MoveStarted, this, &OExportCorrection::OnMoveStarted);
        connect(m_holder, &OExportHolderItem::RotateStarted, this, &OExportCorrection::OnRotateStarted);
        connect(m_holder, &OExportHolderItem::GestureMoved, this, &OExportCorrection::OnGestureMoved);
        connect(m_holder, &OExportHolderItem::GestureFinished, this, &OExportCorrection::OnGestureFinished);
    }

    DressHolder(output.result.placements.at(m_index));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::Reset()
{
    ClearSelection();
    m_undoStack.clear();
    m_lastScenePos = QPointF();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::Deselect()
{
    // A gesture may still be running. Escape does not release the mouse button, so it can arrive
    // in the middle of a drag — and the preview was never written to the store, so the pieces on
    // screen are still standing where the preview put them. Dropping the holder alone would leave
    // them there, showing a plan the model does not agree with, and nothing would repair it
    // until the next gesture.
    //
    // Redrawing from the model is what puts them back. Queued rather than called, for the same
    // reason the gesture path queues it: the rebuild empties the scene, and this may be running
    // from an event whose item is about to stop existing.
    const bool abandoned = m_gesturing;
    const quint32 id = m_store->ActiveId();

    ClearSelection();

    if (abandoned && id != 0)
    {
        QMetaObject::invokeMethod(this, [this, id]() { m_repaint(id); }, Qt::QueuedConnection);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::OnMoveStarted(const QPointF &scenePos)
{
    const OExportOutput *output = m_store->Find(m_outputId);
    if (output == nullptr || m_index < 0 || m_index >= output->result.placements.size())
    {
        return;
    }

    m_start = output->result.placements.at(m_index);
    m_preview = m_start;
    m_pressScene = scenePos;
    m_rotating = false;
    m_gesturing = true;
    m_follow = FollowSet(m_index);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::OnRotateStarted(const QPointF &scenePos)
{
    OnMoveStarted(scenePos);
    if (not m_gesturing)
    {
        return;
    }

    // The pivot is where the footprint is NOW, not where the generation first put it: a turn
    // after a move must turn the piece about its own centre as the user sees it. Anchoring on
    // the original point would send the piece orbiting on the second gesture.
    m_rotating = true;
    m_startCentre = OExportFootprint::Centre(m_start);
    m_pressAngle = OExportFootprint::AngleOf(m_startCentre, scenePos);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::OnGestureMoved(const QPointF &scenePos)
{
    if (not m_gesturing)
    {
        return;
    }

    if (m_rotating)
    {
        const qreal delta = OExportFootprint::AngleOf(m_startCentre, scenePos) - m_pressAngle;
        Preview(OExportFootprint::Rotated(m_start, delta, m_plan));
        return;
    }

    const QPointF delta = scenePos - m_pressScene;
    Preview(OExportFootprint::Moved(m_start, delta, m_plan));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::Preview(const OExportPlacement &placement)
{
    m_preview = placement;

    // One rigid motion, computed from the placement the gesture started at rather than
    // accumulated from mouse movements — so a long drag cannot drift. Expressed as the
    // difference of two placement frames, it covers a translation, a turn and the slide a
    // clamp adds afterwards without any of them being a special case.
    //
    // The order matters and is not the obvious one: QTransform composes for ROW vectors, so in
    // `A * B` it is A that is applied first. Taking the geometry from start to preview means
    // undoing start, then applying preview — hence start⁻¹ on the LEFT. Written the other way
    // round the two agree whenever the rotation is zero, because translations commute, and come
    // apart as soon as the nesting has turned the piece. Measured, not reasoned: with a frame
    // turned 90° and moved 5 units, the right order lands on the preview and the wrong one lands
    // 7 units away.
    const QTransform delta =
        OExportFootprint::Matrix(m_start).inverted() * OExportFootprint::Matrix(m_preview);

    for (const FollowItem &follow : m_follow)
    {
        OExportFootprint::ApplySceneDelta(follow.item, follow.baseScene, delta);
    }

    DressHolder(m_preview);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::OnGestureFinished()
{
    if (not m_gesturing)
    {
        return;
    }

    // Read before it is cleared: it decides the wording of the history entry, which the menu
    // shows as "Undo Move a piece on the plan" or "Undo Turn a piece on the plan".
    const bool turned = m_rotating;

    m_gesturing = false;
    m_rotating = false;
    m_follow.clear();

    if (SamePlacement(m_preview, m_start))
    {
        return;
    }

    // A gesture that ended where it began leaves no history and no rebuild, the way the
    // annotation items of the other modes decide it.
    Commit(m_preview, turned);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::Commit(const OExportPlacement &placement, bool turned)
{
    // The store is NOT written here. QUndoStack::push() runs the command's redo() straight away,
    // so handing the command over IS the write — doing both would be the same write twice, and
    // the day one of them was changed the other would quietly disagree with it.
    //
    // `m_start` is the placement before the gesture, read again from the model at press time,
    // and not `m_preview` moved back: it is the model's own value, so an undo returns to what
    // the plan actually held rather than to what this code believes it held.
    m_undoStack.push(
        new OExportSetPlacement(m_store, m_outputId, m_index, m_start, placement, turned));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCorrection::OnStackIndexChanged()
{
    // The ACTIVE generation rather than the selected one. After an undo the user may have
    // selected nothing at all, and it is still the plan on screen that has to be redrawn. The
    // callback the window supplied already no-ops when the mode has changed or when the id is
    // not the one displayed, so neither guard is repeated here.
    const quint32 id = m_store->ActiveId();
    if (id == 0)
    {
        return;
    }

    QMetaObject::invokeMethod(this, [this, id]() { m_repaint(id); }, Qt::QueuedConnection);
}
