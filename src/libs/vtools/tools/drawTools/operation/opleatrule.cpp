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

#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vpatterndb/vcontainer.h"
#include "../toolpoint/toolsinglepoint/odartcaprule.h"

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
/** @brief What the objects that travel are, around one departure line: how many lie beyond it, and how many behind.
 *
 *  The two sides are those of the line's own direction, exactly as SideOf reads them: @a beyond is the side of
 *  `(-u.y, u.x)`. A count on both sides says the selection straddles the line. */
struct OPleatSides
{
    int beyond{0};
    int behind{0};
};

//---------------------------------------------------------------------------------------------------------------------
auto SidesOf(quint32 firstId, quint32 secondId, const QVector<SourceItem> &source, const VContainer *data)
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

    for (const SourceItem &item : source)
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
auto SidesOf(const OPleatDeparture &departure, const QVector<SourceItem> &source, const VContainer *data)
    -> OPleatSides
{
    return SidesOf(departure.first.pointId, departure.second.pointId, source, data);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The answer the sides make: which way the fabric goes, or nothing when the line cannot tell. */
auto AnswerOf(const OPleatSides &sides) -> OPleatSide
{
    if (sides.beyond > 0 && sides.behind > 0)
    {
        return OPleatSide::Undetermined; // the selection straddles the line: the pleat has no side
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
/** @brief The direction the fabric goes, read of every departure line that can answer for it.
 *
 *  A line says which way the fabric goes when the objects that travel all lie on one side of it. A line the
 *  selection straddles — one with travelling fabric on both of its sides, which a line lying between two pleats
 *  always has — says nothing, and is passed over.
 *
 *  Several lines may answer, and they do not all answer the same way: a line the fabric has gone beyond asks for
 *  the fabric to travel further, while a line the travelling objects lie *behind* — the inner line of two folds
 *  taken on the same band of fabric — asks for the opposite. The line the objects have gone the furthest beyond is
 *  the one that answers: it is the outermost line, the one the fabric leaves, and the pleats then all fold the same
 *  way, whether the fabric that travels goes past one line or past both.
 *
 *  A null vector means no line could answer, and the tool then refuses rather than folding a pleat the wrong way. */
auto ReferenceNormal(const QVector<OPleatDeparture> &departures, const QVector<SourceItem> &source,
                     const VContainer *data) -> QPointF
{
    QPointF found{};
    int best = -1;

    for (const OPleatDeparture &departure : departures)
    {
        const OPleatSides sides = SidesOf(departure, source, data);
        const OPleatSide side = AnswerOf(sides);
        if (side == OPleatSide::Undetermined)
        {
            continue;
        }

        const int beyond = qMax(sides.beyond, sides.behind);
        if (beyond > best)
        {
            best = beyond;
            found = NormalOf(departure, side);
        }
    }

    return found;
}
} // namespace

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
auto OPleatPlan::Move(const QPointF &point) const -> QPointF
{
    QPointF move{};

    for (const OPleatPlace &place : places)
    {
        // On the line, or beyond it: the fabric the fold takes is the one that travels, and the line itself
        // belongs to it — folding carries the departure line onto its own copy.
        if (Dot(point - place.origin, place.normal) >= -place.tolerance)
        {
            move += place.step * place.pleats;
        }
    }

    return move;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatPlan::Place(const QPointF &prisePoint, int departure, int line) const -> QPointF
{
    const OPleatPlace place = places.value(departure, OPleatPlace{});
    return prisePoint + place.carry + place.step * line;
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
auto OPleatRule::SideOf(const QVector<SourceItem> &source, quint32 firstId, quint32 secondId,
                        const VContainer *data) -> OPleatSide
{
    return AnswerOf(SidesOf(firstId, secondId, source, data));
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

    // Which way the fabric goes, read of the departure lines that can answer for it. Every line then keeps its own
    // direction — a pleat takes the fabric beyond its own line and moves it along its own normal — and the reference
    // is what turns each of them towards the half-plane the operation folds into.
    const QPointF reference = ReferenceNormal(departures, source, data);
    if (reference.isNull())
    {
        plan.failure = OPleatFailure::NoSide;
        return plan;
    }

    // What each line is worth, and how it is folded: one pass, since a line's carry depends on the others' steps.
    plan.places.resize(departures.size());
    for (int index = 0; index < departures.size(); ++index)
    {
        const OPleatDeparture &departure = departures.at(index);
        OPleatPlace &place = plan.places[index];

        const QPointF first = NormalOf(departure, OPleatSide::First);
        const OPleatSide side = (Dot(first, reference) > 0.) ? OPleatSide::First : OPleatSide::Second;

        place.origin = departure.first.position;
        place.normal = NormalOf(departure, side);
        place.step = place.normal * (2. * values.at(index)); // one pleat takes twice the value it shows
        place.pleats = (departure.kind == OPleatKind::Double) ? 2 : 1;
        place.tolerance = kDirectionTolerance * QLineF(departure.first.position, departure.second.position).length();
    }

    // How far each departure line has been carried: by every other pleat whose line it lies beyond, wherever that
    // line runs. A pleat takes the fabric beyond its line, so a line lying beyond another one travels with it — the
    // answer is read from the geometry, never from the order the user clicked the lines in.
    for (int index = 0; index < departures.size(); ++index)
    {
        const QPointF origin = departures.at(index).first.position;

        for (int other = 0; other < departures.size(); ++other)
        {
            if (other == index)
            {
                continue; // a line does not carry itself, and two lines drawn on top of each other carry nothing
            }

            const OPleatPlace &carrier = plan.places.at(other);
            if (Dot(origin - carrier.origin, carrier.normal) > carrier.tolerance)
            {
                plan.places[index].carry += carrier.step * carrier.pleats;
            }
        }
    }

    // The pleats are numbered in the order the user gave their lines, which is the order the figure holds them
    // in: how they are folded is another question, and the carries above are the only place it is answered.
    int pleat = 0;
    for (OPleatPlace &place : plan.places)
    {
        place.firstPleat = pleat;
        pleat += place.pleats;
    }

    for (int departure = 0; departure < departures.size(); ++departure)
    {
        const OPleatDeparture &line = departures.at(departure);
        const OPleatPlace &place = plan.places.at(departure);

        const QPointF u = DirectionOf(line);

        for (int rank = 0; rank < place.pleats; ++rank)
        {
            const QPointF shift = place.carry + place.step * rank; // where this pleat starts, in the fabric
            const QPointF centre0 = line.first.position + shift + place.step / 2.;
            const QPointF centre1 = line.second.position + shift + place.step / 2.;
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

                const QPointF origin = edge.position + shift; // the line, for this pleat
                const QPointF copy = origin + place.step;     // and its copy, where the fabric arrives

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

    return plan;
}
