/************************************************************************
 **
 **  @file   DockWrapper.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/08/2026
 **
 **  @brief  Standard dock content wrapper — implementation.
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

#include "DockWrapper.h"

#include <QDockWidget>
#include <QGroupBox>
#include <QScrollArea>
#include <QVBoxLayout>

#include "vtheme.h"

void WrapDockContent(QDockWidget *dock, QWidget *content)
{
    // ── Root widget — 8px $DockBody gap between ODockTitleBar and QScrollArea ──
    auto *root = new QWidget(dock);
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 8, 0, 0);
    rootLayout->setSpacing(0);

    // ── QScrollArea — viewport is a plain QWidget, the dot-selector works on it ──
    auto *scroll = new QScrollArea(root);
    scroll->setWidgetResizable(true);

    // Apply the viewport background fix immediately at creation time,
    // without waiting for showEvent or theme change. The dot-selector
    // .QWidget { background } is the only mechanism that overrides the
    // native macOS white viewport (SUBTILITES_CODE.md §13).
    {
        const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
        const auto bg = dark ? QStringLiteral("#191919") : QStringLiteral("#e5e5e5");
        scroll->viewport()->setStyleSheet(QStringLiteral(".QWidget { background: %1; }").arg(bg));
    }

    // ── Scroll contents — 8px $DockTitle gap between viewport and QGroupBox ──
    auto *scrollContents = new QWidget(scroll);
    auto *scrollLayout = new QVBoxLayout(scrollContents);
    scrollLayout->setContentsMargins(0, 8, 0, 0);
    scrollLayout->setSpacing(0);

    // ── QGroupBox (no title) — wraps the actual content ──
    auto *groupBox = new QGroupBox(QString(), scrollContents);
    groupBox->setObjectName(QStringLiteral("dockContentGroupBox"));
    auto *groupLayout = new QVBoxLayout(groupBox);
    groupLayout->setContentsMargins(0, 0, 0, 0);
    groupLayout->addWidget(content);
    content->setVisible(true);

    scrollLayout->addWidget(groupBox);
    scroll->setWidget(scrollContents);
    rootLayout->addWidget(scroll);
    dock->setWidget(root);
}
