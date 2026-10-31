/************************************************************************
 **
 **  @file   ospreadtransform.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The transformation of a slash-and-spread: the composition of the rotations that carry one object.
 **
 **          Unlike a rotation, which turns every source around the same pivot, a slash-and-spread turns each
 **          object around a sequence of pivots that depends on where it sits in the ordered selection. The rule
 **          resolves that sequence once, into one list of steps per source id; this class only replays it, so
 **          the destination factory can use it exactly as it uses ORotationTransform.
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

#ifndef OSPREADTRANSFORM_H
#define OSPREADTRANSFORM_H

#include <QHash>
#include <type_traits>
#include <QLineF>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vgeometry/vspline.h"
#include "../../vpatterndb/vcontainer.h"
#include "ospreadrule.h"
#include "ospreadsmooth.h"

/**
 * @brief The transformation a slash-and-spread applies to one object.
 *
 * Points, arcs, splines and the three curve-with-segments types all answer to Rotate(origin, angle, name) with
 * the same signature, so one template serves every shape, exactly as a rotation does. The steps are read from a
 * table keyed by source id: each object has its own list of (pivot, angle), empty when no rotation carries it,
 * in which case the copy stays in place and only takes the new name.
 *
 * When the smoothing is on, the runs of consecutive curves are rebuilt into one C2 chain passing through their
 * junctions (see OSpreadSmooth), and each of them takes the geometry of its own segment back. The copy keeps
 * everything else it has — its name, its pen, its colour — and the objects of the pattern are the same, one per
 * source: nothing is added, nothing is cut.
 *
 * A point posed on a curve has no geometry of its own: it is a place on that curve. Turning it like a free point
 * would leave it beside the outline the moment the curve is smoothed, since the curve the rotation made is not
 * the one the chain gives. Such a point therefore follows its curve — it is put back at the place it had along
 * it, and the copy stays on the outline.
 */
class OSpreadTransform
{
public:
    OSpreadTransform(const QVector<SourceItem> &source, const QVector<OSpreadRotation> &rotations,
                     const QVector<qreal> &anglesDeg, const VContainer *data, bool smooth = false)
    {
        const OSpreadPlan plan = OSpreadRule::Compose(source, rotations, anglesDeg, data);
        const QVector<QVector<OSpreadStep>> &steps = plan.steps;
        m_stepsById.reserve(source.size());
        for (int i = 0; i < source.size(); ++i)
        {
            m_stepsById.insert(source.at(i).id, steps.at(i));
        }

        if (smooth)
        {
            BuildSmoothing(source, data);
        }
    }

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        Item result = Rotated(item, name);
        // Only a single cubic takes part in a chain: a path and an arc keep the image the rotations made, and
        // the test is on the type here rather than on the object, since that is what the template knows at
        // compile time.
        if constexpr (std::is_same_v<Item, VSpline>)
        {
            Smooth(result, item.id());
        }
        else if constexpr (std::is_same_v<Item, VPointF>)
        {
            // A point that sat on one of those curves is no longer where the rotation put it.
            PlaceOnCurve(result, item.id());
        }
        return result;
    }

private:
    QHash<quint32, QVector<OSpreadStep>> m_stepsById{};
    QHash<quint32, OSpreadSmoothImage> m_smoothed{};
    QHash<quint32, QPointF> m_pointsOnCurves{};

    /** @brief The steps applied, and nothing else: this is the image the operation has always made. */
    template <class Item> auto Rotated(const Item &item, const QString &name) const -> Item
    {
        Item result = item;

        // The key is the object's own id in the container (item.id()), not the id of the tool that drew it
        // (getIdObject()): the table is built from the source ids the operation was given.
        const auto it = m_stepsById.constFind(item.id());
        if (it == m_stepsById.cend() || it->isEmpty())
        {
            // No rotation carries this object: a copy in place, renamed. An angle of 0 turns nothing, whatever
            // the pivot.
            return result.Rotate(QPointF(0, 0), 0., name);
        }

        const QVector<OSpreadStep> &steps = it.value();
        for (int k = 0; k < steps.size(); ++k)
        {
            // Only the last step carries the name: the intermediate positions are never materialized, so their
            // names do not matter, and handing each one a name would stack suffixes on top of each other.
            const QString stepName = (k == steps.size() - 1) ? name : QString();
            result = result.Rotate(steps.at(k).pivot, steps.at(k).angle, stepName);
        }

        return result;
    }

    /** @brief Give the copy the geometry of its chain segment. Only the control points move: the name, the
     *  pen and the colour the rotation gave it stay. */
    auto Smooth(VSpline &image, quint32 id) const -> void
    {
        const auto it = m_smoothed.constFind(id);
        if (it == m_smoothed.cend())
        {
            return;
        }

        const OSpreadSmoothImage &segment = it.value();

        const VPointF p1 = image.GetP1();
        const VPointF p4 = image.GetP4();

        image.SetP1(VPointF(segment.p1, p1.name(), p1.mx(), p1.my()));
        image.SetP2(segment.p2);
        image.SetP3(segment.p3);
        image.SetP4(VPointF(segment.p4, p4.name(), p4.mx(), p4.my()));
    }

    /** @brief Put the copy back where its curve now passes. The place the point had along the curve is read off
     *  the curve it was posed on — the source, not the rotated one, since the point was placed on that one — and
     *  the same place is read back on the smoothed segment, which is drawn the same way round as its source. */
    auto PlaceOnCurve(VPointF &image, quint32 id) const -> void
    {
        const auto it = m_pointsOnCurves.constFind(id);
        if (it == m_pointsOnCurves.cend())
        {
            return;
        }

        image.setX(it.value().x());
        image.setY(it.value().y());
    }

    /** @brief The source points that were posed on a curve taking part in a chain, and the place they hold on it.
     *  The test is geometric because it is how a point is posed on a curve: the pattern keeps the curve and the
     *  point apart, and their meeting is only a place they both reach — a cut curve leaves its point exactly on
     *  it. A point whose distance is not zero to the last bit is a point of its own, and stays free. */
    void AttachPoints(const QVector<SourceItem> &source, const VContainer *data)
    {
        for (const SourceItem &item : std::as_const(source))
        {
            const QSharedPointer<VGObject> object = data->GetGObject(item.id);
            if (object.isNull() || object->getType() != GOType::Point)
            {
                continue;
            }

            const QPointF point = data->GeometricObject<VPointF>(item.id)->toQPointF();

            for (auto it = m_smoothed.cbegin(); it != m_smoothed.cend(); ++it)
            {
                const QSharedPointer<VSpline> curve = data->GeometricObject<VSpline>(it.key());
                const qreal t = curve->ParamT(point);
                if (qFloor(t) == -1 || QLineF(point, PlaceAt(*curve, t)).length() > 1e-3)
                {
                    continue; // Not on that curve: ParamT says so, or says it about another branch.
                }

                const OSpreadSmoothImage &segment = it.value();
                const VSpline smoothed(VPointF(segment.p1), segment.p2, segment.p3, VPointF(segment.p4));

                const QPointF placed = PlaceAt(smoothed, t);
                m_pointsOnCurves.insert(item.id, placed);
                break;
            }
        }
    }

    /** @brief The runs of curves, as the rotations give them, handed to OSpreadSmooth. A curve whose image is
     *  not a plain spline — a path, an arc — is left out: the chain is a chain of single cubic segments. */
    void BuildSmoothing(const QVector<SourceItem> &source, const VContainer *data)
    {
        QVector<OSpreadSmoothCurve> curves;
        curves.reserve(source.size());

        for (const SourceItem &item : std::as_const(source))
        {
            const QSharedPointer<VGObject> object = data->GetGObject(item.id);
            if (object.isNull() || object->getType() != GOType::Spline)
            {
                continue;
            }

            const VSpline raw = Rotated(*data->GeometricObject<VSpline>(item.id), QString());
            const QPointF start = raw.GetP1().toQPointF();
            const QPointF end = raw.GetP4().toQPointF();

            curves.append({.id = item.id,
                           .start = start,
                           .end = end,
                           .startTangent = Unit(raw.GetP2().toQPointF() - start),
                           .endTangent = Unit(end - raw.GetP3().toQPointF()),
                           .steps = m_stepsById.value(item.id)});
        }

        m_smoothed = OSpreadSmooth::Rebuild(curves);
        AttachPoints(source, data);
    }

    static auto Unit(const QPointF &vector) -> QPointF
    {
        const qreal length = std::hypot(vector.x(), vector.y());
        return length > 1e-12 ? QPointF(vector.x() / length, vector.y() / length) : QPointF(0., 0.);
    }

    /** @brief The place of parameter t on a curve. A cubic gives a point only by cutting itself, and that cut
     *  is precisely what a point posed on a curve already is: the two halves meet where the point is. */
    static auto PlaceAt(const VSpline &curve, qreal t) -> QPointF
    {
        QPointF firstHandle;
        QPointF firstMiddle;
        QPointF secondMiddle;
        QPointF secondHandle;
        return curve.CutSplineAtParam(t, firstHandle, firstMiddle, secondMiddle, secondHandle);
    }
};

#endif // OSPREADTRANSFORM_H
