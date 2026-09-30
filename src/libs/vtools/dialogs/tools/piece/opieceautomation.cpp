/************************************************************************
 **
 **  @file   opieceautomation.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Shared curve sense automation for the piece tool.
 **          Migrated from DialogSeamAllowance (dialogseamallowance_automation.cpp)
 **          so the legacy edit dialog and the new creation dialog share the
 **          exact same behavior without duplication.
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

#include "ocurvesense.h"

#include "opieceautomation.h"

#include "../../../tools/drawTools/toolcurve/vtoolabstractcurve.h"
#include "../../../tools/drawTools/toolpoint/vabstractpoint.h"
#include "../../../tools/vdatatool.h"
#include "../../../../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/typedef.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecepath.h"

#include <QDebug>
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
OPieceAutomation::OPieceAutomation(const VContainer *data, QObject *parent)
  : QObject(parent),
    m_data(data)
{
    SCASSERT(m_data != nullptr)
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceAutomation::GetCurveEndPoints(quint32 curveId, QPointF &start, QPointF &end) const -> bool
{
    return OCurveSense::CurveEndPoints(m_data, curveId, start, end);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceAutomation::PointIsOnCurve(quint32 pointId, quint32 curveId) const -> bool
{
    try
    {
        const QSharedPointer<VPointF> point = m_data->GeometricObject<VPointF>(pointId);
        const QSharedPointer<VGObject> obj = m_data->GetGObject(curveId);
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
auto OPieceAutomation::CurveEntryPoint(quint32 curveId, QPointF &entry) const -> bool
{
    if (m_nodes.isEmpty())
    {
        return false;
    }

    return OCurveSense::EntryFromLast(m_data, m_nodes.last(), curveId, entry);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceAutomation::ComputeReverseFromEntry(const QPointF &entry, quint32 curveId, bool &reverse) const -> bool
{
    return OCurveSense::ReverseFromEntry(entry, m_data, curveId, reverse);
}

//---------------------------------------------------------------------------------------------------------------------
// The sense of a curve whose ENTRY is an interior point is determined by its
// EXIT (the next selected point): exit == curve start → reversed traversal,
// exit == curve end → intrinsic traversal.
auto OPieceAutomation::ComputeReverseFromExit(quint32 exitPointId, quint32 curveId, bool &reverse) const -> bool
{
    QPointF start, end;
    if (not GetCurveEndPoints(curveId, start, end))
    {
        return false;
    }
    QPointF exit;
    try
    {
        exit = m_data->GeometricObject<VPointF>(exitPointId)->toQPointF();
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
auto OPieceAutomation::CurvePointParam(quint32 curveId, quint32 pointId) const -> qreal
{
    try
    {
        const QSharedPointer<VAbstractCurve> curve = m_data->GeometricObject<VAbstractCurve>(curveId);
        const QPointF p = m_data->GeometricObject<VPointF>(pointId)->toQPointF();
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
auto OPieceAutomation::LastCurveReverse(quint32 curveId, bool &reverse) const -> bool
{
    for (int i = static_cast<int>(m_nodes.size()) - 1; i >= 0; --i)
    {
        const VPieceNode node = m_nodes.at(i);
        if (IsCurveNode(node) && node.GetId() == curveId)
        {
            reverse = node.GetReverse();
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::AddCurveNode(quint32 id, Tool nodeTool)
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
                const VPieceNode prev = m_nodes.isEmpty() ? VPieceNode() : m_nodes.last();
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

    AppendNode(VPieceNode(id, nodeTool, reverse));
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::AutoExtendCurve(quint32 pointId)
{
    // Need the last point plus the curve node before it to detect a continuation.
    if (m_nodes.size() < 2)
    {
        return;
    }

    const VPieceNode last = m_nodes.last();
    if (last.GetTypeTool() != Tool::NodePoint)
    {
        return;
    }

    const VPieceNode prev = m_nodes.at(m_nodes.size() - 2);
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
    AppendNode(prev);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::ApplyReverseToCurveRows(quint32 curveId, bool reverse)
{
    for (auto &node : m_nodes)
    {
        if (IsCurveNode(node) && node.GetId() == curveId && node.GetReverse() != reverse)
        {
            node.SetReverse(reverse);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::ReconcilePendingReverse()
{
    if (m_pendingReverseCurves.isEmpty())
    {
        return;
    }

    // The closure entry needs at least the first curve node plus one more node.
    if (m_nodes.size() < 2)
    {
        return;
    }

    // The contour closes on itself: the last node is the entry of the first node.
    const VPieceNode first = m_nodes.first();
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
        emit ListChanged();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::ResolvePendingReverseFromExit(quint32 exitPointId)
{
    if (m_pendingReverseCurves.isEmpty())
    {
        return;
    }

    if (m_nodes.size() < 3)
    {
        return; // need [entry point, curve, exit point]
    }

    const VPieceNode curve = m_nodes.at(m_nodes.size() - 2);
    if (not IsCurveNode(curve) || not m_pendingReverseCurves.contains(curve.GetId()))
    {
        return;
    }

    // The exit must lie on the curve; the entry is the point before the curve node.
    if (not PointIsOnCurve(exitPointId, curve.GetId()))
    {
        return;
    }
    const VPieceNode entry = m_nodes.at(m_nodes.size() - 3);
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
    emit ListChanged();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceAutomation::CompleteClosingCurve(bool endpointsOnly) -> bool
{
    if (m_nodes.size() < 3)
    {
        return false;
    }

    // The closing edge (last → first) is straight only if both ends are point nodes.
    const VPieceNode last = m_nodes.last();
    const VPieceNode first = m_nodes.first();
    if (last.GetTypeTool() != Tool::NodePoint || first.GetTypeTool() != Tool::NodePoint)
    {
        return false;
    }

    QPointF lastPt;
    try
    {
        lastPt = m_data->GeometricObject<VPointF>(last.GetId())->toQPointF();
    }
    catch (const VExceptionBadId &)
    {
        return false;
    }

    // A curve of the path carrying one closing point completes the loop if it also
    // carries the other: take its remaining portion.
    for (const auto &node : m_nodes)
    {
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

        AppendNode(VPieceNode(node.GetId(), node.GetTypeTool(), reverse));
        emit ListChanged();
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::ShowSenseMessage()
{
    emit ToolTipMessage(tr("Select at least two points of a curve to determine its direction"));
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::HandleObjectChosen(quint32 id, const SceneObject &type)
{
    // A prematurely auto-added closing curve: a new object either cuts it (a cut
    // point on the closing portion — keep it and split it), or continues the
    // contour elsewhere and the closing curve is dropped.
    bool cuttingPremature = false;
    VPieceNode prematureCurve;
    if (m_hasPrematureClosingCurve && not m_nodes.isEmpty() && m_nodes.last().GetTypeTool() != Tool::NodePoint)
    {
        prematureCurve = m_nodes.last();
        cuttingPremature = type == SceneObject::Point && PointIsOnCurve(id, prematureCurve.GetId());
        if (not cuttingPremature)
        {
            m_nodes.removeLast();
            m_hasPrematureClosingCurve = false;
        }
    }

    if (id != LastId())
    {
        switch (type)
        {
            case SceneObject::Arc:
                AddCurveNode(id, Tool::NodeArc);
                break;
            case SceneObject::ElArc:
                AddCurveNode(id, Tool::NodeElArc);
                break;
            case SceneObject::Point:
                AutoExtendCurve(id);
                AppendNode(VPieceNode(id, Tool::NodePoint));
                ResolvePendingReverseFromExit(id);
                if (cuttingPremature)
                {
                    // The point cuts the premature closing curve: split it by
                    // adding a continuation curve node (same traversal sense).
                    bool rev = false;
                    LastCurveReverse(prematureCurve.GetId(), rev);
                    AppendNode(VPieceNode(prematureCurve.GetId(), prematureCurve.GetTypeTool(), rev));
                    emit ListChanged();
                    m_hasPrematureClosingCurve = true;
                }
                else
                {
                    m_hasPrematureClosingCurve = CompleteClosingCurve(true);
                }
                break;
            case SceneObject::Spline:
                AddCurveNode(id, Tool::NodeSpline);
                break;
            case SceneObject::SplinePath:
                AddCurveNode(id, Tool::NodeSplinePath);
                break;
            case SceneObject::Line:
            case SceneObject::Detail:
            case SceneObject::Unknown:
            default:
                qDebug() << "Got wrong scene object. Ignore.";
                break;
        }
    }
    else
    {
        if (m_nodes.size() > 1)
        {
            RemoveNodeById(id);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceAutomation::Nodes() const -> const QVector<VPieceNode> &
{
    return m_nodes;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::SetNodes(const QVector<VPieceNode> &nodes)
{
    m_nodes = nodes;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceAutomation::LastId() const -> quint32
{
    return m_nodes.isEmpty() ? NULL_ID : m_nodes.last().GetId();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceAutomation::NodeCount() const -> int
{
    return static_cast<int>(m_nodes.size());
}

//---------------------------------------------------------------------------------------------------------------------
auto OPieceAutomation::PendingReverseCurves() const -> const QSet<quint32> &
{
    return m_pendingReverseCurves;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::ClearPendingReverse()
{
    m_pendingReverseCurves.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::AppendNode(const VPieceNode &node)
{
    // Same deduplication as DialogTool::NewNodeItem: never twice the same id in a row.
    if (m_nodes.isEmpty() || m_nodes.last().GetId() != node.GetId())
    {
        m_nodes.append(node);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceAutomation::RemoveNodeById(quint32 id)
{
    for (int i = 0; i < m_nodes.size(); ++i)
    {
        if (m_nodes.at(i).GetId() == id)
        {
            m_nodes.removeAt(i);
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DefaultPieceName(const VContainer *data) -> QString
{
    const QList<VPiece> pieces = data->DataPieces()->values();
    QSet<QString> names;

    for (const auto &piece : pieces)
    {
        names.insert(piece.GetName());
    }

    const QString defName = QStringLiteral("Detail");
    QString name = defName;
    int i = 0;

    while (names.contains(name))
    {
        name = defName + QStringLiteral("_%1").arg(++i);
    }
    return name;
}
