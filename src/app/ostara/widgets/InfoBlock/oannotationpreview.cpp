/************************************************************************
 **
 **  @file   oannotationpreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Live visual preview of one annotation style (see oannotationpreview.h).
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

#include "oannotationpreview.h"

#include <QEvent>
#include <QPainter>

OAnnotationPreview::OAnnotationPreview(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(200, 120);
}

void OAnnotationPreview::SetStyle(const OPieceCartoucheStorage::AnnotationStyleEntry &style)
{
    m_style = style;
    update();
}

void OAnnotationPreview::SetText(const QString &text)
{
    m_text = text;
    update();
}

void OAnnotationPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);

    // Optional colored background (transparent = the preview's base).
    bool backgroundColorValid = false;
    if (!m_style.backgroundColor.isEmpty())
    {
        const QColor bg(m_style.backgroundColor);
        if (bg.isValid())
        {
            painter.fillRect(rect(), bg);
            backgroundColorValid = true;
        }
    }
    if (!backgroundColorValid)
    {
        painter.fillRect(rect(), palette().base());
    }

    if (m_text.isEmpty())
    {
        painter.setPen(palette().placeholderText().color());
        painter.drawText(rect(), Qt::AlignCenter, tr("No annotation selected"));
        return;
    }

    QFont font;
    if (!m_style.fontFamily.isEmpty())
    {
        font.setFamily(m_style.fontFamily);
    }
    if (m_style.fontSize > 0)
    {
        font.setPixelSize(m_style.fontSize);
    }
    if (!m_style.fontFace.isEmpty())
    {
        font.setStyleName(m_style.fontFace);
    }
    painter.setFont(font);

    // Text color: explicit from the style, black by default.
    QColor textColor(QStringLiteral("#000000"));
    if (!m_style.textColor.isEmpty())
    {
        const QColor c(m_style.textColor);
        if (c.isValid())
        {
            textColor = c;
        }
    }
    painter.setPen(textColor);
    painter.drawText(rect(), Qt::AlignCenter, m_text);
}

//---------------------------------------------------------------------------------------------------------------------
void OAnnotationPreview::changeEvent(QEvent *event)
{
    // ⚠️ **Le texte de cet aperçu est PEINT, pas posé.** Il n'est la propriété d'aucun widget, donc
    // le moteur de retraduction ne peut pas l'atteindre — un texte peint dans un `paintEvent` n'est
    // pas une chaîne mémorisable. Le seul geste qui ait un sens sur `LanguageChange` est de
    // redemander un repaint, pour que le `paintEvent` relise `tr()`.
    if (event->type() == QEvent::LanguageChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}
