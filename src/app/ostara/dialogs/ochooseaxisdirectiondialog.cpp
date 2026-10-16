/************************************************************************
 **
 **  @file   ochooseaxisdirectiondialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The four directions a curve handle can take from its axis, as a choice.
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

#include "ochooseaxisdirectiondialog.h"

#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
auto CurveAxisSenseText(CurveAxisSense sense) -> QString
{
    switch (sense)
    {
        case CurveAxisSense::Line:
            return QCoreApplication::translate("CurveAxisSense", "Angle of the line");
        case CurveAxisSense::LineReverse:
            return QCoreApplication::translate("CurveAxisSense", "Reverse angle of the line");
        case CurveAxisSense::Perpendicular:
            return QCoreApplication::translate("CurveAxisSense", "Perpendicular to the line");
        case CurveAxisSense::PerpendicularReverse:
            return QCoreApplication::translate("CurveAxisSense", "Reverse perpendicular to the line");
    }
    return QCoreApplication::translate("CurveAxisSense", "Angle of the line");
}

//---------------------------------------------------------------------------------------------------------------------
OChooseAxisDirectionDialog::OChooseAxisDirectionDialog(const Config &config, QWidget *parent)
  : QDialog(parent),
    m_selected(config.selected)
{
    setModal(true);
    setWindowTitle(tr("Direction of the axis"));
    setMinimumWidth(360);

    auto *layout = new QVBoxLayout(this);

    if (not config.intro.isEmpty())
    {
        auto *intro = new QLabel(config.intro, this);
        intro->setWordWrap(true);
        layout->addWidget(intro);
    }

    // The four directions, and only four: turning the handle round and reading the axis the other way are the
    // same half-turn, so the two questions are one (see AngleParAxe.md).
    auto *group = new QButtonGroup(this);
    const QVector<CurveAxisSense> senses{CurveAxisSense::Line, CurveAxisSense::LineReverse,
                                         CurveAxisSense::Perpendicular, CurveAxisSense::PerpendicularReverse};

    for (const CurveAxisSense sense : senses)
    {
        auto *button = new QRadioButton(CurveAxisSenseText(sense), this);
        button->setChecked(sense == m_selected);
        layout->addWidget(button);
        // The id is the value of the sense: the group answers by the direction itself, so no table to keep.
        group->addButton(button, static_cast<int>(sense));
        m_buttons.append(button);
    }

    connect(group, &QButtonGroup::idClicked, this,
            [this](int id)
            {
                // idClicked fires on every press, even on the button already checked — and the caller redraws
                // the curve on each signal. Re-checking the direction in force changes nothing: say nothing.
                if (const auto sense = static_cast<CurveAxisSense>(id); sense != m_selected)
                {
                    m_selected = sense;
                    emit SenseChanged(m_selected);
                }
            });

    auto *buttonBox = new QDialogButtonBox(this);
    QPushButton *acceptButton
        = buttonBox->addButton(config.acceptText.isEmpty() ? tr("OK") : config.acceptText, QDialogButtonBox::AcceptRole);
    acceptButton->setAutoDefault(false);
    buttonBox->addButton(QDialogButtonBox::Cancel);
    // Set the default button only once every button is in the box: the box hands the default to the last one
    // added, whichever it is.
    acceptButton->setDefault(true);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttonBox);

    for (auto *button : std::as_const(m_buttons))
    {
        if (button->isChecked())
        {
            button->setFocus();
            break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OChooseAxisDirectionDialog::ChosenSense() const -> CurveAxisSense
{
    return m_selected;
}
