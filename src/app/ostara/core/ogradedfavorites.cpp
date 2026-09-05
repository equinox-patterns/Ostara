/************************************************************************
 **
 **  @file   ogradedfavorites.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Implementation of the graded favorites bridge.
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

#include "ogradedfavorites.h"

#include <QVariantMap>

#include "../vmisc/vcommonsettings.h"

namespace
{

constexpr auto kMin = "min";
constexpr auto kMax = "max";
constexpr auto kAxisValues = "axisValues";

} // namespace

namespace OGradedFavorites
{

//---------------------------------------------------------------------------------------------------------------------
auto FavoriteNames(const VCommonSettings *settings, const QString &canonicalPath,
                   const QString &axisName) -> QStringList
{
    if (settings == nullptr)
    {
        return {};
    }
    QStringList names = settings->GetGradedFavorites(canonicalPath, axisName).keys();
    names.sort();
    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto IsFavorite(const VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                const QString &name) -> bool
{
    if (settings == nullptr || name.isEmpty())
    {
        return false;
    }
    return settings->GetGradedFavorites(canonicalPath, axisName).contains(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto RangeFromFavorite(const VCommonSettings *settings, const QString &canonicalPath,
                       const QString &axisName, const QString &name) -> OSizerange
{
    OSizerange range;
    if (settings == nullptr)
    {
        return range;
    }

    const QVariantMap record = settings->GetGradedFavorites(canonicalPath, axisName).value(name).toMap();
    range.name = name;
    range.measurement = canonicalPath;
    range.sizeAxis = axisName;
    range.minValue = record.value(QLatin1String(kMin)).toString();
    range.maxValue = record.value(QLatin1String(kMax)).toString();

    const QVariantMap frozen = record.value(QLatin1String(kAxisValues)).toMap();
    for (auto it = frozen.cbegin(); it != frozen.cend(); ++it)
    {
        range.axisValues.insert(it.key(), it.value().toString());
    }
    return range;
}

//---------------------------------------------------------------------------------------------------------------------
auto StoreFavorite(VCommonSettings *settings, const OSizerange &range) -> bool
{
    if (settings == nullptr || not range.IsValid())
    {
        return false;
    }

    QVariantMap frozen;
    for (auto it = range.axisValues.cbegin(); it != range.axisValues.cend(); ++it)
    {
        frozen.insert(it.key(), it.value());
    }

    QVariantMap record;
    record.insert(QLatin1String(kMin), range.minValue);
    record.insert(QLatin1String(kMax), range.maxValue);
    record.insert(QLatin1String(kAxisValues), frozen);

    settings->SetGradedFavorite(range.measurement, range.sizeAxis, range.name, record);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void RemoveFavorite(VCommonSettings *settings, const OSizerange &range)
{
    if (settings == nullptr)
    {
        return;
    }
    settings->RemoveGradedFavorite(range.measurement, range.sizeAxis, range.name);
}

} // namespace OGradedFavorites
