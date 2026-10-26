/************************************************************************
 **
 **  @file   oexportholder.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Export mode — the frame a hand drags a footprint by.
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

#include "oexportholder.h"

#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyleOptionGraphicsItem>

#include "mainwindow_helpers.h"

#include "../vmisc/def.h"
#include "../vmisc/literals.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"

namespace
{
/// The holder's ink. The paper under it is always white — the canvas paints the bin and the
/// sheets in white — so one fixed colour reads everywhere, and the plan's own lines are black,
/// which is what the holder has to be told apart from. The same reasoning as the grid's pen,
/// which is a fixed black for the same reason.
const QColor kHolderColor = QColor(0, 122, 204);
}

//---------------------------------------------------------------------------------------------------------------------
OExportHolderItem::OExportHolderItem(QGraphicsItem *parent)
  : QGraphicsRectItem(parent)
{
    // Movable and selectable are both false on purpose: this item never moves itself and is
    // never part of a rubber band. It reports gestures and the controller applies them, so
    // there is exactly one place where a move can be refused.
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);

    // Above the cut lines. The scene raises the item under the mouse on hover — a cut line
    // lifting itself over the holder would draw the sheet edge across the thing being dragged.
    setZValue(10.0);

    setPen(Qt::NoPen);
    setBrush(Qt::NoBrush);
    setCursor(BodyCursor());
    hide();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportHolderItem::type() const -> int
{
    return Type;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportHolderItem::SetPlacement(const OExportPlacement &placement)
{
    // The rect is not a bounding box of anything: it IS the footprint, in the footprint's own
    // frame. See the note on the class.
    setRect(QRectF(0.0, 0.0, placement.extentX, placement.extentY));
    setPos(placement.x, placement.y);
    setRotation(placement.rotation);
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportHolderItem::SetViewScale(qreal scale)
{
    if (scale <= 0.0 || qFuzzyCompare(m_viewScale, scale))
    {
        return;
    }

    m_viewScale = scale;
    prepareGeometryChange();
    update();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportHolderItem::boundingRect() const -> QRectF
{
    // Widened by the grab area, not by the paint: without it the margin around a corner is
    // outside the item, and the click there lands on whatever is under it — the grid, or
    // another piece — instead of starting the turn.
    const qreal half = kHandleHitHalfPx / m_viewScale;
    return rect().adjusted(-half, -half, half, half);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportHolderItem::shape() const -> QPainterPath
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportHolderItem::HandleCentre(Handle handle) const -> QPointF
{
    const QRectF r = rect();
    switch (handle)
    {
        case Handle::TopLeft:
            return r.topLeft();
        case Handle::TopRight:
            return r.topRight();
        case Handle::BottomRight:
            return r.bottomRight();
        case Handle::BottomLeft:
            return r.bottomLeft();
        case Handle::None:
            break;
    }
    return r.center();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportHolderItem::HitTestHandle(const QPointF &localPos) const -> Handle
{
    const qreal half = kHandleHitHalfPx / m_viewScale;
    const Handle corners[] = {Handle::TopLeft, Handle::TopRight, Handle::BottomRight,
                              Handle::BottomLeft};

    for (Handle handle : corners)
    {
        const QPointF centre = HandleCentre(handle);
        if (qAbs(localPos.x() - centre.x()) <= half && qAbs(localPos.y() - centre.y()) <= half)
        {
            return handle;
        }
    }
    return Handle::None; // the body
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportHolderItem::BodyCursor() const -> QCursor
{
    // The project's own gesture for moving something: the same pair VToolSeamAllowance shows
    // while a piece is dragged. It used to be gated on the pointer mode, which no longer exists.
    //
    // Through the cache, which is also what strips an ICC profile macOS refuses — the reason
    // QPixmapFromCache exists at all.
    return QCursor(QPixmapFromCache(m_dragging ? cursorArrowCloseHand : cursorArrowOpenHand), 1, 1);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportHolderItem::HandleCursor() const -> QCursor
{
    // Deliberately NOT subject to the tool-glyph preference above. That preference is about
    // the pointer wearing the active tool's icon; this corner has no other way of saying that
    // it turns, and Qt has no rotation cursor to fall back on — the four-arrow shape means
    // "move", which is precisely what a corner does not do.
    const QString path = ResolveToolCursorResource(QStringLiteral("toolcursor"),
                                                   QStringLiteral("rotation_cursor.png"));
    return QCursor(QPixmapFromCache(path), 2, 2);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportHolderItem::ApplyCursor(Handle handle)
{
    setCursor(handle == Handle::None ? BodyCursor() : HandleCursor());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportHolderItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    const qreal half = kHandleHalfPx / m_viewScale;

    // Width 0 is a cosmetic pen: one device pixel whatever the zoom, so the outline stays a
    // hairline instead of thickening as the drawing is enlarged.
    QPen outline(kHolderColor, 0.0, Qt::DashLine);
    painter->setPen(outline);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect());

    painter->setPen(Qt::NoPen);
    painter->setBrush(kHolderColor);
    const qreal side = 2.0 * half;
    for (Handle handle : {Handle::TopLeft, Handle::TopRight, Handle::BottomRight,
                          Handle::BottomLeft})
    {
        const QPointF centre = HandleCentre(handle);
        painter->drawRect(QRectF(centre.x() - half, centre.y() - half, side, side));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportHolderItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    ApplyCursor(HitTestHandle(event->pos()));
    QGraphicsRectItem::hoverMoveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportHolderItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    setCursor(BodyCursor());
    QGraphicsRectItem::hoverLeaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportHolderItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
    {
        QGraphicsRectItem::mousePressEvent(event);
        return;
    }

    m_handle = HitTestHandle(event->pos());
    m_dragging = true;
    setCursor(m_handle == Handle::None ? BodyCursor() : HandleCursor());
    event->accept();

    if (m_handle == Handle::None)
    {
        emit MoveStarted(event->scenePos());
    }
    else
    {
        emit RotateStarted(event->scenePos());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportHolderItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (not m_dragging)
    {
        QGraphicsRectItem::mouseMoveEvent(event);
        return;
    }

    // The item stays where the controller puts it: a gesture is previewed by recomputing the
    // placement from the mouse's position, never by accumulating a delta onto the item — which
    // is what keeps the preview and the eventual model write from drifting apart.
    emit GestureMoved(event->scenePos());
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportHolderItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (not m_dragging)
    {
        QGraphicsRectItem::mouseReleaseEvent(event);
        return;
    }

    m_dragging = false;
    m_handle = Handle::None;
    setCursor(BodyCursor());
    event->accept();
    emit GestureFinished();
}
