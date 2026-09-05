/************************************************************************
 **
 **  @file   ogradedrangedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  Graded-mode size-range dialog (3 steps, favorites aware).
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
#include <QRadioButton>
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
    m_modePage = new QWidget(this);
    m_step2 = new QWidget(this);
    m_stack->addWidget(m_step1);   // page 0 — axis (only when needed)
    m_stack->addWidget(m_modePage); // page 1 — create / load from favorite
    m_stack->addWidget(m_step2);   // page 2 — range form

    BuildStep1Page();
    BuildModePage();

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
    connect(m_nextButton, &QPushButton::clicked, this, &OGradedRangeDialog::OnNext);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);

    m_okButton->setEnabled(false);

    if (m_config.editing)
    {
        // Edit: open straight on the range form, pre-filled (no axis, no favorite).
        BuildStep2Page();
        m_stack->setCurrentWidget(m_step2);
        CheckState();
        LoadExistingRange();
    }
    else if (m_config.axisStepNeeded)
    {
        m_stack->setCurrentWidget(m_step1); // step 1 first (axis still unknown)
    }
    else
    {
        // Axis already known — ask create vs favorite.
        ShowModePage();
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
void OGradedRangeDialog::BuildModePage()
{
    auto *layout = new QVBoxLayout(m_modePage);

    m_newRadio = new QRadioButton(tr("Create a new size range"), m_modePage);
    m_loadRadio = new QRadioButton(tr("Load existing from favorite"), m_modePage);
    m_newRadio->setChecked(true);

    auto *favoriteLabel = new QLabel(tr("Favorite:"), m_modePage);
    m_favoriteCombo = MakeComboBox(m_modePage);
    m_favoriteCombo->setEnabled(false);

    layout->addWidget(m_newRadio);
    layout->addWidget(m_loadRadio);
    layout->addSpacing(6);
    layout->addWidget(favoriteLabel);
    layout->addWidget(m_favoriteCombo);
    layout->addStretch(1);

    auto updateMode = [this]() {
        const bool load = m_loadRadio != nullptr && m_loadRadio->isChecked();
        if (m_favoriteCombo != nullptr)
        {
            m_favoriteCombo->setEnabled(load);
        }
        UpdateModeState();
    };
    connect(m_newRadio, &QRadioButton::toggled, this, updateMode);
    connect(m_loadRadio, &QRadioButton::toggled, this, updateMode);
    connect(m_favoriteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateMode);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::ShowModePage()
{
    m_favoriteCombo->clear();
    if (m_config.favoriteNames)
    {
        m_favoriteCombo->addItems(m_config.favoriteNames(m_sizeAxis));
    }

    const bool hasFavorites = m_favoriteCombo->count() > 0;
    m_loadRadio->setEnabled(hasFavorites);
    // Default: load a favorite when at least one exists, otherwise create new.
    if (hasFavorites)
    {
        m_loadRadio->setChecked(true);
    }
    else
    {
        m_newRadio->setChecked(true);
    }

    UpdateModeState();
    m_stack->setCurrentWidget(m_modePage);
    UpdateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::UpdateModeState()
{
    if (m_stack == nullptr || m_stack->currentWidget() != m_modePage)
    {
        return;
    }

    const bool load = m_loadRadio != nullptr && m_loadRadio->isChecked();
    const bool loadOk = load && m_favoriteCombo != nullptr && m_favoriteCombo->currentIndex() >= 0;
    const bool createOk = m_newRadio != nullptr && m_newRadio->isChecked();
    m_nextButton->setEnabled(createOk || loadOk);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::OnNext()
{
    if (m_stack->currentWidget() == m_step1)
    {
        // Axis chosen → move to create/load.
        if (m_axisCombo != nullptr)
        {
            m_sizeAxis = m_axisCombo->currentText();
        }
        ShowModePage();
        return;
    }

    if (m_stack->currentWidget() == m_modePage)
    {
        if (m_loadRadio != nullptr && m_loadRadio->isChecked()
            && m_favoriteCombo != nullptr && m_favoriteCombo->currentIndex() >= 0)
        {
            // Load from favorite: build directly from the stored definition.
            m_favoriteName = m_favoriteCombo->currentText();
            m_loadedFromFavorite = true;
            accept();
        }
        else
        {
            GoToStep3(); // create a new range → range form
        }
        return;
    }

    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::GoToStep3()
{
    BuildStep2Page();
    m_stack->setCurrentWidget(m_step2);
    UpdateButtons();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::BuildStep2Page()
{
    // Rebuild (idempotent: navigation may run more than once).
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
    const bool onStep3 = m_stack->currentWidget() == m_step2;
    m_nextButton->setVisible(!onStep3);
    m_okButton->setVisible(onStep3);
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
void OGradedRangeDialog::LoadExistingRange()
{
    if (not m_config.editing)
    {
        return;
    }

    const OSizerange &range = m_config.initial;

    if (m_nameEdit != nullptr)
    {
        m_nameEdit->setText(range.name);
    }

    const QStringList values = m_config.axisValues.value(m_sizeAxis);
    if (m_minCombo != nullptr)
    {
        const int idx = static_cast<int>(values.indexOf(range.minValue));
        if (idx >= 0)
        {
            m_minCombo->setCurrentIndex(idx);
        }
    }
    if (m_maxCombo != nullptr)
    {
        const int idx = static_cast<int>(values.indexOf(range.maxValue));
        if (idx >= 0)
        {
            m_maxCombo->setCurrentIndex(idx);
        }
    }

    for (const auto &pair : m_otherCombos)
    {
        const QString current = range.axisValues.value(pair.first);
        if (not current.isEmpty())
        {
            const int idx = pair.second->findText(current);
            if (idx >= 0)
            {
                pair.second->setCurrentIndex(idx);
            }
        }
    }

    CheckState();
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

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::LoadedFromFavorite() const -> bool
{
    return m_loadedFromFavorite;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::FavoriteName() const -> QString
{
    return m_favoriteName;
}
