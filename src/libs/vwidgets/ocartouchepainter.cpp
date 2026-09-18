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

#include <QFontMetricsF>

#include <algorithm>

namespace OCartouchePainter
{
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

QVector<OCartoucheRenderLine> ScaledLines(const QVector<OCartoucheRenderLine> &lines, qreal fontScale)
{
    QVector<OCartoucheRenderLine> scaled = lines;
    for (OCartoucheRenderLine &line : scaled)
    {
        if (line.kind == OCartoucheRenderLine::Kind::Text && line.font.pixelSize() > 0)
        {
            QFont font = line.font;
            font.setPixelSize(qRound(font.pixelSize() * fontScale));
            line.font = font;
        }
    }
    return scaled;
}

QFont SectionFont(const QHash<QString, QVariant> &options, const QFont &baseFont)
{
    QFont font = baseFont;
    const QString family = options.value(QStringLiteral("fontFamily")).toString();
    const int weight = options.value(QStringLiteral("fontWeight"), 0).toInt();
    const double size = options.value(QStringLiteral("fontSize"), 0.0).toDouble();
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

QSizeF NaturalSize(const QVector<OCartoucheRenderLine> &lines)
{
    // Simulate the Paint() layout exactly: top margin, one line height per
    // line, an inter-line gap (kLineGap + the line-spacing option) BETWEEN lines
    // only, then the bottom margin. Summing the raw advances would over-count
    // the trailing gap and drop the bottom padding — the block would be smaller
    // than its content and the clipped content would be cropped.
    qreal width = 0.0;
    qreal y = kMargin;
    for (int i = 0; i < lines.size(); ++i)
    {
        const OCartoucheRenderLine &line = lines.at(i);
        if (line.kind == OCartoucheRenderLine::Kind::Text)
        {
            // Ink bounds, not the pen advance: script/calligraphy fonts overhang
            // their advance, so the block must be sized to the glyph ink.
            width = (std::max)(width, QFontMetricsF(line.font).boundingRect(line.text).width());
            y += QFontMetricsF(line.font).height();
        }
        else
        {
            width = (std::max)(width, static_cast<qreal>(line.logo.width()));
            y += line.logoHeight;
        }
        y += (i == lines.size() - 1) ? kMargin : (kLineGap + line.lineSpacing);
    }
    return {width + 2.0 * kMargin, y};
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
        OCartoucheRenderItem item;
        if (line.kind == OCartoucheRenderLine::Kind::Text)
        {
            const QFontMetricsF fm(line.font);
            const QRectF ink = fm.boundingRect(line.text);
            // Center the glyph ink, not the pen advance: script/calligraphy
            // glyphs overhang their advance, so advance-based centering shifts
            // the line sideways. Vertical placement stays consistent (ascent +
            // descent centered in the line band).
            item.kind = OCartoucheRenderItem::Kind::Text;
            item.text = line.text;
            item.font = line.font;
            item.pen = QPointF(rect.left() + rect.width() / 2.0 - ink.left() - ink.width() / 2.0,
                               y + (fm.height() - fm.ascent() - fm.descent()) / 2.0 + fm.ascent());
            y += LineHeight(line.font, line.lineSpacing, scale);
        }
        else
        {
            item.kind = OCartoucheRenderItem::Kind::Logo;
            item.pixmap = line.logo;
            item.sourcePath = line.sourcePath;
            const qreal drawW = line.logo.width() * scale;
            const qreal drawH = line.logoHeight * scale;
            item.target = QRectF(rect.left() + (rect.width() - drawW) / 2.0, y, drawW, drawH);
            y += LogoLineHeight(line.logoHeight, line.lineSpacing, scale);
        }
        items.append(item);
    }
    return items;
}

void Paint(QPainter &painter, const QRectF &rect, const QVector<OCartoucheRenderLine> &lines, qreal scale)
{
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QVector<OCartoucheRenderItem> items = Layout(rect, lines, scale);
    for (const OCartoucheRenderItem &item : items)
    {
        if (item.kind == OCartoucheRenderItem::Kind::Text)
        {
            painter.setFont(item.font);
            painter.drawText(item.pen, item.text);
        }
        else
        {
            painter.drawPixmap(item.target, item.pixmap, QRectF(item.pixmap.rect()));
        }
    }
}
}  // namespace OCartouchePainter
