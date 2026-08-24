/************************************************************************
 **
 **  @file   OPieceCartoucheSection.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/08/2026
 **
 **  @brief  One section type of the cartouche catalogue (catalogue entry).
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

#include "OPieceCartoucheSection.h"

namespace
{
/// Default reference logo size in cm (matches the VCommonSettings default).
constexpr double kDefaultLogoSize = 2.0;
}  // namespace

OPieceCartoucheSection::OPieceCartoucheSection(const QString &id, const QString &displayName,
                                               const QString &description,
                                               const QList<OPieceCartoucheOptionField> &fields)
  : m_id(id), m_displayName(displayName), m_description(description), m_fields(fields)
{
}

auto OPieceCartoucheSection::IsValid() const -> bool
{
    return !m_id.isEmpty();
}

auto OPieceCartoucheSection::Id() const -> QString
{
    return m_id;
}

auto OPieceCartoucheSection::DisplayName() const -> QString
{
    return m_displayName;
}

auto OPieceCartoucheSection::Description() const -> QString
{
    return m_description;
}

auto OPieceCartoucheSection::Fields() const -> const QList<OPieceCartoucheOptionField> &
{
    return m_fields;
}

auto OPieceCartoucheSection::HasField(const QString &key) const -> bool
{
    for (const OPieceCartoucheOptionField &field : m_fields)
    {
        if (field.key == key)
        {
            return true;
        }
    }
    return false;
}

auto OPieceCartoucheSection::DefaultValue(const QString &key) const -> QVariant
{
    for (const OPieceCartoucheOptionField &field : m_fields)
    {
        if (field.key != key)
        {
            continue;
        }
        switch (field.kind)
        {
            case OPieceCartoucheOptionKind::UseDefaults:
                return true;
            case OPieceCartoucheOptionKind::FontFamily:
                return QString();
            case OPieceCartoucheOptionKind::FontWeight:
                return 0;
            case OPieceCartoucheOptionKind::FontSize:
                return 0.0;
            case OPieceCartoucheOptionKind::Axis:
                return QStringLiteral("X");
            case OPieceCartoucheOptionKind::MeasureName:
                return QString();
            case OPieceCartoucheOptionKind::Logo:
                return QString();
            case OPieceCartoucheOptionKind::LogoWidth:
                return kDefaultLogoSize;
            case OPieceCartoucheOptionKind::LogoHeight:
                return kDefaultLogoSize;
        }
        break;
    }
    return QVariant();
}

auto OPieceCartoucheSection::DefaultOptions() const -> QHash<QString, QVariant>
{
    QHash<QString, QVariant> options;
    for (const OPieceCartoucheOptionField &field : m_fields)
    {
        options.insert(field.key, DefaultValue(field.key));
    }
    return options;
}
