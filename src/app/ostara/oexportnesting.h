/************************************************************************
 **
 **  @file   oexportnesting.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — feed the Trame nesting engine with the pattern's
 **          own geometry and bring back the placements.
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

#ifndef OEXPORTNESTING_H
#define OEXPORTNESTING_H

#include <QString>
#include <QVector>

struct OExportSizeLayer;

/// @brief One placed footprint.
///
/// The engine places a footprint by the min corner of its unrotated polygon, so
/// `originX/originY` are needed to rebuild the transform that puts the piece back
/// where the nesting decided: translate(x, y) · rotate(rotation) · translate(-originX, -originY).
struct OExportPlacement
{
    double x{0.0};        ///< pixels
    double y{0.0};        ///< pixels
    double rotation{0.0}; ///< degrees
    double originX{0.0};  ///< pixels — min-bbox corner of the footprint
    double originY{0.0};  ///< pixels
};

/// @brief The result of one nesting run.
struct OExportNestingResult
{
    bool ok{false};
    QString error{};
    QVector<QString> keys{}; ///< footprint key, index-aligned with placements
    QVector<OExportPlacement> placements{};
    double binWidthPx{0.0};
    double usedHeightPx{0.0};
};

/// @brief Runs the Trame engine on geometry that never left the application.
///
/// Builds the engine's input from the resolved pieces — one footprint per piece,
/// or per group, covering every size of the range — and places them in a bin of
/// the imposed width.
///
/// Deliberately absent: per-piece constraints (forbid/force flipping, grainline,
/// symmetrical copy) and quantities. The engine does not know them; the mode will
/// grow into them.
class OExportNesting
{
public:
    /// @brief Nest the given layers in a bin of the imposed width.
    /// @param layers      the pattern resolved for every size of the range
    /// @param binWidthPx  imposed bin width, in pixels
    static auto Run(const QVector<OExportSizeLayer> &layers, double binWidthPx) -> OExportNestingResult;
};

#endif // OEXPORTNESTING_H
