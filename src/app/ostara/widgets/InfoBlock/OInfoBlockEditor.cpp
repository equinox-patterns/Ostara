/************************************************************************
 **
 **  @file   OInfoBlockEditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Information block editor — the template editing widget (3 zones).
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

#include "OInfoBlockEditor.h"

#include "OInfoBlockPreview.h"
#include "OInfoBlockSectionEditor.h"
#include "OInfoBlockTemplatesPage.h"
#include "OLogoPreview.h"
#include "OPieceCartoucheCatalogue.h"
#include "OPieceCartoucheStorage.h"

#include "def.h"
#include "FactoryWidgets.h"
#include "vmisc/vabstractvalapplication.h"
#include "vmisc/vcommonsettings.h"

#include <QAction>
#include <QComboBox>
#include <QDate>
#include <QDir>
#include <QFrame>
#include <QStandardItemModel>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStringList>
#include <QTreeWidget>
#include <QVBoxLayout>

OInfoBlockEditor::OInfoBlockEditor(QWidget *parent) : QWidget(parent)
{
    SetupUI();
}

void OInfoBlockEditor::SetupUI()
{
    OPieceCartoucheStorage::EnsureDefaultTemplate();

    auto *outer = new QHBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_menuList = new QListWidget();
    m_menuList->setFixedWidth(170);
    m_menuList->addItem(tr("Existing templates"));
    m_menuList->addItem(tr("Create new"));
    m_menuList->addItem(tr("Logo"));
    m_menuList->addItem(tr("Clients"));
    m_menuList->addItem(tr("Collections"));
    connect(m_menuList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        const int row = m_menuList->row(item);
        if (row == 0)
        {
            ShowTemplatesList();
        }
        else if (row == 1)
        {
            NewTemplate();
        }
        else if (row == 2)
        {
            ShowLogosPage();
        }
        else if (row == 3)
        {
            ShowClientsPage();
        }
        else if (row == 4)
        {
            ShowCollectionsPage();
        }
    });
    outer->addWidget(m_menuList);

    m_stack = new QStackedWidget();
    SetupEditorPage();        // index 0
    SetupTemplatesPage();     // index 1
    SetupLogosPage();         // index 2
    SetupClientsPage();       // index 3
    SetupCollectionsPage();   // index 4
    outer->addWidget(m_stack, 1);

    m_menuList->setCurrentRow(1);
    NewTemplate();
}

void OInfoBlockEditor::RetranslateUI()
{
    // The catalogue caches its translated section names once — drop it so the
    // rebuild below (menu, section list) resolves in the new language.
    OPieceCartoucheCatalogue::InvalidateCache();

    // Left menu — 5 fixed entries.
    if (m_menuList->count() > 0)
    {
        m_menuList->item(0)->setText(tr("Existing templates"));
    }
    if (m_menuList->count() > 1)
    {
        m_menuList->item(1)->setText(tr("Create new"));
    }
    if (m_menuList->count() > 2)
    {
        m_menuList->item(2)->setText(tr("Logo"));
    }
    if (m_menuList->count() > 3)
    {
        m_menuList->item(3)->setText(tr("Clients"));
    }
    if (m_menuList->count() > 4)
    {
        m_menuList->item(4)->setText(tr("Collections"));
    }

    // Editor page — top bar.
    m_nameEdit->setPlaceholderText(tr("Template name"));
    m_addBtn->setText(tr("+ Add information"));
    m_saveBtn->setText(tr("Save"));
    m_saveAsBtn->setText(tr("Save As…"));
    m_upBtn->setToolTip(tr("Move up"));
    m_downBtn->setToolTip(tr("Move down"));
    m_deleteBtn->setToolTip(tr("Remove information"));

    // Editor page — block display language row (rebuilt: trash tooltips and
    // the "Add language" label resolve in the new language).
    m_languageLabel->setText(tr("Block display language"));
    m_languageHint->setText(tr("You can choose several for a multilingual block"));
    RebuildLanguageRow();

    // "+ Add information" dropdown — the actions were built once from the
    // catalogue; rebuild them so the labels resolve in the new language.
    m_addMenu->clear();
    for (const OPieceCartoucheSection &section : OPieceCartoucheCatalogue::Sections())
    {
        QAction *action = m_addMenu->addAction(section.DisplayName());
        action->setToolTip(section.Description());
        connect(action, &QAction::triggered, this, [this, id = section.Id()]() { OnAddSection(id); });
    }

    // Templates page.
    m_templatesPage->RetranslateUI();

    // Logos page.
    m_logoEditGroup->setTitle(tr("Logo"));
    m_logoNameLabel->setText(tr("Name:"));
    m_logoNotesLabel->setText(tr("Notes:"));
    m_logoNameEdit->setPlaceholderText(tr("Logo name"));
    m_logoNotesEdit->setPlaceholderText(tr("Notes"));
    m_logosTree->setHeaderLabels({tr("Name"), tr("Notes"), tr("Path")});
    m_addLogoBtn->setText(tr("Add Logo"));
    m_removeLogoBtn->setText(tr("Remove Logo"));
    if (m_logoEntries.isEmpty() && m_logosTree->topLevelItemCount() > 0)
    {
        m_logosTree->topLevelItem(0)->setText(0, tr("No logos yet. Add one."));
    }

    // Clients page.
    m_clientEditGroup->setTitle(tr("Client"));
    m_clientNameLabel->setText(tr("Name:"));
    m_clientMailLabel->setText(tr("Email:"));
    m_clientPhoneLabel->setText(tr("Phone:"));
    m_clientInfoLabel->setText(tr("Information:"));
    m_clientNameEdit->setPlaceholderText(tr("Client name"));
    m_clientMailEdit->setPlaceholderText(tr("Email"));
    m_clientPhoneEdit->setPlaceholderText(tr("Phone"));
    m_clientInfoEdit->setPlaceholderText(tr("Information"));
    m_clientsTree->setHeaderLabels({tr("Name")});
    m_addClientBtn->setText(tr("Add Client"));
    m_removeClientBtn->setText(tr("Remove Client"));
    m_historyGroup->setTitle(tr("History"));
    m_historyPlaceholder->setText(tr("No associated patterns yet."));
    if (m_clientEntries.isEmpty() && m_clientsTree->topLevelItemCount() > 0)
    {
        m_clientsTree->topLevelItem(0)->setText(0, tr("No clients yet. Add one."));
    }

    // Collections page.
    m_collectionEditGroup->setTitle(tr("Collection"));
    m_collectionNameLabel->setText(tr("Name:"));
    m_collectionYearLabel->setText(tr("Year:"));
    m_collectionPeriodLabel->setText(tr("Period:"));
    m_collectionDescriptionLabel->setText(tr("Description:"));
    m_collectionNameEdit->setPlaceholderText(tr("Collection name"));
    m_collectionPeriodEdit->setPlaceholderText(tr("Period"));
    m_collectionDescriptionEdit->setPlaceholderText(tr("Description"));
    m_collectionsTree->setHeaderLabels({tr("Name"), tr("Year"), tr("Period")});
    m_addCollectionBtn->setText(tr("Add Collection"));
    m_removeCollectionBtn->setText(tr("Remove Collection"));
    m_collectionsSortLabel->setText(tr("Sort by year:"));
    m_collectionsSortCombo->setItemText(0, tr("Newest first"));
    m_collectionsSortCombo->setItemText(1, tr("Oldest first"));
    m_collectionsHistoryGroup->setTitle(tr("History"));
    m_collectionsHistoryPlaceholder->setText(tr("No associated patterns yet."));
    if (m_collectionEntries.isEmpty() && m_collectionsTree->topLevelItemCount() > 0)
    {
        m_collectionsTree->topLevelItem(0)->setText(0, tr("No collections yet. Add one."));
    }

    // Section names come from the catalogue — refresh the ordered list. Keep the
    // current row so the section editor below stays on the same section.
    const int currentRow = m_sectionList->currentRow();
    RefreshSectionList();
    if (currentRow >= 0)
    {
        m_sectionList->setCurrentRow(currentRow);  // re-selects → rebuilds the section editor
    }
    else
    {
        m_sectionEditor->RetranslateUI();  // rebuilds the "Select a section…" hint
    }

    // The preview is a child widget too — re-render it in the new language.
    m_preview->RetranslateUI();
}

void OInfoBlockEditor::SetupEditorPage()
{
    auto *editorPage = new QWidget();
    auto *vbox = new QVBoxLayout(editorPage);
    vbox->setContentsMargins(8, 8, 8, 8);

    // Top bar: template name + add information + order arrows + save
    auto *topBar = new QHBoxLayout();

    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText(tr("Template name"));
    connect(m_nameEdit, &QLineEdit::textEdited, this, [this](const QString &text) {
        m_template.SetName(text);
        SetModified(true);
    });
    topBar->addWidget(m_nameEdit, 1);

    m_addBtn = new QPushButton(tr("+ Add information"));
    m_addMenu = new QMenu(m_addBtn);
    for (const OPieceCartoucheSection &section : OPieceCartoucheCatalogue::Sections())
    {
        QAction *action = m_addMenu->addAction(section.DisplayName());
        action->setToolTip(section.Description());
        connect(action, &QAction::triggered, this, [this, id = section.Id()]() { OnAddSection(id); });
    }
    m_addBtn->setMenu(m_addMenu);

    m_upBtn = new QPushButton();
    m_upBtn->setObjectName(QStringLiteral("infoBlockUpBtn"));
    m_upBtn->setToolTip(tr("Move up"));
    connect(m_upBtn, &QPushButton::clicked, this, [this]() {
        const int row = m_sectionList->currentRow();
        if (row <= 0)
        {
            return;
        }
        m_template.MoveUp(row);
        RefreshSectionList();
        m_sectionList->setCurrentRow(row - 1);
        SetModified(true);
    });

    m_downBtn = new QPushButton();
    m_downBtn->setObjectName(QStringLiteral("infoBlockDownBtn"));
    m_downBtn->setToolTip(tr("Move down"));
    connect(m_downBtn, &QPushButton::clicked, this, [this]() {
        const int row = m_sectionList->currentRow();
        if (row < 0 || row >= m_template.SectionCount() - 1)
        {
            return;
        }
        m_template.MoveDown(row);
        RefreshSectionList();
        m_sectionList->setCurrentRow(row + 1);
        SetModified(true);
    });

    topBar->addStretch();

    m_saveBtn = new QPushButton(tr("Save"));
    connect(m_saveBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnSave);
    topBar->addWidget(m_saveBtn);

    m_saveAsBtn = new QPushButton(tr("Save As…"));
    connect(m_saveAsBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnSaveAs);
    topBar->addWidget(m_saveAsBtn);

    vbox->addLayout(topBar);

    SetupLanguageRow();
    vbox->addWidget(m_languagesContainer);

    // Ordered list (top-left) + data-driven edit zone (bottom-left) + preview (right)
    m_sectionList = new QListWidget();
    connect(m_sectionList, &QListWidget::currentRowChanged, this, [this](int row) { SelectSection(row); });

    m_sectionEditor = new OInfoBlockSectionEditor();
    connect(m_sectionEditor, &OInfoBlockSectionEditor::OptionsChanged, this, [this]() {
        SetModified(true);
        RefreshPreview();
    });

    m_preview = new OInfoBlockPreview();
    m_preview->SetTemplate(&m_template);
    m_preview->SetTable(&m_table);

    auto *left = new QWidget();
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    // Order bar just above the list: Add information + move up/down + delete
    auto *listBar = new QHBoxLayout();
    listBar->setSpacing(4);
    listBar->addWidget(m_addBtn);
    listBar->addWidget(m_upBtn);
    listBar->addWidget(m_downBtn);

    m_deleteBtn = new QPushButton();
    m_deleteBtn->setObjectName(QStringLiteral("infoBlockTrashBtn"));  // same theme icon as the language trash
    m_deleteBtn->setToolTip(tr("Remove information"));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, [this]() {
        const int row = m_sectionList->currentRow();
        if (row < 0)
        {
            return;
        }
        m_template.RemoveSection(row);
        RefreshSectionList();
        if (m_template.SectionCount() > 0)
        {
            const int newRow = row < m_template.SectionCount() ? row : m_template.SectionCount() - 1;
            m_sectionList->setCurrentRow(newRow);  // → SelectSection (enables delete)
        }
        else
        {
            SelectSection(-1);
        }
        SetModified(true);
    });
    listBar->addWidget(m_deleteBtn);

    listBar->addStretch();
    leftLayout->addLayout(listBar);

    leftLayout->addWidget(m_sectionList, 1);
    leftLayout->addWidget(m_sectionEditor, 0);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({400, 400});  // 50/50 at open — Qt keeps the ratio on window resize

    vbox->addWidget(splitter, 1);

    m_stack->addWidget(editorPage);
}

void OInfoBlockEditor::SetupLanguageRow()
{
    m_languageLabel = new QLabel(tr("Block display language"));
    m_languageHint = new QLabel(tr("You can choose several for a multilingual block"));
    m_languageHint->setObjectName(QStringLiteral("infoBlockLanguageHint"));

    auto *languageTop = new QHBoxLayout();
    languageTop->setContentsMargins(0, 0, 0, 0);
    languageTop->setSpacing(6);
    languageTop->addWidget(m_languageLabel);
    languageTop->addWidget(m_languageHint);
    languageTop->addStretch(1);

    m_languagesContainer = new QWidget();
    auto *containerLayout = new QVBoxLayout(m_languagesContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(4);
    containerLayout->addLayout(languageTop);

    m_languagesLayout = new QHBoxLayout();
    m_languagesLayout->setContentsMargins(0, 0, 0, 0);
    m_languagesLayout->setSpacing(4);
    containerLayout->addLayout(m_languagesLayout);

    // Hairline separator after the language row (1 px, 80% — WelcomeScreen pattern).
    auto *separatorBar = new QHBoxLayout();
    separatorBar->setContentsMargins(0, 0, 0, 0);
    separatorBar->addStretch(1);
    auto *separator = new QFrame();
    separator->setObjectName(QStringLiteral("infoBlockLanguageSeparator"));
    separator->setFixedHeight(1);
    separatorBar->addWidget(separator, 8);  // 80% = 8/(1+8+1)
    separatorBar->addStretch(1);
    containerLayout->addLayout(separatorBar);

    RebuildLanguageRow();
}

void OInfoBlockEditor::RebuildLanguageRow()
{
    const QStringList available = AvailableUiLanguages();
    const QStringList selected = m_template.Languages();

    m_updatingLanguages = true;

    // Drop the previous row widgets.
    m_languageCombos.clear();
    while (QLayoutItem *item = m_languagesLayout->takeAt(0))
    {
        if (QWidget *w = item->widget())
        {
            w->deleteLater();
        }
        delete item;
    }

    for (const QString &code : selected)
    {
        auto *combo = MakeComboBox(m_languagesContainer);
        AddLanguageItems(combo, available);

        // Connect before selecting so the guard silences the setup signal.
        connect(combo, &QComboBox::currentIndexChanged, this, &OInfoBlockEditor::OnLanguageChanged);

        const int idx = combo->findData(code);
        if (idx >= 0)
        {
            combo->setCurrentIndex(idx);
        }

        // A language already chosen in another combo is disabled here.
        auto *model = qobject_cast<QStandardItemModel *>(combo->model());
        if (model != nullptr)
        {
            for (int i = 0; i < combo->count(); ++i)
            {
                if (i == idx)
                {
                    continue;
                }
                if (selected.contains(combo->itemData(i).toString()))
                {
                    QStandardItem *item = model->item(i);
                    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
                }
            }
        }

        m_languageCombos.append(combo);
        m_languagesLayout->addWidget(combo);

        auto *trash = new QPushButton();
        trash->setObjectName(QStringLiteral("infoBlockTrashBtn"));
        trash->setToolTip(tr("Remove language"));
        trash->setEnabled(selected.size() > 1);
        connect(trash, &QPushButton::clicked, this, [this, code]() { OnRemoveLanguage(code); });
        m_languagesLayout->addWidget(trash);
    }

    m_addLanguageBtn = new QPushButton(tr("Add language"));
    m_addLanguageBtn->setEnabled(selected.size() < available.size());
    connect(m_addLanguageBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnAddLanguage);
    m_languagesLayout->addWidget(m_addLanguageBtn);

    m_updatingLanguages = false;
}

void OInfoBlockEditor::OnLanguageChanged()
{
    if (m_updatingLanguages)
    {
        return;
    }
    auto *combo = qobject_cast<QComboBox *>(sender());
    if (combo == nullptr)
    {
        return;
    }
    const int row = static_cast<int>(m_languageCombos.indexOf(combo));
    if (row < 0)
    {
        return;
    }
    QStringList langs = m_template.Languages();
    if (row >= langs.size())
    {
        return;
    }
    const QString code = combo->currentData().toString();
    if (langs.at(row) == code)
    {
        return;
    }
    langs[row] = code;
    m_template.SetLanguages(langs);
    SetModified(true);
    RebuildLanguageRow();
    RefreshPreview();
    // The section edit zone shows one tab per block language — refresh it.
    m_sectionEditor->SetSection(&m_template, m_sectionList->currentRow(), &m_table);
}

void OInfoBlockEditor::OnAddLanguage()
{
    const QStringList available = AvailableUiLanguages();
    QStringList langs = m_template.Languages();
    for (const QString &avail : available)
    {
        if (!langs.contains(avail))
        {
            langs.append(avail);
            m_template.SetLanguages(langs);
            SetModified(true);
            RebuildLanguageRow();
            RefreshPreview();
            m_sectionEditor->SetSection(&m_template, m_sectionList->currentRow(), &m_table);
            return;
        }
    }
}

void OInfoBlockEditor::OnRemoveLanguage(const QString &code)
{
    QStringList langs = m_template.Languages();
    if (langs.size() <= 1)
    {
        return;  // keep at least one display language
    }
    langs.removeAll(code);
    m_template.SetLanguages(langs);
    SetModified(true);
    RebuildLanguageRow();
    RefreshPreview();
    // The section edit zone shows one tab per block language — refresh it.
    m_sectionEditor->SetSection(&m_template, m_sectionList->currentRow(), &m_table);
}

void OInfoBlockEditor::SetupTemplatesPage()
{
    m_templatesPage = new OInfoBlockTemplatesPage();
    connect(m_templatesPage, &OInfoBlockTemplatesPage::OpenTemplate, this, &OInfoBlockEditor::LoadFile);
    m_stack->addWidget(m_templatesPage);
}

void OInfoBlockEditor::SetupLogosPage()
{
    auto *logosPage = new QWidget();
    auto *vbox = new QVBoxLayout(logosPage);
    vbox->setContentsMargins(8, 8, 8, 8);

    auto *left = new QWidget();
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto *listBar = new QHBoxLayout();
    m_addLogoBtn = new QPushButton(tr("Add Logo"));
    connect(m_addLogoBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnAddLogo);
    listBar->addWidget(m_addLogoBtn);
    m_removeLogoBtn = new QPushButton(tr("Remove Logo"));
    connect(m_removeLogoBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnRemoveLogo);
    listBar->addWidget(m_removeLogoBtn);
    listBar->addStretch();
    leftLayout->addLayout(listBar);

    m_logosTree = new QTreeWidget();
    m_logosTree->setColumnCount(3);
    m_logosTree->setHeaderLabels({tr("Name"), tr("Notes"), tr("Path")});
    m_logosTree->setRootIsDecorated(false);
    m_logosTree->setUniformRowHeights(true);
    connect(m_logosTree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
                Q_UNUSED(previous)
                OnLogoSelected(current);
            });
    leftLayout->addWidget(m_logosTree, 1);

    m_logoEditGroup = new QGroupBox(tr("Logo"));
    auto *form = new QFormLayout(m_logoEditGroup);
    m_logoNameEdit = new QLineEdit();
    m_logoNameEdit->setPlaceholderText(tr("Logo name"));
    connect(m_logoNameEdit, &QLineEdit::textChanged, this, &OInfoBlockEditor::OnLogoNameEdited);
    m_logoNameLabel = new QLabel(tr("Name:"));
    form->addRow(m_logoNameLabel, m_logoNameEdit);
    m_logoNotesEdit = new QPlainTextEdit();
    m_logoNotesEdit->setPlaceholderText(tr("Notes"));
    m_logoNotesEdit->setFixedHeight(70);
    connect(m_logoNotesEdit, &QPlainTextEdit::textChanged, this, &OInfoBlockEditor::OnLogoNotesChanged);
    m_logoNotesLabel = new QLabel(tr("Notes:"));
    form->addRow(m_logoNotesLabel, m_logoNotesEdit);
    leftLayout->addWidget(m_logoEditGroup);

    m_logoPreview = new OLogoPreview();
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(m_logoPreview);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({400, 400});  // 50/50 at open — ratio kept on window resize
    vbox->addWidget(splitter, 1);

    m_stack->addWidget(logosPage);
}

void OInfoBlockEditor::ShowLogosPage()
{
    m_logoEntries = OPieceCartoucheStorage::LoadLogos();
    RefreshLogosList();
    m_logosTree->setCurrentItem(nullptr);
    OnLogoSelected(nullptr);
    m_stack->setCurrentIndex(2);
}

void OInfoBlockEditor::RefreshLogosList()
{
    m_logosTree->clear();
    if (m_logoEntries.isEmpty())
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, tr("No logos yet. Add one."));
        item->setFlags(Qt::NoItemFlags);
        m_logosTree->addTopLevelItem(item);
        return;
    }
    for (const OPieceCartoucheStorage::LogoEntry &entry : m_logoEntries)
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, entry.name.isEmpty() ? QFileInfo(entry.fileName).completeBaseName() : entry.name);
        item->setText(1, entry.notes);
        item->setText(2, OPieceCartoucheStorage::LogosDirectory() + QLatin1Char('/') + entry.fileName);
        item->setData(0, Qt::UserRole, entry.fileName);
        m_logosTree->addTopLevelItem(item);
    }
}

void OInfoBlockEditor::OnLogoSelected(QTreeWidgetItem *current)
{
    m_updatingLogos = true;
    if (current == nullptr)
    {
        m_logoNameEdit->clear();
        m_logoNotesEdit->clear();
        m_logoPreview->SetSvgFile(QString());
    }
    else
    {
        const QString fileName = current->data(0, Qt::UserRole).toString();
        for (const OPieceCartoucheStorage::LogoEntry &entry : m_logoEntries)
        {
            if (entry.fileName == fileName)
            {
                m_logoNameEdit->setText(entry.name);
                m_logoNotesEdit->setPlainText(entry.notes);
                break;
            }
        }
        m_logoPreview->SetSvgFile(OPieceCartoucheStorage::LogosDirectory() + QLatin1Char('/') + fileName);
    }
    m_updatingLogos = false;
}

void OInfoBlockEditor::OnLogoNameEdited(const QString &text)
{
    if (m_updatingLogos)
    {
        return;
    }
    QTreeWidgetItem *item = m_logosTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString fileName = item->data(0, Qt::UserRole).toString();
    for (OPieceCartoucheStorage::LogoEntry &entry : m_logoEntries)
    {
        if (entry.fileName == fileName)
        {
            entry.name = text;
            item->setText(0, text.isEmpty() ? QFileInfo(fileName).completeBaseName() : text);
            break;
        }
    }
    PersistLogos();
}

void OInfoBlockEditor::OnLogoNotesChanged()
{
    if (m_updatingLogos)
    {
        return;
    }
    QTreeWidgetItem *item = m_logosTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString fileName = item->data(0, Qt::UserRole).toString();
    const QString notes = m_logoNotesEdit->toPlainText();
    for (OPieceCartoucheStorage::LogoEntry &entry : m_logoEntries)
    {
        if (entry.fileName == fileName)
        {
            entry.notes = notes;
            item->setText(1, notes);
            break;
        }
    }
    PersistLogos();
}

void OInfoBlockEditor::PersistLogos()
{
    QString err;
    if (!OPieceCartoucheStorage::SaveLogos(m_logoEntries, &err))
    {
        QMessageBox::critical(this, tr("Logo"), err);
    }
}

void OInfoBlockEditor::OnAddLogo()
{
    const QString src = QFileDialog::getOpenFileName(this, tr("Add Logo"), QDir::homePath(),
                                                     tr("SVG files (*.svg)"));
    if (src.isEmpty())
    {
        return;
    }

    QString err;
    if (!OPieceCartoucheStorage::AddLogo(src, &err))
    {
        QMessageBox::critical(this, tr("Logo"), err);
        return;
    }

    const QString newFile = QFileInfo(src).fileName();
    ShowLogosPage();
    // Select the freshly added logo.
    for (int i = 0; i < m_logosTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = m_logosTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == newFile)
        {
            m_logosTree->setCurrentItem(item);
            break;
        }
    }
    emit StatusMessage(tr("Logo added"));
}

void OInfoBlockEditor::OnRemoveLogo()
{
    QTreeWidgetItem *item = m_logosTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString fileName = item->data(0, Qt::UserRole).toString();
    if (fileName.isEmpty())
    {
        return;  // the empty-state hint item
    }

    const auto answer = QMessageBox::question(
        this, tr("Remove Logo"), tr("Remove the logo \"%1\" from the library?").arg(item->text(0)));
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    QString err;
    if (!OPieceCartoucheStorage::RemoveLogo(fileName, &err))
    {
        QMessageBox::critical(this, tr("Logo"), err);
        return;
    }
    ShowLogosPage();
    emit StatusMessage(tr("Logo removed"));
}

void OInfoBlockEditor::SetupClientsPage()
{
    auto *clientsPage = new QWidget();
    auto *vbox = new QVBoxLayout(clientsPage);
    vbox->setContentsMargins(8, 8, 8, 8);

    auto *left = new QWidget();
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto *listBar = new QHBoxLayout();
    m_addClientBtn = new QPushButton(tr("Add Client"));
    connect(m_addClientBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnAddClient);
    listBar->addWidget(m_addClientBtn);
    m_removeClientBtn = new QPushButton(tr("Remove Client"));
    connect(m_removeClientBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnRemoveClient);
    listBar->addWidget(m_removeClientBtn);
    listBar->addStretch();
    leftLayout->addLayout(listBar);

    m_clientsTree = new QTreeWidget();
    m_clientsTree->setColumnCount(1);
    m_clientsTree->setHeaderLabels({tr("Name")});
    m_clientsTree->setRootIsDecorated(false);
    m_clientsTree->setUniformRowHeights(true);
    connect(m_clientsTree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
                Q_UNUSED(previous)
                OnClientSelected(current);
            });
    leftLayout->addWidget(m_clientsTree, 1);

    m_clientEditGroup = new QGroupBox(tr("Client"));
    auto *form = new QFormLayout(m_clientEditGroup);
    m_clientNameEdit = new QLineEdit();
    m_clientNameEdit->setPlaceholderText(tr("Client name"));
    connect(m_clientNameEdit, &QLineEdit::textChanged, this, &OInfoBlockEditor::OnClientNameEdited);
    m_clientNameLabel = new QLabel(tr("Name:"));
    form->addRow(m_clientNameLabel, m_clientNameEdit);
    m_clientMailEdit = new QLineEdit();
    m_clientMailEdit->setPlaceholderText(tr("Email"));
    connect(m_clientMailEdit, &QLineEdit::textChanged, this, &OInfoBlockEditor::OnClientMailEdited);
    m_clientMailLabel = new QLabel(tr("Email:"));
    form->addRow(m_clientMailLabel, m_clientMailEdit);
    m_clientPhoneEdit = new QLineEdit();
    m_clientPhoneEdit->setPlaceholderText(tr("Phone"));
    connect(m_clientPhoneEdit, &QLineEdit::textChanged, this, &OInfoBlockEditor::OnClientPhoneEdited);
    m_clientPhoneLabel = new QLabel(tr("Phone:"));
    form->addRow(m_clientPhoneLabel, m_clientPhoneEdit);
    m_clientInfoEdit = new QPlainTextEdit();
    m_clientInfoEdit->setPlaceholderText(tr("Information"));
    m_clientInfoEdit->setFixedHeight(70);
    connect(m_clientInfoEdit, &QPlainTextEdit::textChanged, this, &OInfoBlockEditor::OnClientInfoChanged);
    m_clientInfoLabel = new QLabel(tr("Information:"));
    form->addRow(m_clientInfoLabel, m_clientInfoEdit);
    leftLayout->addWidget(m_clientEditGroup);

    m_historyGroup = new QGroupBox(tr("History"));
    auto *historyLayout = new QVBoxLayout(m_historyGroup);
    m_historyPlaceholder = new QLabel(tr("No associated patterns yet."));
    m_historyPlaceholder->setAlignment(Qt::AlignCenter);
    m_historyPlaceholder->setWordWrap(true);
    historyLayout->addWidget(m_historyPlaceholder, 1);
    // The pattern list appears here once the pattern library exists
    // (spec clients.md §3, Historique — for now always empty).

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(m_historyGroup);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({400, 400});  // 50/50 at open — ratio kept on window resize
    vbox->addWidget(splitter, 1);

    m_stack->addWidget(clientsPage);
}

void OInfoBlockEditor::ShowClientsPage()
{
    m_clientEntries = OPieceCartoucheStorage::LoadClients();
    RefreshClientsList();
    m_clientsTree->setCurrentItem(nullptr);
    OnClientSelected(nullptr);
    m_stack->setCurrentIndex(3);
}

void OInfoBlockEditor::RefreshClientsList()
{
    m_clientsTree->clear();
    if (m_clientEntries.isEmpty())
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, tr("No clients yet. Add one."));
        item->setFlags(Qt::NoItemFlags);
        m_clientsTree->addTopLevelItem(item);
        return;
    }
    for (const OPieceCartoucheStorage::ClientEntry &entry : m_clientEntries)
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, entry.name.isEmpty() ? tr("(unnamed)") : entry.name);
        item->setData(0, Qt::UserRole, entry.id);
        m_clientsTree->addTopLevelItem(item);
    }
}

void OInfoBlockEditor::OnClientSelected(QTreeWidgetItem *current)
{
    m_updatingClients = true;
    if (current == nullptr)
    {
        m_clientNameEdit->clear();
        m_clientMailEdit->clear();
        m_clientPhoneEdit->clear();
        m_clientInfoEdit->clear();
    }
    else
    {
        const QString id = current->data(0, Qt::UserRole).toString();
        for (const OPieceCartoucheStorage::ClientEntry &entry : m_clientEntries)
        {
            if (entry.id == id)
            {
                m_clientNameEdit->setText(entry.name);
                m_clientMailEdit->setText(entry.mail);
                m_clientPhoneEdit->setText(entry.phone);
                m_clientInfoEdit->setPlainText(entry.info);
                break;
            }
        }
    }
    m_updatingClients = false;
}

void OInfoBlockEditor::OnClientNameEdited(const QString &text)
{
    if (m_updatingClients)
    {
        return;
    }
    QTreeWidgetItem *item = m_clientsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    for (OPieceCartoucheStorage::ClientEntry &entry : m_clientEntries)
    {
        if (entry.id == id)
        {
            entry.name = text;
            item->setText(0, text.isEmpty() ? tr("(unnamed)") : text);
            break;
        }
    }
    PersistClients();
}

void OInfoBlockEditor::OnClientMailEdited(const QString &text)
{
    if (m_updatingClients)
    {
        return;
    }
    QTreeWidgetItem *item = m_clientsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    for (OPieceCartoucheStorage::ClientEntry &entry : m_clientEntries)
    {
        if (entry.id == id)
        {
            entry.mail = text;
            break;
        }
    }
    PersistClients();
}

void OInfoBlockEditor::OnClientPhoneEdited(const QString &text)
{
    if (m_updatingClients)
    {
        return;
    }
    QTreeWidgetItem *item = m_clientsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    for (OPieceCartoucheStorage::ClientEntry &entry : m_clientEntries)
    {
        if (entry.id == id)
        {
            entry.phone = text;
            break;
        }
    }
    PersistClients();
}

void OInfoBlockEditor::OnClientInfoChanged()
{
    if (m_updatingClients)
    {
        return;
    }
    QTreeWidgetItem *item = m_clientsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    const QString info = m_clientInfoEdit->toPlainText();
    for (OPieceCartoucheStorage::ClientEntry &entry : m_clientEntries)
    {
        if (entry.id == id)
        {
            entry.info = info;
            break;
        }
    }
    PersistClients();
}

void OInfoBlockEditor::PersistClients()
{
    QString err;
    if (!OPieceCartoucheStorage::SaveClients(m_clientEntries, &err))
    {
        QMessageBox::critical(this, tr("Client"), err);
    }
}

void OInfoBlockEditor::OnAddClient()
{
    QString err;
    const QString newId = OPieceCartoucheStorage::AddClient(&err);
    if (newId.isEmpty())
    {
        QMessageBox::critical(this, tr("Add Client"), err);
        return;
    }

    ShowClientsPage();
    // Select the freshly added client.
    for (int i = 0; i < m_clientsTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = m_clientsTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == newId)
        {
            m_clientsTree->setCurrentItem(item);
            m_clientNameEdit->setFocus();
            break;
        }
    }
    emit StatusMessage(tr("Client added"));
}

void OInfoBlockEditor::OnRemoveClient()
{
    QTreeWidgetItem *item = m_clientsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    if (id.isEmpty())
    {
        return;  // the empty-state hint item
    }

    const auto answer = QMessageBox::question(
        this, tr("Remove Client"), tr("Remove the client \"%1\" from the library?").arg(item->text(0)));
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    QString err;
    if (!OPieceCartoucheStorage::RemoveClient(id, &err))
    {
        QMessageBox::critical(this, tr("Client"), err);
        return;
    }
    ShowClientsPage();
    emit StatusMessage(tr("Client removed"));
}

void OInfoBlockEditor::SetupCollectionsPage()
{
    auto *collectionsPage = new QWidget();
    auto *vbox = new QVBoxLayout(collectionsPage);
    vbox->setContentsMargins(8, 8, 8, 8);

    auto *left = new QWidget();
    auto *leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(4);

    auto *listBar = new QHBoxLayout();
    m_addCollectionBtn = new QPushButton(tr("Add Collection"));
    connect(m_addCollectionBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnAddCollection);
    listBar->addWidget(m_addCollectionBtn);
    m_removeCollectionBtn = new QPushButton(tr("Remove Collection"));
    connect(m_removeCollectionBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnRemoveCollection);
    listBar->addWidget(m_removeCollectionBtn);
    listBar->addStretch();
    m_collectionsSortLabel = new QLabel(tr("Sort by year:"));
    listBar->addWidget(m_collectionsSortLabel);
    m_collectionsSortCombo = MakeComboBox();
    m_collectionsSortCombo->addItem(tr("Newest first"), false);
    m_collectionsSortCombo->addItem(tr("Oldest first"), true);
    connect(m_collectionsSortCombo, &QComboBox::currentIndexChanged, this, &OInfoBlockEditor::OnSortChanged);
    listBar->addWidget(m_collectionsSortCombo);
    leftLayout->addLayout(listBar);

    m_collectionsTree = new QTreeWidget();
    m_collectionsTree->setColumnCount(3);
    m_collectionsTree->setHeaderLabels({tr("Name"), tr("Year"), tr("Period")});
    m_collectionsTree->setRootIsDecorated(false);
    m_collectionsTree->setUniformRowHeights(true);
    connect(m_collectionsTree, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem *current, QTreeWidgetItem *previous) {
                Q_UNUSED(previous)
                OnCollectionSelected(current);
            });
    leftLayout->addWidget(m_collectionsTree, 1);

    m_collectionEditGroup = new QGroupBox(tr("Collection"));
    auto *form = new QFormLayout(m_collectionEditGroup);
    m_collectionNameEdit = new QLineEdit();
    m_collectionNameEdit->setPlaceholderText(tr("Collection name"));
    connect(m_collectionNameEdit, &QLineEdit::textChanged, this, &OInfoBlockEditor::OnCollectionNameEdited);
    m_collectionNameLabel = new QLabel(tr("Name:"));
    form->addRow(m_collectionNameLabel, m_collectionNameEdit);
    m_collectionYearSpin = new QSpinBox();
    m_collectionYearSpin->setRange(1900, 2100);
    connect(m_collectionYearSpin, &QSpinBox::valueChanged, this, &OInfoBlockEditor::OnCollectionYearEdited);
    m_collectionYearLabel = new QLabel(tr("Year:"));
    form->addRow(m_collectionYearLabel, m_collectionYearSpin);
    m_collectionPeriodEdit = new QLineEdit();
    m_collectionPeriodEdit->setPlaceholderText(tr("Period"));
    connect(m_collectionPeriodEdit, &QLineEdit::textChanged, this, &OInfoBlockEditor::OnCollectionPeriodEdited);
    m_collectionPeriodLabel = new QLabel(tr("Period:"));
    form->addRow(m_collectionPeriodLabel, m_collectionPeriodEdit);
    m_collectionDescriptionEdit = new QPlainTextEdit();
    m_collectionDescriptionEdit->setPlaceholderText(tr("Description"));
    m_collectionDescriptionEdit->setFixedHeight(70);
    connect(m_collectionDescriptionEdit, &QPlainTextEdit::textChanged, this,
            &OInfoBlockEditor::OnCollectionDescriptionChanged);
    m_collectionDescriptionLabel = new QLabel(tr("Description:"));
    form->addRow(m_collectionDescriptionLabel, m_collectionDescriptionEdit);
    leftLayout->addWidget(m_collectionEditGroup);

    m_collectionsHistoryGroup = new QGroupBox(tr("History"));
    auto *historyLayout = new QVBoxLayout(m_collectionsHistoryGroup);
    m_collectionsHistoryPlaceholder = new QLabel(tr("No associated patterns yet."));
    m_collectionsHistoryPlaceholder->setAlignment(Qt::AlignCenter);
    m_collectionsHistoryPlaceholder->setWordWrap(true);
    historyLayout->addWidget(m_collectionsHistoryPlaceholder, 1);
    // The pattern list appears here once the pattern library exists
    // (spec collections.md §3, Historique — for now always empty).

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(m_collectionsHistoryGroup);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({400, 400});  // 50/50 at open — ratio kept on window resize
    vbox->addWidget(splitter, 1);

    m_stack->addWidget(collectionsPage);
}

void OInfoBlockEditor::ShowCollectionsPage()
{
    const bool ascending = VAbstractValApplication::VApp()->Settings()->GetCollectionsSortAscending();
    m_collectionsSortCombo->setCurrentIndex(ascending ? 1 : 0);
    m_collectionEntries = OPieceCartoucheStorage::LoadCollections(ascending);
    RefreshCollectionsList();
    m_collectionsTree->setCurrentItem(nullptr);
    OnCollectionSelected(nullptr);
    m_stack->setCurrentIndex(4);
}

void OInfoBlockEditor::RefreshCollectionsList()
{
    m_collectionsTree->clear();
    if (m_collectionEntries.isEmpty())
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, tr("No collections yet. Add one."));
        item->setFlags(Qt::NoItemFlags);
        m_collectionsTree->addTopLevelItem(item);
        return;
    }
    for (const OPieceCartoucheStorage::CollectionEntry &entry : m_collectionEntries)
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, entry.name.isEmpty() ? tr("(unnamed)") : entry.name);
        item->setText(1, QString::number(entry.year));
        item->setText(2, entry.period);
        item->setData(0, Qt::UserRole, entry.id);
        m_collectionsTree->addTopLevelItem(item);
    }
}

void OInfoBlockEditor::OnCollectionSelected(QTreeWidgetItem *current)
{
    m_updatingCollections = true;
    if (current == nullptr)
    {
        m_collectionNameEdit->clear();
        m_collectionYearSpin->setValue(QDate::currentDate().year());
        m_collectionPeriodEdit->clear();
        m_collectionDescriptionEdit->clear();
    }
    else
    {
        const QString id = current->data(0, Qt::UserRole).toString();
        for (const OPieceCartoucheStorage::CollectionEntry &entry : m_collectionEntries)
        {
            if (entry.id == id)
            {
                m_collectionNameEdit->setText(entry.name);
                m_collectionYearSpin->setValue(entry.year);
                m_collectionPeriodEdit->setText(entry.period);
                m_collectionDescriptionEdit->setPlainText(entry.description);
                break;
            }
        }
    }
    m_updatingCollections = false;
}

void OInfoBlockEditor::OnCollectionNameEdited(const QString &text)
{
    if (m_updatingCollections)
    {
        return;
    }
    QTreeWidgetItem *item = m_collectionsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    for (OPieceCartoucheStorage::CollectionEntry &entry : m_collectionEntries)
    {
        if (entry.id == id)
        {
            entry.name = text;
            item->setText(0, text.isEmpty() ? tr("(unnamed)") : text);
            break;
        }
    }
    PersistCollections();
}

void OInfoBlockEditor::OnCollectionYearEdited(int year)
{
    if (m_updatingCollections)
    {
        return;
    }
    QTreeWidgetItem *item = m_collectionsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    for (OPieceCartoucheStorage::CollectionEntry &entry : m_collectionEntries)
    {
        if (entry.id == id)
        {
            entry.year = year;
            item->setText(1, QString::number(year));
            break;
        }
    }
    PersistCollections();
}

void OInfoBlockEditor::OnCollectionPeriodEdited(const QString &text)
{
    if (m_updatingCollections)
    {
        return;
    }
    QTreeWidgetItem *item = m_collectionsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    for (OPieceCartoucheStorage::CollectionEntry &entry : m_collectionEntries)
    {
        if (entry.id == id)
        {
            entry.period = text;
            item->setText(2, text);
            break;
        }
    }
    PersistCollections();
}

void OInfoBlockEditor::OnCollectionDescriptionChanged()
{
    if (m_updatingCollections)
    {
        return;
    }
    QTreeWidgetItem *item = m_collectionsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    const QString description = m_collectionDescriptionEdit->toPlainText();
    for (OPieceCartoucheStorage::CollectionEntry &entry : m_collectionEntries)
    {
        if (entry.id == id)
        {
            entry.description = description;
            break;
        }
    }
    PersistCollections();
}

void OInfoBlockEditor::OnSortChanged()
{
    const bool ascending = m_collectionsSortCombo->currentData().toBool();
    VAbstractValApplication::VApp()->Settings()->SetCollectionsSortAscending(ascending);
    m_collectionEntries = OPieceCartoucheStorage::LoadCollections(ascending);
    RefreshCollectionsList();
    m_collectionsTree->setCurrentItem(nullptr);
    OnCollectionSelected(nullptr);
}

void OInfoBlockEditor::PersistCollections()
{
    QString err;
    if (!OPieceCartoucheStorage::SaveCollections(m_collectionEntries, &err))
    {
        QMessageBox::critical(this, tr("Collection"), err);
    }
}

void OInfoBlockEditor::OnAddCollection()
{
    QString err;
    const QString newId = OPieceCartoucheStorage::AddCollection(&err);
    if (newId.isEmpty())
    {
        QMessageBox::critical(this, tr("Add Collection"), err);
        return;
    }

    ShowCollectionsPage();
    // Select the freshly added collection.
    for (int i = 0; i < m_collectionsTree->topLevelItemCount(); ++i)
    {
        QTreeWidgetItem *item = m_collectionsTree->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == newId)
        {
            m_collectionsTree->setCurrentItem(item);
            m_collectionNameEdit->setFocus();
            break;
        }
    }
    emit StatusMessage(tr("Collection added"));
}

void OInfoBlockEditor::OnRemoveCollection()
{
    QTreeWidgetItem *item = m_collectionsTree->currentItem();
    if (item == nullptr)
    {
        return;
    }
    const QString id = item->data(0, Qt::UserRole).toString();
    if (id.isEmpty())
    {
        return;  // the empty-state hint item
    }

    const auto answer = QMessageBox::question(
        this, tr("Remove Collection"),
        tr("Remove the collection \"%1\" from the library?").arg(item->text(0)));
    if (answer != QMessageBox::Yes)
    {
        return;
    }

    QString err;
    if (!OPieceCartoucheStorage::RemoveCollection(id, &err))
    {
        QMessageBox::critical(this, tr("Collection"), err);
        return;
    }
    ShowCollectionsPage();
    emit StatusMessage(tr("Collection removed"));
}

void OInfoBlockEditor::NewTemplate()
{
    m_template = OPieceCartoucheTemplate(tr("Template 1"));
    m_filePath.clear();
    m_stack->setCurrentIndex(0);
    m_nameEdit->setText(m_template.Name());
    RebuildLanguageRow();
    RefreshSectionList();
    SelectSection(-1);
    SetModified(false);
}

void OInfoBlockEditor::ShowTemplatesList()
{
    m_templatesPage->Refresh();
    m_stack->setCurrentIndex(1);
}

void OInfoBlockEditor::LoadFile(const QString &filePath)
{
    OPieceCartoucheTemplate tpl;
    QString err;
    if (!OPieceCartoucheStorage::Load(filePath, &tpl, &err))
    {
        QMessageBox::critical(this, tr("Information block"), err);
        return;
    }

    m_template = tpl;
    m_filePath = filePath;
    m_stack->setCurrentIndex(0);
    m_menuList->clearSelection();
    m_nameEdit->setText(m_template.Name());
    RebuildLanguageRow();
    RefreshSectionList();
    SelectSection(-1);
    SetModified(false);
}

void OInfoBlockEditor::OnAddSection(const QString &typeId)
{
    m_template.AddSection(typeId);
    RefreshSectionList();
    m_sectionList->setCurrentRow(m_template.SectionCount() - 1);
    SetModified(true);
}

void OInfoBlockEditor::RefreshSectionList()
{
    m_sectionList->clear();
    for (int i = 0; i < m_template.SectionCount(); ++i)
    {
        const QString typeId = m_template.SectionTypeId(i);
        const auto &type = OPieceCartoucheCatalogue::Section(typeId);
        m_sectionList->addItem(type.IsValid() ? type.DisplayName() : typeId);
    }
}

void OInfoBlockEditor::SelectSection(int index)
{
    m_table.Load();
    m_sectionEditor->SetSection(&m_template, index, &m_table);
    RefreshPreview();
    if (m_deleteBtn != nullptr)
    {
        m_deleteBtn->setEnabled(index >= 0);  // delete needs a selected information
    }
}

void OInfoBlockEditor::RefreshPreview()
{
    m_table.Load();  // no-op when the resolved .omt path has not changed
    m_preview->Refresh();
}

void OInfoBlockEditor::DoSave(const QString &filePath)
{
    QString err;
    if (!OPieceCartoucheStorage::Save(m_template, filePath, &err))
    {
        QMessageBox::critical(this, tr("Information block"), err);
        emit StatusMessage(tr("Save failed"));
        return;
    }
    m_filePath = filePath;
    SetModified(false);
    emit StatusMessage(tr("Template saved"));
}

void OInfoBlockEditor::OnSave()
{
    if (m_filePath.isEmpty())
    {
        OnSaveAs();
        return;
    }
    DoSave(m_filePath);
}

void OInfoBlockEditor::OnSaveAs()
{
    const QString dir = OPieceCartoucheStorage::DefaultDirectory();
    QDir().mkpath(dir);

    QString base = m_template.Name();
    if (base.isEmpty())
    {
        base = QStringLiteral("template");
    }

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Save cartouche template"), dir + QLatin1Char('/') + base + QStringLiteral(".oct"),
        tr("Cartouche template (*.oct)"));

    // macOS: the native Save As sheet hands the focus back to MainWindow, which
    // leaves the Information block window deactivated behind it (spec §19). The
    // NonModal fix covers plain Save, but the Save As sheet still does this —
    // bring the modeless window back to the front.
    if (QWidget *top = window())
    {
        top->raise();
        top->activateWindow();
    }

    if (path.isEmpty())
    {
        return;
    }

    QString finalPath = path;
    if (QFileInfo(finalPath).suffix().isEmpty())
    {
        finalPath += QStringLiteral(".oct");
    }
    DoSave(finalPath);
}

void OInfoBlockEditor::SetModified(bool modified)
{
    if (m_modified == modified)
    {
        return;
    }
    m_modified = modified;
    emit DocumentModified();
}

auto OInfoBlockEditor::IsModified() const -> bool
{
    return m_modified;
}
