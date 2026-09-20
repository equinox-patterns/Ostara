/************************************************************************
 **
 **  @file   ocoversizerangedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  Choosing the size range a cover page's preview lists.
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

#include "ocoversizerangedialog.h"

#include "vmisc/FactoryWidgets.h"
#include "widgets/osizerangeaxis.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

OCoverSizeRangeDialog::OCoverSizeRangeDialog(const Config &config, QWidget *parent)
  : QDialog(parent), m_config(config), m_chosen(config.current)
{
    setWindowTitle(tr("Cover page — size range for the preview"));
    BuildUi();
}

void OCoverSizeRangeDialog::BuildUi()
{
    auto *layout = new QVBoxLayout(this);

    auto *hint = new QLabel(tr("The page will list these sizes while you design it. Which sizes "
                               "the printed page holds is decided when the export is run."),
                            this);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);

    m_axisCombo = MakeComboBox(this);
    m_axisCombo->addItems(m_config.axisNames);
    form->addRow(tr("Graded axis:"), m_axisCombo);
    connect(m_axisCombo, &QComboBox::currentTextChanged, this,
            [this](const QString &axis) { ApplyAxis(axis); });

    m_minCombo = MakeComboBox(this);
    form->addRow(tr("Minimum:"), m_minCombo);
    m_maxCombo = MakeComboBox(this);
    form->addRow(tr("Maximum:"), m_maxCombo);
    connect(m_minCombo, &QComboBox::currentTextChanged, this, [this](const QString &) {
        UpdateState();
    });
    connect(m_maxCombo, &QComboBox::currentTextChanged, this, [this](const QString &) {
        UpdateState();
    });

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    layout->addWidget(buttons);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Opening state: the range the information already holds when it has one, and the table's
    // declared axis when it does not — the axis the user graded along last time is the one they
    // mean again, and asking them to name it a second time would be asking a question they answered.
    m_lastAxis = m_chosen.axis;
    const QString opening = !m_chosen.axis.isEmpty() ? m_chosen.axis : m_config.declaredAxis;
    if (!opening.isEmpty())
    {
        const int index = m_axisCombo->findText(opening);
        if (index >= 0)
        {
            m_axisCombo->setCurrentIndex(index);
        }
    }
    ApplyAxis(CurrentAxis());

    if (m_chosen.HasRange() && m_chosen.axis == CurrentAxis())
    {
        const int minIndex = m_minCombo->findText(m_chosen.minValue);
        const int maxIndex = m_maxCombo->findText(m_chosen.maxValue);
        if (minIndex >= 0)
        {
            m_minCombo->setCurrentIndex(minIndex);
        }
        if (maxIndex >= 0)
        {
            m_maxCombo->setCurrentIndex(maxIndex);
        }
    }
    UpdateState();
}

auto OCoverSizeRangeDialog::CurrentAxis() const -> QString
{
    return m_axisCombo->currentText();
}

void OCoverSizeRangeDialog::ApplyAxis(const QString &axisName)
{
    // A new axis is a new set of values, and a range belongs to one axis. What was chosen for the
    // previous one cannot be carried over, so both ends open on the whole of the new axis — but
    // re-entering the axis the range was made against keeps it, names included, which is what the
    // dialog does when it opens on the range the information already holds.
    if (axisName != m_lastAxis)
    {
        m_chosen.names.clear();
    }
    m_lastAxis = axisName;

    OSizeRangeAxis::FillMinMax(m_minCombo, m_maxCombo, m_config.axisValues.value(axisName));
    UpdateState();
}

void OCoverSizeRangeDialog::UpdateState()
{
    // A range whose first size stands after its last holds nothing, so there is nothing to accept.
    if (m_okButton != nullptr)
    {
        m_okButton->setEnabled(OSizeRangeAxis::IsOrdered(m_minCombo, m_maxCombo));
    }
}

auto OCoverSizeRangeDialog::Chosen() const -> OCoverGradedAxis
{
    OCoverGradedAxis chosen = m_chosen;
    chosen.measurement = m_config.canonicalPath;
    chosen.axis = CurrentAxis();
    chosen.minValue = m_minCombo->currentText();
    chosen.maxValue = m_maxCombo->currentText();
    return chosen;
}
