/************************************************************************
 **
 **  @file   otemplatesectioneditor.cpp
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

#include "otemplatesectioneditor.h"

#include "core/osizetable.h"
#include "core/otemplateoptioneditor.h"

#include "core/ologolibrary.h"

#include "vmisc/FactoryWidgets.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QSpinBox>
#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>

OTemplateSectionEditor::OTemplateSectionEditor(QWidget *parent) : QWidget(parent)
{
    m_root = new QVBoxLayout(this);
    m_root->setContentsMargins(0, 0, 0, 0);
    Rebuild();
}

void OTemplateSectionEditor::SetSection(OTemplateSectionHost *tpl, int index, const OSizeTable *table)
{
    m_template = tpl;
    m_table = table;
    m_index = index;
    Rebuild();
}

void OTemplateSectionEditor::RetranslateUI()
{
    // The whole edit zone is data-driven — rebuild it so every tr() literal and
    // catalogue label is re-resolved in the new language.
    Rebuild();
}

void OTemplateSectionEditor::Rebuild()
{
    if (m_content != nullptr)
    {
        m_root->removeWidget(m_content);
        delete m_content;
        m_content = nullptr;
    }
    m_form = nullptr;
    m_conditionals.clear();
    m_composites.clear();

    if (m_template == nullptr || m_index < 0 || m_index >= m_template->SectionCount())
    {
        auto *hint = new QLabel(tr("Select a section to edit its settings."), this);
        hint->setAlignment(Qt::AlignCenter);
        hint->setWordWrap(true);
        m_root->addWidget(hint);
        m_content = hint;
        return;
    }

    // The entry of the family being edited — not a catalogue this widget picked for itself. Two
    // families may hold an information of the same name and mean different things by it.
    const OTemplateSection &type = m_template->SectionType(m_template->SectionTypeId(m_index));
    const QHash<QString, QVariant> options = m_template->SectionOptions(m_index);

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
    m_form = form;

    for (const OTemplateOptionField &field : type.Fields())
    {
        // **A composite builds its own control**, and this editor never learns what it is. The
        // family that declared the composite provided the factory, and what the control is, which
        // keys it writes and how it draws them stay with that family — the same reason the field
        // declares its sub-options rather than the editor recognising a kind.
        if (field.compositeEditor)
        {
            const OTemplateOptionContext context = OptionContext();
            auto *control = field.compositeEditor(content, context);
            if (field.onOwnRow)
            {
                layout->addWidget(control);
            }
            else
            {
                form->addRow(field.label, control);
            }
            m_composites.append(control);
            continue;
        }

        // Where this field's row is about to land, noted before it is added: the fields the
        // schema shows conditionally are found again by their row once the form is built.
        const int row = form->rowCount();

        switch (field.kind)
        {
            case OTemplateOptionKind::FontFamily:
            {
                auto *combo = new QFontComboBox(content);
                const QString family = options.value(field.key).toString();
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
            case OTemplateOptionKind::FontWeight:
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
                const int current = options.value(field.key, 0).toInt();
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
            case OTemplateOptionKind::FontSize:
            {
                auto *spin = new QSpinBox(content);
                spin->setRange(0, 288);
                spin->setSuffix(QStringLiteral(" px"));
                spin->setSpecialValueText(tr("Default"));  // 0 = use default size
                spin->setValue(options.value(field.key, 0.0).toInt());
                connect(spin, &QSpinBox::valueChanged, this, [this, field](int v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                form->addRow(field.label, spin);
                break;
            }
            case OTemplateOptionKind::LineSpacing:
            {
                auto *spin = new QSpinBox(content);
                spin->setRange(-100, 100);
                spin->setSuffix(QStringLiteral(" px"));
                spin->setToolTip(tr("Extra vertical space between this line and the next one. "
                                    "0 = default spacing, negative = tighter, positive = more space."));
                spin->setValue(options.value(field.key, 0).toInt());
                connect(spin, &QSpinBox::valueChanged, this, [this, field](int v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                form->addRow(field.label, spin);
                break;
            }
            case OTemplateOptionKind::Axis:
            {
                auto *combo = MakeComboBox(content);
                const QString stored = options.value(field.key).toString();
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
                    emit OptionsChanged();
                    // Another field changed what a composite may be reading — the name it renames.
                    // It is told, and decides for itself what an alias of a name that is gone is
                    // worth.
                    RefreshCompositeEditors();
                });
                form->addRow(field.label, combo);
                break;
            }
            case OTemplateOptionKind::MeasureName:
            {
                auto *combo = MakeComboBox(content);
                const QString stored = options.value(field.key).toString();
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
                    emit OptionsChanged();
                    // Same as the axis: what the composite was renaming is not what it renames now.
                    RefreshCompositeEditors();
                });
                form->addRow(field.label, combo);
                break;
            }
            case OTemplateOptionKind::Separator:
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
                const QString current = options.value(field.key).toString();
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
            case OTemplateOptionKind::NumberStyle:
            {
                auto *row = new QWidget(content);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);
                auto *group = new QButtonGroup(row);
                const QString stored = options.value(field.key).toString();
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
            case OTemplateOptionKind::Logo:
            {
                auto *combo = MakeComboBox(content);
                const QString stored = options.value(field.key).toString();
                const QList<OLogoLibrary::Entry> logos = OLogoLibrary::Load();
                if (logos.isEmpty())
                {
                    combo->setEnabled(false);
                    combo->addItem(tr("No logos yet — add them in the Logo page"));
                }
                else
                {
                    for (const OLogoLibrary::Entry &entry : logos)
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
            case OTemplateOptionKind::LogoSize:
            {
                auto *spin = new QDoubleSpinBox(content);
                spin->setRange(0.1, 200.0);
                spin->setDecimals(2);
                spin->setSuffix(QStringLiteral(" cm"));
                spin->setValue(options.value(field.key, 2.0).toDouble());
                connect(spin, &QDoubleSpinBox::valueChanged, this, [this, field](double v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                form->addRow(field.label, spin);
                break;
            }
            case OTemplateOptionKind::TextSource:
            {
                // The two answers are whole sentences rather than words, because the question is
                // not "where is it stored" but "will this be the same everywhere, or will I be
                // asked again" — and that is what the sentence says.
                struct SourceEntry
                {
                    const char *name;
                    QString value;
                };
                static const SourceEntry kSources[] = {
                    {QT_TRANSLATE_NOOP("OTemplateSectionEditor", "Same on every cover page"),
                     OTemplateTextSource::kHere},
                    {QT_TRANSLATE_NOOP("OTemplateSectionEditor", "Filled in for each pattern"),
                     OTemplateTextSource::kPerPattern},
                };

                auto *combo = MakeComboBox(content);
                const QString current = options.value(field.key, OTemplateTextSource::kHere).toString();
                for (const SourceEntry &entry : kSources)
                {
                    combo->addItem(tr(entry.name), entry.value);
                    if (entry.value == current)
                    {
                        combo->setCurrentIndex(combo->count() - 1);
                    }
                }
                connect(combo, &QComboBox::currentIndexChanged, this, [this, field, combo](int) {
                    m_template->SetSectionOption(m_index, field.key, combo->currentData().toString());
                    // The answer decides whether the text is written here or elsewhere, so the
                    // field that holds it comes and goes.
                    UpdateFieldVisibility();
                    emit OptionsChanged();
                });
                form->addRow(field.label, combo);
                break;
            }
            case OTemplateOptionKind::Text:
            {
                auto *edit = new QPlainTextEdit(content);
                edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
                // Three lines to begin with, and it grows with what is written — what the free
                // text of a piece does, so the two read alike.
                const auto minHeight = [edit]() {
                    return edit->fontMetrics().lineSpacing() * 3 + edit->frameWidth() * 2
                           + static_cast<int>(edit->document()->documentMargin()) * 2;
                };
                edit->setFixedHeight(minHeight());
                edit->setPlainText(options.value(field.key).toString());
                connect(edit, &QPlainTextEdit::textChanged, this, [this, field, edit, minHeight]() {
                    const int height =
                        static_cast<int>(edit->document()->size().height()) + edit->frameWidth() * 2;
                    edit->setFixedHeight((std::max)(height, minHeight()));
                    // Written on each keystroke rather than after a pause: the preview follows what
                    // is being typed, and waiting would leave it a step behind the hand.
                    m_template->SetSectionOption(m_index, field.key, edit->toPlainText());
                    emit OptionsChanged();
                });
                form->addRow(field.label, edit);
                break;
            }
            case OTemplateOptionKind::Multilingual:
            {
                // Checkbox in plain language (no technical jargon) — it goes
                // flush-left in the edit zone (not in the form's field column),
                // aligned with the section description. When checked, the user
                // fills this information once per block language in the pattern /
                // piece options (icons = the theme's default square /
                // square-logo-fill QCheckBox look).
                auto *check = new QCheckBox(field.label, content);
                check->setChecked(options.value(field.key, false).toBool());
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
            // A composite is built before this switch, from the factory its field carries. A field
            // of one of these kinds that declared none has nothing to draw, and draws nothing.
            case OTemplateOptionKind::Cut:
            case OTemplateOptionKind::AxisRename:
                break;
        }

        // The field says it is shown under a condition; the editor obeys without knowing which
        // kind asked for it. A field the form never received has no row to hide, and is left alone.
        if (!field.visibleWhenKey.isEmpty() && form->rowCount() > row)
        {
            if (QLayoutItem *item = form->itemAt(row, QFormLayout::FieldRole);
                item != nullptr && item->widget() != nullptr)
            {
                m_conditionals.append(ConditionalField{item->widget(), field.visibleWhenKey,
                                                       field.visibleWhenValues});
            }
        }
    }

    layout->addLayout(form);
    layout->addStretch();

    m_root->addWidget(content);
    m_content = content;

    UpdateFieldVisibility();
}

void OTemplateSectionEditor::UpdateFieldVisibility()
{
    if (m_form == nullptr || m_template == nullptr || m_index < 0
        || m_index >= m_template->SectionCount())
    {
        return;
    }

    const QHash<QString, QVariant> options = m_template->SectionOptions(m_index);
    for (const ConditionalField &field : m_conditionals)
    {
        const QString value = options.value(field.whenKey).toString();
        m_form->setRowVisible(field.control, field.whenValues.contains(value));
    }
}

void OTemplateSectionEditor::RefreshCompositeEditors()
{
    if (m_template == nullptr || m_index < 0 || m_index >= m_template->SectionCount())
    {
        return;
    }

    // A composite may read something another field writes — the axis a rename applies to — so it
    // is told when the options change under it, and re-reads them itself. Which of them cares is
    // no more this editor's business than what any of them is.
    const OTemplateOptionContext context = OptionContext();
    for (OTemplateOptionEditor *editor : m_composites)
    {
        if (editor != nullptr)
        {
            editor->Refresh(context);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSectionEditor::OptionContext() -> OTemplateOptionContext
{
    OTemplateOptionContext context;
    if (m_template == nullptr || m_index < 0 || m_index >= m_template->SectionCount())
    {
        return context;
    }

    context.options = m_template->SectionOptions(m_index);
    context.sectionTypeId = m_template->SectionTypeId(m_index);
    context.languages = m_template->Languages();
    context.table = m_table;

    // A control writes back through the same door as any other field: the section is never told
    // which kind of field changed one of its values.
    context.apply = [this](const QString &key, const QVariant &value) {
        m_template->SetSectionOption(m_index, key, value);
        emit OptionsChanged();
    };
    return context;
}
