/************************************************************************
 **
 **  @file   otoolbarcatalogue.h
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef OTOOLBARCATALOGUE_H
#define OTOOLBARCATALOGUE_H

#include <QIcon>
#include <QString>
#include <QStringList>
#include <QVector>

/**
 * @brief Every tool the Draw-mode palette can show, and the groups it ships with.
 *
 * One table, read by two clients that must never disagree:
 *  - the palette itself, which draws the buttons;
 *  - the preferences page, which lets the user regroup them.
 *
 * The table used to live inside `DrawToolBar::InitializeGroups()` — names and order in the
 * literal, icons derived by convention in two separate places, and thirteen buttons built by
 * hand. A page that reads the palette's own table is the only way the two lists cannot drift.
 *
 * The translated strings stay in the `DrawToolBar` context on purpose: the French catalogue
 * already holds them there, and moving them would silently blank every tooltip until the next
 * `lupdate`.
 */
namespace OToolBarCatalogue
{
/// Mode bitmask, the same one the palette filters its groups by.
enum ModeMask : int
{
    ModeDraw = 1 << 0,   // Draw::Calculation
    ModeDetail = 1 << 1, // Draw::Modeling
    ModeGraded = 1 << 3, // Draw::Graded
    ModeExport = 1 << 4  // Draw::Export
};

struct Tool
{
    QString id;   // stable identifier, also the name of its icon under :/tools/
    QString name; // translated label
};

struct Group
{
    QString key; // shipped groups only; a group the user created has none
    QString name;
    QStringList tools;
    int modeMask{0};
};

/// Every tool of the palette, in the order the shipped groups present them.
auto Tools() -> QVector<Tool>;

/// Only the tools of the Draw mode — what the preferences page may regroup.
auto DrawTools() -> QVector<Tool>;

/// Translated label of a tool. Empty for an unknown id.
auto ToolName(const QString &id) -> QString;

/// Whether the button arms a tool (true) or opens a window (false).
auto IsCheckable(const QString &id) -> bool;

/// The icon of a tool in the current theme.
auto IconFor(const QString &id) -> QIcon;

/// The seven groups of the Draw mode, in their shipped order and content.
auto DrawGroups() -> QVector<Group>;

/// The tools Ostara leaves out of the shipped organisation. They are real tools, still listed by
/// `DrawTools()` — the preferences page offers them in the list of tools to dispatch — but a
/// `Default()` layout does not show them until the user puts them back. This is the "hidden by
/// absence" of the toolbar spec: nothing is stored for them, the default simply does not name them.
auto HiddenByDefault() -> QStringList;

/// The groups of the other modes. They are not user-configurable and the palette builds them
/// as they are.
auto OtherGroups() -> QVector<Group>;

/// Translated name of a shipped group, by key. Empty for an unknown key.
auto GroupName(const QString &key) -> QString;
} // namespace OToolBarCatalogue

#endif // OTOOLBARCATALOGUE_H
