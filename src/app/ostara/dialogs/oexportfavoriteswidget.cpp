/************************************************************************
 **
 **  @file   oexportfavoriteswidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of one list of favorites.
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

#include "oexportfavoriteswidget.h"

#include "oexportfavoriterowwidget.h"

#include <QCheckBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QPushButton>

//---------------------------------------------------------------------------------------------------------------------
OExportFavoritesWidget::OExportFavoritesWidget(QWidget *parent)
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

    connect(m_selectAll, &QPushButton::clicked, this, &OExportFavoritesWidget::SelectAll);
    connect(m_deselectAll, &QPushButton::clicked, this, &OExportFavoritesWidget::DeselectAll);

    Retranslate();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoritesWidget::SetFavorites(const QStringList &names)
{
    // Walked over a copy of the list, which is what makes removing while walking safe.
    for (OExportFavoriteRowWidget *row : RowWidgets<OExportFavoriteRowWidget>())
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
void OExportFavoritesWidget::SetState(const QString &name, OExportRunState state)
{
    if (auto *row = RowAt(name); row != nullptr)
    {
        row->SetState(state);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoritesWidget::RemoveRow(const QString &name)
{
    if (auto *row = RowAt(name); row != nullptr)
    {
        RemoveRowWidget(row);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavoritesWidget::IsChecked(const QString &name) const -> bool
{
    const auto *row = RowAt(name);
    return row != nullptr && row->CheckBox()->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavoritesWidget::CheckedNames() const -> QStringList
{
    QStringList names;
    for (OExportFavoriteRowWidget *row : RowWidgets<OExportFavoriteRowWidget>())
    {
        if (row->CheckBox()->isChecked())
        {
            names.append(row->Name());
        }
    }
    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavoritesWidget::CheckedCount() const -> int
{
    int count = 0;
    for (OExportFavoriteRowWidget *row : RowWidgets<OExportFavoriteRowWidget>())
    {
        if (row->CheckBox()->isChecked())
        {
            ++count;
        }
    }
    return count;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavoritesWidget::HasFavorites() const -> bool
{
    return !IsEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoritesWidget::SelectAll()
{
    for (OExportFavoriteRowWidget *row : RowWidgets<OExportFavoriteRowWidget>())
    {
        row->CheckBox()->setChecked(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoritesWidget::DeselectAll()
{
    for (OExportFavoriteRowWidget *row : RowWidgets<OExportFavoriteRowWidget>())
    {
        row->CheckBox()->setChecked(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoritesWidget::AppendRow(const QString &name)
{
    auto *row = new OExportFavoriteRowWidget(name);
    AppendRowWidget(row); // reparents it into the scrolling stack

    connect(row->CheckBox(), &QCheckBox::toggled, this, [this](bool) { emit CheckedChanged(); });

    // The name is captured by value, not read back off the row when the click lands: the click
    // is handled by the dialog, which may rebuild this list before anything else runs.
    connect(row->StarButton(), &QPushButton::clicked, this, [this, name]() { emit RemoveRequested(name); });
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavoritesWidget::RowAt(const QString &name) const -> OExportFavoriteRowWidget *
{
    for (OExportFavoriteRowWidget *row : RowWidgets<OExportFavoriteRowWidget>())
    {
        if (row->Name() == name)
        {
            return row;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoritesWidget::Refresh()
{
    OExportRowListWidget::Refresh();

    for (OExportFavoriteRowWidget *row : RowWidgets<OExportFavoriteRowWidget>())
    {
        row->StarButton()->setEnabled(!IsBusy());
        row->CheckBox()->setEnabled(!IsBusy());
    }

    // « Deselect all » n'a rien à faire sur une liste vide, et il le dit en s'éteignant plutôt
    // qu'en ne faisant rien quand on clique.
    if (m_selectAll != nullptr)
    {
        m_selectAll->setEnabled(!IsBusy() && HasFavorites());
    }
    if (m_deselectAll != nullptr)
    {
        m_deselectAll->setEnabled(!IsBusy() && CheckedCount() > 0);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoritesWidget::Retranslate()
{
    SetEmptyHint(tr("No favorite yet.\nStar a generation in the Outputs dock to keep its recipe here."));
    m_selectAll->setText(tr("Select all"));
    m_deselectAll->setText(tr("Deselect all"));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavoritesWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    OExportRowListWidget::changeEvent(event);
}
