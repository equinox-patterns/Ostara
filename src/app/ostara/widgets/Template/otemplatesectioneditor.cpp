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
#include <QFontDatabase>
#include <QFontInfo>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStringList>
#include <QTabWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>

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

//---------------------------------------------------------------------------------------------------------------------
/// @brief The prefix a font option's key is built on — « fontFamily » and « cornerFontFamily ».
///
/// The spelling is `SectionFont`'s own: an empty prefix gives « fontFamily », and a prefix takes
/// the stem capitalized — « corner » with « FontFamily ». Read here rather than guessed, so a new
/// group of typography controls needs no third list to be kept in step with the other two.
static auto FontOptionPrefix(const QString &key) -> QString
{
    for (const char *stem : {"fontFamily", "fontWeight", "FontFamily", "FontWeight"})
    {
        const QString ending = QString::fromLatin1(stem);
        if (key.endsWith(ending))
        {
            return key.left(key.size() - ending.size());
        }
    }
    return key;
}

/// @brief The two halves of a typography pair, from either half.
static auto SiblingFontKey(const QString &key, const char *wanted) -> QString
{
    const QString prefix = FontOptionPrefix(key);
    if (prefix.isEmpty())
    {
        return QString::fromLatin1(wanted);
    }
    QString stem = QString::fromLatin1(wanted);
    stem[0] = stem.at(0).toUpper();
    return prefix + stem;
}

//---------------------------------------------------------------------------------------------------------------------
/// @brief Fill @p combo with the faces @p family really has, keeping the answer already given.
///
/// **Décision de Sophie, 22-09-2026** — « ça ne devrait être que les vraies graisses de la font,
/// pas des choix génériques qui n'existent pas. » **Regular is on every family**, and the rest
/// belongs to the family: that is Illustrator's rule, and the only one that can be true of a font
/// nobody has seen yet.
///
/// The nine numbered entries this control used to offer were CSS weights wearing Qt's names, and
/// two of them named a face the family never gave: asking « Light » (300) drew « ExtraLight », and
/// « Black » (900) drew « ExtraBold » — measured on the families installed here. **A list of
/// numbers cannot name faces**, and cannot be right for the next font either.
///
/// @param family the family whose faces are wanted; empty falls back to the page's own font.
/// @param stored what the file asks for: a face name when it holds one, and — for a file written
///               before this control named faces — the number whose face it must show. A number of
///               nought is an unset weight, and an unset weight is Regular.
static void FillFontFaces(QComboBox *combo, const QString &family, const QString &stored)
{
    const QString current = family.isEmpty() ? OCartouchePainter::BaseFont().family() : family;
    const QStringList faces = QFontDatabase::styles(current);
    if (faces.isEmpty())
    {
        return; // a family with no face to offer keeps what it had
    }

    QSignalBlocker block(combo); // filling the list is not the user answering it
    const QString kept = combo->currentData().toString();
    combo->clear();
    for (const QString &face : faces)
    {
        combo->addItem(face, face);
    }

    const QString want = kept.isEmpty() ? stored : kept;
    int at = want.isEmpty() ? -1 : combo->findData(want);
    bool numbered = false;
    const int weight = want.toInt(&numbered);
    if (at < 0 && numbered && weight > 0)
    {
        // An old file, and the face its number really resolves to — the same answer the drawing
        // gets, so the list cannot show one face while the page is drawn in another.
        QFont probe(current);
        probe.setWeight(QFont::Weight(weight));
        at = combo->findText(QFontInfo(probe).styleName());
    }
    if (at < 0)
    {
        at = combo->findText(QStringLiteral("Regular"));
    }
    combo->setCurrentIndex(at < 0 ? 0 : at);
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
    // **Every list of controls dies with the controls.** The form is deleted just above, so these
    // hold dangling pointers until they are cleared — and anything that walks them before the new
    // form is built would be calling into freed widgets. A list added here without a line here is
    // a crash the first time a section is rebuilt.
    m_conditionals.clear();
    m_enabled.clear();
    m_composites.clear();
    m_rows.clear();
    m_weightCombos.clear();

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
                    // **The faces belong to the family, so they change with it.** Refilled here
                    // rather than when the list opens, so that what the control shows is true the
                    // moment it is read — a list still offering the faces of the font that was
                    // chosen before it would name none of the ones the family actually has.
                    if (QComboBox *weights = m_weightCombos.value(SiblingFontKey(field.key, "fontWeight")))
                    {
                        FillFontFaces(weights, f.family(), QString());
                    }
                    emit OptionsChanged();
                });
                PlaceControl(field, combo);
                break;
            }
            case OTemplateOptionKind::FontWeight:
            {
                // ── What the font really has, and not a list of nine numbers ────────────────
                //
                // **Décision de Sophie, 22-09-2026** — « ça ne devrait être que les vraies graisses
                // de la font, pas des choix génériques qui n'existent pas. » **Regular est sur
                // toutes les familles**, le reste appartient à la famille : c'est la règle
                // d'Illustrator, et la seule qui puisse être vraie d'une police qu'on n'a pas vue.
                //
                // Les neuf entrées numérotées que ce contrôle offrait étaient des graisses CSS
                // portant les noms de Qt — « Demi Bold » là où la famille écrit « SemiBold », et
                // « Light » tombant sur la fonte que la famille nomme « ExtraLight ». Relevé sur les
                // familles installées ici : dix-huit fontes pour Jost, dix-huit pour Inter, six pour
                // Helvetica. Une liste de numéros ne peut pas les nommer — ni avoir raison de la
                // prochaine.
                auto *combo = MakeComboBox(content);
                m_weightCombos.insert(field.key, combo);
                FillFontFaces(combo, options.value(SiblingFontKey(field.key, "fontFamily")).toString(),
                              options.value(field.key).toString());
                connect(combo, &QComboBox::currentIndexChanged, this, [this, field, combo](int) {
                    // **The face's NAME is what is written**, and not a number: a number names no
                    // face — the same 300 is « Light » in one family and « ExtraLight » in the next.
                    m_template->SetSectionOption(m_index, field.key, combo->currentData().toString());
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
            case OTemplateOptionKind::Pixels:
            {
                // **A distance, typed and not measured.** What the distance is *of* — the room an
                // empty space takes, tomorrow the gap between two other things — belongs to the
                // field and its label, which is why this control asks only for the number and shows
                // the unit beside it.
                //
                // Unlike a font size there is no « unset » state to fall back on: a distance is a
                // value, zero included, and the catalogue that wanted room says how much it wants.
                auto *spin = new QSpinBox(content);
                spin->setRange(0, 288);
                spin->setSuffix(QStringLiteral(" px"));
                spin->setValue(options.value(field.key, 0).toInt());
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
                spin->setToolTip(tr("Extra vertical space below this line. 0 = default spacing, "
                                    "negative = tighter, positive = more space. It counts on the "
                                    "last line of the information as well, which is what moves what "
                                    "stands beneath it."));
                spin->setValue(options.value(field.key, 0).toInt());
                connect(spin, &QSpinBox::valueChanged, this, [this, field](int v) {
                    m_template->SetSectionOption(m_index, field.key, v);
                    emit OptionsChanged();
                });
                PlaceControl(field, spin);
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
                    QString hint;
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
                    QString hint;
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
                    else if (value == OTemplateSeparatorOptions::kParenthesesSeparator)
                    {
                        // **Neither punctuation nor a word**, so it is named rather than shown: the
                        // token would read as jargon, and a bracket would read as a bracket the user
                        // asked to print. The hint says the rest, since where each language goes is
                        // what the choice actually decides.
                        label = tr("In brackets");
                        hint = tr("The first language is written plainly, the others in brackets — "
                                  "Tailles (Sizes).");
                    }
                    entries.append({label, value, hint});
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
                    radio->setToolTip(entry.hint);
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

                    // **The model has the last word.** An answer the template refuses — a page that
                    // would be asked twice for the same thing — leaves the model as it was, and the
                    // control has to say what the model says rather than what was clicked, or the
                    // page would show an answer that was never taken.
                    {
                        const QSignalBlocker blocker(combo);
                        const int settled =
                            combo->findData(m_template->SectionOptions(m_index).value(field.key).toString());
                        if (settled >= 0 && settled != combo->currentIndex())
                        {
                            combo->setCurrentIndex(settled);
                        }
                    }

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
            case OTemplateOptionKind::Choice:
            {
                // **A question the section waits on.** The first item is the answer not yet given,
                // and it is a word rather than a blank: an empty combo says nothing about why it is
                // empty, which is the "silent round" this application has already had to correct
                // once — a separator's empty value now reads « No separator ».
                //
                // Answering it is what makes the rest of the section usable: the declaring
                // catalogue puts an enabledWhen on every other field, and this is the answer that
                // predicate is asked about.
                auto *combo = MakeComboBox(content);
                combo->addItem(tr("Choose…"), QString());
                const QString current = options.value(field.key).toString();
                for (int i = 0; i < field.values.size(); ++i)
                {
                    const QString value = field.values.at(i);
                    combo->addItem(i < field.valueLabels.size() ? field.valueLabels.at(i) : value,
                                   value);
                    if (value == current)
                    {
                        combo->setCurrentIndex(combo->count() - 1);
                    }
                }
                connect(combo, &QComboBox::currentIndexChanged, this, [this, field, combo](int) {
                    m_template->SetSectionOption(m_index, field.key, combo->currentData().toString());
                    // The answer decides what the rest of the section is, so it is looked at again.
                    UpdateFieldVisibility();
                    // **And a composite may be showing something that depends on it** — the title
                    // of one case is not the title of the other — so it is told which answer stands
                    // now. Without this it would go on renaming the case the user has just left.
                    RefreshCompositeEditors();
                    emit OptionsChanged();
                });
                PlaceControl(field, combo);
                break;
            }
            case OTemplateOptionKind::CheckGroup:
            {
                // **A row of boxes, and a set that is never empty.** What the group holds is the
                // values ticked among those the field offers; unticking the last one is refused,
                // because a set of no answers is not an answer — a value written in no unit at all
                // is not a value, and the page would print a name followed by nothing.
                auto *row = new QWidget(content);
                auto *rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(8);

                const QStringList ticked = options.value(field.key).toStringList();
                QVector<QPair<QCheckBox *, QString>> boxes;
                for (int i = 0; i < field.values.size(); ++i)
                {
                    const QString value = field.values.at(i);
                    auto *box = new QCheckBox(i < field.valueLabels.size() ? field.valueLabels.at(i)
                                                                           : value,
                                              row);
                    box->setChecked(ticked.contains(value));
                    boxes.append({box, value});
                    rowLayout->addWidget(box);
                }
                rowLayout->addStretch();

                // What stands ticked, read off the boxes rather than off the option: between two
                // clicks the option is what it was, and the question is what the user is looking at.
                const auto Ticked = [boxes]() {
                    QStringList values;
                    for (const QPair<QCheckBox *, QString> &entry : boxes)
                    {
                        if (entry.first->isChecked())
                        {
                            values.append(entry.second);
                        }
                    }
                    return values;
                };

                for (const QPair<QCheckBox *, QString> &entry : boxes)
                {
                    QCheckBox *box = entry.first;
                    connect(box, &QCheckBox::toggled, this, [this, field, box, Ticked](bool) {
                        if (Ticked().isEmpty())
                        {
                            // Put back what the user has just taken away, without writing anything:
                            // the option never held the empty set, so nothing changed.
                            QSignalBlocker blocker(box);
                            box->setChecked(true);
                            return;
                        }
                        m_template->SetSectionOption(m_index, field.key, Ticked());
                        emit OptionsChanged();
                    });
                }

                PlaceControl(field, row);
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
