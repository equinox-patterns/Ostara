/************************************************************************
 **
 **  @file   OPieceCartoucheTemplate.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  Cartouche template model — ordered section list + per-section options.
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

#include "OPieceCartoucheTemplate.h"

#include "OPieceCartoucheCatalogue.h"
#include "OPieceCartoucheOptions.h"
#include "core/otemplateseparatoroptions.h"

#include "def.h"

#include <QDomDocument>
#include <QDomText>
#include <QStringList>

namespace
{
const QString kTagRoot = QStringLiteral("cartoucheTemplate");
const QString kAttrVersion = QStringLiteral("version");
const QString kTagName = QStringLiteral("name");
const QString kTagSections = QStringLiteral("sections");
const QString kTagSection = QStringLiteral("section");
const QString kTagLanguages = QStringLiteral("languages");
const QString kTagLanguage = QStringLiteral("language");
const QString kAttrType = QStringLiteral("type");
const QString kAttrCode = QStringLiteral("code");
// The <rename> tags that used to stand here moved to the shared writer, with the rest of the axis
// group: a cover page writes the same three things about the same table.
const QString kFormatVersion = QStringLiteral("1.0");
}  // namespace

OPieceCartoucheTemplate::OPieceCartoucheTemplate(const QString &name) : m_name(name), m_languages{DefaultUiLanguage()}
{
}

auto OPieceCartoucheTemplate::Name() const -> QString
{
    return m_name;
}

void OPieceCartoucheTemplate::SetName(const QString &name)
{
    m_name = name;
}

auto OPieceCartoucheTemplate::Languages() const -> QStringList
{
    return m_languages;
}

void OPieceCartoucheTemplate::SetLanguages(const QStringList &languages)
{
    m_languages = languages;
}

void OPieceCartoucheTemplate::AddLanguage(const QString &code)
{
    if (!m_languages.contains(code))
    {
        m_languages.append(code);
    }
}

void OPieceCartoucheTemplate::RemoveLanguage(const QString &code)
{
    m_languages.removeAll(code);
}

auto OPieceCartoucheTemplate::ContainsLanguage(const QString &code) const -> bool
{
    return m_languages.contains(code);
}

auto OPieceCartoucheTemplate::SectionCount() const -> int
{
    return static_cast<int>(m_sections.size());
}

auto OPieceCartoucheTemplate::SectionTypeId(int index) const -> QString
{
    if (index < 0 || index >= m_sections.size())
    {
        return QString();
    }
    return m_sections.at(index).typeId;
}

auto OPieceCartoucheTemplate::SectionType(const QString &typeId) const -> const OTemplateSection &
{
    // The cartouche's own catalogue — the entry that says what this information is *on a piece*.
    return OPieceCartoucheCatalogue::Section(typeId);
}

auto OPieceCartoucheTemplate::SectionOptions(int index) const -> QHash<QString, QVariant>
{
    return SectionAt(index).options;
}

auto OPieceCartoucheTemplate::SectionAt(int index) const -> const Section &
{
    static const Section kEmpty;
    if (index < 0 || index >= m_sections.size())
    {
        return kEmpty;
    }
    return m_sections.at(index);
}

void OPieceCartoucheTemplate::SetSectionOption(int index, const QString &key, const QVariant &value)
{
    if (index < 0 || index >= m_sections.size())
    {
        return;
    }
    m_sections[index].options.insert(key, value);
}

void OPieceCartoucheTemplate::AddSection(const QString &typeId)
{
    const auto &type = OPieceCartoucheCatalogue::Section(typeId);
    Section section;
    section.typeId = typeId;
    if (type.IsValid())
    {
        section.options = type.DefaultOptions();
    }
    m_sections.append(section);
}

void OPieceCartoucheTemplate::RemoveSection(int index)
{
    if (index < 0 || index >= m_sections.size())
    {
        return;
    }
    m_sections.removeAt(index);
}

void OPieceCartoucheTemplate::MoveUp(int index)
{
    if (index <= 0 || index >= m_sections.size())
    {
        return;
    }
    m_sections.move(index, index - 1);
}

void OPieceCartoucheTemplate::MoveDown(int index)
{
    if (index < 0 || index >= m_sections.size() - 1)
    {
        return;
    }
    m_sections.move(index, index + 1);
}

void OPieceCartoucheTemplate::Clear()
{
    m_sections.clear();
}

auto OPieceCartoucheTemplate::IsEmpty() const -> bool
{
    return m_sections.isEmpty();
}

auto OPieceCartoucheTemplate::ToXml(QDomDocument &doc) const -> QDomElement
{
    QDomElement root = doc.createElement(kTagRoot);
    root.setAttribute(kAttrVersion, kFormatVersion);

    QDomElement nameEl = doc.createElement(kTagName);
    nameEl.appendChild(doc.createTextNode(m_name));
    root.appendChild(nameEl);

    if (!m_languages.isEmpty())
    {
        QDomElement languagesEl = doc.createElement(kTagLanguages);
        for (const QString &code : m_languages)
        {
            QDomElement languageEl = doc.createElement(kTagLanguage);
            languageEl.setAttribute(kAttrCode, code);
            languagesEl.appendChild(languageEl);
        }
        root.appendChild(languagesEl);
    }

    QDomElement sectionsEl = doc.createElement(kTagSections);
    for (const Section &section : m_sections)
    {
        QDomElement sectionEl = doc.createElement(kTagSection);
        sectionEl.setAttribute(kAttrType, section.typeId);

        const auto &type = OPieceCartoucheCatalogue::Section(section.typeId);
        for (const OTemplateOptionField &field : type.Fields())
        {
            if (field.kind == OTemplateOptionKind::Cut)
            {
                // The Cut section expands into its six per-fabric sub-options.
                for (const QString &key : OPieceCartoucheCut::kAllKeys)
                {
                    const QVariant value = section.options.value(key);
                    if (value.isValid())
                    {
                        sectionEl.setAttribute(key, value.toString());
                    }
                }
                continue;
            }
            const QVariant value = section.options.value(field.key);
            if (!value.isValid())
            {
                continue;
            }
            switch (field.kind)
            {
                case OTemplateOptionKind::FontWeight:
                case OTemplateOptionKind::LineSpacing:
                    sectionEl.setAttribute(field.key, QString::number(value.toInt()));
                    break;
                case OTemplateOptionKind::FontSize:
                case OTemplateOptionKind::LogoSize:
                    sectionEl.setAttribute(field.key, QString::number(value.toDouble()));
                    break;
                case OTemplateOptionKind::FontFamily:
                case OTemplateOptionKind::Axis:
                case OTemplateOptionKind::MeasureName:
                case OTemplateOptionKind::Separator:
                case OTemplateOptionKind::NumberStyle:
                case OTemplateOptionKind::Logo:
                case OTemplateOptionKind::TextSource:
                case OTemplateOptionKind::Text:
                case OTemplateOptionKind::SizeCase:
                    sectionEl.setAttribute(field.key, value.toString());
                    break;
                case OTemplateOptionKind::Multilingual:
                case OTemplateOptionKind::Boolean:
                    sectionEl.setAttribute(field.key,
                                           value.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
                    break;
                case OTemplateOptionKind::AxisRename:
                    // Which axes, how each is named in each language, and whether every language
                    // takes a line of its own. Written by the shared writer: a cover page states
                    // the same three things about the same table, and two writers would be two
                    // places for the format to drift.
                    WriteAxisOptions(doc, sectionEl, section.options);
                    break;
                case OTemplateOptionKind::Cut:
                    break;  // handled above (expands into the six fabric sub-keys)
                case OTemplateOptionKind::Rule:
                    // A rule holds no value, and the guard above has already skipped it: writing
                    // one would put a key in the file that says nothing. The branch exists because
                    // the switch is exhaustive.
                    break;
                case OTemplateOptionKind::SizeRange:
                    // A cartouche states a piece's sizes, never a range of them: it draws no
                    // sample and lists nothing, so it declares no such field and writes none.
                    break;
            }
        }
        sectionsEl.appendChild(sectionEl);
    }
    root.appendChild(sectionsEl);

    return root;
}

auto OPieceCartoucheTemplate::LoadFromXml(const QDomElement &root) -> bool
{
    if (root.tagName() != kTagRoot)
    {
        return false;
    }

    m_sections.clear();
    m_name = root.firstChildElement(kTagName).text();

    // Languages — absent element (older file) falls back to the default.
    m_languages.clear();
    const QDomElement languagesEl = root.firstChildElement(kTagLanguages);
    for (QDomElement languageEl = languagesEl.firstChildElement(kTagLanguage); !languageEl.isNull();
         languageEl = languageEl.nextSiblingElement(kTagLanguage))
    {
        m_languages.append(languageEl.attribute(kAttrCode));
    }
    if (m_languages.isEmpty())
    {
        m_languages.append(DefaultUiLanguage());
    }

    const QDomElement sectionsEl = root.firstChildElement(kTagSections);
    for (QDomElement sectionEl = sectionsEl.firstChildElement(kTagSection); !sectionEl.isNull();
         sectionEl = sectionEl.nextSiblingElement(kTagSection))
    {
        Section section;
        section.typeId = sectionEl.attribute(kAttrType);

        const auto &type = OPieceCartoucheCatalogue::Section(section.typeId);
        if (type.IsValid())
        {
            // Start from defaults, then overlay the stored attributes — a
            // missing attribute keeps the default (forward compatibility).
            section.options = type.DefaultOptions();
            for (const OTemplateOptionField &field : type.Fields())
            {
                if (field.kind == OTemplateOptionKind::Cut)
                {
                    for (const QString &key : OPieceCartoucheCut::kAllKeys)
                    {
                        if (!sectionEl.hasAttribute(key))
                        {
                            continue;
                        }
                        const QString attr = sectionEl.attribute(key);
                        if (OPieceCartoucheCut::kQtyKeys.contains(key))
                        {
                            section.options.insert(key, attr.toInt());
                        }
                        else
                        {
                            section.options.insert(key, attr.toInt() != 0
                                                             || attr.compare(QStringLiteral("true"),
                                                                             Qt::CaseInsensitive) == 0);
                        }
                    }
                    continue;
                }
                if (field.kind == OTemplateOptionKind::AxisRename)
                {
                    // Read by the shared reader, which also reads the older single-axis shape: a
                    // cartouche saved before today opens showing what it showed.
                    ReadAxisOptions(sectionEl, section.options);
                    continue;
                }
                if (!sectionEl.hasAttribute(field.key))
                {
                    continue;
                }
                const QString attr = sectionEl.attribute(field.key);
                switch (field.kind)
                {
                    case OTemplateOptionKind::FontWeight:
                    case OTemplateOptionKind::LineSpacing:
                        section.options.insert(field.key, attr.toInt());
                        break;
                    case OTemplateOptionKind::FontSize:
                    case OTemplateOptionKind::LogoSize:
                        section.options.insert(field.key, attr.toDouble());
                        break;
                    case OTemplateOptionKind::Axis:
                        // Axis names are user-defined in the table — stored verbatim.
                        section.options.insert(field.key, attr);
                        break;
                    case OTemplateOptionKind::Separator:
                    {
                        // Every separator any information may hold — the same list the cover page
                        // checks its own values against, so a file that one family accepts is a file
                        // the other accepts.
                        const QStringList allowed = field.values.isEmpty()
                                                        ? OTemplateSeparatorOptions::AllSeparators()
                                                        : field.values;
                        const QString fallback =
                            field.defaultValue.isValid() ? field.defaultValue.toString() : QString();
                        section.options.insert(field.key, allowed.contains(attr) ? attr : fallback);
                        break;
                    }
                    case OTemplateOptionKind::NumberStyle:
                    {
                        const QString fallback = field.defaultValue.isValid()
                                                     ? field.defaultValue.toString()
                                                     : OPieceCartoucheNumber::kPlain;
                        section.options.insert(field.key, field.values.contains(attr) ? attr : fallback);
                        break;
                    }
                    case OTemplateOptionKind::FontFamily:
                    case OTemplateOptionKind::MeasureName:
                    case OTemplateOptionKind::Logo:
                    case OTemplateOptionKind::TextSource:
                    case OTemplateOptionKind::Text:
                    case OTemplateOptionKind::SizeCase:
                        section.options.insert(field.key, attr);
                        break;
                    case OTemplateOptionKind::Multilingual:
                    case OTemplateOptionKind::Boolean:
                        section.options.insert(field.key, attr.toInt() != 0
                                                             || attr.compare(QStringLiteral("true"),
                                                                             Qt::CaseInsensitive) == 0);
                        break;
                    case OTemplateOptionKind::Cut:
                        break;  // handled above (expands into the six fabric sub-keys)
                    case OTemplateOptionKind::AxisRename:
                        break;  // handled above (per-language <rename> children)
                    case OTemplateOptionKind::Rule:
                        break;  // never written, so never read — see the writer
                    case OTemplateOptionKind::SizeRange:
                        break;  // a cartouche declares none — see the writer
                }
            }
        }
        m_sections.append(section);
    }

    return true;
}
