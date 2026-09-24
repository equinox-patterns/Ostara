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
#include <QRectF>
#include <QString>
#include <QVector>

#include <functional>

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

/// @brief Un rectangle du tissu plié : où il est dans la colonne, et de quel côté son pli.
///
/// **Le tissu plié à deux bords n'est pas un bac, c'est trois.** Les lisières gauche et droite
/// repliées vers l'intérieur font des deux grands côtés de la pile deux plis, et des lisières une
/// ligne au milieu : les pièces au pli se collent donc à l'un ou à l'autre, et une pièce ne peut
/// pas traverser la ligne des lisières.
///
///   +----------+----------+     L : pli sur le bord GAUCHE
///   |    L     |    R     |     R : pli sur le bord DROIT
///   +----------+----------+     la ligne du milieu : les deux lisières
///   |         B           |     B : pli sur le bord GAUCHE
///   +---------------------+     (le pli simple, sous les deux bandes)
///
/// Vide sur un plan à bord unique : la colonne est d'un seul tenant, et le canvas dessine
/// exactement ce qu'il dessinait avant.
struct OExportFoldedZone
{
    /// Pixels, dans le repère de la colonne — celui du bac, (0, 0) en haut à gauche.
    QRectF rect{};

    /// Le côté du rectangle où le tissu est plié. L'autre côté est une lisière (ou une cassure de
    /// lisière), donc une matière à ne pas traverser.
    bool foldOnTheLeft{true};
};

/// @brief The result of one nesting run.
struct OExportNestingResult
{
    bool ok{false};

    /// @brief The run was given up at the caller's request, and what it holds means nothing.
    ///
    /// **Not a failure, and never reported as one.** The user asked for the nesting to stop, and a
    /// message saying it did not work would be the application arguing with a decision that was
    /// theirs. Everything else in the result is a half-built plan and is to be discarded.
    bool abandoned{false};

    QString error{};
    QVector<QString> keys{}; ///< footprint key, index-aligned with placements
    QVector<OExportPlacement> placements{};
    double binWidthPx{0.0};
    double usedHeightPx{0.0};

    /// Les rectangles du tissu plié à deux bords, et rien sur un bord unique. **Le dessin du
    /// découpage** : ce sont eux que le canvas trace en pointillé, pour que l'utilisatrice
    /// comprenne comment plier son tissu.
    QVector<OExportFoldedZone> foldedZones{};
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

    /// @brief Whether this run is a cutting plan.
    ///
    /// It is the one thing that separates the two runs: a cutting plan lays pieces on fabric, whose
    /// grain the pieces must follow, so each one is pinned to the rotations its direction allows.
    /// **A print run is not**, and false here leaves the engine exactly as it was — free to turn
    /// every piece to any of its 180 orientations.
    bool cuttingPlan{false};

    /// @brief « Sens unique » — aucune pièce n'est retournée d'un demi-tour.
    ///
    /// Retire un **sens**, pas un axe : une pièce « Double » garde le droit-fil ET le contre-fil.
    /// Voir OExportCuttingDirection::AllowedAngles, qui en fait tout le calcul.
    bool oneWayLayout{false};

    /// @brief Whether the cutting plan is laid on fabric folded in two.
    ///
    /// It changes what the engine is ASKED here, and only here: a piece cut on the fold gets the one
    /// rotation that lays its fold line on the fabric's fold, and it is **glued to that fold edge**
    /// instead of being placed freely across the bin. Both are things only this file can say — it is
    /// the one place that holds the piece's own geometry and the list the engine receives, in the
    /// order it receives it.
    /// @see OExportCuttingDirection::FoldAngle, Anchor
    bool foldedFabric{false};

    // The artboard grid — two values that are NOT lengths, unlike the three
    // above, even though the third is a length like them. Zero on both means a
    // run that is not tiled (the roll, A0), and the engine then places exactly
    // as it did before tiling existed: the grid is what turns the adjustment
    // pass on, and nothing else does.
    int    artboardCount{0};   ///< frames side by side across the bin width
    double artboardHeightPx{0.0}; ///< frame height, converted to pixels like the rest

    /// The block the cover page reserves, in PIXELS from the bin's own top-left corner — empty
    /// when the run carries no cover page, or when its format gives the page a sheet of its own.
    ///
    /// **Empty is not a small number.** Nothing is reserved, `Bin` gets an empty zone list, and
    /// both engines then place exactly as they did before zones could be handed to them: the
    /// whole of the no-regression argument, stated as a default.
    ///
    /// A rectangle rather than the seat, because the seat is the format's business
    /// (OExportCoverPage) and the engine is told what it needs and nothing more.
    QRectF coverZonePx{};

    /// @brief Asked by the engine between its iterations: true means the run is to be given up.
    ///
    /// **Handed over rather than decided here**, because who wants the run stopped is the one
    /// watching it — the dialog in front of the person who closed it. Null leaves the engine with
    /// nobody to ask. See Engine::Config::abandoned, which this becomes.
    std::function<bool()> abandoned{};
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
