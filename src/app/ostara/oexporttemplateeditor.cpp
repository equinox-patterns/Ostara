/************************************************************************
 **
 **  @file   oexporttemplateeditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Export template editor — one tab per template family.
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

#include "oexporttemplateeditor.h"

#include "oexporttemplatepreview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QShowEvent>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
/**
 * Menu entries of the « Assembly marks » tab, in menu order. The row index is also the
 * index of the page it shows, which is why the two are never stored apart.
 */
enum MenuRow : int
{
    MenuExistingTemplates = 0,
    MenuCreateTemplate = 1,
    MenuCustomMarks = 2
};

constexpr int kMenuWidth = 170;

/**
 * Minimum width of each half of the « Create a template » split. Deliberately the same on both
 * sides: before an explicit split is applied, the splitter falls back on the two widgets' size
 * hints, and a hint of ~90 px against one of 200 px hands the preview two thirds of the width.
 */
constexpr int kPaneMinimum = 200;

enum Tab : int
{
    TabCoverPage = 0,
    TabAssemblyMarks = 1
};
} // namespace

OExportTemplateEditor::OExportTemplateEditor(QWidget *parent) : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);

    m_tabs = new QTabWidget();
    m_tabs->addTab(MakePlaceholderPage(), QString());  // index 0 — cover page
    m_tabs->addTab(MakeAssemblyMarksTab(), QString()); // index 1 — assembly marks
    outer->addWidget(m_tabs);

    RetranslateUI();

    m_tabs->setCurrentIndex(TabAssemblyMarks);
    m_menuList->setCurrentRow(MenuCreateTemplate);
    ShowMenuPage(MenuCreateTemplate);
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::OpenOnTab(int tab)
{
    if (tab >= 0 && tab < m_tabs->count())
    {
        m_tabs->setCurrentIndex(tab);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::RetranslateUI()
{
    m_tabs->setTabText(TabCoverPage, tr("Cover page"));
    m_tabs->setTabText(TabAssemblyMarks, tr("Assembly marks"));

    m_menuList->item(MenuExistingTemplates)->setText(tr("Existing templates"));
    m_menuList->item(MenuCreateTemplate)->setText(tr("Create a template"));
    m_menuList->item(MenuCustomMarks)->setText(tr("Custom marks"));

    for (QLabel *label : m_placeholders)
    {
        label->setText(tr("Coming soon"));
    }
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeAssemblyMarksTab()
{
    auto *page = new QWidget();

    auto *layout = new QHBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    m_menuList = new QListWidget();
    m_menuList->setFixedWidth(kMenuWidth);
    m_menuList->addItem(QString()); // index 0 — existing templates
    m_menuList->addItem(QString()); // index 1 — create a template
    m_menuList->addItem(QString()); // index 2 — custom marks
    connect(m_menuList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        ShowMenuPage(m_menuList->row(item));
    });
    layout->addWidget(m_menuList);

    m_stack = new QStackedWidget();
    m_stack->addWidget(MakePlaceholderPage());    // index 0 — existing templates
    m_stack->addWidget(MakeCreateTemplatePage()); // index 1 — create a template
    m_stack->addWidget(MakePlaceholderPage());    // index 2 — custom marks
    layout->addWidget(m_stack, 1);

    return page;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeCreateTemplatePage()
{
    m_splitter = new QSplitter(Qt::Horizontal);

    auto *edition = MakeEditionPane();
    edition->setMinimumWidth(kPaneMinimum);
    m_splitter->addWidget(edition);

    m_preview = new OExportTemplatePreview();
    m_preview->setMinimumWidth(kPaneMinimum);
    m_splitter->addWidget(m_preview);

    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    // The explicit split is asked for in showEvent() — see there why it cannot be asked here.

    return m_splitter;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);

    // 50/50, asked once the first layout pass has run. Called straight from here, setSizes()
    // would be read against a splitter that has no width yet, and the splitter would fall back
    // on the two widgets' size hints. Queued, it runs after the geometry has been applied.
    // setSizes() normalises proportionally, so two equal values mean two equal halves.
    if (!m_splitterSized)
    {
        m_splitterSized = true;
        QTimer::singleShot(0, this, [this]() {
            if (m_splitter != nullptr)
            {
                m_splitter->setSizes({1, 1});
            }
        });
    }
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakeEditionPane()
{
    auto *pane = new QWidget();

    auto *layout = new QVBoxLayout(pane);
    layout->addWidget(MakePlaceholderLabel());

    return pane;
}

//---------------------------------------------------------------------------------------------------------------------

QLabel *OExportTemplateEditor::MakePlaceholderLabel()
{
    auto *label = new QLabel();
    label->setAlignment(Qt::AlignCenter);
    m_placeholders.append(label);
    return label;
}

//---------------------------------------------------------------------------------------------------------------------

QWidget *OExportTemplateEditor::MakePlaceholderPage()
{
    auto *page = new QWidget();

    auto *layout = new QVBoxLayout(page);
    layout->addWidget(MakePlaceholderLabel());

    return page;
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplateEditor::ShowMenuPage(int row)
{
    if (row < 0 || row >= m_stack->count())
    {
        return;
    }

    m_stack->setCurrentIndex(row);
}
