/************************************************************************
 **
 **  @file   opleatcaprule.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/10/2026
 **
 **  @brief  The cap of a pleat: where the piece's edge crosses the fold line, and the two sides the fold draws
 **          from it. See o-pleatcaprule.h for what a cap is and why a curved edge asks for the curve itself.
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

#include "opleatcaprule.h"

#include <QLineF>
#include <QString>

#include "../../../../vexception/vexception.h"
#include "../../vgeometry/varc.h"
#include "../../vgeometry/vcubicbezier.h"
#include "../../vgeometry/vcubicbezierpath.h"
#include "../../vgeometry/vellipticalarc.h"
#include "../../vgeometry/vgeometrydef.h"
#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vspline.h"
#include "../../vgeometry/vsplinepath.h"
#include "../../vpatterndb/vcontainer.h"
#include "../ifc/xml/vabstractpattern.h"
#include "oedgereader.h"
#include "opleatnotch.h"

namespace
{
/** How close to an end of the edge the point of a departure line has to stand for the cap of that end to be read on
 *  it. The lengths of a pattern are its own units — a departure point that a curve merely passes through is never a
 *  single unit from one of its ends — so the question is asked with a tolerance of that size and no more. */
constexpr qreal kEndTolerance = 1.;

//---------------------------------------------------------------------------------------------------------------------
/** @brief Where a line crosses a polyline, the line being taken as the line it is and not as the segment it is drawn
 *  as.
 *
 *  The geometry of the pattern answers with the intersections of the *segments* it draws, and the fold line is drawn
 *  between the two middles of the departure line: the piece's edge crosses it beyond that, exactly where the fold
 *  line enters and leaves the piece. That crossing is a crossing all the same, and it is the one a cap is made of. */
auto CrossingsOf(const QVector<QPointF> &points, const QLineF &line) -> QVector<QPointF>
{
    QVector<QPointF> crossings;

    const QPointF direction = line.p2() - line.p1();
    for (int i = 0; i + 1 < points.size(); ++i)
    {
        const QPointF start = points.at(i);
        const QPointF piece = points.at(i + 1) - start;
        const qreal denominator = direction.x() * piece.y() - direction.y() * piece.x();
        if (qFuzzyIsNull(denominator))
        {
            continue; // the line runs along that piece of the edge, or the piece is a point: they meet nowhere
        }

        const QPointF toStart = start - line.p1();
        const qreal where = (toStart.x() * direction.y() - toStart.y() * direction.x()) / denominator;
        // Written the positive way round: a piece of the edge the crossing misses, and a quotient that is not a
        // number at all, both fall out of it without a question of their own.
        if (where >= 0. && where <= 1.)
        {
            crossings.append(start + where * piece);
        }
    }

    return crossings;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Where the point of a departure line stands on a curve: its length from the curve's start, and how long the
 *  curve is. False when the point does not stand on that curve at all, or when the geometry refuses to be read. */
auto StandsOn(const QSharedPointer<VAbstractCurve> &curve, const QPointF &position, qreal *from, qreal *full) -> bool
{
    *from = -1.;
    *full = 0.;

    if (curve.isNull())
    {
        return false;
    }

    try
    {
        *full = curve->GetLength();
        *from = curve->GetLengthByPoint(position);
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        return false;
    }

    return *from >= 0.;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief True when the point of a departure line stands at one of the two ends of the curve, and not somewhere in
 *  the middle of it.
 *
 *  An edge is a piece of the piece's outline, and the outline **meets** the points of the pattern: the curve that
 *  carries it ends or begins there. A curve a point merely passes through is the object the outline was drawn from —
 *  the very curve a cut leaves behind beside its own halves — and the fold turns over the edge, never that object:
 *  its own ends are elsewhere, and the piece's edge is not read there. */
auto AtAnEnd(qreal from, qreal full) -> bool
{
    return from <= kEndTolerance || full - from <= kEndTolerance;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief One curve of the figure, moved by a plain offset.
 *
 *  Every kind of curve knows how to be moved, and none of them answers through the base class: this is only the
 *  dispatch that says which kind a curve is. The kinds are leaves — a spline is not a bezier, an arc is not an
 *  elliptical arc — so the order of the questions does not matter. */
auto MovedCurve(const QSharedPointer<VAbstractCurve> &curve, const QPointF &offset) -> QSharedPointer<VAbstractCurve>
{
    const QLineF move(QPointF(), offset);
    const qreal length = move.length();
    const qreal angle = move.angle();

    if (const auto spline = qSharedPointerDynamicCast<VSpline>(curve); not spline.isNull())
    {
        return QSharedPointer<VAbstractCurve>(new VSpline(spline->Move(length, angle)));
    }

    if (const auto bezier = qSharedPointerDynamicCast<VCubicBezier>(curve); not bezier.isNull())
    {
        return QSharedPointer<VAbstractCurve>(new VCubicBezier(bezier->Move(length, angle)));
    }

    if (const auto path = qSharedPointerDynamicCast<VSplinePath>(curve); not path.isNull())
    {
        return QSharedPointer<VAbstractCurve>(new VSplinePath(path->Move(length, angle)));
    }

    if (const auto path = qSharedPointerDynamicCast<VCubicBezierPath>(curve); not path.isNull())
    {
        return QSharedPointer<VAbstractCurve>(new VCubicBezierPath(path->Move(length, angle)));
    }

    if (const auto arc = qSharedPointerDynamicCast<VAbstractArc>(curve); not arc.isNull())
    {
        if (arc->getType() == GOType::EllipticalArc)
        {
            const auto ellipse = qSharedPointerDynamicCast<VEllipticalArc>(curve);
            return QSharedPointer<VAbstractCurve>(new VEllipticalArc(ellipse->Move(length, angle)));
        }

        const auto circle = qSharedPointerDynamicCast<VArc>(curve);
        return QSharedPointer<VAbstractCurve>(new VArc(circle->Move(length, angle)));
    }

    // A kind of curve no one has taught this function: answer nothing rather than hand the curve of the pattern
    // back, which the caller would then rename and recolour in place.
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief One curve of the figure, flipped about a line — the very reflection a fold is.
 *
 *  The same dispatch as for the move, and for the same reason. The name left empty is the caller's business: the
 *  sides of a cap are named after the points they join, and the geometry has no point of the pattern to offer
 *  (a flipped spline renames its own ends, see OPleatNotchRule::Cut). */
auto FlippedCurve(const QSharedPointer<VAbstractCurve> &curve, const QLineF &axis) -> QSharedPointer<VAbstractCurve>
{
    if (const auto spline = qSharedPointerDynamicCast<VSpline>(curve); not spline.isNull())
    {
        return QSharedPointer<VAbstractCurve>(new VSpline(spline->Flip(axis)));
    }

    if (const auto bezier = qSharedPointerDynamicCast<VCubicBezier>(curve); not bezier.isNull())
    {
        return QSharedPointer<VAbstractCurve>(new VCubicBezier(bezier->Flip(axis)));
    }

    if (const auto path = qSharedPointerDynamicCast<VSplinePath>(curve); not path.isNull())
    {
        return QSharedPointer<VAbstractCurve>(new VSplinePath(path->Flip(axis)));
    }

    if (const auto path = qSharedPointerDynamicCast<VCubicBezierPath>(curve); not path.isNull())
    {
        return QSharedPointer<VAbstractCurve>(new VCubicBezierPath(path->Flip(axis)));
    }

    if (const auto arc = qSharedPointerDynamicCast<VAbstractArc>(curve); not arc.isNull())
    {
        if (arc->getType() == GOType::EllipticalArc)
        {
            const auto ellipse = qSharedPointerDynamicCast<VEllipticalArc>(curve);
            return QSharedPointer<VAbstractCurve>(new VEllipticalArc(ellipse->Flip(axis)));
        }

        const auto circle = qSharedPointerDynamicCast<VArc>(curve);
        return QSharedPointer<VAbstractCurve>(new VArc(circle->Flip(axis)));
    }

    // See MovedCurve: nothing rather than the curve of the pattern itself.
    return {};
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPleatCapRule::Curved(const OPleatEnd &end, const QPointF &side, const QLineF &foldLine, const QPointF &offset,
                           const VContainer *data) -> OPleatCapPlacement
{
    OPleatCapPlacement placement;

    if (end.edgePointId != NULL_ID || data == nullptr)
    {
        // The user designated the point that carries the edge: a designated edge is a line, and a line has no shape
        // to follow. Nothing is asked of the pattern then, exactly as the notch asks nothing of it.
        return placement;
    }

    // The curve the edge runs on: what the reader of edges answers, as long as it answers with a curve the point
    // stands at an end of. A curve the point merely passes through is the object the outline was drawn from, and not
    // the edge of that end: the straight tracings then stay, as they always did.
    qreal from = -1.;
    qreal fullLength = 0.;
    QSharedPointer<VAbstractCurve> curve;

    const OEdgeCarrier carrier = OEdgeReader::Carrier(end.pointId, side, data);
    if (carrier.objectId != NULL_ID && data->HasObject(carrier.objectId))
    {
        curve = qSharedPointerDynamicCast<VAbstractCurve>(data->GetGObject(carrier.objectId));
        if (not StandsOn(curve, end.position, &from, &fullLength) || not AtAnEnd(from, fullLength))
        {
            // The object the point was drawn from, and not the edge of that end: a cut leaves it beside its own
            // halves, and it passes through the point without ending there.
            curve.clear();
            from = -1.;
        }
    }

    if (curve.isNull() || from < 0.)
    {
        return placement; // a line carries the edge: the two straight tracings are exact there, and they stay
    }

    const bool fromStart = from <= fullLength / 2.;

    try
    {
        // The first crossing walking away from the point of the departure line, whichever end it stands at: an edge
        // crosses the fold line once near each cap, and one further along belongs to the cap of another end.
        qreal length = -1.;
        QPointF crossing;
        for (const QPointF &candidate : CrossingsOf(curve->GetPoints(), foldLine))
        {
            const qreal where = curve->GetLengthByPoint(candidate);
            if (where < 0.)
            {
                continue;
            }

            if (length < 0. || (fromStart ? where < length : where > length))
            {
                length = where;
                crossing = candidate;
            }
        }

        if (length < 0.)
        {
            // The edge never meets the fold line on this curve: the two never cross before the curve runs out, and
            // the straight tracing stays — the same answer the figure gave before the edge was read as a shape.
            return placement;
        }

        placement.point = crossing + offset; // the apex, carried to the place the cap belongs to
        placement.edgeId = carrier.objectId;
        placement.length = length;
        placement.fromStart = fromStart;
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        // A curve the geometry refuses to read carries no cap of its own: fail closed, and the tangent stays.
        placement = OPleatCapPlacement{};
    }

    return placement;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatCapRule::Portion(const OPleatEnd &end, const QPointF &side, const QLineF &armLine,
                            const QLineF &foldLine, qreal value, const QPointF &armOffset, const VContainer *data)
    -> OPleatCapSides
{
    OPleatCapSides sides;

    if (end.edgePointId != NULL_ID || data == nullptr || value <= 0. || armLine.isNull() || foldLine.isNull())
    {
        return sides; // a designated edge is a line, and a line has no portion to follow
    }

    const OEdgeCarrier carrier = OEdgeReader::Carrier(end.pointId, side, data);
    if (carrier.objectId == NULL_ID || not data->HasObject(carrier.objectId))
    {
        return sides; // no curve carries the edge: the two straight tracings are exact, and they stay
    }

    const QSharedPointer<VAbstractCurve> curve
        = qSharedPointerDynamicCast<VAbstractCurve>(data->GetGObject(carrier.objectId));
    qreal from = -1.;
    qreal fullLength = 0.;
    if (curve.isNull() || not StandsOn(curve, end.position, &from, &fullLength))
    {
        return sides;
    }

    const bool fromStart = from <= fullLength / 2.;

    try
    {
        // **The portion: the edge, from the arm, over the value of the pleat.** Measured along the curve, as the
        // drawing measures it — a line follows the same rule, its own length being its direction.
        const qreal cut = fromStart ? (from + value) : (from - value);
        if (cut < 0. || cut > fullLength)
        {
            return sides; // the edge is shorter than the value: there is no portion to take
        }

        const OPleatNotchHalves halves = OPleatNotchRule::Cut(curve, cut, QString(), {}, {});
        const QSharedPointer<VAbstractCurve> portion = fromStart ? halves.first : halves.second;
        if (portion.isNull())
        {
            return sides;
        }

        // The edge is read where the pattern drew it, and a pleat only translates: the portion is carried to the
        // place of this arm, then turned over by the arm — that is what lays its far end on the fold line — and
        // turned over again by the fold line, which is what the fold does to it.
        const QSharedPointer<VAbstractCurve> atArm = MovedCurve(portion, armOffset);
        sides.edge = FlippedCurve(atArm, armLine);
        sides.image = FlippedCurve(sides.edge, foldLine);

        if (sides.edge.isNull() || sides.image.isNull())
        {
            return OPleatCapSides{};
        }

        // The tip: where the side the arm turned over meets the fold line. Its far end is the meeting the drawing
        // shows; when the portion runs past it, the crossing is what the fold reads.
        const QVector<QPointF> points = sides.edge->GetPoints();
        if (not points.isEmpty())
        {
            sides.tip = points.last();
        }

        for (const QSharedPointer<VAbstractCurve> &one : {sides.edge, sides.image})
        {
            one->SetDerivative(true);
            one->SetColor(curve->GetColor());
            one->SetPenStyle(curve->GetPenStyle());
        }
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        sides = OPleatCapSides{};
    }

    return sides;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatCapRule::Sides(const OPleatMark &mark, const OPleatEnd &prise, const QPointF &origin, const QLineF &foldLine,
                          const QString &apexName, const VContainer *data) -> OPleatCapSides
{
    OPleatCapSides sides;

    if (mark.edgeId == NULL_ID || apexName.isEmpty() || data == nullptr || not data->HasObject(mark.edgeId))
    {
        return sides;
    }

    const QSharedPointer<VAbstractCurve> curve
        = qSharedPointerDynamicCast<VAbstractCurve>(data->GetGObject(mark.edgeId));
    if (curve.isNull())
    {
        return sides;
    }

    // The edge is cut where the apex stands, and the half that runs from the point of the departure line to it is
    // the side the fold turns over: the other half is the rest of the edge, which no cap follows.
    const OPleatNotchHalves halves = OPleatNotchRule::Cut(curve, mark.length, apexName, {}, {});
    const QSharedPointer<VAbstractCurve> piece = mark.fromStart ? halves.first : halves.second;
    if (piece.isNull())
    {
        return sides;
    }

    // The edge is read where the pattern drew it, and a pleat only translates: what carries the piece to the place
    // of this pleat is the offset between that place and the point as it is drawn.
    const QPointF offset = origin - prise.position;

    sides.edge = MovedCurve(piece, offset);
    sides.image = FlippedCurve(sides.edge, foldLine);

    if (sides.edge.isNull() || sides.image.isNull())
    {
        return OPleatCapSides{};
    }

    // The two sides are objects of the operation — the piece is built through the figure, and neither is a curve to
    // be cut in turn — and they wear the look of the edge they came from until the figure gives them its own.
    for (const QSharedPointer<VAbstractCurve> &side : {sides.edge, sides.image})
    {
        side->SetDerivative(true);
        side->SetColor(curve->GetColor());
        side->SetPenStyle(curve->GetPenStyle());
    }

    return sides;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatCapRule::Moved(const QSharedPointer<VAbstractCurve> &curve, const QPointF &offset)
    -> QSharedPointer<VAbstractCurve>
{
    if (curve.isNull())
    {
        return {};
    }

    return offset.isNull() ? curve : MovedCurve(curve, offset);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatCapRule::Dart(const QSharedPointer<VAbstractCurve> &edge, const QPointF &point, const QLineF &arm,
                         const QLineF &axis, const QString &apexName, const QPointF &offset, QPointF *apex,
                         qreal *length) -> OPleatCapSides
{
    OPleatCapSides sides;

    if (apex != nullptr)
    {
        *apex = QPointF();
    }
    if (length != nullptr)
    {
        *length = -1.;
    }

    if (edge.isNull() || apexName.isEmpty())
    {
        return sides;
    }

    try
    {
        // The edge has to end at the point of the opening: a curve the point merely passes through is the object the
        // outline was drawn from, never the edge of that end — the same question Curved asks of a pleat's cap.
        qreal from = -1.;
        qreal fullLength = 0.;
        if (not StandsOn(edge, point, &from, &fullLength) || not AtAnEnd(from, fullLength))
        {
            return sides;
        }

        // The arm and the fold line **as the pattern draws the edge**: the two are given at the place the cap belongs,
        // and the edge may be drawn elsewhere — a copy whose operation copied the point and not the curve. Reflecting
        // about the arm moved back is reflecting the curve drawn where it is, and the figure is carried forward at the
        // end (see the header).
        const QLineF armAtEdge(arm.p1() - offset, arm.p2() - offset);
        const QLineF axisAtEdge(axis.p1() - offset, axis.p2() - offset);

        // THE FIRST FOLD: the piece of the edge is brought onto the fold by the reflection about the arm. The point
        // of the opening lies on that arm, so it stays where it is, and the curve that comes out of it runs from
        // there to the fold line — which is what makes the cap of a dart, and what a pleat has no need of: there the
        // two axes are parallel and the edge already ends on the fold.
        const QSharedPointer<VAbstractCurve> carried = FlippedCurve(edge, armAtEdge);
        if (carried.isNull())
        {
            return sides;
        }

        qreal carriedFrom = -1.;
        qreal carriedFull = 0.;
        if (not StandsOn(carried, point, &carriedFrom, &carriedFull))
        {
            return sides;
        }

        const bool fromStart = carriedFrom <= carriedFull / 2.;

        // THE SECOND FOLD meets it at the point C: the first crossing walking away from the point of the opening,
        // read exactly as Curved reads the apex of a pleat's cap. The curve STOPS there — what runs on past the fold
        // is the rest of the edge of the piece, and no cap follows it.
        qreal cut = -1.;
        QPointF crossing;
        for (const QPointF &candidate : CrossingsOf(carried->GetPoints(), axisAtEdge))
        {
            const qreal where = carried->GetLengthByPoint(candidate);
            if (where < 0.)
            {
                continue;
            }

            if (cut < 0. || (fromStart ? where < cut : where > cut))
            {
                cut = where;
                crossing = candidate;
            }
        }

        if (cut < 0.)
        {
            // The carried curve never meets the fold line: no cap is built on it, and the straight tracings stand.
            return sides;
        }

        const OPleatNotchHalves halves = OPleatNotchRule::Cut(carried, cut, apexName, {}, {});
        const QSharedPointer<VAbstractCurve> piece = fromStart ? halves.first : halves.second;
        if (piece.isNull())
        {
            return OPleatCapSides{};
        }

        // The side is carried to the place the cap belongs; a null offset leaves it exactly as the pattern drew it.
        sides.edge = offset.isNull() ? piece : MovedCurve(piece, offset);
        if (sides.edge.isNull())
        {
            return OPleatCapSides{};
        }

        // THE FOLD itself: the first side comes back on the copy by the reflection about the fold line, which is the
        // perpendicular bisector of the two points of the opening — so it starts where the copy stands.
        sides.image = FlippedCurve(sides.edge, axis);
        if (sides.image.isNull())
        {
            return OPleatCapSides{};
        }

        // See Sides: the two sides are objects of the operation, and neither is a curve to be cut in turn.
        for (const QSharedPointer<VAbstractCurve> &side : {sides.edge, sides.image})
        {
            side->SetDerivative(true);
        }

        if (apex != nullptr)
        {
            *apex = crossing + offset; // the apex, carried to the place the cap belongs
        }
        if (length != nullptr)
        {
            *length = cut;
        }
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        // A curve the geometry refuses to read carries no cap of its own: fail closed, and the tracings stay.
        return OPleatCapSides{};
    }

    return sides;
}
