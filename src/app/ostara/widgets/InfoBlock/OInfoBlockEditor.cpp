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
#include "OPieceCartoucheCatalogue.h"
#include "OPieceCartoucheStorage.h"

#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSplitter>
#include <QStackedWidget>
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
    m_menuList->addItem(tr("Libraries"));
    m_menuList->item(2)->setFlags(m_menuList->item(2)->flags() & ~Qt::ItemIsEnabled);
    m_menuList->item(2)->setToolTip(tr("Coming in a later version."));
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
    });
    outer->addWidget(m_menuList);

    m_stack = new QStackedWidget();
    SetupEditorPage();    // index 0
    SetupTemplatesPage(); // index 1
    outer->addWidget(m_stack, 1);

    m_menuList->setCurrentRow(1);
    NewTemplate();
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
    auto *menu = new QMenu(m_addBtn);
    for (const OPieceCartoucheSection &section : OPieceCartoucheCatalogue::Sections())
    {
        QAction *action = menu->addAction(section.DisplayName());
        action->setToolTip(section.Description());
        connect(action, &QAction::triggered, this, [this, id = section.Id()]() { OnAddSection(id); });
    }
    m_addBtn->setMenu(menu);

    auto *upBtn = new QPushButton();
    upBtn->setObjectName(QStringLiteral("infoBlockUpBtn"));
    upBtn->setToolTip(tr("Move up"));
    connect(upBtn, &QPushButton::clicked, this, [this]() {
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

    auto *downBtn = new QPushButton();
    downBtn->setObjectName(QStringLiteral("infoBlockDownBtn"));
    downBtn->setToolTip(tr("Move down"));
    connect(downBtn, &QPushButton::clicked, this, [this]() {
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

    auto *saveBtn = new QPushButton(tr("Save"));
    connect(saveBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnSave);
    topBar->addWidget(saveBtn);

    auto *saveAsBtn = new QPushButton(tr("Save As…"));
    connect(saveAsBtn, &QPushButton::clicked, this, &OInfoBlockEditor::OnSaveAs);
    topBar->addWidget(saveAsBtn);

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
    listBar->addWidget(upBtn);
    listBar->addWidget(downBtn);
    listBar->addStretch();
    leftLayout->addLayout(listBar);

    leftLayout->addWidget(m_sectionList, 1);
    leftLayout->addWidget(m_sectionEditor, 0);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(left);
    splitter->addWidget(m_preview);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    vbox->addWidget(splitter, 1);

    m_stack->addWidget(editorPage);
}

void OInfoBlockEditor::SetupTemplatesPage()
{
    auto *templatesPage = new QWidget();
    auto *layout = new QVBoxLayout(templatesPage);

    auto *title = new QLabel(tr("Existing templates"), templatesPage);
    QFont titleFont = title->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    m_templatesList = new QListWidget();
    connect(m_templatesList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        LoadFile(item->data(Qt::UserRole).toString());
    });
    layout->addWidget(m_templatesList, 1);

    m_stack->addWidget(templatesPage);
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
