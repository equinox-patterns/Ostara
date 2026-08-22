/************************************************************************
 **
 **  @file   osavedmarginrowwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/08/2026
 **
 **  @brief  One saved custom seam-allowance row — implementation.
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

#include "osavedmarginrowwidget.h"

#include "../vwidgets/osamarginpalette.h"

#include "../vmisc/FactoryWidgets.h"
#include "../vtheme/vtheme.h"

#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>

//---------------------------------------------------------------------------------------------------------------------
OSavedMarginRowWidget::OSavedMarginRowWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_nameEdit = new QLineEdit();
    layout->addWidget(m_nameEdit, 1);

    m_valueEdit = new QLineEdit();
    m_valueEdit->setAlignment(Qt::AlignCenter);
    m_valueEdit->setFixedWidth(50);
    layout->addWidget(m_valueEdit);

    m_colorCombo = MakeComboBox(this);
    m_colorCombo->setIconSize(QSize(16, 16));
    for (const QColor &c : OSAMarginPalette::Colors())
    {
        QPixmap px(16, 16);
        px.fill(c);
        m_colorCombo->addItem(QIcon(px), QString());
    }
    m_colorCombo->setFixedWidth(60);
    layout->addWidget(m_colorCombo);

    m_deleteButton = new QPushButton();
    // An icon-only button must never become the dialog's default button (it would
    // take the :default style — same look as :hover in the theme).
    m_deleteButton->setAutoDefault(false);
    m_deleteButton->setFixedWidth(30);
    refreshDeleteIcon();
    layout->addWidget(m_deleteButton);

    // Reload the themed trash icon when the theme changes (Light/Dark toggle).
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &OSavedMarginRowWidget::refreshDeleteIcon);
}

//---------------------------------------------------------------------------------------------------------------------
void OSavedMarginRowWidget::refreshDeleteIcon()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_deleteButton->setIcon(QIcon(QStringLiteral(":/icons/%1/trash_%1.svg")
                                      .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"))));
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavedMarginRowWidget::nameEdit() const -> QLineEdit *
{
    return m_nameEdit;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavedMarginRowWidget::valueEdit() const -> QLineEdit *
{
    return m_valueEdit;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavedMarginRowWidget::colorCombo() const -> QComboBox *
{
    return m_colorCombo;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSavedMarginRowWidget::deleteButton() const -> QPushButton *
{
    return m_deleteButton;
}
