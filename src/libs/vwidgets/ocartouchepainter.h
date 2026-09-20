/************************************************************************
 **
 **  @file   ocartouchepainter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Shared cartouche rendering formula — one stacked line per
 **          section, fonts from the template options. Used by the editor
 **          live preview and by the on-piece information blocks so the
 **          two always render identically.
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

#ifndef OCARTOUCHEPAINTER_H
#define OCARTOUCHEPAINTER_H

#include <QColor>
#include <QFont>
#include <QHash>
#include <QLineF>
#include <QPainter>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QSizeF>
#include <QVariant>
#include <QVector>
#include <QtGlobal>

/**
 * @brief A sample of a size's trace, drawn after a line's text, on the same line.
 *
 * **A sample is read, not measured.** It stands for the pen a graded size is drawn with — its
 * colour, its dash pitch — over a length the user typed in centimetres. It is therefore content,
 * exactly like the words beside it, and it scales with them. That is what separates it from a test
 * square's outline, which is a rule and stays one screen pixel wide whatever the page is scaled to.
 *
 * Every length here is in print pixels at scale 1, like the font's pixel size. `dashes` is **not**
 * in the points the user works in: the resolver has already converted, because points are a unit of
 * the style layer and the painter knows only pixels. Layout scales the three, and Paint divides the
 * dashes by the pen width, Qt counting a dash pattern in multiples of it.
 */
struct OCartoucheTraceSample
{
    bool enabled = false;          ///< false — the line is plain text, and nothing is drawn.
    qreal lengthPx = 0.0;          ///< print px — the centimetres the user chose, converted.
    QColor color = Qt::black;      ///< the size's own colour.
    QVector<qreal> dashes;         ///< print px, [dash, space, ...] — empty = solid.
    qreal penWidth = 1.0;          ///< print px. Never cosmetic: see Paint.

    /// True when the line carries a sample worth drawing — one with nothing to draw is no sample.
    auto HasTrace() const -> bool { return enabled && lengthPx > 0.0; }
};

/**
 * @brief One text a shape places inside itself, at the spot its author chose.
 *
 * A shape is a piece of geometry, and what it carries is placed against the geometry rather than
 * stacked after it: a corner label sits at a corner, a caption sits in the middle. So the box and
 * the alignment are part of the text, not of the painter's line formula.
 *
 * The text may run over several lines, and — as everywhere else — one break is one line on the
 * page. The whole block is aligned in @ref box: `Qt::AlignLeft | Qt::AlignTop` starts it at the
 * box's top-left corner, `Qt::AlignCenter` centres it on the box's middle.
 */
struct OCartoucheShapeLabel
{
    QRectF box;                             // in the shape's own coordinates (px).
    Qt::Alignment align = Qt::AlignCenter;  // where the text sits in its box.
    QString text;                           // one break, one line.
    QFont font;                             // fully styled, as a text line's is.
    int lineSpacing = 0;                    // extra px below the line (0 = default).
};

/**
 * @brief One rendered line of an information block (text, logo, or geometry).
 *
 * The font is fully styled (family / weight / pixel size) before being
 * stored; the painter only lays the line out.
 *
 * **A shape is the line that is not text and not an image.** Some information is a piece of
 * geometry — a square a needleworker measures against a ruler, the tracé a size is graded along —
 * with words placed against that geometry. It measures by its own box rather than by a font, and
 * it draws outlines instead of glyphs, which the other two kinds cannot express.
 */
struct OCartoucheRenderLine
{
    enum class Kind
    {
        Text,
        Logo,
        Shape
    };

    Kind kind = Kind::Text;
    QString text;         // Kind::Text — the line text.
    QFont font;           // Kind::Text — fully styled font.
    int lineSpacing = 0;  // Kind::Text, Kind::Shape — extra px below the line (0 = default).
    QPixmap logo;         // Kind::Logo — pre-rendered logo pixmap.
    qreal logoHeight = 0; // Kind::Logo — rendered height in px.
    QString sourcePath;   // Kind::Logo — original SVG file path; Paint ignores it,
                          // the SVG writer re-inlines this file (Val Fork behaviour).
    QVector<QRectF> shapeOutlines;               // Kind::Shape — rectangles to stroke, unfilled.
    QVector<OCartoucheShapeLabel> shapeLabels;   // Kind::Shape — texts placed against them.

    /**
     * Kind::Text — a sample of a size's trace, drawn after the text and on the same line.
     *
     * **A decoration of the line, not a line of its own.** Text and sample share one line spacing
     * and are centred together, which is what the user is promised when the two are called a line;
     * a render line of its own would stack beneath the text instead of standing beside it.
     */
    OCartoucheTraceSample trace;
};

/**
 * @brief One fully placed element of a block, ready to draw or to serialize.
 *
 * Produced by Layout() from the render lines: text lines carry the resolved
 * font and the drawText origin (penX, baseline); logo lines carry the target
 * draw rect plus the original SVG path for the writer.
 */
struct OCartoucheRenderItem
{
    enum class Kind
    {
        Text,
        Logo,
        Shape
    };

    Kind kind = Kind::Text;
    QString text;       // Kind::Text — the line text.
    QFont font;         // Kind::Text — fully styled font.
    QPointF pen;        // Kind::Text — drawText origin (penX, baseline).
    QPixmap pixmap;     // Kind::Logo — pre-rendered pixmap (screen painter).
    QRectF target;      // Kind::Logo, Kind::Shape — target draw rect.
    QString sourcePath; // Kind::Logo — original SVG file path (SVG writer only).

    // Kind::Text — the trace sample, placed. Null when the line carries none, which is why the
    // three below need no separate "has one": a placed segment says so by existing.
    QLineF trace;
    QColor traceColor;          // its ink — the one thing on a line that is not the caller's pen.
    QVector<qreal> traceDashes; // px at the drawn scale (Paint divides by the width).
    qreal tracePenWidth = 1.0;  // px at the drawn scale.
};

/**
 * @brief Shared cartouche painting helpers.
 *
 * The line formula is the single source of truth for the editor preview and
 * the canvas blocks: line height = font height + 6px + line spacing option,
 * text centered horizontally in the block.
 */
namespace OCartouchePainter
{
constexpr qreal kMargin = 16.0;
constexpr qreal kLineGap = 6.0;

/**
 * @brief The space between a line's text and the trace sample that follows it, in print px.
 *
 * A gap, and it scales like one — it is not content the user typed but the breathing room between
 * two things, the same way the inter-line gap is.
 */
constexpr qreal kTraceGap = 6.0;

/**
 * @brief The width a shape's outline is drawn with, in pixels.
 *
 * **A screen pixel, not a print pixel**: the outline is a line rather than content, so it does not
 * scale with the page — see Paint. One print pixel at `PrintDPI` is a little over a quarter of a
 * millimetre, which is what the SVG writers put on paper; on screen the line stays one pixel wide
 * however far the page is reduced, because a line reduced below a pixel is a line nobody can
 * measure against.
 */
constexpr qreal kShapePenWidth = 1.0;

/**
 * @brief The font a template is set in when it does not say.
 *
 * **The application's own font, which is the one the drawing path uses.** An editor that read the
 * size off the widget it lives in would answer with the stylesheet's — a screen size, not the size
 * the page is printed at — and a section left at the default would preview at one size and print at
 * another. `SectionFont` falls back to this, the previews draw with it, and the size control shows
 * it, so the three cannot disagree about what "not set" means.
 */
QFont BaseFont();

/**
 * @brief Section font from the template options (fontFamily / fontWeight / fontSize).
 *
 * @param prefix what the three keys start with. Empty for a section's own typography; an
 *               information that sets more than one thing in type — a value at a corner and a
 *               caption in the middle — names each group by a prefix, and gets one font per group
 *               out of the same three keys. The keys are built here rather than spelled out by
 *               every resolver, so a group and its typography cannot come apart.
 */
QFont SectionFont(const QHash<QString, QVariant> &options, const QFont &baseFont,
                  const QString &prefix = {});

/** @brief Build a text render line. */
OCartoucheRenderLine TextLine(const QString &text, const QFont &font, int lineSpacing);

/**
 * @brief Build a text render line followed, on the same line, by a sample of a trace.
 *
 * The sample is placed after the text with @ref kTraceGap between them, and the two are centred
 * together: what the reader sees is one line, and it is laid out as one.
 */
OCartoucheRenderLine TextLineWithTrace(const QString &text, const QFont &font, int lineSpacing,
                                       const OCartoucheTraceSample &trace);

/**
 * @brief Build one render line per line of @p text, in order.
 *
 * A text written by the user may run over several lines, and **one render line is one line on the
 * page**: the painter measures a line by its font, so a text of three lines handed over as one
 * entry would be drawn three lines tall and measured one — the block above it would sit on it.
 * Splitting is therefore where the text becomes lines, not in the painter's own layout.
 */
QVector<OCartoucheRenderLine> TextLines(const QString &text, const QFont &font, int lineSpacing);

/** @brief Build a logo render line (pre-rendered pixmap + original SVG source). */
OCartoucheRenderLine LogoLine(const QPixmap &logo, qreal logoHeight, int lineSpacing,
                              const QString &sourcePath = {});

/**
 * @brief Build a shape render line: outlines to stroke, and the texts placed against them.
 *
 * The outlines and the label boxes are in the shape's own coordinates, and are drawn at their true
 * size — a square that says five centimetres measures five centimetres on the paper. What the line
 * measures is the whole of them, so a band gives it the room it needs and no more.
 */
OCartoucheRenderLine ShapeLine(const QVector<QRectF> &outlines,
                               const QVector<OCartoucheShapeLabel> &labels, int lineSpacing);

/** @brief Return the lines with every text pixel size multiplied by fontScale. */
QVector<OCartoucheRenderLine> ScaledLines(const QVector<OCartoucheRenderLine> &lines, qreal fontScale);

/**
 * @brief Vertical advance of a text line: font height + (6px + line spacing) × scale.
 *
 * The font is expected to already carry the scale (pre-scaled pixel size); the
 * scale factor scales the fixed gap and the line-spacing option so a resized
 * block stays fully proportional.
 */
qreal LineHeight(const QFont &font, int lineSpacing, qreal scale = 1.0);

/** @brief Vertical advance of a logo line (scales the logo height and the gap). */
qreal LogoLineHeight(qreal logoHeight, int lineSpacing, qreal scale = 1.0);

/** @brief Vertical advance of a shape line (scales the shape's height and the gap). */
qreal ShapeLineHeight(qreal shapeHeight, int lineSpacing, qreal scale = 1.0);

/** @brief Natural size of a stack of lines (width = widest line + margins). */
QSizeF NaturalSize(const QVector<OCartoucheRenderLine> &lines);

/**
 * @brief Lay the lines out inside rect, fully placed.
 *
 * Single source of the placement formula (margins, ink-centering, baselines,
 * inter-line gap) shared by Paint() (draws) and the SVG writer (serializes) —
 * the two outputs can never drift apart.
 */
QVector<OCartoucheRenderItem> Layout(const QRectF &rect, const QVector<OCartoucheRenderLine> &lines,
                                     qreal scale = 1.0);

/**
 * @brief Paint the lines stacked, each centered. Uses painter.pen() for text.
 *
 * Draws exactly the elements produced by Layout(); a scale factor resizes the
 * content proportionally (see Layout).
 */
void Paint(QPainter &painter, const QRectF &rect, const QVector<OCartoucheRenderLine> &lines, qreal scale = 1.0);

/**
 * @brief The pen a placed trace sample is drawn with.
 *
 * **One builder, because three things draw the same sample**: the screen painter, and the two SVG
 * writers, which must agree with it to the pixel or a page would preview one way and print another.
 * A dash pattern counts in multiples of the pen width, so the pixel lengths Layout placed are
 * divided by it here — the inverse of what the SVG writer multiplies back.
 */
QPen TracePen(const OCartoucheRenderItem &item);
}  // namespace OCartouchePainter

#endif // OCARTOUCHEPAINTER_H
