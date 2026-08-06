/************************************************************************
 **
 **  @file   DialogImportStructure.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/07/2026
 **
 **  @brief  Import structure dialog implementation.
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

#include "DialogImportStructure.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

DialogImportStructure::DialogImportStructure(const QList<LibraryFileEntry> &entries,
                                               QStringList favorites,
                                               QWidget *parent)
    : QDialog(parent)
    , m_favorites(std::move(favorites))
{
    setWindowTitle(tr("Import structure"));
    setMinimumWidth(400);
    setMinimumHeight(300);

    SetupUI();

    // Store entries
    for (const auto &entry : entries)
    {
        EntryItem ei;
        ei.entry = entry;
        m_entryItems.append(ei);
    }

    PopulateList();
}

void DialogImportStructure::SetupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    auto *label = new QLabel(tr("Choose a measurement table whose structure you want "
                                 "to import (names, descriptions, groups).\n"
                                 "Values and intervals will not be imported."));
    label->setWordWrap(true);
    mainLayout->addWidget(label);

    m_listWidget = new QListWidget();
    connect(m_listWidget, &QListWidget::currentItemChanged,
            this, &DialogImportStructure::OnSelectionChanged);
    connect(m_listWidget, &QListWidget::itemDoubleClicked,
            this, &DialogImportStructure::OnItemDoubleClicked);
    mainLayout->addWidget(m_listWidget, 1);

    // Favorite button
    m_favoriteBtn = new QPushButton(tr("⭐ Add/Remove from favorites"));
    m_favoriteBtn->setEnabled(false);
    connect(m_favoriteBtn, &QPushButton::clicked,
            this, &DialogImportStructure::OnToggleFavorite);
    mainLayout->addWidget(m_favoriteBtn);

    // Buttons
    auto *buttonBox = new QDialogButtonBox();
    m_importBtn = buttonBox->addButton(tr("Import"), QDialogButtonBox::AcceptRole);
    m_importBtn->setEnabled(false);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void DialogImportStructure::PopulateList()
{
    m_listWidget->clear();
    for (int i = 0; i < m_entryItems.size(); ++i)
    {
        m_entryItems[i].item = new QListWidgetItem(m_entryItems[i].entry.displayName);
        m_entryItems[i].item->setData(Qt::UserRole, i); // index into m_entryItems
        m_listWidget->addItem(m_entryItems[i].item);
    }
}

void DialogImportStructure::OnSelectionChanged()
{
    UpdateImportButton();
}

void DialogImportStructure::OnItemDoubleClicked(QListWidgetItem * /*item*/)
{
    if (m_importBtn->isEnabled())
    {
        accept();
    }
}

void DialogImportStructure::OnToggleFavorite()
{
    auto *current = m_listWidget->currentItem();
    if (!current)
    {
        return;
    }

    int idx = current->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_entryItems.size())
    {
        return;
    }

    const QString &path = m_entryItems[idx].entry.filePath;
    if (m_favorites.contains(path))
    {
        m_favorites.removeAll(path);
        m_entryItems[idx].entry.isFavorite = false;
        m_entryItems[idx].entry.displayName.remove(QStringLiteral("⭐ "));
    }
    else
    {
        m_favorites.append(path);
        m_entryItems[idx].entry.isFavorite = true;
        m_entryItems[idx].entry.displayName = QStringLiteral("⭐ ")
            + m_entryItems[idx].entry.displayName;
    }

    // Refresh the list widget text
    current->setText(m_entryItems[idx].entry.displayName);
}

void DialogImportStructure::UpdateImportButton()
{
    m_importBtn->setEnabled(m_listWidget->currentItem() != nullptr);
    m_favoriteBtn->setEnabled(m_listWidget->currentItem() != nullptr);
}

void DialogImportStructure::RefreshFavoriteStars()
{
    for (auto &ei : m_entryItems)
    {
        if (m_favorites.contains(ei.entry.filePath))
        {
            ei.entry.isFavorite = true;
            if (!ei.entry.displayName.startsWith(QStringLiteral("⭐ ")))
            {
                ei.entry.displayName = QStringLiteral("⭐ ") + ei.entry.displayName;
            }
        }
        if (ei.item)
        {
            ei.item->setText(ei.entry.displayName);
        }
    }
}

auto DialogImportStructure::selectedFilePath() const -> QString
{
    auto *current = m_listWidget->currentItem();
    if (!current)
    {
        return {};
    }

    int idx = current->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_entryItems.size())
    {
        return {};
    }

    return m_entryItems[idx].entry.filePath;
}

auto DialogImportStructure::favorites() const -> QStringList
{
    return m_favorites;
}
