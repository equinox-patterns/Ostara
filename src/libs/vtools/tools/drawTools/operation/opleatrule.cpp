/************************************************************************
 **
 **  @file   o-pleatrule.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The rule of a pleat: see the header for what it is meant to answer.
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

#include "opleatrule.h"

#include <QLineF>
#include <QSharedPointer>

#include "../../vgeometry/vabstractcurve.h"
#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vpatterndb/vcontainer.h"
#include "../toolpoint/toolsinglepoint/odartcaprule.h"
#include "oedgereader.h"
#include "opleatnotch.h"

namespace
{
/** Directions are handled normalized: below this length a vector carries no direction at all, and two figures
 *  are considered parallel or perpendicular when the dot product is this close to the bound. */
constexpr qreal kDirectionTolerance = 1e-7;

/** How far the point that carries the "line to extend" sits from the base point it must be attached to. The
 *  cap rule attaches the line to the base point nearest that carrier, so the carrier has to be closer to the
 *  base point it stands for than to the other one — a thousandth of the step is far enough, and small enough
 *  to leave the direction exact. */
constexpr qreal kCarrierDistance = 1e-3;

//---------------------------------------------------------------------------------------------------------------------
/** @brief The dot product of two vectors: their cosine when both are unit vectors, which is what tells parallel
 *  from perpendicular without ever dividing by a length that could be null. */
auto Dot(const QPointF &left, const QPointF &right) -> qreal
{
    return left.x() * right.x() + left.y() * right.y();
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The unit vector of a departure line, or a null vector when the line is degenerate. */
auto DirectionOf(const OPleatDeparture &departure) -> QPointF
{
    const QPointF prise = departure.second.position - departure.first.position;
    const qreal length = QLineF(QPointF(), prise).length();
    return (length < kDirectionTolerance) ? QPointF() : prise / length;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The unit normal of a departure line, on the side the pleats are built: a null vector when the line is
 *  degenerate, there being then no normal to speak of. */
auto NormalOf(const OPleatDeparture &departure, OPleatSide side) -> QPointF
{
    const QPointF u = DirectionOf(departure);
    if (u.isNull())
    {
        return {};
    }

    return (side == OPleatSide::Second) ? QPointF(u.y(), -u.x()) : QPointF(-u.y(), u.x());
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Where one point of a departure line stands in the ordered selection, or -1 when it is not one of the
 *  objects that travel: a file edited by hand is the only thing that can hold such a line. */
auto IndexOf(const QVector<SourceItem> &source, quint32 pointId) -> int
{
    for (int index = 0; index < source.size(); ++index)
    {
        if (source.at(index).id == pointId)
        {
            return index;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The run of the ordered selection a pleat takes: what lies between the two points of its departure line,
 *  both included. An empty run is an empty vector, and the pleat then moves nothing. */
auto RegionOf(const QVector<SourceItem> &source, const OPleatDeparture &departure) -> QVector<SourceItem>
{
    const int first = IndexOf(source, departure.first.pointId);
    const int second = IndexOf(source, departure.second.pointId);
    if (first < 0 || second < 0)
    {
        return {};
    }

    const int start = qMin(first, second);
    const int last = qMax(first, second);

    QVector<SourceItem> region;
    region.reserve(last - start + 1);
    for (int index = start; index <= last; ++index)
    {
        region.append(source.at(index));
    }

    return region;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief What the objects that travel are, around one line: how many lie beyond it, and how many behind.
 *
 *  The two sides are those of the line's own direction, exactly as SideOf reads them: @a beyond is the side of
 *  `(-u.y, u.x)`. A count on both sides says the fabric straddles the line. */
struct OPleatSides
{
    int beyond{0};
    int behind{0};
};

//---------------------------------------------------------------------------------------------------------------------
auto SidesOf(const QVector<SourceItem> &region, quint32 firstId, quint32 secondId, const VContainer *data)
    -> OPleatSides
{
    OPleatSides sides;

    if (data == nullptr || !data->HasObject(firstId) || !data->HasObject(secondId))
    {
        return sides;
    }

    const QSharedPointer<VPointF> first = qSharedPointerDynamicCast<VPointF>(data->GetGObject(firstId));
    const QSharedPointer<VPointF> second = qSharedPointerDynamicCast<VPointF>(data->GetGObject(secondId));
    if (first.isNull() || second.isNull())
    {
        return sides;
    }

    const QPointF origin = static_cast<QPointF>(*first);
    const QPointF prise = static_cast<QPointF>(*second) - origin;
    const qreal priseLength = QLineF(QPointF(), prise).length();
    if (priseLength < kDirectionTolerance)
    {
        return sides;
    }

    const QPointF u = prise / priseLength;
    const QPointF normal(-u.y(), u.x());
    const qreal tolerance = kDirectionTolerance * priseLength;

    for (const SourceItem &item : region)
    {
        // The two points of the line lie on it: they carry no side, and they are not asked for one.
        if (item.id == firstId || item.id == secondId || !data->HasObject(item.id))
        {
            continue;
        }

        // Only a point is read: a curve or a line describes no side by itself, and the objects that travel
        // hold enough points for the answer.
        const QSharedPointer<VPointF> point = qSharedPointerDynamicCast<VPointF>(data->GetGObject(item.id));
        if (point.isNull())
        {
            continue;
        }

        const QPointF offset = static_cast<QPointF>(*point) - origin;
        const qreal side = offset.x() * normal.x() + offset.y() * normal.y();
        if (side > tolerance)
        {
            ++sides.beyond;
        }
        else if (side < -tolerance)
        {
            ++sides.behind;
        }
    }

    return sides;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The answer the sides make: which way the fabric goes, or nothing when the line cannot tell. */
auto AnswerOf(const OPleatSides &sides) -> OPleatSide
{
    if (sides.beyond > 0 && sides.behind > 0)
    {
        return OPleatSide::Undetermined; // the fabric straddles the line: the pleat has no side
    }
    if (sides.beyond > 0)
    {
        return OPleatSide::First;
    }
    if (sides.behind > 0)
    {
        return OPleatSide::Second;
    }

    return OPleatSide::Undetermined; // everything lies on the line: there is nothing to fold
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Which way the fabric of a region goes, read of that region, and of the whole selection when the region
 *  cannot answer for itself — an empty region, or one whose objects all lie on the line. */
auto SideOfRegion(const QVector<SourceItem> &region, const QVector<SourceItem> &source,
                  const OPleatDeparture &departure, const VContainer *data) -> OPleatSide
{
    const OPleatSide answer =
        AnswerOf(SidesOf(region, departure.first.pointId, departure.second.pointId, data));
    if (answer != OPleatSide::Undetermined)
    {
        return answer;
    }

    return AnswerOf(SidesOf(source, departure.first.pointId, departure.second.pointId, data));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPleatPlan::Move(quint32 objectId) const -> QPointF
{
    return moves.value(objectId, QPointF());
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatPlan::Mark(int pleat, int end) const -> const OPleatMark *
{
    for (const OPleatMark &mark : marks)
    {
        if (mark.pleat == pleat && mark.end == end)
        {
            return &mark;
        }
    }

    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatPlan::Notch(int pleat, int end) const -> const OPleatNotch *
{
    for (const OPleatNotch &notch : notches)
    {
        if (notch.pleat == pleat && notch.end == end)
        {
            return &notch;
        }
    }

    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatPlan::Place(const OPleatEnd &prise, int departure, int line) const -> QPointF
{
    const OPleatPlace place = places.value(departure, OPleatPlace{});

    // What the pleats took this point with: its own pleat is the line itself, and is added place by place.
    QPointF carried = moves.value(prise.pointId, QPointF());
    const int index = static_cast<int>(sources.indexOf(prise.pointId));
    if (place.start >= 0 && index >= place.start && index <= place.last)
    {
        carried -= place.step * place.pleats;
    }

    return prise.position + carried + place.step * line;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatPlan::PleatCount() const -> int
{
    int count = 0;
    for (const OPleatPlace &place : places)
    {
        count += place.pleats;
    }

    return count;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatRule::SideOf(const QVector<SourceItem> &region, quint32 firstId, quint32 secondId,
                        const VContainer *data) -> OPleatSide
{
    return AnswerOf(SidesOf(region, firstId, secondId, data));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatRule::Compose(const QVector<OPleatDeparture> &departures, const QVector<qreal> &values,
                         const QVector<SourceItem> &source, const VContainer *data) -> OPleatPlan
{
    OPleatPlan plan;

    if (departures.isEmpty() || values.size() != departures.size())
    {
        // No line to fold on: there is no pleat at all, and the tool says so rather than drawing nothing.
        plan.failure = OPleatFailure::DegeneratePrise;
        return plan;
    }

    for (const OPleatDeparture &departure : departures)
    {
        if (DirectionOf(departure).isNull())
        {
            plan.failure = OPleatFailure::DegeneratePrise;
            return plan;
        }
    }

    for (const qreal value : values)
    {
        if (value <= kDirectionTolerance)
        {
            // The value is the pleat as it shows once folded, so it is a length: nothing, or less than nothing,
            // is no fabric to take.
            plan.failure = OPleatFailure::DegenerateValue;
            return plan;
        }
    }

    plan.sources.reserve(source.size());
    for (const SourceItem &item : source)
    {
        plan.sources.append(item.id);
    }

    // The region of each departure line, and the side its fabric is on: what the pleat takes is the run of the
    // ordered selection between the two points of its line, never a stretch of geometry.
    plan.places.resize(departures.size());
    for (int index = 0; index < departures.size(); ++index)
    {
        const OPleatDeparture &departure = departures.at(index);
        OPleatPlace &place = plan.places[index];

        const QVector<SourceItem> region = RegionOf(source, departure);
        place.start = region.isEmpty() ? -1 : IndexOf(source, region.constFirst().id);
        place.last = region.isEmpty() ? -1 : place.start + static_cast<int>(region.size()) - 1;

        const OPleatSide side = SideOfRegion(region, source, departure, data);
        if (side == OPleatSide::Undetermined)
        {
            plan.failure = OPleatFailure::NoSide;
            return plan;
        }

        place.normal = NormalOf(departure, side);
        place.step = place.normal * (2. * values.at(index)); // one pleat takes twice the value it shows
        place.pleats = (departure.kind == OPleatKind::Double) ? 2 : 1;
    }

    // The pleats are numbered in the order the user gave their lines, which is the order the figure holds them
    // in: which objects they take is another question, and the regions above are the only place it is answered.
    int pleat = 0;
    for (OPleatPlace &place : plan.places)
    {
        place.firstPleat = pleat;
        pleat += place.pleats;
    }

    // What each object of the selection travels: the sum of the pleats whose region holds it, and nothing when
    // no region does. Every object answers for itself, so the two portions of a cut curve each travel with the
    // pleats that took them — which is what "respecting the portions" means.
    for (int index = 0; index < source.size(); ++index)
    {
        QPointF move;

        for (const OPleatPlace &place : std::as_const(plan.places))
        {
            if (place.start >= 0 && index >= place.start && index <= place.last)
            {
                move += place.step * place.pleats;
            }
        }

        plan.moves.insert(source.at(index).id, move);
    }

    for (int departure = 0; departure < departures.size(); ++departure)
    {
        const OPleatDeparture &line = departures.at(departure);
        const OPleatPlace &place = plan.places.at(departure);
        const QPointF u = DirectionOf(line);

        for (int rank = 0; rank < place.pleats; ++rank)
        {
            const QPointF shift = place.step / 2.; // the fold line, half a step beyond the place it belongs to
            const QPointF centre0 = plan.Place(line.first, departure, rank) + shift;
            const QPointF centre1 = plan.Place(line.second, departure, rank) + shift;
            // The cap rule builds its axis from a point of the fold line and the middle of the opening; the
            // middle of the fold line itself is the point that is never confounded with either end.
            const QPointF apex = (centre0 + centre1) / 2.;

            for (int end = 0; end < 2; ++end)
            {
                const OPleatEnd &edge = (end == 0) ? line.first : line.second;
                if (!edge.showCap)
                {
                    continue; // the user asked for no cap there: no mark, and so nothing for the fold line either
                }

                OPleatMark mark;
                mark.departure = departure;
                mark.pleat = place.firstPleat + rank;
                mark.end = end;
                mark.point = (end == 0) ? centre0 : centre1;

                const QPointF origin = plan.Place(edge, departure, rank); // the line, for this pleat
                const QPointF copy = origin + place.step;                 // and its copy, where the fabric arrives

                const qreal directionLength = QLineF(QPointF(), edge.edgeDirection).length();
                if (directionLength < kDirectionTolerance)
                {
                    // No edge was read at that point: the cap cannot be built, and a pleat is never built
                    // halfway. The caller asks the user to designate the point that carries the edge, and only
                    // comes back when it has one — or says why it could not.
                    plan.failure = OPleatFailure::UnresolvedEdge;
                    return plan;
                }

                const QPointF e = edge.edgeDirection / directionLength;
                const qreal alongMove = Dot(e, place.normal);

                if (qAbs(alongMove) > 1. - kDirectionTolerance)
                {
                    // The edge runs parallel to the move: it is carried onto itself by the fold, it meets the
                    // fold line at the centre, and both senses give that same point. There is no V to draw — it
                    // would have no thickness. The centre is the whole mark.
                    mark.flat = true;
                    plan.marks.append(mark);
                    continue;
                }

                const qreal alongFold = Dot(e, u);
                if (qAbs(alongFold) > 1. - kDirectionTolerance)
                {
                    // The edge runs along the fold line: the two never meet, so no cap point exists there.
                    plan.failure = OPleatFailure::EdgeAlongFold;
                    return plan;
                }

                // The line to extend: the piece's edge at the point of the departure line (first sense), or the
                // same edge carried by the move (second sense, the edge the copy wears).
                //
                // The second pleat of a double is the first one reversed — the two folds lie opposite ways, which
                // is the whole difference between a pleat and the one beside it — so its cap opens the other way.
                // Swapping the two ends of its opening is what says so, and it is the only difference: the sides
                // still join the same two lines, only the point of the V moves to the other side of them.
                const bool secondSense = edge.secondSense != (rank == 1);
                const QPointF base = secondSense ? copy : origin;
                const QPointF carrier = base + e * (QLineF(QPointF(), place.step).length() * kCarrierDistance);

                QPointF cap;
                if (ODartCapRule::IsFailure(ODartCapRule::CapPoint(apex, origin, copy, carrier, &cap)))
                {
                    // The only failure left possible here: the line to extend does not meet the fold line.
                    plan.failure = OPleatFailure::EdgeAlongFold;
                    return plan;
                }

                mark.point = cap;
                plan.marks.append(mark);
            }
        }
    }

    // The notch of every end that asks for one: the mark a fitter reads on the piece's edge, a value away from the
    // line the sense measures the fold on, walked **along the edge** and never along the move. It comes last
    // because it needs the cap: a pleat whose edge could not be followed above has already been refused, and there
    // is then no edge to mark.
    for (int departure = 0; departure < departures.size(); ++departure)
    {
        const OPleatDeparture &line = departures.at(departure);
        const OPleatPlace &place = plan.places.at(departure);

        for (int rank = 0; rank < place.pleats; ++rank)
        {
            for (int end = 0; end < 2; ++end)
            {
                const OPleatEnd &edge = (end == 0) ? line.first : line.second;
                if (not edge.showCap || not edge.showNotch)
                {
                    continue; // no cap, no edge to mark; and the end asks for no notch
                }

                // The sense decides which line the value is counted from: the pleat's own departure line when it
                // follows the edge as drawn, its copy when it follows the edge the move carries — and the second
                // pleat of a double is the first one reversed, which is the whole difference between the two.
                const bool secondSense = edge.secondSense != (rank == 1);
                const QPointF from = plan.Place(edge, departure, secondSense ? rank + 1 : rank);

                // The side the notch leaves the fold by: the fabric lies on the normal side of the line, so a
                // notch counted from the departure line goes against it, and one counted from the copy goes with.
                const QPointF side = secondSense ? place.normal : -place.normal;

                OPleatNotch notch;
                notch.departure = departure;
                notch.pleat = place.firstPleat + rank;
                notch.end = end;

                // Where the edge is, on that side. A point the user designated to carry the edge describes a line
                // and no object, so nothing is asked of the pattern then.
                const OEdgeCarrier carrier
                    = (edge.edgePointId == NULL_ID) ? OEdgeReader::Carrier(edge.pointId, side, data)
                                                    : OEdgeCarrier{};
                const QSharedPointer<VAbstractCurve> curve
                    = (carrier.objectId != NULL_ID && data != nullptr && data->HasObject(carrier.objectId))
                          ? qSharedPointerDynamicCast<VAbstractCurve>(data->GetGObject(carrier.objectId))
                          : QSharedPointer<VAbstractCurve>{};

                if (not curve.isNull())
                {
                    // A curve carries the edge: the notch is walked along it, from the end the point is, and it
                    // is that curve the figure will cut in two.
                    const OPleatNotchPlacement placement
                        = OPleatNotchRule::Place(curve, edge.position, values.at(departure));
                    if (placement.length < 0.)
                    {
                        plan.failure = OPleatFailure::NotchOffEdge;
                        return plan;
                    }

                    notch.edgeId = carrier.objectId;
                    notch.length = placement.length;
                    notch.point = from + (placement.point - edge.position);
                }
                else
                {
                    QPointF inward = carrier.inward;
                    if (inward.isNull())
                    {
                        const qreal length = QLineF(QPointF(), edge.edgeDirection).length();
                        if (length <= kDirectionTolerance)
                        {
                            plan.failure = OPleatFailure::UnresolvedEdge;
                            return plan;
                        }

                        const QPointF unit = edge.edgeDirection / length;
                        inward = (Dot(unit, side) > 0.) ? unit : -unit;
                    }

                    // A line carries it: the notch is a point on that line, and there is nothing to cut.
                    notch.point = from + inward * values.at(departure);
                }

                plan.notches.append(notch);
            }
        }
    }

    return plan;
}
