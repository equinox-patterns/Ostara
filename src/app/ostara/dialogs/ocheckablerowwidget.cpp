/************************************************************************
 **
 **  @file   ocheckablerowwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/10/2026
 **
 **  @brief  Implementation of one line of a checkable list.
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

#include "ocheckablerowwidget.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>

//---------------------------------------------------------------------------------------------------------------------
OCheckableRowWidget::OCheckableRowWidget(const QString &name, QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_check = new QCheckBox(this);
    layout->addWidget(m_check);

    m_nameLabel = new QLabel(name, this);
    layout->addWidget(m_nameLabel, 1);

    m_noteLabel = new QLabel(this);
    // Greyed rather than coloured: the note is not a warning about anything the user did wrong, it
    // is the state of that name — the disabled palette is exactly that register.
    m_noteLabel->setEnabled(false);
    m_noteLabel->setVisible(false);
    layout->addWidget(m_noteLabel);
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckableRowWidget::Name() const -> QString
{
    return m_nameLabel->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckableRowWidget::CheckBox() const -> QCheckBox *
{
    return m_check;
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowWidget::SetChecked(bool checked)
{
    m_check->setChecked(checked);
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowWidget::SetUnavailable(const QString &reason)
{
    m_available = false;
    m_check->setChecked(false);
    m_noteLabel->setText(reason);
    m_noteLabel->setVisible(true);
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckableRowWidget::IsAvailable() const -> bool
{
    return m_available;
}
