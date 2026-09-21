/************************************************************************
 **
 **  @file   oseamallowancepreview.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  Shows what one margin of the library says.
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

#include "oseamallowancepreview.h"

#include "core/oexportunitformat.h"

#include <QFont>
#include <QFontInfo>
#include <QPainter>
#include <QPaintEvent>

OSeamAllowancePreview::OSeamAllowancePreview(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(160);
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowancePreview::SetEntry(const QString &name, bool byDefault,
                                     OSeamAllowanceLibrary::Comparison comparison, qreal mm,
                                     const Unit &unit)
{
    m_hasEntry = true;
    m_name = name;
    m_byDefault = byDefault;
    m_comparison = comparison;
    m_mm = mm;
    m_unit = unit;
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowancePreview::Clear()
{
    m_hasEntry = false;
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowancePreview::RetranslateUI()
{
    // paintEvent re-resolves its words on every paint.
    update();
}

//---------------------------------------------------------------------------------------------------------------------

void OSeamAllowancePreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), palette().base());

    const QRect box = rect().adjusted(12, 12, -12, -12);

    if (!m_hasEntry)
    {
        painter.setPen(palette().placeholderText().color());
        painter.drawText(box, Qt::AlignCenter, tr("No margin selected."));
        return;
    }

    QFont nameFont = font();
    nameFont.setBold(true);
    // **Sized in pixels, and asked for the size the font really has.** The theme sets its fonts in
    // pixels, so `pointSizeF()` answers -1 here — and a font scaled from -1 is a font with a
    // negative point size, which Qt refuses and complains about on every paint. `QFontInfo` answers
    // the size actually in use, whichever way it was set, and never a negative one.
    const int namePx = QFontInfo(nameFont).pixelSize();
    if (namePx > 0)
    {
        nameFont.setPixelSize(qMax(1, qRound(namePx * 1.2)));
    }

    // The name as it will read on the page, then the rule that put it there. **The width is shown in
    // the unit the application is working in**, which is the one the field beside it is written in —
    // showing it in any other would make the two disagree on screen.
    painter.setPen(palette().text().color());
    painter.setFont(nameFont);
    const QRect nameBox(box.left(), box.top(), box.width(), QFontMetrics(nameFont).height());
    painter.drawText(nameBox, Qt::AlignLeft | Qt::AlignVCenter, m_name);

    QFont ruleFont = font();
    painter.setFont(ruleFont);
    const QString rule =
        m_byDefault
            ? tr("The application's default seam allowance")
            : (m_comparison == OSeamAllowanceLibrary::Comparison::AtLeast ? tr("from %1 on")
                                                                          : tr("exactly %1"))
                  .arg(OExportUnitFormat::Format(m_mm, m_unit) + QLatin1Char(' ')
                       + UnitsToStr(m_unit, true));

    const QRect ruleBox(nameBox.left(), nameBox.bottom() + 6, box.width(),
                        QFontMetrics(ruleFont).height());
    painter.setPen(palette().placeholderText().color());
    painter.drawText(ruleBox, Qt::AlignLeft | Qt::AlignVCenter, rule);
}
