/************************************************************************
 **
 **  @file   oexportfavoriteswidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  One list of favorites of the entry dialog — the same widget for the cutting
 **          plans, the prints and the plotter plans.
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

#ifndef OEXPORTFAVORITESWIDGET_H
#define OEXPORTFAVORITESWIDGET_H

#include "oexportrowlistwidget.h"

#include <QStringList>

#include "../core/oexportrunqueue.h"

class OExportFavoriteRowWidget;
class QPushButton;

/**
 * @brief The favorites of one kind — cutting plan, print, or plotter plan.
 *
 * Instanced three times, once per sub-tab, and there is no per-kind behaviour to speak of: the
 * dialog hands each instance the favorites of its own kind, already filtered against the
 * current measurements table. Hence one class rather than three.
 *
 * Every line starts ticked, and the batch runs the ticked ones. Two buttons say so in bulk —
 * "Select all" and "Deselect all" — because a batch is described by exception as often as not:
 * five favorites kept, one to leave out today.
 *
 * The NAME is the identity here, not the row: the batch writes a line's state while it walks,
 * and the list is sorted, so an index would be a position that moves under the walk. A name
 * cannot move — two favorites of one (table, axis) never share it.
 */
class OExportFavoritesWidget : public OExportRowListWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OExportFavoritesWidget(QWidget *parent = nullptr);
    ~OExportFavoritesWidget() override = default;

    /// @brief Show @p names, in the order given, every box ticked.
    void SetFavorites(const QStringList &names);

    /// @brief Write where the favorite @p name stands. No-op when it is not listed.
    void SetState(const QString &name, OExportRunState state);

    /// @brief Drop the row of @p name. No-op when it is not listed.
    void RemoveRow(const QString &name);

    /// @brief Whether the box of @p name is ticked. False when it is not listed.
    auto IsChecked(const QString &name) const -> bool;

    /// @brief The names whose box is ticked — what the batch will run.
    auto CheckedNames() const -> QStringList;

    auto CheckedCount() const -> int;
    auto HasFavorites() const -> bool;

    void SelectAll();
    void DeselectAll();

signals:
    /// @brief A box was ticked or unticked: what the batch would run has changed.
    void CheckedChanged();

    /// @brief The trash of the favorite @p name was clicked.
    void RemoveRequested(const QString &name);

protected:
    void changeEvent(QEvent *event) override;

    /// @brief Lock the buttons and the trash while a batch walks the list, and keep the bulk
    ///        buttons in step with what is ticked.
    void Refresh() override;

private:
    void Retranslate();
    void AppendRow(const QString &name);

    auto RowAt(const QString &name) const -> OExportFavoriteRowWidget *;

    QPushButton *m_selectAll{nullptr};
    QPushButton *m_deselectAll{nullptr};
};

#endif // OEXPORTFAVORITESWIDGET_H
