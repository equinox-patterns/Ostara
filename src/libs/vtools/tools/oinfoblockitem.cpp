/************************************************************************
 **
 **  @file   oinfoblockitem.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  On-piece information block (see oinfoblockitem.h).
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

#include "oinfoblockitem.h"

#include <QApplication>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QStyleOptionGraphicsItem>

#include <algorithm>

namespace
{
// Scale the text lines by fontScale (fonts without an explicit pixel size keep
// their size — they are rare and their relative layout still follows the box).
QVector<OCartoucheRenderLine> ScaledLines(const QVector<OCartoucheRenderLine> &lines, qreal fontScale)
{
    QVector<OCartoucheRenderLine> scaled = lines;
    for (OCartoucheRenderLine &line : scaled)
    {
        if (line.kind == OCartoucheRenderLine::Kind::Text && line.font.pixelSize() > 0)
        {
            QFont font = line.font;
            font.setPixelSize(qRound(font.pixelSize() * fontScale));
            line.font = font;
        }
    }
    return scaled;
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
OInfoBlockItem::OInfoBlockItem(int blockIndex, QGraphicsItem *parent)
  : QGraphicsRectItem(parent), m_blockIndex(blockIndex)
{
    setFlags(ItemIsMovable | ItemSendsGeometryChanges | ItemIsFocusable);
    setAcceptHoverEvents(true);
    setCursor(Qt::SizeAllCursor);
    setZValue(m_blockIndex);  // higher blocks stack above lower ones by default
    hide();
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::SetContent(const QVector<OCartoucheRenderLine> &lines, qreal fontScale)
{
    m_lines = lines;
    m_fontScale = fontScale;
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::SetBlockGeom(const OInfoBlockGeom &geom)
{
    setPos(geom.pos);
    setRotation(-geom.rotation);
    setRect(0, 0, geom.width, geom.height);
    m_fontScale = geom.fontScale;
    update();
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockItem::BlockIndex() const -> int
{
    return m_blockIndex;
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::SetReadOnly(bool readOnly)
{
    setFlag(ItemIsMovable, not readOnly);
    setAcceptHoverEvents(not readOnly);
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::SetActive(bool active)
{
    if (m_active == active)
    {
        return;
    }
    m_active = active;
    // The active block is brought to the front so its handles are reachable even
    // over overlapping blocks; on deactivation it returns to its index z-value.
    setZValue(active ? 1000 + m_blockIndex : m_blockIndex);
    update();
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockItem::IsActive() const -> bool
{
    return m_active;
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockItem::boundingRect() const -> QRectF
{
    // Expanded to the capture-zone radius so clicks in the invisible handle zone
    // (beyond the painted handles) are picked up by the scene.
    return QGraphicsRectItem::boundingRect().adjusted(-kHandleHitHalf, -kHandleHitHalf, kHandleHitHalf,
                                                      kHandleHitHalf);
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockItem::shape() const -> QPainterPath
{
    // Include the (invisible, doubled) handle capture zones so clicks on them hit
    // the item instead of falling through to the piece below.
    QPainterPath path;
    path.addRect(rect().adjusted(-kHandleHitHalf, -kHandleHitHalf, kHandleHitHalf, kHandleHitHalf));
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    const QRectF r = rect();

    // Theme-aware text color (the app palette follows the Light/Dark theme), so
    // the block content matches the editor preview.
    const QColor textColor = QApplication::palette().color(QPalette::Text);

    // Content, clipped to the block rect so overflow can never paint outside the
    // item's boundingRect (otherwise moving the block leaves trails). The scale
    // factor is passed to the painter so the inter-line gap, the line-spacing
    // option and the logo all scale proportionally with the block.
    painter->save();
    painter->setClipRect(r);
    painter->setPen(QPen(textColor, 1));
    if (qFuzzyCompare(m_fontScale, 1.0))
    {
        OCartouchePainter::Paint(*painter, r, m_lines);
    }
    else
    {
        OCartouchePainter::Paint(*painter, r, ScaledLines(m_lines, m_fontScale), m_fontScale);
    }
    painter->restore();

    // Light dashed outline + 8 filled square handles only when the block is active
    // (clicked for editing) — otherwise only the content is visible.
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
auto OInfoBlockItem::HitTestHandle(const QPointF &localPos) const -> Handle
{
    const QRectF r = rect();
    const QPointF centers[] = {r.topLeft(),
                               r.topRight(),
                               r.bottomLeft(),
                               r.bottomRight(),
                               QPointF(r.center().x(), r.top()),
                               QPointF(r.center().x(), r.bottom()),
                               QPointF(r.left(), r.center().y()),
                               QPointF(r.right(), r.center().y())};
    const Handle handles[] = {Handle::TopLeft,   Handle::TopRight,  Handle::BottomLeft, Handle::BottomRight,
                              Handle::Top,       Handle::Bottom,    Handle::Left,       Handle::Right};
    for (int i = 0; i < 8; ++i)
    {
        if (qAbs(localPos.x() - centers[i].x()) <= kHandleHitHalf
            && qAbs(localPos.y() - centers[i].y()) <= kHandleHitHalf)
        {
            return handles[i];
        }
    }
    return Handle::None;
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockItem::CursorForHandle(Handle handle) -> Qt::CursorShape
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
        default:
            return Qt::SizeAllCursor;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    setCursor(CursorForHandle(HitTestHandle(event->pos())));
    QGraphicsRectItem::hoverMoveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    setCursor(Qt::SizeAllCursor);
    QGraphicsRectItem::hoverLeaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !(flags() & ItemIsMovable))
    {
        QGraphicsRectItem::mousePressEvent(event);
        return;
    }

    m_activeHandle = HitTestHandle(event->pos());
    if (m_activeHandle != Handle::None)
    {
        // Resize: every handle scales the block proportionally (both dimensions
        // and the content). The opposite corner / edge midpoint is the fixed anchor.
        m_origW = rect().width();
        m_origH = rect().height();
        switch (m_activeHandle)
        {
            case Handle::TopLeft:
                m_anchorLocal = rect().bottomRight();
                break;
            case Handle::TopRight:
                m_anchorLocal = rect().bottomLeft();
                break;
            case Handle::BottomLeft:
                m_anchorLocal = rect().topRight();
                break;
            case Handle::BottomRight:
                m_anchorLocal = rect().topLeft();
                break;
            case Handle::Top:
                m_anchorLocal = QPointF(rect().center().x(), rect().bottom());
                break;
            case Handle::Bottom:
                m_anchorLocal = QPointF(rect().center().x(), rect().top());
                break;
            case Handle::Left:
                m_anchorLocal = QPointF(rect().right(), rect().center().y());
                break;
            case Handle::Right:
                m_anchorLocal = QPointF(rect().left(), rect().center().y());
                break;
            default:
                break;
        }
        m_dragStartScene = event->scenePos();
        m_dragStartPos = pos();
        m_previewW = m_origW;
        m_previewH = m_origH;
        event->accept();
        return;
    }

    // Move: handled manually (the default would drag the selected piece instead
    // of this child). Keep the grab by accepting the press.
    m_activeHandle = Handle::None;
    m_dragStartScene = event->scenePos();
    m_dragStartPos = pos();
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_activeHandle != Handle::None)
    {
        // Drag delta, sign per handle.
        const QPointF delta = event->scenePos() - m_dragStartScene;
        const int sx = (m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::BottomLeft
                        || m_activeHandle == Handle::Left)
                           ? -1
                           : 1;
        const int sy = (m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::TopRight
                        || m_activeHandle == Handle::Top)
                           ? -1
                           : 1;
        qreal newW = m_origW + sx * delta.x();
        qreal newH = m_origH + sy * delta.y();

        // Every handle scales the block proportionally. The ratio is driven by
        // the handle's own axis only (no max/min flip on the other axis, which
        // would make a shrink flicker or rebound).
        qreal ratio;
        if (m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::TopRight
            || m_activeHandle == Handle::BottomLeft || m_activeHandle == Handle::BottomRight)
        {
            // Corner: the dragged corner drives both axes together.
            if (newW <= 0.0 || newH <= 0.0)
            {
                return;
            }
            ratio = (std::max)(newW / m_origW, newH / m_origH);
        }
        else if (m_activeHandle == Handle::Top || m_activeHandle == Handle::Bottom)
        {
            if (newH <= 0.0)
            {
                return;
            }
            ratio = newH / m_origH;  // the height drives, the width follows
        }
        else
        {
            if (newW <= 0.0)
            {
                return;
            }
            ratio = newW / m_origW;  // the width drives, the height follows
        }
        ratio = (std::max)(ratio, kMinSize / (std::max)(m_origW, 1.0));
        ratio = (std::max)(ratio, kMinSize / (std::max)(m_origH, 1.0));
        m_previewW = m_origW * ratio;
        m_previewH = m_origH * ratio;

        // Live preview: scale the whole item and keep the anchor fixed. The
        // position is recomputed from the press-time position every event, so
        // the anchor never drifts across successive move events.
        const qreal sxT = m_previewW / m_origW;
        const qreal syT = m_previewH / m_origH;
        setTransform(QTransform().fromScale(sxT, syT));
        const QPointF anchorLocal = m_anchorLocal;
        setPos(m_dragStartPos + anchorLocal - QPointF(sxT * anchorLocal.x(), syT * anchorLocal.y()));
        event->accept();
        return;
    }

    // Move: manual delta — never the base class, which would drag the selected
    // piece (the block's movable ancestor) instead of the block.
    setPos(m_dragStartPos + (event->scenePos() - m_dragStartScene));
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_activeHandle != Handle::None)
    {
        const qreal finalW = m_previewW;
        const qreal finalH = m_previewH;
        setTransform(QTransform());
        m_activeHandle = Handle::None;
        if (qAbs(finalW - m_origW) > kMinResizeDelta || qAbs(finalH - m_origH) > kMinResizeDelta)
        {
            setRect(0, 0, finalW, finalH);
            emit Resized(m_blockIndex, finalW, finalH, m_origW, m_origH);
        }
        event->accept();
        return;
    }

    if (pos() != m_dragStartPos)
    {
        emit Moved(m_blockIndex, pos());
    }
    event->accept();
}
