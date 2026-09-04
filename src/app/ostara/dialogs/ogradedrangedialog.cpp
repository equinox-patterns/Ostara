/************************************************************************
 **
 **  @file   ogradedrangedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  Graded-mode entry dialog (2 steps).
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

#include "ogradedrangedialog.h"

#include "../vmisc/FactoryWidgets.h"

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

//---------------------------------------------------------------------------------------------------------------------
OGradedRangeDialog::OGradedRangeDialog(const Config &config, QWidget *parent)
  : QDialog(parent)
  , m_config(config)
  , m_sizeAxis(config.declaredAxis)
{
    setWindowTitle(tr("Graded mode — size range"));

    auto *rootLayout = new QVBoxLayout(this);

    m_stack = new QStackedWidget(this);
    rootLayout->addWidget(m_stack);

    m_step1 = new QWidget(this);
    m_step2 = new QWidget(this);
    m_stack->addWidget(m_step1);
    m_stack->addWidget(m_step2);

    BuildStep1Page();

    // Buttons: Cancel / Next / OK
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);

    auto *cancelButton = new QPushButton(tr("Cancel"), this);
    m_nextButton = new QPushButton(tr("Next"), this);
    m_okButton = new QPushButton(tr("OK"), this);

    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(m_nextButton);
    buttonLayout->addWidget(m_okButton);
    rootLayout->addLayout(buttonLayout);

    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_nextButton, &QPushButton::clicked, this, &OGradedRangeDialog::GoToStep2);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);

    m_okButton->setEnabled(false);

    if (m_config.axisStepNeeded)
    {
        m_stack->setCurrentWidget(m_step1);
    }
    else
    {
        // Axis already declared — go straight to step 2.
        BuildStep2Page();
        m_stack->setCurrentWidget(m_step2);
        CheckState(); // leave OK disabled until name + min/max are valid
    }

    UpdateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::BuildStep1Page()
{
    auto *layout = new QVBoxLayout(m_step1);

    auto *label = new QLabel(tr("Which axis of the table is the size axis (the graded one)?"), m_step1);
    label->setWordWrap(true);
    layout->addWidget(label);

    m_axisCombo = MakeComboBox(m_step1);
    m_axisCombo->addItems(m_config.axisNames);
    if (m_axisCombo->count() > 0)
    {
        m_axisCombo->setCurrentIndex(0);
    }
    layout->addWidget(m_axisCombo);
    layout->addStretch(1);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::GoToStep2()
{
    if (m_axisCombo == nullptr || m_axisCombo->currentIndex() < 0)
    {
        return;
    }

    m_sizeAxis = m_axisCombo->currentText();
    BuildStep2Page();
    m_stack->setCurrentWidget(m_step2);
    UpdateButtons();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::BuildStep2Page()
{
    // Rebuild (idempotent: GoToStep2 may only run once, but keep it clean anyway).
    if (QLayout *oldLayout = m_step2->layout(); oldLayout != nullptr)
    {
        while (QLayoutItem *item = oldLayout->takeAt(0))
        {
            if (QWidget *w = item->widget(); w != nullptr)
            {
                w->deleteLater();
            }
            delete item;
        }
        delete oldLayout;
    }
    m_otherCombos.clear();

    auto *layout = new QFormLayout(m_step2);

    m_nameEdit = new QLineEdit(m_step2);
    m_nameEdit->setPlaceholderText(tr("Size range name"));
    layout->addRow(tr("Name:"), m_nameEdit);

    const QStringList values = m_config.axisValues.value(m_sizeAxis);

    m_minCombo = MakeComboBox(m_step2);
    m_minCombo->addItems(values);
    m_minCombo->setCurrentIndex(0);
    layout->addRow(tr("Minimum:"), m_minCombo);

    m_maxCombo = MakeComboBox(m_step2);
    m_maxCombo->addItems(values);
    m_maxCombo->setCurrentIndex(values.size() > 0 ? static_cast<int>(values.size() - 1) : 0);
    layout->addRow(tr("Maximum:"), m_maxCombo);

    // One row per other axis — dynamic label = the real axis name; default 1st value.
    for (const QString &axis : m_config.axisNames)
    {
        if (axis == m_sizeAxis)
        {
            continue;
        }

        QComboBox *combo = MakeComboBox(m_step2);
        combo->addItems(m_config.axisValues.value(axis));
        combo->setCurrentIndex(0);
        m_otherCombos.append({axis, combo});
        layout->addRow(axis + QLatin1Char(':'), combo);
    }

    connect(m_nameEdit, &QLineEdit::textChanged, this, &OGradedRangeDialog::CheckState);
    connect(m_minCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OGradedRangeDialog::CheckState);
    connect(m_maxCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OGradedRangeDialog::CheckState);
    for (const auto &pair : m_otherCombos)
    {
        connect(pair.second, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                &OGradedRangeDialog::CheckState);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::UpdateButtons()
{
    const bool onStep2 = m_stack->currentWidget() == m_step2;
    m_nextButton->setVisible(!onStep2);
    m_okButton->setVisible(onStep2);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::CheckState()
{
    if (m_nameEdit == nullptr || m_minCombo == nullptr || m_maxCombo == nullptr)
    {
        return;
    }

    const QString name = m_nameEdit->text().trimmed();
    const bool nameOk = !name.isEmpty() && !m_config.takenNames.contains(name);
    const bool rangeOk = m_minCombo->currentIndex() >= 0 && m_maxCombo->currentIndex() >= 0
                         && m_minCombo->currentIndex() <= m_maxCombo->currentIndex();
    m_okButton->setEnabled(nameOk && rangeOk);
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::sizeAxis() const -> QString
{
    return m_sizeAxis;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::range() const -> OSizerange
{
    OSizerange range;
    range.measurement = m_config.canonicalPath;
    range.sizeAxis = sizeAxis();

    if (m_nameEdit != nullptr)
    {
        range.name = m_nameEdit->text().trimmed();
    }
    if (m_minCombo != nullptr)
    {
        range.minValue = m_minCombo->currentText();
    }
    if (m_maxCombo != nullptr)
    {
        range.maxValue = m_maxCombo->currentText();
    }

    for (const auto &pair : m_otherCombos)
    {
        range.axisValues.insert(pair.first, pair.second->currentText());
    }

    return range;
}
