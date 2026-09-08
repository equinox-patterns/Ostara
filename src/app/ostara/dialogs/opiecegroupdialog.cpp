/************************************************************************
 **
 **  @file   opiecegroupdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/09/2026
 **
 **  @brief  Creation dialog for a piece group.
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

#include "opiecegroupdialog.h"

#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "../vpatterndb/vcontainer.h"
#include "opiecedialog_piecestep.h"

//---------------------------------------------------------------------------------------------------------------------
OPieceGroupDialog::OPieceGroupDialog(const VContainer *data, int memberCount, QWidget *parent)
  : QDialog(parent)
{
    setWindowTitle(tr("Link pieces"));

    auto *layout = new QVBoxLayout(this);
    m_step = new OPiecePieceStep(this);
    // The layout flags (Forbid/Force flipping, Symmetrical copy, Follow grainline) belong to a
    // standalone piece, not to a linked group — the shared step hides that block here.
    m_step->SetFlippingFlagsVisible(false);
    layout->addWidget(m_step);

    // Piece number total: the group replaces its memberCount standalone pieces by 1
    // (a group of K members no longer counts as K pieces, it counts as 1).
    m_step->SetPieceTotal(data != nullptr ? data->GetEffectivePieceCount() - memberCount + 1 : 0);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(m_buttonBox);

    auto *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(false);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_step, &OPiecePieceStep::NameChanged, this, &OPieceGroupDialog::UpdateButtons);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceGroupDialog::UpdateButtons(const QString &name)
{
    auto *okButton = m_buttonBox->button(QDialogButtonBox::Ok);
    okButton->setEnabled(!name.trimmed().isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceGroupDialog::GetGroup() const -> OPieceGroup
{
    OPieceGroup group;
    group.name = m_step->GetPieceName().trimmed();
    group.m_pieceNumber = m_step->GetPieceNumber();
    group.m_cutFabricQty = m_step->GetCutFabricQty();
    group.m_cutFabricOnFold = m_step->IsCutFabricOnFold();
    group.m_cutInterfacingQty = m_step->GetCutInterfacingQty();
    group.m_cutInterfacingOnFold = m_step->IsCutInterfacingOnFold();
    group.m_cutLiningQty = m_step->GetCutLiningQty();
    group.m_cutLiningOnFold = m_step->IsCutLiningOnFold();
    group.m_freeText = m_step->GetFreeText();
    group.alignH = m_step->GetGradedAlignH();
    group.alignV = m_step->GetGradedAlignV();
    return group;
}
