/************************************************************************
 **
 **  @file   preferencestoolbarpage.cpp
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

#include "preferencestoolbarpage.h"

#include "../../core/otoolbarcatalogue.h"
#include "vtheme.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
constexpr int kIconSize = 22;
constexpr int kButtonSize = 28;

auto ThemedIcon(const QString &name) -> QIcon
{
    const QString theme = VTheme::ColorSheme() == VColorSheme::Dark ? QStringLiteral("dark")
                                                                    : QStringLiteral("light");
    return QIcon(QStringLiteral(":/icons/%1/%2_%1.svg").arg(theme, name));
}

auto MakeToolButton(QWidget *parent, const QString &objectName) -> QToolButton *
{
    auto *button = new QToolButton(parent);
    button->setObjectName(objectName);
    button->setAutoRaise(true);
    button->setIconSize(QSize(kIconSize, kIconSize));
    button->setFixedSize(kButtonSize, kButtonSize);
    return button;
}

auto MakeBar(const QVector<QToolButton *> &buttons) -> QHBoxLayout *
{
    auto *bar = new QHBoxLayout;
    bar->setContentsMargins(0, 0, 0, 0);
    bar->setSpacing(2);
    for (QToolButton *button : buttons)
    {
        bar->addWidget(button);
    }
    bar->addStretch();
    return bar;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
PreferencesToolBarPage::PreferencesToolBarPage(QWidget *parent)
  : QWidget(parent)
{
    BuildUi();
    LoadLayout();
    ReloadLists();
    RetranslateUi();
}

//---------------------------------------------------------------------------------------------------------------------
PreferencesToolBarPage::~PreferencesToolBarPage() = default;

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::BuildUi()
{
    auto *layout = new QVBoxLayout(this);

    // ── the two lists side by side ───────────────────────────────────────
    m_groupBox = new QGroupBox(this);
    m_groupList = new QListWidget(m_groupBox);
    m_groupList->setIconSize(QSize(kIconSize, kIconSize));
    m_groupList->setMinimumWidth(170);
    m_groupList->setMaximumWidth(260);
    // A double click opens the editor of the name — that is the whole renaming gesture.
    m_groupList->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    connect(m_groupList, &QListWidget::currentRowChanged, this, [this]() { ReloadTools(); });
    connect(m_groupList, &QListWidget::itemChanged, this, &PreferencesToolBarPage::RenameGroup);

    auto *groupButtons = new QHBoxLayout;
    groupButtons->setContentsMargins(0, 0, 0, 0);
    groupButtons->setSpacing(2);
    m_addGroup = MakeToolButton(m_groupBox, QStringLiteral("toolBarAddGroupBtn"));
    m_upGroup = MakeToolButton(m_groupBox, QStringLiteral("toolBarUpGroupBtn"));
    m_downGroup = MakeToolButton(m_groupBox, QStringLiteral("toolBarDownGroupBtn"));
    m_removeGroup = MakeToolButton(m_groupBox, QStringLiteral("toolBarTrashGroupBtn"));
    groupButtons->addWidget(m_addGroup);
    groupButtons->addWidget(m_upGroup);
    groupButtons->addWidget(m_downGroup);
    groupButtons->addWidget(m_removeGroup);
    groupButtons->addStretch();

    auto *groupLayout = new QVBoxLayout(m_groupBox);
    groupLayout->addLayout(groupButtons);
    groupLayout->addWidget(m_groupList);

    connect(m_addGroup, &QToolButton::clicked, this, &PreferencesToolBarPage::AddGroup);
    connect(m_upGroup, &QToolButton::clicked, this, [this]() { MoveGroup(-1); });
    connect(m_downGroup, &QToolButton::clicked, this, [this]() { MoveGroup(1); });
    connect(m_removeGroup, &QToolButton::clicked, this, &PreferencesToolBarPage::RemoveGroup);

    m_toolBox = new QGroupBox(this);
    m_toolList = new QListWidget(m_toolBox);
    m_toolList->setIconSize(QSize(kIconSize, kIconSize));

    m_upTool = MakeToolButton(m_toolBox, QStringLiteral("toolBarUpToolBtn"));
    m_downTool = MakeToolButton(m_toolBox, QStringLiteral("toolBarDownToolBtn"));
    m_removeTool = MakeToolButton(m_toolBox, QStringLiteral("toolBarTrashToolBtn"));

    auto *toolLayout = new QVBoxLayout(m_toolBox);
    toolLayout->addLayout(MakeBar({m_upTool, m_downTool, m_removeTool}));
    toolLayout->addWidget(m_toolList);

    connect(m_toolList, &QListWidget::currentRowChanged, this, [this]() { UpdateButtons(); });
    connect(m_hiddenList, &QListWidget::currentRowChanged, this, [this]() { UpdateButtons(); });

    connect(m_upTool, &QToolButton::clicked, this, [this]() { MoveTool(-1); });
    connect(m_downTool, &QToolButton::clicked, this, [this]() { MoveTool(1); });
    connect(m_removeTool, &QToolButton::clicked, this, &PreferencesToolBarPage::RemoveTool);

    auto *top = new QHBoxLayout;
    top->addWidget(m_groupBox);
    top->addWidget(m_toolBox, 1);
    layout->addLayout(top, 1);

    // ── the tools that belong to no group ────────────────────────────────
    m_hiddenBox = new QGroupBox(this);
    m_hiddenList = new QListWidget(m_hiddenBox);
    m_hiddenList->setIconSize(QSize(kIconSize, kIconSize));

    m_assignTool = MakeToolButton(m_hiddenBox, QStringLiteral("toolBarAssignToolBtn"));
    connect(m_assignTool, &QToolButton::clicked, this, &PreferencesToolBarPage::AssignTool);
    // The same gesture as the button: the tool goes into the group selected on the left.
    connect(m_hiddenList, &QListWidget::itemDoubleClicked, this, [this]() { AssignTool(); });

    auto *hiddenLayout = new QVBoxLayout(m_hiddenBox);
    hiddenLayout->addLayout(MakeBar({m_assignTool}));
    hiddenLayout->addWidget(m_hiddenList);

    layout->addWidget(m_hiddenBox, 1);

    auto *bottom = new QHBoxLayout;
    m_defaults = new QPushButton(this);
    connect(m_defaults, &QPushButton::clicked, this, &PreferencesToolBarPage::RestoreDefaults);
    bottom->addWidget(m_defaults);
    bottom->addStretch();
    layout->addLayout(bottom);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::LoadLayout()
{
    const OToolBarLayout::Layout layout = OToolBarLayout::Load();
    m_groups = layout.groups;
    ComputeHidden(layout);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::SetDefaultLayout()
{
    // In memory only: Apply() writes it. Restoring the default and then pressing Cancel must not
    // have touched the palette.
    const OToolBarLayout::Layout layout = OToolBarLayout::Default();
    m_groups = layout.groups;
    ComputeHidden(layout);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::ComputeHidden(const OToolBarLayout::Layout &layout)
{
    // Catalogue order: the list must not jump around as the user moves tools, and a tool taken out
    // of a group lands in the same place every time.
    const QStringList assigned = layout.Assigned();
    m_hidden.clear();
    for (const OToolBarCatalogue::Tool &tool : OToolBarCatalogue::DrawTools())
    {
        if (not assigned.contains(tool.id))
        {
            m_hidden << tool.id;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesToolBarPage::Apply() -> QStringList
{
    OToolBarLayout::Layout layout;
    layout.groups = m_groups;

    // Nothing to store when the arrangement is the shipped one: the key stays absent, and a later
    // version of Ostara that changes its defaults is still the user's default.
    if (layout.IsDefault())
    {
        OToolBarLayout::Reset();
    }
    else
    {
        OToolBarLayout::Save(layout);
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesToolBarPage::CurrentGroup() const -> int
{
    return m_groupList->currentRow();
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesToolBarPage::GroupTools(int group) -> QStringList &
{
    return m_groups[group].tools;
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::ReloadLists()
{
    const int previous = m_groupList->currentRow();

    m_reloading = true;
    m_groupList->clear();
    for (const OToolBarLayout::Group &group : m_groups)
    {
        auto *item = new QListWidgetItem(group.DisplayName(), m_groupList);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    }

    if (not m_groups.isEmpty())
    {
        m_groupList->setCurrentRow(qBound(0, previous, static_cast<int>(m_groups.size()) - 1));
    }
    m_reloading = false;

    ReloadTools();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::ReloadTools()
{
    m_reloading = true;
    m_toolList->clear();
    m_hiddenList->clear();

    const int group = CurrentGroup();

    if (group >= 0 && group < m_groups.size())
    {
        for (const QString &id : m_groups.at(group).tools)
        {
            auto *item = new QListWidgetItem(OToolBarCatalogue::IconFor(id), OToolBarCatalogue::ToolName(id),
                                             m_toolList);
            item->setData(Qt::UserRole, id);
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        }
        m_toolBox->setTitle(tr("Tools of « %1 »").arg(m_groups.at(group).DisplayName()));
    }
    else
    {
        m_toolBox->setTitle(tr("Tools"));
    }

    for (const QString &id : m_hidden)
    {
        auto *item = new QListWidgetItem(OToolBarCatalogue::IconFor(id), OToolBarCatalogue::ToolName(id),
                                         m_hiddenList);
        item->setData(Qt::UserRole, id);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    }

    m_reloading = false;

    UpdateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::AddGroup()
{
    OToolBarLayout::Group group;
    group.name = tr("New group");
    m_groups.append(group);

    ReloadLists();
    m_groupList->setCurrentRow(static_cast<int>(m_groups.size()) - 1);

    // The name is asked straight away: a group called "New group" teaches nothing.
    if (m_groupList->currentItem() != nullptr)
    {
        m_groupList->editItem(m_groupList->currentItem());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::MoveGroup(int delta)
{
    const int from = CurrentGroup();
    const int to = from + delta;
    if (from < 0 || to < 0 || to >= m_groups.size())
    {
        return;
    }

    std::swap(m_groups[from], m_groups[to]);

    ReloadLists();
    m_groupList->setCurrentRow(to);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::RemoveGroup()
{
    const int group = CurrentGroup();
    if (group < 0 || group >= m_groups.size())
    {
        return;
    }

    // Its tools are not destroyed with it: they go back to the list of the ones to dispatch.
    for (const QString &id : m_groups.at(group).tools)
    {
        if (not m_hidden.contains(id))
        {
            m_hidden << id;
        }
    }

    m_groups.removeAt(group);
    ReloadLists();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::RenameGroup(QListWidgetItem *item)
{
    if (m_reloading || item == nullptr)
    {
        return;
    }

    const int row = m_groupList->row(item);
    if (row < 0 || row >= m_groups.size())
    {
        return;
    }

    QString name = item->text().trimmed();
    if (name.isEmpty())
    {
        // An empty header on the palette would be a nameless group: keep the old name.
        m_reloading = true;
        item->setText(m_groups.at(row).DisplayName());
        m_reloading = false;
        return;
    }

    // The name is now the user's, not the one Ostara ships: it stops following the language.
    m_groups[row].key.clear();
    m_groups[row].name = name;

    ReloadTools();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::MoveTool(int delta)
{
    const int group = CurrentGroup();
    const int row = m_toolList->currentRow();
    if (group < 0 || group >= m_groups.size() || row < 0)
    {
        return;
    }

    QStringList &tools = GroupTools(group);
    const int to = row + delta;
    if (to < 0 || to >= tools.size())
    {
        return;
    }

    tools.swapItemsAt(row, to);

    ReloadTools();
    m_toolList->setCurrentRow(to);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::RemoveTool()
{
    const int group = CurrentGroup();
    const int row = m_toolList->currentRow();
    if (group < 0 || group >= m_groups.size() || row < 0)
    {
        return;
    }

    QStringList &tools = GroupTools(group);
    const QString id = tools.takeAt(row);
    if (not m_hidden.contains(id))
    {
        m_hidden << id;
    }

    ReloadTools();
    m_toolList->setCurrentRow(qMin(row, static_cast<int>(tools.size()) - 1));
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::AssignTool()
{
    const int group = CurrentGroup();
    const QListWidgetItem *item = m_hiddenList->currentItem();
    if (group < 0 || group >= m_groups.size() || item == nullptr)
    {
        return;
    }

    const QString id = item->data(Qt::UserRole).toString();
    if (id.isEmpty())
    {
        return;
    }

    m_hidden.removeAll(id);
    m_groups[group].tools << id;

    ReloadTools();
    m_toolList->setCurrentRow(static_cast<int>(m_groups.at(group).tools.size()) - 1);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::RestoreDefaults()
{
    SetDefaultLayout();
    ReloadLists();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::UpdateButtons()
{
    if (m_reloading)
    {
        return;
    }

    const int group = CurrentGroup();
    const bool hasGroup = group >= 0 && group < m_groups.size();

    m_upGroup->setEnabled(hasGroup && group > 0);
    m_downGroup->setEnabled(hasGroup && group < m_groups.size() - 1);
    m_removeGroup->setEnabled(hasGroup);

    const int tool = m_toolList->currentRow();
    const int toolCount = hasGroup ? static_cast<int>(m_groups.at(group).tools.size()) : 0;
    m_upTool->setEnabled(tool > 0);
    m_downTool->setEnabled(tool >= 0 && tool < toolCount - 1);
    m_removeTool->setEnabled(tool >= 0);

    m_assignTool->setEnabled(hasGroup && m_hiddenList->currentItem() != nullptr);

    OToolBarLayout::Layout layout;
    layout.groups = m_groups;
    m_defaults->setEnabled(not layout.IsDefault());
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::RetranslateUi()
{
    m_groupBox->setTitle(tr("Groups"));
    m_hiddenBox->setTitle(tr("Tools not shown"));
    m_defaults->setText(tr("Restore defaults"));

    m_addGroup->setToolTip(tr("Add a group"));
    m_upGroup->setToolTip(tr("Move the group up"));
    m_downGroup->setToolTip(tr("Move the group down"));
    m_removeGroup->setToolTip(tr("Delete the group"));
    m_upTool->setToolTip(tr("Move the tool up"));
    m_downTool->setToolTip(tr("Move the tool down"));
    m_removeTool->setToolTip(tr("Take the tool out of the group"));
    m_assignTool->setToolTip(tr("Put the tool into the selected group"));

    m_addGroup->setIcon(ThemedIcon(QStringLiteral("plus")));
    m_upGroup->setIcon(ThemedIcon(QStringLiteral("caret-up")));
    m_downGroup->setIcon(ThemedIcon(QStringLiteral("caret-down")));
    m_downTool->setIcon(ThemedIcon(QStringLiteral("caret-down")));
    m_upTool->setIcon(ThemedIcon(QStringLiteral("caret-up")));
    m_removeGroup->setIcon(ThemedIcon(QStringLiteral("trash")));
    m_removeTool->setIcon(ThemedIcon(QStringLiteral("trash")));
    m_assignTool->setIcon(ThemedIcon(QStringLiteral("plus")));

    ReloadTools();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesToolBarPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        RetranslateUi();
    }
    else if (event->type() == QEvent::PaletteChange)
    {
        // The icons have a light and a dark file: a theme switch has to swap them.
        RetranslateUi();
    }

    QWidget::changeEvent(event);
}
