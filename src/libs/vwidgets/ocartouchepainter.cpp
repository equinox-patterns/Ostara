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
#include <QRegularExpression>
#include <QStringList>
#include <QTransform>

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
 * @brief The width of the column the marks of a block stand in — nought when it has none.
 *
 * **The column belongs to the block, not to the line.** A legend is read down its marks and then
 * across its words: a mark too narrow to fill the column would let its word begin short of the
 * others, and words beginning together is the whole point of the two.
 */
auto MarksColumn(const QVector<OCartoucheRenderLine> &lines) -> qreal
{
    qreal column = 0.0;
    for (const OCartoucheRenderLine &line : lines)
    {
        if (line.kind == OCartoucheRenderLine::Kind::Text && !line.marks.IsEmpty())
        {
            column = (std::max)(column, line.marks.size.width());
        }
    }
    return column;
}

/**
 * @brief What a stack of lines comes to, with no margin around it.
 *
 * @param gapScale multiplies the fixed gaps, the line-spacing option and a trace sample's length,
 *                 and nothing else: a font is pre-scaled by ScaledLines before it arrives here,
 *                 exactly as LineHeight has it, and what a shape measures is geometry. Measured at
 *                 1, this is what NaturalSize pads with its two margins.
 */
auto InkSize(const QVector<OCartoucheRenderLine> &lines, qreal gapScale) -> QSizeF
{
    // The column the marks of this block stand in, if it has any — the block's, not a line's.
    const qreal marksColumn = MarksColumn(lines);

    qreal width = 0.0;
    qreal height = 0.0;
    for (int i = 0; i < lines.size(); ++i)
    {
        const OCartoucheRenderLine &line = lines.at(i);
        const bool hasNext = i != lines.size() - 1;

        // What the line takes on its own, and the least it may be given and still be drawn whole.
        // The two are equal for a logo and a shape, which are exactly what they measure; a line of
        // text is a font's box with its glyphs in the middle of it, so it can be tightened by the
        // empty part of that box and no further.
        qreal own = 0.0;
        qreal drawn = 0.0;

        switch (line.kind)
        {
            case OCartoucheRenderLine::Kind::Text:
            {
                // Ink bounds, not the pen advance: script/calligraphy fonts overhang their
                // advance, so the block must be sized to the glyph ink.
                const QFontMetricsF fm(line.font);
                const QRectF box = fm.boundingRect(line.text);
                qreal lineWidth = box.width();
                if (line.trace.HasTrace())
                {
                    // **The sample is part of what the line measures.** What stands beside the words
                    // takes room on the page like the words do, and a block sized to the text alone
                    // would let its samples run out of the band it was given.
                    lineWidth += (line.text.isEmpty() ? 0.0 : kTraceGap * gapScale)
                                 + line.trace.lengthPx * gapScale;
                }
                if (!line.marks.IsEmpty())
                {
                    // **And so are the marks**, on the other side: a mark stands on the page like the
                    // words do, and a block sized to the text alone would let it run out of its band.
                    // What the line reserves is the block's column rather than its own marks, since
                    // that is what will actually be left of it — see MarksColumn.
                    lineWidth += kMarksGap * gapScale + marksColumn;
                }
                width = (std::max)(width, lineWidth);

                // The glyphs are written centred in the band of the font's height (@ref CentredPen),
                // so the lowest the writing reaches is that band's middle plus half the ink below
                // the baseline. **That ink is asked of tightBoundingRect, never of boundingRect**:
                // boundingRect answers with the font's line box — its bottom is the font's descent
                // whether or not one single glyph reaches down there — so a floor taken from it
                // equals the line's own height and no spacing could ever tighten anything. The room
                // a line has to give is exactly what its glyphs do not reach: the whole descent for
                // a line of capitals, nothing at all for one whose letters hang low.
                //
                // It is then capped at the line's own height, so that ink reaching a hair below the
                // font's descent changes nothing: at a spacing of 0 a block measures what it always
                // measured. A line with no words draws nothing, and there is nothing to keep inside.
                const qreal floor = (fm.height() - fm.ascent() - fm.descent()) / 2.0 + fm.ascent()
                                    + fm.tightBoundingRect(line.text).bottom();
                const qreal textDrawn = line.text.isEmpty() ? 0.0 : (std::min)(fm.height(), floor);

                // **A line carrying marks is as tall as they are.** They are drawn at the size they
                // will print at, which is not the size of the words beside them — a grain line is
                // taller than any line of text — so the band has to hold the taller of the two. And
                // what they take cannot be tightened away: a mark drawn shorter than it is would be a
                // mark the reader measures wrong, so the spacing closes the room below the text and
                // never the room the marks stand in.
                const qreal marksHeight = line.marks.IsEmpty() ? 0.0 : line.marks.size.height();
                own = (std::max)(fm.height(), marksHeight);
                drawn = (std::max)(textDrawn, marksHeight);
                break;
            }
            case OCartoucheRenderLine::Kind::Logo:
                width = (std::max)(width, static_cast<qreal>(line.logo.width()));
                own = drawn = line.logoHeight;
                break;
            case OCartoucheRenderLine::Kind::Shape:
            {
                const QRectF bounds = ShapeBounds(line);
                width = (std::max)(width, bounds.width());
                own = drawn = bounds.height();
                break;
            }
        }

        // **The spacing is counted below every line, the last one included.** That is what the
        // option says it is — room below the line — and the last line of a block has a bottom like
        // any other: it is the one the band underneath is brought closer by, which no single-line
        // information could do at all while the spacing was counted between lines only. The fixed
        // gap stays what it is, a separation between two lines: after the last one there is nothing
        // left to separate it from.
        //
        // **A line may be tightened, never eaten.** What the spacing closes is the empty room below
        // a line, and it stops where the line's own drawing begins: a block measured shorter than
        // what it draws would have its words standing outside it.
        qreal slot = own + line.lineSpacing * gapScale;
        if (hasNext)
        {
            slot += kLineGap * gapScale;
        }
        height += (std::max)(slot, drawn);
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
 *
 * @param extraRight room to reserve on the right of the ink, for something drawn after the text.
 *                   **It is reserved, not ignored**: the text and what follows it are centred
 *                   together, so a line carrying a trace sample sits where the two of them belong
 *                   rather than where the words alone would.
 * @param extraLeft  the same on the other side, for something drawn *before* the text — the marks a
 *                   legend line stands for. Text and marks are centred as one block, so the words sit
 *                   past where the marks end rather than on top of them.
 */
auto Baseline(const QFontMetricsF &fm, qreal top) -> qreal
{
    return top + (fm.height() - fm.ascent() - fm.descent()) / 2.0 + fm.ascent();
}

auto CentredPen(const QRectF &rect, qreal top, const QFontMetricsF &fm, const QRectF &ink,
                qreal extraRight = 0.0, qreal extraLeft = 0.0) -> QPointF
{
    const qreal total = extraLeft + ink.width() + extraRight;
    return QPointF(rect.left() + rect.width() / 2.0 - total / 2.0 - ink.left() + extraLeft,
                   Baseline(fm, top));
}
} // namespace

OCartoucheRenderLine TextLine(const QString &text, const QFont &font, int lineSpacing)
{
    return TextLineWithTrace(text, font, lineSpacing, OCartoucheTraceSample{});
}

OCartoucheRenderLine TextLineWithTrace(const QString &text, const QFont &font, int lineSpacing,
                                       const OCartoucheTraceSample &trace)
{
    OCartoucheRenderLine line;
    line.kind = OCartoucheRenderLine::Kind::Text;
    line.text = text;
    line.font = font;
    line.lineSpacing = lineSpacing;
    line.trace = trace;
    return line;
}

OCartoucheRenderLine TextLineWithSymbols(const QString &text, const QFont &font, int lineSpacing,
                                         const OCartoucheSymbolMarks &marks)
{
    OCartoucheRenderLine line;
    line.kind = OCartoucheRenderLine::Kind::Text;
    line.text = text;
    line.font = font;
    line.lineSpacing = lineSpacing;
    line.marks = marks;
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

QVector<OCartoucheRenderLine> TextLinesFitting(const QString &text, const QFont &font,
                                               int lineSpacing, qreal maxWidth)
{
    if (maxWidth <= 0.0)
    {
        return TextLines(text, font, lineSpacing);
    }

    const QFontMetricsF metrics(font);
    QVector<OCartoucheRenderLine> lines;

    // One paragraph at a time: the breaks the user typed are theirs and are kept as they are. Only
    // what a width makes necessary is added.
    const QStringList paragraphs = text.split(QLatin1Char('\n'));
    lines.reserve(paragraphs.size());
    for (const QString &paragraph : paragraphs)
    {
        // A blank line is a line: it takes its height and moves what follows, which is what the
        // user asked for when they pressed Enter twice.
        if (paragraph.isEmpty())
        {
            lines.append(TextLine(QString(), font, lineSpacing));
            continue;
        }

        QString current;
        // Split on whitespace and keep nothing of it: a line break replaces the space it was taken
        // from, so a wrapped text does not start with the space that ended the previous line.
        const QStringList words = paragraph.split(QRegularExpression(QStringLiteral("\\s+")),
                                                  Qt::SkipEmptyParts);
        for (const QString &word : words)
        {
            const QString candidate = current.isEmpty() ? word : current + QLatin1Char(' ') + word;
            if (current.isEmpty() || metrics.boundingRect(candidate).width() <= maxWidth)
            {
                // The first word of a line is taken whatever its width: a word too wide for the
                // page still has to be written somewhere, and it is the smallest thing that can be
                // moved to the next line.
                current = candidate;
            }
            else
            {
                lines.append(TextLine(current, font, lineSpacing));
                current = word;
            }
        }
        lines.append(TextLine(current, font, lineSpacing));
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
                if (!line.marks.IsEmpty())
                {
                    // The marks are geometry, so they are brought to the scale here, where the font is
                    // — and, like the font, **once**: the painter lays them out at the size it is
                    // given, which is why InkSize measures them as they arrive rather than scaling
                    // them again.
                    const QTransform transform = QTransform::fromScale(fontScale, fontScale);
                    for (QPainterPath &path : line.marks.strokes)
                    {
                        path = transform.map(path);
                    }
                    for (QPainterPath &path : line.marks.fills)
                    {
                        path = transform.map(path);
                    }
                    line.marks.size *= fontScale;
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

namespace
{
/**
 * @brief The option key one part of a typeface is stored under, for a group or for the whole.
 *
 * **The two spellings are not the same, and that is the whole of this function.** The typography a
 * section is set in is stored as `fontFamily`, `fontWeight`, `fontSize`; the typography of a *named
 * group* — the corner labels of a test square, the axes of a cover page — is stored as
 * `axisFontFamily`, `sizesFontSize` and so on. The group's name carries into the key, and the word
 * after it is capitalised there and not here.
 *
 * Getting this wrong is silent: the lookup misses, no font is found, and the group is drawn in the
 * section's own type — which looks like a control that does nothing. It is written once, here,
 * because four callers pass a prefix and every one of them would have to remember the spelling.
 */
auto FontKey(const QString &prefix, const char *stem) -> QString
{
    if (prefix.isEmpty())
    {
        return QString::fromLatin1(stem);
    }
    QString capitalized = QString::fromLatin1(stem);
    capitalized[0] = capitalized.at(0).toUpper();
    return prefix + capitalized;
}
} // namespace

QFont SectionFont(const QHash<QString, QVariant> &options, const QFont &baseFont,
                  const QString &prefix)
{
    QFont font = baseFont;
    const QString family = options.value(FontKey(prefix, "fontFamily")).toString();
    const int weight = options.value(FontKey(prefix, "fontWeight"), 0).toInt();
    const double size = options.value(FontKey(prefix, "fontSize"), 0.0).toDouble();
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
    // The content plus two margins, top and bottom, and one on each side. The fixed gap falls
    // BETWEEN lines only: counting the trailing one would drop the bottom padding, and the block
    // would be smaller than its content — what is drawn on it would be cropped. The spacing the
    // user asks for is another matter: it is room below a line, so it is counted below every line,
    // the last one included (@ref InkSize).
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

    // **The marks column of the block, and the width of the block that holds it.** Worked out once,
    // for every line: what makes the words of a legend begin together is that each of them is placed
    // against the same edge, and no line can know that on its own. A block with no marks — every
    // information but a legend — comes out of this with nothing to align, and lays out as it always
    // has.
    const qreal marksColumn = MarksColumn(lines);
    const qreal marksReserve = marksColumn > 0.0 ? marksColumn + kMarksGap * scale : 0.0;
    qreal blockWidth = 0.0;
    for (const OCartoucheRenderLine &line : lines)
    {
        if (line.kind == OCartoucheRenderLine::Kind::Text && !line.marks.IsEmpty())
        {
            blockWidth =
                (std::max)(blockWidth,
                           marksReserve + QFontMetricsF(line.font).boundingRect(line.text).width());
        }
    }
    const qreal blockLeft = rect.center().x() - blockWidth / 2.0;
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
                const QRectF ink = fm.boundingRect(line.text);
                OCartoucheRenderItem item;
                item.kind = OCartoucheRenderItem::Kind::Text;
                item.text = line.text;
                item.font = line.font;

                // The band the line's ink stands in: the font's own height, or the marks' when they
                // are the taller. **Text and marks share its middle**, so a legend line reads as one
                // thing rather than as a word pinned to the top of a picture.
                const qreal marksWidth = line.marks.IsEmpty() ? 0.0 : line.marks.size.width();
                const qreal marksHeight = line.marks.IsEmpty() ? 0.0 : line.marks.size.height();
                const qreal bandHeight = (std::max)(fm.height(), marksHeight);
                const qreal bandTop = y + (bandHeight - fm.height()) / 2.0;

                if (!line.marks.IsEmpty())
                {
                    // **Placed in the block's column rather than centred on its own width.** The marks
                    // stand centred in the column and the words begin at its right edge, which is what
                    // puts every word of a legend on the same x: a line centred on its own length would
                    // put the shortest one's word first. See MarksColumn.
                    item.pen = QPointF(blockLeft + marksReserve - ink.left(), Baseline(fm, bandTop));
                    item.marks = line.marks;
                    item.marksRect =
                        QRectF(blockLeft + (marksColumn - marksWidth) / 2.0,
                               y + (bandHeight - marksHeight) / 2.0, marksWidth, marksHeight);
                }
                else if (line.trace.HasTrace())
                {
                    // The gap separates two things, so a sample following no words asks for none —
                    // the way the trailing inter-line gap is not part of what the content measures.
                    const qreal gap = line.text.isEmpty() ? 0.0 : kTraceGap * scale;
                    const qreal length = line.trace.lengthPx * scale;
                    // **Text and sample are centred together**, which is what "one line" means to
                    // the reader: the pair moves as a whole, so neither is pushed off centre.
                    item.pen = CentredPen(rect, bandTop, fm, ink, gap + length);
                    const qreal left = item.pen.x() + ink.left() + ink.width() + gap;
                    // Centred on the words' own ink, measured from the baseline the pen sits on: a
                    // sample drawn on the baseline would hang below the text it belongs to.
                    const qreal middle = item.pen.y() + ink.center().y();
                    item.trace = QLineF(left, middle, left + length, middle);
                    item.traceColor = line.trace.color;
                    item.tracePenWidth = line.trace.penWidth * scale;
                    item.traceDashes = line.trace.dashes;
                    for (qreal &value : item.traceDashes)
                    {
                        value *= scale;
                    }
                }
                else
                {
                    item.pen = CentredPen(rect, bandTop, fm, ink);
                }

                items.append(item);
                // The line takes the band it stands in, not the font's height: a band measured on the
                // font alone would let a taller mark run into the line below it.
                y += bandHeight + (kLineGap + line.lineSpacing) * scale;
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

QPen TracePen(const OCartoucheRenderItem &item)
{
    QPen pen(item.traceColor);
    pen.setWidthF(item.tracePenWidth);
    if (item.traceDashes.isEmpty())
    {
        pen.setStyle(Qt::SolidLine);
    }
    else
    {
        // A Qt dash pattern counts in multiples of the pen width, so a pixel length is divided by
        // it here — the exact inverse of what the SVG writer multiplies back.
        const qreal width = item.tracePenWidth > 0.0 ? item.tracePenWidth : 1.0;
        QVector<qreal> pattern;
        pattern.reserve(item.traceDashes.size());
        for (qreal value : item.traceDashes)
        {
            pattern.append(value / width);
        }
        pen.setStyle(Qt::CustomDashLine);
        pen.setDashPattern(pattern);
    }
    // Butt, so a dash ends where it says it does: Qt's default square cap stretches each dash by
    // half a width, and the SVG export caps at butt — the same choice for the same reason as the
    // size traces drawn on the canvas.
    pen.setCapStyle(Qt::FlatCap);
    return pen;
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
                if (!item.marks.IsEmpty())
                {
                    // **Drawn in the caller's ink, unlike the sample beside the words.** A mark is not
                    // content carrying a colour of its own — it is the shape the legend is made of,
                    // and it belongs to the page the way the words do. Traced first, then filled:
                    // an arrowhead is filled over the line it stands on, never under it.
                    painter.save();
                    {
                        const QColor ink = painter.pen().color();
                        const QTransform place =
                            QTransform::fromTranslate(item.marksRect.left(), item.marksRect.top());

                        painter.setBrush(Qt::NoBrush);
                        for (const QPainterPath &path : item.marks.strokes)
                        {
                            painter.drawPath(place.map(path));
                        }

                        painter.setPen(Qt::NoPen);
                        painter.setBrush(ink);
                        for (const QPainterPath &path : item.marks.fills)
                        {
                            painter.drawPath(place.map(path));
                        }
                    }
                    painter.restore();
                }
                painter.setFont(item.font);
                painter.drawText(item.pen, item.text);
                if (!item.trace.isNull())
                {
                    // **The one thing on a line that carries its own ink.** The words around it are
                    // written in the caller's pen; the sample is drawn in the size's own colour and
                    // dash, so the pen is borrowed and given back — as the shape below borrows its
                    // own, and for the same reason: the words of the next line must be the caller's
                    // again.
                    //
                    // **Not cosmetic, unlike the shape's outline.** The outline is a rule, so it
                    // stays measurable at any scale; the sample is content, so it is drawn at the
                    // page's scale and its dashes keep their pitch against the segment however far
                    // the page is reduced. A cosmetic pen here would keep the device-pixel pitch and
                    // show a four-centimetre sample at several times the real number of dashes —
                    // the sample would be lying about the trace it stands for.
                    //
                    // No render hint is set either: the caller's antialiasing is what the sample
                    // wants, and the shape arm's deliberate blur-free setting belongs to the rule.
                    painter.save();
                    {
                        painter.setPen(TracePen(item));
                        painter.drawLine(item.trace);
                    }
                    painter.restore();
                }
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
