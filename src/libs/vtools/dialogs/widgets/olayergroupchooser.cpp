/************************************************************************
 **
 **  @file   olayergroupchooser.cpp
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

#include "olayergroupchooser.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QVector>

#include <algorithm>

#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/typedef.h"

//---------------------------------------------------------------------------------------------------------------------
OLayerGroupChooser::OLayerGroupChooser(QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    // MakeComboBox, like every combo Ostara builds in code: the native macOS popup ignores the stylesheet.
    m_combo = MakeComboBox(this);
    m_combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    layout->addWidget(m_combo, 1);

    m_newGroupName = new QLineEdit(this);
    m_newGroupName->setPlaceholderText(tr("Group name"));
    m_newGroupName->setVisible(false);
    layout->addWidget(m_newGroupName, 1);

    connect(m_combo, &QComboBox::currentIndexChanged, this,
            [this](int)
            {
                ShowNameField();
                emit Changed();
            });
    connect(m_newGroupName, &QLineEdit::textChanged, this, [this](const QString &) { emit Changed(); });

    FillCombo();
    ShowNameField();
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupChooser::SetGroups(const QMap<quint32, QString> &groups)
{
    m_groups = groups;
    FillCombo();
    ShowNameField();
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupChooser::SetCreatable(bool creatable)
{
    if (m_creatable == creatable)
    {
        return;
    }

    m_creatable = creatable;
    FillCombo();
    ShowNameField();
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupChooser::SetGroup(quint32 id)
{
    const QSignalBlocker blocker(m_combo);

    const int row = m_combo->findData(id);
    m_combo->setCurrentIndex(row >= 0 ? row : 0);

    ShowNameField();
}

//---------------------------------------------------------------------------------------------------------------------
auto OLayerGroupChooser::Group() const -> quint32
{
    if (m_combo->currentIndex() < 0)
    {
        return NULL_ID;
    }

    const quint32 id = m_combo->currentData().toUInt();

    if (id != kCreateGroup)
    {
        return id;
    }

    // The typed name is what decides: it is a NEW group only when no group of the piece already wears it. Two
    // groups the same name would be two rows the reader cannot tell apart, and the intent is unambiguous.
    const QString name = NewGroupName();
    for (auto it = m_groups.constBegin(); it != m_groups.constEnd(); ++it)
    {
        if (QString::compare(it.value(), name, Qt::CaseInsensitive) == 0)
        {
            return it.key();
        }
    }

    return kCreateGroup;
}

//---------------------------------------------------------------------------------------------------------------------
auto OLayerGroupChooser::NewGroupName() const -> QString
{
    return m_newGroupName->text().simplified();
}

//---------------------------------------------------------------------------------------------------------------------
auto OLayerGroupChooser::IsValid() const -> bool
{
    // Group() answers kCreateGroup only for a name that is filled in and belongs to no group yet.
    return Group() != kCreateGroup;
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupChooser::FillCombo()
{
    const QSignalBlocker blocker(m_combo);

    // What the reader had chosen, so a refresh of the list does not move the answer to another group.
    const quint32 previous = m_combo->currentIndex() < 0 ? NULL_ID : m_combo->currentData().toUInt();

    m_combo->clear();
    m_combo->addItem(tr("No group"), NULL_ID);

    // By name: the dock lists the groups that way, and a list read twice has to be the same list.
    QVector<QPair<QString, quint32>> sorted;
    sorted.reserve(m_groups.size());
    for (auto it = m_groups.constBegin(); it != m_groups.constEnd(); ++it)
    {
        sorted.append({it.value(), it.key()});
    }

    std::sort(sorted.begin(), sorted.end(),
              [](const QPair<QString, quint32> &left, const QPair<QString, quint32> &right)
              { return QString::compare(left.first, right.first, Qt::CaseInsensitive) < 0; });

    for (const auto &[name, id] : sorted)
    {
        m_combo->addItem(name, id);
    }

    if (m_creatable)
    {
        m_combo->addItem(tr("New group…"), kCreateGroup);
    }

    const int row = m_combo->findData(previous);
    m_combo->setCurrentIndex(row >= 0 ? row : 0);
}

//---------------------------------------------------------------------------------------------------------------------
void OLayerGroupChooser::ShowNameField()
{
    const bool creating = m_combo->currentIndex() >= 0 && m_combo->currentData().toUInt() == kCreateGroup;
    m_newGroupName->setVisible(creating);

    if (creating)
    {
        m_newGroupName->setFocus();
    }
}
