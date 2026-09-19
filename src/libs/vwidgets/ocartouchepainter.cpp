/************************************************************************
 **
 **  @file   ocartouchepainter.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Shared cartouche rendering formula (see ocartouchepainter.h).
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

#include "ocartouchepainter.h"

#include <QApplication>
#include <QFontMetricsF>

#include <algorithm>

namespace OCartouchePainter
{
namespace
{
/**
 * @brief What a shape's outlines and placed texts come to, in the shape's own coordinates.
 *
 * It is both what the line measures and the frame its content is drawn in, so a shape written
 * around an origin of its own keeps it rather than being silently moved to (0, 0).
 */
auto ShapeBounds(const OCartoucheRenderLine &line) -> QRectF
{
    QRectF bounds;
    for (const QRectF &outline : line.shapeOutlines)
    {
        bounds = bounds.united(outline);
    }
    for (const OCartoucheShapeLabel &label : line.shapeLabels)
    {
        bounds = bounds.united(label.box);
    }
    return bounds;
}

/**
 * @brief What a stack of lines comes to, with no margin around it.
 *
 * @param gapScale multiplies the fixed gap and the line-spacing option, and nothing else: a font
 *                 is pre-scaled by ScaledLines before it arrives here, exactly as LineHeight has
 *                 it. Measured at 1, this is what NaturalSize pads with its two margins.
 */
auto InkSize(const QVector<OCartoucheRenderLine> &lines, qreal gapScale) -> QSizeF
{
    qreal width = 0.0;
    qreal height = 0.0;
    for (int i = 0; i < lines.size(); ++i)
    {
        const OCartoucheRenderLine &line = lines.at(i);
        switch (line.kind)
        {
            case OCartoucheRenderLine::Kind::Text:
                // Ink bounds, not the pen advance: script/calligraphy fonts overhang their
                // advance, so the block must be sized to the glyph ink.
                width = (std::max)(width, QFontMetricsF(line.font).boundingRect(line.text).width());
                height += QFontMetricsF(line.font).height();
                break;
            case OCartoucheRenderLine::Kind::Logo:
                width = (std::max)(width, static_cast<qreal>(line.logo.width()));
                height += line.logoHeight;
                break;
            case OCartoucheRenderLine::Kind::Shape:
            {
                const QRectF bounds = ShapeBounds(line);
                width = (std::max)(width, bounds.width());
                height += bounds.height();
                break;
            }
        }
        // Between lines only: a trailing gap is not part of what the content measures.
        if (i != lines.size() - 1)
        {
            height += (kLineGap + line.lineSpacing) * gapScale;
        }
    }
    return {width, height};
}

/**
 * @brief Where one line of text is written: its glyph ink centred in @p rect, on the band of the
 * font's own height that starts at @p top.
 *
 * Center the glyph ink, not the pen advance: script/calligraphy glyphs overhang their advance, so
 * advance-based centering shifts the line sideways. Vertical placement stays consistent (ascent +
 * descent centered in the line band). The two callers — a stacked line, and a label a shape places
 * in a box of its own — write a line the same way because it is the same line.
 */
auto CentredPen(const QRectF &rect, qreal top, const QFontMetricsF &fm, const QRectF &ink) -> QPointF
{
    return QPointF(rect.left() + rect.width() / 2.0 - ink.left() - ink.width() / 2.0,
                   top + (fm.height() - fm.ascent() - fm.descent()) / 2.0 + fm.ascent());
}
} // namespace

OCartoucheRenderLine TextLine(const QString &text, const QFont &font, int lineSpacing)
{
    OCartoucheRenderLine line;
    line.kind = OCartoucheRenderLine::Kind::Text;
    line.text = text;
    line.font = font;
    line.lineSpacing = lineSpacing;
    return line;
}

QVector<OCartoucheRenderLine> TextLines(const QString &text, const QFont &font, int lineSpacing)
{
    QVector<OCartoucheRenderLine> lines;
    // A blank line is a line: it takes its height and moves what follows, which is what the user
    // asked for when they pressed Enter twice.
    const QStringList written = text.split(QLatin1Char('\n'));
    lines.reserve(written.size());
    for (const QString &one : written)
    {
        lines.append(TextLine(one, font, lineSpacing));
    }
    return lines;
}

OCartoucheRenderLine LogoLine(const QPixmap &logo, qreal logoHeight, int lineSpacing,
                              const QString &sourcePath)
{
    OCartoucheRenderLine line;
    line.kind = OCartoucheRenderLine::Kind::Logo;
    line.logo = logo;
    line.logoHeight = logoHeight;
    line.lineSpacing = lineSpacing;
    line.sourcePath = sourcePath;
    return line;
}

OCartoucheRenderLine ShapeLine(const QVector<QRectF> &outlines,
                               const QVector<OCartoucheShapeLabel> &labels, int lineSpacing)
{
    OCartoucheRenderLine line;
    line.kind = OCartoucheRenderLine::Kind::Shape;
    line.shapeOutlines = outlines;
    line.shapeLabels = labels;
    line.lineSpacing = lineSpacing;
    return line;
}

QVector<OCartoucheRenderLine> ScaledLines(const QVector<OCartoucheRenderLine> &lines, qreal fontScale)
{
    QVector<OCartoucheRenderLine> scaled = lines;
    for (OCartoucheRenderLine &line : scaled)
    {
        switch (line.kind)
        {
            case OCartoucheRenderLine::Kind::Text:
                if (line.font.pixelSize() > 0)
                {
                    QFont font = line.font;
                    font.setPixelSize(qRound(font.pixelSize() * fontScale));
                    line.font = font;
                }
                break;
            case OCartoucheRenderLine::Kind::Shape:
                // The texts a shape places are set in their own fonts like any other text, and a
                // block redrawn smaller redraws them smaller. What the shape measures is geometry
                // and does not scale here: the caller scales the rect it is laid out in.
                for (OCartoucheShapeLabel &label : line.shapeLabels)
                {
                    if (label.font.pixelSize() > 0)
                    {
                        QFont font = label.font;
                        font.setPixelSize(qRound(font.pixelSize() * fontScale));
                        label.font = font;
                    }
                }
                break;
            case OCartoucheRenderLine::Kind::Logo:
                // A logo is a pixmap already rendered at the size it is drawn at: there is no font
                // in it to scale.
                break;
        }
    }
    return scaled;
}

QFont BaseFont()
{
    return QApplication::font();
}

QFont SectionFont(const QHash<QString, QVariant> &options, const QFont &baseFont,
                  const QString &prefix)
{
    QFont font = baseFont;
    const QString family = options.value(prefix + QStringLiteral("fontFamily")).toString();
    const int weight = options.value(prefix + QStringLiteral("fontWeight"), 0).toInt();
    const double size = options.value(prefix + QStringLiteral("fontSize"), 0.0).toDouble();
    if (!family.isEmpty())
    {
        font.setFamily(family);
    }
    if (weight > 0)
    {
        font.setWeight(QFont::Weight(weight));
    }
    if (size > 0.0)
    {
        font.setPixelSize(qRound(size));
    }
    return font;
}

qreal LineHeight(const QFont &font, int lineSpacing, qreal scale)
{
    return QFontMetricsF(font).height() + (kLineGap + lineSpacing) * scale;
}

qreal LogoLineHeight(qreal logoHeight, int lineSpacing, qreal scale)
{
    return logoHeight * scale + (kLineGap + lineSpacing) * scale;
}

qreal ShapeLineHeight(qreal shapeHeight, int lineSpacing, qreal scale)
{
    return shapeHeight * scale + (kLineGap + lineSpacing) * scale;
}

QSizeF NaturalSize(const QVector<OCartoucheRenderLine> &lines)
{
    // The content plus two margins, top and bottom, and one on each side. The inter-line gap falls
    // BETWEEN lines only: counting the trailing one would drop the bottom padding, and the block
    // would be smaller than its content — what is drawn on it would be cropped.
    const QSizeF ink = InkSize(lines, 1.0);
    return {ink.width() + 2.0 * kMargin, ink.height() + 2.0 * kMargin};
}

QVector<OCartoucheRenderItem> Layout(const QRectF &rect, const QVector<OCartoucheRenderLine> &lines, qreal scale)
{
    QVector<OCartoucheRenderItem> items;
    // The margins scale with the content so a zoomed block stays a clean
    // proportional copy of its content (a fixed margin would shrink the padding
    // on every resize).
    const qreal margin = kMargin * scale;
    qreal y = rect.top() + margin;
    for (const OCartoucheRenderLine &line : lines)
    {
        if (y > rect.bottom())
        {
            break;
        }
        switch (line.kind)
        {
            case OCartoucheRenderLine::Kind::Text:
            {
                const QFontMetricsF fm(line.font);
                OCartoucheRenderItem item;
                item.kind = OCartoucheRenderItem::Kind::Text;
                item.text = line.text;
                item.font = line.font;
                item.pen = CentredPen(rect, y, fm, fm.boundingRect(line.text));
                items.append(item);
                y += LineHeight(line.font, line.lineSpacing, scale);
                break;
            }
            case OCartoucheRenderLine::Kind::Logo:
            {
                OCartoucheRenderItem item;
                item.kind = OCartoucheRenderItem::Kind::Logo;
                item.pixmap = line.logo;
                item.sourcePath = line.sourcePath;
                const qreal drawW = line.logo.width() * scale;
                const qreal drawH = line.logoHeight * scale;
                item.target = QRectF(rect.left() + (rect.width() - drawW) / 2.0, y, drawW, drawH);
                items.append(item);
                y += LogoLineHeight(line.logoHeight, line.lineSpacing, scale);
                break;
            }
            case OCartoucheRenderLine::Kind::Shape:
            {
                // The shape takes the width it says it has, centred like any other line, and what
                // it carries is placed against it rather than stacked after it — a corner label
                // belongs at a corner. So the shape's own frame is mapped into the page once, and
                // every outline and every text is read through that one mapping.
                const QRectF bounds = ShapeBounds(line);
                const qreal drawW = bounds.width() * scale;
                const QPointF origin(rect.left() + (rect.width() - drawW) / 2.0 - bounds.left() * scale,
                                     y - bounds.top() * scale);

                for (const QRectF &outline : line.shapeOutlines)
                {
                    OCartoucheRenderItem item;
                    item.kind = OCartoucheRenderItem::Kind::Shape;
                    item.target = QRectF(origin.x() + outline.left() * scale,
                                         origin.y() + outline.top() * scale, outline.width() * scale,
                                         outline.height() * scale);
                    items.append(item);
                }

                for (const OCartoucheShapeLabel &label : line.shapeLabels)
                {
                    const QRectF box(origin.x() + label.box.left() * scale,
                                     origin.y() + label.box.top() * scale,
                                     label.box.width() * scale, label.box.height() * scale);
                    const QVector<OCartoucheRenderLine> text =
                        TextLines(label.text, label.font, label.lineSpacing);
                    const QSizeF ink = InkSize(text, scale);

                    // The whole block of lines is placed in the label's box first, and each line
                    // is then written centred in it — the same rule the stack above follows.
                    qreal left = box.left();
                    if (label.align.testFlag(Qt::AlignHCenter))
                    {
                        left = box.center().x() - ink.width() / 2.0;
                    }
                    else if (label.align.testFlag(Qt::AlignRight))
                    {
                        left = box.right() - ink.width();
                    }

                    qreal top = box.top();
                    if (label.align.testFlag(Qt::AlignVCenter))
                    {
                        top = box.center().y() - ink.height() / 2.0;
                    }
                    else if (label.align.testFlag(Qt::AlignBottom))
                    {
                        top = box.bottom() - ink.height();
                    }

                    for (const OCartoucheRenderLine &one : text)
                    {
                        const QFontMetricsF fm(one.font);
                        OCartoucheRenderItem item;
                        item.kind = OCartoucheRenderItem::Kind::Text;
                        item.text = one.text;
                        item.font = one.font;
                        item.pen = CentredPen(QRectF(left, top, ink.width(), fm.height()), top, fm,
                                             fm.boundingRect(one.text));
                        items.append(item);
                        top += LineHeight(one.font, one.lineSpacing, scale);
                    }
                }

                y += ShapeLineHeight(bounds.height(), line.lineSpacing, scale);
                break;
            }
        }
    }
    return items;
}

void Paint(QPainter &painter, const QRectF &rect, const QVector<OCartoucheRenderLine> &lines, qreal scale)
{
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QVector<OCartoucheRenderItem> items = Layout(rect, lines, scale);
    for (const OCartoucheRenderItem &item : items)
    {
        switch (item.kind)
        {
            case OCartoucheRenderItem::Kind::Text:
                painter.setFont(item.font);
                painter.drawText(item.pen, item.text);
                break;
            case OCartoucheRenderItem::Kind::Logo:
                painter.drawPixmap(item.target, item.pixmap, QRectF(item.pixmap.rect()));
                break;
            case OCartoucheRenderItem::Kind::Shape:
                // An outline, and nothing but: no fill, and a black ink of its own rather than
                // whichever pen the caller set for the words around it.
                //
                // **One pixel on the screen, whatever the page is shown at.** The line is not part
                // of what the page is made of — it is what a needleworker measures against — so it
                // does not shrink with the page: multiplied by the display factor it would come out
                // under a pixel and be drawn as a half-lit blur, thinner the further the page is
                // reduced and thinner again wherever its edges happen to fall between two pixels.
                // A line that says « this is five centimetres » has to be the same line wherever it
                // lands, which is also why antialiasing is off for it: a one-pixel line split
                // across two pixels is a line nobody can measure against. Both are put back before
                // the words that follow, which do want them.
                painter.save();
                {
                    QPen pen(Qt::black);
                    pen.setWidthF(kShapePenWidth);
                    pen.setCosmetic(true);
                    painter.setPen(pen);
                    painter.setBrush(Qt::NoBrush);
                    painter.setRenderHint(QPainter::Antialiasing, false);
                    painter.drawRect(item.target);
                }
                painter.restore();
                break;
        }
    }
}
}  // namespace OCartouchePainter
