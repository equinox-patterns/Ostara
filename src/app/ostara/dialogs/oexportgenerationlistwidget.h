/************************************************************************
 **
 **  @file   oexportgenerationlistwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The Export mode's "list to generate" tab — the runs that have
 **          been described but not fired, and the button that fires them.
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

#ifndef OEXPORTGENERATIONLISTWIDGET_H
#define OEXPORTGENERATIONLISTWIDGET_H

#include "oexportrowlistwidget.h"

#include "../core/oexportrunqueue.h"

class OExportGenerationRowWidget;

/**
 * @brief The list of runs to generate, as a stack of rows.
 *
 * A view and nothing else: it holds no queue of its own. The entry dialog owns the queue
 * and this shows it, which is why every mutator here is something the dialog calls after
 * it has already changed the queue — the two can never disagree about what a line is,
 * only about whether a row has caught up.
 *
 * Rows are added and removed one at a time rather than reloaded from the queue: a reload
 * would rebuild every row on every change, and the batch repaints this list line by line
 * while the user watches.
 *
 * The frame — the scroll area, the hint shown while the list is empty, what busy means —
 * belongs to OExportRowListWidget, which every list of this dialog's entry screen shares.
 */
class OExportGenerationListWidget : public OExportRowListWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OExportGenerationListWidget(QWidget *parent = nullptr);
    ~OExportGenerationListWidget() override = default;

    /// @brief Stack one more row at the end of the list.
    void AppendRow(const QString &name);

    /// @brief Take the row at @p index down. No-op when the index is out of range.
    void RemoveRow(int index);

    /// @brief Write where the row at @p index stands. No-op when the index is out of range.
    void SetState(int index, OExportRunState state);

signals:
    /// @brief The delete button of the row at @p index was clicked.
    void RemoveRequested(int index);

protected:
    void changeEvent(QEvent *event) override;

    /// @brief Show or hide the rows and the hint — and lock the delete buttons while a batch
    ///        walks the list.
    void Refresh() override;

private:
    void Retranslate();

    auto RowAt(int index) const -> OExportGenerationRowWidget *;
};

#endif // OEXPORTGENERATIONLISTWIDGET_H
