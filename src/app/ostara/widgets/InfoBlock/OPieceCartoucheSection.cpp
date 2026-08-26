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

OPieceCartoucheSection::OPieceCartoucheSection(const QString &id, const QString &sourceName,
                                               const QString &displayName, const QString &description,
                                               const QList<OPieceCartoucheOptionField> &fields)
  : m_id(id), m_sourceName(sourceName), m_displayName(displayName), m_description(description), m_fields(fields)
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

auto OPieceCartoucheSection::SourceName() const -> QString
{
    return m_sourceName;
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
        if (field.defaultValue.isValid())
        {
            return field.defaultValue;
        }
        switch (field.kind)
        {
            case OPieceCartoucheOptionKind::FontFamily:
                return QString();
            case OPieceCartoucheOptionKind::FontWeight:
                return 0;
            case OPieceCartoucheOptionKind::FontSize:
                return 0.0;
            case OPieceCartoucheOptionKind::LineSpacing:
                return 0;
            case OPieceCartoucheOptionKind::Axis:
                return QString();
            case OPieceCartoucheOptionKind::MeasureName:
                return QString();
            case OPieceCartoucheOptionKind::Separator:
                return QString();
            case OPieceCartoucheOptionKind::Logo:
                return QString();
            case OPieceCartoucheOptionKind::LogoSize:
                return kDefaultLogoSize;
            case OPieceCartoucheOptionKind::Cut:
                return QVariant();  // handled in DefaultOptions (composite sub-keys)
            case OPieceCartoucheOptionKind::Multilingual:
                return false;
            case OPieceCartoucheOptionKind::AxisRename:
                return QVariant();  // handled in DefaultOptions (composite sub-keys)
        }
        break;
    }
    return QVariant();
}

auto ParseAxisRenames(const QVariant &var) -> QMap<QString, OAxisRename>
{
    QMap<QString, OAxisRename> result;
    const QVariantMap map = var.toMap();
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
    {
        const QVariantMap entry = it.value().toMap();
        OAxisRename rename;
        rename.name = entry.value(QStringLiteral("name")).toString();
        const QVariantList list = entry.value(QStringLiteral("values")).toList();
        rename.values.reserve(list.size());
        for (const QVariant &value : list)
        {
            rename.values.append(value.toString());
        }
        result.insert(it.key(), rename);
    }
    return result;
}

auto MakeAxisRenames(const QMap<QString, OAxisRename> &renames) -> QVariant
{
    QVariantMap map;
    for (auto it = renames.constBegin(); it != renames.constEnd(); ++it)
    {
        QVariantMap entry;
        entry.insert(QStringLiteral("name"), it.value().name);
        QVariantList list;
        list.reserve(it.value().values.size());
        for (const QString &value : it.value().values)
        {
            list.append(value);
        }
        entry.insert(QStringLiteral("values"), list);
        map.insert(it.key(), entry);
    }
    return map;
}

auto OPieceCartoucheSection::DefaultOptions() const -> QHash<QString, QVariant>
{
    QHash<QString, QVariant> options;
    for (const OPieceCartoucheOptionField &field : m_fields)
    {
        if (field.kind == OPieceCartoucheOptionKind::Cut)
        {
            // The Cut section expands into its six per-fabric sub-options.
            // Fabric is pre-filled (1 + on fold) so a newly added Cut section
            // shows an immediate preview — template-level example only; pieces
            // start empty (spec §24).
            options.insert(OPieceCartoucheCut::kFabricQty, 1);
            options.insert(OPieceCartoucheCut::kFabricOnFold, true);
            options.insert(OPieceCartoucheCut::kInterfacingQty, 0);
            options.insert(OPieceCartoucheCut::kInterfacingOnFold, false);
            options.insert(OPieceCartoucheCut::kLiningQty, 0);
            options.insert(OPieceCartoucheCut::kLiningOnFold, false);
            continue;
        }
        if (field.kind == OPieceCartoucheOptionKind::AxisRename)
        {
            // Composite: no aliases + one-line-per-language off by default.
            options.insert(OPieceCartoucheAxisRename::kRenames, QVariantMap{});
            options.insert(OPieceCartoucheAxisRename::kOneLine, false);
            continue;
        }
        options.insert(field.key, DefaultValue(field.key));
    }
    return options;
}
