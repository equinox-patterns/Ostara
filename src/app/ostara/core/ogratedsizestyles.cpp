/************************************************************************
 **
 **  @file   ogratedsizestyles.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Implementation of the graded size styles bridge.
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

#include "ogratedsizestyles.h"

#include <QVariantList>
#include <QVariantMap>

#include <cmath>

#include "../vmisc/vcommonsettings.h"

namespace
{

constexpr auto kLayerName = "layerName";
constexpr auto kColor = "color";
constexpr auto kDash = "dash";

//---------------------------------------------------------------------------------------------------------------------
auto ParseStyle(const QVariant &variant) -> OGradedSizeStyle
{
    OGradedSizeStyle style;
    const QVariantMap record = variant.toMap();

    style.layerName = record.value(QLatin1String(kLayerName)).toString();

    const QString colorText = record.value(QLatin1String(kColor)).toString();
    if (!colorText.isEmpty())
    {
        QColor color(colorText);
        if (!color.isValid() && !colorText.startsWith(QLatin1Char('#')))
        {
            color = QColor(QStringLiteral("#") + colorText); // accept "RRGGBB" too
        }
        if (color.isValid())
        {
            style.color = color;
        }
    }

    // Defensive read: the software store is hand-editable, so only keep finite,
    // strictly positive values (a 0/negative/NaN dash would poison QPen).
    const QVariantList dashes = record.value(QLatin1String(kDash)).toList();
    constexpr int kMaxDashValues = 6; // three (dash, space) pairs
    for (const QVariant &value : dashes)
    {
        if (style.dashes.size() >= kMaxDashValues)
        {
            break;
        }
        const qreal dash = value.toReal();
        if (dash > 0.0 && std::isfinite(dash))
        {
            style.dashes.append(dash);
        }
    }
    if (style.dashes.size() % 2 != 0)
    {
        style.dashes.removeLast(); // Qt requires an even [dash, space, ...] pattern
    }
    return style;
}

//---------------------------------------------------------------------------------------------------------------------
auto SerializeStyle(const OGradedSizeStyle &style) -> QVariantMap
{
    QVariantMap record;
    if (style.HasColor())
    {
        record.insert(QLatin1String(kColor), style.color.name()); // #RRGGBB
    }
    if (!style.layerName.isEmpty())
    {
        record.insert(QLatin1String(kLayerName), style.layerName);
    }
    if (!style.dashes.isEmpty())
    {
        QVariantList dashes;
        dashes.reserve(static_cast<int>(style.dashes.size()));
        for (qreal value : style.dashes)
        {
            dashes.append(value);
        }
        record.insert(QLatin1String(kDash), dashes);
    }
    return record;
}

} // namespace

namespace OGradedSizeStyles
{

//---------------------------------------------------------------------------------------------------------------------
auto StylesForAxis(const VCommonSettings *settings, const QString &canonicalPath,
                   const QString &axisName) -> QHash<QString, OGradedSizeStyle>
{
    QHash<QString, OGradedSizeStyle> styles;
    if (settings == nullptr)
    {
        return styles;
    }

    const QVariantMap stored = settings->GetGradedSizeStyles(canonicalPath, axisName);
    styles.reserve(static_cast<int>(stored.size()));
    for (auto it = stored.cbegin(); it != stored.cend(); ++it)
    {
        styles.insert(it.key(), ParseStyle(it.value()));
    }
    return styles;
}

//---------------------------------------------------------------------------------------------------------------------
void SaveAxisStyles(VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                    const QHash<QString, OGradedSizeStyle> &styles)
{
    if (settings == nullptr)
    {
        return;
    }

    QVariantMap stored;
    for (auto it = styles.cbegin(); it != styles.cend(); ++it)
    {
        if (it->HasContent())
        {
            stored.insert(it.key(), SerializeStyle(it.value()));
        }
    }
    settings->SetGradedSizeStyles(canonicalPath, axisName, stored);
}

} // namespace OGradedSizeStyles
