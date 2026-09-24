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

#include <QFont>
#include <QGraphicsRectItem>
#include <QObject>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

#include "../../vpatterndb/othumbnaildata.h"
#include "../../vwidgets/idoubleclickconsumer.h"

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
 *
 * The block also *says* something — the seam-allowance values it is cut with, and the legend that
 * names their unit. Those words belong to the application, so they arrive ready to draw (see
 * @ref OThumbnailTexts): the item places and paints them, it never formats them.
 */
class OThumbnailItem : public QObject, public QGraphicsRectItem, public IDoubleClickConsumer
{
    Q_OBJECT // NOLINT

public:
    /** @brief One word to draw: what it says, where it sits, and whether that place was chosen. */
    struct LabelView
    {
        QString text;
        QPointF offset{};   // From the block rect's top-left corner, in the item's coordinates.
        bool placed{false}; // False: the item lays it out in the row above the block.
    };

    explicit OThumbnailItem(QGraphicsItem *parent = nullptr);
    ~OThumbnailItem() override = default;

    /** @brief Source drawing (piece coordinates) and the frame it must be fitted into. */
    void SetContent(const QPainterPath &content, const QRectF &contentBounds);
    void SetBlockGeom(const OThumbnailGeom &geom);
    void SetReadOnly(bool readOnly);
    /** @brief Show/hide the editing outline + resize handles (active on click). */
    void SetActive(bool active);
    auto IsActive() const -> bool;
    /**
     * @brief Whether a click on the block opens it — true only when it was already selected.
     *
     * Set by the piece on every press, because the press that selects and the press that opens look
     * exactly alike from inside the item: by the time it is called, the selection has already been
     * granted.
     */
    void SetOpensOnClick(bool opens);

    /** @brief The words to draw. The legend is empty when no unit is asked for. */
    void SetLabels(const QVector<LabelView> &labels, const QString &legend, const QPointF &legendOffset,
                   bool legendPlaced);
    /** @brief Enter/leave the "inside the miniature" sub-mode, in which the words can be moved. */
    void SetEditing(bool editing);
    auto IsEditing() const -> bool;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = QGraphicsItem::UserType + 1300
    };

    /** @brief The block takes a click pair rather than letting the view zoom to fit. */
    auto ConsumesDoubleClick() const -> bool override;

signals:
    /** @brief Committed geometry after a move, a resize, or both (one undo step). */
    void GeometryChanged(const QPointF &pos, qreal width, qreal height);
    /** @brief A word was dropped somewhere new (one undo step). */
    void LabelMoved(int index, const QPointF &offset);
    void LegendMoved(const QPointF &offset);
    /** @brief The sub-mode was entered or left, so the window can say what Escape does. */
    void EditingChanged(bool editing);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    auto boundingRect() const -> QRectF override;
    auto shape() const -> QPainterPath override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
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

    /// @brief The app's own text, at its default size — the miniature's wording until the font
    /// settings of the miniature editor exist.
    static constexpr int kLabelPixelSize = 14;
    static constexpr qreal kLabelGap = 6.0; // Between two values of the default row.
    static constexpr qreal kBandGap = 4.0;  // Between a band and the block rect.

    /// @brief The dragged word of the legend, in @ref m_dragTarget.
    static constexpr int kLegendTarget = -1;

    auto HitTestHandle(const QPointF &pos) const -> Handle;
    static auto CursorForHandle(Handle handle) -> Qt::CursorShape;
    /** @brief New rect + position for a resize gesture, aspect ratio preserved. */
    void ResizeTo(const QPointF &mouseInParent, QRectF &newRect, QPointF &newPos) const;

    static auto Font() -> QFont;
    auto TextRect(const QString &text, const QPointF &offset) const -> QRectF;
    /** @brief Where a word actually sits: its own place, or the default row when it has none. */
    auto EffectiveOffset(int index) const -> QPointF;
    auto EffectiveLegendOffset() const -> QPointF;
    /** @brief The label or legend under @p pos, or -2 when there is none. */
    auto HitTestWord(const QPointF &pos) const -> int;

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OThumbnailItem) // NOLINT

    QPainterPath m_content;
    QRectF m_contentBounds;

    QVector<LabelView> m_labels;
    QString m_legend;
    QPointF m_legendOffset;
    bool m_legendPlaced{false};

    bool m_readOnly{false};
    bool m_active{false};
    bool m_editing{false};
    bool m_opensOnClick{false};
    Handle m_activeHandle{Handle::None};

    int m_dragTarget{-2}; // -2: nothing, -1: the legend, >= 0: a label.
    QPointF m_dragWordStart;      // The word's offset when the drag began.
    QPointF m_dragWordStartPos;   // Where the press landed, in the item's coordinates.
    bool m_dragWordWasPlaced{false};
    bool m_pendingEnter{false}; // A click on an already-selected block; a drag is not that click.
    QPointF m_dragStartScenePos;
    QPointF m_dragStartPos;
    QRectF m_dragStartRect;
    bool m_geometryChanged{false};
};

#endif // OTHUMBNAILITEM_H
