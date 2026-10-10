/************************************************************************
 **
 **  @file   oannotationitem.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  On-piece annotation: renders a floating multi-line text with an
 **          optional colored background and a rotation, draggable and
 **          resizable via 8 handles — the 4 corners scale the whole block
 **          (content + box), the 4 edges grow only the box (visible only
 **          with a colored background). Child of the piece item, piece-local
 **          px.
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

#ifndef OANNOTATIONITEM_H
#define OANNOTATIONITEM_H

#include <QGraphicsRectItem>
#include <QObject>
#include <QVector>

#include "../../vpatterndb/oannotationdata.h"

class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QStyleOptionGraphicsItem;

/**
 * @brief On-piece annotation graphics item.
 *
 * Paints the multi-line text with the resolved style (font + optional
 * background + explicit text color), centered, scaled by fontScale. Corners
 * resize proportionally (content + box), edges grow only the box and only
 * when a colored background is active.
 */
class OAnnotationItem : public QObject, public QGraphicsRectItem
{
    Q_OBJECT // NOLINT

public:
    explicit OAnnotationItem(int index, QGraphicsItem *parent = nullptr);
    ~OAnnotationItem() override = default;

    /// Position of this annotation in the piece's list (drives signals/z-value).
    void SetIndex(int index);

    /// Text + resolved render style; recomputes the natural content size.
    void SetContent(const QString &text, const OAnnotationRenderStyle &style);

    /// Position, rotation (degrees, applied as -angle), box and fontScale.
    void SetAnnotationGeom(const OAnnotationGeom &geom, qreal rotation);

    void SetReadOnly(bool readOnly);
    /** @brief Show/hide the editing outline + resize handles (active on click). */
    void SetActive(bool active);
    auto IsActive() const -> bool;

    /** @brief Rectangle the annotation may not leave, in the parent's frame. Null = unconstrained.
     *
     * Handed over by the item that owns the annotation — the piece or the group holder — because
     * only that host knows which rectangle forms the piece (its outline, never the seam
     * allowance). */
    void SetContainmentRect(const QRectF &rect);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = QGraphicsItem::UserType + 1201
    };

signals:
    void Moved(int index, const QPointF &pos);
    void Resized(int index, qreal newW, qreal newH, qreal origW, qreal origH);

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

    auto HitTestHandle(const QPointF &localPos) const -> Handle;
    /// Cursor for a handle, matched to the box's on-screen orientation.
    static auto CursorForHandle(Handle handle, qreal rotationDeg) -> Qt::CursorShape;
    /// Local point of the fixed corner/edge for a handle, on the current rect.
    static auto AnchorLocal(Handle handle, const QRectF &r) -> QPointF;
    void FixAnchorPosition();
    auto MinBoxWidth() const -> qreal;
    auto MinBoxHeight() const -> qreal;
    auto ResolveFont() const -> QFont;

    /** @brief The box's corners in the parent's frame, where the annotation now stands.
     *
     * The corners, and not the bounding box: a rotated annotation is inside the rectangle exactly
     * when its four corners are, while its bounding box would demand more room than it needs. */
    auto CornerPoints() const -> QVector<QPointF>;
    /** @brief Move the annotation back inside its containment rectangle; a no-op without one. */
    void ContainPosition();

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OAnnotationItem) // NOLINT

    int m_index;
    QString m_text;
    OAnnotationRenderStyle m_style;
    QSizeF m_naturalSize{};
    qreal m_fontScale{1.0};
    bool m_hasBackground{false};
    QRectF m_containmentRect{};

    Handle m_activeHandle{Handle::None};
    bool m_active{false};
    qreal m_origW{0.0};
    qreal m_origH{0.0};
    QPointF m_anchorScene{};
    QPointF m_dragStartScene{};
    QPointF m_dragStartPos{};
    qreal m_previewW{0.0};
    qreal m_previewH{0.0};

    static constexpr qreal kHandleHalf = 5.0;      // visible handle square half-size
    static constexpr qreal kHandleHitHalf = 10.0;  // invisible capture zone half-size (cursor + resize)
    static constexpr qreal kMinSize = 40.0;
    static constexpr qreal kMinResizeDelta = 0.5;
};

#endif // OANNOTATIONITEM_H
