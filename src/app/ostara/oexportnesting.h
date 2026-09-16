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

#include <QPolygonF>
#include <QString>
#include <QVector>

struct OExportSizeLayer;

/// @brief One placed footprint.
///
/// The engine places a footprint by the min corner of its unrotated polygon, so
/// `originX/originY` are needed to rebuild the transform that puts the piece back
/// where the nesting decided: translate(x, y) · rotate(rotation) · translate(-originX, -originY).
/// That frame is the footprint's own — local (0, 0) is `origin` — so the origin and the
/// extent together are its four corners, and therefore its centre.
struct OExportPlacement
{
    double x{0.0};        ///< pixels
    double y{0.0};        ///< pixels
    double rotation{0.0}; ///< degrees
    double originX{0.0};  ///< pixels — min-bbox corner of the footprint
    double originY{0.0};  ///< pixels

    /// Pixels — the footprint's unrotated extent, measured FROM the origin. Not the piece's
    /// size: the union of every size of the piece, or of every member and every size of a
    /// group, which is the rectangle the engine reserved.
    ///
    /// Carried rather than measured again from the layers, because the engine keeps
    /// `unified[0]` of a real Clipper union (Footprint.cpp). Where two sizes of one piece are
    /// disjoint, the outline placed is not the naive union of their contours, and a second
    /// union computed elsewhere would answer a different rectangle.
    double extentX{0.0}; ///< pixels
    double extentY{0.0}; ///< pixels

    /// The footprint's own outline, in the frame the extent describes: origin at local (0, 0),
    /// pixels. What the shape IS, where extentX/extentY are only the box around it.
    ///
    /// A box is a safe place to draw the holder and an unfair one to measure against: its
    /// corners are empty, so a piece clamped by its box stops short of an edge the engine's
    /// own shape would have reached. The engine never reasoned on the box — it placed the
    /// polygon — and a hand correcting the plan is entitled to the same room.
    QPolygonF outline{};
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

/// @brief What the engine needs besides the geometry itself.
///
/// Three lengths, in the same pixels as the polygons — and grouped rather than passed in
/// a row, because two of them are the same order of magnitude and both are lengths. Handed
/// over positionally, a margin and a gap would swap without the compiler saying a word,
/// and the run would come out with the right shape and the wrong spacing.
struct OExportNestingParams
{
    double binWidthPx{0.0}; ///< imposed bin width — the height is what the nesting produces
    double marginPx{0.0};   ///< document margin, kept clear on every side of the bin
    double gapPx{0.0};      ///< space kept between two pieces

    // The artboard grid — two values that are NOT lengths, unlike the three
    // above, even though the third is a length like them. Zero on both means a
    // run that is not tiled (the roll, A0), and the engine then places exactly
    // as it did before tiling existed: the grid is what turns the adjustment
    // pass on, and nothing else does.
    int    artboardCount{0};   ///< frames side by side across the bin width
    double artboardHeightPx{0.0}; ///< frame height, converted to pixels like the rest
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
    /// @param layers the pattern resolved for every size of the range
    /// @param params the imposed width, and the two lengths that space the pieces out
    static auto Run(const QVector<OExportSizeLayer> &layers, const OExportNestingParams &params)
        -> OExportNestingResult;
};

#endif // OEXPORTNESTING_H
