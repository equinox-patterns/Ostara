/************************************************************************
 **
 **  @file   preferencesshortcutspage.cpp
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

#include "preferencesshortcutspage.h"
#include "../vmisc/dialogs/vshortcutdialog.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractshortcutmanager.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "ui_preferencesshortcutspage.h"

#include <QPushButton>
#include <QTableWidgetItem>

//---------------------------------------------------------------------------------------------------------------------
PreferencesShortcutsPage::PreferencesShortcutsPage(QWidget *parent)
  : QWidget(parent),
    ui(new Ui::PreferencesShortcutsPage)
{
    ui->setupUi(this);

    InitShortcuts();
    connect(ui->pushButtonRestoreDefaults, &QPushButton::clicked, this, [this]() { InitShortcuts(true); });
    connect(ui->shortcutsTable, &QTableWidget::cellDoubleClicked, this,
            &PreferencesShortcutsPage::ShortcutCellDoubleClicked);
}

//---------------------------------------------------------------------------------------------------------------------
PreferencesShortcutsPage::~PreferencesShortcutsPage()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto PreferencesShortcutsPage::Apply() -> QStringList
{
    if (VAbstractShortcutManager *manager = VAbstractValApplication::VApp()->GetShortcutManager())
    {
        const auto &shortcutsList = manager->GetShortcutsList();
        for (int i = 0; i < m_transientShortcuts.length(); i++)
        {
            const QStringList &current = m_transientShortcuts.value(i);
            const QStringList &defaults = shortcutsList.value(i).defaultShortcuts;

            // Only persist shortcuts that differ from code defaults.
            // When a default changes (e.g. Cmd+W → Cmd+D for DrawMode), the old
            // persisted value must not lock the user to the stale shortcut.
            if (current != defaults)
            {
                VAbstractValApplication::VApp()->ValentinaSettings()->SetActionShortcuts(
                    VAbstractShortcutManager::ShortcutActionToString(shortcutsList.value(i).type), current);
            }
            else
            {
                // Remove any stale persisted value so the code default applies.
                VAbstractValApplication::VApp()->ValentinaSettings()->RemoveActionShortcuts(
                    VAbstractShortcutManager::ShortcutActionToString(shortcutsList.value(i).type));
            }
        }

        manager->UpdateShortcuts();
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        // retranslate designer form (single inheritance approach)
        ui->retranslateUi(this);
        RetranslateShortcutsTable();
    }
    // remember to call base class implementation
    QWidget::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::ShortcutCellDoubleClicked(int row, int column)
{
    Q_UNUSED(column)
    auto *shortcutDialog = new VShortcutDialog(row, this);
    connect(shortcutDialog, &VShortcutDialog::ShortcutsListChanged, this,
            [this](int index, const QStringList &stringListShortcuts)
            {
                m_transientShortcuts.replace(index, stringListShortcuts);
                UpdateShortcutsTable();
            });
    shortcutDialog->open();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::InitShortcuts(bool defaults)
{
    VAbstractShortcutManager *manager = VAbstractValApplication::VApp()->GetShortcutManager();
    if (manager == nullptr)
    {
        return;
    }

    manager->UpdateShortcuts();
    m_transientShortcuts.clear();
    ui->shortcutsTable->clearContents();
    const auto &shortcutsList = manager->GetShortcutsList();
    ui->shortcutsTable->setRowCount(static_cast<int>(shortcutsList.length()));

    for (int i = 0; i < shortcutsList.length(); i++)
    {
        const VAbstractShortcutManager::VSShortcut &shortcut = shortcutsList.value(i);

        // Add shortcut to transient shortcut list
        if (defaults)
        {
            m_transientShortcuts.append(shortcut.defaultShortcuts);
        }
        else
        {
            m_transientShortcuts.append(shortcut.shortcuts);
        }

        // Add shortcut to table widget
        auto *nameItem = new QTableWidgetItem();
        nameItem->setText(VAbstractShortcutManager::ReadableName(shortcut.type));
        ui->shortcutsTable->setItem(i, 0, nameItem);

        auto *shortcutsItem = new QTableWidgetItem();
        shortcutsItem->setText(VAbstractShortcutManager::StringListToReadableString(m_transientShortcuts.value(i)));
        ui->shortcutsTable->setItem(i, 1, shortcutsItem);
    }
    UpdateShortcutsTable();
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::UpdateShortcutsTable()
{
    for (int i = 0; i < m_transientShortcuts.length(); i++)
    {
        const QStringList &shortcuts = m_transientShortcuts.value(i);
        ui->shortcutsTable->item(i, 1)->setText(VAbstractShortcutManager::StringListToReadableString(shortcuts));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void PreferencesShortcutsPage::RetranslateShortcutsTable()
{
    VAbstractShortcutManager  const*manager = VAbstractApplication::VApp()->GetShortcutManager();
    if (manager == nullptr)
    {
        return;
    }

    const auto &shortcutsList = manager->GetShortcutsList();
    for (int i = 0; i < shortcutsList.length(); i++)
    {
        const VAbstractShortcutManager::VSShortcut &shortcut = shortcutsList.value(i);

        if (QTableWidgetItem *it = ui->shortcutsTable->item(i, 0))
        {
            it->setText(VAbstractShortcutManager::ReadableName(shortcut.type));
        }
    }
}
