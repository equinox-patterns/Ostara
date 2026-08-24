/************************************************************************
 **
 **  @file   OInfoBlockPreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Live preview of a cartouche template — one stacked line per section.
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

#include "OInfoBlockPreview.h"

#include "OPieceCartoucheCatalogue.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPaintEvent>

OInfoBlockPreview::OInfoBlockPreview(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(180);
}

void OInfoBlockPreview::SetTemplate(const OPieceCartoucheTemplate *tpl)
{
    m_template = tpl;
    update();
}

void OInfoBlockPreview::Refresh()
{
    update();
}

void OInfoBlockPreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), palette().base());

    if (m_template == nullptr || m_template->IsEmpty())
    {
        painter.setPen(palette().placeholderText().color());
        painter.drawText(rect(), Qt::AlignCenter, tr("No sections yet."));
        return;
    }

    const int margin = 16;
    qreal y = margin;
    for (int i = 0; i < m_template->SectionCount(); ++i)
    {
        const auto &type = OPieceCartoucheCatalogue::Section(m_template->SectionTypeId(i));
        if (!type.IsValid())
        {
            continue;
        }

        QFont font = painter.font();
        const auto &options = m_template->SectionAt(i).options;
        if (!options.value(QStringLiteral("useDefaults"), true).toBool())
        {
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
                font.setPointSizeF(size);
            }
        }

        painter.setFont(font);
        painter.setPen(palette().text().color());
        const QRectF lineRect(margin, y, width() - 2.0 * margin, QFontMetricsF(font).height());
        painter.drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, type.DisplayName());

        y += QFontMetricsF(font).height() + 6.0;
        if (y > height())
        {
            break;
        }
    }
}
