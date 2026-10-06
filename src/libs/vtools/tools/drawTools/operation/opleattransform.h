/************************************************************************
 **
 **  @file   opleattransform.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The translation the pleats apply to the objects that travel.
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

#ifndef OPLEATTRANSFORM_H
#define OPLEATTRANSFORM_H

#include <QLineF>
#include <QPair>
#include <QPointF>
#include <QString>

#include "../../vgeometry/varc.h"
#include "../../vgeometry/vcubicbezier.h"
#include "../../vgeometry/vcubicbezierpath.h"
#include "../../vgeometry/vellipticalarc.h"
#include "../../vgeometry/vpointf.h"
#include "../../vgeometry/vspline.h"
#include "../../vgeometry/vsplinepath.h"
#include "opleatrule.h"

namespace OPleatAnchor
{
/** @brief The two ends of a pattern object, as the pleats ask about them: a point has the same one twice, and a
 *  curve answers with the first and the last point it carries.
 *
 *  A pleat takes the fabric beyond a line and moves it, so a curve that crosses a departure line has to be carried
 *  one way or the other, and no answer is exact: the pattern does not cut it. It is carried by its furthest end —
 *  the part of it the folds took the furthest — so that it never comes apart from the fabric it belongs to. */
inline auto Ends(const VPointF &point) -> QPair<QPointF, QPointF>
{
    const QPointF position = static_cast<QPointF>(point);
    return {position, position};
}

inline auto Ends(const VArc &arc) -> QPair<QPointF, QPointF>
{
    return {arc.GetP1(), arc.GetP2()};
}

inline auto Ends(const VEllipticalArc &arc) -> QPair<QPointF, QPointF>
{
    return {arc.GetP1(), arc.GetP2()};
}

inline auto Ends(const VSpline &spline) -> QPair<QPointF, QPointF>
{
    return {static_cast<QPointF>(spline.GetP1()), static_cast<QPointF>(spline.GetP4())};
}

inline auto Ends(const VCubicBezier &bezier) -> QPair<QPointF, QPointF>
{
    return {static_cast<QPointF>(bezier.GetP1()), static_cast<QPointF>(bezier.GetP4())};
}

/** @brief The first point of the first sub-spline, and the last point of the last one: what a path begins and ends
 *  at, whatever it is made of. A path the container holds is never empty, and the two are then the same answer. */
template <class Path> inline auto PathEnds(const Path &path, vsizetype count) -> QPair<QPointF, QPointF>
{
    if (count < 1)
    {
        return {};
    }

    const QPointF first = static_cast<QPointF>(path.GetSpline(1).GetP1());
    const QPointF last = static_cast<QPointF>(path.GetSpline(count).GetP4());
    return {first, last};
}

inline auto Ends(const VSplinePath &path) -> QPair<QPointF, QPointF>
{
    return PathEnds(path, path.CountSubSpl());
}

inline auto Ends(const VCubicBezierPath &path) -> QPair<QPointF, QPointF>
{
    return (path.CountPoints() >= 4) ? PathEnds(path, path.CountSubSpl()) : QPair<QPointF, QPointF>{};
}
} // namespace OPleatAnchor

/**
 * @brief The translation the pleats apply to the objects that travel.
 *
 *  A pleat takes the fabric beyond its departure line and moves it, so the objects do not all travel the same
 *  distance: one between two departure lines is carried by the first pleat alone, one beyond both by the two of
 *  them. The plan is what answers that question — it holds the lines and their carries — and every object is
 *  asked where it lies before it is moved.
 *
 *  Move(length, angle, name) is the shape every pattern object answers to, so the vector is turned into the
 *  couple the geometry expects, with the very convention the rest of the application uses (QLineF, as
 *  VPointF::MovePF does). An object the rule did not move, and a pleat the rule refused, leave a null vector,
 *  which moves nothing.
 */
class OPleatTransform
{
public:
    explicit OPleatTransform(const OPleatPlan &plan) : m_plan(plan) {}

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        const QPair<QPointF, QPointF> ends = OPleatAnchor::Ends(item);
        const QPointF fromFirst = m_plan.Move(ends.first);
        const QPointF fromLast = m_plan.Move(ends.second);

        // Every move is a distance along the one normal of the operation, so the furthest end is the longer vector.
        const QPointF move = (QLineF(QPointF(), fromLast).length() > QLineF(QPointF(), fromFirst).length()) ? fromLast
                                                                                                           : fromFirst;
        return item.Move(QLineF(QPointF(), move).length(), QLineF(QPointF(), move).angle(), name);
    }

private:
    /** The rule, run once: what each departure line is carried by, and which pleats a point lies on or beyond. */
    OPleatPlan m_plan;
};

#endif // OPLEATTRANSFORM_H
