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
#include "OLogoPreview.h"
#include "OPieceCartoucheCatalogue.h"
#include "OPieceCartoucheStorage.h"

#include <QAction>
#include <QDir>
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
    });
    outer->addWidget(m_menuList);

    m_stack = new QStackedWidget();
    SetupEditorPage();     // index 0
    SetupTemplatesPage();  // index 1
    SetupLogosPage();      // index 2
    outer->addWidget(m_stack, 1);

    m_menuList->setCurrentRow(1);
    NewTemplate();
}

void OInfoBlockEditor::RetranslateUI()
{
    // The catalogue caches its translated section names once — drop it so the
    // rebuild below (menu, section list) resolves in the new language.
    OPieceCartoucheCatalogue::InvalidateCache();

    // Left menu — 3 fixed entries.
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

    // Editor page — top bar.
    m_nameEdit->setPlaceholderText(tr("Template name"));
    m_addBtn->setText(tr("+ Add information"));
    m_saveBtn->setText(tr("Save"));
    m_saveAsBtn->setText(tr("Save As…"));
    m_upBtn->setToolTip(tr("Move up"));
    m_downBtn->setToolTip(tr("Move down"));

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
    m_templatesTitle->setText(tr("Existing templates"));
    if (m_templatesList->count() == 1 && m_templatesList->item(0)->data(Qt::UserRole).toString().isEmpty())
    {
        m_templatesList->item(0)->setText(tr("No templates yet. Create one."));
    }

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

    // Order bar just above the list: Add information + move up/down
    auto *listBar = new QHBoxLayout();
    listBar->setSpacing(4);
    listBar->addWidget(m_addBtn);
    listBar->addWidget(m_upBtn);
    listBar->addWidget(m_downBtn);
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

void OInfoBlockEditor::SetupTemplatesPage()
{
    auto *templatesPage = new QWidget();
    auto *layout = new QVBoxLayout(templatesPage);

    m_templatesTitle = new QLabel(tr("Existing templates"), templatesPage);
    QFont titleFont = m_templatesTitle->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    m_templatesTitle->setFont(titleFont);
    layout->addWidget(m_templatesTitle);

    m_templatesList = new QListWidget();
    connect(m_templatesList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        LoadFile(item->data(Qt::UserRole).toString());
    });
    layout->addWidget(m_templatesList, 1);

    m_stack->addWidget(templatesPage);
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

void OInfoBlockEditor::NewTemplate()
{
    m_template = OPieceCartoucheTemplate(tr("Template 1"));
    m_filePath.clear();
    m_stack->setCurrentIndex(0);
    m_nameEdit->setText(m_template.Name());
    RefreshSectionList();
    SelectSection(-1);
    SetModified(false);
}

void OInfoBlockEditor::ShowTemplatesList()
{
    RefreshTemplateList();
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

void OInfoBlockEditor::RefreshTemplateList()
{
    m_templatesList->clear();
    const QStringList paths = OPieceCartoucheStorage::ScanDirectory(OPieceCartoucheStorage::DefaultDirectory());
    if (paths.isEmpty())
    {
        auto *item = new QListWidgetItem(tr("No templates yet. Create one."));
        item->setFlags(Qt::NoItemFlags);
        m_templatesList->addItem(item);
        return;
    }
    for (const QString &path : paths)
    {
        auto *item = new QListWidgetItem(QFileInfo(path).completeBaseName());
        item->setData(Qt::UserRole, path);
        m_templatesList->addItem(item);
    }
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
