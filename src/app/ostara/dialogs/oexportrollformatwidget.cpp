/************************************************************************
 **
 **  @file   oexportrollformatwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the Roll shape.
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

#include "oexportrollformatwidget.h"

#include "../core/oexportunitformat.h"

#include <QEvent>
#include <QFormLayout>
#include <QLabel>

//---------------------------------------------------------------------------------------------------------------------
OExportRollFormatWidget::OExportRollFormatWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QFormLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Boxed rather than bare: it sits in a column of fields, and a line of plain text there would
    // read as a caption that lost its field. Not disabled either — a disabled field greys its text
    // and reads as something the user is being kept from, when it is what the model imposed.
    m_value = new QLabel(this);
    m_value->setObjectName(QStringLiteral("exportSheetSizeValue"));
    layout->addRow(m_value);

    Retranslate();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::SetWidthMm(qreal widthMm)
{
    m_widthMm = widthMm;
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRollFormatWidget::WidthMm() const -> qreal
{
    return m_widthMm;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::SetUnit(const Unit &unit)
{
    m_unit = unit;
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::Refresh()
{
    // The same line the tiled shape writes, with one length instead of two — the words and the
    // unit come from the same places, so the two shapes cannot drift apart in how they read.
    m_value->setText(tr("Width:") + QLatin1Char(' ') + OExportUnitFormat::Format(m_widthMm, m_unit)
                     + QLatin1Char(' ') + UnitsToStr(m_unit, true));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::Retranslate()
{
    // Rebuilt, not retitled: the line carries a translated word AND a figure formatted in the
    // unit of the moment, so there is no widget per word to relabel.
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
