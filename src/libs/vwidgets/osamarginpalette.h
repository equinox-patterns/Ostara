/************************************************************************
 **
 **  @file   osamarginpalette.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/08/2026
 **
 **  @brief  Theme-aware color palette for seam-allowance margin highlights
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

#ifndef OSAMARGINPALETTE_H
#define OSAMARGINPALETTE_H

#include <QColor>
#include <QStringList>
#include <QVector>

/**
 * @brief OSAMarginPalette is the single source of truth for the seam-allowance
 *        margin highlight colors: the 9 pen-style colors (excluding Black/White),
 *        each with a light and a dark rendering.
 *
 * The palette is positional — `colorIndex` values in the XML and in the saved
 * settings index into ColorNames(). Resolved colors follow the current theme
 * (VTheme::ColorSheme()); Color()/Colors() re-resolve on every call and must
 * never be cached.
 */
namespace OSAMarginPalette
{
constexpr int kColorCount = 9;

auto ColorNames() -> const QStringList &; // pen color names, in display order
auto Colors() -> QVector<QColor>;         // resolved for the current theme
auto Color(int index) -> QColor;          // resolved for the current theme; clamps to 0
} // namespace OSAMarginPalette

#endif // OSAMARGINPALETTE_H
