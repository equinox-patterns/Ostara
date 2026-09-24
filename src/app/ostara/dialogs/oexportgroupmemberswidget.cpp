/************************************************************************
 **
 **  @file   oexportgroupmemberswidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of the "group members to cut" question.
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

#include "oexportgroupmemberswidget.h"

#include "../core/opiecegrouplayoutflags.h"
#include "../ogradedgeometry.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QCheckBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
OExportGroupMembersWidget::OExportGroupMembersWidget(const VContainer *data, QWidget *parent)
  : QGroupBox(parent),
    m_data(data)
{
    Populate();
    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGroupMembersWidget::ExcludedPieces() const -> QSet<quint32>
{
    QSet<quint32> excluded;

    for (const GroupRow &row : m_groupRows)
    {
        for (const MemberRow &member : row.members)
        {
            if (!member.check->isChecked())
            {
                excluded.insert(member.pieceId);
            }
        }
    }

    return excluded;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGroupMembersWidget::HasGroups() const -> bool
{
    return !m_groupRows.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGroupMembersWidget::Populate()
{
    auto *rows = new QVBoxLayout(this);
    m_groupRows.clear();

    const QHash<quint32, OPieceGroup> *groups = m_data != nullptr ? m_data->DataPieceGroups() : nullptr;
    if (groups != nullptr)
    {
        QList<quint32> groupIds = groups->keys();
        std::sort(groupIds.begin(), groupIds.end());

        for (quint32 groupId : groupIds)
        {
            const OPieceGroup group = groups->value(groupId);
            const QVector<quint32> members = OPieceGroupLayoutFlags::ExistingMemberIds(group.pieceIds, m_data);

            // Un groupe de moins de deux membres n'est pas un groupe : le canvas, l'export et le
            // conteneur le lisent tous les trois comme ça. Il n'y a donc rien à écarter, et ses
            // membres se coupent comme des pièces ordinaires.
            if (members.size() < OGraded::kMinGroupMembers)
            {
                continue;
            }

            auto *name = new QLabel(this);
            name->setText(group.GetName().isEmpty() ? tr("Unnamed group") : group.GetName());
            rows->addWidget(name);

            // Les cases se lisent SOUS leur groupe. Sans ce retrait, la ligne de cases suivrait le
            // nom du groupe suivant et se lirait comme la sienne.
            auto *memberRow = new QHBoxLayout();
            memberRow->setContentsMargins(16, 0, 0, 0);

            GroupRow row{groupId, name, {}};

            for (quint32 pieceId : members)
            {
                const VPiece piece = m_data->GetPiece(pieceId);

                auto *check = new QCheckBox(this);
                check->setText(piece.GetName().isEmpty() ? tr("Unnamed") : piece.GetName());

                // **Cochée d'avance, et ce n'est pas un détail de confort.** Un plotter décompose les
                // groupes : toutes les cases cochées, le run coupe exactement ce qu'il a toujours
                // coupé. Une case décochée est une EXCEPTION, pas un choix à faire.
                check->setChecked(true);
                connect(check, &QCheckBox::toggled, this, [this](bool) { emit Changed(); });

                memberRow->addWidget(check);
                row.members.append(MemberRow{pieceId, check});
            }

            memberRow->addStretch();
            rows->addLayout(memberRow);

            m_groupRows.append(row);
        }
    }

    // Le cadre se taît quand il n'y a aucun groupe à montrer : un cadre vide avec un titre serait une
    // question sans réponse à donner.
    setVisible(!m_groupRows.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGroupMembersWidget::Retranslate()
{
    setTitle(tr("Group members to cut"));
    setToolTip(
        tr("A plotter cuts every member of a group, one by one: the group is decomposed, not printed "
           "as one shape.\n\n"
           "Untick a member to leave it out of this run. All ticked is what a plotter run has always "
           "cut."));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGroupMembersWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QGroupBox::changeEvent(event);
}
