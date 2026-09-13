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

#include "OAxisRenameEditor.h"
#include "OPieceCartoucheCatalogue.h"
#include "OPieceCartoucheStorage.h"
#include "OPieceCartoucheTable.h"

#include "vmisc/FactoryWidgets.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>
#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>

OInfoBlockSectionEditor::OInfoBlockSectionEditor(QWidget *parent) : QWidget(parent)
{
    m_root = new QVBoxLayout(this);
    m_root->setContentsMargins(0, 0, 0, 0);
    Rebuild();
}

void OInfoBlockSectionEditor::SetSection(OPieceCartoucheTemplate *tpl, int index, const OPieceCartoucheTable *table)
{
    m_template = tpl;
    m_table = table;
    m_index = index;
    Rebuild();
}

void OInfoBlockSectionEditor::RetranslateUI()
{
    // The whole edit zone is data-driven — rebuild it so every tr() literal and
    // catalogue label is re-resolved in the new language.
    Rebuild();
}

void OInfoBlockSectionEditor::Rebuild()
{
    if (m_content != nullptr)
    {
        m_root->removeWidget(m_content);
        delete m_content;
        m_content = nullptr;
    }
    m_axisRenameEditor = nullptr;

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
                auto *combo = MakeComboBox(content);
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
                form->addRow(field.label, combo);
                break;
            }
            case OPieceCartoucheOptionKind::FontSize:
            {
                auto *spin = new QSpinBox(content);
                spin->setRange(0, 288);
                spin->setSuffix(QStringLiteral(" px"));
                spin->setSpecialValueText(tr("Default"));  // 0 = use default size
                spin->setValue(section.options.value(field.key, 0.0).toInt());
                connect(spin, &QSpinBox::valueChanged, this, [this, field](int v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                form->addRow(field.label, spin);
                break;
            }
            case OPieceCartoucheOptionKind::LineSpacing:
            {
                auto *spin = new QSpinBox(content);
                spin->setRange(-100, 100);
                spin->setSuffix(QStringLiteral(" px"));
                spin->setToolTip(tr("Extra vertical space between this line and the next one. "
                                    "0 = default spacing, negative = tighter, positive = more space."));
                spin->setValue(section.options.value(field.key, 0).toInt());
                connect(spin, &QSpinBox::valueChanged, this, [this, field](int v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                form->addRow(field.label, spin);
                break;
            }
            case OPieceCartoucheOptionKind::Axis:
            {
                auto *combo = MakeComboBox(content);
                const QString stored = section.options.value(field.key).toString();
                const QStringList axes = m_table != nullptr ? m_table->AxisNames() : QStringList();
                if (axes.isEmpty())
                {
                    combo->setEnabled(false);  // no multi-size table loaded
                }
                else
                {
                    combo->addItems(axes);
                    if (!stored.isEmpty() && axes.contains(stored))
                    {
                        combo->setCurrentText(stored);
                    }
                    else
                    {
                        combo->setCurrentIndex(-1);  // stored axis not in the current table
                    }
                }
                connect(combo, &QComboBox::currentTextChanged, this, [this, field](const QString &v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    // The axis changed — its per-language aliases no longer apply.
                    m_template->SetSectionOption(m_index, OPieceCartoucheAxisRename::kRenames, QVariantMap{});
                    emit OptionsChanged();
                    UpdateAxisRenameEditor();
                });
                form->addRow(field.label, combo);
                break;
            }
            case OPieceCartoucheOptionKind::MeasureName:
            {
                auto *combo = MakeComboBox(content);
                const QString stored = section.options.value(field.key).toString();
                QStringList fullNames;
                if (m_table != nullptr)
                {
                    const QStringList names = m_table->MeasurementNames();
                    fullNames.reserve(names.size());
                    for (const QString &name : names)
                    {
                        const QString full = m_table->MeasurementFullName(name);
                        if (!full.isEmpty())
                        {
                            fullNames.append(full);
                        }
                    }
                }
                if (fullNames.isEmpty())
                {
                    combo->setEnabled(false);  // no measurement table loaded
                }
                else
                {
                    combo->addItems(fullNames);
                    if (!stored.isEmpty() && fullNames.contains(stored))
                    {
                        combo->setCurrentText(stored);
                    }
                    else
                    {
                        combo->setCurrentIndex(-1);  // stored measurement not in the current table
                    }
                }
                connect(combo, &QComboBox::currentTextChanged, this, [this, field](const QString &v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    // The measurement changed — its per-language alias no longer applies.
                    m_template->SetSectionOption(m_index, OPieceCartoucheAxisRename::kRenames, QVariantMap{});
                    emit OptionsChanged();
                    UpdateAxisRenameEditor();
                });
                form->addRow(field.label, combo);
                break;
            }
            case OPieceCartoucheOptionKind::Separator:
            {
                struct SeparatorEntry
                {
                    QString label;
                    QString value;
                };
                QList<SeparatorEntry> entries;
                if (field.values.isEmpty())
                {
                    // Default set: no separator, then the common punctuation.
                    entries = {{tr("None"), QString()},
                               {QStringLiteral(":"), QStringLiteral(":")},
                               {QStringLiteral("-"), QStringLiteral("-")},
                               {QStringLiteral("/"), QStringLiteral("/")},
                               {QStringLiteral("·"), QStringLiteral("·")}};
                }
                else
                {
                    for (const QString &value : field.values)
                    {
                        // Word separators ("of") are translated; punctuation stays as-is.
                        const QString label = value == QLatin1String("of") ? tr("of") : value;
                        entries.append({label, value});
                    }
                }

                auto *row = new QWidget(content);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                auto *group = new QButtonGroup(row);
                const QString current = section.options.value(field.key).toString();
                for (const SeparatorEntry &entry : entries)
                {
                    auto *radio = new QRadioButton(entry.label, row);
                    radio->setObjectName(QStringLiteral("separatorRadio"));
                    radio->setChecked(entry.value == current);
                    group->addButton(radio);
                    rowLayout->addWidget(radio);
                    connect(radio, &QRadioButton::toggled, this, [this, field, entry](bool on) {
                        if (on)
                        {
                            m_template->SetSectionOption(m_index, field.key, entry.value);
                            emit OptionsChanged();
                        }
                    });
                }
                rowLayout->addStretch();
                form->addRow(field.label, row);
                break;
            }
            case OPieceCartoucheOptionKind::NumberStyle:
            {
                auto *row = new QWidget(content);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                auto *group = new QButtonGroup(row);
                const QString stored = section.options.value(field.key).toString();
                const QString current = field.values.contains(stored) ? stored : field.values.value(0);
                for (const QString &value : field.values)
                {
                    // Digits, not words: the label is the choice itself ("1" or "01").
                    auto *radio = new QRadioButton(value, row);
                    radio->setChecked(value == current);
                    group->addButton(radio);
                    rowLayout->addWidget(radio);
                    connect(radio, &QRadioButton::toggled, this, [this, field, value](bool on) {
                        if (on)
                        {
                            m_template->SetSectionOption(m_index, field.key, value);
                            emit OptionsChanged();
                        }
                    });
                }
                rowLayout->addStretch();
                form->addRow(field.label, row);
                break;
            }
            case OPieceCartoucheOptionKind::Logo:
            {
                auto *combo = MakeComboBox(content);
                const QString stored = section.options.value(field.key).toString();
                const QList<OPieceCartoucheStorage::LogoEntry> logos = OPieceCartoucheStorage::LoadLogos();
                if (logos.isEmpty())
                {
                    combo->setEnabled(false);
                    combo->addItem(tr("No logos yet — add them in the Logo page"));
                }
                else
                {
                    for (const OPieceCartoucheStorage::LogoEntry &entry : logos)
                    {
                        combo->addItem(entry.name.isEmpty() ? entry.fileName : entry.name, entry.fileName);
                    }
                    const int index = combo->findData(stored);
                    combo->setCurrentIndex(index < 0 ? -1 : index);
                }
                connect(combo, &QComboBox::currentIndexChanged, this, [this, field, combo](int) {
                    m_template->SetSectionOption(m_index, field.key, combo->currentData().toString());
                    emit OptionsChanged();
                });
                form->addRow(field.label, combo);
                break;
            }
            case OPieceCartoucheOptionKind::LogoSize:
            {
                auto *spin = new QDoubleSpinBox(content);
                spin->setRange(0.1, 200.0);
                spin->setDecimals(2);
                spin->setSuffix(QStringLiteral(" cm"));
                spin->setValue(section.options.value(field.key, 2.0).toDouble());
                connect(spin, &QDoubleSpinBox::valueChanged, this, [this, field](double v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                form->addRow(field.label, spin);
                break;
            }
            case OPieceCartoucheOptionKind::Multilingual:
            {
                // Checkbox in plain language (no technical jargon) — it goes
                // flush-left in the edit zone (not in the form's field column),
                // aligned with the section description. When checked, the user
                // fills this information once per block language in the pattern /
                // piece options (icons = the theme's default square /
                // square-logo-fill QCheckBox look).
                auto *check = new QCheckBox(field.label, content);
                check->setChecked(section.options.value(field.key, false).toBool());
                check->setToolTip(tr("When checked, you will write this information once for each language "
                                     "of the block, in the pattern (or piece) options. "
                                     "It is not translated automatically."));
                connect(check, &QCheckBox::toggled, this, [this, field](bool on) {
                    m_template->SetSectionOption(m_index, field.key, on);
                    emit OptionsChanged();
                });
                layout->addWidget(check);
                break;
            }
            case OPieceCartoucheOptionKind::AxisRename:
            {
                m_axisRenameEditor = new OAxisRenameEditor(content);
                m_axisRenameEditor->SetTitle(field.label);
                connect(m_axisRenameEditor, &OAxisRenameEditor::RenamesChanged, this,
                        [this](const QMap<QString, OAxisRename> &renames) {
                            m_template->SetSectionOption(m_index, OPieceCartoucheAxisRename::kRenames,
                                                         MakeAxisRenames(renames));
                            emit OptionsChanged();
                        });
                connect(m_axisRenameEditor, &OAxisRenameEditor::OneLineChanged, this, [this](bool on) {
                    m_template->SetSectionOption(m_index, OPieceCartoucheAxisRename::kOneLine, on);
                    emit OptionsChanged();
                });
                layout->addWidget(m_axisRenameEditor);
                UpdateAxisRenameEditor();
                break;
            }
            case OPieceCartoucheOptionKind::Cut:
            {
                struct CutTab
                {
                    const char *name;
                    QString qtyKey;
                    QString foldKey;
                };
                static const CutTab kCutTabs[] = {
                    {QT_TRANSLATE_NOOP("OInfoBlockSectionEditor", "Fabric"),
                     OPieceCartoucheCut::kFabricQty, OPieceCartoucheCut::kFabricOnFold},
                    {QT_TRANSLATE_NOOP("OInfoBlockSectionEditor", "Interfacing"),
                     OPieceCartoucheCut::kInterfacingQty, OPieceCartoucheCut::kInterfacingOnFold},
                    {QT_TRANSLATE_NOOP("OInfoBlockSectionEditor", "Lining"),
                     OPieceCartoucheCut::kLiningQty, OPieceCartoucheCut::kLiningOnFold},
                };

                auto *tabs = new QTabWidget(content);
                for (const CutTab &tab : kCutTabs)
                {
                    auto *page = new QWidget(tabs);
                    auto *pageLayout = new QHBoxLayout(page);
                    pageLayout->setContentsMargins(6, 6, 6, 6);

                    auto *label = new QLabel(tr("Cut"), page);
                    pageLayout->addWidget(label);

                    auto *spin = new QSpinBox(page);
                    spin->setRange(0, 99);
                    spin->setValue(section.options.value(tab.qtyKey, 0).toInt());
                    connect(spin, &QSpinBox::valueChanged, this, [this, tab](int v) {
                        m_template->SetSectionOption(m_index, tab.qtyKey, v);
                        emit OptionsChanged();
                    });
                    pageLayout->addWidget(spin);

                    auto *fold = new QCheckBox(tr("On fold"), page);
                    fold->setChecked(section.options.value(tab.foldKey, false).toBool());
                    connect(fold, &QCheckBox::toggled, this, [this, tab](bool on) {
                        m_template->SetSectionOption(m_index, tab.foldKey, on);
                        emit OptionsChanged();
                    });
                    pageLayout->addWidget(fold);

                    pageLayout->addStretch();
                    tabs->addTab(page, tr(tab.name));
                }
                form->addRow(field.label, tabs);
                break;
            }
        }
    }

    layout->addLayout(form);
    layout->addStretch();

    m_root->addWidget(content);
    m_content = content;
}

void OInfoBlockSectionEditor::UpdateAxisRenameEditor()
{
    if (m_axisRenameEditor == nullptr || m_template == nullptr || m_index < 0 || m_index >= m_template->SectionCount())
    {
        return;
    }
    const auto &section = m_template->SectionAt(m_index);
    const QString typeId = m_template->SectionTypeId(m_index);

    // The "original" the aliases override: the axis name + its values (Sizes),
    // or the measurement name (Measurement — the value is computed).
    QString originalName;
    QStringList values;
    if (typeId == QStringLiteral("axis"))
    {
        originalName = section.options.value(QStringLiteral("axis")).toString();
        values = m_table != nullptr ? m_table->AxisValues(originalName) : QStringList();
    }
    else if (typeId == QStringLiteral("measure"))
    {
        originalName = section.options.value(QStringLiteral("measureName")).toString();
    }

    const QMap<QString, OAxisRename> renames =
        ParseAxisRenames(section.options.value(OPieceCartoucheAxisRename::kRenames));
    const bool oneLine = section.options.value(OPieceCartoucheAxisRename::kOneLine, false).toBool();
    m_axisRenameEditor->SetData(renames, oneLine, m_template->Languages(), originalName, values);
}
