/************************************************************************
 **
 **  @file   ocoverpagetemplate.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  A cover page template — the four bands, and their serialisation.
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

#include "ocoverpagetemplate.h"

#include "ocoverpagecatalogue.h"
#include "ocovergradedaxis.h"
#include "otemplateaxisoptions.h"
#include "otemplateseparatoroptions.h"

#include "vmisc/def.h"

#include <QDomDocument>

namespace
{
const QString kTagRoot = QStringLiteral("coverPageTemplate");
const QString kAttrVersion = QStringLiteral("version");
const QString kFormatVersion = QStringLiteral("1.0");

const QString kTagName = QStringLiteral("name");

const QString kTagLanguages = QStringLiteral("languages");
const QString kTagLanguage = QStringLiteral("language");
const QString kAttrCode = QStringLiteral("code");

const QString kTagSection = QStringLiteral("section");
const QString kAttrType = QStringLiteral("type");

/// What holds the values of a group of ticked boxes apart, written and read from here alone.
const QChar kValueJoin = QLatin1Char(',');

/// The element a band is written under. Named here and nowhere else, so the four move together.
auto BandTag(OCoverPageTemplate::Placement placement) -> QString
{
    switch (placement)
    {
        case OCoverPageTemplate::Placement::Top:
            return QStringLiteral("top");
        case OCoverPageTemplate::Placement::MiddleLeft:
            return QStringLiteral("middleLeft");
        case OCoverPageTemplate::Placement::MiddleRight:
            return QStringLiteral("middleRight");
        case OCoverPageTemplate::Placement::Bottom:
            return QStringLiteral("bottom");
    }
    return QString();
}

/// The band an element names — null when the element is not one of the four.
auto TagBand(const QString &tag) -> std::optional<OCoverPageTemplate::Placement>
{
    for (OCoverPageTemplate::Placement placement : OCoverPageTemplate::Placements())
    {
        if (BandTag(placement) == tag)
        {
            return placement;
        }
    }
    return std::nullopt;
}

/// Writes one information's options, driven by the fields its catalogue entry declares.
void WriteSection(QDomDocument &doc, QDomElement &parent, const OCoverPageTemplate::Section &section)
{
    QDomElement element = doc.createElement(kTagSection);
    element.setAttribute(kAttrType, section.typeId);

    // The catalogue says which fields the type has, and each field says how its value is written.
    // Nothing here names an information, so adding one to the catalogue is enough for it to be
    // saved — this loop already knows how.
    const OTemplateSection &type = OCoverPageCatalogue::Section(section.typeId);
    for (const OTemplateOptionField &field : type.Fields())
    {
        const QVariant value = section.options.value(field.key);
        if (!value.isValid())
        {
            continue;
        }
        switch (field.kind)
        {
            case OTemplateOptionKind::FontWeight:
            case OTemplateOptionKind::LineSpacing:
            case OTemplateOptionKind::Pixels:
                element.setAttribute(field.key, QString::number(value.toInt()));
                break;
            case OTemplateOptionKind::FontSize:
            case OTemplateOptionKind::LogoSize:
                element.setAttribute(field.key, QString::number(value.toDouble()));
                break;
            case OTemplateOptionKind::Boolean:
                element.setAttribute(field.key, value.toBool());
                break;

            // A composite stores several values under keys the field names, and writing them is
            // more than this loop can know — so the family that declares one writes it, or hands
            // the writing to a shared piece. The axes are shared: a cartouche writes the very same
            // three things about the very same table, and one writer is one format.
            case OTemplateOptionKind::Cut:
                break;

            case OTemplateOptionKind::AxisRename:
                WriteAxisOptions(doc, element, section.options);
                break;

            // The sizes the page lists, and what each of them is called. Written by the piece that
            // owns those keys, as the axes are: the control that writes them and the resolver that
            // reads them must not come apart.
            case OTemplateOptionKind::SizeRange:
                OCoverGradedAxisOptions::Write(doc, element, section.options);
                break;

            // A rule holds nothing: it is a line the editor draws between groups of fields, and the
            // guard above has already skipped it, since a value-less field sits in the options under
            // an invalid variant. Writing one would put a key in the file that says nothing.
            case OTemplateOptionKind::Rule:
                break;

            case OTemplateOptionKind::FontFamily:
            case OTemplateOptionKind::MeasureName:
            case OTemplateOptionKind::Separator:
            case OTemplateOptionKind::NumberStyle:
            case OTemplateOptionKind::Logo:
            case OTemplateOptionKind::TextSource:
            case OTemplateOptionKind::Text:
            case OTemplateOptionKind::SizeCase:
            case OTemplateOptionKind::Choice:
                // A text written here may run over several lines. Qt writes the breaks as &#xa;
                // inside the attribute, which the parser reads back as breaks — the text is not
                // flattened onto one line.
                //
                // A choice not yet made is written the same way, as an empty attribute: the file
                // says the question is still open rather than leaving it unsaid.
                element.setAttribute(field.key, value.toString());
                break;

            case OTemplateOptionKind::CheckGroup:
                // **A group of ticked values is written as one text.** What it holds is a list,
                // which this loop could not otherwise write — but a list joined by a character no
                // value of the group can contain is a text, and one attribute says it whole.
                element.setAttribute(field.key, value.toStringList().join(kValueJoin));
                break;
        }
    }

    parent.appendChild(element);
}

/// Reads one information back, over the defaults its catalogue entry declares.
auto ReadSection(const QDomElement &element) -> OCoverPageTemplate::Section
{
    OCoverPageTemplate::Section section;
    section.typeId = element.attribute(kAttrType);

    const OTemplateSection &type = OCoverPageCatalogue::Section(section.typeId);
    if (type.IsValid())
    {
        section.options = type.DefaultOptions();
    }

    // **The aliases are read before the loop, and not by it.** A rename is written as a child
    // element, and the loop's guard asks whether the *field's* key is an attribute of the section —
    // which, for a rename, is never true: the key names the map the renames live in, and no
    // attribute carries it. A section whose key happens to be one of the attributes its writer sets
    // — the axes — would be read either way; one that holds nothing but renames would not be read at
    // all, and the names the user wrote would be gone the next time the file was opened. The
    // cartouche reads its own this way, for the same reason.
    //
    // Nothing is invented here: what the reader does not find, it does not insert.
    ReadAxisOptions(element, section.options);

    for (const OTemplateOptionField &field : type.Fields())
    {
        if (!element.hasAttribute(field.key))
        {
            continue;
        }
        const QString raw = element.attribute(field.key);
        switch (field.kind)
        {
            case OTemplateOptionKind::FontWeight:
            case OTemplateOptionKind::LineSpacing:
            case OTemplateOptionKind::Pixels:
                section.options.insert(field.key, raw.toInt());
                break;
            case OTemplateOptionKind::FontSize:
            case OTemplateOptionKind::LogoSize:
                section.options.insert(field.key, raw.toDouble());
                break;
            case OTemplateOptionKind::Boolean:
                section.options.insert(field.key,
                                       raw.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0
                                           || raw.toInt() != 0);
                break;

            case OTemplateOptionKind::Cut:
                break;

            case OTemplateOptionKind::AxisRename:
                // Read above, before the guard could turn it away — see the note there. The branch
                // exists because the switch is exhaustive.
                break;

            case OTemplateOptionKind::SizeRange:
                // A section that names no range leaves the empty one its defaults hold, which is
                // what tells the page there are no sizes to list yet.
                OCoverGradedAxisOptions::Read(element, section.options);
                break;

            // Nothing to read: a rule is never written, so the guard above never lets one through.
            // The branch exists because the switch is exhaustive, and it says so rather than
            // pretending there is a value to restore.
            case OTemplateOptionKind::Rule:
                break;

            case OTemplateOptionKind::Separator:
            {
                // The value has to be one the field offers. A separator read back from a file that
                // this build no longer proposes would be drawn exactly as it stands, and the page
                // would write a character nobody chose — so an unknown one takes the field's own
                // default. **The same guard the cartouche has always had for the same value**,
                // because it is the same value and the same question.
                const QStringList allowed = field.values.isEmpty()
                                                ? OTemplateSeparatorOptions::AllSeparators()
                                                : field.values;
                const QString fallback =
                    field.defaultValue.isValid() ? field.defaultValue.toString() : QString();
                section.options.insert(field.key, allowed.contains(raw) ? raw : fallback);
                break;
            }

            case OTemplateOptionKind::FontFamily:
            case OTemplateOptionKind::MeasureName:
            case OTemplateOptionKind::NumberStyle:
            case OTemplateOptionKind::Logo:
            case OTemplateOptionKind::TextSource:
            case OTemplateOptionKind::Text:
            case OTemplateOptionKind::SizeCase:
                section.options.insert(field.key, raw);
                break;

            case OTemplateOptionKind::Choice:
            {
                // **An answer this build no longer offers leaves the question open** rather than
                // answering it with a word nobody chose, and the rest of the section stays held
                // back until someone answers it again. The same guard as the separator's, for the
                // same reason: a file is not a promise that the vocabulary has not moved.
                const QString fallback =
                    field.defaultValue.isValid() ? field.defaultValue.toString() : QString();
                section.options.insert(field.key, field.values.contains(raw) ? raw : fallback);
                break;
            }

            case OTemplateOptionKind::CheckGroup:
            {
                // What is kept is what the field still offers: a box ticked for a value this build
                // no longer proposes would be drawn as a choice and could never be unticked. An
                // answer that comes out empty falls back to the field's own default, because a
                // group of ticked boxes never comes out empty.
                QStringList ticked;
                for (const QString &token : raw.split(kValueJoin, Qt::SkipEmptyParts))
                {
                    if (field.values.contains(token) && !ticked.contains(token))
                    {
                        ticked.append(token);
                    }
                }
                if (ticked.isEmpty())
                {
                    ticked = field.defaultValue.toStringList();
                }
                section.options.insert(field.key, ticked);
                break;
            }
        }
    }

    return section;
}

/**
 * @brief Whether a page already asks for this text to be filled in once per pattern, elsewhere.
 *
 * **A page asks for it once.** What such a text holds is filled in on the pattern, and the pattern
 * holds one — so a second information asking the same question would have the two answering with
 * the same words and no way to tell which was meant. The text written here may be repeated as often
 * as the page likes, because it is written on the page and needs nothing from anybody.
 *
 * Every band is looked at, not just the one being edited: the four are one page.
 */
auto FilledInPerPatternElsewhere(const OCoverPageTemplate &tpl,
                                 OCoverPageTemplate::Placement placement, int index) -> bool
{
    for (OCoverPageTemplate::Placement band : OCoverPageTemplate::Placements())
    {
        const QList<OCoverPageTemplate::Section> &sections = tpl.Sections(band);
        for (int i = 0; i < sections.size(); ++i)
        {
            if (band == placement && i == index)
            {
                continue;
            }
            const OCoverPageTemplate::Section &section = sections.at(i);
            if (section.typeId == QLatin1String("freeText")
                && section.options.value(QStringLiteral("textSource")).toString()
                       == OTemplateTextSource::kPerPattern)
            {
                return true;
            }
        }
    }
    return false;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplate::Placements() -> const QList<Placement> &
{
    // The page's own order, top to bottom, and the order the editor shows them in.
    static const QList<Placement> kPlacements{Placement::Top, Placement::MiddleLeft,
                                              Placement::MiddleRight, Placement::Bottom};
    return kPlacements;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplate::Sections(Placement placement) const -> const QList<Section> &
{
    static const QList<Section> kEmpty;
    const auto it = bands.constFind(placement);
    return it == bands.constEnd() ? kEmpty : it.value();
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplate::SectionAt(Placement placement, int index) const -> const Section &
{
    static const Section kEmpty;
    const QList<Section> &sections = Sections(placement);
    if (index < 0 || index >= sections.size())
    {
        return kEmpty;
    }
    return sections.at(index);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplate::AddSection(Placement placement, const QString &typeId)
{
    // The entry says what an information of this type starts as: a new logo section arrives with
    // its size already at the catalogue's default, so the form has something to show rather than
    // a row full of nothing. An unknown id is still added — the file stays readable, and a
    // catalogue that no longer knows a type does not get to delete it.
    const OTemplateSection &type = OCoverPageCatalogue::Section(typeId);

    Section section;
    section.typeId = typeId;
    if (type.IsValid())
    {
        section.options = type.DefaultOptions();
    }
    bands[placement].append(section);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplate::RemoveSection(Placement placement, int index)
{
    QList<Section> &sections = bands[placement];
    if (index < 0 || index >= sections.size())
    {
        return;
    }
    sections.removeAt(index);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplate::MoveUp(Placement placement, int index)
{
    QList<Section> &sections = bands[placement];
    if (index <= 0 || index >= sections.size())
    {
        return;
    }
    sections.move(index, index - 1);
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplate::MoveDown(Placement placement, int index)
{
    QList<Section> &sections = bands[placement];
    if (index < 0 || index >= sections.size() - 1)
    {
        return;
    }
    sections.move(index, index + 1);
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplate::ToXml(QDomDocument &doc) const -> QDomElement
{
    QDomElement root = doc.createElement(kTagRoot);
    root.setAttribute(kAttrVersion, kFormatVersion);

    QDomElement nameElement = doc.createElement(kTagName);
    nameElement.appendChild(doc.createTextNode(name));
    root.appendChild(nameElement);

    // Omitted when empty rather than written as an empty element: a template that declares no
    // language is one that falls back to the application's, and saying so with nothing is what
    // an older reader understands.
    if (!languages.isEmpty())
    {
        QDomElement languagesElement = doc.createElement(kTagLanguages);
        for (const QString &code : languages)
        {
            QDomElement languageElement = doc.createElement(kTagLanguage);
            languageElement.setAttribute(kAttrCode, code);
            languagesElement.appendChild(languageElement);
        }
        root.appendChild(languagesElement);
    }

    // A band with nothing in it is left out: the four elements are the four placements, and
    // writing three empty ones would say the file carries a skeleton it does not.
    for (Placement placement : Placements())
    {
        const QList<Section> &sections = Sections(placement);
        if (sections.isEmpty())
        {
            continue;
        }

        QDomElement bandElement = doc.createElement(BandTag(placement));
        for (const Section &section : sections)
        {
            WriteSection(doc, bandElement, section);
        }
        root.appendChild(bandElement);
    }

    return root;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageTemplate::LoadFromXml(const QDomElement &root) -> bool
{
    if (root.tagName() != kTagRoot)
    {
        return false;
    }

    // Read over a fresh template rather than over whatever was here: every setting the file does
    // not carry takes its default, and nothing of a previously loaded template survives.
    const OCoverPageTemplate defaults;
    *this = defaults;

    name = root.firstChildElement(kTagName).text();

    const QDomElement languagesElement = root.firstChildElement(kTagLanguages);
    for (QDomElement languageElement = languagesElement.firstChildElement(kTagLanguage);
         !languageElement.isNull(); languageElement = languageElement.nextSiblingElement(kTagLanguage))
    {
        languages.append(languageElement.attribute(kAttrCode));
    }

    // A file that declares none — written before the languages existed, or emptied by hand — is
    // read as the application's own rather than as nothing: a template with no language is one
    // that could never be drawn.
    if (languages.isEmpty())
    {
        languages = {DefaultUiLanguage()};
    }

    // Walk the whole element rather than asking for the four tags: a band this build does not
    // know is skipped by name, and the ones it does know are found wherever they sit.
    for (QDomElement bandElement = root.firstChildElement(); !bandElement.isNull();
         bandElement = bandElement.nextSiblingElement())
    {
        const std::optional<Placement> placement = TagBand(bandElement.tagName());
        if (!placement.has_value())
        {
            continue;
        }

        for (QDomElement sectionElement = bandElement.firstChildElement(kTagSection);
             !sectionElement.isNull();
             sectionElement = sectionElement.nextSiblingElement(kTagSection))
        {
            bands[*placement].append(ReadSection(sectionElement));
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------

OCoverPageBandHost::OCoverPageBandHost(OCoverPageTemplate *tpl, OCoverPageTemplate::Placement placement)
  : m_template(tpl), m_placement(placement)
{
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageBandHost::SetPlacement(OCoverPageTemplate::Placement placement)
{
    m_placement = placement;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageBandHost::Languages() const -> QStringList
{
    return m_template == nullptr ? QStringList() : m_template->languages;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageBandHost::SectionCount() const -> int
{
    return m_template == nullptr ? 0 : static_cast<int>(m_template->Sections(m_placement).size());
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageBandHost::SectionTypeId(int index) const -> QString
{
    return m_template == nullptr ? QString() : m_template->SectionAt(m_placement, index).typeId;
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageBandHost::SectionType(const QString &typeId) const -> const OTemplateSection &
{
    // The cover page's own catalogue — the entry that says what this information is *here*. The
    // cartouche has an information of the same name that means something else, and it is this one
    // the editor must be given.
    return OCoverPageCatalogue::Section(typeId);
}

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageBandHost::SectionOptions(int index) const -> QHash<QString, QVariant>
{
    return m_template == nullptr ? QHash<QString, QVariant>()
                                 : m_template->SectionAt(m_placement, index).options;
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageBandHost::SetSectionOption(int index, const QString &key, const QVariant &value)
{
    if (m_template == nullptr)
    {
        return;
    }

    QList<OCoverPageTemplate::Section> &sections = m_template->bands[m_placement];
    if (index < 0 || index >= sections.size())
    {
        return;
    }

    // **The one answer a page may give only once.** A free text left to be filled in per pattern is
    // written on the pattern, and a pattern holds one such text; a second information asking the
    // same question would answer with the same words and leave no way to tell them apart. Refused
    // here rather than greyed out in the editor, because the question is about the whole page and
    // no field can see the whole page — and the control reads the model back, so what it shows is
    // what was kept.
    if (key == QLatin1String("textSource")
        && value.toString() == OTemplateTextSource::kPerPattern
        && FilledInPerPatternElsewhere(*m_template, m_placement, index))
    {
        return;
    }

    sections[index].options.insert(key, value);
}
