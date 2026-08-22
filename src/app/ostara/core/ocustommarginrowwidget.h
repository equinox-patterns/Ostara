/************************************************************************
 **
 **  @file   ocustommarginrowwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/08/2026
 **
 **  @brief  One custom seam-allowance margin row (name, value, color, show, delete)
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

#ifndef OCUSTOMMARGINROWWIDGET_H
#define OCUSTOMMARGINROWWIDGET_H

#include <QColor>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLineEdit;
class QPushButton;

/**
 * @brief OCustomMarginRowWidget renders one custom seam-allowance margin group as a
 *        single horizontal row of controls. It is a dumb container — it owns its
 *        child widgets and the group's node IDs, but the parent wires the signals
 *        and performs the backend mutations.
 */
class OCustomMarginRowWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OCustomMarginRowWidget(const QVector<quint32> &nodeIds, const QVector<QColor> &colors,
                                    QWidget *parent = nullptr);
    ~OCustomMarginRowWidget() override = default;

    auto nameEdit() const -> QLineEdit *;
    auto valueEdit() const -> QLineEdit *;
    auto colorCombo() const -> QComboBox *;
    auto showCheck() const -> QCheckBox *;
    auto deleteButton() const -> QPushButton *;
    auto nodeIds() const -> QVector<quint32>;

private:
    QVector<quint32> m_nodeIds;

    QLineEdit *m_nameEdit{nullptr};
    QLineEdit *m_valueEdit{nullptr};
    QComboBox *m_colorCombo{nullptr};
    QCheckBox *m_showCheck{nullptr};
    QPushButton *m_deleteButton{nullptr};

    void refreshDeleteIcon();

    Q_DISABLE_COPY_MOVE(OCustomMarginRowWidget) // NOLINT
};

#endif // OCUSTOMMARGINROWWIDGET_H
