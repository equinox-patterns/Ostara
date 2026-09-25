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
#include <QFontMetricsF>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QLineF>
#include <QPainter>
#include <QStringList>
#include <QStyleOptionGraphicsItem>
#include <QtGlobal>

#include "../../vmisc/def.h"
#include "../../vmisc/vabstractapplication.h"
#include "../../vmisc/vabstractvalapplication.h"
#include "../../vmisc/vvalentinasettings.h"
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
auto OThumbnailItem::ResolveBlockGeom(OThumbnailGeom geom, const QRectF &hostBounds, bool placed)
    -> OThumbnailGeom
{
    if (qFuzzyIsNull(geom.width) || qFuzzyIsNull(geom.height))
    {
        // Auto-size: a third of the piece, clamped between 5 and 15 cm — small enough to read as a
        // thumbnail, large enough to stay legible.
        constexpr qreal kFraction = 0.30;
        constexpr qreal kMinCm = 5.0;
        constexpr qreal kMaxCm = 15.0;
        const qreal cmW = qBound(kMinCm, FromPixel(hostBounds.width(), Unit::Cm) * kFraction, kMaxCm);
        const qreal cmH = qBound(kMinCm, FromPixel(hostBounds.height(), Unit::Cm) * kFraction, kMaxCm);
        geom.width = ToPixel(cmW, Unit::Cm);
        geom.height = ToPixel(cmH, Unit::Cm);
    }

    if (not placed)
    {
        // Never placed: centred, and centred on the host — an untouched miniature sits on the piece
        // it draws, on whichever canvas draws it.
        geom.pos = hostBounds.center() - QPointF(geom.width / 2.0, geom.height / 2.0);
    }
    return geom;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::SetReadOnly(bool readOnly)
{
    m_readOnly = readOnly;
    setFlag(QGraphicsItem::ItemIsMovable, not readOnly);
    setAcceptedMouseButtons(readOnly ? Qt::NoButton : Qt::AllButtons);
    if (readOnly)
    {
        // A block that cannot be touched cannot be open either: the sub-mode is a gesture, and there
        // is no gesture left to leave it with.
        SetEditing(false);
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
void OThumbnailItem::SetLabels(const QVector<LabelView> &labels, const QString &legend,
                               const QPointF &legendOffset, bool legendPlaced)
{
    // The words are inside what the item claims to paint, so a change of words is a change of
    // bounding rect — and the scene has to be told before it happens, not after.
    prepareGeometryChange();

    m_labels = labels;
    m_legend = legend;
    m_legendOffset = legendOffset;
    m_legendPlaced = legendPlaced;
    m_wordsInBlock = false;
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::SetPlacedWords(const QVector<PlacedWord> &words)
{
    // The same courtesy as SetLabels: the words are inside what the item claims to paint.
    prepareGeometryChange();

    m_labels.clear();
    m_labels.reserve(words.size());
    for (const PlacedWord &word : words)
    {
        // `placed` is deliberately left false: it is what tells EffectiveOffset to run the fit, and
        // there is no fit left to run — `origin` already went through it. m_wordsInBlock is the flag
        // that says so.
        m_labels.append({word.text, word.origin, false, word.legend});
    }

    // The legend travels in the same list — a record carries every word placed, the caption among
    // them, and each now says which of the two texts it is. Keeping one here as well would draw it
    // twice.
    m_legend.clear();
    m_legendOffset = QPointF();
    m_legendPlaced = false;
    m_wordsInBlock = true;
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::SetOpensOnClick(bool opens)
{
    m_opensOnClick = opens;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::SetSubModeEnabled(bool enabled)
{
    m_subModeEnabled = enabled;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::IsSubModeEnabled() const -> bool
{
    return m_subModeEnabled;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::SetEditing(bool editing)
{
    if (m_editing == editing)
    {
        return;
    }
    m_editing = editing;
    m_dragTarget = -2;
    // Leaving the sub-mode puts the cursor back where the block expects it: the handles are the
    // gesture outside, the words are the gesture inside, and the pointer says which one is live.
    setCursor(m_editing ? Qt::OpenHandCursor : Qt::SizeAllCursor);
    update();
    emit EditingChanged(m_editing);
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::IsEditing() const -> bool
{
    return m_editing;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::ConsumesDoubleClick() const -> bool
{
    // Only while the block is there to be entered: a hidden, read-only or sub-mode-less miniature
    // has no sub-mode, and claiming the gesture would then take the zoom away for nothing.
    return m_subModeEnabled && m_active && not m_readOnly && not m_editing;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::boundingRect() const -> QRectF
{
    // Handles are painted just outside the rect, and the hit area is wider still.
    QRectF box = rect().adjusted(-kHandleHitHalf, -kHandleHitHalf, kHandleHitHalf, kHandleHitHalf);

    // The words too, and at their current place rather than at a band: a word dragged near the
    // block, or over it, has to stay inside what the item claims to paint. An empty word is not a
    // word and claims nothing.
    for (int i = 0; i < m_labels.size(); ++i)
    {
        if (m_labels.at(i).text.isEmpty())
        {
            continue;
        }
        const LabelView &word = m_labels.at(i);
        box = box.united(TextRect(word.text, EffectiveOffset(i), word.legend ? LegendFont() : ValuesFont()));
    }
    if (not m_legend.isEmpty())
    {
        box = box.united(TextRect(m_legend, EffectiveLegendOffset(), LegendFont()));
    }

    return box;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::shape() const -> QPainterPath
{
    QPainterPath path;
    path.addRect(boundingRect());
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::StyledFont(const QString &family, const QString &face, int pixelSize) -> QFont
{
    // **One answer for the screen and the file.** The SVG export draws the words from here too, so
    // the document cannot end up with a typography the screen never showed: the Thumbnail page of the
    // block editor writes the preferences, this reads them, and neither invents anything.
    //
    // The face is a NAME and not a weight (see FillFontFaces): a family calls its bold « DemiBold »
    // where Qt calls it « Demi Bold », and only the family knows which face that is. Empty means the
    // family's Regular, which is what a miniature nobody has styled keeps.
    QFont font(family);
    if (!face.isEmpty())
    {
        font.setStyleName(face);
    }
    font.setPixelSize(pixelSize > 0 ? pixelSize : kLabelPixelSize);
    return font;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::ValuesFont() -> QFont
{
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    return StyledFont(settings->GetThumbnailValuesFontFamily(), settings->GetThumbnailValuesFontFace(),
                      settings->GetThumbnailValuesFontPixelSize());
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::LegendFont() -> QFont
{
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    return StyledFont(settings->GetThumbnailLegendFontFamily(), settings->GetThumbnailLegendFontFace(),
                      settings->GetThumbnailLegendFontPixelSize());
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::LabelTextSize(const QString &text, const QFont &font) -> QSizeF
{
    // **`QFontMetricsF::horizontalAdvance()` is not that box.** A legend written in two languages is
    // one string with a line break in it, and the advance of that string measures the two languages
    // joined end to end: centred on that number the legend slides off to one side, and a one-line box
    // handed to `drawText()` clips the second language away entirely.
    const QFontMetricsF metrics(font);
    const QStringList lines = text.split(QLatin1Char('\n'));

    qreal width = 0.0;
    for (const QString &line : lines)
    {
        width = qMax(width, metrics.horizontalAdvance(line));
    }
    return QSizeF(width, metrics.height() * static_cast<qreal>(lines.size()));
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::TextRect(const QString &text, const QPointF &offset, const QFont &font) const -> QRectF
{
    return QRectF(offset, LabelTextSize(text, font));
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::FittedWordOrigin(const QRectF &contentBounds, const QRectF &block,
                                      const QPointF &contentPos) -> QPointF
{
    // No drawing yet: there is no fit to go through, and nothing is painted anyway.
    if (contentBounds.isEmpty())
    {
        return contentPos;
    }
    return OCartouchePainter::FitTransform(contentBounds, block).map(contentPos);
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::WordOrigin(const QPointF &contentPos) const -> QPointF
{
    return FittedWordOrigin(m_contentBounds, rect(), contentPos);
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::WordPlacement(const QPointF &origin) const -> QPointF
{
    if (m_contentBounds.isEmpty())
    {
        return origin;
    }
    bool invertible = false;
    const QTransform inverse = OCartouchePainter::FitTransform(m_contentBounds, rect()).inverted(&invertible);
    return invertible ? inverse.map(origin) : origin;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::DefaultWordOrigin(const QVector<LabelView> &labels, int index, const QRectF &block,
                                       const QFont &font) -> QPointF
{
    // **The row is not stored, it is derived.** Values the user never moved sit above the block in
    // the order they were added, centred on it — so adding a fourth value moves the row instead of
    // leaving a gap where a value was once dragged away.
    const QFontMetricsF metrics(font);
    qreal rowWidth = 0.0;
    int unplaced = 0;
    for (const LabelView &other : labels)
    {
        if (other.placed || other.text.isEmpty())
        {
            continue; // An empty word holds no place in the row, and no gap either.
        }
        rowWidth += LabelTextSize(other.text, font).width() + kLabelGap;
        ++unplaced;
    }
    if (unplaced > 0)
    {
        rowWidth -= kLabelGap; // No gap after the last word of the row.
    }

    qreal x = block.center().x() - rowWidth / 2.0;
    for (int i = 0; i < index; ++i)
    {
        if (labels.at(i).placed || labels.at(i).text.isEmpty())
        {
            continue;
        }
        x += LabelTextSize(labels.at(i).text, font).width() + kLabelGap;
    }

    return QPointF(x, -kBandGap - metrics.height());
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::DefaultLegendOrigin(const QString &legend, const QRectF &block, const QFont &font)
    -> QPointF
{
    // **Centred on the block, which is the rectangle the miniature is.** Not on the piece: the legend
    // is the block's caption, and centring it on the piece would slide it away from the drawing it
    // speaks for as soon as the block is not sitting on the piece's middle.
    return QPointF(block.center().x() - LabelTextSize(legend, font).width() / 2.0,
                   block.bottom() + kBandGap);
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::EffectiveOffset(int index) const -> QPointF
{
    const LabelView &label = m_labels.at(index);
    if (m_wordsInBlock)
    {
        // The place came from a record, fit included — see SetPlacedWords. It is not a point of the
        // drawing to be fitted, it is where the words already are.
        return label.offset;
    }
    if (label.placed)
    {
        // A placed word is a point of the drawing: it follows the piece wherever the fit puts it.
        return WordOrigin(label.offset);
    }
    // The row this lays out is the values' own: a replayed word is already placed and never reaches
    // this line (see m_wordsInBlock), so the row is measured in the values' font.
    return DefaultWordOrigin(m_labels, index, rect(), ValuesFont());
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::EffectiveLegendOffset() const -> QPointF
{
    if (m_legendPlaced)
    {
        return WordOrigin(m_legendOffset);
    }
    return DefaultLegendOrigin(m_legend, rect(), LegendFont());
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailItem::HitTestWord(const QPointF &pos) const -> int
{
    // Last painted is on top, so the search runs backwards — the same order the eye reads the
    // stacking in.
    if (not m_legend.isEmpty() && TextRect(m_legend, EffectiveLegendOffset(), LegendFont()).contains(pos))
    {
        return kLegendTarget;
    }
    for (qsizetype i = m_labels.size() - 1; i >= 0; --i)
    {
        const LabelView &word = m_labels.at(i);
        const QFont font = word.legend ? LegendFont() : ValuesFont();
        if (TextRect(word.text, EffectiveOffset(static_cast<int>(i)), font).contains(pos))
        {
            return static_cast<int>(i);
        }
    }
    return -2;
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
        //
        // ⚠️ **`m11()` is not that scale once the transform turns**, and reading it as one is what
        // broke the miniature on the Export canvas. The entry is the first column of the matrix, so a
        // rotation of θ makes it `scale · cos θ` — the true scale at 0°, half of it at 60°, **zero at
        // 90°** and **negative past it**. A null scale collapsed the stroke to nothing; a negative one
        // handed `setWidthF()` a width it refuses ("Setting a pen width that is out of range") and a
        // stroke the canvas then drew at the wrong weight — the whole miniature wrong, on a rotated
        // owner alone. Details and Graded never showed it because nothing turns a miniature there; the
        // Export canvas hangs one under the carrier of a placed owner, and the nesting turns a carrier
        // by any angle.
        //
        // The scale is the LENGTH of the image of a unit vector, which is the one thing a rotation
        // leaves alone. `fit` is a pure scale (@ref OCartouchePainter::FitTransform), so its own
        // `m11()` is still its scale, and the ratio of the two is the zoom.
        const QTransform device = painter->transform();
        const qreal scale =
            QLineF(device.map(QPointF(0.0, 0.0)), device.map(QPointF(1.0, 0.0))).length();
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
        // ⚠️ A pen refuses a width that is not a positive finite number, and says so. The scale above
        // cannot be zero on any transform these canvases build — but a pen that is never handed a
        // width it refuses is worth the guard: it keeps the default and draws, where a refusal left it
        // at whatever the last paint had asked for.
        const qreal penWidth = scale > 0.0 ? stroke / scale : stroke;
        if (qIsFinite(penWidth) and penWidth > 0.0)
        {
            pen.setWidthF(penWidth);
        }
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(m_content);
        painter->restore();
    }

    // **The words, in the item's own coordinates — not inside the fit transform.** They are the
    // block's caption, not part of the piece drawing: resizing the block resizes the drawing and
    // leaves the words where they are, while a change of zoom carries them exactly as it carries the
    // cartouche's text. They also stay out of the cached content, which is geometry alone.
    painter->setPen(textColor);
    painter->setBrush(Qt::NoBrush);
    for (int i = 0; i < m_labels.size(); ++i)
    {
        const LabelView &word = m_labels.at(i);
        if (word.text.isEmpty())
        {
            continue;
        }
        // **Each word in its own font.** A value and the caption that names its unit are two texts
        // with two settings; and the caption travels among the values when a record is replayed (see
        // SetPlacedWords), so it must not come out as loud as the numbers it captions.
        const QFont font = word.legend ? LegendFont() : ValuesFont();
        painter->setFont(font);
        painter->drawText(TextRect(word.text, EffectiveOffset(i), font), Qt::AlignLeft | Qt::AlignTop,
                          word.text);
    }
    if (not m_legend.isEmpty())
    {
        painter->setFont(LegendFont());
        painter->drawText(TextRect(m_legend, EffectiveLegendOffset(), LegendFont()),
                          Qt::AlignLeft | Qt::AlignTop, m_legend);
    }

    // Light dashed outline + 8 filled square handles only when the block is active (clicked), like
    // the information blocks — otherwise only the drawing and its words are visible.
    if (m_editing)
    {
        // **Orange, and no handles.** It is the colour the canvas already uses to say a piece is
        // held rather than merely selected, and it means here what it means there: this block is
        // open. The handles go because inside the miniature the gesture is moving a word, not the
        // frame.
        painter->setPen(QPen(QColor(255, 165, 0), 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);
    }
    else if (m_active)
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

    // **Inside the miniature, the press belongs to the words.** The block neither moves nor resizes
    // there: the sub-mode exists to place values, and a frame that shifted under the same gesture
    // would make every placement a guess.
    if (m_editing)
    {
        m_dragTarget = HitTestWord(event->pos());
        m_dragWordStartPos = event->pos();
        m_dragWordWasPlaced = m_dragTarget == kLegendTarget ? m_legendPlaced
                              : m_dragTarget >= 0           ? m_labels.at(m_dragTarget).placed
                                                            : false;
        if (m_dragTarget == kLegendTarget)
        {
            m_dragWordStart = EffectiveLegendOffset();
            m_legendOffset = WordPlacement(m_dragWordStart);
            m_legendPlaced = true;
        }
        else if (m_dragTarget >= 0)
        {
            m_dragWordStart = EffectiveOffset(m_dragTarget);
            m_labels[m_dragTarget].offset = WordPlacement(m_dragWordStart);
            m_labels[m_dragTarget].placed = true;
        }
        setCursor(m_dragTarget == -2 ? Qt::ArrowCursor : Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    m_activeHandle = HitTestHandle(event->pos());
    m_dragStartScenePos = event->scenePos();
    m_dragStartPos = pos();
    m_dragStartRect = rect();
    m_geometryChanged = false;

    // **A press on an already-selected block is a click, and a click opens the miniature** — but
    // only if the pointer stays put. The decision is therefore taken on release, so that the same
    // press can still drag the block; see mouseReleaseEvent(). The "already selected" answer comes
    // from the piece, because by now the selection has been granted either way.
    m_pendingEnter = m_subModeEnabled && m_opensOnClick && m_activeHandle == Handle::None;

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
void OThumbnailItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    // Qt never delivers the second press of a fast pair: the pair arrives as a press, then as this
    // event. It is the same gesture as the second click, so it does the same thing — and the view
    // stays out of it, which is what ConsumesDoubleClick() is for.
    if (not m_subModeEnabled || m_readOnly || m_editing || event->button() != Qt::LeftButton
        || HitTestHandle(event->pos()) != Handle::None)
    {
        event->ignore();
        return;
    }

    m_pendingEnter = false;
    SetEditing(true);
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

    if (m_editing)
    {
        if (m_dragTarget == -2)
        {
            event->accept();
            return;
        }
        const QPointF delta = event->pos() - m_dragWordStartPos;
        // The pointer speaks in item coordinates, what is stored speaks of the drawing: the gesture
        // is converted at every step, so the word lands where the mouse is and is remembered as a
        // spot of the piece — the block's frame never enters what is kept.
        if (m_dragTarget == kLegendTarget)
        {
            m_legendOffset = WordPlacement(m_dragWordStart + delta);
        }
        else
        {
            m_labels[m_dragTarget].offset = WordPlacement(m_dragWordStart + delta);
        }
        // The words can be moved out of the block's own box; boundingRect() follows them, so the
        // item keeps claiming everything it paints.
        prepareGeometryChange();
        update();
        event->accept();
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

    if (m_editing)
    {
        const int target = m_dragTarget;
        m_dragTarget = -2;
        setCursor(Qt::OpenHandCursor);
        event->accept();

        if (target == -2)
        {
            return;
        }

        // Judged in item coordinates, where the gesture happened — the stored place now lives on the
        // drawing, and a round trip through the fit is not an exact equality: a click would read as a
        // move and freeze the word out of its row for good. Half a pixel is a hand shaking.
        const QPointF landed =
            target == kLegendTarget ? EffectiveLegendOffset() : EffectiveOffset(target);
        const bool moved = QLineF(landed, m_dragWordStart).length() > 0.5;
        if (not moved)
        {
            // A click that moved nothing is not a placement: putting the word back in the default
            // row is what keeps a mere click from freezing it out of that row for good.
            if (target == kLegendTarget)
            {
                m_legendPlaced = m_dragWordWasPlaced;
            }
            else
            {
                m_labels[target].placed = m_dragWordWasPlaced;
            }
            return;
        }

        if (target == kLegendTarget)
        {
            emit LegendMoved(m_legendOffset);
        }
        else
        {
            emit LabelMoved(target, m_labels.at(target).offset);
        }
        return;
    }

    const bool resized = m_activeHandle != Handle::None;
    const bool moved = pos() != m_dragStartPos;
    // **A hand that shook is still a click.** Judging the entry on any movement at all would refuse
    // it to anyone but a machine, and would leave a one-pixel nudge — with its undo step — in place
    // of the miniature.
    const qreal travel = QLineF(event->scenePos(), m_dragStartScenePos).length();
    const bool click = m_pendingEnter && travel < QApplication::startDragDistance();
    m_activeHandle = Handle::None;
    m_pendingEnter = false;
    setCursor(Qt::SizeAllCursor);
    event->accept();

    if (click)
    {
        if (moved)
        {
            // Opening the miniature must not leave the nudge that preceded it behind.
            setPos(m_dragStartPos);
            setRect(m_dragStartRect);
            m_geometryChanged = false;
        }
        SetEditing(true);
        return;
    }

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
        if (m_editing)
        {
            setCursor(HitTestWord(event->pos()) == -2 ? Qt::ArrowCursor : Qt::OpenHandCursor);
        }
        else
        {
            setCursor(CursorForHandle(HitTestHandle(event->pos())));
        }
    }
    QGraphicsRectItem::hoverMoveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (not m_readOnly)
    {
        setCursor(m_editing ? Qt::ArrowCursor : Qt::SizeAllCursor);
    }
    QGraphicsRectItem::hoverLeaveEvent(event);
}
