/************************************************************************
 **
 **  @file   ospiral.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Golden spiral: the logarithmic spiral whose radius is
 **          multiplied by the golden ratio at every quarter turn, laid
 **          over a start point and stopped at an exact arc length.
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

#ifndef OSPIRAL_H
#define OSPIRAL_H

#include <QString>
#include <QtGlobal>

class VPointF;
class VSplinePath;

/**
 * @brief The golden spiral, as geometry.
 *
 * The curve answers one question: given a start point, the radius of its first quarter turn and the length
 * wanted, where does the spiral go? The length is what decides where it stops, and it stops in the middle of a
 * turn when that is where the length falls.
 *
 * The law is @c r(t) = r0 * q^(2t/pi) with @c q the golden ratio, so the radius is multiplied by q at every
 * quarter turn, and the arc length from the start is @c (sqrt(1+b^2)/b) * (r - r0) with @c b = 2*ln(q)/pi.
 * Both are closed forms: the point where the spiral stops is known before any curve is built.
 *
 * The curve really emitted is the cubic chain through samples of that law, not the law itself, so the stop is
 * solved on the emitted spline: the swept angle is bisected until the spline measures the length wanted. The
 * measure is the application's own — the one the tooltip and the formulas report — so what the user asked for
 * is what the user gets.
 *
 * 0 degree convention: the start point is the top left corner of the first square, the pole is its top right
 * corner, and the first quarter turn runs from the start point to the bottom right corner. A positive angle
 * turns the whole construction about the start point, counter-clockwise on screen, the convention the
 * application's angles already use.
 */
class OSpiral
{
public:
    OSpiral() = delete;

    /// Growth factor of the radius per quarter turn: the golden ratio.
    static constexpr qreal kGrowth = 1.618033988749894848;

    /// Angular step of the sampling: 32 samples per turn, fine enough for the emitted spline to be smooth.
    static constexpr qreal kSamplesPerTurn = 32.0;

    /**
     * @brief Build the spiral.
     *
     * @param start        point the spiral starts at, the eye of the spiral. Its name is the first half of the
     *                     native name of the result.
     * @param radius       radius of the first quarter turn, in the same unit as the coordinates (pixels).
     * @param length       arc length wanted, in the same unit as the coordinates (pixels). Exact to a hundredth
     *                     of a millimetre on the emitted spline.
     * @param angle        rotation about @p start, in degrees. Positive turns counter-clockwise on screen.
     * @param endPointName name of the last knot; the second half of the native name of the result.
     * @return the spiral as a VSplinePath, ordered from the eye outwards. Never empty for a positive radius and
     *         length; both are raised to the smallest value the geometry can hold instead.
     */
    static auto Build(const VPointF &start, qreal radius, qreal length, qreal angle, const QString &endPointName)
        -> VSplinePath;

    /// Radius reached after @p length of arc, from a start radius @p radius.
    static auto RadiusForLength(qreal radius, qreal length) -> qreal;

    /// Angle swept to reach @p length of arc, from a start radius @p radius.
    static auto ThetaForLength(qreal radius, qreal length) -> qreal;

    /// Growth exponent of the law: @c b = 2*ln(q)/pi.
    static auto GrowthExponent() -> qreal;
};

#endif // OSPIRAL_H
