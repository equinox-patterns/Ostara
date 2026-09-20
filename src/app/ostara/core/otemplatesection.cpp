/************************************************************************
 **
 **  @file   otemplatesection.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  One section type of a template catalogue, and its declarative option schema.
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

#include "otemplatesection.h"

namespace
{
/// Default logo size in cm — what a logo is drawn at until someone says otherwise.
constexpr double kDefaultLogoSize = 2.0;
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OTemplateSection::OTemplateSection(const QString &id, const QString &sourceName,
                                   const QString &displayName, const QString &description,
                                   const QList<OTemplateOptionField> &fields)
  : m_id(id), m_sourceName(sourceName), m_displayName(displayName), m_description(description), m_fields(fields)
{
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSection::IsValid() const -> bool
{
    return !m_id.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSection::Id() const -> QString
{
    return m_id;
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSection::DisplayName() const -> QString
{
    return m_displayName;
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSection::SourceName() const -> QString
{
    return m_sourceName;
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSection::Description() const -> QString
{
    return m_description;
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSection::Fields() const -> const QList<OTemplateOptionField> &
{
    return m_fields;
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSection::HasField(const QString &key) const -> bool
{
    for (const OTemplateOptionField &field : m_fields)
    {
        if (field.key == key)
        {
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSection::DefaultValue(const QString &key) const -> QVariant
{
    for (const OTemplateOptionField &field : m_fields)
    {
        if (field.key != key)
        {
            continue;
        }
        if (field.defaultValue.isValid())
        {
            return field.defaultValue;
        }
        switch (field.kind)
        {
            case OTemplateOptionKind::FontFamily:
                return QString();
            case OTemplateOptionKind::FontWeight:
                return 0;
            case OTemplateOptionKind::FontSize:
                return 0.0;
            case OTemplateOptionKind::LineSpacing:
                return 0;
            case OTemplateOptionKind::Axis:
                return QString();
            case OTemplateOptionKind::MeasureName:
                return QString();
            case OTemplateOptionKind::Separator:
                return QString();
            case OTemplateOptionKind::NumberStyle:
                return QString();
            case OTemplateOptionKind::Logo:
                return QString();
            case OTemplateOptionKind::TextSource:
                return OTemplateTextSource::kHere;
            case OTemplateOptionKind::Text:
                return QString();
            case OTemplateOptionKind::SizeCase:
                return OTemplateSizeCase::kSingle;
            case OTemplateOptionKind::LogoSize:
                return kDefaultLogoSize;
            case OTemplateOptionKind::Multilingual:
                return false;
            // A yes-or-no declared without a default is off: the catalogue that wants it on says
            // so in the field, which is where a question's own answer belongs.
            case OTemplateOptionKind::Boolean:
                return false;

            // Composites carry no value of their own: they expand into their sub-options, which
            // the declaring catalogue puts in the field. Reaching here means one was declared
            // without them, and an option with no default is the honest answer.
            case OTemplateOptionKind::Cut:
            case OTemplateOptionKind::AxisRename:
            case OTemplateOptionKind::SizeRange:
                return QVariant();

            // A rule is not a question, so it has no answer: it is a line drawn between groups of
            // fields, and the catalogue declares it to say where the groups part.
            case OTemplateOptionKind::Rule:
                return QVariant();
        }
        break;
    }
    return QVariant();
}

//---------------------------------------------------------------------------------------------------------------------

auto OTemplateSection::DefaultOptions() const -> QHash<QString, QVariant>
{
    QHash<QString, QVariant> options;
    for (const OTemplateOptionField &field : m_fields)
    {
        // A composite was told what it expands into, by whoever declared it. The section does not
        // recognise the kind — it reads the field.
        if (!field.compositeDefaults.isEmpty())
        {
            for (auto it = field.compositeDefaults.constBegin(); it != field.compositeDefaults.constEnd();
                 ++it)
            {
                options.insert(it.key(), it.value());
            }
            continue;
        }
        options.insert(field.key, DefaultValue(field.key));
    }
    return options;
}
