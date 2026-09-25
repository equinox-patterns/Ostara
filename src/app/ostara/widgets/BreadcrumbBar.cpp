/************************************************************************
 **
 **  @file   BreadcrumbBar.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/08/2026
 **
 **  @brief  Breadcrumb bar shown above the canvas while navigating an attachment.
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

#include "BreadcrumbBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

//---------------------------------------------------------------------------------------------------------------------
BreadcrumbBar::BreadcrumbBar(QWidget *parent)
  : QWidget(parent),
    m_canvasButton(new QPushButton(tr("Canvas"), this)),
    m_groupButton(new QPushButton(this)),
    m_groupSeparator(new QLabel(QStringLiteral(">"), this)),
    m_objectSeparator(new QLabel(QStringLiteral(">"), this)),
    m_objectLabel(new QLabel(this))
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(6);
    layout->addWidget(m_canvasButton);
    layout->addWidget(m_groupSeparator);
    layout->addWidget(m_groupButton);
    layout->addWidget(m_objectSeparator);
    layout->addWidget(m_objectLabel);
    layout->addStretch();

    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_canvasButton->setFlat(true);
    m_canvasButton->setCursor(Qt::PointingHandCursor);
    m_groupButton->setFlat(true);
    m_groupButton->setCursor(Qt::PointingHandCursor);

    connect(m_canvasButton, &QPushButton::clicked, this, [this](bool) { emit canvasClicked(); });
    connect(m_groupButton, &QPushButton::clicked, this, [this](bool) { emit groupClicked(); });

    m_objectSeparator->setVisible(false);
    m_objectLabel->setVisible(false);

    hide();
}

//---------------------------------------------------------------------------------------------------------------------
BreadcrumbBar::~BreadcrumbBar() = default;

//---------------------------------------------------------------------------------------------------------------------
void BreadcrumbBar::RetranslateUI()
{
    m_canvasButton->setText(tr("Canvas"));

    // Le nom du groupe est un nom de patron, pas un mot traduit : seule la formule « Group %1 »
    // change de langue. Un groupe vide n'a rien à réécrire.
    if (not m_groupName.isEmpty())
    {
        m_groupButton->setText(tr("Group %1").arg(m_groupName));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void BreadcrumbBar::Update(const QString &groupName, const QString &objectName)
{
    m_groupName = groupName;
    m_groupButton->setText(tr("Group %1").arg(groupName));

    const bool hasObject = not objectName.isEmpty();
    m_objectSeparator->setVisible(hasObject);
    m_objectLabel->setVisible(hasObject);
    if (hasObject)
    {
        m_objectLabel->setText(objectName);
    }

    show();
    raise();
}

//---------------------------------------------------------------------------------------------------------------------
void BreadcrumbBar::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        RetranslateUI();
    }
    QWidget::changeEvent(event);
}
