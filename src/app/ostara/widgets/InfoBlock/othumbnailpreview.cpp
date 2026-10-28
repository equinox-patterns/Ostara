/************************************************************************
 **
 **  @file   othumbnailpreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/09/2026
 **
 **  @brief  What a miniature says, drawn without the piece — implementation.
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

#include "othumbnailpreview.h"

#include "core/othumbnailtextbuilder.h"

#include "../vtools/tools/othumbnailitem.h"
#include "vabstractvalapplication.h"
#include "vvalentinasettings.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QSizeF>
#include <QVector>

namespace
{
/// The margin the preview states: 15 mm. **A sample, and nothing else** — the block editor edits a
/// template, not a piece, so it holds no margin of its own to show. Fifteen millimetres is a length
/// that says something in both units at once (1.5 cm, 9/16 in), which is the case the page exists to
/// make visible: two units on is not the same sentence as one.
constexpr qreal kSampleMm = 15.0;

constexpr qreal kInset = 14.0; // Between the frame and the words.
constexpr qreal kGap = 8.0;    // Between the value and the legend.
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OThumbnailPreview::OThumbnailPreview(QWidget *parent) : QWidget(parent)
{
    // A block, not a page: the words are one short line and a caption, and the frame only has to be
    // big enough for the pair to be read at the size they will be printed.
    setMinimumHeight(130);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailPreview::SetLanguages(const QStringList &languages)
{
    m_languages = languages;
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailPreview::Refresh()
{
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);

    // The frame stands for the block a miniature occupies, and **nothing is drawn inside it**: the
    // piece belongs to the pattern, and what this page sets is what the block says, not what it holds.
    const QRectF frame = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(palette().mid().color(), 1.0));
    painter.setBrush(palette().base());
    painter.drawRect(frame);

    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    const bool centimeters = settings->IsThumbnailShowCentimeters();
    const bool inches = settings->IsThumbnailShowInches();

    struct Line
    {
        QString text;
        QFont font;
    };
    QVector<Line> lines;
    const QString value = OThumbnailTextBuilder::ValueText(kSampleMm, centimeters, inches);
    if (!value.isEmpty())
    {
        lines.append({value, OThumbnailItem::ValuesFont()});
    }
    const QString legend = OThumbnailTextBuilder::Legend(m_languages, centimeters, inches);
    if (!legend.isEmpty())
    {
        lines.append({legend, OThumbnailItem::LegendFont()});
    }

    // **Measured first, drawn second.** The two blocks are centred together, so the value cannot end
    // up wherever the legend's height happens to leave it — and a legend written in three languages
    // is three lines high, which moving the value must take into account.
    QVector<QSizeF> sizes;
    qreal total = 0.0;
    for (const Line &line : lines)
    {
        const QFontMetricsF metrics(line.font);
        const QStringList parts = line.text.split(QLatin1Char('\n'));
        qreal width = 0.0;
        for (const QString &part : parts)
        {
            width = qMax(width, metrics.horizontalAdvance(part));
        }
        const QSizeF size(width, metrics.height() * static_cast<qreal>(parts.size()));
        sizes.append(size);
        total += size.height();
    }
    if (lines.size() > 1)
    {
        total += kGap;
    }

    const QRectF content = frame.adjusted(kInset, kInset, -kInset, -kInset);
    if (content.width() < 1.0 || content.height() < 1.0)
    {
        return; // narrower than its own margins: there is no room for a word, and nothing to say
    }

    painter.setPen(palette().text().color());
    qreal y = content.center().y() - total / 2.0;
    for (int i = 0; i < lines.size(); ++i)
    {
        const Line &line = lines.at(i);
        const QFontMetricsF metrics(line.font);
        // The canvas anchors a word by the top-left of its BOX, and every line of that box is centred
        // in it — the box itself is centred here, so a legend in two languages comes out as one
        // centred caption rather than one line hanging left under the other. Asked of the item
        // rather than remeasured: the preview has to show what the canvas will draw.
        const QPointF topLeft(content.center().x() - sizes.at(i).width() / 2.0, y);
        const QVector<QPointF> origins = OThumbnailItem::CenteredLineOrigins(line.text, topLeft, line.font);
        const qreal ascent = metrics.ascent();
        const QStringList parts = line.text.split(QLatin1Char('\n'));
        painter.setFont(line.font);
        for (qsizetype j = 0; j < parts.size(); ++j)
        {
            if (!parts.at(j).isEmpty())
            {
                painter.drawText(origins.at(j) + QPointF(0.0, ascent), parts.at(j));
            }
        }
        y += sizes.at(i).height() + (i + 1 < lines.size() ? kGap : 0.0);
    }
}
