/************************************************************************
 **
 **  @file   oflouncespiral.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  The flounce spiral, as geometry. See the header for the law
 **          and the conventions.
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

#include "oflouncespiral.h"

#include "oc2chain.h"
#include "vsplinepath.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// Smallest value the construction can hold. Below it the arcs collapse on the eye and nothing is measurable.
constexpr qreal kSmallest = 1e-3;

//---------------------------------------------------------------------------------------------------------------------
/// Point of a circle.
auto OnCircle(const QPointF &centre, qreal radius, qreal angle) -> QPointF
{
    return QPointF(centre.x() + radius * std::cos(angle), centre.y() + radius * std::sin(angle));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief The construction, read as a single parameter rather than walked arc after arc.
 *
 * Arc 1 is centred on A with radius q and starts at B; arc 2 on B with radius 2q, and so on, the radius growing
 * by q every half turn and the centre alternating. A is placed @c q away from the start point, at the angle the
 * user asked for, so that the first circle passes through the point the user clicked.
 *
 * The position is parameterised by @c t, the number of half turns walked since B, and every point comes out of
 * @c t alone. Walking the arcs while carrying a cursor is the same figure, but every junction is then a chance
 * to lose a fraction of a turn, and a fraction lost at the start is a whole band width lost at the end.
 *
 * The outer edge is the parallel curve at the band width, taken along the unit normal drawn from the centre of
 * the arc the point sits on. That normal is the outward one by construction, so there is no sign to guess: the
 * offset never crosses the curve, and the band width holds exactly at every point.
 */
struct Construction
{
    QPointF start;         ///< the point the inner edge starts at, and where the outer edge comes back
    QPointF a;             ///< the other centre, half a band width away
    qreal q = 0.0;         ///< half the band width
    qreal rotation = 0.0;  ///< radians

    /// Whole half turns walked at parameter @p t.
    static auto HalfTurns(qreal t) -> int { return static_cast<int>(std::floor(t)); }

    static auto IsEven(int k) -> bool { return k % 2 == 0; }

    /// Centre of the arc carrying parameter @p t.
    auto Centre(qreal t) const -> QPointF { return IsEven(HalfTurns(t)) ? a : start; }

    /// Radius of the arc carrying parameter @p t.
    auto Radius(qreal t) const -> qreal { return (HalfTurns(t) + 1) * q; }

    /// Unit vector from the start point to the other centre A: the axis both centres share, and the axis every
    /// arc of the construction begins on.
    auto Axis() const -> QPointF { return QPointF(std::cos(rotation), std::sin(rotation)); }

    /// Point where the arc carrying parameter @p t begins, on the axis both centres share.
    auto Support(qreal t) const -> QPointF
    {
        const int k = HalfTurns(t);
        if (k == 0)
        {
            return start;
        }
        // The support sits on the axis, one radius away from the centre of its own arc: every even arc starts on
        // the far side of A, every odd one on the far side of B.
        //
        // Written along THAT axis and not along the x axis. The offset used to be added to the x coordinate
        // alone, which describes the figure only while A happens to sit on the horizontal — past that, the
        // centres of the half circles no longer sit where the previous arc ended, and the spiral folds onto
        // itself. That is what made the figure collapse as soon as the angle was changed.
        const qreal radius = Radius(t);
        const QPointF axis = Axis();
        if (IsEven(k))
        {
            return QPointF(a.x() - radius * axis.x(), a.y() - radius * axis.y());
        }
        return QPointF(start.x() + radius * axis.x(), start.y() + radius * axis.y());
    }

    /// Angle the arc carrying parameter @p t starts at, about its own centre.
    auto Angle0(qreal t) const -> qreal
    {
        const QPointF centre = Centre(t);
        const QPointF support = Support(t);
        return std::atan2(support.y() - centre.y(), support.x() - centre.x());
    }

    /// Angle of the point at parameter @p t, about the centre of its arc.
    auto AngleAt(qreal t) const -> qreal { return Angle0(t) + M_PI * (t - HalfTurns(t)); }

    /// Unit normal of the inner edge at parameter @p t, pointing away from the centre of the arc.
    auto Normal(qreal t) const -> QPointF
    {
        const qreal at = AngleAt(t);
        return QPointF(std::cos(at), std::sin(at));
    }

    /// Point of the inner edge at parameter @p t. At t = 0 this is the start point itself.
    auto InnerPoint(qreal t) const -> QPointF
    {
        return OnCircle(Centre(t), Radius(t), AngleAt(t));
    }

    /// Point of the outer edge at parameter @p t, one band width out along the normal.
    auto OuterPoint(qreal t) const -> QPointF
    {
        const QPointF normal = Normal(t);
        const QPointF inner = InnerPoint(t);
        return QPointF(inner.x() + 2.0 * q * normal.x(), inner.y() + 2.0 * q * normal.y());
    }

    /// Unit tangent of the inner edge at parameter @p t, in the direction of travel.
    auto InnerTangent(qreal t) const -> QPointF
    {
        const qreal at = AngleAt(t);
        return QPointF(-std::sin(at), std::cos(at));
    }

    /// Unit tangent of the outer edge at parameter @p t. The outer edge runs the other way round.
    auto OuterTangent(qreal t) const -> QPointF
    {
        const QPointF tangent = InnerTangent(t);
        return QPointF(-tangent.x(), -tangent.y());
    }
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiral::InnerLength(qreal q, int halfTurns) -> qreal
{
    if (halfTurns <= 0)
    {
        return 0.0;
    }
    return M_PI * q * halfTurns * (halfTurns + 1) / 2.0;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiral::InnerRadius(qreal q, int halfTurns) -> qreal
{
    return halfTurns <= 0 ? q : halfTurns * q;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiral::Build(const QPointF &start, qreal bandWidth, qreal length, qreal angle,
                           const EdgeNames &begin, const EdgeNames &outer) -> Figure
{
    Figure figure;
    figure.start = start;
    figure.bandWidth = std::max(bandWidth, kSmallest);
    length = std::max(length, kSmallest);

    const qreal q = figure.bandWidth / 2.0;

    Construction construction;
    construction.start = start;
    construction.q = q;
    // The angle is the direction of A read from the start point, and it follows the convention the application
    // uses for every angle it shows: counter-clockwise ON SCREEN, where the y axis grows downwards, so 90 degrees
    // points up the screen. That is how QLineF measures its angles and how VPointF::RotatePF turns a point, which
    // is what every other angled tool of the application is built on. The y component is therefore negated.
    // 180 degrees, the default, puts A to the left of the start point.
    construction.rotation = -angle * M_PI / 180.0;
    construction.a = QPointF(start.x() + q * std::cos(construction.rotation),
                             start.y() + q * std::sin(construction.rotation));

    // One edge, as a spline, for a given swept angle. The count of samples follows the angle, so the search below
    // only ever asks for a curve of a settled description.
    auto BuildEdge = [&construction](qreal tFrom, qreal tTo, bool outerEdge,
                                     const EdgeNames &names) -> VSplinePath
    {
        const qreal span = tTo - tFrom;
        const int steps = std::clamp(static_cast<int>(std::ceil(span * kSamplesPerHalfTurn)), 4, kMaxSamples);
        QVector<QPointF> points;
        points.reserve(steps + 1);
        for (int i = 0; i <= steps; ++i)
        {
            const qreal t = tFrom + span * i / steps;
            points.append(outerEdge ? construction.OuterPoint(t) : construction.InnerPoint(t));
        }

        if (outerEdge)
        {
            // The outer edge runs the other way round: emitted from the outer start back to the outer end.
            std::reverse(points.begin(), points.end());
            // The names follow the REVERSED walk: the first knot of the outer edge is the outer START, the last
            // one the outer end. Naming both ends alike — as this did — left the two edges of the figure with the
            // same name, and the two curves of one flounce could not be told apart in a tooltip or anywhere else.
            return OC2Chain::Chain(points, construction.OuterTangent(tTo), construction.OuterTangent(tFrom),
                                   names.first, names.last);
        }
        return OC2Chain::Chain(points, construction.InnerTangent(tFrom), construction.InnerTangent(tTo),
                               names.first, names.last);
    };

    // How many complete half turns the closed form gives, and how much of the next arc is needed on top. The
    // partial arc is what makes the length exact rather than quantised to a whole number of half turns.
    int complete = 0;
    while (InnerLength(q, complete + 1) <= length)
    {
        ++complete;
    }

    const qreal fullRadius = construction.Radius(static_cast<qreal>(complete));
    qreal lower = complete;
    qreal upper = complete + 1.0;
    qreal span = complete + std::clamp((length - InnerLength(q, complete)) / (M_PI * fullRadius), 0.0, 1.0);

    // The measure is taken on the spline really emitted, not on the law: a cubic chain through samples of a
    // circle is shorter than the circle, and the closed form would leave a gap of a fraction of a millimetre.
    // What the user asked for is what the user gets — the application's own measure, the one formulas report.
    qreal error = std::abs(BuildEdge(0.0, span, false, begin).GetLength() - length);
    for (int i = 0; i < 48 && error > kLengthTolerance && (upper - lower) > kAngleResolution; ++i)
    {
        const qreal middle = (lower + upper) / 2.0;
        const qreal measured = BuildEdge(0.0, middle, false, begin).GetLength() - length;

        if (std::abs(measured) < error)
        {
            error = std::abs(measured);
            span = middle;
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

    figure.sweptAngle = span * M_PI;
    figure.finalRadius = span * q;

    // The two curves the tool hands out, and the reason they are cut THERE.
    //
    // The figure is one single spiral, from the start point to the outer start: one turn (two half turns) grows
    // the radius by exactly the band width, so the outer edge IS the inner edge one turn further out. The two
    // points the figure carries — the outer end, one turn from the start, and the inner end, where the length
    // lands — are only CUTS on that single curve.
    //
    // Cutting it at the outer end keeps the two curves DISJOINT: [0, 2] and [2, span+2] share nothing but that
    // point. Handing out instead the inner edge [0, span] and the outer edge [2, span+2] made them overlap over
    // [2, span] — 80 % of the figure for a five-turn flounce — and the application, which recognises an object by
    // asking whether a point lies on it, could no longer tell which of the two a click had landed on: it attached
    // segments to the wrong turn, or refused them altogether.
    figure.begin = BuildEdge(0.0, 2.0, false, begin);
    // The outer edge keeps its own parameterisation — OuterPoint(t) over [0, span] — because that is what the
    // offset law describes: it is the same spiral one turn further out, but read on the outer side of the band.
    figure.outer = BuildEdge(0.0, span, true, outer);

    // The inner edge — the length the user typed — is no longer a curve of its own: it runs from the start point
    // along `begin` and on into `outer`, as far as the inner end. Its measure is still taken on a chain emitted
    // for it alone, so the value the formulas report is the one the user asked for.
    figure.innerLength = BuildEdge(0.0, span, false, begin).GetLength();
    figure.outerLength = figure.outer.GetLength();

    figure.innerEnd = construction.InnerPoint(span);
    figure.outerEnd = figure.outer.LastPoint().toQPointF();
    figure.outerStart = figure.outer.FirstPoint().toQPointF();

    return figure;
}
