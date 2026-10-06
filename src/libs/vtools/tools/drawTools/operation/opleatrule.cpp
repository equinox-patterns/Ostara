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
 *  base point it stands for than to the other one — a thousandth of the move is far enough, and small enough
 *  to leave the direction exact. */
constexpr qreal kCarrierDistance = 1e-3;
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
auto OPleatRule::SideOf(const QVector<SourceItem> &source, quint32 firstId, quint32 secondId,
                        const VContainer *data) -> OPleatSide
{
    if (data == nullptr || !data->HasObject(firstId) || !data->HasObject(secondId))
    {
        return OPleatSide::Undetermined;
    }

    const QSharedPointer<VPointF> first = qSharedPointerDynamicCast<VPointF>(data->GetGObject(firstId));
    const QSharedPointer<VPointF> second = qSharedPointerDynamicCast<VPointF>(data->GetGObject(secondId));
    if (first.isNull() || second.isNull())
    {
        return OPleatSide::Undetermined;
    }

    const QPointF origin = static_cast<QPointF>(*first);
    const QPointF prise = static_cast<QPointF>(*second) - origin;
    const qreal priseLength = QLineF(QPointF(), prise).length();
    if (priseLength < kDirectionTolerance)
    {
        return OPleatSide::Undetermined;
    }

    const QPointF u = prise / priseLength;
    const QPointF normal(-u.y(), u.x());
    const qreal tolerance = kDirectionTolerance * priseLength;

    int positive = 0;
    int negative = 0;
    for (const SourceItem &item : source)
    {
        // The two points of the prise lie on the line: they carry no side, and they are not asked for one.
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
            ++positive;
        }
        else if (side < -tolerance)
        {
            ++negative;
        }
    }

    if (positive > 0 && negative > 0)
    {
        return OPleatSide::Undetermined; // the selection straddles the prise: the pleat has no side
    }
    if (positive > 0)
    {
        return OPleatSide::First;
    }
    if (negative > 0)
    {
        return OPleatSide::Second;
    }

    return OPleatSide::Undetermined; // everything lies on the line: there is nothing to fold
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatRule::Compose(const OPleatEnd &first, const OPleatEnd &second, qreal value, OPleatKind kind,
                         OPleatSide side) -> OPleatPlan
{
    OPleatPlan plan;

    if (kind != OPleatKind::Double)
    {
        kind = OPleatKind::Simple;
    }

    if (side == OPleatSide::Undetermined)
    {
        // The prise has no side: either everything that travels lies on it, or the selection straddles it. Neither
        // can be folded, and the two are the same answer here — the caller says which in its own words.
        plan.failure = OPleatFailure::NoSide;
        return plan;
    }

    const QPointF prise = second.position - first.position;
    const qreal priseLength = QLineF(QPointF(), prise).length();
    if (priseLength < kDirectionTolerance)
    {
        plan.failure = OPleatFailure::DegeneratePrise;
        return plan;
    }
    if (qAbs(value) < kDirectionTolerance)
    {
        plan.failure = OPleatFailure::DegenerateValue;
        return plan;
    }

    const QPointF u = prise / priseLength;
    const QPointF normal = (side == OPleatSide::Second) ? QPointF(u.y(), -u.x()) : QPointF(-u.y(), u.x());

    const int pleats = (kind == OPleatKind::Double) ? 2 : 1;
    plan.step = normal * (2. * value);   // one pleat takes twice the value it shows
    plan.move = plan.step * pleats;      // and the objects that travel take all of it

    const qreal stepLength = QLineF(QPointF(), plan.step).length();

    for (int pleat = 0; pleat < pleats; ++pleat)
    {
        const QPointF shift = plan.step * pleat;                    // where this pleat starts on the prise
        const QPointF centre0 = first.position + shift + plan.step / 2.;  // the fold line, at the first end
        const QPointF centre1 = second.position + shift + plan.step / 2.; // and at the other one
        // The cap rule builds its axis from a point of the fold line and the middle of the opening; the middle
        // of the fold line itself is the point that is never confounded with either end.
        const QPointF apex = (centre0 + centre1) / 2.;

        for (int end = 0; end < 2; ++end)
        {
            const OPleatEnd &source = (end == 0) ? first : second;
            if (!source.showCap)
            {
                continue; // the user asked for no cap there: no mark, and so nothing for the fold line either
            }

            OPleatMark mark;
            mark.pleat = pleat;
            mark.end = end;
            mark.point = (end == 0) ? centre0 : centre1;

            const QPointF origin = source.position + shift;   // the point of the prise, for this pleat
            const QPointF copy = origin + plan.step;          // and its copy, where the fabric arrives

            const qreal directionLength = QLineF(QPointF(), source.edgeDirection).length();
            if (directionLength < kDirectionTolerance)
            {
                // No edge was read at that point: the cap cannot be built, and a pleat is never built halfway. The
                // caller asks the user to designate the point that carries the edge, and only comes back when it has
                // one — or says why it could not.
                plan.failure = OPleatFailure::UnresolvedEdge;
                return plan;
            }

            const QPointF e = source.edgeDirection / directionLength;
            const qreal alongMove = e.x() * normal.x() + e.y() * normal.y();

            if (qAbs(alongMove) > 1. - kDirectionTolerance)
            {
                // The edge runs parallel to the move: it is carried onto itself by the fold, it meets the fold
                // line at the centre, and both senses give that same point. There is no V to draw — it would
                // have no thickness. The centre is the whole mark.
                mark.flat = true;
                plan.marks.append(mark);
                continue;
            }

            const qreal alongFold = e.x() * u.x() + e.y() * u.y();
            if (qAbs(alongFold) > 1. - kDirectionTolerance)
            {
                // The edge runs along the fold line: the two never meet, so no cap point exists there.
                plan.failure = OPleatFailure::EdgeAlongFold;
                return plan;
            }

            // The line to extend: the piece's edge at the point of the prise (first sense), or the same edge
            // carried by the move (second sense, the edge the copy wears).
            //
            // The second pleat of a double is the first one reversed — the two folds lie opposite ways, which is the
            // whole difference between a pleat and the one beside it — so its cap opens the other way. Swapping the
            // two ends of its opening is what says so, and it is the only difference: the sides still join the same
            // two lines, only the point of the V moves to the other side of them.
            const bool secondSense = source.secondSense != (pleat == 1);
            const QPointF base = secondSense ? copy : origin;
            const QPointF carrier = base + e * (stepLength * kCarrierDistance);

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

    return plan;
}
