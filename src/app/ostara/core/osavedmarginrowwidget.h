/************************************************************************
 **
 **  @file   osavedmarginrowwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/08/2026
 **
 **  @brief  One saved custom seam-allowance row (name, value, color, delete)
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

#ifndef OSAVEDMARGINROWWIDGET_H
#define OSAVEDMARGINROWWIDGET_H

#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;

/**
 * @brief OSavedMarginRowWidget renders one saved custom seam-allowance entry as a
 *        single horizontal row (editable name, editable value, color combo, delete
 *        button). It is a dumb container — the dialog wires the persistence.
 */
class OSavedMarginRowWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OSavedMarginRowWidget(QWidget *parent = nullptr);
    ~OSavedMarginRowWidget() override = default;

    auto nameEdit() const -> QLineEdit *;
    auto valueEdit() const -> QLineEdit *;
    auto colorCombo() const -> QComboBox *;
    auto deleteButton() const -> QPushButton *;

private:
    void refreshDeleteIcon();

    QLineEdit *m_nameEdit{nullptr};
    QLineEdit *m_valueEdit{nullptr};
    QComboBox *m_colorCombo{nullptr};
    QPushButton *m_deleteButton{nullptr};

    Q_DISABLE_COPY_MOVE(OSavedMarginRowWidget) // NOLINT
};

#endif // OSAVEDMARGINROWWIDGET_H
