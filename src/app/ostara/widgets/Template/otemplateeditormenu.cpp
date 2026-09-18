/************************************************************************
 **
 **  @file   otemplateeditormenu.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The left menu of a template editor, and the pages it opens.
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

#include "otemplateeditormenu.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QStackedWidget>

namespace
{
/**
 * The width of the column of names.
 *
 * Fixed rather than left to the longest name: a menu that widened when the language changed would
 * move the page beside it, and the pages are what the user is looking at.
 */
constexpr int kRowsWidth = 170;
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OTemplateEditorMenu::OTemplateEditorMenu(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_rows = new QListWidget(this);
    m_rows->setFixedWidth(kRowsWidth);
    connect(m_rows, &QListWidget::itemClicked, this,
            [this](QListWidgetItem *item) { emit RowClicked(m_rows->row(item)); });
    layout->addWidget(m_rows);

    m_pages = new QStackedWidget(this);
    layout->addWidget(m_pages, 1);
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateEditorMenu::AddPage(QWidget *page)
{
    m_pages->addWidget(page);
    m_rows->addItem(QString()); // named by the family, when it words itself
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateEditorMenu::SetRowLabel(int row, const QString &label)
{
    if (QListWidgetItem *item = m_rows->item(row); item != nullptr)
    {
        item->setText(label);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateEditorMenu::ShowPage(int page)
{
    if (page < 0 || page >= m_pages->count())
    {
        return;
    }

    m_pages->setCurrentIndex(page);
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateEditorMenu::CurrentPage() const -> int
{
    return m_pages->currentIndex();
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateEditorMenu::HighlightRow(int row)
{
    if (row < 0)
    {
        m_rows->clearSelection();
        m_rows->setCurrentRow(-1);
        return;
    }

    if (m_rows->currentRow() != row)
    {
        m_rows->setCurrentRow(row);
    }
}
