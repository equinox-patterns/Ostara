/************************************************************************
 **
 **  @file   ologopreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/08/2026
 **
 **  @brief  Live preview of one logo from the logo library.
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

#include "ologopreview.h"

#include "core/ologorender.h"

#include <QEvent>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QSvgRenderer>

OLogoPreview::OLogoPreview(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(200, 150);
}

void OLogoPreview::SetLogoFile(const QString &filePath)
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

    // What is drawn, and how big it says it is: a drawing knows its own size, a photograph the
    // size of the picture in it. Everything below the fork is the same for both.
    QSvgRenderer renderer;
    QImage image;
    QSize base;
    const bool vector = OLogoRender::IsVector(m_filePath);
    if (vector)
    {
        renderer.load(m_filePath);
        if (!renderer.isValid())
        {
            painter.setPen(palette().placeholderText().color());
            painter.drawText(rect(), Qt::AlignCenter, tr("Not a readable image"));
            return;
        }
        base = renderer.defaultSize();
    }
    else
    {
        QImageReader reader(m_filePath);
        reader.setAutoTransform(true);
        image = reader.read();
        if (image.isNull())
        {
            painter.setPen(palette().placeholderText().color());
            painter.drawText(rect(), Qt::AlignCenter, tr("Not a readable image"));
            return;
        }
        base = image.size();
    }

    // Fit the logo into the widget keeping its aspect ratio, centered.
    QRectF target = rect();
    if (base.isValid() && base.width() > 0 && base.height() > 0)
    {
        const QSize fit = base.scaled(rect().size(), Qt::KeepAspectRatio);
        target = QRectF(QPointF((rect().width() - fit.width()) / 2.0,
                                (rect().height() - fit.height()) / 2.0),
                        QSizeF(fit));
    }
    if (vector)
    {
        renderer.render(&painter, target);
    }
    else
    {
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawImage(target, image);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OLogoPreview::changeEvent(QEvent *event)
{
    // ⚠️ **Le texte de cet aperçu est PEINT, pas posé** — le moteur de retraduction ne peut pas
    // l'atteindre. Sur `LanguageChange`, on redemande un repaint pour que `paintEvent` relise `tr()`.
    if (event->type() == QEvent::LanguageChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}
