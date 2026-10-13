/************************************************************************
 **
 **  @file   vmaingraphicsscene.cpp
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

#include "vscenepoint.h"
#include "vmaingraphicsscene.h"

#include <QFile>
#include <QTextStream>

#include <QEvent>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>

#include "global.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VMainGraphicsScene default constructor.
 */
VMainGraphicsScene::VMainGraphicsScene(QObject *parent)
  : QGraphicsScene(parent),
    horScrollBar(0),
    verScrollBar(0),
    _transform(QTransform()),
    scenePos(QPointF())
{
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VMainGraphicsScene constructor.
 * @param sceneRect scene rect.
 * @param parent parent object.
 */
VMainGraphicsScene::VMainGraphicsScene(const QRectF &sceneRect, QObject *parent)
  : QGraphicsScene(sceneRect, parent),
    horScrollBar(0),
    verScrollBar(0),
    _transform(QTransform()),
    scenePos()
{
}

//---------------------------------------------------------------------------------------------------------------------
auto VMainGraphicsScene::ClosestHoverItem(const QPointF &scenePos) const -> QGraphicsItem *
{
    QGraphicsItem *closest = nullptr;
    qreal closestDist = 0;
    const QList<QGraphicsItem *> under = items(scenePos);
    for (auto *item : under)
    {
        if (not item->acceptHoverEvents())
        {
            continue;
        }
        const qreal d = (scenePos - item->sceneBoundingRect().center()).manhattanLength();
        if (closest == nullptr || d < closestDist)
        {
            closestDist = d;
            closest = item;
        }
    }
    return closest;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ItemToRaise item that has to be raised temporarily so it wins the event dispatch.
 *
 * Points built by operation tools (Rotate, Move, Flipping) and by double point tools (TrueDarts) are
 * children of their tool item, not top-level items. Qt stacks a child only among its siblings, so the
 * Z=1 they inherit from VScenePoint never lifts them above a top-level curve at Z=0, and raising such
 * a point has no effect at all. Raise its top-level ancestor instead. When both items share the same
 * top-level ancestor, sibling ordering already applies and the item itself is the one to raise.
 *
 * @param scenePos cursor position in scene coordinates.
 * @return item to raise, or nullptr when nothing has to be raised.
 */
auto VMainGraphicsScene::ItemToRaise(const QPointF &scenePos) const -> QGraphicsItem *
{
    const QList<QGraphicsItem *> under = items(scenePos);
    if (under.size() < 2)
    {
        return nullptr;
    }

    // **Un point, quel qu'il soit, passe avant tout.** Ce n'est pas la distance qui décide quand un point est
    // sous le curseur : le point est ce qu'on vise, et un objet posé dessus — une courbe, la pièce, un tracé — ne
    // peut pas le prendre. `ClosestHoverItem` élisait l'objet dont le **centre** est le plus proche du curseur :
    // une courbe dont le tracé passe par le point gagnait donc le clic, et c'est sa PB qui s'ouvrait (mesuré sur
    // `R2'`, quand Options trouvait bien le point). Un point enfant d'un outil ne se lève pas seul : c'est son
    // item de premier niveau qui monte, comme le reste de cette fonction le fait déjà.
    for (QGraphicsItem *item : under)
    {
        if (dynamic_cast<VScenePoint *>(item) != nullptr)
        {
            return item->topLevelItem();
        }
    }

    QGraphicsItem *closest = ClosestHoverItem(scenePos);
    QGraphicsItem *topmost = under.constFirst();
    if (closest == nullptr || closest == topmost)
    {
        return nullptr;
    }

    return closest->topLevelItem() != topmost->topLevelItem() ? closest->topLevelItem() : closest;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mouseMoveEvent handle mouse move events.
 * @param event mouse move event.
 */
void VMainGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    scenePos = event->scenePos();
    emit mouseMove(event->scenePos());

    // A point must win the hover over any overlapping line/curve/arc, like the click already
    // does (VMainGraphicsView::PrioritizeItems picks the item whose center is closest to the
    // cursor). Temporarily raise the hover-accepting item closest to the cursor so it becomes
    // topmost for the hover dispatch, then restore its Z.
    QGraphicsItem *raisedItem = ItemToRaise(event->scenePos());
    qreal raisedItemZ = 0;
    if (raisedItem != nullptr)
    {
        raisedItemZ = raisedItem->zValue();
        raisedItem->setZValue(raisedItemZ + 2.0);
    }

    QGraphicsScene::mouseMoveEvent(event);

    if (raisedItem != nullptr)
    {
        raisedItem->setZValue(raisedItemZ);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mousePressEvent mouse press events.
 * @param event mouse press event
 */
/**
 * @brief L'item sous @p pos, **un point avant tout**.
 *
 * Un point est ce qu'on vise : un objet posé dessus — une courbe, la pièce, un tracé — ne peut pas le prendre.
 * `itemAt` rendait l'item du dessus par profondeur et, à profondeur égale, le dernier ajouté : le clic gauche
 * ouvrait la courbe qu'un point recouvre, là où le clic droit, lui, trouvait le point. Un seul choix, ici.
 */
auto PickItem(QGraphicsScene *scene, const QPointF &pos, const QTransform &deviceTransform) -> QGraphicsItem *
{
    const QList<QGraphicsItem *> items = scene->items(pos, Qt::IntersectsItemShape, Qt::DescendingOrder,
                                                      deviceTransform);
    for (QGraphicsItem *item : items)
    {
        // Un item invisible ne prend pas le clic : c'est la règle partout (voir VMainGraphicsView::PrioritizeItems).
        if (not item->isVisible())
        {
            continue;
        }

        if (dynamic_cast<VScenePoint *>(item) != nullptr)
        {
            return item;
        }
    }

    return items.isEmpty() ? nullptr : items.first();
}

void VMainGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->type() != QEvent::GraphicsSceneMouseDoubleClick)
    {
        emit MouseLeftPressed();

        QTransform deviceTransform;
        auto *view = qobject_cast<QGraphicsView *>(event->widget());
        if (view != nullptr)
        {
            deviceTransform = view->transform();
        }
        QGraphicsItem *picked = PickItem(this, event->scenePos(), deviceTransform);

        emit ItemByMousePress(picked);
    }

    // Mirror the hover dispatch (mouseMoveEvent): when several items overlap, a point must win
    // the click over an overlapping line/curve/arc. Temporarily raise the hover-accepting item
    // closest to the cursor so it becomes topmost for the press dispatch, then restore its Z.
    // Without this, a child line that ends up topmost swallows the click (it emits no Choosed
    // signal), so the active tool never fires on the point.
    QGraphicsItem *raisedItem = ItemToRaise(event->scenePos());
    qreal raisedItemZ = 0;
    if (raisedItem != nullptr)
    {
        raisedItemZ = raisedItem->zValue();
        raisedItem->setZValue(raisedItemZ + 2.0);
    }

    QGraphicsScene::mousePressEvent(event);

    if (raisedItem != nullptr)
    {
        raisedItem->setZValue(raisedItemZ);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && event->type() != QEvent::GraphicsSceneMouseDoubleClick)
    {
        emit MouseLeftReleased();

        QTransform deviceTransform;
        auto *view = qobject_cast<QGraphicsView *>(event->widget());
        if (view != nullptr)
        {
            deviceTransform = view->transform();
        }
        emit ItemByMouseRelease(itemAt(event->scenePos(), deviceTransform));
    }
    QGraphicsScene::mouseReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::SetAcceptDrop(bool newAcceptDrop)
{
    m_acceptDrop = newAcceptDrop;
}

//---------------------------------------------------------------------------------------------------------------------
auto VMainGraphicsScene::AcceptDrop() const -> bool
{
    return m_acceptDrop;
}

//---------------------------------------------------------------------------------------------------------------------
auto VMainGraphicsScene::IsNonInteractive() const -> bool
{
    return m_nonInteractive;
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::SetNonInteractive(bool nonInteractive)
{
    m_nonInteractive = nonInteractive;
}

//---------------------------------------------------------------------------------------------------------------------
auto VMainGraphicsScene::getScenePos() const -> QPointF
{
    return scenePos;
}

//---------------------------------------------------------------------------------------------------------------------
auto VMainGraphicsScene::VisibleItemsBoundingRect() const -> QRectF
{
    QRectF rect;
    const QList<QGraphicsItem *> qItems = items();
    for (auto *item : qItems)
    {
        if (not item->isVisible())
        {
            continue;
        }
        rect = rect.united(item->sceneBoundingRect());
    }
    return rect;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief transform return view transformation.
 * @return view transformation.
 */
auto VMainGraphicsScene::transform() const -> QTransform
{
    return _transform;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setTransform set view transformation.
 * @param transform view transformation.
 */
void VMainGraphicsScene::setTransform(const QTransform &transform)
{
    _transform = transform;
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::EnableTools()
{
    emit EnableItem();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedItem emit ChoosedObject signal.
 * @param id object id.
 * @param type object scene type.
 */
void VMainGraphicsScene::ChoosedItem(quint32 id, const SceneObject &type)
{
    emit ChoosedObject(id, type);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ChoosedSegmentItem(quint32 id, quint32 point1, quint32 point2)
{
    emit ChoosedSegmentObject(id, point1, point2);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::SelectedItem(bool selected, quint32 object, quint32 tool)
{
    emit SelectedObject(selected, object, tool);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::EnableItemMove(bool move)
{
    emit EnableToolMove(move);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::EnableDetailsMode(bool mode)
{
    emit CurveDetailsMode(mode);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ItemsSelection(const SelectionType &type)
{
    emit ItemSelection(type);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::HighlightItem(quint32 id)
{
    emit HighlightDetail(id);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::UpdatePiecePassmarks()
{
    emit UpdatePassmarks();
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleLabelSelection(bool enabled)
{
    emit EnableLabelItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::TogglePointSelection(bool enabled)
{
    emit EnablePointItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleLineSelection(bool enabled)
{
    emit EnableLineItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleArcSelection(bool enabled)
{
    emit EnableArcItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleElArcSelection(bool enabled)
{
    emit EnableElArcItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleSplineSelection(bool enabled)
{
    emit EnableSplineItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleSplinePathSelection(bool enabled)
{
    emit EnableSplinePathItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleNodeLabelSelection(bool enabled)
{
    emit EnableNodeLabelItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleNodePointSelection(bool enabled)
{
    emit EnableNodePointItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleDetailSelection(bool enabled)
{
    emit EnableDetailItemSelection(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleLabelHover(bool enabled)
{
    emit EnableLabelItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::TogglePointHover(bool enabled)
{
    emit EnablePointItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleLineHover(bool enabled)
{
    emit EnableLineItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleArcHover(bool enabled)
{
    emit EnableArcItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleElArcHover(bool enabled)
{
    emit EnableElArcItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleSplineHover(bool enabled)
{
    emit EnableSplineItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleSplinePathHover(bool enabled)
{
    emit EnableSplinePathItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleNodeLabelHover(bool enabled)
{
    emit EnableNodeLabelItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleNodePointHover(bool enabled)
{
    emit EnableNodePointItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleDetailHover(bool enabled)
{
    emit EnableDetailItemHover(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleArcSegmentVisibility(bool visible)
{
    emit ShowArcSegmentLabel(visible);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleElArcSegmentVisibility(bool visible)
{
    emit ShowElArcSegmentLabel(visible);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleSplineSegmentVisibility(bool visible)
{
    emit ShowSplineSegmentLabel(visible);
}

//---------------------------------------------------------------------------------------------------------------------
void VMainGraphicsScene::ToggleSplinePathSegmentVisibility(bool visible)
{
    emit ShowSplinePathSegmentLabel(visible);
}
