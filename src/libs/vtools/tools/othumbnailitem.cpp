/************************************************************************
 **
 **  @file   othumbnailitem.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  On-piece piece miniature block (see othumbnailitem.h).
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

#include "othumbnailitem.h"

#include <QApplication>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtGlobal>

#include "../../vmisc/vabstractapplication.h"
#include "../../vwidgets/global.h"
#include "../../vwidgets/ocartouchepainter.h"

//---------------------------------------------------------------------------------------------------------------------
OThumbnailItem::OThumbnailItem(QGraphicsItem *parent) : QGraphicsRectItem(parent)
{
    setZValue(10001);  // above the information blocks (z = block index, 1..11)
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);
    setAcceptHoverEvents(true);
    setCursor(Qt::SizeAllCursor);
    setPen(Qt::NoPen);
    setBrush(Qt::NoBrush);
    hide();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::SetContent(const QPainterPath &content, const QRectF &contentBounds)
{
    m_content = content;
    m_contentBounds = contentBounds;
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::SetBlockGeom(const OThumbnailGeom &geom)
{
    setPos(geom.pos);
    setRect(0.0, 0.0, geom.width, geom.height);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::SetReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    setFlag(QGraphicsItem::ItemIsMovable, not readOnly);
    setAcceptedMouseButtons(readOnly ? Qt::NoButton : Qt::AllButtons);
    if (readOnly)
    {
        setActive(false);
        unsetCursor();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::SetActive(bool active)
{
    if (m_active == active)
    {
        return;
    }
    m_active = active;
    update();
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::IsActive() const -> bool
{
    return m_active;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::boundingRect() const -> QRectF
{
    // Handles are painted just outside the rect, and the hit area is wider still.
    return rect().adjusted(-kHandleHitHalf, -kHandleHitHalf, kHandleHitHalf, kHandleHitHalf);
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::shape() const -> QPainterPath
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    const QRectF r = rect();

    // Theme-aware colour (the app palette follows the Light/Dark theme), so the miniature matches
    // the rest of the canvas.
    const QColor textColor = QApplication::palette().color(QPalette::Text);

    if (not m_content.isEmpty() && not m_contentBounds.isEmpty())
    {
        // Clipped to the item's bounding rect, not to the block rect: the drawing can still never
        // paint outside boundingRect (otherwise moving the block would leave trails), but the stroke
        // stays whole. Fitting the drawing to the block rect lays the outline exactly on that rect's
        // edges, so clipping there shaves off the outer half of every stroke running along the
        // bounding box — on a piece that is the straight hem, and only it, over its whole length
        // (the top and sides merely touch the box at a tangent point). The half stroke allowed back
        // is half a pixel, far inside the 10-unit margin boundingRect() already reserves.
        painter->save();
        painter->setClipRect(boundingRect());
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QTransform fit = OCartouchePainter::FitTransform(m_contentBounds, r);
        painter->setTransform(fit, true);

        // Both scales are read off the painter: `fit` maps the drawing into the block rect, and the
        // painter transform adds the view zoom on top of it. So `viewScale` is the canvas zoom, and
        // dividing by the full scale lands the requested number of pixels on screen whatever the
        // size of the block.
        const qreal scale = painter->transform().m11();
        const qreal fitScale = fit.m11();
        const qreal viewScale = fitScale > 0.0 ? scale / fitScale : 1.0;

        // The miniature never carries a heavier line than the piece it draws, and never more than one
        // pixel. The pieces' own stroke shrinks below 100 % zoom (`ScaleWidth()` only compensates
        // above 1), so a stroke pinned at one pixel reads heavier and heavier as the canvas shrinks.
        // Mirroring the pieces' law keeps the miniature in step with them at every zoom, while the
        // one pixel ceiling keeps the drawing light where the piece's line is at full strength.
        const qreal hairLine = VAbstractApplication::VApp()->Settings()->WidthHairLine();
        const qreal stroke = qMin(1.0, ScaleWidth(hairLine, viewScale) * viewScale);

        QPen pen(textColor);
        pen.setWidthF(scale > 0.0 ? stroke / scale : stroke);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(m_content);
        painter->restore();
    }

    // Light dashed outline + 8 filled square handles only when the block is active (clicked), like
    // the information blocks — otherwise only the drawing is visible.
    if (m_active)
    {
        QColor borderColor = textColor;
        borderColor.setAlpha(130);
        painter->setPen(QPen(borderColor, 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);

        painter->setBrush(QBrush(textColor));
        painter->setPen(Qt::NoPen);
        const QVector<QPointF> handles{r.topLeft(),
                                       r.topRight(),
                                       r.bottomLeft(),
                                       r.bottomRight(),
                                       QPointF(r.center().x(), r.top()),
                                       QPointF(r.center().x(), r.bottom()),
                                       QPointF(r.left(), r.center().y()),
                                       QPointF(r.right(), r.center().y())};
        for (const QPointF &handle : handles)
        {
            painter->drawRect(QRectF(handle.x() - kHandleHalf, handle.y() - kHandleHalf, 2.0 * kHandleHalf,
                                     2.0 * kHandleHalf));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::HitTestHandle(const QPointF &pos) const -> Handle
{
    if (m_readOnly)
    {
        return Handle::None;
    }

    const QRectF r = rect();
    const bool nearLeft = qAbs(pos.x() - r.left()) <= kHandleHitHalf;
    const bool nearRight = qAbs(pos.x() - r.right()) <= kHandleHitHalf;
    const bool nearTop = qAbs(pos.y() - r.top()) <= kHandleHitHalf;
    const bool nearBottom = qAbs(pos.y() - r.bottom()) <= kHandleHitHalf;

    if (nearLeft && nearTop)
    {
        return Handle::TopLeft;
    }
    if (nearRight && nearTop)
    {
        return Handle::TopRight;
    }
    if (nearLeft && nearBottom)
    {
        return Handle::BottomLeft;
    }
    if (nearRight && nearBottom)
    {
        return Handle::BottomRight;
    }
    if (nearTop)
    {
        return Handle::Top;
    }
    if (nearBottom)
    {
        return Handle::Bottom;
    }
    if (nearLeft)
    {
        return Handle::Left;
    }
    if (nearRight)
    {
        return Handle::Right;
    }
    return Handle::None;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::CursorForHandle(Handle handle) -> Qt::CursorShape
{
    switch (handle)
    {
        case Handle::TopLeft:
        case Handle::BottomRight:
            return Qt::SizeFDiagCursor;
        case Handle::TopRight:
        case Handle::BottomLeft:
            return Qt::SizeBDiagCursor;
        case Handle::Top:
        case Handle::Bottom:
            return Qt::SizeVerCursor;
        case Handle::Left:
        case Handle::Right:
            return Qt::SizeHorCursor;
        case Handle::None:
            break;
    }
    return Qt::SizeAllCursor;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::ResizeTo(const QPointF &mouseInParent, QRectF &newRect, QPointF &newPos) const
{
    const QRectF start = m_dragStartRect;
    const QPointF anchorTopLeft = m_dragStartPos;
    const QPointF anchorBottomRight = m_dragStartPos + QPointF(start.width(), start.height());
    const qreal aspect = start.height() > 0.0 ? start.width() / start.height() : 1.0;

    qreal newW = start.width();
    qreal newH = start.height();

    switch (m_activeHandle)
    {
        case Handle::BottomRight:
        case Handle::TopRight:
        case Handle::Right:
            newW = mouseInParent.x() - anchorTopLeft.x();
            break;
        case Handle::TopLeft:
        case Handle::BottomLeft:
        case Handle::Left:
            newW = anchorBottomRight.x() - mouseInParent.x();
            break;
        case Handle::Bottom:
            newH = mouseInParent.y() - anchorTopLeft.y();
            break;
        case Handle::Top:
            newH = anchorBottomRight.y() - mouseInParent.y();
            break;
        case Handle::None:
            return;
    }

    // The aspect ratio is locked: the leading axis decides, the other follows, and neither can
    // shrink below the minimum.
    if (m_activeHandle == Handle::Bottom || m_activeHandle == Handle::Top)
    {
        newH = qMax(newH, kMinSize);
        newW = newH * aspect;
    }
    else
    {
        newW = qMax(newW, kMinSize);
        newH = newW / aspect;
    }

    switch (m_activeHandle)
    {
        case Handle::BottomRight:
            newPos = anchorTopLeft;
            break;
        case Handle::TopRight:
            newPos = QPointF(anchorTopLeft.x(), anchorBottomRight.y() - newH);
            break;
        case Handle::TopLeft:
            newPos = anchorBottomRight - QPointF(newW, newH);
            break;
        case Handle::BottomLeft:
            newPos = QPointF(anchorBottomRight.x() - newW, anchorTopLeft.y());
            break;
        case Handle::Right:
            newPos = QPointF(anchorTopLeft.x(), anchorTopLeft.y() + (start.height() - newH) / 2.0);
            break;
        case Handle::Left:
            newPos = QPointF(anchorBottomRight.x() - newW, anchorTopLeft.y() + (start.height() - newH) / 2.0);
            break;
        case Handle::Bottom:
            newPos = QPointF(anchorTopLeft.x() + (start.width() - newW) / 2.0, anchorTopLeft.y());
            break;
        case Handle::Top:
            newPos = QPointF(anchorTopLeft.x() + (start.width() - newW) / 2.0, anchorBottomRight.y() - newH);
            break;
        case Handle::None:
            return;
    }

    newRect = QRectF(0.0, 0.0, newW, newH);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_readOnly || event->button() != Qt::LeftButton)
    {
        event->ignore();
        return;
    }

    m_activeHandle = HitTestHandle(event->pos());
    m_dragStartScenePos = event->scenePos();
    m_dragStartPos = pos();
    m_dragStartRect = rect();
    m_geometryChanged = false;

    if (m_activeHandle != Handle::None)
    {
        setCursor(CursorForHandle(m_activeHandle));
    }
    else
    {
        setCursor(Qt::ClosedHandCursor);
    }
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_readOnly || (event->buttons() & Qt::LeftButton) == 0)
    {
        event->ignore();
        return;
    }

    if (m_activeHandle != Handle::None)
    {
        const QPointF mouseInParent = parentItem() != nullptr ? parentItem()->mapFromScene(event->scenePos())
                                                             : event->scenePos();
        QRectF newRect;
        QPointF newPos;
        ResizeTo(mouseInParent, newRect, newPos);
        setPos(newPos);
        setRect(newRect);
        m_geometryChanged = true;
    }
    else
    {
        setPos(m_dragStartPos + (event->scenePos() - m_dragStartScenePos));
        m_geometryChanged = true;
    }
    update();
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_readOnly || event->button() != Qt::LeftButton)
    {
        event->ignore();
        return;
    }

    const bool resized = m_activeHandle != Handle::None;
    const bool moved = pos() != m_dragStartPos;
    m_activeHandle = Handle::None;
    setCursor(Qt::SizeAllCursor);
    event->accept();

    if (m_geometryChanged && (resized || moved))
    {
        // One signal, one undo step: a resize moves the block when it pins the opposite side.
        emit GeometryChanged(pos(), rect().width(), rect().height());
    }
    m_geometryChanged = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (not m_readOnly)
    {
        setCursor(CursorForHandle(HitTestHandle(event->pos())));
    }
    QGraphicsRectItem::hoverMoveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (not m_readOnly)
    {
        setCursor(Qt::SizeAllCursor);
    }
    QGraphicsRectItem::hoverLeaveEvent(event);
}
