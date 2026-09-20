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
#include "core/otemplateseparatoroptions.h"

#include "core/ologolibrary.h"

#include "vmisc/FactoryWidgets.h"
#include "vwidgets/ocartouchepainter.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
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
    m_rows.clear();

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

    // **The frame carries the name of the information being edited.** Which information these
    // settings belong to is the first thing the user has to be able to read, and a titled frame
    // says it — a bare form read as the continuation of the list above it, and a background alone
    // did not cut the two apart. An information the catalogue no longer declares falls back to its
    // id, as the preview's placeholder does, so the title is never empty.
    const QString title = type.IsValid() ? type.DisplayName() : m_template->SectionTypeId(m_index);
    auto *content = new QGroupBox(title, this);
    // Held from here rather than at the end: the fields are built into it as the loop runs, and a
    // control they put on a shared line needs to know what it belongs to while it is being placed.
    m_content = content;
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
                // A block rather than a value: it takes a whole row of the form, at the very place
                // its field is declared. **That is what makes the form follow the catalogue's
                // order.** A block added to the page instead would land after the settings
                // whatever the catalogue said, because the form is only put on the page once the
                // whole loop has run — and an information whose first field is a combo, whose
                // second is a block and whose rest are settings would show its combo last.
                form->addRow(control);
            }
            else
            {
                form->addRow(field.label, control);
            }
            m_composites.append(control);
            // A block is a field like any other: it may be shown only under a condition, and the
            // catalogue says so on the same terms. It declares its own rows, so the form holds no
            // field to hide — a block on a row of its own is hidden whole, and its contents with it.
            RegisterConditional(field, control, field.onOwnRow);
            RegisterEnabler(field, control);
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
                PlaceControl(field, combo);
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
                // **The nine weights are all there is**, and one of them is what an unset weight
                // means: Regular, which the application's own font carries. An entry called
                // « Default » offered a tenth weight that is not a weight — the value it stood for
                // was already on the list. A stored 0 still reads as Regular, and picking anything
                // from the list — Regular included — writes a real one.
                auto *combo = MakeComboBox(content);
                const int stored = options.value(field.key, 0).toInt();
                const int current = stored > 0 ? stored : QFont::Normal;
                bool named = false;
                for (const WeightEntry &entry : kWeights)
                {
                    combo->addItem(tr(entry.name), entry.weight);
                    if (entry.weight == current)
                    {
                        combo->setCurrentIndex(combo->count() - 1);
                        named = true;
                    }
                }
                if (!named)
                {
                    // A weight the list does not hold — written by hand, or by a build that knew
                    // more of them. Showing the first entry would name a weight the file does not
                    // ask for, so the closest honest answer is the one an unset weight gets.
                    combo->setCurrentIndex(combo->findData(QFont::Normal));
                }
                connect(combo, &QComboBox::currentIndexChanged, this, [this, field, combo](int) {
                    m_template->SetSectionOption(m_index, field.key, combo->currentData().toInt());
                    emit OptionsChanged();
                });
                PlaceControl(field, combo);
                break;
            }
            case OTemplateOptionKind::FontSize:
            {
                // The size a template is set in when it does not say is a real number of pixels,
                // and it is that number the control shows — read from the font the page is drawn
                // in, not from the widget this editor lives in, whose stylesheet carries a screen
                // size. An entry called « Default » named no size at all. **Nothing is written
                // until the user turns the dial**: a field nobody touched stays unset in the file
                // and keeps following the page's font.
                auto *spin = new QSpinBox(content);
                spin->setRange(1, 288);
                spin->setSuffix(QStringLiteral(" px"));
                const int stored = options.value(field.key, 0.0).toInt();
                const int baseSize = OCartouchePainter::BaseFont().pixelSize();
                spin->setValue(stored > 0 ? stored : (baseSize > 0 ? baseSize : spin->minimum()));
                connect(spin, &QSpinBox::valueChanged, this, [this, field](int v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                PlaceControl(field, spin);
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
                PlaceControl(field, spin);
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
                PlaceControl(field, combo);
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
                PlaceControl(field, combo);
                break;
            }
            case OTemplateOptionKind::Separator:
            {
                struct SeparatorEntry
                {
                    QString label;
                    QString value;
                };
                // The values the field offers, or — when it names none of its own — every separator
                // there is. **One list and one loop**: the default set used to be spelled out here a
                // second time, which is another place to forget the day a separator is added.
                const QStringList separators =
                    field.values.isEmpty() ? OTemplateSeparatorOptions::AllSeparators() : field.values;

                QList<SeparatorEntry> entries;
                for (const QString &value : separators)
                {
                    QString label = value;
                    if (value.isEmpty())
                    {
                        // **The « none » entry has to say so.** It is the first of every list and the
                        // one checked by default, so a radio button with nothing written beside it is
                        // a choice the user cannot read — and cannot tell from the ones next to it.
                        label = tr("No separator");
                    }
                    else if (value == QLatin1String("of"))
                    {
                        // Word separators are translated; punctuation stays as it is written.
                        label = tr("of");
                    }
                    entries.append({label, value});
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
                PlaceControl(field, row);
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
                PlaceControl(field, row);
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
                PlaceControl(field, combo);
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
                PlaceControl(field, spin);
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
                PlaceControl(field, combo);
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
                PlaceControl(field, edit);
                break;
            }
            case OTemplateOptionKind::SizeCase:
            {
                // **Both cases are offered, and each of them sets the information up.** A page
                // stating one size names its axes and stops; a page listing a range states the
                // ranges it holds, one size to a line, and the settings that belong to that are
                // the ones the answer brings and takes away — see UpdateFieldVisibility.
                //
                // The words are the question's own, like the text sources above — what the two
                // cases are does not depend on which family is asking.
                struct CaseEntry
                {
                    const char *name;
                    QString value;
                };
                static const CaseEntry kCases[] = {
                    {QT_TRANSLATE_NOOP("OTemplateSectionEditor", "Single size"),
                     OTemplateSizeCase::kSingle},
                    {QT_TRANSLATE_NOOP("OTemplateSectionEditor", "A range of sizes"),
                     OTemplateSizeCase::kMulti},
                };

                auto *combo = MakeComboBox(content);
                const QString current =
                    options.value(field.key, OTemplateSizeCase::kSingle).toString();
                for (const CaseEntry &entry : kCases)
                {
                    combo->addItem(tr(entry.name), entry.value);
                    if (entry.value == current)
                    {
                        combo->setCurrentIndex(combo->count() - 1);
                    }
                }
                connect(combo, &QComboBox::currentIndexChanged, this, [this, field, combo](int) {
                    m_template->SetSectionOption(m_index, field.key, combo->currentData().toString());
                    // The answer decides which case is being written, so the settings that belong
                    // to one of them come and go.
                    UpdateFieldVisibility();
                    emit OptionsChanged();
                });
                PlaceControl(field, combo);
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
            case OTemplateOptionKind::Boolean:
            {
                // A yes-or-no the information asks about itself. The box carries the field's own
                // label, so it takes its row whole instead of standing under a caption in the
                // field column — a caption beside a checkbox says the same thing twice.
                auto *check = new QCheckBox(field.label, content);
                check->setChecked(options.value(field.key, false).toBool());
                connect(check, &QCheckBox::toggled, this, [this, field](bool on) {
                    m_template->SetSectionOption(m_index, field.key, on);
                    emit OptionsChanged();
                });
                form->addRow(check);
                break;
            }
            // A rule says where one group of settings ends and the next begins. It asks nothing and
            // keeps nothing — the catalogue declares it purely to place it — so it takes its row
            // whole, with no caption, and the editor draws it here because this is the only place
            // that knows the form's full width. Its look comes from the theme, like every other
            // rule in the application.
            case OTemplateOptionKind::Rule:
            {
                auto *rule = new QFrame(content);
                rule->setObjectName(QStringLiteral("templateSectionRule"));
                rule->setFixedHeight(1);
                rule->setFrameShape(QFrame::NoFrame);
                form->addRow(rule);
                break;
            }
            // A composite is built before this switch, from the factory its field carries. A field
            // of one of these kinds that declared none has nothing to draw, and draws nothing.
            case OTemplateOptionKind::Cut:
            case OTemplateOptionKind::AxisRename:
            case OTemplateOptionKind::SizeRange:
                break;
        }

        // The field says it is shown, or made usable, under a condition; the editor obeys without
        // knowing which kind asked for it. A field the form never received has no row to reach, and
        // is left alone.
        if ((!field.visibleWhenKey.isEmpty() || field.enabledWhen) && form->rowCount() > row)
        {
            QWidget *control = nullptr;
            bool ownRow = false;
            if (QLayoutItem *item = form->itemAt(row, QFormLayout::FieldRole);
                item != nullptr && item->widget() != nullptr)
            {
                control = item->widget();
            }
            else if (QLayoutItem *spanning = form->itemAt(row, QFormLayout::SpanningRole);
                     spanning != nullptr && spanning->widget() != nullptr)
            {
                // A field that took its row whole — a rule, or a checkbox carrying its own words —
                // has no field role to hide. The control itself is what goes.
                control = spanning->widget();
                ownRow = true;
            }
            RegisterConditional(field, control, ownRow);
            RegisterEnabler(field, control);
        }
    }

    // Every shared line is closed with a stretch once all its fields have been added: the controls
    // keep the width they ask for and the rest of the line stays empty.
    for (QHBoxLayout *row : m_rows)
    {
        row->addStretch();
    }

    layout->addLayout(form);
    layout->addStretch();

    m_root->addWidget(content);

    UpdateFieldVisibility();
}

//---------------------------------------------------------------------------------------------------------------------

void OTemplateSectionEditor::PlaceControl(const OTemplateOptionField &field, QWidget *control)
{
    if (m_form == nullptr)
    {
        return;
    }

    // A field that names no line takes a row of its own, under its own words. Every field did,
    // before a field could share one.
    if (field.rowKey.isEmpty())
    {
        m_form->addRow(field.label, control);
        return;
    }

    // **The line this field named.** The first field to name it opens it — under the words the
    // catalogue gave that line, or under none at all, which is what a group of controls that name
    // themselves wants: a caption beside a font, a weight and a size would be spelling out what the
    // reader already sees. The fields that follow join the same line.
    QHBoxLayout *row = m_rows.value(field.rowKey);
    if (row == nullptr)
    {
        auto *line = new QWidget(m_content);
        row = new QHBoxLayout(line);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(8);
        if (!field.rowLabel.isEmpty())
        {
            row->addWidget(new QLabel(field.rowLabel, line));
        }
        m_rows.insert(field.rowKey, row);
        m_form->addRow(line);
    }

    // What a caption gave permanently is still one hover away. Without it, a size and a line
    // spacing are two spin boxes of pixels that nothing tells apart.
    if (control->toolTip().isEmpty())
    {
        control->setToolTip(field.label);
    }
    row->addWidget(control);
}

//---------------------------------------------------------------------------------------------------------------------

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
        const bool shown = field.whenValues.contains(value);
        if (field.ownRow)
        {
            // Nothing to hide but the control itself: it has no caption column to take with it.
            field.control->setVisible(shown);
        }
        else
        {
            m_form->setRowVisible(field.control, shown);
        }
    }

    // **A part is made unusable, not hidden**, when the section is not yet in a state to answer it:
    // the labels stay readable, so the user sees what the information is made of and what is still
    // waiting for them. This one is safe to run as often as the options change — it toggles, and
    // rebuilding a control would take the focus out of whatever is being typed in.
    for (const EnabledField &field : m_enabled)
    {
        field.control->setEnabled(field.when(options));
    }
}

void OTemplateSectionEditor::RegisterConditional(const OTemplateOptionField &field, QWidget *control,
                                                 bool ownRow)
{
    if (field.visibleWhenKey.isEmpty() || control == nullptr)
    {
        return;
    }
    m_conditionals.append(ConditionalField{control, field.visibleWhenKey, field.visibleWhenValues,
                                           ownRow});
}

void OTemplateSectionEditor::RegisterEnabler(const OTemplateOptionField &field, QWidget *control)
{
    if (!field.enabledWhen || control == nullptr)
    {
        return;
    }
    m_enabled.append(EnabledField{control, field.enabledWhen});
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
        // **A block can decide what the rest of the form may answer** — a range, once chosen, is
        // what makes the settings below it usable — so what the fields obey is worked out again
        // here. It only shows, hides and greys: rebuilding a control would take the focus out of
        // the field the user is typing in, which is the one place this must never do.
        UpdateFieldVisibility();
        emit OptionsChanged();
    };
    return context;
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSectionEditor::CurrentOptions() const -> QHash<QString, QVariant>
{
    if (m_template == nullptr || m_index < 0 || m_index >= m_template->SectionCount())
    {
        return {};
    }
    return m_template->SectionOptions(m_index);
}
