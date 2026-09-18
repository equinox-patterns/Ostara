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
                element.setAttribute(field.key, QString::number(value.toInt()));
                break;
            case OTemplateOptionKind::FontSize:
            case OTemplateOptionKind::LogoSize:
                element.setAttribute(field.key, QString::number(value.toDouble()));
                break;
            case OTemplateOptionKind::Multilingual:
                element.setAttribute(field.key, value.toBool());
                break;

            // Composites store several values under keys the field names, and writing them is
            // more than this loop can know. A cover page declares none today; the day it does,
            // this is where the expansion goes.
            case OTemplateOptionKind::Cut:
            case OTemplateOptionKind::AxisRename:
                break;

            case OTemplateOptionKind::FontFamily:
            case OTemplateOptionKind::Axis:
            case OTemplateOptionKind::MeasureName:
            case OTemplateOptionKind::Separator:
            case OTemplateOptionKind::NumberStyle:
            case OTemplateOptionKind::Logo:
            case OTemplateOptionKind::TextSource:
            case OTemplateOptionKind::Text:
                // A text written here may run over several lines. Qt writes the breaks as &#xa;
                // inside the attribute, which the parser reads back as breaks — the text is not
                // flattened onto one line.
                element.setAttribute(field.key, value.toString());
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
                section.options.insert(field.key, raw.toInt());
                break;
            case OTemplateOptionKind::FontSize:
            case OTemplateOptionKind::LogoSize:
                section.options.insert(field.key, raw.toDouble());
                break;
            case OTemplateOptionKind::Multilingual:
                section.options.insert(field.key,
                                       raw.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0
                                           || raw.toInt() != 0);
                break;

            case OTemplateOptionKind::Cut:
            case OTemplateOptionKind::AxisRename:
                break;

            case OTemplateOptionKind::FontFamily:
            case OTemplateOptionKind::Axis:
            case OTemplateOptionKind::MeasureName:
            case OTemplateOptionKind::Separator:
            case OTemplateOptionKind::NumberStyle:
            case OTemplateOptionKind::Logo:
            case OTemplateOptionKind::TextSource:
            case OTemplateOptionKind::Text:
                section.options.insert(field.key, raw);
                break;
        }
    }

    return section;
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
    sections[index].options.insert(key, value);
}
