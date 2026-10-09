/************************************************************************
 **
 **  @file   otransferlayersdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  Move layers of a group to another one — or out of every group,
 **          which is how a group is emptied.
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

#include "otransferlayersdialog.h"

#include <QDialogButtonBox>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>

#include "../vtools/dialogs/widgets/olayergroupchooser.h"

//---------------------------------------------------------------------------------------------------------------------
OTransferLayersDialog::OTransferLayersDialog(QWidget *parent)
  : QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Transfer layers"));
    setMinimumWidth(420);

    auto *layout = new QVBoxLayout(this);

    auto *intro = new QLabel(tr("Choose the layers to transfer, and the group they go to."), this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    m_layers = new QListWidget(this);
    m_layers->setSelectionMode(QAbstractItemView::NoSelection);
    m_layers->setMinimumHeight(140);
    layout->addWidget(m_layers, 1);

    layout->addWidget(new QLabel(tr("Move to:"), this));

    m_target = new OLayerGroupChooser(this);
    // A transfer moves layers into a group that exists: naming a new one belongs to the layer's own dialog and to
    // "Add a group", not here.
    m_target->SetCreatable(false);
    layout->addWidget(m_target);

    auto *buttonBox = new QDialogButtonBox(this);
    m_acceptButton = buttonBox->addButton(QDialogButtonBox::Ok);
    m_acceptButton->setAutoDefault(false);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    // Asserted last, once the whole box is in: the box lays its buttons out as they arrive and can hand the
    // default to the one added after the accept button.
    m_acceptButton->setDefault(true);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    connect(m_layers, &QListWidget::itemChanged, this, [this](QListWidgetItem *) { RefreshState(); });
    connect(m_target, &OLayerGroupChooser::Changed, this, [this]() { RefreshState(); });

    RefreshState();
}

//---------------------------------------------------------------------------------------------------------------------
OTransferLayersDialog::~OTransferLayersDialog() = default;

//---------------------------------------------------------------------------------------------------------------------
void OTransferLayersDialog::SetLayers(const QMap<quint32, QString> &layers)
{
    m_layers->clear();

    for (auto it = layers.constBegin(); it != layers.constEnd(); ++it)
    {
        auto *item = new QListWidgetItem(it.value(), m_layers);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        // Ticked: the whole group moves unless the user says otherwise.
        item->setCheckState(Qt::Checked);
        item->setData(Qt::UserRole, it.key());
    }

    RefreshState();
}

//---------------------------------------------------------------------------------------------------------------------
void OTransferLayersDialog::SetTargets(const QMap<quint32, QString> &groups)
{
    m_target->SetGroups(groups);
    RefreshState();
}

//---------------------------------------------------------------------------------------------------------------------
auto OTransferLayersDialog::SelectedLayers() const -> QVector<quint32>
{
    QVector<quint32> layers;

    for (int i = 0; i < m_layers->count(); ++i)
    {
        if (const QListWidgetItem *item = m_layers->item(i); item->checkState() == Qt::Checked)
        {
            layers.append(item->data(Qt::UserRole).toUInt());
        }
    }

    return layers;
}

//---------------------------------------------------------------------------------------------------------------------
auto OTransferLayersDialog::TargetGroup() const -> quint32
{
    return m_target->Group();
}

//---------------------------------------------------------------------------------------------------------------------
auto OTransferLayersDialog::IsValid() const -> bool
{
    return not SelectedLayers().isEmpty() && m_target->IsValid();
}

//---------------------------------------------------------------------------------------------------------------------
void OTransferLayersDialog::RefreshState()
{
    if (m_acceptButton != nullptr)
    {
        m_acceptButton->setEnabled(IsValid());
    }
}
