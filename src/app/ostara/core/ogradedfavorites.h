/************************************************************************
 **
 **  @file   ogradedfavorites.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Typed bridge between a graded size range (OSizerange) and the
 **          software-level favorites store (VCommonSettings, keyed by the
 **          measurements-table canonical path + size axis). Favorites never
 **          live in the .omt / .ostara.
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

#ifndef OGRADEDFAVORITES_H
#define OGRADEDFAVORITES_H

#include <QStringList>

#include "../vpatterndb/osizerange.h"

class VCommonSettings;

/**
 * @brief Software-level favorites for the graded size ranges.
 *
 * A favorite is the DEFINITION of a range (name, min/max, frozen values of the
 * other axes), stored once per measurements table (canonical path) + size axis.
 * It is a model reused by every pattern that loads the same table.
 */
namespace OGradedFavorites
{

/** @brief Names of the favorites stored for (canonical, axis), sorted. */
auto FavoriteNames(const VCommonSettings *settings, const QString &canonicalPath,
                   const QString &axisName) -> QStringList;

/** @brief True when a favorite named @p name exists for (canonical, axis). */
auto IsFavorite(const VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                const QString &name) -> bool;

/** @brief Build a range definition from a stored favorite. Invalid when missing. */
auto RangeFromFavorite(const VCommonSettings *settings, const QString &canonicalPath,
                       const QString &axisName, const QString &name) -> OSizerange;

/** @brief Save (add or update) the favorite matching @p range's definition.
 *         Name uniqueness must be checked by the caller. */
auto StoreFavorite(VCommonSettings *settings, const OSizerange &range) -> bool;

/** @brief Remove the favorite matching @p range (by name + table + axis). */
void RemoveFavorite(VCommonSettings *settings, const OSizerange &range);

} // namespace OGradedFavorites

#endif // OGRADEDFAVORITES_H
