/************************************************************************
 **
 **  @file   ospiral.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Golden spiral geometry: the law, its closed forms, and the
 **          cubic chain emitted out of it at an exact length.
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

#include "ospiral.h"

#include <QPointF>
#include <QString>
#include <QVector>

#include <algorithm>
#include <cmath>

#include "../vmisc/def.h"
#include "oc2chain.h"
#include "vpointf.h"
#include "vsplinepath.h"

namespace
{
/// Smallest spiral the geometry can hold, should a file carry a zero radius or a zero length.
constexpr qreal kSmallest = MmToPixel(0.01); // a hundredth of a millimetre

/// The promise the tool makes: the emitted spline measures the length asked for to a hundredth of a millimetre.
constexpr qreal kLengthTolerance = MmToPixel(0.01);

/// Below this the swept angle is a floating point detail, not a length anybody can see.
constexpr qreal kAngleResolution = 1e-12;

/// Safety net on the bracket search: the emitted spline falls a hair short of the law, never short by a factor.
constexpr int kMaxBracketSteps = 64;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OSpiral::GrowthExponent() -> qreal
{
    return 2.0 * std::log(kGrowth) / M_PI;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSpiral::RadiusForLength(qreal radius, qreal length) -> qreal
{
    const qreal b = GrowthExponent();
    return radius + b * length / std::sqrt(1.0 + b * b);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSpiral::ThetaForLength(qreal radius, qreal length) -> qreal
{
    if (radius <= 0.0)
    {
        return 0.0;
    }

    return std::log(RadiusForLength(radius, length) / radius) / GrowthExponent();
}

//---------------------------------------------------------------------------------------------------------------------
auto OSpiral::Build(const VPointF &start, qreal radius, qreal length, qreal angle, const QString &endPointName)
    -> VSplinePath
{
    radius = std::max(radius, kSmallest);
    length = std::max(length, kSmallest);

    const qreal b = GrowthExponent();
    const qreal root = std::sqrt(1.0 + b * b);

    // Samples are spread on a count of steps decided once, and kept for the whole search. Let the count follow
    // the angle and the emitted length would jump every time a sample is added, by far more than the tolerance
    // asked for, leaving no bracket a bisection could trust.
    const qreal estimate = ThetaForLength(radius, length);
    const int steps = std::clamp(static_cast<int>(std::ceil(estimate / (2.0 * M_PI / kSamplesPerTurn))), 2, 2048);

    // Canonical frame: the start point is the origin, the pole sits one start radius to its right, and the
    // spiral leaves the start point at 180 degrees from the pole.
    auto Position = [radius](qreal swept) -> QPointF
    {
        const qreal phi = M_PI - swept;
        const qreal r = radius * std::pow(OSpiral::kGrowth, 2.0 * swept / M_PI);
        return QPointF(radius + r * std::cos(phi), r * std::sin(phi));
    };

    // The tangent of a logarithmic spiral keeps a constant angle with its radius: b * radial + tangential.
    auto Direction = [b, root](qreal swept) -> QPointF
    {
        const qreal phi = M_PI - swept;
        return QPointF(b * std::cos(phi) + std::sin(phi), b * std::sin(phi) - std::cos(phi)) / root;
    };

    auto Samples = [&Position, steps](qreal swept) -> QVector<QPointF>
    {
        QVector<QPointF> samples;
        samples.reserve(steps + 1);
        for (int i = 0; i <= steps; ++i)
        {
            samples.append(Position(swept * i / steps));
        }
        return samples;
    };

    // What the spiral really measures at a given swept angle — on the spline emitted, not on the law, because
    // no spline is the law. This is the application's own measure, the one the tooltip and the formulas report.
    auto EmittedLength = [&Samples, &Direction, &start, &endPointName](qreal swept) -> qreal
    {
        return OC2Chain::Chain(Samples(swept), Direction(0.0), Direction(swept), start.name(), endPointName)
            .GetLength();
    };

    // The closed form lands short of the target: the chain cuts the corners of the law. Doubling the angle
    // overshoots it, and everything in between is one bisection away.
    qreal lower = 0.0;
    qreal upper = estimate;
    for (int i = 0; i < kMaxBracketSteps && EmittedLength(upper) < length; ++i)
    {
        upper *= 2.0;
    }

    qreal swept = upper;
    qreal error = std::abs(EmittedLength(swept) - length);
    for (int i = 0; i < 64 && error > kLengthTolerance && (upper - lower) > kAngleResolution; ++i)
    {
        const qreal middle = (lower + upper) / 2.0;
        const qreal measured = EmittedLength(middle) - length;

        if (std::abs(measured) < error)
        {
            error = std::abs(measured);
            swept = middle;
        }

        if (measured < 0.0)
        {
            lower = middle;
        }
        else
        {
            upper = middle;
        }
    }

    // Rotation is an isometry, so it is applied to the samples and to the two end directions rather than to the
    // path: turning a path would rename its knots, and the two ends are what names it.
    const qreal radians = angle * M_PI / 180.0;
    const qreal cosine = std::cos(radians);
    const qreal sine = std::sin(radians);
    auto Turn = [cosine, sine](const QPointF &point) -> QPointF
    {
        // Same convention as VPointF::RotatePF: a positive angle reads counter-clockwise on screen.
        return QPointF(point.x() * cosine + point.y() * sine, -point.x() * sine + point.y() * cosine);
    };

    const QPointF origin = start.toQPointF();
    QVector<QPointF> turned;
    const QVector<QPointF> canonical = Samples(swept);
    turned.reserve(canonical.size());
    for (const QPointF &point : canonical)
    {
        turned.append(origin + Turn(point));
    }

    return OC2Chain::Chain(turned, Turn(Direction(0.0)), Turn(Direction(swept)), start.name(), endPointName);
}
