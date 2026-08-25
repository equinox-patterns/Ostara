/************************************************************************
 **
 **  @file   OLogoPreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/08/2026
 **
 **  @brief  Live preview of one SVG logo from the logo library.
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

#include "OLogoPreview.h"

#include <QPainter>
#include <QSvgRenderer>

OLogoPreview::OLogoPreview(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(200, 150);
}

void OLogoPreview::SetSvgFile(const QString &filePath)
{
    m_filePath = filePath;
    update();
}

void OLogoPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), palette().base());

    if (m_filePath.isEmpty())
    {
        painter.setPen(palette().placeholderText().color());
        painter.drawText(rect(), Qt::AlignCenter, tr("No logo selected"));
        return;
    }

    QSvgRenderer renderer(m_filePath);
    if (!renderer.isValid())
    {
        painter.setPen(palette().placeholderText().color());
        painter.drawText(rect(), Qt::AlignCenter, tr("Invalid SVG"));
        return;
    }

    // Fit the SVG into the widget keeping its aspect ratio, centered.
    QRectF target = rect();
    const QSize base = renderer.defaultSize();
    if (base.isValid() && base.width() > 0 && base.height() > 0)
    {
        const QSize fit = base.scaled(rect().size(), Qt::KeepAspectRatio);
        target = QRectF(QPointF((rect().width() - fit.width()) / 2.0,
                                (rect().height() - fit.height()) / 2.0),
                        QSizeF(fit));
    }
    renderer.render(&painter, target);
}
