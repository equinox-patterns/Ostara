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
#include <QSizeF>
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
        QPointF offset{};   // In the drawing's own coordinates when placed — the block's frame when not.
        bool placed{false}; // False: the item lays it out in the row above the block.
        /// True for the caption of the values rather than a value: it is drawn in the legend's own
        /// font — a record carries the legend among the values (see @ref SetPlacedWords), and the
        /// two texts are set separately.
        bool legend{false};
    };

    /** @brief One word whose place is already DECIDED — the form a stored record carries it in. */
    struct PlacedWord
    {
        QString text;
        QPointF origin{}; // In the block's own frame, the fit already applied.
        bool legend{false};
    };

    explicit OThumbnailItem(QGraphicsItem *parent = nullptr);
    ~OThumbnailItem() override = default;

    /** @brief Source drawing (piece coordinates) and the frame it must be fitted into. */
    void SetContent(const QPainterPath &content, const QRectF &contentBounds);
    /** @brief The frame of the block, in its parent's coordinates. */
    void SetBlockGeom(const OThumbnailGeom &geom);
    /**
     * @brief Completes a frame nobody ever chose: `width`/`height` at 0 → a third of the host box,
     * clamped between 5 and 15 cm; `placed == false` → centred on the host box.
     *
     * One rule for two canvases: the Details canvas measures the piece the miniature belongs to, the
     * Graded canvas measures the reference-layer item — and "not chosen yet" has to mean the same
     * thing on both, or the same untouched miniature would change size when the mode changes.
     */
    static auto ResolveBlockGeom(OThumbnailGeom geom, const QRectF &hostBounds, bool placed) -> OThumbnailGeom;
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
    /**
     * @brief Whether a click on the selected block opens the "inside the miniature" sub-mode.
     *
     * Off on the Graded canvas: the wording has no placement of its own in a size range, so the
     * second click must stay what it is for every other child — nothing — and the double-click must
     * stay the view's zoom (see ConsumesDoubleClick()).
     */
    void SetSubModeEnabled(bool enabled);
    auto IsSubModeEnabled() const -> bool;

    /** @brief The words to draw. The legend is empty when no unit is asked for. */
    void SetLabels(const QVector<LabelView> &labels, const QString &legend, const QPointF &legendOffset,
                   bool legendPlaced);
    /**
     * @brief Draw words whose places are ALREADY decided, instead of computing them.
     *
     * The canvas decides where a word goes (see @ref EffectiveOffset); a stored record does not — the
     * writers were handed the finished places, and redrawing it has to land them exactly there. So the
     * item is told the places themselves rather than the raw offsets, and stops computing.
     *
     * The legends travel in the same list: a record does not say which word is a legend, and it does
     * not have to — all of them are words at a place. Setting the words this way therefore clears any
     * @ref SetLabels wording, and vice versa.
     */
    void SetPlacedWords(const QVector<PlacedWord> &words);
    /** @brief Enter/leave the "inside the miniature" sub-mode, in which the words can be moved. */
    void SetEditing(bool editing);
    auto IsEditing() const -> bool;

    // --- Where a word goes: one implementation, two canvases and the export --------------------
    //
    // The screen and the file have to put a word at the same place, and the only way to keep that
    // promise is to have a single answer to "where does it go". These are that answer: the item
    // paints with them, and the SVG writer places the same words with the same numbers instead of
    // transcribing the formulas — a second transcription would drift the first time one of them
    // learned something new.
    /** @brief The font the seam-allowance values are drawn in. */
    static auto ValuesFont() -> QFont;
    /** @brief The font the legend is drawn in — set apart from the values, as its own text. */
    static auto LegendFont() -> QFont;
    /** @brief The box a word occupies — as wide as its widest line, as tall as all of its lines. */
    static auto LabelTextSize(const QString &text, const QFont &font) -> QSizeF;
    /** @brief Where a word nobody moved sits: the row above @p block, centred, in the order given. */
    static auto DefaultWordOrigin(const QVector<LabelView> &labels, int index, const QRectF &block,
                                  const QFont &font) -> QPointF;
    /** @brief Where a legend nobody moved sits: centred under @p block. */
    static auto DefaultLegendOrigin(const QString &legend, const QRectF &block, const QFont &font)
        -> QPointF;
    /**
     * @brief Where each line of a word stands: every line centred in the word's own box.
     *
     * A legend is ONE string carrying a line break per language (see OThumbnailTextBuilder::Legend),
     * so its box is as wide as its widest language. Drawn from the box's left edge, the shorter
     * language starts where the longer one starts and the caption reads left-aligned under the piece;
     * centred in the box, the caption reads as one caption — which is what a caption is.
     *
     * A word on one line is untouched by this: its box IS its line, so the single origin is the one
     * it always had. Every painter of these words asks here — the canvases, the block editor's
     * preview and both SVG writers — for the same reason @ref LabelTextSize is asked: a second
     * transcription of the rule would drift from the first.
     */
    static auto CenteredLineOrigins(const QString &text, const QPointF &topLeft, const QFont &font)
        -> QVector<QPointF>;
    /**
     * @brief Where a word placed by hand lands: a point of the drawing, put through the same fit
     * the drawing itself goes through.
     */
    static auto FittedWordOrigin(const QRectF &contentBounds, const QRectF &block,
                                 const QPointF &contentPos) -> QPointF;

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

    /// @brief The size a miniature falls back on when the preference holds none — a font set in
    /// points has no pixel size at all. The real one is the Thumbnail page's, in the preferences.
    static constexpr int kLabelPixelSize = 14;
    static constexpr qreal kLabelGap = 6.0; // Between two values of the default row.
    static constexpr qreal kBandGap = 4.0;  // Between a band and the block rect.

    /// @brief The dragged word of the legend, in @ref m_dragTarget.
    static constexpr int kLegendTarget = -1;

    auto HitTestHandle(const QPointF &pos) const -> Handle;
    static auto CursorForHandle(Handle handle) -> Qt::CursorShape;
    /** @brief New rect + position for a resize gesture, aspect ratio preserved. */
    void ResizeTo(const QPointF &mouseInParent, QRectF &newRect, QPointF &newPos) const;

    /// @brief The font a family, a face and a pixel size make — one answer for the screen and the file.
    static auto StyledFont(const QString &family, const QString &face, int pixelSize) -> QFont;

    auto TextRect(const QString &text, const QPointF &offset, const QFont &font) const -> QRectF;
    /**
     * @brief Where a placed word lands on screen: a point of the drawing, put through the same fit
     * the drawing itself goes through.
     *
     * This is what makes a resize carry the words: the frame changes, the fit changes with it, and
     * every word stays on the spot of the piece it names — the block's own corner is never a
     * reference.
     */
    auto WordOrigin(const QPointF &contentPos) const -> QPointF;
    /** @brief The other way round: an item position, read back as a point of the drawing — what a
     *  word drag commits, so that what is stored is a place on the piece and not a place on screen. */
    auto WordPlacement(const QPointF &origin) const -> QPointF;
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
    /// Whether the words of @ref m_labels are already placed — see @ref SetPlacedWords. Their
    /// @c offset is then the finished origin, in the block's frame, and nothing is computed.
    bool m_wordsInBlock{false};

    bool m_readOnly{false};
    bool m_active{false};
    bool m_editing{false};
    bool m_opensOnClick{false};
    bool m_subModeEnabled{true};
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
