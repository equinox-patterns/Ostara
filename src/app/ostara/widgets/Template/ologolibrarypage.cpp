/************************************************************************
 **
 **  @file   ologolibrarypage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The logo library page — one page, whichever editor opened it.
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

#include "ologolibrarypage.h"

#include "ologopreview.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace
{
/// The file name a row stands for — the library's identity, carried in the row rather than
/// looked up, because a row survives a refresh that the list has moved on from.
constexpr int kFileNameRole = Qt::UserRole;
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OLogoLibraryPage::OLogoLibraryPage(QWidget *parent) : QWidget(parent)
{
    SetupUI();
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::SetupUI()
{
    auto *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(8, 8, 8, 8);

    auto *left = new QWidget();
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto *listBar = new QHBoxLayout();
    m_addBtn = new QPushButton();
    connect(m_addBtn, &QPushButton::clicked, this, &OLogoLibraryPage::OnAdd);
    listBar->addWidget(m_addBtn);
    m_removeBtn = new QPushButton();
    connect(m_removeBtn, &QPushButton::clicked, this, &OLogoLibraryPage::OnRemove);
    listBar->addWidget(m_removeBtn);
    listBar->addStretch();
    leftLayout->addLayout(listBar);

    m_tree = new QTreeWidget();
    m_tree->setColumnCount(3);
    m_tree->setRootIsDecorated(false);
    m_tree->setUniformRowHeights(true);
    connect(m_tree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *) { OnSelectionChanged(current); });
    leftLayout->addWidget(m_tree, 1);

    m_editGroup = new QGroupBox();
    auto *form = new QFormLayout(m_editGroup);
    m_nameLabel = new QLabel();
    m_nameEdit = new QLineEdit();
    connect(m_nameEdit, &QLineEdit::textChanged, this, &OLogoLibraryPage::OnNameEdited);
    form->addRow(m_nameLabel, m_nameEdit);

    m_notesLabel = new QLabel();
    m_notesEdit = new QPlainTextEdit();
    m_notesEdit->setFixedHeight(70);
    connect(m_notesEdit, &QPlainTextEdit::textChanged, this, &OLogoLibraryPage::OnNotesChanged);
    form->addRow(m_notesLabel, m_notesEdit);
    leftLayout->addWidget(m_editGroup);

    m_preview = new OLogoPreview();
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    vbox->addWidget(splitter, 1);

    RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::RetranslateUI()
{
    m_addBtn->setText(tr("Add Logo"));
    m_removeBtn->setText(tr("Remove Logo"));
    m_tree->setHeaderLabels({tr("Name"), tr("Notes"), tr("Path")});
    m_editGroup->setTitle(tr("Logo"));
    m_nameLabel->setText(tr("Name:"));
    m_nameEdit->setPlaceholderText(tr("Logo name"));
    m_notesLabel->setText(tr("Notes:"));
    m_notesEdit->setPlaceholderText(tr("Notes"));
    RefreshList();
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::Show()
{
    // Read on every opening and never kept: the folder is the library, and a logo added from the
    // other editor since this page was last looked at belongs in the list.
    m_entries = OLogoLibrary::Load();
    RefreshList();
    m_tree->setCurrentItem(nullptr);
    OnSelectionChanged(nullptr);
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::RefreshList()
{
    m_tree->clear();

    if (m_entries.isEmpty())
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, tr("No logos yet. Add one."));
        item->setFlags(Qt::NoItemFlags);
        m_tree->addTopLevelItem(item);
        return;
    }

    for (const OLogoLibrary::Entry &entry : m_entries)
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, entry.name.isEmpty() ? QFileInfo(entry.fileName).completeBaseName() : entry.name);
        item->setText(1, entry.notes);
        item->setText(2, OLogoLibrary::Directory() + QLatin1Char('/') + entry.fileName);
        item->setData(0, kFileNameRole, entry.fileName);
        m_tree->addTopLevelItem(item);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::Select(const QString &fileName)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = m_tree->topLevelItem(i);
        if (item->data(0, kFileNameRole).toString() == fileName)
        {
            m_tree->setCurrentItem(item);
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::OnSelectionChanged(QTreeWidgetItem *current)
{
    m_updating = true;

    if (current == nullptr)
    {
        m_nameEdit->clear();
        m_notesEdit->clear();
        m_preview->SetSvgFile(QString());
    }
    else
    {
        const QString fileName = current->data(0, kFileNameRole).toString();
        for (const OLogoLibrary::Entry &entry : m_entries)
        {
            if (entry.fileName == fileName)
            {
                m_nameEdit->setText(entry.name);
                m_notesEdit->setPlainText(entry.notes);
                break;
            }
        }
        m_preview->SetSvgFile(OLogoLibrary::Directory() + QLatin1Char('/') + fileName);
    }

    m_updating = false;
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::OnNameEdited(const QString &text)
{
    if (m_updating)
    {
        return;
    }

    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == nullptr)
    {
        return;
    }

    const QString fileName = item->data(0, kFileNameRole).toString();
    if (fileName.isEmpty())
    {
        return;
    }

    item->setText(0, text.isEmpty() ? QFileInfo(fileName).completeBaseName() : text);
    PersistEntry(fileName, text, m_notesEdit->toPlainText());
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::OnNotesChanged()
{
    if (m_updating)
    {
        return;
    }

    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == nullptr)
    {
        return;
    }

    const QString fileName = item->data(0, kFileNameRole).toString();
    if (fileName.isEmpty())
    {
        return;
    }

    const QString notes = m_notesEdit->toPlainText();
    item->setText(1, notes);
    PersistEntry(fileName, m_nameEdit->text(), notes);
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::PersistEntry(const QString &fileName, const QString &name, const QString &notes)
{
    // The library is read again here rather than written from m_entries: this is one entry being
    // renamed, not the whole library being replaced, and a logo added since this page was filled
    // must survive the save.
    QList<OLogoLibrary::Entry> entries = OLogoLibrary::Load();
    for (OLogoLibrary::Entry &entry : entries)
    {
        if (entry.fileName == fileName)
        {
            entry.name = name;
            entry.notes = notes;
            break;
        }
    }

    QString err;
    if (!OLogoLibrary::Save(entries, &err))
    {
        QMessageBox::critical(this, tr("Logo"), err);
        return;
    }

    m_entries = entries;
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::OnAdd()
{
    const QString src =
        QFileDialog::getOpenFileName(this, tr("Add Logo"), QDir::homePath(), tr("SVG files (*.svg)"));
    if (src.isEmpty())
    {
        return;
    }

    QString err;
    if (!OLogoLibrary::Add(src, &err))
    {
        QMessageBox::critical(this, tr("Logo"), err);
        return;
    }

    const QString added = QFileInfo(src).fileName();
    Show();
    Select(added);
    emit StatusMessage(tr("Logo added"));
}

//---------------------------------------------------------------------------------------------------------------------

void OLogoLibraryPage::OnRemove()
{
    QTreeWidgetItem *item = m_tree->currentItem();
    if (item == nullptr)
    {
        return;
    }

    const QString fileName = item->data(0, kFileNameRole).toString();
    if (fileName.isEmpty())
    {
        return; // the empty-state hint, which is not a logo
    }

    const auto answer = QMessageBox::question(
        this, tr("Remove Logo"), tr("Remove the logo \"%1\" from the library?").arg(item->text(0)));
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    QString err;
    if (!OLogoLibrary::Remove(fileName, &err))
    {
        QMessageBox::critical(this, tr("Logo"), err);
        return;
    }

    Show();
    emit StatusMessage(tr("Logo removed"));
}
