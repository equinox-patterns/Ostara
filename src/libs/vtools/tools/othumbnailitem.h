/************************************************************************
 **
 **  @file   othumbnailitem.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  On-piece piece miniature block: draws the piece outline (seam
 **          allowances included), its notches and its internal paths, fitted
 **          inside the block. Move is manual (never the base class: a piece
 **          drag would move the piece itself); resize starts on one of the
 **          8 handles, keeps the aspect ratio and pins the opposite side.
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

#ifndef OTHUMBNAILITEM_H
#define OTHUMBNAILITEM_H

#include <QGraphicsRectItem>
#include <QObject>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>

#include "../../vpatterndb/othumbnaildata.h"

class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QStyleOptionGraphicsItem;

/**
 * @brief Piece miniature block graphics item.
 *
 * The content is a vector drawing in the piece's own coordinates; it is fitted
 * (never stretched) inside the block rect on every paint, so a resize or a zoom
 * needs no recomputation of the drawing itself.
 *
 * One signal for both gestures: a resize pins the opposite side, which moves the
 * block, and committing the two separately would cost two undo steps for one
 * gesture.
 */
class OThumbnailItem : public QObject, public QGraphicsRectItem
{
    Q_OBJECT // NOLINT

public:
    explicit OThumbnailItem(QGraphicsItem *parent = nullptr);
    ~OThumbnailItem() override = default;

    /** @brief Source drawing (piece coordinates) and the frame it must be fitted into. */
    void SetContent(const QPainterPath &content, const QRectF &contentBounds);
    void SetBlockGeom(const OThumbnailGeom &geom);
    void SetReadOnly(bool readOnly);
    /** @brief Show/hide the editing outline + resize handles (active on click). */
    void SetActive(bool active);
    auto IsActive() const -> bool;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = QGraphicsItem::UserType + 1300
    };

signals:
    /** @brief Committed geometry after a move, a resize, or both (one undo step). */
    void GeometryChanged(const QPointF &pos, qreal width, qreal height);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    auto boundingRect() const -> QRectF override;
    auto shape() const -> QPainterPath override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private:
    enum class Handle
    {
        None,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight,
        Top,
        Bottom,
        Left,
        Right
    };

    static constexpr qreal kHandleHalf = 5.0;
    static constexpr qreal kHandleHitHalf = 10.0;
    static constexpr qreal kMinSize = 40.0; // ~1 cm at 96 DPI

    auto HitTestHandle(const QPointF &pos) const -> Handle;
    static auto CursorForHandle(Handle handle) -> Qt::CursorShape;
    /** @brief New rect + position for a resize gesture, aspect ratio preserved. */
    void ResizeTo(const QPointF &mouseInParent, QRectF &newRect, QPointF &newPos) const;

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OThumbnailItem) // NOLINT

    QPainterPath m_content;
    QRectF m_contentBounds;

    bool m_readOnly{false};
    bool m_active{false};
    Handle m_activeHandle{Handle::None};

    QPointF m_dragStartScenePos;
    QPointF m_dragStartPos;
    QRectF m_dragStartRect;
    bool m_geometryChanged{false};
};

#endif // OTHUMBNAILITEM_H
