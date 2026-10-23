/************************************************************************
 **
 **  @file   ocheckablerowlistwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/10/2026
 **
 **  @brief  A list of names to tick, with "select all" / "deselect all" — what acts on a whole
 **          list of names when the gesture is "those, in one go".
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

#ifndef OCHECKABLEROWLISTWIDGET_H
#define OCHECKABLEROWLISTWIDGET_H

#include "oexportrowlistwidget.h"

#include <QStringList>

class OCheckableRowWidget;
class QPushButton;

/**
 * @brief Several names, each with a box, and the two buttons that tick them in bulk.
 *
 * Built for the graded size ranges' "load existing from favorite" step, where a combo used to ask
 * for ONE favorite — a range was created per dialog, so loading five of them was five dialogs. The
 * question asked there is "which of these do I want", and that question has a shape of its own: a
 * list, ticked by default, read back once.
 *
 * It sits on @ref OExportRowListWidget for its frame only — the scrolling stack, the line of text
 * shown while it is empty, and the one copy of the macOS viewport workaround that frame carries.
 * Nothing above is Export-shaped: a name, a box, whether it may be ticked, and why not when it may
 * not.
 *
 * Every box starts ticked, like a list of favorites kept to be run: an unticked box means "not that
 * one today", and a list where nothing is ticked is a dialog that cannot be accepted.
 */
class OCheckableRowListWidget : public OExportRowListWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OCheckableRowListWidget(QWidget *parent = nullptr);
    ~OCheckableRowListWidget() override = default;

    /// @brief Show @p names, in the order given, every box ticked.
    void SetNames(const QStringList &names);

    /// @brief Lock those of @p names out of ticking, each saying @p reason beside its name.
    ///
    /// Called after SetNames(), on names it lists — an unknown name is ignored.
    void SetUnavailable(const QStringList &names, const QString &reason);

    /// @brief The names whose box is ticked, in the list's order — what the gesture applies to.
    auto CheckedNames() const -> QStringList;

    auto CheckedCount() const -> int;
    auto HasNames() const -> bool;

    /// @brief Tick every name that may be ticked.
    void SelectAll();

    void DeselectAll();

signals:
    /// @brief A box was ticked or unticked: what the gesture would apply to has changed.
    void CheckedChanged();

protected:
    void changeEvent(QEvent *event) override;

    /// @brief Keep the boxes in step with the busy flag and with what each row allows, and the
    ///        bulk buttons in step with what is ticked.
    void Refresh() override;

private:
    void Retranslate();
    void AppendRow(const QString &name);

    auto RowAt(const QString &name) const -> OCheckableRowWidget *;

    QPushButton *m_selectAll{nullptr};
    QPushButton *m_deselectAll{nullptr};
};

#endif // OCHECKABLEROWLISTWIDGET_H
