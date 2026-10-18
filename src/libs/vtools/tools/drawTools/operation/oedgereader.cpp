/************************************************************************
 **
 **  @file   oedgereader.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/10/2026
 **
 **  @brief  The line of the pattern carried by a point: the edge of the piece the point sits on.
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

#include "oedgereader.h"

// The tolerance of a direction is the rule's, written once: this reader is the layer below and takes it from
// there rather than keep a second copy of the same value.
#include "opleatrule.h"

#include <algorithm>

#include <QLineF>
#include <QPair>
#include <QPointF>
#include <QSet>
#include <QSharedPointer>
#include <QVector>

#include "../../vabstracttool.h"
#include "../../vgeometry/vabstractarc.h"
#include "../../vgeometry/vabstractcubicbezier.h"
#include "../../vgeometry/vabstractcubicbezierpath.h"
#include "../../vgeometry/vabstractcurve.h"
#include "../../vgeometry/varc.h"
#include "../../vgeometry/vellipticalarc.h"
#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vgeometrydef.h"
#include "../../vgeometry/vpointf.h"
#include "../../vgeometry/vspline.h"
#include "../../vgeometry/vsplinepath.h"
#include "../../vlayout/vsapoint.h"
#include "../../vpatterndb/vcontainer.h"
#include "../../vpatterndb/vpiece.h"
#include "../../vpatterndb/vpiecenode.h"
#include "../../vpatterndb/vpiecepath.h"
#include "../../../../vexception/vexception.h"
#include "../ifc/xml/vabstractpattern.h"

namespace
{
/** Below this length a vector carries no direction at all: a degenerate segment, or two points that the pattern
 *  put on top of each other, answer as little as a point that has no edge. */
constexpr qreal kDirectionTolerance = kPleatTolerance; // one value, written in the rule's header

/** Two directions describe the same line when the sine of the angle between them is below this. On a piece a
 *  hundred centimetres wide that is a deviation of a hundredth of a millimetre: a corner is never mistaken for a
 *  straight line, and the two halves of one cut — computed apart — always agree. */
constexpr qreal kSameLineTolerance = 1e-4;

//---------------------------------------------------------------------------------------------------------------------
/** @brief Whether two directions describe the same line.
 *
 *  The fold reads a line, so a vector and its opposite are one answer — and two halves of one cut, computed
 *  apart, give slightly different numbers for the same line. */
auto SameLine(const QPointF &one, const QPointF &other) -> bool
{
    const qreal oneLength = QLineF(QPointF(), one).length();
    const qreal otherLength = QLineF(QPointF(), other).length();
    if (oneLength <= kDirectionTolerance || otherLength <= kDirectionTolerance)
    {
        return false;
    }

    const qreal cross = (one.x() * other.y() - one.y() * other.x()) / (oneLength * otherLength);
    return qAbs(cross) <= kSameLineTolerance;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The one line a set of candidates describes, a null vector when they disagree.
 *
 *  Two different lines through one point mean a corner or a crossing: nothing there may be read, and the caller
 *  asks the user to designate the edge. This is what tells the two halves of one cut — the same line — from the
 *  two curves of an intersection, which meet at an angle, or the two sides of a corner. */
auto AgreedDirection(const QVector<QPointF> &directions) -> QPointF
{
    QPointF answer{};
    for (const QPointF &direction : directions)
    {
        if (QLineF(QPointF(), direction).length() <= kDirectionTolerance)
        {
            continue;
        }

        if (answer.isNull())
        {
            answer = direction;
            continue;
        }

        if (not SameLine(answer, direction))
        {
            return {};
        }
    }

    return answer;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The tangent at one of the two ends of a path of cubic beziers: the handle leaving that end.
 *
 *  The sub-splines of a path are numbered from one — `GetSpline(0)` throws — and the last one carries the end of
 *  the path, not the one before it. */
auto TangentAtPath(const VAbstractCubicBezierPath &path, const QPointF &point) -> QPointF
{
    const vsizetype count = path.CountSubSpl();
    if (count < 1)
    {
        return {};
    }

    const VSpline first = path.GetSpline(1);
    if (VFuzzyComparePoints(static_cast<QPointF>(first.GetP1()), point))
    {
        return static_cast<QPointF>(first.GetP2()) - static_cast<QPointF>(first.GetP1());
    }

    const VSpline last = path.GetSpline(count);
    if (VFuzzyComparePoints(static_cast<QPointF>(last.GetP4()), point))
    {
        return static_cast<QPointF>(last.GetP4()) - static_cast<QPointF>(last.GetP3());
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The dot product of two vectors: positive when they leave the same way, which is how a side is answered. */
auto Dot(const QPointF &left, const QPointF &right) -> qreal
{
    return left.x() * right.x() + left.y() * right.y();
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The tangent of a curve at one of its two ends, or a null vector when @a point is not one of them.
 *
 *  A bezier curve stores the direction of each end as the handle leaving it, so the tangent is read exactly — and
 *  a path of them reads the handle of its first or last sub-spline. A circular arc is tangent to the radius at
 *  every point, so the perpendicular of the radius is its tangent line, in either direction, since only the line
 *  is read. An elliptical arc is no such circle, but it is drawn through the spline path it builds for itself
 *  (`ToSplinePath`), whose handles touch the ellipse exactly at its ends — where a cut always places its point. */
auto TangentAt(const QSharedPointer<VAbstractCurve> &curve, const QPointF &point) -> QPointF
{
    if (curve.isNull())
    {
        return {};
    }

    if (const auto bezier = qSharedPointerDynamicCast<VAbstractCubicBezier>(curve); not bezier.isNull())
    {
        const QPointF start = static_cast<QPointF>(bezier->GetP1());
        if (VFuzzyComparePoints(start, point))
        {
            return static_cast<QPointF>(bezier->GetP2()) - start; // the first handle lies along the tangent
        }

        const QPointF end = static_cast<QPointF>(bezier->GetP4());
        if (VFuzzyComparePoints(end, point))
        {
            return end - static_cast<QPointF>(bezier->GetP3()); // and the second one, from the other end
        }

        return {};
    }

    if (const auto path = qSharedPointerDynamicCast<VAbstractCubicBezierPath>(curve); not path.isNull())
    {
        return TangentAtPath(*path, point);
    }

    if (const auto ellipse = qSharedPointerDynamicCast<VEllipticalArc>(curve); not ellipse.isNull())
    {
        // Its spline path is expensive to build — the whole arc is sampled into it — so the two ends are asked
        // first, and the path is only built for the one point it can answer for.
        if (not VFuzzyComparePoints(ellipse->GetP1(), point) && not VFuzzyComparePoints(ellipse->GetP2(), point))
        {
            return {};
        }

        return TangentAtPath(ellipse->ToSplinePath(), point);
    }

    if (const auto arc = qSharedPointerDynamicCast<VArc>(curve); not arc.isNull())
    {
        // A circle is tangent to the radius at every point of it, but this arc is not the whole circle: only its
        // two ends belong to the curve, and the perpendicular of the radius elsewhere is the tangent of a point
        // the arc does not carry.
        if (not VFuzzyComparePoints(arc->GetP1(), point) && not VFuzzyComparePoints(arc->GetP2(), point))
        {
            return {};
        }

        const QPointF radius = point - static_cast<QPointF>(arc->GetCenter());
        return {-(radius.y()), radius.x()};
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The one segment a tool draws through a point, as a direction: the line that point sits on.
 *
 *  This is the line the point's **own tool** drew from it — the base segment of an alongLine or an endLine, the
 *  axis of a height, the side a rectangle draws towards it. A tool that draws two of them through the same point
 *  means a corner — the vertex of a triangle, the meeting of a side and a diagonal — and nothing here may choose
 *  between two edges: the answer is then empty, exactly as for a point that carries none.
 *
 *  Two of them on **one straight line** are not a corner, though: they are the two halves a cut made of that
 *  line, which is what an alongLine draws through the point it created. The line is then read as the tool
 *  answered before it declared the second half — the first half names it — and only two edges that do not lie
 *  on the same line meet. */
auto DirectionFromSegments(quint32 sourceId, const VContainer *data) -> QPointF
{
    auto *tool = qobject_cast<VAbstractTool *>(VAbstractPattern::FindTool(sourceId));
    if (tool == nullptr)
    {
        return {};
    }

    const QPointF position = OEdgeReader::Position(sourceId, data);

    quint32 other = NULL_ID;
    QPointF direction;
    for (const VDrawnSegment &segment : tool->DrawnSegments())
    {
        quint32 candidate = NULL_ID;
        if (segment.point1 == sourceId)
        {
            candidate = segment.point2;
        }
        else if (segment.point2 == sourceId)
        {
            candidate = segment.point1;
        }
        else
        {
            continue;
        }

        if (other == NULL_ID)
        {
            other = candidate;
            direction = position - OEdgeReader::Position(candidate, data);
            continue;
        }

        // The same edge declared twice says nothing more about the line.
        if (other == candidate)
        {
            continue;
        }

        if (SameLine(direction, position - OEdgeReader::Position(candidate, data)))
        {
            continue; // the other half of one straight line: the answer above is still that line
        }

        return {}; // two edges through one point: a corner, and no way to tell which one the fold follows
    }

    if (other == NULL_ID)
    {
        return {};
    }

    return direction;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The tangent, at a point, of a curve of the pattern that ends there.
 *
 *  With @a toolId, only what that very tool produced is read: the two halves a cut made of its parent, both of
 *  them carrying the tool's id and both ending at the cut point. A crossing leaves four such curves, and the two
 *  curves that meet there are not tangent — the candidates then describe two lines and nothing is read.
 *
 *  Without it, every curve of the pattern is read, and only the position links a curve to its point: a curve
 *  names nobody, and nothing says which tool drew it. That is what answers for the end of an arc or a spline, and
 *  for the copy an operation made of a curve. */
auto DirectionFromCurves(const QPointF &position, const VContainer *data, quint32 toolId = NULL_ID) -> QPointF
{
    const auto *objects = data->CalculationGObjects();
    if (objects == nullptr)
    {
        return {};
    }

    QVector<QPointF> directions;
    for (auto it = objects->constBegin(); it != objects->constEnd(); ++it)
    {
        const QSharedPointer<VGObject> &object = it.value();
        if (object.isNull() || (toolId != NULL_ID && object->getIdObject() != toolId))
        {
            continue;
        }

        const QPointF direction = TangentAt(qSharedPointerDynamicCast<VAbstractCurve>(object), position);
        if (QLineF(QPointF(), direction).length() > kDirectionTolerance)
        {
            directions.append(direction);
        }
    }

    return AgreedDirection(directions);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The tangent of a curve at one of its ends, oriented from the point **into** the curve.
 *
 *  @a TangentAt answers along the curve's travel, so at the start of a curve it points into it and at its end out of
 *  it — and which of the two this point is would take, per type, the very casts the reader means to keep in one
 *  place. The middle of the curve says it without any of that: a tangent turned towards a point further along the
 *  curve is the one that goes into it. */
auto InwardTangent(const QSharedPointer<VAbstractCurve> &curve, const QPointF &point) -> QPointF
{
    const QPointF tangent = TangentAt(curve, point);
    if (tangent.isNull())
    {
        return {};
    }

    const QPointF middle = static_cast<QPointF>(curve->GetMidpoint());
    return (Dot(tangent, middle - point) < 0.) ? -tangent : tangent;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The curves of the pattern that end at a point, each with the way that runs from the point into it.
 *
 *  With @a toolId, only what that very tool produced is read — the two halves a cut made of its parent, which is
 *  what a notch has to cut in its turn. Without it, every curve is read, and only the position links one to the
 *  point: a curve names nobody. */
void CandidatesFromCurves(const QPointF &position, const VContainer *data, quint32 toolId,
                          QVector<OEdgeCarrier> *candidates)
{
    const auto *objects = data->CalculationGObjects();
    if (objects == nullptr)
    {
        return;
    }

    for (auto it = objects->constBegin(); it != objects->constEnd(); ++it)
    {
        const QSharedPointer<VGObject> &object = it.value();
        if (object.isNull() || (toolId != NULL_ID && object->getIdObject() != toolId))
        {
            continue;
        }

        const QPointF inward = InwardTangent(qSharedPointerDynamicCast<VAbstractCurve>(object), position);
        if (QLineF(QPointF(), inward).length() > kDirectionTolerance)
        {
            candidates->append({it.key(), inward});
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The **segments** of the pattern that meet a point, each with the way that runs from the point into it.
 *
 *  A curve is looked for in the whole pattern; a segment was not — only the one the point's **own** tool draws
 *  through it was read, and every other tool's was ignored. Yet the two are one question: what does the outline look
 *  like where this point stands. Leaving it half answered is what refused a pleat whose edge is a hem, drawn as two
 *  segments by the tools of **its** two ends and not by the tool of the point the user clicked. */
void CandidatesFromSegments(const QPointF &position, const QPointF &side, const VContainer *data,
                           QVector<OEdgeCarrier> *candidates)
{
    const auto *objects = data->CalculationGObjects();
    if (objects == nullptr)
    {
        return;
    }

    // Kept apart and sorted before being handed over: the pattern is a hash, and a hash has no order of its own —
    // two segments meeting at a point would be read in a different order from one parse to the next.
    QVector<QPair<qreal, QPointF>> found;

    QSet<quint32> read;
    for (auto it = objects->constBegin(); it != objects->constEnd(); ++it)
    {
        const QSharedPointer<VGObject> &object = it.value();
        if (qSharedPointerDynamicCast<VPointF>(object).isNull())
        {
            continue; // only a point knows the tool that drew it, and only that tool knows its segments
        }

        const quint32 sourceId = (object->getIdObject() != NULL_ID) ? object->getIdObject() : it.key();
        if (read.contains(sourceId))
        {
            continue; // one tool answers once, whichever of its points is read
        }
        read.insert(sourceId);

        auto *tool = qobject_cast<VAbstractTool *>(VAbstractPattern::FindTool(sourceId));
        if (tool == nullptr)
        {
            continue;
        }

        for (const VDrawnSegment &segment : tool->DrawnSegments())
        {
            const QPointF first = OEdgeReader::Position(segment.point1, data);
            const QPointF second = OEdgeReader::Position(segment.point2, data);

            QPointF other;
            if (QLineF(first, position).length() <= accuracyPointOnLine)
            {
                other = second;
            }
            else if (QLineF(second, position).length() <= accuracyPointOnLine)
            {
                other = first;
            }
            else
            {
                continue; // that segment does not meet the point
            }

            const QPointF inward = other - position; // into the segment, away from the point
            const qreal length = QLineF(QPointF(), inward).length();
            if (length > kDirectionTolerance)
            {
                found.append({Dot(inward / length, side), inward});
            }
        }
    }

    std::sort(found.begin(), found.end(),
              [](const QPair<qreal, QPointF> &left, const QPair<qreal, QPointF> &right)
              { return left.first > right.first; });

    for (const QPair<qreal, QPointF> &one : std::as_const(found))
    {
        candidates->append({NULL_ID, one.second});
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The candidate that leaves the point the way @a side points, or none.
 *
 *  At a cut both halves end at the point and leave it in opposite directions: the side is the only thing that tells
 *  which of the two the notch is cut into. */
auto ChosenCarrier(const QVector<OEdgeCarrier> &candidates, const QPointF &side) -> OEdgeCarrier
{
    for (const OEdgeCarrier &candidate : candidates)
    {
        if (Dot(candidate.inward, side) > 0.)
        {
            return candidate;
        }
    }

    return {};
}

/** One block of the outline of a piece: a single point, or the samples of a curve. */
struct ContourBlock
{
    QVector<QPointF> points;
    bool sampled{false}; //!< the points are the curve's own samples: the facets between them are real edges
};

//---------------------------------------------------------------------------------------------------------------------
/** @brief Records a facet of the outline as a direction, when the point lies on it.
 *
 *  A facet of zero length carries no direction: the block of a curve that a neighbour eats entirely leaves such a
 *  filler behind, and answering with a null vector would stop the reading for nothing. */
void EmitFacet(const QPointF &from, const QPointF &to, const QPointF &position, QVector<QPointF> *directions)
{
    if (QLineF(from, to).isNull())
    {
        return;
    }

    if (IsPointOnLineSegment(position, from, to, accuracyPointOnLine))
    {
        directions->append(to - from);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The outline of a piece, block by block, exactly as the application draws it.
 *
 *  A point node contributes its point. A curve node contributes its own samples, trimmed by its neighbours the way
 *  the seam allowance trims it — but only when the point could be on that curve: the trimming costs an
 *  intersection with each neighbour, and the point has to lie on the curve to lie on one of its facets. The two
 *  ends are kept either way, because a straight filler joins a block to the next one, and a curve the point is
 *  not on must not offer its chord as an edge. */
auto ContourBlocks(const VPiece &piece, const QPointF &position, const VContainer *data) -> QVector<ContourBlock>
{
    QVector<ContourBlock> blocks;
    const QVector<VPieceNode> path = piece.GetUnitedPath(data);

    for (int i = 0; i < path.size(); ++i)
    {
        const VPieceNode &node = path.at(i);
        if (node.IsExcluded())
        {
            continue;
        }

        ContourBlock block;
        if (node.GetTypeTool() == Tool::NodePoint)
        {
            block.points.append(static_cast<QPointF>(*data->GeometricObject<VPointF>(node.GetId())));
        }
        else
        {
            const QSharedPointer<VAbstractCurve> curve = data->GeometricObject<VAbstractCurve>(node.GetId());
            const VSAPoint begin = VPiecePath::StartSegment(data, path, i);
            const VSAPoint end = VPiecePath::EndSegment(data, path, i);

            if (not curve.isNull() && curve->IsPointOnCurve(position))
            {
                block.points = curve->GetSegmentPoints(begin, end, node.GetReverse(), piece.GetName());
                block.sampled = true;
            }
            else
            {
                block.points.append(static_cast<QPointF>(begin));
                block.points.append(static_cast<QPointF>(end));
            }
        }

        if (not block.points.isEmpty())
        {
            blocks.append(block);
        }
    }

    return blocks;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The direction of the contour of a piece at a point: the facet of that contour the point lies on.
 *
 *  The outline is the truth about the edge of a piece: a point may sit on a line a tool drew, or on a curve, or on
 *  the straight filler that joins two nodes — the contour is the same line in all three cases, and it is the only
 *  source that can decide between two edges meeting at a point, which is what a corner is.
 *
 *  A pattern point can be a node of no piece, of one, or of several, and its outline is closed
 *  (VPiece::MainPathPath). Every piece answers, and the answers have to describe the same line: two pieces that
 *  meet there give two edges, and nothing is read. */
auto DirectionFromContour(const QPointF &position, const VContainer *data) -> QPointF
{
    const auto *pieces = data->DataPieces();
    if (pieces == nullptr)
    {
        return {};
    }

    QVector<QPointF> directions;
    for (auto it = pieces->constBegin(); it != pieces->constEnd(); ++it)
    {
        QVector<ContourBlock> blocks;
        try
        {
            blocks = ContourBlocks(it.value(), position, data);
        }
        catch (const VException &) // NOLINT(bugprone-empty-catch)
        {
            continue; // an outline that cannot be read carries no edge: fail closed, the caller asks the user
        }

        for (int i = 0; i < blocks.size(); ++i)
        {
            const ContourBlock &block = blocks.at(i);
            if (block.sampled)
            {
                for (int k = 1; k < block.points.size(); ++k)
                {
                    EmitFacet(block.points.at(k - 1), block.points.at(k), position, &directions);
                }
            }

            // the filler that joins this block to the next one — and, after the last, the closing filler
            const ContourBlock &next = blocks.at((i + 1) % blocks.size());
            EmitFacet(block.points.constLast(), next.points.constFirst(), position, &directions);
        }
    }

    return AgreedDirection(directions);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OEdgeReader::Direction(quint32 pointId, const VContainer *data) -> QPointF
{
    if (data == nullptr || not data->HasObject(pointId))
    {
        return {};
    }

    try
    {
        const QSharedPointer<VGObject> object = data->GetGObject(pointId);
        const QSharedPointer<VPointF> point = qSharedPointerDynamicCast<VPointF>(object);
        if (point.isNull())
        {
            return {}; // only a point of the pattern sits on an edge
        }

        // A point an operation produced, or a node copy of a piece, is not the object whose tool drew it: the
        // object it copies is. Every id a tool writes — the point, the tool itself, the lines it registers — is
        // that one, so it is the id the sources below have to look for. For a point drawn by a tool of its own,
        // the two are the same and nothing changes.
        const quint32 sourceId = (object->getIdObject() != NULL_ID) ? object->getIdObject() : pointId;
        const QPointF position = static_cast<QPointF>(*point);

        // 1. the contour of the piece: the outline is the truth about the edge, and the only source that can
        //    decide between two edges meeting at the point
        if (const QPointF direction = DirectionFromContour(position, data); not direction.isNull())
        {
            return direction;
        }

        // 2. the segment the point's own tool draws through it
        if (const QPointF direction = DirectionFromSegments(sourceId, data); not direction.isNull())
        {
            return direction;
        }

        // 3. the curves that tool produced at that point: a cut makes two of its parent, both ending there
        if (const QPointF direction = DirectionFromCurves(position, data, sourceId); not direction.isNull())
        {
            return direction;
        }

        // 4. and, last, any curve of the pattern that ends there
        return DirectionFromCurves(position, data);
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        // A point whose edge cannot be read is a point the user designates by hand — never an exception thrown
        // into a parse, which would refuse the whole pattern for a cap the pleat can do without.
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OEdgeReader::Direction(quint32 pointId, const QPointF &side, const VContainer *data) -> QPointF
{
    if (data == nullptr || not data->HasObject(pointId) || side.isNull())
    {
        return Direction(pointId, data);
    }

    try
    {
        const QSharedPointer<VGObject> object = data->GetGObject(pointId);
        const QSharedPointer<VPointF> point = qSharedPointerDynamicCast<VPointF>(object);
        if (point.isNull())
        {
            return Direction(pointId, data);
        }

        const QPointF position = static_cast<QPointF>(*point);

        // The outline the point stands on, and the branch that leaves it the way the fabric goes.
        QVector<OEdgeCarrier> candidates;
        CandidatesFromCurves(position, data, NULL_ID, &candidates);
        CandidatesFromSegments(position, side, data, &candidates);

        if (const OEdgeCarrier chosen = ChosenCarrier(candidates, side); not chosen.inward.isNull())
        {
            return chosen.inward;
        }
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        // fall through to the plain reading, which knows how to answer nothing
    }

    return Direction(pointId, data);
}

//---------------------------------------------------------------------------------------------------------------------
auto OEdgeReader::Carrier(quint32 pointId, const QPointF &side, const VContainer *data) -> OEdgeCarrier
{
    if (data == nullptr || not data->HasObject(pointId) || side.isNull())
    {
        return {};
    }

    try
    {
        const QSharedPointer<VGObject> object = data->GetGObject(pointId);
        const QSharedPointer<VPointF> point = qSharedPointerDynamicCast<VPointF>(object);
        if (point.isNull())
        {
            return {};
        }

        const quint32 sourceId = (object->getIdObject() != NULL_ID) ? object->getIdObject() : pointId;
        const QPointF position = static_cast<QPointF>(*point);

        // The contour of the piece, first, exactly as Direction reads it: it is the truth about the edge, and a
        // curve that merely passes through the point must not be taken for the one the fold follows.
        const QPointF contour = DirectionFromContour(position, data);

        QVector<OEdgeCarrier> candidates;
        CandidatesFromCurves(position, data, sourceId, &candidates);
        if (candidates.isEmpty())
        {
            CandidatesFromCurves(position, data, NULL_ID, &candidates);
        }

        if (not contour.isNull())
        {
            // Only the curves that carry the very edge the contour describes: the others cross it or belong to
            // another construction, and cutting one of them would mark a piece the fold never reaches.
            QVector<OEdgeCarrier> carried;
            for (const OEdgeCarrier &candidate : candidates)
            {
                if (SameLine(candidate.inward, contour))
                {
                    carried.append(candidate);
                }
            }
            candidates = carried;        }

        if (const OEdgeCarrier chosen = ChosenCarrier(candidates, side); chosen.objectId != NULL_ID)
        {
            return chosen;
        }

        // No curve carries that edge: what is left is a line — the contour of the piece, the straight filler that
        // joins two of its nodes, or the segment the point's own tool draws through it. Only the way matters then,
        // and the side is what fixes it.
        const QPointF line = contour.isNull() ? DirectionFromSegments(sourceId, data) : contour;
        const qreal length = QLineF(QPointF(), line).length();
        if (length <= kDirectionTolerance)
        {
            return {};
        }

        const QPointF unit = line / length;
        return {NULL_ID, (Dot(unit, side) > 0.) ? unit : -unit};
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        // A point whose edge cannot be read is a point the user designates by hand — never an exception thrown
        // into a parse, which would refuse the whole pattern for a notch the pleat can do without.
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OEdgeReader::Position(quint32 pointId, const VContainer *data) -> QPointF
{
    if (data == nullptr || not data->HasObject(pointId))
    {
        return {};
    }

    const QSharedPointer<VPointF> point = qSharedPointerDynamicCast<VPointF>(data->GetGObject(pointId));
    return point.isNull() ? QPointF() : static_cast<QPointF>(*point);
}

//---------------------------------------------------------------------------------------------------------------------
auto OEdgeReader::SegmentsAt(quint32 pointId, const VContainer *data) -> QVector<QPointF>
{
    QVector<QPointF> directions;

    if (data == nullptr || not data->HasObject(pointId))
    {
        return directions;
    }

    try
    {
        // The side only orders the candidates the reader collects, and every segment that meets the point is kept:
        // any direction does here, since the caller decides which of the segments is the edge it is after.
        QVector<OEdgeCarrier> candidates;
        CandidatesFromSegments(Position(pointId, data), QPointF(0., -1.), data, &candidates);

        directions.reserve(candidates.size());
        for (const OEdgeCarrier &candidate : candidates)
        {
            directions.append(candidate.inward);
        }
    }
    catch (const VException &) // NOLINT(bugprone-empty-catch)
    {
        // A point whose segments cannot be read is a point the caller will find nothing at — never an exception
        // thrown into a parse.
        directions.clear();
    }

    return directions;
}
