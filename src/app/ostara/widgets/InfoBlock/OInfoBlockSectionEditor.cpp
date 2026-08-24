/************************************************************************
 **
 **  @file   OInfoBlockSectionEditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Data-driven edit zone for one template section.
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

#include "OInfoBlockSectionEditor.h"

#include "OPieceCartoucheCatalogue.h"

#include "vabstractapplication.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFontComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QStringList>
#include <QVBoxLayout>

OInfoBlockSectionEditor::OInfoBlockSectionEditor(QWidget *parent) : QWidget(parent)
{
    m_root = new QVBoxLayout(this);
    m_root->setContentsMargins(0, 0, 0, 0);
    Rebuild();
}

void OInfoBlockSectionEditor::SetSection(OPieceCartoucheTemplate *tpl, int index)
{
    m_template = tpl;
    m_index = index;
    Rebuild();
}

void OInfoBlockSectionEditor::Rebuild()
{
    m_useDefaultsCheck = nullptr;
    m_fontControls.clear();
    if (m_content != nullptr)
    {
        m_root->removeWidget(m_content);
        delete m_content;
        m_content = nullptr;
    }

    if (m_template == nullptr || m_index < 0 || m_index >= m_template->SectionCount())
    {
        auto *hint = new QLabel(tr("Select a section to edit its settings."), this);
        hint->setAlignment(Qt::AlignCenter);
        hint->setWordWrap(true);
        m_root->addWidget(hint);
        m_content = hint;
        return;
    }

    const auto &type = OPieceCartoucheCatalogue::Section(m_template->SectionTypeId(m_index));
    const auto &section = m_template->SectionAt(m_index);

    auto *content = new QWidget(this);
    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *description = new QLabel(type.Description(), content);
    description->setWordWrap(true);
    layout->addWidget(description);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    for (const OPieceCartoucheOptionField &field : type.Fields())
    {
        switch (field.kind)
        {
            case OPieceCartoucheOptionKind::UseDefaults:
            {
                auto *check = new QCheckBox(field.label, content);
                check->setChecked(section.options.value(field.key, true).toBool());
                connect(check, &QCheckBox::toggled, this,
                        [this, field](bool on) { m_template->SetSectionOption(m_index, field.key, on); emit OptionsChanged(); });
                form->addRow(QString(), check);
                m_useDefaultsCheck = check;
                break;
            }
            case OPieceCartoucheOptionKind::FontFamily:
            {
                auto *combo = new QFontComboBox(content);
                const QString family = section.options.value(field.key).toString();
                if (!family.isEmpty())
                {
                    combo->setCurrentFont(QFont(family));
                }
                connect(combo, &QFontComboBox::currentFontChanged, this, [this, field](const QFont &f) {
                    m_template->SetSectionOption(m_index, field.key, f.family());
                    emit OptionsChanged();
                });
                m_fontControls.append(combo);
                form->addRow(field.label, combo);
                break;
            }
            case OPieceCartoucheOptionKind::FontWeight:
            {
                struct WeightEntry
                {
                    int weight;
                    const char *name;
                };
                static const WeightEntry kWeights[] = {
                    {100, "Thin"},       {200, "Extra Light"}, {300, "Light"},   {400, "Normal"},
                    {500, "Medium"},     {600, "Demi Bold"},   {700, "Bold"},    {800, "Extra Bold"},
                    {900, "Black"},
                };
                auto *combo = new QComboBox(content);
                const int current = section.options.value(field.key, 0).toInt();
                combo->addItem(tr("Default"), 0);  // 0 = use default weight
                for (const WeightEntry &entry : kWeights)
                {
                    combo->addItem(tr(entry.name), entry.weight);
                    if (entry.weight == current)
                    {
                        combo->setCurrentIndex(combo->count() - 1);
                    }
                }
                connect(combo, &QComboBox::currentIndexChanged, this, [this, field, combo](int) {
                    m_template->SetSectionOption(m_index, field.key, combo->currentData().toInt());
                    emit OptionsChanged();
                });
                m_fontControls.append(combo);
                form->addRow(field.label, combo);
                break;
            }
            case OPieceCartoucheOptionKind::FontSize:
            {
                auto *spin = new QDoubleSpinBox(content);
                spin->setRange(0.0, 288.0);
                spin->setDecimals(1);
                spin->setSuffix(QStringLiteral(" pt"));
                spin->setSpecialValueText(tr("Default"));  // 0.0 = use default size
                spin->setValue(section.options.value(field.key, 0.0).toDouble());
                connect(spin, &QDoubleSpinBox::valueChanged, this, [this, field](double v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                m_fontControls.append(spin);
                form->addRow(field.label, spin);
                break;
            }
            case OPieceCartoucheOptionKind::Axis:
            {
                auto *combo = new QComboBox(content);
                combo->addItems({QStringLiteral("X"), QStringLiteral("Y"), QStringLiteral("Z")});
                combo->setCurrentText(section.options.value(field.key, QStringLiteral("X")).toString());
                connect(combo, &QComboBox::currentTextChanged, this, [this, field](const QString &v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                form->addRow(field.label, combo);
                break;
            }
            case OPieceCartoucheOptionKind::MeasureName:
            {
                auto *combo = new QComboBox(content);
                combo->setEditable(true);
                combo->setInsertPolicy(QComboBox::NoInsert);
                combo->setEditText(section.options.value(field.key).toString());
                connect(combo, &QComboBox::editTextChanged, this, [this, field](const QString &v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                form->addRow(field.label, combo);
                break;
            }
            case OPieceCartoucheOptionKind::Logo:
            {
                auto *combo = new QComboBox(content);
                const QString stored = section.options.value(field.key).toString();
                const QString logoPath = VAbstractApplication::VApp()->Settings()->GetLogoPath();

                QStringList candidates;
                if (!logoPath.isEmpty())
                {
                    candidates << logoPath;
                }
                if (!stored.isEmpty() && !candidates.contains(stored))
                {
                    candidates << stored;
                }

                if (candidates.isEmpty())
                {
                    combo->addItem(tr("No logo configured"));
                }
                else
                {
                    for (const QString &candidate : candidates)
                    {
                        combo->addItem(QFileInfo(candidate).completeBaseName(), candidate);
                    }
                    const int index = static_cast<int>(candidates.indexOf(stored));
                    combo->setCurrentIndex(index < 0 ? 0 : index);
                }

                connect(combo, &QComboBox::currentIndexChanged, this, [this, field, combo](int) {
                    m_template->SetSectionOption(m_index, field.key, combo->currentData().toString());
                    emit OptionsChanged();
                });

                // Keep the stored option in sync with the visible selection even
                // when the combo has a single item (no index change would fire).
                m_template->SetSectionOption(m_index, field.key, combo->currentData().toString());

                form->addRow(field.label, combo);
                break;
            }
            case OPieceCartoucheOptionKind::LogoWidth:
            case OPieceCartoucheOptionKind::LogoHeight:
            {
                auto *spin = new QDoubleSpinBox(content);
                spin->setRange(0.1, 200.0);
                spin->setDecimals(2);
                spin->setSuffix(QStringLiteral(" cm"));
                const double defaultSize = field.kind == OPieceCartoucheOptionKind::LogoWidth
                                               ? VAbstractApplication::VApp()->Settings()->GetLogoWidth()
                                               : VAbstractApplication::VApp()->Settings()->GetLogoHeight();
                spin->setValue(section.options.value(field.key, defaultSize).toDouble());
                connect(spin, &QDoubleSpinBox::valueChanged, this, [this, field](double v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                form->addRow(field.label, spin);
                break;
            }
        }
    }

    layout->addLayout(form);
    layout->addStretch();

    if (m_useDefaultsCheck != nullptr)
    {
        connect(m_useDefaultsCheck, &QCheckBox::toggled, this, [this](bool on) {
            for (QWidget *control : m_fontControls)
            {
                control->setEnabled(!on);
            }
        });
        const bool useDefaults = section.options.value(QStringLiteral("useDefaults"), true).toBool();
        for (QWidget *control : m_fontControls)
        {
            control->setEnabled(!useDefaults);
        }
    }

    m_root->addWidget(content);
    m_content = content;
}
