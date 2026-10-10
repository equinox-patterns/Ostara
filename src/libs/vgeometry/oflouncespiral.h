/************************************************************************
 **
 **  @file   oflouncespiral.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  The flounce spiral — the "snail" a flounce is cut from. Two
 **          parallel spirals a band width apart, joined at both ends by a
 **          straight segment, so that the band closes on itself.
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

#ifndef OFLOUNCESPIRAL_H
#define OFLOUNCESPIRAL_H

#include <QPointF>
#include <QVector>
#include <QtGlobal>

#include "vsplinepath.h"

class VSplinePath;

/**
 * @brief The flounce spiral, as geometry.
 *
 * The figure answers one question: given the point the inner spiral starts at, the width of the band and the
 * length wanted for the inner edge, where do the two spirals go? The outer edge is longer than the inner
 * one by the band width times the angle swept, in radians — being parallel, it takes the outside of every
 * turn.
 *
 * The construction is the one of the dressmaking books, and it is exact rather than approximate. Two centres A
 * and B sit half a band width apart. Half circles are drawn about them in turn, the radius growing by that
 * same half width at every step: q, then 2q, 3q and so on. The locus is an Archimedean spiral, @c r(t) = q +
 * (q/pi) * t, whose offset by the band width is another Archimedean spiral of the same pitch. That is what
 * makes the two edges parallel to the millimetre rather than approximately: the band width IS the pitch.
 *
 * Because the pitch is the band width, consecutive turns of the band touch. Nothing may be added on the edges
 * afterwards, which is why the seam and hem allowances belong in the two values the user types, and why this
 * curve is never a stitch line.
 *
 * The length is what decides where the spiral stops, and it stops in the middle of a turn when that is where
 * the length falls. The measure is taken on the spline really emitted, not on the law, so what the user asked
 * for is what the user gets — the application's own measure, the one the tooltip and the formulas report.
 *
 * The two edges meet at the start point: the outer spiral comes back to it. Its last half circle is centred on
 * the start point itself, with the band width for radius, so the point lies on it. The band is closed, and the
 * pointed tip the books leave at the eye falls out of the construction instead of being drawn by hand.
 *
 * Angle convention, in the canonical frame anchored on the start point: A sits at 180 degrees from the start
 * point for the default reading, and a positive angle turns the whole figure about that point, counter-
 * clockwise on screen, the convention the application's angles already use.
 */
class OFlounceSpiral
{
public:
    OFlounceSpiral() = delete;

    /// Angular step of the sampling: 32 samples per turn, fine enough for the emitted spline to be smooth.
    static constexpr qreal kSamplesPerTurn = 32.0;

    /// Samples per half turn of the construction, so that a short spiral is as well described as a long one.
    static constexpr int kSamplesPerHalfTurn = 16;

    /// Ceiling on the sample count, so that a very long spiral cannot ask for an unreasonable chain.
    static constexpr int kMaxSamples = 4096;

    /// Length resolution of the emitted spline, in the pattern unit. A hundredth of a millimetre.
    static constexpr qreal kLengthTolerance = 0.01;

    /// Angle resolution of the search for the stop, in radians.
    static constexpr qreal kAngleResolution = 1e-9;

    /// The figure, as plain geometry. Points are absolute, the two paths carry no name of their own.
    struct Figure
    {
        VSplinePath begin; ///< the first turn of the inner edge: from the start point to the outer end
        VSplinePath outer; ///< the outer edge, from the outer start back to the outer end

        QPointF start;   ///< the point the inner spiral starts at — the one the user clicked
        QPointF outerStart; ///< start of the outer edge, one band width away from @c start
        QPointF innerEnd;   ///< end of the inner edge
        QPointF outerEnd;   ///< end of the outer edge — the point the outer spiral comes back to

        qreal bandWidth = 0.0; ///< width of the band
        qreal innerLength = 0.0; ///< arc length actually emitted for the inner edge
        qreal outerLength = 0.0; ///< arc length actually emitted for the outer edge
        qreal sweptAngle = 0.0; ///< angle swept by the construction, in radians
        qreal finalRadius = 0.0; ///< distance from the eye to the end of the inner edge
    };

    /// The two names that stand at the ends of one edge. A path is named after the two points it runs
    /// between — @c OC2Chain::Chain carries the name of its first and last knot, and the tooltip, the history
    /// and the aliases all read them — so an edge whose two ends carry the same name is a path the user cannot
    /// tell from its neighbour.
    struct EdgeNames
    {
        QString first; ///< name of the point the edge starts at
        QString last;  ///< name of the point the edge ends at
    };

    /**
     * @brief Build the flounce spiral.
     *
     * @param start      point the inner spiral starts at. The inner edge begins there and the outer edge comes
     *                   back to it.
     * @param bandWidth  width of the band, seam and hem allowances included. Must be positive; a smaller value
     *                   is raised to the smallest the geometry can hold.
     * @param length     arc length wanted for the inner edge, allowances included. Must be positive.
     * @param angle      direction of A from the start point, in degrees. Positive turns counter-clockwise on
     *                   screen. 180 degrees, the default reading, puts A to the left of the start point.
     * @param begin      names of the two ends of the first curve: the start point, then the OUTER end — the point
     *                   one turn from the start, where the single spiral is cut.
     * @param outer      names of the two ends of the outer edge: the outer start, then the outer end. The two
     *                   curves share nothing but that outer end.
     * @return the figure. Both paths are empty only when the width or the length is not usable.
     */
    static auto Build(const QPointF &start, qreal bandWidth, qreal length, qreal angle, const EdgeNames &begin,
                      const EdgeNames &outer) -> Figure;

    /// Arc length of the inner edge after @p halfTurns complete half turns, for a half width @p q.
    static auto InnerLength(qreal q, int halfTurns) -> qreal;

    /// Radius reached by the inner edge after @p halfTurns complete half turns.
    static auto InnerRadius(qreal q, int halfTurns) -> qreal;
};

#endif // OFLOUNCESPIRAL_H
