/************************************************************************
 **
 **  @file   otoolbarlayout.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 26/10/2026.
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

#include "otoolbarlayout.h"

#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "otoolbarcatalogue.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <algorithm>

namespace
{
constexpr int kLayoutVersion = 1;

/// Whether @p id is one of the tools the preferences page may show.
auto IsDrawTool(const QString &id) -> bool
{
    const auto tools = OToolBarCatalogue::DrawTools();
    return std::any_of(tools.cbegin(), tools.cend(), [&id](const OToolBarCatalogue::Tool &tool) { return tool.id == id; });
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarLayout::Group::DisplayName() const -> QString
{
    if (not key.isEmpty())
    {
        const QString shipped = OToolBarCatalogue::GroupName(key);
        if (not shipped.isEmpty())
        {
            return shipped;
        }
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarLayout::Layout::Assigned() const -> QStringList
{
    QStringList ids;
    for (const Group &group : groups)
    {
        ids += group.tools;
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarLayout::Layout::GroupIndex(const QString &toolId) const -> int
{
    for (int i = 0; i < groups.size(); ++i)
    {
        if (groups.at(i).tools.contains(toolId))
        {
            return i;
        }
    }
    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarLayout::Layout::IsDefault() const -> bool
{
    const Layout shipped = Default();
    if (groups.size() != shipped.groups.size())
    {
        return false;
    }

    for (int i = 0; i < groups.size(); ++i)
    {
        const Group &mine = groups.at(i);
        const Group &theirs = shipped.groups.at(i);
        if (mine.key != theirs.key || mine.tools != theirs.tools || mine.DisplayName() != theirs.DisplayName())
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarLayout::Default() -> Layout
{
    // The tools Ostara hides by default are dropped from the groups here, and only here: the
    // catalogue keeps them in its table, which is what keeps them in the dispatch list of the
    // preferences page. A group whose whole content is hidden is dropped too — a header over
    // nothing would be a hole in the palette.
    const QStringList hidden = OToolBarCatalogue::HiddenByDefault();

    Layout layout;
    for (const OToolBarCatalogue::Group &group : OToolBarCatalogue::DrawGroups())
    {
        QStringList tools = group.tools;
        for (const QString &id : hidden)
        {
            tools.removeAll(id);
        }
        if (tools.isEmpty())
        {
            continue;
        }
        layout.groups.append(Group{group.key, group.name, tools});
    }
    return layout;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolBarLayout::Load() -> Layout
{
    const QString stored = VAbstractValApplication::VApp()->ValentinaSettings()->GetToolBarGroups();
    if (stored.trimmed().isEmpty())
    {
        return Default();
    }

    const QJsonDocument document = QJsonDocument::fromJson(stored.toUtf8());
    if (not document.isObject())
    {
        return Default();
    }

    const QJsonObject root = document.object();
    if (not root.contains(QStringLiteral("groups")) || not root.value(QStringLiteral("groups")).isArray())
    {
        return Default();
    }

    // An EMPTY array is a choice, not a broken value: the user may have deleted every group, and
    // the palette is then empty. The preferences page is always reachable to give it one back.
    const QJsonArray array = root.value(QStringLiteral("groups")).toArray();

    Layout layout;
    QStringList used;

    for (const QJsonValue &value : array)
    {
        const QJsonObject object = value.toObject();

        Group group;
        group.key = object.value(QStringLiteral("key")).toString();
        group.name = object.value(QStringLiteral("name")).toString().trimmed();

        if (not group.key.isEmpty() && OToolBarCatalogue::GroupName(group.key).isEmpty())
        {
            group.key.clear(); // a group of a version that no longer ships it
        }

        for (const QJsonValue &tool : object.value(QStringLiteral("tools")).toArray())
        {
            const QString id = tool.toString();
            // A tool cannot be in two groups, and a tool this version does not know is dropped
            // rather than drawn as an empty button.
            if (IsDrawTool(id) && not used.contains(id))
            {
                group.tools.append(id);
                used.append(id);
            }
        }

        if (group.key.isEmpty() && group.name.isEmpty())
        {
            group.name = QCoreApplication::translate("OToolBarLayout", "Group");
        }

        layout.groups.append(group);
    }

    return layout;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolBarLayout::Save(const Layout &layout)
{
    QJsonArray array;
    for (const Group &group : layout.groups)
    {
        QJsonArray tools;
        for (const QString &id : group.tools)
        {
            tools.append(id);
        }

        QJsonObject object;
        if (not group.key.isEmpty())
        {
            object.insert(QStringLiteral("key"), group.key);
        }
        object.insert(QStringLiteral("name"), group.DisplayName());
        object.insert(QStringLiteral("tools"), tools);
        array.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("version"), kLayoutVersion);
    root.insert(QStringLiteral("groups"), array);

    VAbstractValApplication::VApp()->ValentinaSettings()->SetToolBarGroups(
        QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact)));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolBarLayout::Reset()
{
    VAbstractValApplication::VApp()->ValentinaSettings()->SetToolBarGroups(QString());
}
