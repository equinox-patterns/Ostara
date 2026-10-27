/************************************************************************
 **
 **  @file   ODockTitleBar.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/08/2026
 **
 **  @brief  Title bar for a docked panel: title label and the float/close buttons.
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

#include "ODockTitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QStyleOption>

ODockTitleBar::ODockTitleBar(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("dockTitleBar"));
    setAutoFillBackground(true);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 4, 0);
    layout->setSpacing(2);

    // Title label
    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setObjectName(QStringLiteral("dockTitleLabel"));

    // Collapse button (caret-down = expanded, caret-right = collapsed)
    m_collapseBtn = new QPushButton(this);
    m_collapseBtn->setObjectName(QStringLiteral("dockCollapseBtn"));
    m_collapseBtn->setFlat(true);
    m_collapseBtn->setCursor(Qt::ArrowCursor);

    // Detach button
    m_detachBtn = new QPushButton(this);
    m_detachBtn->setObjectName(QStringLiteral("dockDetachBtn"));
    m_detachBtn->setFlat(true);
    m_detachBtn->setCursor(Qt::ArrowCursor);

    // Close button
    m_closeBtn = new QPushButton(this);
    m_closeBtn->setObjectName(QStringLiteral("dockCloseBtn"));
    m_closeBtn->setFlat(true);
    m_closeBtn->setCursor(Qt::ArrowCursor);

    layout->addWidget(m_titleLabel);
    layout->addStretch();
    layout->addWidget(m_collapseBtn);
    layout->addWidget(m_detachBtn);
    layout->addWidget(m_closeBtn);

    connect(m_collapseBtn, &QPushButton::clicked, this, [this]() {
        setCollapsed(!m_collapsed);
        emit collapseToggled(m_collapsed);
    });
    connect(m_detachBtn, &QPushButton::clicked, this, &ODockTitleBar::detachRequested);
    connect(m_closeBtn, &QPushButton::clicked, this, &ODockTitleBar::closeRequested);

    updateCollapseIcon();
}

void ODockTitleBar::setCollapsed(bool collapsed)
{
    if (m_collapsed == collapsed)
    {
        return;
    }

    m_collapsed = collapsed;
    updateCollapseIcon();

    // Force QSS re-evaluation for dynamic property selector
    style()->unpolish(m_collapseBtn);
    style()->polish(m_collapseBtn);
}

void ODockTitleBar::updateCollapseIcon()
{
    m_collapseBtn->setProperty("collapsed", m_collapsed);
}

void ODockTitleBar::setFloating(bool floating)
{
    m_detachBtn->setDisabled(floating);
}

void ODockTitleBar::setTitle(const QString &title)
{
    m_titleLabel->setText(title);
}

void ODockTitleBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}
