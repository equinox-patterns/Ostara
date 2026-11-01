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

#include <QColor>
#include <QHash>
#include <type_traits>
#include <QLineF>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../ifc/ifcdef.h"
#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vgeometry/vspline.h"
#include "../../vgeometry/vsplinepath.h"
#include "../../vpatterndb/vcontainer.h"
#include "../vtheme/vtheme.h"
#include "ospreadrule.h"
#include "ospreadsmooth.h"

/**
 * @brief The ten colours that identify the curves of a slash-and-spread.
 *
 * A selection of curves has no name to read: the copies are "Curve 1", "Curve 2", and saying so twice in a row tells
 * nothing. Each curve the smoothing rebuilds therefore carries a colour, the same one in the tool's report and in the
 * preview over the canvas, so a line of the report can be pointed at on the drawing.
 *
 * Red and green open the list — the two most legible — and the eight others are the pen colours the pattern already
 * uses, minus magenta and orange: those sit too close to the red on one side and to the brown on the other, and two
 * curves a report makes look alike are worse than no report. The list wraps past ten curves.
 */
namespace OSpreadColors
{
inline constexpr int Count = 10;

inline auto Color(int rank) -> QColor
{
    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;

    switch ((rank - 1) % Count)
    {
        case 0:
            return dark ? QColor(0xFF, 0x5C, 0x5C) : QColor(0xC2, 0x18, 0x1C); // red
        case 1:
            return dark ? QColor(0x35, 0xC4, 0x6B) : QColor(0x00, 0x84, 0x3D); // green
        case 2:
            return ColorNameToQColorForTheme(ColorLightBlue, dark);
        case 3:
            return ColorNameToQColorForTheme(ColorDarkBlue, dark);
        case 4:
            return ColorNameToQColorForTheme(ColorPurple, dark);
        case 5:
            return ColorNameToQColorForTheme(ColorYellow, dark);
        case 6:
            return ColorNameToQColorForTheme(ColorLightGreen, dark);
        case 7:
            return ColorNameToQColorForTheme(ColorDarkGreen, dark);
        case 8:
            return ColorNameToQColorForTheme(ColorBrown, dark);
        default:
            return ColorNameToQColorForTheme(ColorBlack, dark);
    }
}
} // namespace OSpreadColors

/**
 * @brief The transformation a slash-and-spread applies to one object.
 *
 * Points, arcs, splines and the three curve-with-segments types all answer to Rotate(origin, angle, name) with
 * the same signature, so one template serves every shape, exactly as a rotation does. The steps are read from a
 * table keyed by source id: each object has its own list of (pivot, angle), empty when no rotation carries it,
 * in which case the copy stays in place and only takes the new name — a point, then, is not copied at all: it is
 * the point itself, and ReferencedPoints names it (see below).
 *
 * When the smoothing is on, the runs of consecutive curves are rebuilt into one C2 chain passing through their
 * junctions (see OSpreadSmooth), and each of them takes the geometry of its own segment back. A path brings each
 * of its sub-curves and takes them all back, so a selection of path pieces — the only way a curve can be cut —
 * is smoothed like a selection of single curves. The copy keeps everything else it has — its name, its pen, its
 * colour — and the objects of the pattern are the same, one per source: nothing is added, nothing is cut.
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

        // The rank of a curve is the rank of its line in the report: one per curve the chain rebuilt, in the order
        // of the selection. It is what gives the curve its colour, in the report and over the canvas alike, so both
        // read the same and cannot drift (see OSpreadColors).
        for (const SourceItem &item : source)
        {
            if (item.id == NULL_ID || SmoothedSize(item.id) == 0)
            {
                continue;
            }

            m_ranks.insert(item.id, ++m_rankCount);
        }

        // A point no rotation carries is left exactly where it is (see Rotated): its image is the point itself,
        // so copying it would put a second point on top of the first. The pivot of a rotation is that case — a
        // region stops at its pivot, so the rotation never turns it, and `B33` and `B33__R` would be one point in
        // two objects. The operation references such a point instead of copying it, and leaves it out of its layer
        // (see OOperationDestination and OToolSpread::KeptInPlace). A curve is copied all the same: a copy of a
        // curve is a shape the user names and the smoothing may redraw, where a copy of a point is the same point.
        m_referenced.reserve(source.size());
        for (const SourceItem &item : source)
        {
            if (item.id == NULL_ID || not m_stepsById.value(item.id).isEmpty() || not data->HasObject(item.id))
            {
                continue;
            }

            if (data->GetGObject(item.id)->getType() == GOType::Point)
            {
                m_referenced.append(item.id);
            }
        }
    }

    /** @brief The rank of the curve among the ones the chain rebuilt, 0 for a curve it left alone.
     *
     *  This is the curve's identity in the report: rank 1 is the first colour, rank 2 the second, and 0 means the
     *  curve has no line and no colour of its own — the rotations alone drew it. */
    auto Rank(quint32 id) const -> int { return m_ranks.value(id, 0); }

    /** @brief How many cubics the chain gave this object: 0 when the smoothing left it alone. */
    auto SmoothedSize(quint32 id) const -> qsizetype
    {
        const auto it = m_smoothed.constFind(id);
        return (it == m_smoothed.cend()) ? 0 : it->size();
    }

    /** @brief The source points the operation does not move, and therefore references instead of copying.
     *
     *  Returned by value, and that matters: a caller asks a temporary — `OSpreadTransform(...).ReferencedPoints()`
     *  — and a reference into an object that dies with the expression is read after it has gone. The list is a
     *  handful of ids, so the copy costs nothing next to a wrong answer. */
    auto ReferencedPoints() const -> QVector<quint32> { return m_referenced; }

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        Item result = Rotated(item, name);
        // A curve and a path take part in a chain, cubic by cubic; an arc keeps the image the rotations made,
        // and the test is on the type here rather than on the object, since that is what the template knows at
        // compile time.
        if constexpr (std::is_same_v<Item, VSpline> || std::is_same_v<Item, VSplinePath>)
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
    QVector<quint32> m_referenced{};
    QHash<quint32, int> m_ranks{};
    int m_rankCount{0};
    QHash<quint32, QVector<OSpreadSmoothImage>> m_smoothed{};
    QHash<quint32, QPointF> m_pointsOnCurves{};

    /** @brief The steps applied, and nothing else: this is the image the operation has always made. */
    template <class Item> auto Rotated(const Item &item, const QString &name) const -> Item
    {
        return Rotated(item, name, item.id());
    }

    /** @brief The same, with the object whose steps to follow given explicitly: the cubic of a path is not an
     *  object of the operation, it is a piece of one, and the steps are those of the path it belongs to. */
    template <class Item> auto Rotated(const Item &item, const QString &name, quint32 id) const -> Item
    {
        Item result = item;

        // The key is the object's own id in the container (item.id()), not the id of the tool that drew it
        // (getIdObject()): the table is built from the source ids the operation was given.
        const auto it = m_stepsById.constFind(id);
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
        if (it == m_smoothed.cend() || it->size() != 1)
        {
            return;
        }

        Give(image, it->constFirst());
    }

    /** @brief The same for a path: every one of its cubics takes the segment the smoothing gave it, and the path
     *  keeps its own knots — same points, same names, same everything the operation gave it. */
    auto Smooth(VSplinePath &image, quint32 id) const -> void
    {
        const auto it = m_smoothed.constFind(id);
        if (it == m_smoothed.cend() || it->size() != image.CountSubSpl())
        {
            return; // A path the smoothing did not cover whole keeps the image the rotations made.
        }

        // The knots are the ones the path already has: only the two handles that meet at a knot move, and they
        // are what the axis of a node carries. Rewriting a path from its cubics, on the other hand, loses a path
        // of a single cubic outright — that constructor hands back an empty path, and the copy disappears.
        QVector<VSplinePoint> nodes = image.GetSplinePath();
        if (nodes.size() != it->size() + 1)
        {
            return;
        }

        for (int k = 0; k < it->size(); ++k)
        {
            VSpline piece = image.GetSpline(k + 1); // GetSpline is 1-based.
            Give(piece, it->at(k));

            nodes[k].SetAngle2(piece.GetStartAngle(), piece.GetStartAngleFormula());
            nodes[k].SetLength2(piece.GetC1Length(), piece.GetC1LengthFormula());
            nodes[k + 1].SetAngle1(piece.GetEndAngle(), piece.GetEndAngleFormula());
            nodes[k + 1].SetLength1(piece.GetC2Length(), piece.GetC2LengthFormula());
        }

        VSplinePath rebuilt(nodes, image.getIdObject(), image.getMode());
        if (rebuilt.CountSubSpl() != image.CountSubSpl())
        {
            return;
        }

        rebuilt.setId(image.id());
        rebuilt.setName(image.name());
        rebuilt.SetAlias(image.GetAlias());
        rebuilt.SetAliasSuffix(image.GetAliasSuffix());
        rebuilt.SetColor(image.GetColor());
        rebuilt.SetPenStyle(image.GetPenStyle());
        rebuilt.SetApproximationScale(image.GetApproximationScale());
        image = rebuilt;
    }

    /** @brief The four control points of a cubic, the knots keeping the name and the label they had. */
    static auto Give(VSpline &curve, const OSpreadSmoothImage &segment) -> void
    {
        const VPointF p1 = curve.GetP1();
        const VPointF p4 = curve.GetP4();

        curve.SetP1(VPointF(segment.p1, p1.name(), p1.mx(), p1.my()));
        curve.SetP2(segment.p2);
        curve.SetP3(segment.p3);
        curve.SetP4(VPointF(segment.p4, p4.name(), p4.mx(), p4.my()));
    }

    /** @brief The cubics an object brings to the chain: itself for a curve, its sub-curves for a path, and
     *  nothing for an arc — a chain is a chain of cubics. */
    static auto Cubics(const QSharedPointer<VGObject> &object, const VContainer *data) -> QVector<VSpline>
    {
        if (object->getType() == GOType::Spline)
        {
            return {*data->GeometricObject<VSpline>(object->id())};
        }

        if (object->getType() == GOType::SplinePath)
        {
            const QSharedPointer<VSplinePath> path = data->GeometricObject<VSplinePath>(object->id());
            QVector<VSpline> cubics;
            cubics.reserve(path->CountSubSpl());
            for (vsizetype k = 1; k <= path->CountSubSpl(); ++k)
            {
                cubics.append(path->GetSpline(k));
            }
            return cubics;
        }

        return {};
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
     *  it. The tolerance is the one the rest of the application calls "on the curve" (`accuracyPointOnLine`):
     *  a point on a curve has to be the same thing for the smoothing and for the piece that walks it, or the
     *  contour stops following the curve the moment the smoothing moves it. A point further away is a point of
     *  its own, and stays free. */
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
                const QVector<VSpline> cubics = Cubics(data->GetGObject(it.key()), data);
                if (cubics.size() != it->size())
                {
                    continue;
                }

                bool placed = false;
                for (int k = 0; k < cubics.size() && not placed; ++k)
                {
                    const qreal t = cubics.at(k).ParamT(point);
                    if (qFloor(t) == -1 || QLineF(point, PlaceAt(cubics.at(k), t)).length() > accuracyPointOnLine)
                    {
                        continue; // Not on that cubic: ParamT says so, or says it about another branch.
                    }

                    const OSpreadSmoothImage &segment = it->at(k);
                    const VSpline smoothed(VPointF(segment.p1), segment.p2, segment.p3, VPointF(segment.p4));
                    m_pointsOnCurves.insert(item.id, PlaceAt(smoothed, t));
                    placed = true;
                }

                if (placed)
                {
                    break;
                }
            }
        }
    }

    /** @brief The runs of cubics, as the rotations give them, handed to OSpreadSmooth. A path brings each of its
     *  sub-curves: a curve of a piece of path is a cubic, and the chain is a chain of cubics. */
    void BuildSmoothing(const QVector<SourceItem> &source, const VContainer *data)
    {
        QVector<OSpreadSmoothCurve> curves;
        curves.reserve(source.size());

        for (const SourceItem &item : std::as_const(source))
        {
            const QSharedPointer<VGObject> object = data->GetGObject(item.id);
            if (object.isNull())
            {
                continue;
            }

            const QVector<VSpline> cubics = Cubics(object, data);
            for (int k = 0; k < cubics.size(); ++k)
            {
                // The steps are those of the object: a cubic of a path is not a source of the operation.
                const VSpline raw = Rotated(cubics.at(k), QString(), item.id);

                curves.append({.id = item.id,
                               .index = k,
                               .p1 = raw.GetP1().toQPointF(),
                               .p2 = raw.GetP2().toQPointF(),
                               .p3 = raw.GetP3().toQPointF(),
                               .p4 = raw.GetP4().toQPointF(),
                               .steps = m_stepsById.value(item.id)});
            }
        }

        m_smoothed = OSpreadSmooth::Rebuild(curves, Outline(source, data));
        AttachPoints(source, data);
    }

    /** @brief Every place where a curve the operation does not touch ends or starts, with the direction it takes
     *  there. A run has to meet that outline: the piece on the other side of a pivot did not take the rotation,
     *  so it is what tells the direction the run had before, and what a hand would follow. */
    auto Outline(const QVector<SourceItem> &source, const VContainer *data) const
        -> QVector<QPair<QPointF, QPointF>>
    {
        QVector<QPair<QPointF, QPointF>> outline;

        QSet<quint32> ours;
        for (const SourceItem &item : std::as_const(source))
        {
            ours.insert(item.id);
        }

        const auto *objects = data->CalculationGObjects();
        if (objects == nullptr)
        {
            return outline;
        }

        for (auto it = objects->cbegin(); it != objects->cend(); ++it)
        {
            if (ours.contains(it.key()) || it.value().isNull())
            {
                continue;
            }

            const QVector<VSpline> cubics = Cubics(it.value(), data);
            for (const VSpline &cubic : cubics)
            {
                const QPointF start = cubic.GetP1().toQPointF();
                const QPointF end = cubic.GetP4().toQPointF();

                outline.append({start, Unit(cubic.GetP2().toQPointF() - start)});
                outline.append({end, Unit(end - cubic.GetP3().toQPointF())});
            }
        }

        return outline;
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
