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

#include <QVector>
#include <QWidget>

#include "../core/oexportrunqueue.h"

class OExportGenerationRowWidget;
class QLabel;
class QScrollArea;
class QVBoxLayout;

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
 * It carries no action button. The tab is a page of a dialog whose button bar follows it
 * — "Generate the whole list" belongs there, with the other ways out, not here where a
 * second button would answer to almost the same name as the bar's own.
 */
class OExportGenerationListWidget : public QWidget
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

    auto IsEmpty() const -> bool;

    /// @brief Lock what acts on the list, without greying what it shows.
    ///
    /// A batch locks the list while it walks it — a row taken down under the walk would
    /// shift every row after it — but the rows themselves must stay legible: the states
    /// are exactly what the user is watching.
    void SetBusy(bool busy);

signals:
    /// @brief The delete button of the row at @p index was clicked.
    void RemoveRequested(int index);

protected:
    void changeEvent(QEvent *event) override;

private:
    void Retranslate();

    /// @brief Put the empty hint and the rows back in step with the list.
    void Refresh();

    auto RowAt(int index) const -> OExportGenerationRowWidget *;

    QVector<OExportGenerationRowWidget *> m_rows{};
    QScrollArea *m_scrollArea{nullptr};
    QWidget *m_listContainer{nullptr};
    QVBoxLayout *m_listLayout{nullptr};
    QLabel *m_emptyHint{nullptr};
    bool m_busy{false};

    Q_DISABLE_COPY_MOVE(OExportGenerationListWidget) // NOLINT
};

#endif // OEXPORTGENERATIONLISTWIDGET_H
