/************************************************************************
 **
 **  @file   dialogseamallowance_automation.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27 August, 2026
 **
 **  @brief  Curve sense automation for the piece tool: auto-reverse of
 **          curves (clockwise contour) and cut point auto-extension.
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

#include "dialogseamallowance.h"

#include "../../dialogtoolbox.h"
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
#include "../vmisc/def.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "ui_tabpaths.h"

#include <QLineF>

#include <limits>

namespace
{

//---------------------------------------------------------------------------------------------------------------------
inline auto SamePoint(const QPointF &p1, const QPointF &p2) -> bool
{
    return VFuzzyComparePoints(p1, p2);
}

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

} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::GetCurveEndPoints(quint32 curveId, QPointF &start, QPointF &end) const -> bool
{
    try
    {
        const QSharedPointer<VGObject> obj = data.GetGObject(curveId);

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
auto DialogSeamAllowance::PointIsOnCurve(quint32 pointId, quint32 curveId) const -> bool
{
    try
    {
        const QSharedPointer<VPointF> point = data.GeometricObject<VPointF>(pointId);
        const QSharedPointer<VGObject> obj = data.GetGObject(curveId);
        const QSharedPointer<VAbstractCurve> curve = qSharedPointerDynamicCast<VAbstractCurve>(obj);
        if (curve.isNull())
        {
            return false;
        }
        return curve->IsPointOnCurve(point->toQPointF());
    }
    catch (const VExceptionBadId &)
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::CurveEntryPoint(quint32 curveId, QPointF &entry) const -> bool
{
    QListWidget *const list = uiTabPaths->listWidgetMainPath;
    const int count = list->count();
    if (count == 0)
    {
        return false;
    }

    const VPieceNode last = RowNode(list, count - 1);
    if (last.GetTypeTool() == Tool::NodePoint)
    {
        try
        {
            const QSharedPointer<VPointF> point = data.GeometricObject<VPointF>(last.GetId());
            entry = point->toQPointF();
            return true;
        }
        catch (const VExceptionBadId &)
        {
            return false;
        }
    }

    // The previous node is a curve: the entry is the endpoint shared with the current curve.
    QPointF curStart, curEnd, prevStart, prevEnd;
    if (not GetCurveEndPoints(curveId, curStart, curEnd) || not GetCurveEndPoints(last.GetId(), prevStart, prevEnd))
    {
        return false;
    }

    if (SamePoint(curStart, prevStart) || SamePoint(curStart, prevEnd))
    {
        entry = curStart;
        return true;
    }
    if (SamePoint(curEnd, prevStart) || SamePoint(curEnd, prevEnd))
    {
        entry = curEnd;
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::ComputeReverseFromEntry(const QPointF &entry, quint32 curveId, bool &reverse) const -> bool
{
    QPointF start, end;
    if (not GetCurveEndPoints(curveId, start, end))
    {
        return false;
    }

    if (SamePoint(entry, start))
    {
        reverse = false;
        return true;
    }
    if (SamePoint(entry, end))
    {
        reverse = true;
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
// The sense of a curve whose ENTRY is an interior point is determined by its
// EXIT (the next selected point): exit == curve start → reversed traversal,
// exit == curve end → intrinsic traversal.
auto DialogSeamAllowance::ComputeReverseFromExit(quint32 exitPointId, quint32 curveId, bool &reverse) const -> bool
{
    QPointF start, end;
    if (not GetCurveEndPoints(curveId, start, end))
    {
        return false;
    }
    QPointF exit;
    try
    {
        exit = data.GeometricObject<VPointF>(exitPointId)->toQPointF();
    }
    catch (const VExceptionBadId &)
    {
        return false;
    }
    if (SamePoint(exit, start))
    {
        reverse = true; // traversed toward the curve start
        return true;
    }
    if (SamePoint(exit, end))
    {
        reverse = false; // traversed toward the curve end
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
// The position (0..1) of a point along the curve's sampled points, or -1.
auto DialogSeamAllowance::CurvePointParam(quint32 curveId, quint32 pointId) const -> qreal
{
    try
    {
        const QSharedPointer<VAbstractCurve> curve = data.GeometricObject<VAbstractCurve>(curveId);
        const QPointF p = data.GeometricObject<VPointF>(pointId)->toQPointF();
        const QVector<QPointF> pts = curve->GetPoints();
        if (pts.size() < 2)
        {
            return -1.0;
        }
        int best = -1;
        qreal bestDist = std::numeric_limits<qreal>::max();
        for (int i = 0; i < pts.size(); ++i)
        {
            const qreal d = QLineF(pts.at(i), p).length();
            if (d < bestDist)
            {
                bestDist = d;
                best = i;
            }
        }
        return best >= 0 ? qreal(best) / qreal(pts.size() - 1) : -1.0;
    }
    catch (const VExceptionBadId &)
    {
        return -1.0;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::LastCurveReverse(quint32 curveId, bool &reverse) const -> bool
{
    QListWidget *const list = uiTabPaths->listWidgetMainPath;
    for (int i = list->count() - 1; i >= 0; --i)
    {
        const VPieceNode node = RowNode(list, i);
        if (IsCurveNode(node) && node.GetId() == curveId)
        {
            reverse = node.GetReverse();
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::AddCurveNode(quint32 id, Tool nodeTool)
{
    bool reverse = false;
    QPointF entry;

    if (CurveEntryPoint(id, entry))
    {
        if (not ComputeReverseFromEntry(entry, id, reverse))
        {
            // The entry is not an endpoint: either a continuation of an already selected
            // curve (keep its sense) or a new curve entered at an interior point.
            if (not LastCurveReverse(id, reverse))
            {
                // An interior cut point as entry: a clockwise contour traverses the curve
                // toward its start, so it is reversed by default. Kept pending so the exit
                // point can correct it if the actual sense is different.
                QListWidget *const list = uiTabPaths->listWidgetMainPath;
                const VPieceNode prev = list->count() > 0 ? RowNode(list, list->count() - 1) : VPieceNode();
                reverse = prev.GetTypeTool() == Tool::NodePoint && PointIsOnCurve(prev.GetId(), id);
                m_pendingReverseCurves.insert(id);
                ShowSenseMessage();
            }
        }
    }
    else
    {
        // No point context before the curve: defer the sense resolution to Enter.
        m_pendingReverseCurves.insert(id);
        ShowSenseMessage();
    }

    NewMainPathItem(VPieceNode(id, nodeTool, reverse));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::AutoExtendCurve(quint32 pointId)
{
    QListWidget *const list = uiTabPaths->listWidgetMainPath;
    const int count = list->count();
    // Need the last point plus the curve node before it to detect a continuation.
    if (count < 2)
    {
        return;
    }

    const VPieceNode last = RowNode(list, count - 1);
    if (last.GetTypeTool() != Tool::NodePoint)
    {
        return;
    }

    const VPieceNode prev = RowNode(list, count - 2);
    if (not IsCurveNode(prev))
    {
        return;
    }

    if (not PointIsOnCurve(pointId, prev.GetId()))
    {
        return;
    }

    // The clicked point continues the curve: re-insert the curve node (same id, same
    // reverse) so the contour follows the curve through the cut point.
    NewMainPathItem(prev);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ApplyReverseToCurveRows(quint32 curveId, bool reverse)
{
    QListWidget *const list = uiTabPaths->listWidgetMainPath;
    for (int i = 0; i < list->count(); ++i)
    {
        QListWidgetItem *const item = list->item(i);
        if (item == nullptr)
        {
            continue;
        }

        auto node = qvariant_cast<VPieceNode>(item->data(Qt::UserRole));
        if (IsCurveNode(node) && node.GetId() == curveId && node.GetReverse() != reverse)
        {
            node.SetReverse(reverse);
            item->setData(Qt::UserRole, QVariant::fromValue(node));
            item->setText(GetNodeName(&data, node, true));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ReconcilePendingReverse()
{
    if (m_pendingReverseCurves.isEmpty())
    {
        return;
    }

    QListWidget *const list = uiTabPaths->listWidgetMainPath;
    const int count = list->count();
    // The closure entry needs at least the first curve node plus one more node.
    if (count < 2)
    {
        return;
    }

    // The contour closes on itself: the last node is the entry of the first node.
    const VPieceNode first = RowNode(list, 0);
    if (not IsCurveNode(first) || not m_pendingReverseCurves.contains(first.GetId()))
    {
        return;
    }

    QPointF entry;
    if (not CurveEntryPoint(first.GetId(), entry))
    {
        return;
    }

    bool reverse = false;
    if (ComputeReverseFromEntry(entry, first.GetId(), reverse))
    {
        ApplyReverseToCurveRows(first.GetId(), reverse);
        m_pendingReverseCurves.remove(first.GetId());
        ValidObjects(MainPathIsValid());
        ListChanged();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ResolvePendingReverseFromExit(quint32 exitPointId)
{
    if (m_pendingReverseCurves.isEmpty())
    {
        return;
    }

    QListWidget *const list = uiTabPaths->listWidgetMainPath;
    const int count = list->count();
    if (count < 3)
    {
        return; // need [entry point, curve, exit point]
    }

    const VPieceNode curve = RowNode(list, count - 2);
    if (not IsCurveNode(curve) || not m_pendingReverseCurves.contains(curve.GetId()))
    {
        return;
    }

    // The exit must lie on the curve; the entry is the point before the curve node.
    if (not PointIsOnCurve(exitPointId, curve.GetId()))
    {
        return;
    }
    const VPieceNode entry = RowNode(list, count - 3);
    if (entry.GetTypeTool() != Tool::NodePoint)
    {
        return;
    }

    // The sense is imposed by the contour: reverse when the exit comes before the
    // entry along the curve's intrinsic order (works for interior points too).
    const qreal entryParam = CurvePointParam(curve.GetId(), entry.GetId());
    const qreal exitParam = CurvePointParam(curve.GetId(), exitPointId);
    if (entryParam < 0.0 || exitParam < 0.0)
    {
        return;
    }

    const bool reverse = exitParam < entryParam;
    ApplyReverseToCurveRows(curve.GetId(), reverse);
    m_pendingReverseCurves.remove(curve.GetId());
    ValidObjects(MainPathIsValid());
    ListChanged();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::CompleteClosingCurve(bool endpointsOnly) -> bool
{
    QListWidget *const list = uiTabPaths->listWidgetMainPath;
    const int count = list->count();
    if (count < 3)
    {
        return false;
    }

    // The closing edge (last → first) is straight only if both ends are point nodes.
    const VPieceNode last = RowNode(list, count - 1);
    const VPieceNode first = RowNode(list, 0);
    if (last.GetTypeTool() != Tool::NodePoint || first.GetTypeTool() != Tool::NodePoint)
    {
        return false;
    }

    QPointF lastPt;
    try
    {
        lastPt = data.GeometricObject<VPointF>(last.GetId())->toQPointF();
    }
    catch (const VExceptionBadId &)
    {
        return false;
    }

    // A curve of the path carrying one closing point completes the loop if it also
    // carries the other: take its remaining portion.
    for (int i = 0; i < count; ++i)
    {
        const VPieceNode &node = RowNode(list, i);
        if (not IsCurveNode(node))
        {
            continue;
        }
        if (not PointIsOnCurve(last.GetId(), node.GetId()) || not PointIsOnCurve(first.GetId(), node.GetId()))
        {
            continue;
        }

        // During collection, only an ENDPOINT is a credible premature closing point:
        // an interior point is a cut point and the curve continues through it.
        if (endpointsOnly)
        {
            QPointF start, end;
            if (not GetCurveEndPoints(node.GetId(), start, end))
            {
                continue;
            }
            if (not SamePoint(lastPt, start) && not SamePoint(lastPt, end))
            {
                continue;
            }
        }

        // The closing edge is traversed from the last point toward the first.
        bool reverse = false;
        if (not ComputeReverseFromEntry(lastPt, node.GetId(), reverse))
        {
            if (not ComputeReverseFromExit(first.GetId(), node.GetId(), reverse))
            {
                // Interior closing points: inherit the traversal direction of the
                // curve's already taken portions (the whole curve shares one sense).
                if (not LastCurveReverse(node.GetId(), reverse))
                {
                    continue; // no known sense: keep the straight closing edge
                }
            }
        }

        NewMainPathItem(VPieceNode(node.GetId(), node.GetTypeTool(), reverse));
        ValidObjects(MainPathIsValid());
        ListChanged();
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ShowSenseMessage()
{
    emit ToolTip(tr("Select at least two points of a curve to determine its direction"));
}
