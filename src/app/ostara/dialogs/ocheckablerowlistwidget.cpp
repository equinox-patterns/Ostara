/************************************************************************
 **
 **  @file   ocheckablerowlistwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/10/2026
 **
 **  @brief  Implementation of the list of names to tick.
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

#include "ocheckablerowlistwidget.h"

#include "ocheckablerowwidget.h"

#include <QCheckBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QPushButton>

//---------------------------------------------------------------------------------------------------------------------
OCheckableRowListWidget::OCheckableRowListWidget(QWidget *parent)
  : OExportRowListWidget(parent)
{
    auto *bar = new QHBoxLayout();
    m_selectAll = new QPushButton(this);
    m_selectAll->setAutoDefault(false);
    bar->addWidget(m_selectAll);
    m_deselectAll = new QPushButton(this);
    m_deselectAll->setAutoDefault(false);
    bar->addWidget(m_deselectAll);
    bar->addStretch();
    SetTopBar(bar);

    connect(m_selectAll, &QPushButton::clicked, this, &OCheckableRowListWidget::SelectAll);
    connect(m_deselectAll, &QPushButton::clicked, this, &OCheckableRowListWidget::DeselectAll);

    Retranslate();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowListWidget::SetNames(const QStringList &names)
{
    // Walked over a copy of the list, which is what makes removing while walking safe.
    for (OCheckableRowWidget *row : RowWidgets<OCheckableRowWidget>())
    {
        RemoveRowWidget(row);
    }

    for (const QString &name : names)
    {
        AppendRow(name);
    }

    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowListWidget::SetUnavailable(const QStringList &names, const QString &reason)
{
    for (const QString &name : names)
    {
        if (auto *row = RowAt(name); row != nullptr)
        {
            row->SetUnavailable(reason);
        }
    }

    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckableRowListWidget::CheckedNames() const -> QStringList
{
    QStringList names;
    for (OCheckableRowWidget *row : RowWidgets<OCheckableRowWidget>())
    {
        if (row->CheckBox()->isChecked())
        {
            names.append(row->Name());
        }
    }
    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckableRowListWidget::CheckedCount() const -> int
{
    int count = 0;
    for (OCheckableRowWidget *row : RowWidgets<OCheckableRowWidget>())
    {
        if (row->CheckBox()->isChecked())
        {
            ++count;
        }
    }
    return count;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckableRowListWidget::HasNames() const -> bool
{
    return !IsEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowListWidget::SelectAll()
{
    // Only what may be ticked: a row locked out of ticking is a name the gesture cannot apply to,
    // and re-ticking it would make "select all" mean "all but the ones that say they cannot".
    for (OCheckableRowWidget *row : RowWidgets<OCheckableRowWidget>())
    {
        if (row->IsAvailable())
        {
            row->SetChecked(true);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowListWidget::DeselectAll()
{
    for (OCheckableRowWidget *row : RowWidgets<OCheckableRowWidget>())
    {
        row->SetChecked(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowListWidget::AppendRow(const QString &name)
{
    auto *row = new OCheckableRowWidget(name);
    AppendRowWidget(row); // reparents it into the scrolling stack
    row->SetChecked(true); // a list is filled to be used as it stands; see the header

    connect(row->CheckBox(), &QCheckBox::toggled, this, [this](bool) { emit CheckedChanged(); });
}

//---------------------------------------------------------------------------------------------------------------------
auto OCheckableRowListWidget::RowAt(const QString &name) const -> OCheckableRowWidget *
{
    for (OCheckableRowWidget *row : RowWidgets<OCheckableRowWidget>())
    {
        if (row->Name() == name)
        {
            return row;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowListWidget::Refresh()
{
    OExportRowListWidget::Refresh();

    for (OCheckableRowWidget *row : RowWidgets<OCheckableRowWidget>())
    {
        row->CheckBox()->setEnabled(!IsBusy() && row->IsAvailable());
    }

    // Each button says why it can do nothing by going out, rather than by doing nothing when clicked.
    if (m_selectAll != nullptr)
    {
        m_selectAll->setEnabled(!IsBusy() && HasNames());
    }
    if (m_deselectAll != nullptr)
    {
        m_deselectAll->setEnabled(!IsBusy() && CheckedCount() > 0);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowListWidget::Retranslate()
{
    SetEmptyHint(tr("No favorite yet.\nStar a size range in the Size Ranges dock to keep its definition here."));
    m_selectAll->setText(tr("Select all"));
    m_deselectAll->setText(tr("Deselect all"));
}

//---------------------------------------------------------------------------------------------------------------------
void OCheckableRowListWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    OExportRowListWidget::changeEvent(event);
}
