/************************************************************************
 **
 **  @file   oexportgroupchoicewidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of the "one piece per group" question.
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

#include "oexportgroupchoicewidget.h"

#include "../core/opiecegrouplayoutflags.h"
#include "../ogradedgeometry.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QComboBox>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
OExportGroupChoiceWidget::OExportGroupChoiceWidget(const VContainer *data, QWidget *parent)
  : QGroupBox(parent),
    m_data(data)
{
    Populate();

    // Les libellés sont écrits par Retranslate et nulle part ailleurs : un widget qui ne l'aurait
    // pas lancée s'ouvrirait sans titre ni noms de groupes.
    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGroupChoiceWidget::MemberChoices() const -> QMap<quint32, quint32>
{
    QMap<quint32, quint32> choices;
    for (const GroupRow &row : m_groupRows)
    {
        if (row.combo->currentIndex() >= 0)
        {
            choices.insert(row.groupId, row.combo->currentData().toUInt());
        }
    }
    return choices;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGroupChoiceWidget::HasGroups() const -> bool
{
    return !m_groupRows.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportGroupChoiceWidget::IsComplete() const -> bool
{
    for (const GroupRow &row : m_groupRows)
    {
        if (row.combo->currentIndex() < 0)
        {
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGroupChoiceWidget::Populate()
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

            // A group of fewer than two members is not a group: the canvas, the export and the
            // container all read it that way. There is nothing to choose between, so there is no
            // question to ask — its members are cut as ordinary pieces.
            if (members.size() < OGraded::kMinGroupMembers)
            {
                continue;
            }

            auto *row = new QHBoxLayout();

            auto *name = new QLabel(this);
            name->setText(group.GetName().isEmpty() ? tr("Unnamed group") : group.GetName());
            row->addWidget(name);

            auto *combo = MakeComboBox(this);
            for (quint32 pieceId : members)
            {
                const VPiece piece = m_data->GetPiece(pieceId);
                combo->addItem(piece.GetName().isEmpty() ? tr("Unnamed") : piece.GetName(), pieceId);
            }
            // Nothing is chosen to begin with: the user must say which piece she cuts, and a combo
            // that had already answered would let a plan run on a piece nobody picked.
            combo->setCurrentIndex(-1);
            connect(combo, &QComboBox::currentIndexChanged, this, [this](int) { emit Changed(); });
            row->addWidget(combo);

            row->addStretch();
            rows->addLayout(row);

            m_groupRows.append(GroupRow{groupId, name, combo});
        }
    }

    // The box says nothing when there is no group to ask about, and an empty box with a title
    // would be a question with no answer to give.
    setVisible(!m_groupRows.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGroupChoiceWidget::Retranslate()
{
    setTitle(tr("One piece per group"));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportGroupChoiceWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QGroupBox::changeEvent(event);
}
