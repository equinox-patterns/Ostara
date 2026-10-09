/************************************************************************
 **
 **  @file   olayergroupchooser.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  One row answering "which group of layers?" — an existing one,
 **          none at all, or a new one the user names on the spot.
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
#ifndef OLAYERGROUPCHOOSER_H
#define OLAYERGROUPCHOOSER_H

#include <QMap>
#include <QString>
#include <QWidget>
#include <QtGlobal>

#include "../ifc/xml/vabstractpattern.h"

class QComboBox;
class QLineEdit;

/// The group of layers a layer belongs to, asked as ONE answer: "no group", one of the groups that exist, or a new
/// one the user names in the field that appears beside the list. The two sentinels travel in the combo's own data,
/// so no caller has to know a row index — and a typed name that is already a group's settles on that group rather
/// than creating a second one the reader could not tell apart.
class OLayerGroupChooser final : public QWidget
{
    Q_OBJECT // NOLINT

public:
    /// The "New group…" entry, shared with the model: the code that writes the answer is what creates the group.
    static constexpr quint32 kCreateGroup = VAbstractPattern::NewLayerGroupId;

    explicit OLayerGroupChooser(QWidget *parent = nullptr);
    ~OLayerGroupChooser() override = default;

    /// The groups that can be joined, id -> name. Rebuilds the list; the answer is kept when the group it names is
    /// still there, so a refresh never moves a layer on its own.
    void SetGroups(const QMap<quint32, QString> &groups);

    /// Whether the list offers "New group…". A transfer moves layers into a group that exists, so it does not.
    void SetCreatable(bool creatable);

    void SetGroup(quint32 id);
    /// NULL_ID for "no group", an existing id, or kCreateGroup when the user is naming a group that does not exist
    /// yet — use NewGroupName() for its name.
    auto Group() const -> quint32;
    auto NewGroupName() const -> QString;
    /// A group is always named: an empty name is not an answer.
    auto IsValid() const -> bool;

signals:
    void Changed();

private:
    Q_DISABLE_COPY_MOVE(OLayerGroupChooser) // NOLINT

    QComboBox *m_combo{nullptr};
    QLineEdit *m_newGroupName{nullptr};
    QMap<quint32, QString> m_groups;
    bool m_creatable{true};

    void FillCombo();
    void ShowNameField();
};

#endif // OLAYERGROUPCHOOSER_H
