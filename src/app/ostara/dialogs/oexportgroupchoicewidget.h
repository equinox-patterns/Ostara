/************************************************************************
 **
 **  @file   oexportgroupchoicewidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  "One piece per group" — the question a cutting plan asks about the groups of a
 **          pattern, as a box that can be handed to more than one dialog.
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

#ifndef OEXPORTGROUPCHOICEWIDGET_H
#define OEXPORTGROUPCHOICEWIDGET_H

#include <QGroupBox>
#include <QMap>
#include <QVector>

class QComboBox;
class QLabel;
class VContainer;

/**
 * @brief One combo per group, asking which of its members is cut.
 *
 * A group serves printing, where two pieces fill one shape on a sheet. On fabric a piece is cut
 * once, so a group is decomposed and one of its members is kept.
 *
 * The box says nothing when the pattern has no group to ask about — an empty box with a title is
 * a question with no answer to give.
 *
 * Extracted from the Cutting plan tab (24-09-2026) because a second caller appeared: the batch
 * pre-flight dialog, which asks the same question once for every favorite of the batch. The
 * population recipe is not obvious — it filters the group members through
 * OPieceGroupLayoutFlags::ExistingMemberIds(), which is mandatory since `pieceIds` is not cleaned
 * up when a piece is deleted and `VContainer::GetPiece()` throws on an unknown id — and two
 * copies of it would be two places to get that wrong.
 */
class OExportGroupChoiceWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit OExportGroupChoiceWidget(const VContainer *data, QWidget *parent = nullptr);

    /// @brief The member kept for each group: group id → piece id. Empty when the pattern has none.
    auto MemberChoices() const -> QMap<quint32, quint32>;

    /// @brief Whether the pattern has a group worth asking about (two living members or more).
    auto HasGroups() const -> bool;

    /// @brief Whether every group has been answered. A group left unanswered is a piece the plan
    ///        would have to pick for the user, so a run is not described until this is true.
    auto IsComplete() const -> bool;

signals:
    /// A choice changed — anything built on these answers has to be re-read.
    void Changed();

protected:
    void changeEvent(QEvent *event) override;

private:
    /// One row of the box: the group's name, and the members to choose from.
    struct GroupRow
    {
        quint32 groupId{0};
        QLabel *label{nullptr};
        QComboBox *combo{nullptr};
    };

    void Populate();
    void Retranslate();

    const VContainer *m_data{nullptr};
    QVector<GroupRow> m_groupRows{};
};

#endif  // OEXPORTGROUPCHOICEWIDGET_H
