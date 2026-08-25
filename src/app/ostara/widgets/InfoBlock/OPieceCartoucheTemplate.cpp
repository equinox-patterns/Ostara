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
const QString kAttrType = QStringLiteral("type");
const QString kFormatVersion = QStringLiteral("1.0");
}  // namespace

OPieceCartoucheTemplate::OPieceCartoucheTemplate(const QString &name) : m_name(name)
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

    QDomElement sectionsEl = doc.createElement(kTagSections);
    for (const Section &section : m_sections)
    {
        QDomElement sectionEl = doc.createElement(kTagSection);
        sectionEl.setAttribute(kAttrType, section.typeId);

        const auto &type = OPieceCartoucheCatalogue::Section(section.typeId);
        for (const OPieceCartoucheOptionField &field : type.Fields())
        {
            const QVariant value = section.options.value(field.key);
            if (!value.isValid())
            {
                continue;
            }
            switch (field.kind)
            {
                case OPieceCartoucheOptionKind::FontWeight:
                    sectionEl.setAttribute(field.key, QString::number(value.toInt()));
                    break;
                case OPieceCartoucheOptionKind::FontSize:
                case OPieceCartoucheOptionKind::LogoWidth:
                case OPieceCartoucheOptionKind::LogoHeight:
                    sectionEl.setAttribute(field.key, QString::number(value.toDouble()));
                    break;
                case OPieceCartoucheOptionKind::FontFamily:
                case OPieceCartoucheOptionKind::Axis:
                case OPieceCartoucheOptionKind::MeasureName:
                case OPieceCartoucheOptionKind::Separator:
                case OPieceCartoucheOptionKind::Logo:
                    sectionEl.setAttribute(field.key, value.toString());
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
                if (!sectionEl.hasAttribute(field.key))
                {
                    continue;
                }
                const QString attr = sectionEl.attribute(field.key);
                switch (field.kind)
                {
                    case OPieceCartoucheOptionKind::FontWeight:
                        section.options.insert(field.key, attr.toInt());
                        break;
                    case OPieceCartoucheOptionKind::FontSize:
                    case OPieceCartoucheOptionKind::LogoWidth:
                    case OPieceCartoucheOptionKind::LogoHeight:
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
                        section.options.insert(field.key, kSeparators.contains(attr) ? attr : QString());
                        break;
                    }
                    case OPieceCartoucheOptionKind::FontFamily:
                    case OPieceCartoucheOptionKind::MeasureName:
                    case OPieceCartoucheOptionKind::Logo:
                        section.options.insert(field.key, attr);
                        break;
                }
            }
        }
        m_sections.append(section);
    }

    return true;
}
