/************************************************************************
 **
 **  @file   oinfoblockitem.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  On-piece information block: renders a stack of cartouche lines,
 **          draggable and resizable via 4 corner handles (proportional
 **          content scaling). Child of the piece item, piece-local px.
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

#ifndef OINFOBLOCKITEM_H
#define OINFOBLOCKITEM_H

#include <QGraphicsRectItem>
#include <QObject>

#include "../../vpatterndb/oinfoblockdata.h"
#include "../../vwidgets/ocartouchepainter.h"

class QGraphicsSceneHoverEvent;
class QGraphicsSceneMouseEvent;
class QStyleOptionGraphicsItem;

/**
 * @brief On-piece information block graphics item.
 *
 * Paints the resolved lines with the shared cartouche formula; the content
 * scales proportionally through fontScale (baked on resize commit). Move is
 * Qt's built-in movable behavior; resize starts on a corner handle and keeps
 * the opposite corner fixed (proportional, aspect preserved).
 */
class OInfoBlockItem : public QObject, public QGraphicsRectItem
{
    Q_OBJECT // NOLINT

public:
    explicit OInfoBlockItem(int blockIndex, QGraphicsItem *parent = nullptr);
    ~OInfoBlockItem() override = default;

    void SetContent(const QVector<OCartoucheRenderLine> &lines, qreal fontScale);
    void SetBlockGeom(const OInfoBlockGeom &geom);
    auto BlockIndex() const -> int;
    void SetReadOnly(bool readOnly);
    /** @brief Show/hide the editing outline + resize handles (active on click). */
    void SetActive(bool active);
    auto IsActive() const -> bool;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = QGraphicsItem::UserType + 1200
    };

signals:
    void Moved(int blockIndex, const QPointF &pos);
    void Resized(int blockIndex, qreal newW, qreal newH, qreal origW, qreal origH);

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
    static auto CursorForHandle(Handle handle) -> Qt::CursorShape;

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OInfoBlockItem) // NOLINT

    int m_blockIndex;
    QVector<OCartoucheRenderLine> m_lines;
    qreal m_fontScale{1.0};

    Handle m_activeHandle{Handle::None};
    bool m_active{false};
    qreal m_origW{0.0};
    qreal m_origH{0.0};
    QPointF m_anchorLocal{};
    QPointF m_dragStartScene{};
    QPointF m_dragStartPos{};
    qreal m_previewW{0.0};
    qreal m_previewH{0.0};

    static constexpr qreal kHandleHalf = 5.0;      // visible handle square half-size
    static constexpr qreal kHandleHitHalf = 10.0;  // invisible capture zone half-size (cursor + resize)
    static constexpr qreal kMinSize = 40.0;
    static constexpr qreal kMinResizeDelta = 0.5;
};

#endif // OINFOBLOCKITEM_H
