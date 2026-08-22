/************************************************************************
 **
 **  @file   ocustommarginrowwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/08/2026
 **
 **  @brief  One custom seam-allowance margin row — implementation.
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

#include "ocustommarginrowwidget.h"

#include "../vmisc/FactoryWidgets.h"
#include "../vtheme/vtheme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>

OCustomMarginRowWidget::OCustomMarginRowWidget(const QVector<quint32> &nodeIds, const QVector<QColor> &colors,
                                               QWidget *parent)
  : QWidget(parent)
  , m_nodeIds(nodeIds)
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
    for (const QColor &c : colors)
    {
        QPixmap px(16, 16);
        px.fill(c);
        m_colorCombo->addItem(QIcon(px), QString());
    }
    m_colorCombo->setFixedWidth(60);
    layout->addWidget(m_colorCombo);

    m_showCheck = new QCheckBox();
    m_showCheck->setFixedSize(18, 18);
    auto *checkContainer = new QFrame();
    checkContainer->setObjectName(QStringLiteral("checkField"));
    checkContainer->setFixedSize(28, 28);
    auto *checkLayout = new QHBoxLayout(checkContainer);
    checkLayout->setContentsMargins(0, 0, 0, 0);
    checkLayout->setAlignment(Qt::AlignCenter);
    checkLayout->addWidget(m_showCheck);
    layout->addWidget(checkContainer);

    m_deleteButton = new QPushButton();
    m_deleteButton->setFixedWidth(30);
    refreshDeleteIcon();
    layout->addWidget(m_deleteButton);

    // Reload the themed trash icon when the theme changes (Light/Dark toggle).
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &OCustomMarginRowWidget::refreshDeleteIcon);
}

//---------------------------------------------------------------------------------------------------------------------
void OCustomMarginRowWidget::refreshDeleteIcon()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_deleteButton->setIcon(QIcon(QStringLiteral(":/icons/%1/trash_%1.svg")
                                      .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"))));
}

//---------------------------------------------------------------------------------------------------------------------
auto OCustomMarginRowWidget::nameEdit() const -> QLineEdit *
{
    return m_nameEdit;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCustomMarginRowWidget::valueEdit() const -> QLineEdit *
{
    return m_valueEdit;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCustomMarginRowWidget::colorCombo() const -> QComboBox *
{
    return m_colorCombo;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCustomMarginRowWidget::showCheck() const -> QCheckBox *
{
    return m_showCheck;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCustomMarginRowWidget::deleteButton() const -> QPushButton *
{
    return m_deleteButton;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCustomMarginRowWidget::nodeIds() const -> QVector<quint32>
{
    return m_nodeIds;
}
