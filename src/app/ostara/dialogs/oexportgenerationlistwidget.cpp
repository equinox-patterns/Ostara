/************************************************************************
 **
 **  @file   oexportgenerationlistwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the "list to generate" tab.
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

#include "oexportgenerationlistwidget.h"

#include "oexportgenerationrowwidget.h"

#include <QEvent>
#include <QLabel>
#include <QPalette>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OExportGenerationListWidget::OExportGenerationListWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_listContainer = new QWidget();
    m_listLayout = new QVBoxLayout(m_listContainer);
    m_listLayout->setContentsMargins(0, 0, 0, 0);
    m_listLayout->setSpacing(0);
    m_scrollArea->setWidget(m_listContainer);
    layout->addWidget(m_scrollArea, 1);

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
    layout->addWidget(m_emptyHint, 1);

    Retranslate();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::AppendRow(const QString &name)
{
    auto *row = new OExportGenerationRowWidget(name, m_listContainer);
    // Vertical Fixed: rows keep their sizeHint and stack one after another instead
    // of being stretched by the container layout's spare space.
    row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // Before the stretch, which must stay last — it is what absorbs the spare height.
    m_listLayout->insertWidget(m_listLayout->count() - 1, row);
    m_rows.append(row);

    connect(row->DeleteButton(), &QPushButton::clicked, this,
            [this, row]()
            {
                // Read here, never captured: a row taken down earlier shifts the ones
                // after it, so the index a row had when it was built is not the one it
                // has when its button is clicked.
                const qsizetype index = m_rows.indexOf(row);
                if (index >= 0)
                {
                    emit RemoveRequested(static_cast<int>(index));
                }
            });

    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::RemoveRow(int index)
{
    auto *row = RowAt(index);
    if (row == nullptr)
    {
        return;
    }

    m_rows.removeAt(index);
    m_listLayout->removeWidget(row);
    // Deferred, not deleted here: this runs inside the clicked() handler of a button of
    // that very row, and the signal that led here is still unwinding through it.
    row->deleteLater();

    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::SetState(int index, OExportRunState state)
{
    if (auto *row = RowAt(index); row != nullptr)
    {
        row->SetState(state);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGenerationListWidget::IsEmpty() const -> bool
{
    return m_rows.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::SetBusy(bool busy)
{
    m_busy = busy;
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGenerationListWidget::RowAt(int index) const -> OExportGenerationRowWidget *
{
    if (index < 0 || index >= m_rows.size())
    {
        return nullptr;
    }

    return m_rows.at(index);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::Refresh()
{
    const bool empty = m_rows.isEmpty();

    m_scrollArea->setVisible(!empty);
    m_emptyHint->setVisible(empty);

    for (OExportGenerationRowWidget *row : m_rows)
    {
        row->DeleteButton()->setEnabled(!m_busy);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::Retranslate()
{
    m_emptyHint->setText(tr("Nothing to generate yet.\nAdd a run from the Print tab."));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
