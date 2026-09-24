/************************************************************************
 **
 **  @file   oexportbatchpreflightdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of the questions a batch asks before it starts.
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

#include "oexportbatchpreflightdialog.h"

#include "oexportgroupchoicewidget.h"
#include "oexportgroupmemberswidget.h"

#include <QDialogButtonBox>
#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OExportBatchPreflightDialog::OExportBatchPreflightDialog(const VContainer *data, bool cuttingPlan, bool plotterPlan,
                                                         QWidget *parent)
  : QDialog(parent)
{
    setModal(true);

    auto *layout = new QVBoxLayout(this);

    m_intro = new QLabel(this);
    m_intro->setWordWrap(true);
    layout->addWidget(m_intro);

    if (cuttingPlan)
    {
        m_groupChoice = new OExportGroupChoiceWidget(data, this);
        connect(m_groupChoice, &OExportGroupChoiceWidget::Changed, this, &OExportBatchPreflightDialog::RefreshOk);
        layout->addWidget(m_groupChoice);
    }

    if (plotterPlan)
    {
        m_groupMembers = new OExportGroupMembersWidget(data, this);
        layout->addWidget(m_groupMembers);
    }

    layout->addStretch();

    m_buttonBox = new QDialogButtonBox(this);
    m_ok = m_buttonBox->addButton(tr("Generate"), QDialogButtonBox::AcceptRole);
    m_ok->setAutoDefault(false);
    m_buttonBox->addButton(QDialogButtonBox::Cancel);
    // Posé en dernier, une fois tous les boutons dans la boîte : elle dispose ses boutons dans
    // l'ordre où ils arrivent, et le défaut irait sinon au dernier venu — Retour annulerait le lot.
    m_ok->setDefault(true);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttonBox);

    Retranslate();
    RefreshOk();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchPreflightDialog::HasQuestions() const -> bool
{
    const bool cutting = m_groupChoice != nullptr && m_groupChoice->HasGroups();
    const bool plotter = m_groupMembers != nullptr && m_groupMembers->HasGroups();
    return cutting || plotter;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchPreflightDialog::GroupChoices() const -> QMap<quint32, quint32>
{
    return m_groupChoice != nullptr ? m_groupChoice->MemberChoices() : QMap<quint32, quint32>();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportBatchPreflightDialog::ExcludedPieces() const -> QSet<quint32>
{
    return m_groupMembers != nullptr ? m_groupMembers->ExcludedPieces() : QSet<quint32>();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchPreflightDialog::RefreshOk()
{
    if (m_ok == nullptr)
    {
        return;
    }

    // Sans section de coupe, il n'y a rien à compléter : la section plotter part de « tout coché »,
    // qui est déjà une réponse.
    m_ok->setEnabled(m_groupChoice == nullptr || m_groupChoice->IsComplete());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchPreflightDialog::Retranslate()
{
    setWindowTitle(tr("Before generating"));
    m_intro->setText(tr("These choices apply to every favorite of this batch."));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportBatchPreflightDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QDialog::changeEvent(event);
}
