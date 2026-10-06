/************************************************************************
 **
 **  @file   o-pleatnotch.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/10/2026
 **
 **  @brief  Where a notch falls on the piece's edge: see the header for what it is meant to answer.
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

#include "opleatnotch.h"

#include <QLineF>
#include <QScopedPointer>
#include <QSharedPointer>

#include "../../vgeometry/vabstractarc.h"
#include "../../vgeometry/vabstractcubicbezier.h"
#include "../../vgeometry/vabstractcubicbezierpath.h"
#include "../../vgeometry/varc.h"
#include "../../vgeometry/vellipticalarc.h"
#include "../../vgeometry/vgeometrydef.h"
#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vgeometry/vspline.h"
#include "../../vgeometry/vsplinepath.h"
#include "../../vmisc/def.h"
#include "../../../../vexception/vexception.h"
#include "../toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/** @brief The two ends of a curve, whichever kind it is.
 *
 *  The walk of a notch starts at an end of the edge, and which of its two ends that is decides whether the value is
 *  counted from the start or taken off the end. The kinds do not agree on how to name their ends — a bezier and a
 *  path hold points, an arc only a pair of positions — so the answer is read here, once, and every other question
 *  of the notch is asked in those terms. */
auto EndsOf(const QSharedPointer<VAbstractCurve> &curve, QPointF *start, QPointF *end) -> bool
{
    if (curve.isNull())
    {
        return false;
    }

    if (const auto bezier = qSharedPointerDynamicCast<VAbstractCubicBezier>(curve); not bezier.isNull())
    {
        *start = static_cast<QPointF>(bezier->GetP1());
        *end = static_cast<QPointF>(bezier->GetP4());
        return true;
    }

    if (const auto path = qSharedPointerDynamicCast<VAbstractCubicBezierPath>(curve); not path.isNull())
    {
        *start = static_cast<QPointF>(path->FirstPoint());
        *end = static_cast<QPointF>(path->LastPoint());
        return true;
    }

    if (const auto arc = qSharedPointerDynamicCast<VAbstractArc>(curve); not arc.isNull())
    {
        *start = arc->GetP1();
        *end = arc->GetP2();
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The point of a curve at a length measured from its start, or the origin when its kind carries none.
 *
 *  A cut is asked of the geometry itself, exactly as the cut tools ask it, and only its point is kept here: the
 *  halves are the business of Cut. */
auto PointAtLength(const QSharedPointer<VAbstractCurve> &curve, qreal length) -> QPointF
{
    try
    {
        if (const auto bezier = qSharedPointerDynamicCast<VAbstractCubicBezier>(curve); not bezier.isNull())
        {
            QPointF firstP2;
            QPointF firstP3;
            QPointF secondP2;
            QPointF secondP3;
            return bezier->CutSpline(length, firstP2, firstP3, secondP2, secondP3);
        }

        if (const auto path = qSharedPointerDynamicCast<VAbstractCubicBezierPath>(curve); not path.isNull())
        {
            qint32 first = 0;
            qint32 second = 0;
            QPointF firstP2;
            QPointF firstP3;
            QPointF secondP2;
            QPointF secondP3;
            return path->CutSplinePath(length, first, second, firstP2, firstP3, secondP2, secondP3, QString());
        }

        if (const auto arc = qSharedPointerDynamicCast<VAbstractArc>(curve); not arc.isNull())
        {
            return arc->CutArc(length, QString());
        }
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        // A curve the geometry refuses to cut carries no notch: fail closed, the caller says so rather than
        // marking a place that does not exist.
    }

    return {};
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPleatNotchRule::Place(const QSharedPointer<VAbstractCurve> &curve, const QPointF &from, qreal value)
    -> OPleatNotchPlacement
{
    OPleatNotchPlacement placement;

    if (curve.isNull() || value <= 0.)
    {
        placement.length = -1.;
        return placement;
    }

    QPointF start;
    QPointF end;
    if (not EndsOf(curve, &start, &end))
    {
        placement.length = -1.; // a curve no cut can split: there is no length to speak of
        return placement;
    }

    const bool atStart = VFuzzyComparePoints(start, from);
    if (not atStart && not VFuzzyComparePoints(end, from))
    {
        placement.length = -1.; // the walk starts at an end of the edge, and this point is not one
        return placement;
    }

    try
    {
        const qreal fullLength = curve->GetLength();
        if (value > fullLength)
        {
            // The edge runs out before the value: the folded edge comes back onto a neighbour, and no notch of
            // this pleat lies on the edge it was asked to mark.
            placement.length = -1.;
            return placement;
        }

        placement.length = atStart ? value : fullLength - value;
        placement.point = PointAtLength(curve, placement.length);
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        // A curve the geometry refuses to cut carries no notch: fail closed, the pleat says so rather than
        // marking a place that does not exist.
        placement = OPleatNotchPlacement{};
        placement.length = -1.;
    }

    return placement;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatNotchRule::Cut(const QSharedPointer<VAbstractCurve> &curve, qreal length, const QString &notchName,
                          const QString &startName, const QString &endName) -> OPleatNotchHalves
{
    OPleatNotchHalves halves;

    if (curve.isNull() || notchName.isEmpty() || length < 0.)
    {
        return halves;
    }

    const QPointF cut = PointAtLength(curve, length);
    if (cut.isNull())
    {
        return halves;
    }

    // The point the two halves share is the notch itself: its name is what the halves take to name their junction.
    VPointF notch(cut);
    notch.setName(notchName);

    if (const auto bezier = qSharedPointerDynamicCast<VAbstractCubicBezier>(curve); not bezier.isNull())
    {
        const VToolCutSpline::SubCurves pieces = VToolCutSpline::SplitCurve(length, bezier);

        VSpline first;
        VSpline second;
        VToolCutSpline::BuildSubCurves(pieces, bezier, notch, first, second);
        halves.first = QSharedPointer<VAbstractCurve>(new VSpline(first));
        halves.second = QSharedPointer<VAbstractCurve>(new VSpline(second));
    }
    else if (const auto arc = qSharedPointerDynamicCast<VAbstractArc>(curve); not arc.isNull())
    {
        // The geometry fills in a concrete arc, so the kind has to be named before the call: an arc and an
        // elliptical arc are two different shapes, and the cut keeps whichever it was given.
        if (arc->getType() == GOType::Arc)
        {
            VArc first;
            VArc second;
            arc->CutArc(length, &first, &second, notchName);
            halves.first = QSharedPointer<VAbstractCurve>(new VArc(first));
            halves.second = QSharedPointer<VAbstractCurve>(new VArc(second));
        }
        else if (arc->getType() == GOType::EllipticalArc)
        {
            VEllipticalArc first;
            VEllipticalArc second;
            arc->CutArc(length, &first, &second, notchName);
            halves.first = QSharedPointer<VAbstractCurve>(new VEllipticalArc(first));
            halves.second = QSharedPointer<VAbstractCurve>(new VEllipticalArc(second));
        }
    }
    else if (const auto path = qSharedPointerDynamicCast<VAbstractCubicBezierPath>(curve); not path.isNull())
    {
        VSplinePath *first = nullptr;
        VSplinePath *second = nullptr;
        // The helper hands its own point back; the halves carry their junction by value, so it only has to be
        // released. They are handed over as they came: the cut is the cut tool's own, junction handles included.
        const QScopedPointer<VPointF> point(VToolCutSplinePath::CutSplinePath(length, path, notchName, &first, &second));

        if (first != nullptr && second != nullptr)
        {
            halves.first = QSharedPointer<VAbstractCurve>(first);
            halves.second = QSharedPointer<VAbstractCurve>(second);
        }
    }

    if (not halves.first.isNull() && not halves.second.isNull())
    {
        // The names are the ones the ends give — the point the edge starts at to the notch, and the notch to the point
        // it ends at — and they are read as the pattern shows them, not as the geometry of the carried curve names its
        // own control points (X1 and X4, see OPleatNotchRule::Cut). Read before the halves are marked derivative, which
        // is what makes them exist only through the figure; an end the caller could not name keeps the name it gives.
        halves.firstName = startName.isEmpty() ? halves.first->HeadlessName()
                                               : startName + QStringLiteral("_") + notchName;
        halves.secondName = endName.isEmpty() ? halves.second->HeadlessName()
                                              : notchName + QStringLiteral("_") + endName;
        halves.first->SetDerivative(true);
        halves.second->SetDerivative(true);

        // They carry the look of the edge they were cut from, exactly as the cut tools give it to their own pieces:
        // should the two be shown one day, they read as the curve they came from, and not as two strangers.
        for (const QSharedPointer<VAbstractCurve> &half : {halves.first, halves.second})
        {
            half->SetColor(curve->GetColor());
            half->SetPenStyle(curve->GetPenStyle());
        }
    }
    else
    {
        halves = OPleatNotchHalves{};
    }

    return halves;
}
