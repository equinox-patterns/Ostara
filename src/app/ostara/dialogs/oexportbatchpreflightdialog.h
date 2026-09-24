/************************************************************************
 **
 **  @file   oexportbatchpreflightdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  What the batch asks before it starts: which piece of each group a cutting plan
 **          keeps, and which members a plotter plan leaves out.
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

#ifndef OEXPORTBATCHPREFLIGHTDIALOG_H
#define OEXPORTBATCHPREFLIGHTDIALOG_H

#include <QDialog>
#include <QMap>
#include <QSet>

class OExportGroupChoiceWidget;
class OExportGroupMembersWidget;
class QDialogButtonBox;
class QLabel;
class QPushButton;
class VContainer;

/**
 * @brief The questions a batch asks once, for every favorite it is about to run.
 *
 * A favorite is a recipe and nothing more: it does not carry which piece of a group is cut, nor
 * which members the plotter keeps (see OExportFavorite). Those depend on the pieces of the
 * pattern it is run in — the very thing a favorite is meant to be portable across — so they are
 * asked here, and the answers are handed to every run of the batch.
 *
 * **Asked once, not once per run.** Five cutting-plan favorites are five plans, but the same
 * pattern: the same piece of a group is cut in all five. Asking five times would be asking the
 * user to say the same thing five times, and — worse — would let her say it differently.
 *
 * The batch's own lines in the Working list are NOT concerned: each was described in its own tab,
 * where its choices were already made, and the form is reset after every "Add to list" precisely
 * so that a second line can differ from the first. Overwriting them here would destroy a choice
 * nobody could see being destroyed.
 */
class OExportBatchPreflightDialog : public QDialog
{
    Q_OBJECT

public:
    /// @param cuttingPlan whether the batch holds at least one cutting-plan favorite. Its
    ///                    section is built only when it does — a question with no run behind it
    ///                    is a question the user should not have to answer.
    /// @param plotterPlan the same, for the plotter plans.
    explicit OExportBatchPreflightDialog(const VContainer *data, bool cuttingPlan, bool plotterPlan,
                                         QWidget *parent = nullptr);

    /// @brief Whether there is anything to decide at all.
    ///
    /// A pattern with no group has nothing to say to either section, and a dialog holding two
    /// empty boxes would be a step in the way. The caller asks this and skips the dialog
    /// entirely rather than opening one that closes itself.
    auto HasQuestions() const -> bool;

    /// @brief The member kept for each group, for every cutting plan of the batch.
    auto GroupChoices() const -> QMap<quint32, quint32>;

    /// @brief The pieces left out of every plotter plan of the batch. Empty means all of them.
    auto ExcludedPieces() const -> QSet<quint32>;

protected:
    void changeEvent(QEvent *event) override;

private:
    void Retranslate();

    /// @brief Whether the batch may start: every group must be answered, or a plan would run on a
    ///        piece nobody picked.
    void RefreshOk();

    OExportGroupChoiceWidget *m_groupChoice{nullptr};
    OExportGroupMembersWidget *m_groupMembers{nullptr};
    QLabel *m_intro{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};
    QPushButton *m_ok{nullptr};
};

#endif // OEXPORTBATCHPREFLIGHTDIALOG_H
