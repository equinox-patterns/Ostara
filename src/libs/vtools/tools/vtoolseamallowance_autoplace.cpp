/************************************************************************
 **
 **  @file   vtoolseamallowance_autoplace.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27 August, 2026
 **
 **  @brief  Auto-placement for the InsertNode tool.
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

#include "vtoolseamallowance_autoplace.h"

#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vlayout/vsapoint.h"
#include "../vmisc/def.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecepath.h"
#include "nodeDetails/vabstractnode.h"

#include <cmath>

namespace
{

//---------------------------------------------------------------------------------------------------------------------
inline auto IsCurveNode(const VPieceNode &node) -> bool
{
    switch (node.GetTypeTool())
    {
        case Tool::NodeArc:
        case Tool::NodeElArc:
        case Tool::NodeSpline:
        case Tool::NodeSplinePath:
            return true;
        default:
            return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
inline auto IsPointOrCurve(GOType type) -> bool
{
    switch (type)
    {
        case GOType::Point:
        case GOType::Arc:
        case GOType::EllipticalArc:
        case GOType::Spline:
        case GOType::SplinePath:
        case GOType::CubicBezier:
        case GOType::CubicBezierPath:
            return true;
        default:
            return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// The original construction object id behind a path node: the node tool was
// created with AddEdge(objectId, toolId), so the object is a graph predecessor.
auto NodeObjectId(const VPieceNode &pathNode, VContainer *data, VAbstractPattern *doc) -> quint32
{
    VPatternGraph const *const graph = doc->PatternGraph();
    if (graph == nullptr)
    {
        return NULL_ID;
    }
    const QVector<vidtype> pre = graph->GetPredecessors(pathNode.GetId());
    for (const quint32 id : pre)
    {
        try
        {
            if (IsPointOrCurve(data->GetGObject(id)->getType()))
            {
                return id;
            }
        }
        catch (const VExceptionBadId &)
        {
        }
    }
    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto PointCoordinates(quint32 objectId, VContainer *data) -> QPointF
{
    try
    {
        return data->GeometricObject<VPointF>(objectId)->toQPointF();
    }
    catch (const VExceptionBadId &)
    {
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
void AddPointObject(VContainer *data, quint32 id, QSet<quint32> &points)
{
    if (id == NULL_ID)
    {
        return;
    }
    try
    {
        if (data->GetGObject(id)->getType() == GOType::Point)
        {
            points.insert(id);
        }
    }
    catch (const VExceptionBadId &)
    {
        // Not a VGObject (e.g. a line has no object) — ignore.
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Index of the path node whose original object id matches, or -1.
auto PathIndexByObjectId(const QVector<VPieceNode> &path, quint32 objectId, VContainer *data,
                         VAbstractPattern *doc) -> int
{
    for (int i = 0; i < path.size(); ++i)
    {
        if (NodeObjectId(path.at(i), data, doc) == objectId)
        {
            return i;
        }
    }
    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
// Index of the path POINT node whose coordinates match, or -1.
auto PathPointIndexByCoords(const QVector<VPieceNode> &path, const QPointF &coords, VContainer *data,
                            VAbstractPattern *doc) -> int
{
    if (coords.isNull())
    {
        return -1;
    }
    for (int i = 0; i < path.size(); ++i)
    {
        const VPieceNode &node = path.at(i);
        if (node.GetTypeTool() != Tool::NodePoint)
        {
            continue;
        }
        if (VFuzzyComparePoints(PointCoordinates(NodeObjectId(node, data, doc), data), coords))
        {
            return i;
        }
    }
    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
// The insertion index right after the path point `pointObjectId` if it is
// followed by a straight edge (a point node). Cyclic: if the point is the
// last node and the first is a point, append is valid. Returns -1 otherwise.
auto StraightSuccessorIndex(const QVector<VPieceNode> &path, quint32 pointObjectId, VContainer *data,
                            VAbstractPattern *doc) -> int
{
    const int i = PathIndexByObjectId(path, pointObjectId, data, doc);
    if (i < 0)
    {
        return -1;
    }
    const int next = (i + 1) % path.size();
    if (path.at(next).GetTypeTool() == Tool::NodePoint)
    {
        return i + 1; // == path.size() means append (cyclic, before the first point)
    }
    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
// P's position along the segment A→B as a parameter t: 0 = at A, 1 = at B,
// < 0 beyond A, > 1 beyond B. Only meaningful when P is on the segment's line.
auto SegmentParametric(const QPointF &a, const QPointF &b, const QPointF &p) -> qreal
{
    const QPointF ab = b - a;
    const qreal len2 = QPointF::dotProduct(ab, ab);
    if (qFuzzyIsNull(len2))
    {
        return 0.0;
    }
    return QPointF::dotProduct(p - a, ab) / len2;
}

//---------------------------------------------------------------------------------------------------------------------
// Whether P is (within tolerance) on the straight line through A and B.
auto IsCollinearWith(const QPointF &a, const QPointF &b, const QPointF &p) -> bool
{
    const QPointF ab = b - a;
    const qreal len = std::sqrt(QPointF::dotProduct(ab, ab));
    if (qFuzzyIsNull(len))
    {
        return false;
    }
    const QPointF ap = p - a;
    const qreal cross = ab.x() * ap.y() - ab.y() * ap.x();
    return qAbs(cross) / len <= 1e-3;
}

//---------------------------------------------------------------------------------------------------------------------
// The curve start/end coordinates for a curve object id.
auto CurveEndPoints(quint32 curveObjectId, VContainer *data, QPointF &start, QPointF &end) -> bool
{
    try
    {
        const QSharedPointer<VGObject> obj = data->GetGObject(curveObjectId);
        switch (obj->getType())
        {
            case GOType::Arc:
            {
                const auto arc = qSharedPointerCast<VArc>(obj);
                start = arc->GetP1();
                end = arc->GetP2();
                return true;
            }
            case GOType::EllipticalArc:
            {
                const auto elArc = qSharedPointerCast<VEllipticalArc>(obj);
                start = elArc->GetP1();
                end = elArc->GetP2();
                return true;
            }
            case GOType::Spline:
            {
                const auto spline = qSharedPointerCast<VSpline>(obj);
                start = spline->GetP1().toQPointF();
                end = spline->GetP4().toQPointF();
                return true;
            }
            case GOType::SplinePath:
            {
                const auto splinePath = qSharedPointerCast<VSplinePath>(obj);
                start = splinePath->FirstPoint().toQPointF();
                end = splinePath->LastPoint().toQPointF();
                return true;
            }
            case GOType::CubicBezier:
            {
                const auto bezier = qSharedPointerCast<VCubicBezier>(obj);
                start = bezier->GetP1().toQPointF();
                end = bezier->GetP4().toQPointF();
                return true;
            }
            case GOType::CubicBezierPath:
            {
                const auto bezierPath = qSharedPointerCast<VCubicBezierPath>(obj);
                start = bezierPath->FirstPoint().toQPointF();
                end = bezierPath->LastPoint().toQPointF();
                return true;
            }
            default:
                return false;
        }
    }
    catch (const VExceptionBadId &)
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Case 1 — the point lies on a curve node of the path: cut point. Split the
// curve segment: insert the point after the curve node + a curve portion.
auto FindCutPointPlacement(const QVector<VPieceNode> &path, quint32 pointObjectId, VContainer *data,
                           VAbstractPattern *doc, VNodePlacement &placement) -> bool
{
    const QPointF p = PointCoordinates(pointObjectId, data);
    if (p.isNull())
    {
        return false;
    }

    for (int i = 0; i < path.size(); ++i)
    {
        const VPieceNode &node = path.at(i);
        if (not IsCurveNode(node))
        {
            continue;
        }

        QSharedPointer<VAbstractCurve> curve;
        try
        {
            curve = data->GeometricObject<VAbstractCurve>(node.GetId());
        }
        catch (const VExceptionBadId &)
        {
            continue;
        }

        const VSAPoint begin = VPiecePath::StartSegment(data, path, i);
        const VSAPoint end = VPiecePath::EndSegment(data, path, i);
        const QVector<QPointF> segment = curve->GetSegmentPoints(begin, end, node.GetReverse());

        if (VAbstractCurve::IsPointOnCurve(segment, p))
        {
            placement.index = i + 1;
            placement.hasCurvePortion = true;
            // The added portion: same curve object (original id), same type, same reverse
            // — a fresh node, not a copy of the path node (which carries SA flags).
            placement.curvePortion = VPieceNode(NodeObjectId(node, data, doc), node.GetTypeTool(), node.GetReverse());
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
// Cases 2-4 — trace the construction graph to find the anchor path point(s).
// The point is placed right after an anchor whose successor is a straight edge.
auto FindGraphPlacement(const QVector<VPieceNode> &path, quint32 pointObjectId, VContainer *data,
                        VAbstractPattern *doc, VNodePlacement &placement) -> bool
{
    VPatternGraph const *const graph = doc->PatternGraph();
    if (graph == nullptr || not graph->HasVertex(pointObjectId))
    {
        return false;
    }

    QSet<quint32> connected;
    const QVector<vidtype> parents = graph->GetPredecessors(pointObjectId);
    const QVector<vidtype> children = graph->GetNeighbors(pointObjectId);

    for (const quint32 id : parents)
    {
        AddPointObject(data, id, connected);
    }
    for (const quint32 id : children)
    {
        // The child id could itself be a point (edge P → point, P is an ancestor).
        AddPointObject(data, id, connected);
        // A tool (line, curve...) referencing P: expand to its point inputs.
        const QVector<vidtype> refs = graph->GetPredecessors(id);
        for (const quint32 ref : refs)
        {
            AddPointObject(data, ref, connected);
        }
    }

    // Keep only the anchors that are path points.
    QList<quint32> anchors;
    for (const quint32 id : connected)
    {
        if (PathIndexByObjectId(path, id, data, doc) >= 0)
        {
            anchors.append(id);
        }
    }

    if (anchors.isEmpty())
    {
        return false;
    }

    // Corner: two anchors joined by a straight edge (one is the other's successor).
    if (anchors.size() >= 2)
    {
        for (const quint32 anchor : anchors)
        {
            const int idx = StraightSuccessorIndex(path, anchor, data, doc);
            if (idx < 0)
            {
                continue;
            }
            const quint32 successorOrig = NodeObjectId(path.at(idx % path.size()), data, doc);
            if (not anchors.contains(successorOrig))
            {
                continue;
            }

            // If the point lies on the segment's line, place it by its position
            // along the line: a point on a line can sit beyond one endpoint.
            const QPointF a = PointCoordinates(anchor, data);
            const QPointF b = PointCoordinates(successorOrig, data);
            const QPointF p = PointCoordinates(pointObjectId, data);
            if (not a.isNull() && not b.isNull() && not p.isNull() && IsCollinearWith(a, b, p))
            {
                const qreal t = SegmentParametric(a, b, p);
                if (t > 1.0)
                {
                    const int bIndex = PathIndexByObjectId(path, successorOrig, data, doc);
                    placement.index = bIndex + 1; // beyond the successor
                    placement.placed = true;
                    return true;
                }
                if (t < 0.0)
                {
                    const int aIndex = PathIndexByObjectId(path, anchor, data, doc);
                    placement.index = aIndex; // before the anchor
                    placement.placed = true;
                    return true;
                }
            }

            // Between the two anchors (real corner or middle of the segment).
            placement.index = idx;
            placement.placed = true;
            return true;
        }
        return false; // 2+ anchors but no straight corner: cannot determine
    }

    // Single anchor: place right after it (if followed by a straight edge).
    const int idx = StraightSuccessorIndex(path, anchors.constFirst(), data, doc);
    if (idx >= 0)
    {
        placement.index = idx;
        placement.placed = true;
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
// Case 3 — a whole curve: place it between its two endpoint path points.
auto FindCurvePlacement(const QVector<VPieceNode> &path, quint32 curveObjectId, VContainer *data,
                        VAbstractPattern *doc, VNodePlacement &placement) -> bool
{
    QPointF start, end;
    if (not CurveEndPoints(curveObjectId, data, start, end))
    {
        return false;
    }

    const int startIdx = PathPointIndexByCoords(path, start, data, doc);
    const int endIdx = PathPointIndexByCoords(path, end, data, doc);
    if (startIdx < 0 || endIdx < 0)
    {
        return false;
    }

    // Both endpoints are path points: the curve goes between them. The contour
    // flows from the point whose successor is the other.
    int insertIdx = -1;
    bool reverse = false;
    if ((startIdx + 1) % path.size() == endIdx)
    {
        insertIdx = startIdx + 1; // contour: start → ... → end
        reverse = false;          // entry == curve start
    }
    else if ((endIdx + 1) % path.size() == startIdx)
    {
        insertIdx = endIdx + 1; // contour: end → ... → start
        reverse = true;         // entry == curve end
    }
    else
    {
        return false; // endpoints not adjacent in the loop
    }

    placement.index = insertIdx;
    placement.placed = true;
    placement.reverse = reverse;
    return true;
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto ComputeNodePlacement(const QVector<VPieceNode> &path, const VPieceNode &node, VContainer *data,
                          VAbstractPattern *doc, VNodePlacement &placement) -> bool
{
    if (path.isEmpty())
    {
        return false;
    }

    if (node.GetTypeTool() == Tool::NodePoint)
    {
        // Case 1: on a curve of the path (cut point).
        if (FindCutPointPlacement(path, node.GetId(), data, doc, placement))
        {
            return true;
        }
        // Cases 2-4: construction graph.
        return FindGraphPlacement(path, node.GetId(), data, doc, placement);
    }

    // A whole curve: between its endpoint path points.
    return FindCurvePlacement(path, node.GetId(), data, doc, placement);
}
