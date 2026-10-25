/************************************************************************
 **
 **  @file   oexportfavorites.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Typed bridge between an Export-mode run (OExportRunRequest) and the
 **          software-level favorites store (VCommonSettings, keyed by the measurements-table
 **          canonical path + size axis).
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

#ifndef OEXPORTFAVORITES_H
#define OEXPORTFAVORITES_H

#include <QSet>
#include <QString>
#include <QVector>

#include "oexportoutput.h"

class VCommonSettings;

/**
 * @brief One stored favorite: the name it was starred under, and the run it replays.
 *
 * The type IS a field now — `format`, stored as a token (`print`, `cutting`, `plotter`,
 * `projector`). It used to be read from the request's two flags, one per format, and the third
 * format is what made that untenable: two flags describe four formats only by agreeing never to
 * be true together. A record written back then carries no token, and its flags are read instead —
 * see `OExportFormat::FromLegacyFlags` — so no favourite is lost.
 *
 * The per-piece choices (`groupMemberChoices`, `excludedPieces`) are deliberately NOT in the
 * recipe: a favorite is a layout, and which piece of a group is cut — or which members of a
 * group the plotter keeps — is decided for the batch, once, in the pre-flight dialog. A
 * favorite that carried them would be a favorite that only meant anything in the pattern it
 * was starred in, which is the opposite of what a favorite is for.
 */
struct OExportFavorite
{
    QString name{};
    OExportRunRequest request{};
};

/**
 * @brief Software-level favorites for the Export mode.
 *
 * A favorite is the RECIPE of a run — everything a generation needs, minus what depends on
 * the pieces of one pattern. It is stored once per measurements table (canonical path) and
 * size axis, exactly like a graded size-range favorite, so every pattern loading the same
 * table offers the same ones.
 *
 * Nothing here ever reaches the .ostara: a favorite is a model, and a model filed in the
 * document would read as pattern data.
 */
namespace OExportFavorites
{
/** @brief Every favorite stored for (canonical, axis), by name, sorted. */
auto All(const VCommonSettings *settings, const QString &canonicalPath, const QString &axisName)
    -> QVector<OExportFavorite>;

/** @brief The names of every favorite stored for (canonical, axis).
 *
 * Handed out as a set because that is how its one caller reads it: once per dock refresh,
 * to know which rows wear a full star — a per-row lookup would re-open the settings file
 * for every line of the dock. */
auto Names(const VCommonSettings *settings, const QString &canonicalPath, const QString &axisName)
    -> QSet<QString>;

/** @brief True when a favorite named @p name exists for (canonical, axis). */
auto IsFavorite(const VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                const QString &name) -> bool;

/** @brief The recipe stored under @p name. Defaults when there is none. */
auto RequestFromFavorite(const VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                         const QString &name) -> OExportRunRequest;

/** @brief Save (add or update) the recipe of @p request under @p name. */
auto StoreFavorite(VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                   const QString &name, const OExportRunRequest &request) -> bool;

/** @brief Remove the favorite named @p name. */
void RemoveFavorite(VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                    const QString &name);

/** @brief Make every recipe of (canonical, axis) point at @p newName instead of @p oldName.
 *
 * What a size-range rename has to propagate: a recipe names the range it was built on
 * (`rangeName`), that name is resolved against the pattern at replay time, and a range that
 * was renamed is a range the recipe would no longer find. The favorite's own NAME is left
 * alone — it is the label the user starred, not a reference. */
void RenameRange(VCommonSettings *settings, const QString &canonicalPath, const QString &axisName,
                 const QString &oldName, const QString &newName);

}  // namespace OExportFavorites

#endif  // OEXPORTFAVORITES_H
