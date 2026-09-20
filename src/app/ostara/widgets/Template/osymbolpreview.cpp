/************************************************************************
 **
 **  @file   osymbolpreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  One line of a legend — the mark at the size it prints at, and
 **          the words beside it.
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

#include "osymbolpreview.h"

#include "core/osymbolrender.h"

#include <QPainter>

namespace
{
/// The air kept on the left of the mark, so that it is read against the pane rather than its edge.
constexpr qreal kMargin = 16.0;

/// Between the mark and its words — the air the legend itself will keep between the two.
constexpr qreal kGap = 12.0;
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OSymbolPreview::OSymbolPreview(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(200, 150);
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolPreview::SetSymbol(const QVector<OSymbolShapes::Shape> &shapes, const QString &text)
{
    m_shapes = shapes;
    m_text = text;
    m_hasSymbol = !shapes.isEmpty();
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolPreview::Clear()
{
    m_hasSymbol = false;
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OSymbolPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), palette().base());

    if (!m_hasSymbol)
    {
        painter.setPen(palette().placeholderText().color());
        painter.drawText(rect(), Qt::AlignCenter, tr("No symbol selected"));
        return;
    }

    // **Each mark at its own size, never the pane's.** The rectangle handed to the renderer is
    // exactly the room the mark takes on paper, so what is drawn here is what will be printed — not
    // a magnified version of it that would flatter a mark too small to read.
    //
    // The marks follow one another on the line, each centred on the line's middle rather than stood
    // on a common baseline: a legend mixes marks of different heights — a grain line is twice a
    // notch — and what makes them one line is their middles, not their feet.
    painter.setRenderHint(QPainter::Antialiasing);

    qreal x = kMargin;
    for (const OSymbolShapes::Shape &shape : m_shapes)
    {
        const QSizeF natural = OSymbolRender::NaturalSize(shape);
        const QRectF mark(x, (height() - natural.height()) / 2.0, natural.width(), natural.height());
        OSymbolRender::Draw(painter, mark, shape, palette().text().color(), 1.0);
        x += natural.width() + kGap;
    }

    // The words come after the last mark, on the same middle line.
    painter.setPen(palette().text().color());
    const QRectF textRect(x, 0, width() - x - kMargin, height());
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_text);
}
