/************************************************************************
 **
 **  @file   ochooselayerdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28/09/2026
 **
 **  @brief  Choose one layer out of a list — the question object picking
 **          cannot ask by itself.
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

#include "ochooselayerdialog.h"

#include "../vmisc/FactoryWidgets.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
// The size the Layers dock draws its own icons at, so a dot here is the dot there.
constexpr int kIconSize = 20;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OChooseLayerDialog::OChooseLayerDialog(const Config &config, QWidget *parent)
  : QDialog(parent)
{
    setModal(true);
    setWindowTitle(tr("Choose a layer"));
    setMinimumWidth(360);

    auto *layout = new QVBoxLayout(this);

    if (not config.intro.isEmpty())
    {
        auto *intro = new QLabel(config.intro, this);
        intro->setWordWrap(true);
        layout->addWidget(intro);
    }

    // One value out of a handful, and a value the user reads before accepting: the line already shows the
    // layer that will be written, where a list would ask them to pick a row and then trust it. MakeComboBox,
    // because the native macOS popup ignores the stylesheet.
    m_combo = MakeComboBox(this);
    m_combo->setIconSize(QSize(kIconSize, kIconSize));

    int preselectedRow = -1;
    for (const Choice &choice : config.choices)
    {
        // The id rides the entry: the name is what the user reads, the id is what the caller writes.
        m_combo->addItem(choice.icon, choice.name, choice.id);
        if (choice.id == config.preselected)
        {
            preselectedRow = m_combo->count() - 1;
        }
    }

    if (preselectedRow >= 0)
    {
        m_combo->setCurrentIndex(preselectedRow);
    }

    layout->addWidget(m_combo);

    auto *buttonBox = new QDialogButtonBox(this);
    m_acceptButton = buttonBox->addButton(config.acceptText.isEmpty() ? tr("OK") : config.acceptText,
                                         QDialogButtonBox::AcceptRole);
    m_acceptButton->setAutoDefault(false);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    // Asserted last, once every button is in: the box lays its buttons out as they arrive and can hand the
    // default to the one added after the accept button. Return reaches it from the field — measured (probe).
    m_acceptButton->setDefault(true);
    // An empty field would leave the dialog with nothing to answer. A piece always has a layer, so this only
    // guards the impossible.
    m_acceptButton->setEnabled(m_combo->count() > 0);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(buttonBox);

    // The field is the answer, so it is what the keyboard is on.
    m_combo->setFocus();
}

//---------------------------------------------------------------------------------------------------------------------
auto OChooseLayerDialog::ChosenLayerId() const -> quint32
{
    return m_combo->currentIndex() < 0 ? NULL_ID : m_combo->currentData().toUInt();
}
