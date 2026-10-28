/************************************************************************
 **
 **  @file   opleatrule.cpp
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

#include <QCoreApplication>

#include <QLineF>
#include <QSharedPointer>

#include "../../vgeometry/vabstractcurve.h"
#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vpatterndb/vcontainer.h"
#include "../toolpoint/toolsinglepoint/odartcaprule.h"
#include "oedgereader.h"
#include "opleatcaprule.h"
#include "opleatnotch.h"

namespace
{
/** Directions are handled normalized: below this length a vector carries no direction at all, and two figures
 *  are considered parallel or perpendicular when the dot product is this close to the bound. */
constexpr qreal kDirectionTolerance = kPleatTolerance; // one value, written in the rule's header

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
//---------------------------------------------------------------------------------------------------------------------
/** @brief The run of the ordered selection a pleat takes: what lies between the two points of its departure line,
 *  both included. An empty run is an empty vector, and the pleat then moves nothing. */
auto RegionOf(const QVector<SourceItem> &source, const OPleatDeparture &departure) -> QVector<SourceItem>
{
    const int first = SourceIndexOf(source, departure.first.pointId);
    const int second = SourceIndexOf(source, departure.second.pointId);
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

//---------------------------------------------------------------------------------------------------------------------
/** @brief The line from @a position to the neighbour of @a pointId in the ordered selection, the way the sense goes.
 *
 *  The selection describes the contour: the two points that come before and after a point of it always make a line
 *  with it, and that line is the edge there whether the pattern drew it or not. @a way is the side the sense
 *  follows — the point after the given one when it is positive — and the other side is tried when that one has no
 *  neighbour, because the edge of a piece may end there: one side is enough, and it is no refusal.
 *
 *  An empty vector means the selection does not hold the point, or holds nothing beside it.
 */
auto SelectedLine(const OPleatPlan &plan, quint32 pointId, const QPointF &position, int way,
                  const VContainer *data) -> QPointF
{
    int at = -1;
    for (int index = 0; index < plan.sources.size(); ++index)
    {
        if (plan.sources.at(index) == pointId)
        {
            at = index;
            break;
        }
    }

    if (at < 0)
    {
        return {};
    }

    for (const int step : {way, -way})
    {
        const int neighbour = at + step;
        if (neighbour < 0 || neighbour >= plan.sources.size())
        {
            continue; // nothing on that side: the piece's edge, which is no refusal
        }

        const QPointF other = OEdgeReader::Position(plan.sources.at(neighbour), data);
        const QPointF line = other - position;
        if (QLineF(QPointF(), line).length() > kDirectionTolerance)
        {
            return line;
        }
    }

    return {};
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
    if (place.given <= 0. && place.start >= 0 && index >= place.start && index <= place.last)
    {
        carried -= place.step * place.pleats;
    }

    return prise.position + carried + place.step * (line - place.given);
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
auto OPleatFailureText(OPleatFailure failure) -> QString
{
    switch (failure)
    {
        case OPleatFailure::None:
            return {};
        case OPleatFailure::DegeneratePrise:
            return QCoreApplication::translate("OPleatRule", "You clicked the same point twice.");
        case OPleatFailure::DegenerateValue:
            return QCoreApplication::translate("OPleatRule", "Put a pleat value greater than zero.");
        case OPleatFailure::NoSide:
            return QCoreApplication::translate("OPleatRule",
                                               "Every object you selected must be on the same side of the departure lines.");
        case OPleatFailure::UnresolvedEdge:
            return QCoreApplication::translate("OPleatRule",
                                               "The edge was not recognized at one end of a departure line.");
        case OPleatFailure::EdgeAlongFold:
            return QCoreApplication::translate("OPleatRule",
                                               "This edge is parallel to the fold line: no cap is possible here.");
        case OPleatFailure::NotchOffEdge:
            return QCoreApplication::translate("OPleatRule",
                                               "The notch of a pleat falls past the end of the edge it should mark.");
        case OPleatFailure::SplitDeparture:
            return QCoreApplication::translate(
                "OPleatRule",
                "Two departure lines share a point, and the pleats do not carry them together: no cap can be built there.");
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatRule::Compose(const QVector<OPleatDeparture> &departures, const QVector<qreal> &values,
                         const QVector<SourceItem> &source, const VContainer *data, OPleatOrigin origin)
    -> OPleatPlan
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
        place.start = region.isEmpty() ? -1 : SourceIndexOf(source, region.constFirst().id);
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
        place.given = (origin == OPleatOrigin::Middle) ? place.pleats / 2. : 0.;
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

        for (int departure = 0; departure < plan.places.size(); ++departure)
        {
            const OPleatPlace &place = plan.places.at(departure);

            if (place.given <= 0.)
            {
                if (place.start >= 0 && index >= place.start && index <= place.last)
                {
                    move += place.step * place.pleats;
                }

                continue;
            }

            // **The variant of the pleat laid on the middle of a piece: the two halves of the outline travel, in
            // opposite directions, by the value of the pleat times the number of pleats it holds** — one value for a
            // simple pleat, two for a double, because a double holds two pleats inside it. The pleat taken on its
            // first arm, by contrast, carries one region two values per pleat. The two points the user drew are the
            // bounds of both halves and stand on the fold: they do not move, and their two copies are the arms.
            // Everything else is the pleat, unchanged (see PleatMid.md).
            const OPleatDeparture &line = departures.at(departure);
            const quint32 id = source.at(index).id;
            if (id == line.first.pointId || id == line.second.pointId)
            {
                continue;
            }

            const bool inFirstRun = place.start >= 0 && index >= place.start && index <= place.last;
            const QPointF half = place.step * (place.pleats / 2.);
            move += (inFirstRun ? half : -half);
        }

        plan.moves.insert(source.at(index).id, move);
    }

    // A departure line is carried as a whole: its two ends lie in the same region, and the pleats translate them by
    // the same vector. When one end is taken by another line and the other is not — two lines that share a point —
    // the edge the cap is built on is no longer parallel to the line itself: the axis of the cap is not the fold any
    // more, the reflection hands back something other than the copy, and the V does not close. Nothing tells which of
    // the two the user meant, so the pleat is refused rather than drawn wrong.
    for (const OPleatDeparture &line : std::as_const(departures))
    {
        // Only a line that carries something is asked: a cap is built ON that axis, and a notch walks along the edge
        // it follows. A line with neither is only carried, and the figure draws it as the rule computed it.
        const bool carries = line.first.showCap || line.second.showCap || line.first.showNotch || line.second.showNotch;
        if (carries && plan.Move(line.first.pointId) != plan.Move(line.second.pointId))
        {
            plan.failure = OPleatFailure::SplitDeparture;
            return plan;
        }
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

                // **The edge read at the point is the line the departure line is drawn on.** A construction line
                // through the point is answered, because that is what the tool **of the point** draws — and such a
                // line is parallel to the fold, so no cap can be built on it and the pleat is refused, even when the
                // piece's edge runs through that very point, drawn by other tools (the two segments of a hem) or by
                // a curve that **starts** there (a shoulder spline).
                //
                // The reading is therefore taken again **with the side the fabric goes**: the outline at the point,
                // chosen by that side. It happens only where the pleat would be refused, so nothing that is read
                // today is read differently.
                QPointF directionHere = edge.edgeDirection;
                if (qAbs(Dot(directionHere / directionLength, u)) > kPleatParallelCosine)
                {
                    // ① a curve carries the edge — the reader is asked with the side the fabric goes; ② the point's
                    // own tool may draw the edge itself. Both are read here.
                    const bool sense = edge.secondSense != (rank == 1);
                    const QPointF read
                        = OEdgeReader::Direction(edge.pointId, sense ? -place.normal : place.normal, data);
                    if (not read.isNull())
                    {
                        directionHere = read;
                    }

                    // ③ **The last resort of the cascade: the line to the neighbour in the selection.** When no
                    // curve answers (①) and the point's own tool draws a line that is not the piece's edge (②), the
                    // **order of the selected points** answers: the point just before, or just after, the way the
                    // sense goes — two points always make a line. A pleat has two senses and the edge of a piece may
                    // offer only one; that is not a refusal, it is the case where the sense is forced (see
                    // PleatMid.md).
                    const qreal lengthHere = QLineF(QPointF(), directionHere).length();
                    if (lengthHere <= kDirectionTolerance
                        || qAbs(Dot(directionHere / lengthHere, u)) > kPleatParallelCosine)
                    {
                        const int way = sense ? 1 : -1; // the sense the cap follows: the point before it, or after
                        const QPointF selected = SelectedLine(plan, edge.pointId, edge.position, way, data);
                        if (not selected.isNull())
                        {
                            directionHere = selected;
                        }
                    }
                }

                const QPointF e = directionHere / QLineF(QPointF(), directionHere).length();
                // **Where a curve carries the edge, it decides first.** A tangent only meets a curve where it is
                // taken: an edge whose tangent at the point is parallel to the move is *not* flat when the curve it
                // belongs to bends away from it — and reading it flat would draw no cap at all on an edge that is
                // plainly there (PleatMid.md, and the pleat taken on its first arm met this first). The flat edge is
                // the one no curve carries: a straight tracing, whose tangent is the whole of it.
                const bool secondSense = edge.secondSense != (rank == 1);

                // Where a curve carries the edge, the cap is read on the curve itself: its apex is the point where
                // that curve meets the fold line — the fold line then runs from edge to edge — and its two sides
                // are the piece of the edge that runs to that point and that piece flipped about the fold line,
                // which is what the fold does to it (see OPleatCapRule). A tangent only meets a curve where it is
                // taken: the V followed the right axis and the wrong shape, and the fold left the difference.
                //
                // Which edge, and which fold line, is what the sense decides, since the two lines of the opening
                // are two places of the same departure line:
                //
                // - the first sense follows the edge **as it is drawn**, the one that leaves the point into the
                //   region — the notch reads the other one, being counted on the side the fold leaves free — and
                //   the fold line is read where it stands;
                // - the second one follows the edge **the move carries**, that is the outline on the other side of
                //   the point: the piece's edge runs through it, and the fabric comes back from there. That side
                //   is a step further, so the fold line is read a step back, and the whole figure lands a step on:
                //   on the place the move carried the line to, which is where that V opens.
                const QPointF side = secondSense ? -place.normal : place.normal;
                const QPointF carry = (plan.Place(edge, departure, rank) - edge.position)
                                      + (secondSense ? place.step : QPointF{});
                // **The cap of a pleat laid on the middle of a piece is a portion of the edge**, taken from the arm
                // over the value of the pleat, turned over by the arm and then by the fold line — never a line read
                // across the pleat (see PleatMid.md, and Sophie's drawing: BustedeBase.ostara, 1052 to 1076). The
                // arm is the copy of the line the user drew, so the portion is read on the edge as the pattern drew
                // it and carried there; the tip is where the arm's turning lays it on the fold line.
                if (place.given > 0.)
                {
                    // **Which arm this cap is read on.** A simple pleat has one fold and two arms, and the sense
                    // says which of the two its cap leans on. A double has two folds and an arm for **each** of its
                    // pleats: the first is read on the piece's near arm, the second on the far one — its own arm,
                    // whichever way the folds lie.
                    const int armHere = (place.pleats > 1) ? (rank == 0 ? 0 : place.pleats)
                                                           : (secondSense ? rank + 1 : rank);
                    const QPointF armPoint = plan.Place(edge, departure, armHere);
                    const OPleatCapSides portion = OPleatCapRule::Portion(
                        edge, side, QLineF(armPoint, armPoint + (centre1 - centre0)), QLineF(centre0, centre1),
                        values.at(departure), armPoint - edge.position, data);

                    if (not portion.edge.isNull() && not portion.image.isNull())
                    {
                        const QVector<QPointF> points = portion.edge->GetPoints();
                        mark.point = portion.tip.isNull() && not points.isEmpty() ? points.last() : portion.tip;
                        mark.portion = portion.edge;
                        mark.turned = portion.image;
                        mark.place = armHere;
                        plan.marks.append(mark);
                        continue;
                    }
                }

                const OPleatCapPlacement curved
                    = OPleatCapRule::Curved(edge, side, QLineF(centre0 - carry, centre1 - carry), carry, data);
                if (not curved.point.isNull())
                {
                    mark.point = curved.point;
                    mark.edgeId = curved.edgeId;
                    mark.length = curved.length;
                    mark.fromStart = curved.fromStart;
                    mark.place = secondSense ? rank + 1 : rank;
                    plan.marks.append(mark);
                    continue;
                }

                const qreal alongMove = Dot(e, place.normal);

                if (qAbs(alongMove) > kPleatParallelCosine)
                {
                    // The edge runs parallel to the move: it is carried onto itself by the fold, it meets the
                    // fold line at the centre, and both senses give that same point. There is no V to draw — it
                    // would have no thickness. The centre is the whole mark.
                    mark.flat = true;
                    plan.marks.append(mark);
                    continue;
                }

                const qreal alongFold = Dot(e, u);
                if (qAbs(alongFold) > kPleatParallelCosine)
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

                // The sense decides **which line the value is counted from** — and the notch reads it the other way
                // round than the cap does: a cran stands beyond the arm the cap lays down, that is **after the
                // second arm** when the cap follows the edge as drawn, and **before the first one** when it follows
                // the edge the move carries. The value is counted from the copy in the first case, and from the
                // departure line in the second — the second pleat of a double being the first one reversed is the
                // whole difference between the two senses.
                //
                // **That place is a point of the fold's own grid, and not the edge**: the value is walked from it
                // **along the edge**, which is what makes the mark readable on the piece (see the two branches
                // below). Reading the edge is a question of its own, and the point's own tool does not answer it.
                const bool secondSense = edge.secondSense != (rank == 1);
                const QPointF from = plan.Place(edge, departure, secondSense ? rank : rank + 1);

                // The side the notch leaves the fold by: the fabric lies on the normal side of the line, so a
                // notch counted from the departure line goes against it, and one counted from the copy goes with.
                const QPointF side = secondSense ? -place.normal : place.normal;

                OPleatNotch notch;
                notch.departure = departure;
                notch.pleat = place.firstPleat + rank;
                notch.end = end;
                notch.place = secondSense ? rank : rank + 1;

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

                    // **The notch stands one value from the place the sense counts it from**, and never one value
                    // from the point the edge happens to be read at. A pleat only translates: the edge as the
                    // pattern drew it is walked a value from the point of the departure line, then carried to the
                    // place of the sense — **after the second arm** for the first sense, on the copy, and at the
                    // departure line itself for the second, where the offset is null. Reading the place off the
                    // carrier's own move put the first sense one value from the departure line instead: the edge
                    // the notch marks is not always an object the pleat carries, and a mark that moves with the
                    // object it is cut from is a mark in the wrong place (Pleat.md, « Le cran »).
                    //
                    // The offset kept here is what the figure moves the two halves by, so the point and the cut
                    // stay one and the same place whatever the pleat does to that curve.
                    notch.point = placement.point + (from - edge.position);
                }
                else
                {
                    QPointF inward = carrier.inward;

                    // **A notch is never read off the line of the point's own tool**, and never off a construction
                    // line at all: where the point stands on the fold itself, that tool draws the fold, and walking
                    // it runs the notch down the pleat instead of along the edge — the very mark meant for the edge.
                    // The ordered selection answers instead, the point just before or just after making a line
                    // with it, which is the cascade the caps already follow (PleatMid.md). Only a reading that is
                    // no edge is replaced: what the contour of a piece answers is better than a neighbour and is
                    // kept.
                    const qreal readLength = QLineF(QPointF(), inward).length();
                    const QPointF u = DirectionOf(line);
                    if (readLength <= kDirectionTolerance
                        || qAbs(Dot(inward / readLength, u)) > kPleatParallelCosine)
                    {
                        const QPointF selected
                            = SelectedLine(plan, edge.pointId, edge.position, secondSense ? 1 : -1, data);
                        if (not selected.isNull())
                        {
                            inward = selected;
                        }
                    }

                    if (inward.isNull())
                    {
                        inward = edge.edgeDirection; // the direction read at the point, when nothing else answers
                    }

                    const qreal inwardLength = QLineF(QPointF(), inward).length();
                    if (inwardLength <= kDirectionTolerance)
                    {
                        plan.failure = OPleatFailure::UnresolvedEdge;
                        return plan;
                    }

                    // A line carries it: the notch is a point on that line, and there is nothing to cut.
                    const QPointF unit = inward / inwardLength;
                    notch.point = from + ((Dot(unit, side) > 0.) ? unit : -unit) * values.at(departure);
                }

                plan.notches.append(notch);
            }
        }
    }

    return plan;
}
