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
const QString kTagRename = QStringLiteral("rename");
const QString kTagRenameValue = QStringLiteral("value");
const QString kAttrLocale = QStringLiteral("locale");
const QString kAttrRenameName = QStringLiteral("name");
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
        for (const OPieceCartoucheOptionField &field : type.Fields())
        {
            if (field.kind == OPieceCartoucheOptionKind::Cut)
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
                case OPieceCartoucheOptionKind::FontWeight:
                case OPieceCartoucheOptionKind::LineSpacing:
                    sectionEl.setAttribute(field.key, QString::number(value.toInt()));
                    break;
                case OPieceCartoucheOptionKind::FontSize:
                case OPieceCartoucheOptionKind::LogoSize:
                    sectionEl.setAttribute(field.key, QString::number(value.toDouble()));
                    break;
                case OPieceCartoucheOptionKind::FontFamily:
                case OPieceCartoucheOptionKind::Axis:
                case OPieceCartoucheOptionKind::MeasureName:
                case OPieceCartoucheOptionKind::Separator:
                case OPieceCartoucheOptionKind::NumberStyle:
                case OPieceCartoucheOptionKind::Logo:
                    sectionEl.setAttribute(field.key, value.toString());
                    break;
                case OPieceCartoucheOptionKind::Multilingual:
                    sectionEl.setAttribute(field.key,
                                           value.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
                    break;
                case OPieceCartoucheOptionKind::AxisRename:
                {
                    // The one-line-per-language flag is an attribute; the
                    // per-language aliases become child <rename> elements. Only
                    // non-empty aliases are persisted (empty = keep the original).
                    const QVariant oneLine = section.options.value(OPieceCartoucheAxisRename::kOneLine);
                    if (oneLine.isValid())
                    {
                        sectionEl.setAttribute(OPieceCartoucheAxisRename::kOneLine,
                                               oneLine.toBool() ? QStringLiteral("true") : QStringLiteral("false"));
                    }
                    const QMap<QString, OAxisRename> renames =
                        ParseAxisRenames(section.options.value(OPieceCartoucheAxisRename::kRenames));
                    for (auto it = renames.constBegin(); it != renames.constEnd(); ++it)
                    {
                        QDomElement renameEl = doc.createElement(kTagRename);
                        renameEl.setAttribute(kAttrLocale, it.key());
                        if (!it.value().name.isEmpty())
                        {
                            renameEl.setAttribute(kAttrRenameName, it.value().name);
                        }
                        for (const QString &valueAlias : it.value().values)
                        {
                            if (valueAlias.isEmpty())
                            {
                                continue;
                            }
                            QDomElement valueEl = doc.createElement(kTagRenameValue);
                            valueEl.appendChild(doc.createTextNode(valueAlias));
                            renameEl.appendChild(valueEl);
                        }
                        sectionEl.appendChild(renameEl);
                    }
                    break;
                }
                case OPieceCartoucheOptionKind::Cut:
                    break;  // handled above (expands into the six fabric sub-keys)
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
            for (const OPieceCartoucheOptionField &field : type.Fields())
            {
                if (field.kind == OPieceCartoucheOptionKind::Cut)
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
                if (field.kind == OPieceCartoucheOptionKind::AxisRename)
                {
                    // One-line-per-language flag (attribute, default false).
                    if (sectionEl.hasAttribute(OPieceCartoucheAxisRename::kOneLine))
                    {
                        const QString attr = sectionEl.attribute(OPieceCartoucheAxisRename::kOneLine);
                        section.options.insert(OPieceCartoucheAxisRename::kOneLine,
                                               attr.toInt() != 0
                                                   || attr.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0);
                    }
                    // Per-language aliases (child <rename> elements).
                    QMap<QString, OAxisRename> renames;
                    for (QDomElement renameEl = sectionEl.firstChildElement(kTagRename); !renameEl.isNull();
                         renameEl = renameEl.nextSiblingElement(kTagRename))
                    {
                        OAxisRename rename;
                        rename.name = renameEl.attribute(kAttrRenameName);
                        for (QDomElement valueEl = renameEl.firstChildElement(kTagRenameValue); !valueEl.isNull();
                             valueEl = valueEl.nextSiblingElement(kTagRenameValue))
                        {
                            rename.values.append(valueEl.text());
                        }
                        renames.insert(renameEl.attribute(kAttrLocale), rename);
                    }
                    if (!renames.isEmpty())
                    {
                        section.options.insert(OPieceCartoucheAxisRename::kRenames, MakeAxisRenames(renames));
                    }
                    continue;
                }
                if (!sectionEl.hasAttribute(field.key))
                {
                    continue;
                }
                const QString attr = sectionEl.attribute(field.key);
                switch (field.kind)
                {
                    case OPieceCartoucheOptionKind::FontWeight:
                    case OPieceCartoucheOptionKind::LineSpacing:
                        section.options.insert(field.key, attr.toInt());
                        break;
                    case OPieceCartoucheOptionKind::FontSize:
                    case OPieceCartoucheOptionKind::LogoSize:
                        section.options.insert(field.key, attr.toDouble());
                        break;
                    case OPieceCartoucheOptionKind::Axis:
                        // Axis names are user-defined in the table — stored verbatim.
                        section.options.insert(field.key, attr);
                        break;
                    case OPieceCartoucheOptionKind::Separator:
                    {
                        static const QStringList kSeparators = {QString(), QStringLiteral(":"), QStringLiteral("-"),
                                                                QStringLiteral("/"), QStringLiteral("·")};
                        const QStringList allowed = field.values.isEmpty() ? kSeparators : field.values;
                        const QString fallback =
                            field.defaultValue.isValid() ? field.defaultValue.toString() : QString();
                        section.options.insert(field.key, allowed.contains(attr) ? attr : fallback);
                        break;
                    }
                    case OPieceCartoucheOptionKind::NumberStyle:
                    {
                        const QString fallback = field.defaultValue.isValid()
                                                     ? field.defaultValue.toString()
                                                     : OPieceCartoucheNumber::kPlain;
                        section.options.insert(field.key, field.values.contains(attr) ? attr : fallback);
                        break;
                    }
                    case OPieceCartoucheOptionKind::FontFamily:
                    case OPieceCartoucheOptionKind::MeasureName:
                    case OPieceCartoucheOptionKind::Logo:
                        section.options.insert(field.key, attr);
                        break;
                    case OPieceCartoucheOptionKind::Multilingual:
                        section.options.insert(field.key, attr.toInt() != 0
                                                             || attr.compare(QStringLiteral("true"),
                                                                             Qt::CaseInsensitive) == 0);
                        break;
                    case OPieceCartoucheOptionKind::Cut:
                        break;  // handled above (expands into the six fabric sub-keys)
                    case OPieceCartoucheOptionKind::AxisRename:
                        break;  // handled above (per-language <rename> children)
                }
            }
        }
        m_sections.append(section);
    }

    return true;
}
