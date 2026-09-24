/************************************************************************
 **
 **  @file   oexportgroupmemberswidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  "Group members to cut" — the question a plotter plan asks about the groups of a
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

#ifndef OEXPORTGROUPMEMBERSWIDGET_H
#define OEXPORTGROUPMEMBERSWIDGET_H

#include <QGroupBox>
#include <QSet>
#include <QVector>

class QCheckBox;
class QLabel;
class VContainer;

/**
 * @brief One ticked box per member of every group, saying which members the run keeps.
 *
 * A plotter DECOMPOSES groups: each member is a piece to cut, one by one. So there is nothing to
 * choose between — there are pieces to leave out. Hence a list of EXCLUSIONS
 * (`ExcludedPieces()`), where empty means "all of them".
 *
 * Every box is ticked from the start, and that is not a comfort: all ticked is what a plotter run
 * has always cut, and an unticked box is an exception rather than a choice to make.
 *
 * Extracted from the Plotter tab (24-09-2026) for the same reason as its twin: the batch
 * pre-flight dialog asks the same question once for every favorite of the batch, and the
 * population recipe (the filter through OPieceGroupLayoutFlags::ExistingMemberIds(), without which
 * GetPiece() throws on a deleted piece) is not one to keep in two places.
 */
class OExportGroupMembersWidget : public QGroupBox
{
    Q_OBJECT

public:
    explicit OExportGroupMembersWidget(const VContainer *data, QWidget *parent = nullptr);

    /// @brief The pieces the user UNTICKED: they are left out of the run. Empty means every one.
    auto ExcludedPieces() const -> QSet<quint32>;

    /// @brief Whether the pattern has a group worth showing (two living members or more).
    auto HasGroups() const -> bool;

signals:
    /// A box was ticked or unticked — anything built on these answers has to be re-read.
    void Changed();

protected:
    void changeEvent(QEvent *event) override;

private:
    /// Un membre d'un groupe et sa case : la case DÉCIDE, le libellé dit qui elle écarte.
    struct MemberRow
    {
        quint32 pieceId{0};
        QCheckBox *check{nullptr};
    };

    /// Une ligne du cadre : le nom du groupe, et une case par membre.
    struct GroupRow
    {
        quint32 groupId{0};
        QLabel *label{nullptr};
        QVector<MemberRow> members{};
    };

    void Populate();
    void Retranslate();

    const VContainer *m_data{nullptr};
    QVector<GroupRow> m_groupRows{};
};

#endif  // OEXPORTGROUPMEMBERSWIDGET_H
