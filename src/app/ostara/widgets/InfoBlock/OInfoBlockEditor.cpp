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
#include "widgets/Template/otemplateeditormenu.h"
#include "widgets/Template/otemplatelanguagerow.h"
#include "widgets/Template/otemplatesectioneditor.h"
#include "OInfoBlockTemplatesPage.h"
#include "OPieceCartoucheCatalogue.h"
#include "OPieceCartoucheStorage.h"
#include "oannotationstylespage.h"
#include "ofoldlinetextpage.h"
#include "othumbnailstylespage.h"

#include "core/ologolibrary.h"
#include "core/opatternlibrary.h"
#include "widgets/olibraryicondelegate.h"

#include "def.h"
#include "FactoryWidgets.h"
#include "vmisc/vabstractvalapplication.h"
#include "vmisc/vcommonsettings.h"

#include <QAction>
#include <QCheckBox>
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
#include <QDesktopServices>
#include <QHeaderView>
#include <QTableWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
// L'historique : les patrons de la bibliothèque qui nomment le client (ou la collection) choisi. Les mêmes
// colonnes que la fenêtre de la bibliothèque, moins les actions : ici on ne range pas un patron, on va le
// chercher sur le disque.
const int kHistoryNameColumn = 0;
const int kHistoryModifiedColumn = 1;
const int kHistoryCollectionColumn = 2;
const int kHistoryCustomerColumn = 3;
const int kHistoryFolderColumn = 4;
const int kHistoryColumnCount = 5;
const int kHistoryRowHeight = 30;
const int kHistoryIconColumnWidth = 44;
const int kHistoryMinimumColumnWidth = 56;
} // namespace

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

    // The menu and the pages it opens — the shared pair. What each row opens is decided here.
    m_menu = new OTemplateEditorMenu();
    connect(m_menu, &OTemplateEditorMenu::RowClicked, this, [this](int row) {
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
        else if (row == 5)
        {
            ShowAnnotationsPage();
        }
        else if (row == 6)
        {
            ShowThumbnailPage();
        }
        else if (row == 7)
        {
            ShowFoldLineTextPage();
        }

        // Last, and whatever happened. An action that ran has just moved the page this reads; one
        // that was cancelled by the unsaved-changes prompt has left it alone — and in that case the
        // click has already moved the highlight, so the menu would name a page never opened.
        ApplyMenuRow();
    });
    outer->addWidget(m_menu);

    SetupEditorPage();        // index 0
    SetupTemplatesPage();     // index 1
    SetupLogosPage();         // index 2
    SetupClientsPage();       // index 3
    SetupCollectionsPage();   // index 4
    SetupAnnotationsPage();   // index 5
    SetupThumbnailPage();     // index 6
    SetupFoldLineTextPage();  // index 7

    // Named here as well as on a language change: this editor is retranslated when the language
    // changes and not when it is shown, and rows without names are a column of nothing.
    NameMenuRows();

    m_menu->HighlightRow(1);
    NewTemplate();
}

void OInfoBlockEditor::NameMenuRows()
{
    // Eight fixed entries — and **the rows are not the pages**. The library is the first row and the
    // editor's own page the second, where the pages themselves are stacked the other way round:
    // which row opens which page is decided in SetupUI, and everything else here counts rows.
    m_menu->SetRowLabel(0, tr("Existing templates"));
    m_menu->SetRowLabel(1, tr("Create new"));
    m_menu->SetRowLabel(2, tr("Logo"));
    m_menu->SetRowLabel(3, tr("Clients"));
    m_menu->SetRowLabel(4, tr("Collections"));
    m_menu->SetRowLabel(5, tr("Annotations"));
    m_menu->SetRowLabel(6, tr("Thumbnail"));
    m_menu->SetRowLabel(7, tr("Foldline text"));
}

void OInfoBlockEditor::RetranslateUI()
{
    // The catalogue caches its translated section names once — drop it so the
    // rebuild below (menu, section list) resolves in the new language.
    OPieceCartoucheCatalogue::InvalidateCache();

    // Left menu — six fixed entries.
    NameMenuRows();

    // Editor page — top bar.
    m_nameEdit->setPlaceholderText(tr("Template name"));
    m_addBtn->setText(tr("+ Add information"));
    m_saveBtn->setText(tr("Save"));
    m_saveAsBtn->setText(tr("Save As…"));
    m_upBtn->setToolTip(tr("Move up"));
    m_downBtn->setToolTip(tr("Move down"));
    m_deleteBtn->setToolTip(tr("Remove information"));

    // Editor page — the display language row, worded for a block.
    m_languageRow->SetLabels(tr("Block display language"),
                             tr("You can choose several for a multilingual block"));
    m_languageRow->RetranslateUI();

    // "+ Add information" dropdown — the actions were built once from the
    // catalogue; rebuild them so the labels resolve in the new language.
    m_addMenu->clear();
    for (const OTemplateSection &section : OPieceCartoucheCatalogue::Sections())
    {
        QAction *action = m_addMenu->addAction(section.DisplayName());
        action->setToolTip(section.Description());
        connect(action, &QAction::triggered, this, [this, id = section.Id()]() { OnAddSection(id); });
    }

    // Templates page.
    m_templatesPage->RetranslateUI();

    // Logos page — its own widget, which words itself.
    m_logosPage->RetranslateUI();

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
    for (QTableWidget *history : {m_clientsHistory, m_collectionsHistory})
    {
        if (history == nullptr)
        {
            continue;
        }
        const QStringList headers{tr("Name"), tr("Modified"), tr("Collection"), tr("Client"), QString()};
        for (int column = 0; column < headers.size() && column < history->columnCount(); ++column)
        {
            if (QTableWidgetItem *header = history->horizontalHeaderItem(column); header != nullptr)
            {
                header->setText(headers.at(column));
            }
        }
    }
    m_clientFolderCheck->setText(tr("Create the folder in the pattern library"));
    m_clientFolderCheck->setToolTip(tr("Makes a folder of the client's name in the library of patterns."));
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
    m_collectionFolderCheck->setText(tr("Create the folder in the pattern library"));
    m_collectionFolderCheck->setToolTip(tr("Makes a folder of the collection's name in the library of patterns."));
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

    // And so is the Annotations page. It was the one child left out of this function, and being a
    // child it never receives QEvent::LanguageChange on its own either — so its labels stayed in
    // the previous language until the window was closed and reopened.
    if (m_annotationStylesPage != nullptr)
    {
        m_annotationStylesPage->RetranslateUI();
    }

    // Thumbnail page — its own widget, which words itself.
    if (m_thumbnailStylesPage != nullptr)
    {
        m_thumbnailStylesPage->RetranslateUI();
    }

    // Foldline text page — same rule.
    if (m_foldLineTextPage != nullptr)
    {
        m_foldLineTextPage->RetranslateUI();
    }

    // ⚠️ **Les deux arbres, pour la seule chaîne qu'ils portent eux-mêmes.** Le repli
    // `tr("(unnamed)")` d'une entrée sans nom est écrit dans `RefreshClientsList()` et
    // `RefreshCollectionsList()`, que rien ne rappelait ici : une entrée non nommée restait
    // « (unnamed) » dans une interface française. Les états vides, eux, sont déjà repris plus haut.
    // (`tr("Template 1")` n'est PAS concerné : c'est le **nom** donné à un nouveau modèle, une
    // donnée enregistrée — la retraduire renommerait un modèle existant au changement de langue.)
    RefreshClientsList();
    RefreshCollectionsList();
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
    for (const OTemplateSection &section : OPieceCartoucheCatalogue::Sections())
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
    vbox->addWidget(m_languageRow);

    // Ordered list (top-left) + data-driven edit zone (bottom-left) + preview (right)
    m_sectionList = new QListWidget();
    connect(m_sectionList, &QListWidget::currentRowChanged, this, [this](int row) { SelectSection(row); });

    m_sectionEditor = new OTemplateSectionEditor();
    connect(m_sectionEditor, &OTemplateSectionEditor::OptionsChanged, this, [this]() {
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

    m_menu->AddPage(editorPage);
}

void OInfoBlockEditor::SetupLanguageRow()
{
    // The shared row, held by both families. It keeps the languages it is given and reports every
    // change as one signal; what the template does with that is decided here.
    m_languageRow = new OTemplateLanguageRow();
    connect(m_languageRow, &OTemplateLanguageRow::LanguagesChanged, this,
            [this](const QStringList &codes) {
                m_template.SetLanguages(codes);
                SetModified(true);
                RefreshPreview();
                // A section that renames something per language shows one tab per language, so the
                // form follows the row.
                m_sectionEditor->SetSection(&m_template, m_sectionList->currentRow(), &m_table);
            });

    m_languageRow->SetLanguages(m_template.Languages());
}





void OInfoBlockEditor::SetupTemplatesPage()
{
    m_templatesPage = new OInfoBlockTemplatesPage();
    connect(m_templatesPage, &OInfoBlockTemplatesPage::OpenTemplate, this, &OInfoBlockEditor::LoadFile);
    m_menu->AddPage(m_templatesPage);
}

void OInfoBlockEditor::SetupLogosPage()
{
    // The library is one folder for the whole installation and both template families draw from
    // it, so the page that manages it is not this editor's. This window only opens it — see
    // OLogoLibraryPage, which is also what the cover page editor opens.
    m_logosPage = new OLogoLibraryPage();
    connect(m_logosPage, &OLogoLibraryPage::StatusMessage, this, &OInfoBlockEditor::StatusMessage);
    m_menu->AddPage(m_logosPage);  // index 2
}

void OInfoBlockEditor::ShowLogosPage()
{
    m_logosPage->Show();
    m_menu->ShowPage(2);
}

void OInfoBlockEditor::SetupAnnotationsPage()
{
    m_annotationStylesPage = new OAnnotationStylesPage();
    connect(m_annotationStylesPage, &OAnnotationStylesPage::StatusMessage, this, &OInfoBlockEditor::StatusMessage);
    m_menu->AddPage(m_annotationStylesPage);  // index 5
}

void OInfoBlockEditor::ShowAnnotationsPage()
{
    m_annotationStylesPage->Show();
    m_menu->ShowPage(5);
}

void OInfoBlockEditor::SetupThumbnailPage()
{
    m_thumbnailStylesPage = new OThumbnailStylesPage();
    m_menu->AddPage(m_thumbnailStylesPage);  // index 6
}

void OInfoBlockEditor::SetupFoldLineTextPage()
{
    m_foldLineTextPage = new OFoldLineTextPage();
    m_menu->AddPage(m_foldLineTextPage);  // index 7

    // The caption is drawn by the tools on the canvas, and they read the preferences when they build
    // it: without this relay, a font change would wait for the next unrelated redraw.
    connect(m_foldLineTextPage, &OFoldLineTextPage::StylesChanged, this,
            &OInfoBlockEditor::FoldLineTextChanged);
}

void OInfoBlockEditor::ShowThumbnailPage()
{
    // `Show()` reloads the controls from the preferences, so coming back to the page never shows a
    // stale answer, and the languages are the template's own.
    m_thumbnailStylesPage->SetLanguages(m_template.Languages());
    m_thumbnailStylesPage->Show();
    m_menu->ShowPage(6);
}

void OInfoBlockEditor::ShowFoldLineTextPage()
{
    // Reload on entry, like the Miniature page: the setting is the application's, and it may have
    // been changed from another window since the last visit.
    m_foldLineTextPage->Show();
    m_menu->ShowPage(7);
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

    // The bridge to the library of patterns: a client is a folder, and the gesture that makes it belongs beside
    // the client it is made for. Ticking the box does the thing at once — it asks, it does not record a setting.
    // The words are given here and not only in RetranslateUI, which this editor is never called through before
    // the window is opened: a row built without its text is a checkbox nobody can read.
    m_clientFolderCheck = new QCheckBox(tr("Create the folder in the pattern library"));
    m_clientFolderCheck->setToolTip(tr("Makes a folder of the client's name in the library of patterns."));
    connect(m_clientFolderCheck, &QCheckBox::toggled, this, &OInfoBlockEditor::OnCreateClientFolder);
    form->addRow(m_clientFolderCheck);

    leftLayout->addWidget(m_clientEditGroup);

    m_historyGroup = new QGroupBox(tr("History"));
    auto *historyLayout = new QVBoxLayout(m_historyGroup);

    m_clientsHistory = CreateHistoryTable();
    connect(m_clientsHistory, &QTableWidget::cellClicked, this,
            [this](int row, int column) { OnHistoryClicked(m_clientsHistory, row, column); });
    historyLayout->addWidget(m_clientsHistory, 1);

    m_historyPlaceholder = new QLabel(tr("No associated patterns yet."));
    m_historyPlaceholder->setAlignment(Qt::AlignCenter);
    m_historyPlaceholder->setWordWrap(true);
    historyLayout->addWidget(m_historyPlaceholder, 1);
    // Un client sans patron montre le texte ; un client qui en a montre la liste (FillHistory).

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(m_historyGroup);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    // Cinq colonnes à droite, quatre champs à gauche : l'historique a besoin de plus de place que la moitié,
    // sinon sa colonne de nom est la première sacrifiée.
    splitter->setSizes({360, 540});
    vbox->addWidget(splitter, 1);

    m_menu->AddPage(clientsPage);
}

void OInfoBlockEditor::ShowClientsPage()
{
    m_clientEntries = OPieceCartoucheStorage::LoadClients();
    RefreshClientsList();
    m_clientsTree->setCurrentItem(nullptr);
    OnClientSelected(nullptr);
    m_menu->ShowPage(3);
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

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief La table de l'historique : les quatre colonnes de la bibliothèque, et une seule action — le dossier.
 *
 * La colonne d'action est peinte par le délégué de la bibliothèque, pour que l'icône soit centrée et change de
 * variante sur la ligne choisie, comme dans la fenêtre.
 */
auto OInfoBlockEditor::CreateHistoryTable() -> QTableWidget *
{
    auto *table = new QTableWidget();
    table->setColumnCount(kHistoryColumnCount);
    table->setHorizontalHeaderLabels({tr("Name"), tr("Modified"), tr("Collection"), tr("Client"), QString()});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setShowGrid(false);
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(kHistoryRowHeight);
    table->horizontalHeader()->setSectionResizeMode(kHistoryNameColumn, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(kHistoryModifiedColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(kHistoryCollectionColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(kHistoryCustomerColumn, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(kHistoryFolderColumn, QHeaderView::Fixed);
    table->setColumnWidth(kHistoryFolderColumn, kHistoryIconColumnWidth);
    // Un plancher : la colonne du nom est celle qui cède la dernière place, et sans plancher les trois colonnes
    // dimensionnées sur leur contenu la réduisent à seize pixels — le nom du patron, illisible.
    table->horizontalHeader()->setMinimumSectionSize(kHistoryMinimumColumnWidth);
    table->setItemDelegateForColumn(kHistoryFolderColumn, new OLibraryIconDelegate(table));
    table->setVisible(false); // il n'y a rien à montrer tant que personne n'est choisi
    return table;
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockEditor::FillHistory(QTableWidget *table, QLabel *placeholder, const QString &name,
                                   const OPatternLibrary::Owner owner)
{
    if (table == nullptr)
    {
        return;
    }

    const QSignalBlocker blocker(table);
    table->setRowCount(0);

    // Le lien est un nom : la bibliothèque est relue à chaque choix, et un patron renommé ailleurs se voit ici
    // dès qu'on rouvre la page. Chercher plutôt que d'indexer, c'est le choix de la fenêtre de la bibliothèque.
    const QList<OPatternLibrary::Entry> patterns
        = OPatternLibrary::PatternsOf(OPatternLibrary::DefaultDirectory(), name, owner);

    table->setVisible(not patterns.isEmpty());
    if (placeholder != nullptr)
    {
        placeholder->setVisible(patterns.isEmpty());
    }

    for (const OPatternLibrary::Entry &entry : patterns)
    {
        const int row = table->rowCount();
        table->insertRow(row);

        auto *nameItem = new QTableWidgetItem(OPatternLibrary::DisplayName(entry));
        nameItem->setToolTip(entry.fileName);
        nameItem->setData(Qt::UserRole, entry.path);
        table->setItem(row, kHistoryNameColumn, nameItem);

        // La date seule dans la cellule : l'heure complète tient dans l'infobulle, et la colonne du nom garde la
        // place qu'il lui faut pour ne pas s'abréger.
        auto *modifiedItem = new QTableWidgetItem(entry.modified.toString(QStringLiteral("dd/MM/yyyy")));
        modifiedItem->setToolTip(entry.modified.toString(QStringLiteral("dd/MM/yyyy HH:mm")));
        table->setItem(row, kHistoryModifiedColumn, modifiedItem);

        table->setItem(row, kHistoryCollectionColumn,
                       new QTableWidgetItem(entry.collection.isEmpty() ? QStringLiteral("-") : entry.collection));
        table->setItem(row, kHistoryCustomerColumn,
                       new QTableWidgetItem(entry.customer.isEmpty() ? QStringLiteral("-") : entry.customer));

        auto *folderItem = new QTableWidgetItem(OLibraryIcon(QStringLiteral("folder-open")), QString());
        folderItem->setToolTip(tr("Show the folder that holds this pattern"));
        folderItem->setData(Qt::UserRole, entry.path);
        table->setItem(row, kHistoryFolderColumn, folderItem);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockEditor::OnHistoryClicked(QTableWidget *table, int row, int column)
{
    if (table == nullptr || column != kHistoryFolderColumn)
    {
        return;
    }

    const QTableWidgetItem *item = table->item(row, kHistoryFolderColumn);
    if (item == nullptr)
    {
        return;
    }

    const QFileInfo pattern(item->data(Qt::UserRole).toString());
    if (not pattern.exists())
    {
        return;
    }

    // Le dossier où vit le patron — ce qu'on irait chercher à la main dans le Finder.
    QDesktopServices::openUrl(QUrl::fromLocalFile(pattern.absolutePath()));
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

    FillHistory(m_clientsHistory, m_historyPlaceholder, m_clientNameEdit->text(),
                OPatternLibrary::Owner::Customer);
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

void OInfoBlockEditor::OnCreateClientFolder(bool checked)
{
    if (not checked)
    {
        return;  // Unticking says nothing: a folder that exists is not this window's to remove.
    }

    // The gesture is done at once, and the box goes back to its resting state: a tick left behind would claim a
    // folder this page would then have to keep true — and would have to unclaim the day it is renamed.
    m_clientFolderCheck->setChecked(false);

    const QString name = m_clientNameEdit->text().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::information(this, tr("Pattern library"), tr("Name the client first: the folder takes its name."));
        return;
    }

    const QString library = OPatternLibrary::DefaultDirectory();
    if (QDir(library).exists(name))
    {
        // The folder is already there — the case a reader makes twice without meaning to. Linking is the answer
        // that costs nothing; refusing is for the day the two are meant to be different things.
        const QMessageBox::StandardButton answer
            = QMessageBox::question(this, tr("Pattern library"),
                                    tr("The folder \"%1\" already exists in the pattern library.\n\n"
                                       "Link the client to it?")
                                        .arg(name),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer == QMessageBox::Yes)
        {
            emit StatusMessage(tr("The client is linked to the folder \"%1\"").arg(name));
        }
        else
        {
            QMessageBox::information(this, tr("Pattern library"),
                                     tr("Rename the client or the folder, so that the two can be told apart."));
        }
        return;
    }

    QString error;
    if (not OPatternLibrary::CreateDirectory(library, name, &error))
    {
        QMessageBox::warning(this, tr("Pattern library"), error);
        return;
    }

    emit StatusMessage(tr("The folder \"%1\" was created in the pattern library").arg(name));
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

    // The same bridge, on the collections' side: one folder per collection, so that the library of patterns can
    // be read the way the collections are thought of. Named here for the same reason as its twin.
    m_collectionFolderCheck = new QCheckBox(tr("Create the folder in the pattern library"));
    m_collectionFolderCheck->setToolTip(tr("Makes a folder of the collection's name in the library of patterns."));
    connect(m_collectionFolderCheck, &QCheckBox::toggled, this, &OInfoBlockEditor::OnCreateCollectionFolder);
    form->addRow(m_collectionFolderCheck);

    leftLayout->addWidget(m_collectionEditGroup);

    m_collectionsHistoryGroup = new QGroupBox(tr("History"));
    auto *historyLayout = new QVBoxLayout(m_collectionsHistoryGroup);

    m_collectionsHistory = CreateHistoryTable();
    connect(m_collectionsHistory, &QTableWidget::cellClicked, this,
            [this](int row, int column) { OnHistoryClicked(m_collectionsHistory, row, column); });
    historyLayout->addWidget(m_collectionsHistory, 1);

    m_collectionsHistoryPlaceholder = new QLabel(tr("No associated patterns yet."));
    m_collectionsHistoryPlaceholder->setAlignment(Qt::AlignCenter);
    m_collectionsHistoryPlaceholder->setWordWrap(true);
    historyLayout->addWidget(m_collectionsHistoryPlaceholder, 1);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(m_collectionsHistoryGroup);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({360, 540}); // voir la page Clients : cinq colonnes à droite
    vbox->addWidget(splitter, 1);

    m_menu->AddPage(collectionsPage);
}

void OInfoBlockEditor::ShowCollectionsPage()
{
    const bool ascending = VAbstractValApplication::VApp()->Settings()->GetCollectionsSortAscending();
    m_collectionsSortCombo->setCurrentIndex(ascending ? 1 : 0);
    m_collectionEntries = OPieceCartoucheStorage::LoadCollections(ascending);
    RefreshCollectionsList();
    m_collectionsTree->setCurrentItem(nullptr);
    OnCollectionSelected(nullptr);
    m_menu->ShowPage(4);
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

    FillHistory(m_collectionsHistory, m_collectionsHistoryPlaceholder, m_collectionNameEdit->text(),
                OPatternLibrary::Owner::Collection);
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

void OInfoBlockEditor::OnCreateCollectionFolder(bool checked)
{
    if (not checked)
    {
        return;  // Unticking says nothing: a folder that exists is not this window's to remove.
    }

    m_collectionFolderCheck->setChecked(false);

    const QString name = m_collectionNameEdit->text().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::information(this, tr("Pattern library"),
                                 tr("Name the collection first: the folder takes its name."));
        return;
    }

    const QString library = OPatternLibrary::DefaultDirectory();
    if (QDir(library).exists(name))
    {
        const QMessageBox::StandardButton answer
            = QMessageBox::question(this, tr("Pattern library"),
                                    tr("The folder \"%1\" already exists in the pattern library.\n\n"
                                       "Link the collection to it?")
                                        .arg(name),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer == QMessageBox::Yes)
        {
            emit StatusMessage(tr("The collection is linked to the folder \"%1\"").arg(name));
        }
        else
        {
            QMessageBox::information(this, tr("Pattern library"),
                                     tr("Rename the collection or the folder, so that the two can be told apart."));
        }
        return;
    }

    QString error;
    if (not OPatternLibrary::CreateDirectory(library, name, &error))
    {
        QMessageBox::warning(this, tr("Pattern library"), error);
        return;
    }

    emit StatusMessage(tr("The folder \"%1\" was created in the pattern library").arg(name));
}

void OInfoBlockEditor::NewTemplate()
{
    // Asking for a new template is asking to leave the one being edited, and this editor never
    // saves on its own. Without this, clicking « Create new » threw the changes away in silence —
    // the same gap the close prompt already covers, on the gesture that is far easier to make by
    // accident.
    if (!ConfirmDiscardChanges())
    {
        return;
    }

    m_template = OPieceCartoucheTemplate(tr("Template 1"));
    m_filePath.clear();
    m_menu->ShowPage(0);
    m_nameEdit->setText(m_template.Name());
    m_languageRow->SetLanguages(m_template.Languages());
    RefreshSectionList();
    SelectSection(-1);
    ApplyMenuRow();
    SetModified(false);
}

void OInfoBlockEditor::ShowTemplatesList()
{
    m_templatesPage->Refresh();
    m_menu->ShowPage(1);
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockEditor::OpenOnPage(int page)
{
    switch (page)
    {
        case 0:
            m_menu->HighlightRow(0);
            ShowTemplatesList();
            break;
        case 1:
            m_menu->HighlightRow(1);
            NewTemplate();
            break;
        case 2:
            m_menu->HighlightRow(2);
            ShowLogosPage();
            break;
        case 3:
            m_menu->HighlightRow(3);
            ShowClientsPage();
            break;
        case 4:
            m_menu->HighlightRow(4);
            ShowCollectionsPage();
            break;
        case 5:
            m_menu->HighlightRow(5);
            ShowAnnotationsPage();
            break;
        case 6:
            m_menu->HighlightRow(6);
            ShowThumbnailPage();
            break;
        case 7:
            m_menu->HighlightRow(7);
            ShowFoldLineTextPage();
            break;
        default:
            break;
    }
}

void OInfoBlockEditor::LoadFile(const QString &filePath)
{
    // Opening a template replaces the one in memory just as surely as « Create new » does, so it
    // asks the same question — and it asks it BEFORE reading the file, so a cancelled open leaves
    // nothing half-done.
    if (!ConfirmDiscardChanges())
    {
        return;
    }

    OPieceCartoucheTemplate tpl;
    QString err;
    if (!OPieceCartoucheStorage::Load(filePath, &tpl, &err))
    {
        QMessageBox::critical(this, tr("Information block"), err);
        return;
    }

    m_template = tpl;
    m_filePath = filePath;
    m_menu->ShowPage(0);
    m_nameEdit->setText(m_template.Name());
    m_languageRow->SetLanguages(m_template.Languages());
    RefreshSectionList();
    SelectSection(-1);
    ApplyMenuRow();
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

    // The Thumbnail page writes one legend line per language of the template being edited, so it
    // follows the same language row the cartouche preview does — pushed here, at the one place that
    // row is read.
    if (m_thumbnailStylesPage != nullptr)
    {
        m_thumbnailStylesPage->SetLanguages(m_template.Languages());
    }
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
    emit TemplateSaved();
}

void OInfoBlockEditor::OnSave()
{
    if (m_filePath.isEmpty())
    {
        OnSaveAs();
        return;
    }

    // The name is the identity of a template: the list shows it and the favourite names it. A name
    // typed in the field therefore MOVES the file rather than sitting beside it.
    //
    // Left alone, saving after a rename wrote a template called « B » into « A.oct » — the list
    // went on showing « A » while the template said « B », two names for one thing and nothing on
    // screen to say which one won.
    //
    // QFileInfo::fileName() rather than the raw text: it drops anything that looks like a path, so
    // a name typed with a slash in it cannot send the file somewhere else.
    const QString typed = QFileInfo(m_template.Name()).fileName();
    const QFileInfo current(m_filePath);

    if (typed.isEmpty() || typed == current.completeBaseName())
    {
        DoSave(m_filePath);
        return;
    }

    const QString target = current.absolutePath() + QLatin1Char('/') + typed
                           + QStringLiteral(".oct");

    // Another template already answers to that name. Writing over it would destroy something the
    // user never asked to touch.
    if (target != m_filePath && QFileInfo::exists(target))
    {
        QMessageBox::warning(this, tr("Information block"),
                             tr("A template called \"%1\" already exists.\n\nChoose another name.")
                                 .arg(typed));
        return;
    }

    // The field and the file agree on what was kept, not on what was typed.
    m_template.SetName(typed);
    m_nameEdit->setText(typed);

    const QString previous = m_filePath;
    DoSave(target);
    if (IsModified())
    {
        return; // the save did not happen: the old file is intact and still the one in force
    }

    // Written first, removed after: a save that fails must never lose the file it was moving.
    QFile::remove(previous);
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

void OInfoBlockEditor::ApplyMenuRow()
{
    // Derived, never stored — see the declaration. The page says which row; the file says whether
    // « Create new » is where the user is, or whether a template that exists is being edited.
    int row = -1;
    switch (m_menu->CurrentPage())
    {
        case 0: // the editor page
            row = m_filePath.isEmpty() ? 1 : -1;
            break;

        case 1: // the library list — row 0, and the page numbers are offset by one from there on
            row = 0;
            break;

        default:
            row = m_menu->CurrentPage();
            break;
    }

    m_menu->HighlightRow(row);
}

auto OInfoBlockEditor::ConfirmDiscardChanges() -> bool
{
    return OTemplateGuard::ConfirmDiscardChanges(this, tr("Unsaved information block"), this);
}

auto OInfoBlockEditor::HasUnsavedChanges() const -> bool
{
    return IsModified();
}

void OInfoBlockEditor::SaveDocument()
{
    OnSave();
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
