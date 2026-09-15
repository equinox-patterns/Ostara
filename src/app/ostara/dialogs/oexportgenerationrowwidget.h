/************************************************************************
 **
 **  @file   oexportgenerationrowwidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  One line of the Export mode's list to generate: the name the
 **          run will be filed under, where it stands, and its delete button.
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

#ifndef OEXPORTGENERATIONROWWIDGET_H
#define OEXPORTGENERATIONROWWIDGET_H

#include <QWidget>

#include "../core/oexportrunqueue.h"

class QLabel;
class QPushButton;

/**
 * @brief One line of the list to generate, as a single horizontal row.
 *
 * A dumb container, like the seam-allowance rows the details dialog stacks: it reads
 * nothing and decides nothing. The queue holds what a line is, the tab that owns it walks
 * the batch — this only shows what it is told.
 *
 * The name is read-only. It was derived and reserved when the line was added, and the run
 * will be filed under exactly that: a line the user could rename here would be a name the
 * generation no longer agrees with.
 */
class OExportGenerationRowWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit OExportGenerationRowWidget(const QString &name, QWidget *parent = nullptr);
    ~OExportGenerationRowWidget() override = default;

    /// @brief Write where the run stands. The state is kept for one reason only — so that
    ///        a language change can write the same state in the other language.
    void SetState(OExportRunState state);

    auto DeleteButton() const -> QPushButton *;

protected:
    void changeEvent(QEvent *event) override;

private:
    static auto StateText(OExportRunState state) -> QString;

    void RefreshDeleteIcon();
    void Retranslate();

    QLabel *m_nameLabel{nullptr};
    QLabel *m_stateLabel{nullptr};
    QPushButton *m_deleteButton{nullptr};
    OExportRunState m_state{OExportRunState::Pending};

    Q_DISABLE_COPY_MOVE(OExportGenerationRowWidget) // NOLINT
};

#endif // OEXPORTGENERATIONROWWIDGET_H
