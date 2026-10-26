/************************************************************************
 **
 **  @file   ofoldlinetextpreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  The fold line caption as it will be drawn.
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

#include "ofoldlinetextpreview.h"

#include "vfoldline.h"

#include <QFontMetrics>
#include <QFontMetricsF>
#include <QPainter>

//---------------------------------------------------------------------------------------------------------------------
OFoldLineTextPreview::OFoldLineTextPreview(QWidget *parent) : QWidget(parent)
{
    // A block, not a page: the caption is one short line, and the frame only has to be big enough to
    // hold it at the size it will be drawn.
    setMinimumHeight(110);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

//---------------------------------------------------------------------------------------------------------------------
void OFoldLineTextPreview::Refresh()
{
    // The frame grows with the caption: a 200 px font is twice the default height, and a fixed block
    // would crop the very thing the page is there to show. Two lines are allowed for, because the
    // caption wraps before it is cut.
    const QFont font = VFoldLine::DefaultLabelFont();
    setMinimumHeight(qMax(110, (QFontMetrics(font).height() * 2) + 48));
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OFoldLineTextPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);

    const QRectF frame = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(QPen(palette().mid().color(), 1.0));
    painter.setBrush(palette().base());
    painter.drawRect(frame);

    // The fold line says where the caption sits on the drawing: the text is written above it, so the
    // preview shows the rule and the words in the same order, at the same size.
    const qreal ruleY = frame.center().y();
    QPen rulePen(palette().text().color(), 1.0, Qt::DashLine);
    painter.setPen(rulePen);
    painter.drawLine(QPointF(frame.left() + 12.0, ruleY), QPointF(frame.right() - 12.0, ruleY));

    const QFont font = VFoldLine::DefaultLabelFont();
    painter.setFont(font);
    painter.setPen(palette().text().color());

    // The caption sits above the rule, centred on it, exactly where VFoldLine places it.
    const QRectF textArea = frame.adjusted(12.0, 8.0, -12.0, -(frame.height() - (ruleY - 8.0)));
    painter.drawText(textArea, Qt::AlignHCenter | Qt::AlignBottom | Qt::TextWordWrap,
                     VFoldLine::DefaultFoldLineLabel());
}
