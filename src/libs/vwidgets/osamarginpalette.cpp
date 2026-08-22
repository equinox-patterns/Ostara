/************************************************************************
 **
 **  @file   osamarginpalette.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/08/2026
 **
 **  @brief  Theme-aware color palette for seam-allowance margin highlights.
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

#include "osamarginpalette.h"

#include "../ifc/ifcdef.h"
#include "../vtheme/vtheme.h"

#include <array>

namespace
{
// Pen-style color keys (the string values of the ColorXxx constants from
// ifcdef.h) in display order, excluding Black/White. The array is sized by
// OSAMarginPalette::kColorCount, so the two can never drift: adding or removing
// an entry without updating kColorCount is a compile-time error.
constexpr std::array<const char *, OSAMarginPalette::kColorCount> kColorKeys = {
    "brown", "darkBlue", "darkGreen", "lightBlue", "lightGreen",
    "orange", "pink", "purple", "yellow"
};
} // namespace

namespace OSAMarginPalette
{
auto ColorNames() -> const QStringList &
{
    // Function-local static: built on first call, after all globals are
    // initialized, so it never depends on cross-TU dynamic-init order.
    static const QStringList names = [] {
        QStringList result;
        result.reserve(kColorKeys.size());
        for (const char *key : kColorKeys)
        {
            result.append(QString::fromLatin1(key));
        }
        return result;
    }();
    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto Colors() -> QVector<QColor>
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    QVector<QColor> colors;
    colors.reserve(kColorCount);
    for (const QString &name : ColorNames())
    {
        colors.append(ColorNameToQColorForTheme(name, dark));
    }
    return colors;
}

//---------------------------------------------------------------------------------------------------------------------
auto Color(int index) -> QColor
{
    const int clamped = (index >= 0 && index < kColorCount) ? index : 0;
    return ColorNameToQColorForTheme(ColorNames().at(clamped), VTheme::ColorSheme() == VColorSheme::Dark);
}
} // namespace OSAMarginPalette
