/************************************************************************
 **
 **  @file   oexportfavorites.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Implementation of the Export-mode favorites bridge.
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

#include "oexportfavorites.h"

#include <QVariantMap>

#include "../vmisc/vcommonsettings.h"

namespace
{

// The field names of the stored record, spelled once: the writer and the reader are the two
// functions below, and a field added to OExportRunRequest and forgotten here would be a field
// a favorite silently drops.
constexpr auto kDocumentWidth = "documentWidthMm";
constexpr auto kDocumentLiningWidth = "documentLiningWidthMm";
constexpr auto kLiningPlan = "liningPlan";
constexpr auto kUnit = "unit";
constexpr auto kRangeName = "rangeName";
constexpr auto kSingleSize = "singleSize";
constexpr auto kBaseA = "baseA";
constexpr auto kBaseB = "baseB";
constexpr auto kBaseC = "baseC";
constexpr auto kMargin = "marginMm";
constexpr auto kGap = "gapMm";
constexpr auto kPrintModelToken = "printModelToken";
constexpr auto kColumns = "columns";
constexpr auto kFrameHeight = "frameHeightMm";
constexpr auto kMarksTemplate = "marksTemplateName";
constexpr auto kCoverPageTemplate = "coverPageTemplateName";
constexpr auto kPinnedAxes = "pinnedAxes";
constexpr auto kCuttingPlan = "cuttingPlan";
constexpr auto kOneWayLayout = "oneWayLayout";
constexpr auto kFoldedFabric = "foldedFabric";
constexpr auto kPlotterPlan = "plotterPlan";
constexpr auto kMatWidth = "matWidthMm";
constexpr auto kMatHeight = "matHeightMm";
constexpr auto kMatUnit = "matUnit";

/// @brief The recipe of @p request, as it is stored.
///
/// Everything the run asks for, except the two fields that speak of the pieces of one
/// pattern: `groupMemberChoices` and `excludedPieces`. They are asked again, once for the
/// batch, in the pre-flight dialog — see OExportFavorite.
auto RecipeOf(const OExportRunRequest &request) -> QVariantMap
{
    QVariantMap pinned;
    for (auto it = request.pinnedAxes.cbegin(); it != request.pinnedAxes.cend(); ++it)
    {
        pinned.insert(it.key(), it.value());
    }

    QVariantMap record;
    record.insert(QLatin1String(kDocumentWidth), request.documentWidthMm);
    record.insert(QLatin1String(kDocumentLiningWidth), request.documentLiningWidthMm);
    record.insert(QLatin1String(kLiningPlan), request.liningPlan);
    record.insert(QLatin1String(kUnit), static_cast<int>(request.unit));
    record.insert(QLatin1String(kRangeName), request.rangeName);
    record.insert(QLatin1String(kSingleSize), request.singleSize);
    record.insert(QLatin1String(kBaseA), request.baseA);
    record.insert(QLatin1String(kBaseB), request.baseB);
    record.insert(QLatin1String(kBaseC), request.baseC);
    record.insert(QLatin1String(kMargin), request.marginMm);
    record.insert(QLatin1String(kGap), request.gapMm);
    record.insert(QLatin1String(kPrintModelToken), request.printModelToken);
    record.insert(QLatin1String(kColumns), request.columns);
    record.insert(QLatin1String(kFrameHeight), request.frameHeightMm);
    record.insert(QLatin1String(kMarksTemplate), request.marksTemplateName);
    record.insert(QLatin1String(kCoverPageTemplate), request.coverPageTemplateName);
    record.insert(QLatin1String(kPinnedAxes), pinned);
    record.insert(QLatin1String(kCuttingPlan), request.cuttingPlan);
    record.insert(QLatin1String(kOneWayLayout), request.oneWayLayout);
    record.insert(QLatin1String(kFoldedFabric), request.foldedFabric);
    record.insert(QLatin1String(kPlotterPlan), request.plotterPlan);
    record.insert(QLatin1String(kMatWidth), request.matWidthMm);
    record.insert(QLatin1String(kMatHeight), request.matHeightMm);
    record.insert(QLatin1String(kMatUnit), static_cast<int>(request.matUnit));
    return record;
}

/// @brief The request a stored @p record describes.
///
/// Read against the request's own defaults rather than against zero: a record written by an
/// older version is missing whatever was added since, and the default of the field is the
/// honest answer — a margin of 5 mm, not a margin of 0.
auto RequestOf(const QVariantMap &record) -> OExportRunRequest
{
    OExportRunRequest request;

    request.documentWidthMm =
        record.value(QLatin1String(kDocumentWidth), request.documentWidthMm).toDouble();
    request.documentLiningWidthMm =
        record.value(QLatin1String(kDocumentLiningWidth), request.documentLiningWidthMm).toDouble();
    request.liningPlan = record.value(QLatin1String(kLiningPlan), request.liningPlan).toBool();
    request.unit = static_cast<Unit>(record.value(QLatin1String(kUnit), static_cast<int>(request.unit)).toInt());
    request.rangeName = record.value(QLatin1String(kRangeName), request.rangeName).toString();
    request.singleSize = record.value(QLatin1String(kSingleSize), request.singleSize).toBool();
    request.baseA = record.value(QLatin1String(kBaseA), request.baseA).toDouble();
    request.baseB = record.value(QLatin1String(kBaseB), request.baseB).toDouble();
    request.baseC = record.value(QLatin1String(kBaseC), request.baseC).toDouble();
    request.marginMm = record.value(QLatin1String(kMargin), request.marginMm).toDouble();
    request.gapMm = record.value(QLatin1String(kGap), request.gapMm).toDouble();
    request.printModelToken = record.value(QLatin1String(kPrintModelToken), request.printModelToken).toString();
    request.columns = record.value(QLatin1String(kColumns), request.columns).toInt();
    request.frameHeightMm = record.value(QLatin1String(kFrameHeight), request.frameHeightMm).toDouble();
    request.marksTemplateName = record.value(QLatin1String(kMarksTemplate), request.marksTemplateName).toString();
    request.coverPageTemplateName =
        record.value(QLatin1String(kCoverPageTemplate), request.coverPageTemplateName).toString();
    request.cuttingPlan = record.value(QLatin1String(kCuttingPlan), request.cuttingPlan).toBool();
    request.oneWayLayout = record.value(QLatin1String(kOneWayLayout), request.oneWayLayout).toBool();
    request.foldedFabric = record.value(QLatin1String(kFoldedFabric), request.foldedFabric).toBool();
    request.plotterPlan = record.value(QLatin1String(kPlotterPlan), request.plotterPlan).toBool();
    request.matWidthMm = record.value(QLatin1String(kMatWidth), request.matWidthMm).toDouble();
    request.matHeightMm = record.value(QLatin1String(kMatHeight), request.matHeightMm).toDouble();
    request.matUnit =
        static_cast<Unit>(record.value(QLatin1String(kMatUnit), static_cast<int>(request.matUnit)).toInt());

    const QVariantMap pinned = record.value(QLatin1String(kPinnedAxes)).toMap();
    for (auto it = pinned.cbegin(); it != pinned.cend(); ++it)
    {
        request.pinnedAxes.insert(it.key(), it.value().toString());
    }

    return request;
}

}  // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavorites::All(const VCommonSettings *settings, const QString &canonicalPath,
                           const QString &axisName) -> QVector<OExportFavorite>
{
    if (settings == nullptr)
    {
        return {};
    }

    const QVariantMap stored = settings->GetExportFavorites(canonicalPath, axisName);
    QStringList names = stored.keys();
    names.sort();

    QVector<OExportFavorite> favorites;
    favorites.reserve(names.size());
    for (const QString &name : names)
    {
        favorites.append(OExportFavorite{name, RequestOf(stored.value(name).toMap())});
    }
    return favorites;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavorites::Names(const VCommonSettings *settings, const QString &canonicalPath,
                             const QString &axisName) -> QSet<QString>
{
    QSet<QString> names;
    if (settings == nullptr)
    {
        return names;
    }

    const QStringList stored = settings->GetExportFavorites(canonicalPath, axisName).keys();
    names.reserve(stored.size());
    for (const QString &name : stored)
    {
        names.insert(name);
    }
    return names;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavorites::IsFavorite(const VCommonSettings *settings, const QString &canonicalPath,
                                  const QString &axisName, const QString &name) -> bool
{
    if (settings == nullptr || name.isEmpty())
    {
        return false;
    }
    return settings->GetExportFavorites(canonicalPath, axisName).contains(name);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavorites::RequestFromFavorite(const VCommonSettings *settings, const QString &canonicalPath,
                                           const QString &axisName, const QString &name) -> OExportRunRequest
{
    if (settings == nullptr)
    {
        return {};
    }
    return RequestOf(settings->GetExportFavorites(canonicalPath, axisName).value(name).toMap());
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportFavorites::StoreFavorite(VCommonSettings *settings, const QString &canonicalPath,
                                     const QString &axisName, const QString &name,
                                     const OExportRunRequest &request) -> bool
{
    if (settings == nullptr || name.isEmpty() || canonicalPath.isEmpty())
    {
        return false;
    }

    settings->SetExportFavorite(canonicalPath, axisName, name, RecipeOf(request));
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavorites::RemoveFavorite(VCommonSettings *settings, const QString &canonicalPath,
                                      const QString &axisName, const QString &name)
{
    if (settings == nullptr || name.isEmpty())
    {
        return;
    }
    settings->RemoveExportFavorite(canonicalPath, axisName, name);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportFavorites::RenameRange(VCommonSettings *settings, const QString &canonicalPath,
                                   const QString &axisName, const QString &oldName, const QString &newName)
{
    if (settings == nullptr || oldName.isEmpty() || oldName == newName)
    {
        return;
    }

    // Walked over a snapshot: each write re-reads the file, and reading the map we are
    // rewriting would be reading our own leftovers.
    const QVariantMap stored = settings->GetExportFavorites(canonicalPath, axisName);
    for (auto it = stored.cbegin(); it != stored.cend(); ++it)
    {
        OExportRunRequest request = RequestOf(it.value().toMap());
        if (request.rangeName != oldName)
        {
            continue;
        }

        request.rangeName = newName;
        settings->SetExportFavorite(canonicalPath, axisName, it.key(), RecipeOf(request));
    }
}
