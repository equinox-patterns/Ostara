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
#include <QLineEdit>

//---------------------------------------------------------------------------------------------------------------------
OExportRollFormatWidget::OExportRollFormatWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QFormLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // Read-only rather than disabled: a disabled field greys its text out and reads as
    // something the user is being kept from, when it is in fact what the model imposed.
    m_widthEdit = new QLineEdit(this);
    m_widthEdit->setReadOnly(true);

    m_widthLabel = new QLabel(this);
    layout->addRow(m_widthLabel, m_widthEdit);

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
    m_widthEdit->setText(OExportUnitFormat::Format(m_widthMm, m_unit));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRollFormatWidget::Retranslate()
{
    m_widthLabel->setText(tr("Width:"));
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
