/************************************************************************
 **
 **  @file   oexportrowlistwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of the scrolling stack of rows.
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

#include "oexportrowlistwidget.h"

#include <QLabel>
#include <QLayout>
#include <QScrollArea>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OExportRowListWidget::OExportRowListWidget(QWidget *parent)
  : QWidget(parent)
{
    m_rootLayout = new QVBoxLayout(this);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_listContainer = new QWidget();
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(0);
    m_scrollArea->setWidget(m_listContainer);
    m_rootLayout->addWidget(m_scrollArea, 1);

    // The scroll-area viewport paints a flat NATIVE background (white on macOS) that
    // overrides global QSS and palette. Only an inline dot-selector with an OPAQUE
    // color works (SubtiliteCode §13); `transparent` would let the native white show
    // through. Use the Window color (#232323 dark / #F9F9F9 light), read from the
    // palette so it always follows the current theme.
    m_scrollArea->viewport()->setStyleSheet(
        QStringLiteral(".QWidget { background: %1; }").arg(palette().color(QPalette::Window).name()));

    // The widgetResizable scroll area stretches the container to the viewport height;
    // without this, the spare space would be spread into gaps between the rows. Absorb
    // it at the end so the rows stay stacked one after another at the top.
    m_listLayout->addStretch(1);

    // Shown instead of the list while it holds nothing: an empty page with a lone greyed
    // button says "broken", where one line of text says what to do about it.
    m_emptyHint = new QLabel(this);
    m_emptyHint->setAlignment(Qt::AlignCenter);
    m_emptyHint->setWordWrap(true);
    m_emptyHint->setObjectName(QStringLiteral("exportListHint"));
    m_rootLayout->addWidget(m_emptyHint, 1);

    // Set directly rather than through Refresh(): the list is empty by construction, and a
    // virtual call from a constructor would run before any subclass exists.
    m_scrollArea->setVisible(false);
    m_emptyHint->setVisible(true);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRowListWidget::IsEmpty() const -> bool
{
    return m_rows.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRowListWidget::IsBusy() const -> bool
{
    return m_busy;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowListWidget::SetBusy(bool busy)
{
    m_busy = busy;
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowListWidget::AppendRowWidget(QWidget *row)
{
    // Vertical Fixed: rows keep their sizeHint and stack one after another instead
    // of being stretched by the container layout's spare space.
    row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Before the stretch, which must stay last — it is what absorbs the spare height.
    m_listLayout->insertWidget(m_listLayout->count() - 1, row);
    m_rows.append(row);

    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowListWidget::RemoveRowWidget(QWidget *row)
{
    const qsizetype index = m_rows.indexOf(row);
    if (index < 0)
    {
        return;
    }

    m_rows.removeAt(index);
    m_listLayout->removeWidget(row);
    row->deleteLater(); // see the header: the click that asked for this may be its own
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowListWidget::SetEmptyHint(const QString &text)
{
    m_emptyHint->setText(text);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowListWidget::SetTopBar(QLayout *bar)
{
    // Above the scroll area, which is the first thing the root layout holds.
    m_rootLayout->insertLayout(0, bar);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRowListWidget::Refresh()
{
    const bool empty = m_rows.isEmpty();

    m_scrollArea->setVisible(!empty);
    m_emptyHint->setVisible(empty);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRowListWidget::RootLayout() const -> QVBoxLayout *
{
    return m_rootLayout;
}
