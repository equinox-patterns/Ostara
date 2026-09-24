/************************************************************************
 **
 **  @file   oexportgenerationlistwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the Export mode's "list to generate" tab.
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
#include <QPushButton>

//---------------------------------------------------------------------------------------------------------------------
OExportGenerationListWidget::OExportGenerationListWidget(QWidget *parent)
  : OExportRowListWidget(parent)
{
    Retranslate();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::AppendRow(const QString &name)
{
    auto *row = new OExportGenerationRowWidget(name);
    AppendRowWidget(row); // reparents it into the scrolling stack

    connect(row->DeleteButton(), &QPushButton::clicked, this,
            [this, row]()
            {
                // Read here, never captured: a row taken down earlier shifts the ones
                // after it, so the index a row had when it was built is not the one it
                // has when its button is clicked.
                const qsizetype index = RowWidgets<OExportGenerationRowWidget>().indexOf(row);
                if (index >= 0)
                {
                    emit RemoveRequested(static_cast<int>(index));
                }
            });
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::RemoveRow(int index)
{
    if (auto *row = RowAt(index); row != nullptr)
    {
        RemoveRowWidget(row);
    }
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
auto OExportGenerationListWidget::RowAt(int index) const -> OExportGenerationRowWidget *
{
    const QVector<OExportGenerationRowWidget *> rows = RowWidgets<OExportGenerationRowWidget>();
    if (index < 0 || index >= rows.size())
    {
        return nullptr;
    }

    return rows.at(index);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::Refresh()
{
    OExportRowListWidget::Refresh();

    for (OExportGenerationRowWidget *row : RowWidgets<OExportGenerationRowWidget>())
    {
        row->DeleteButton()->setEnabled(!IsBusy());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::Retranslate()
{
    SetEmptyHint(tr("Nothing to generate yet.\nAdd a run from the Print tab."));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGenerationListWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    OExportRowListWidget::changeEvent(event);
}
