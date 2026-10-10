/************************************************************************
 **
 **  @file   oannotationitem.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  On-piece annotation (see oannotationitem.h).
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

#include "oannotationitem.h"

#include "../../vgeometry/ocontainment.h"

#include <QApplication>
#include <QFontInfo>
#include <QFontMetricsF>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QStyleOptionGraphicsItem>
#include <QtMath>

#include <algorithm>
#include <cmath>

//---------------------------------------------------------------------------------------------------------------------
OAnnotationItem::OAnnotationItem(int index, QGraphicsItem *parent)
  : QGraphicsRectItem(parent), m_index(index)
{
    setFlags(ItemIsMovable | ItemSendsGeometryChanges | ItemIsFocusable);
    setAcceptHoverEvents(true);
    setCursor(Qt::SizeAllCursor);
    setZValue(m_index);
    hide();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::SetIndex(int index)
{
    m_index = index;
    setZValue(m_active ? 1000 + index : index);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::SetContent(const QString &text, const OAnnotationRenderStyle &style)
{
    m_text = text;
    m_style = style;
    m_hasBackground = !style.backgroundColor.isEmpty() && QColor(style.backgroundColor).isValid();

    // Natural content size at fontScale 1.0; the box follows it linearly.
    QFontMetricsF const fm(ResolveFont());
    const QRectF br = fm.boundingRect(QRectF(0, 0, 0, 0), Qt::AlignLeft, m_text);
    m_naturalSize = QSizeF(br.width(), br.height());
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::SetAnnotationGeom(const OAnnotationGeom &geom, qreal rotation)
{
    m_fontScale = geom.fontScale > 0.0 ? geom.fontScale : 1.0;
    setPos(geom.pos);
    setRotation(-rotation);

    // Box: explicit when stored, otherwise the natural content size. The box
    // never goes below the content (spec §10) — a later text edit follows it.
    qreal w = geom.boxWidth;
    qreal h = geom.boxHeight;
    const qreal contentW = m_naturalSize.width() * m_fontScale;
    const qreal contentH = m_naturalSize.height() * m_fontScale;
    if (qFuzzyIsNull(w))
    {
        w = contentW;
    }
    if (qFuzzyIsNull(h))
    {
        h = contentH;
    }
    w = (std::max)(w, contentW);
    h = (std::max)(h, contentH);
    setRect(0, 0, w, h);
    // The stored position is the one the user left, and a file written while the piece still had
    // its seam allowance can carry one the outline no longer holds: reading it back is where the
    // rectangle that forms the piece gets its say, so an annotation can never come back outside.
    ContainPosition();
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::SetContainmentRect(const QRectF &rect)
{
    m_containmentRect = rect;
    ContainPosition();
    update();
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::CornerPoints() const -> QVector<QPointF>
{
    const QRectF r = rect();
    return {mapToParent(r.topLeft()), mapToParent(r.topRight()), mapToParent(r.bottomLeft()),
            mapToParent(r.bottomRight())};
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::ContainPosition()
{
    if (not OContainment::IsUsable(m_containmentRect))
    {
        return; // no rectangle to stay in — a piece with nothing drawn, or no host rectangle yet
    }

    const QVector<QPointF> corners = CornerPoints();
    // An annotation box bigger than the piece — a long text — has no inside position: the
    // furthest-point rule would make it swing from one edge to the other at every refresh, and that
    // position is published back to the model. Such a box stays where it is.
    if (not OContainment::FitsInside(m_containmentRect, corners))
    {
        return;
    }

    // Only the position is fitted, never the box: the rotation and the size stay the user's, and
    // the translation the rule returns is the move of the whole item, which is what pos() holds.
    const OContainment::Fit fit = OContainment::FitPoints(m_containmentRect, corners);
    if (not fit.contained)
    {
        setPos(pos() + fit.translation);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::SetReadOnly(bool readOnly)
{
    setFlag(ItemIsMovable, not readOnly);
    setAcceptHoverEvents(not readOnly);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::SetActive(bool active)
{
    if (m_active == active)
    {
        return;
    }
    m_active = active;
    setZValue(active ? 1000 + m_index : m_index);
    update();
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::IsActive() const -> bool
{
    return m_active;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::boundingRect() const -> QRectF
{
    return QGraphicsRectItem::boundingRect().adjusted(-kHandleHitHalf, -kHandleHitHalf, kHandleHitHalf,
                                                      kHandleHitHalf);
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::shape() const -> QPainterPath
{
    QPainterPath path;
    path.addRect(rect().adjusted(-kHandleHitHalf, -kHandleHitHalf, kHandleHitHalf, kHandleHitHalf));
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::ResolveFont() const -> QFont
{
    QFont font;
    if (!m_style.fontFamily.isEmpty())
    {
        font.setFamily(m_style.fontFamily);
    }
    if (!m_style.fontFace.isEmpty())
    {
        font.setStyleName(m_style.fontFace);
    }
    if (m_style.fontSize > 0)
    {
        font.setPixelSize(m_style.fontSize);
    }
    return font;
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    const QRectF r = rect();

    // Optional colored background (transparent = nothing drawn).
    if (m_hasBackground)
    {
        painter->fillRect(r, QColor(m_style.backgroundColor));
    }

    // Text — multi-line, centered, font × fontScale, explicit color (black default).
    // The base pixel size is the style size, or the resolved default when the
    // style uses "Default" — the corner handles must scale the glyphs either way.
    QFont font = ResolveFont();
    const int basePixel = m_style.fontSize > 0 ? m_style.fontSize : QFontInfo(font).pixelSize();
    font.setPixelSize(qRound(basePixel * m_fontScale));
    QColor textColor(QStringLiteral("#000000"));
    if (!m_style.textColor.isEmpty())
    {
        const QColor c(m_style.textColor);
        if (c.isValid())
        {
            textColor = c;
        }
    }
    painter->setFont(font);
    painter->setPen(textColor);
    painter->drawText(r, Qt::AlignCenter, m_text);

    // Editing outline + handles only when active.
    if (m_active)
    {
        QColor borderColor = textColor;
        borderColor.setAlpha(130);
        painter->setPen(QPen(borderColor, 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);

        painter->setBrush(QBrush(textColor));
        painter->setPen(Qt::NoPen);
        // 4 corners always; the 4 edges only with a colored background (§7.2).
        const QVector<QPointF> corners{r.topLeft(),
                                       r.topRight(),
                                       r.bottomLeft(),
                                       r.bottomRight()};
        for (const QPointF &c : corners)
        {
            painter->drawRect(QRectF(c.x() - kHandleHalf, c.y() - kHandleHalf, 2.0 * kHandleHalf,
                                     2.0 * kHandleHalf));
        }
        if (m_hasBackground)
        {
            const QVector<QPointF> edges{QPointF(r.center().x(), r.top()),
                                         QPointF(r.center().x(), r.bottom()),
                                         QPointF(r.left(), r.center().y()),
                                         QPointF(r.right(), r.center().y())};
            for (const QPointF &e : edges)
            {
                painter->drawRect(QRectF(e.x() - kHandleHalf, e.y() - kHandleHalf, 2.0 * kHandleHalf,
                                         2.0 * kHandleHalf));
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::HitTestHandle(const QPointF &localPos) const -> Handle
{
    const QRectF r = rect();
    const QVector<QPointF> corners{r.topLeft(), r.topRight(), r.bottomLeft(), r.bottomRight()};
    const Handle cornerHandles[] = {Handle::TopLeft, Handle::TopRight, Handle::BottomLeft, Handle::BottomRight};
    for (int i = 0; i < corners.size(); ++i)
    {
        if (qAbs(localPos.x() - corners[i].x()) <= kHandleHitHalf
            && qAbs(localPos.y() - corners[i].y()) <= kHandleHitHalf)
        {
            return cornerHandles[i];
        }
    }
    // Edge handles respond only with a colored background.
    if (m_hasBackground)
    {
        const QVector<QPointF> edges{QPointF(r.center().x(), r.top()),
                                     QPointF(r.center().x(), r.bottom()),
                                     QPointF(r.left(), r.center().y()),
                                     QPointF(r.right(), r.center().y())};
        const Handle edgeHandles[] = {Handle::Top, Handle::Bottom, Handle::Left, Handle::Right};
        for (int i = 0; i < edges.size(); ++i)
        {
            if (qAbs(localPos.x() - edges[i].x()) <= kHandleHitHalf
                && qAbs(localPos.y() - edges[i].y()) <= kHandleHitHalf)
            {
                return edgeHandles[i];
            }
        }
    }
    return Handle::None;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::CursorForHandle(Handle handle, qreal rotationDeg) -> Qt::CursorShape
{
    // With a ~90°/270° rotation the box's axes appear swapped on screen — swap
    // the horizontal/vertical cursors (and the diagonals) so the cursor matches
    // the visual direction of the resize.
    const qreal norm = qAbs(std::fmod(rotationDeg, 180.0));
    const bool swapped = norm > 45.0 && norm < 135.0;
    switch (handle)
    {
        case Handle::TopLeft:
        case Handle::BottomRight:
            return swapped ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor;
        case Handle::TopRight:
        case Handle::BottomLeft:
            return swapped ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor;
        case Handle::Top:
        case Handle::Bottom:
            return swapped ? Qt::SizeHorCursor : Qt::SizeVerCursor;
        case Handle::Left:
        case Handle::Right:
            return swapped ? Qt::SizeVerCursor : Qt::SizeHorCursor;
        case Handle::None:
        default:
            return Qt::SizeAllCursor;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    setCursor(CursorForHandle(HitTestHandle(event->pos()), rotation()));
    QGraphicsRectItem::hoverMoveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    setCursor(Qt::SizeAllCursor);
    QGraphicsRectItem::hoverLeaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !(flags() & ItemIsMovable))
    {
        QGraphicsRectItem::mousePressEvent(event);
        return;
    }

    m_activeHandle = HitTestHandle(event->pos());
    if (m_activeHandle != Handle::None)
    {
        m_origW = rect().width();
        m_origH = rect().height();
        m_anchorScene = mapToScene(AnchorLocal(m_activeHandle, rect()));
        m_dragStartScene = event->scenePos();
        m_dragStartPos = pos();
        m_previewW = m_origW;
        m_previewH = m_origH;
        setCursor(CursorForHandle(m_activeHandle, rotation()));
        event->accept();
        return;
    }

    // Move (manual — the default would drag the selected piece instead).
    m_activeHandle = Handle::None;
    m_dragStartScene = event->scenePos();
    m_dragStartPos = pos();
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::AnchorLocal(Handle handle, const QRectF &r) -> QPointF
{
    switch (handle)
    {
        case Handle::TopLeft:
            return r.bottomRight();
        case Handle::TopRight:
            return r.bottomLeft();
        case Handle::BottomLeft:
            return r.topRight();
        case Handle::BottomRight:
            return r.topLeft();
        case Handle::Top:
            return QPointF(r.center().x(), r.bottom());
        case Handle::Bottom:
            return QPointF(r.center().x(), r.top());
        case Handle::Left:
            return QPointF(r.right(), r.center().y());
        case Handle::Right:
            return QPointF(r.left(), r.center().y());
        case Handle::None:
        default:
            return QPointF();
    }
}

void OAnnotationItem::FixAnchorPosition()
{
    // Snap the anchor back to its press-time scene position. The anchor is the
    // fixed corner/edge of the CURRENT rect — for an edge resize the rect
    // changed, so the fixed edge is at the new rect boundary. mapToScene
    // accounts for the full transform (pos + preview scale + rotation).
    const QPointF newAnchorScene = mapToScene(AnchorLocal(m_activeHandle, rect()));
    setPos(pos() - (newAnchorScene - m_anchorScene));
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::MinBoxWidth() const -> qreal
{
    return m_naturalSize.width() * m_fontScale;
}

//---------------------------------------------------------------------------------------------------------------------
auto OAnnotationItem::MinBoxHeight() const -> qreal
{
    return m_naturalSize.height() * m_fontScale;
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_activeHandle != Handle::None)
    {
        // Offset of the cursor from the fixed anchor, expressed in the item's
        // local frame (the resize axes follow the box, not the screen) — this
        // keeps the resize correct under rotation.
        const QPointF sceneOffset = event->scenePos() - m_anchorScene;
        const qreal a = qDegreesToRadians(rotation());
        const qreal cosA = std::cos(a);
        const qreal sinA = std::sin(a);
        const QPointF localOffset(sceneOffset.x() * cosA + sceneOffset.y() * sinA,
                                  -sceneOffset.x() * sinA + sceneOffset.y() * cosA);

        const int sx = (m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::BottomLeft
                        || m_activeHandle == Handle::Left)
                           ? -1
                           : 1;
        const int sy = (m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::TopRight
                        || m_activeHandle == Handle::Top)
                           ? -1
                           : 1;
        const bool corner = (m_activeHandle == Handle::TopLeft || m_activeHandle == Handle::TopRight
                             || m_activeHandle == Handle::BottomLeft || m_activeHandle == Handle::BottomRight);

        if (corner)
        {
            // Corner: uniform scale of the whole block (content + box), driven by
            // the dragged corner in the box's own axes. kMinSize bounds the ratio.
            if (qFuzzyIsNull(m_origW) || qFuzzyIsNull(m_origH))
            {
                return;  // a zero-sized box cannot scale proportionally
            }
            const qreal ratioX = sx * localOffset.x() / m_origW;
            const qreal ratioY = sy * localOffset.y() / m_origH;
            qreal ratio = (std::max)(ratioX, ratioY);
            if (ratio <= 0.0)
            {
                return;  // the cursor crossed the anchor — keep the previous size
            }
            ratio = (std::max)(ratio, kMinSize / (std::max)(m_origW, 1.0));
            ratio = (std::max)(ratio, kMinSize / (std::max)(m_origH, 1.0));
            m_previewW = m_origW * ratio;
            m_previewH = m_origH * ratio;
            setTransform(QTransform().fromScale(ratio, ratio));
        }
        else
        {
            // Edge: grow only the box (the content scale is untouched); the box
            // never goes below the content size. The cursor distance from the
            // fixed edge, along the box axis, is the new dimension.
            if (m_activeHandle == Handle::Top || m_activeHandle == Handle::Bottom)
            {
                const qreal newH = (m_activeHandle == Handle::Top) ? -localOffset.y() : localOffset.y();
                if (newH <= 0.0)
                {
                    return;
                }
                m_previewW = m_origW;
                m_previewH = (std::max)(newH, MinBoxHeight());
                setRect(0, 0, m_origW, m_previewH);
            }
            else
            {
                const qreal newW = (m_activeHandle == Handle::Left) ? -localOffset.x() : localOffset.x();
                if (newW <= 0.0)
                {
                    return;
                }
                m_previewW = (std::max)(newW, MinBoxWidth());
                m_previewH = m_origH;
                setRect(0, 0, m_previewW, m_origH);
            }
        }
        FixAnchorPosition();
        event->accept();
        return;
    }

    setPos(m_dragStartPos + (event->scenePos() - m_dragStartScene));
    ContainPosition();
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_activeHandle != Handle::None)
    {
        const qreal finalW = m_previewW;
        const qreal finalH = m_previewH;
        setTransform(QTransform());
        setRect(0, 0, finalW, finalH);
        m_activeHandle = Handle::None;
        if (qAbs(finalW - m_origW) > kMinResizeDelta || qAbs(finalH - m_origH) > kMinResizeDelta)
        {
            emit Resized(m_index, finalW, finalH, m_origW, m_origH);
        }
        event->accept();
        return;
    }

    if (pos() != m_dragStartPos)
    {
        emit Moved(m_index, pos());
    }
    event->accept();
}
