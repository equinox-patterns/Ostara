/************************************************************************
 **
 **  @file   ocurvesense.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   8 September, 2026
 **
 **  @brief  Shared curve sense helpers (see ocurvesense.h). Bodies migrated
 **          from OPieceAutomation (dialogseamallowance_automation lineage)
 **          so the internal path tool decides a curve's sense exactly like
 **          the piece contour, from a single source of truth.
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

#include "../vmisc/def.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../../../../ifc/exception/vexceptionbadid.h"

#include <QSharedPointer>

namespace
{

//---------------------------------------------------------------------------------------------------------------------
inline auto SamePoint(const QPointF &p1, const QPointF &p2) -> bool
{
    return VFuzzyComparePoints(p1, p2);
}

} // namespace

namespace OCurveSense
{

//---------------------------------------------------------------------------------------------------------------------
bool CurveEndPoints(const VContainer *data, quint32 curveId, QPointF &start, QPointF &end)
{
    SCASSERT(data != nullptr)
    try
    {
        const QSharedPointer<VGObject> obj = data->GetGObject(curveId);

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
bool ReverseFromEntry(const QPointF &entry, const VContainer *data, quint32 curveId, bool &reverse)
{
    QPointF start, end;
    if (not CurveEndPoints(data, curveId, start, end))
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
bool EntryFromLast(const VContainer *data, const VPieceNode &last, quint32 curveId, QPointF &entry)
{
    SCASSERT(data != nullptr)

    if (last.GetTypeTool() == Tool::NodePoint)
    {
        try
        {
            const QSharedPointer<VPointF> point = data->GeometricObject<VPointF>(last.GetId());
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
    if (not CurveEndPoints(data, curveId, curStart, curEnd) || not CurveEndPoints(data, last.GetId(), prevStart, prevEnd))
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

} // namespace OCurveSense
