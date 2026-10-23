/************************************************************************
 **
 **  @file   ocheckablerowwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/10/2026
 **
 **  @brief  One line of a list of names to tick: a box, the name, and — when the row cannot be
 **          ticked — the reason, on the line itself.
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

#ifndef OCHECKABLEROWWIDGET_H
#define OCHECKABLEROWWIDGET_H

#include <QWidget>

class QCheckBox;
class QLabel;

/**
 * @brief One name of a checkable list: ticked, named, and saying why when it cannot be ticked.
 *
 * Nothing here is specific to what the list does with the ticks — the row only knows it is a name
 * with a box. That is the difference with the Export dialog's rows, which also carry where a run
 * stands and the star that drops a favorite: those are two questions about a run, and a list that
 * asks neither has no business showing the columns.
 *
 * **A row that cannot be ticked says so on itself.** The alternative — a box that refuses the click
 * without a word, or a message box once the dialog is gone — leaves the user looking at a control
 * that does not answer. The reason is therefore written next to the name, in the grey of a disabled
 * label, and the box starts unticked so that a bulk "select all" cannot re-tick it.
 */
class OCheckableRowWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OCheckableRowWidget(const QString &name, QWidget *parent = nullptr);
    ~OCheckableRowWidget() override = default;

    /// @brief The name this row stands for. Also the key it is read back by.
    auto Name() const -> QString;

    auto CheckBox() const -> QCheckBox *;

    /// @brief Tick or untick the box, whether the user may or not. Used to fill the list.
    void SetChecked(bool checked);

    /// @brief Lock the row out of ticking and write @p reason beside the name. Unticks it: a row
    ///        that cannot be chosen must not look chosen.
    void SetUnavailable(const QString &reason);

    /// @brief Whether the row may be ticked. False once SetUnavailable() said why it may not.
    auto IsAvailable() const -> bool;

private:
    QCheckBox *m_check{nullptr};
    QLabel *m_nameLabel{nullptr};
    QLabel *m_noteLabel{nullptr};
    bool m_available{true};

    Q_DISABLE_COPY_MOVE(OCheckableRowWidget) // NOLINT
};

#endif // OCHECKABLEROWWIDGET_H
