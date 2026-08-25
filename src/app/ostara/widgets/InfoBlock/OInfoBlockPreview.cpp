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
#include "OPieceCartoucheStorage.h"
#include "OPieceCartoucheTable.h"

#include "def.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPaintEvent>
#include <QSvgRenderer>

namespace
{
// Compose "name [separator] value", e.g. "Stature : 168" or "Tour de poitrine 89 cm".
QString ComposeLine(const QString &name, const QString &value, const QString &separator)
{
    if (value.isEmpty())
    {
        return name;
    }
    if (separator.isEmpty())
    {
        return name + QLatin1Char(' ') + value;
    }
    return name + QLatin1Char(' ') + separator + QLatin1Char(' ') + value;
}
}  // namespace

OInfoBlockPreview::OInfoBlockPreview(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(180);
}

void OInfoBlockPreview::SetTemplate(const OPieceCartoucheTemplate *tpl)
{
    m_template = tpl;
    update();
}

void OInfoBlockPreview::SetTable(const OPieceCartoucheTable *table)
{
    m_table = table;
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
    const QFont baseFont = font();  // widget's own font — reset per line, no style cascade
    qreal y = margin;
    for (int i = 0; i < m_template->SectionCount(); ++i)
    {
        const auto &type = OPieceCartoucheCatalogue::Section(m_template->SectionTypeId(i));
        if (!type.IsValid())
        {
            continue;
        }

        const auto &options = m_template->SectionAt(i).options;

        QString line = type.DisplayName();
        const QString separator = options.value(QStringLiteral("separator")).toString();
        if (type.Id() == QStringLiteral("axis"))
        {
            if (m_table == nullptr || !m_table->HasMultiSizeTable())
            {
                continue;  // dead data: no multi-size table
            }
            const QString axis = options.value(QStringLiteral("axis")).toString();
            if (axis.isEmpty() || !m_table->AxisNames().contains(axis))
            {
                continue;  // dead data: axis not in the current table
            }
            line = ComposeLine(axis, m_table->AnchorValue(axis), separator);
        }
        else if (type.Id() == QStringLiteral("measure"))
        {
            if (m_table == nullptr || !m_table->HasTable())
            {
                continue;  // dead data: no measurement table (.omt or .otu)
            }
            const QString storedFull = options.value(QStringLiteral("measureName")).toString();
            if (storedFull.isEmpty())
            {
                continue;
            }
            const QString name = m_table->MeasurementNameFromFullName(storedFull);
            if (name.isEmpty())
            {
                continue;  // dead data: measurement not in the current table
            }
            const QString value = OPieceCartoucheTable::FormatValue(m_table->MeasurementValue(name))
                                  + QLatin1Char(' ') + m_table->UnitsStr();
            line = ComposeLine(storedFull, value, separator);
        }
        else if (type.Id() == QStringLiteral("logo"))
        {
            const QString stored = options.value(QStringLiteral("logo")).toString();
            if (stored.isEmpty())
            {
                continue;  // dead data: no logo chosen
            }
            const QString fullPath =
                OPieceCartoucheStorage::LogosDirectory() + QLatin1Char('/') + stored;
            QSvgRenderer renderer(fullPath);
            if (!renderer.isValid())
            {
                continue;  // dead data: missing or invalid SVG
            }
            const double sizeCm = options.value(QStringLiteral("logoSize"), 2.0).toDouble();
            const double targetH = qMax(8.0, ToPixel(sizeCm, Unit::Cm));
            const QSize base = renderer.defaultSize();
            double drawW = targetH;
            const double drawH = targetH;
            if (base.isValid() && base.width() > 0 && base.height() > 0)
            {
                drawW = targetH * (static_cast<double>(base.width()) / base.height());
            }
            const QRectF logoRect(margin + (width() - 2.0 * margin - drawW) / 2.0, y, drawW, drawH);
            renderer.render(&painter, logoRect);
            y += drawH + 6.0;
            if (y > height())
            {
                break;
            }
            continue;
        }

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

        painter.setFont(font);
        painter.setPen(palette().text().color());
        const QRectF lineRect(margin, y, width() - 2.0 * margin, QFontMetricsF(font).height());
        painter.drawText(lineRect, Qt::AlignHCenter | Qt::AlignVCenter, line);

        y += QFontMetricsF(font).height() + 6.0;
        if (y > height())
        {
            break;
        }
    }
}
