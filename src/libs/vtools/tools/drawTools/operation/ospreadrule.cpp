/************************************************************************
 **
 **  @file   ospreadrule.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The rule of a slash-and-spread: which rotation turns each object, and around which pivot.
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

#include "ospreadrule.h"

#include <QSharedPointer>

#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vpatterndb/vcontainer.h"
#include "../ifc/exception/vexceptionbadid.h"

using namespace Qt::Literals::StringLiterals;

const QString OSpreadRule::DefaultAngle = QStringLiteral("0");

//---------------------------------------------------------------------------------------------------------------------
auto OSpreadRule::DefaultRotationName(int index) -> QString
{
    return QStringLiteral("Rotation %1").arg(index + 1);
}

//---------------------------------------------------------------------------------------------------------------------
auto OSpreadRule::IndexOf(const QVector<SourceItem> &source, quint32 id) -> int
{
    for (int i = 0; i < source.size(); ++i)
    {
        if (source.at(i).id == id)
        {
            return i;
        }
    }

    return -1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSpreadRule::Compose(const QVector<SourceItem> &source, const QVector<OSpreadRotation> &rotations,
                          const QVector<qreal> &anglesDeg, const VContainer *data) -> OSpreadPlan
{
    OSpreadPlan plan;
    plan.steps = QVector<QVector<OSpreadStep>>(source.size());
    plan.cuts = QVector<OSpreadCut>(rotations.size());

    // One resolved rotation: its region as positions in the source list, its pivot position (already carried by
    // the earlier rotations), and its angle.
    struct Resolved
    {
        int startIndex{-1};
        int pivotIndex{-1};
        QPointF pivot{};
        qreal angle{0.};
    };

    QVector<Resolved> resolved;
    resolved.reserve(rotations.size());

    for (int k = 0; k < rotations.size(); ++k)
    {
        const OSpreadRotation &rotation = rotations.at(k);

        const int startIndex = IndexOf(source, rotation.startId);
        const int pivotIndex = IndexOf(source, rotation.pivotId);
        if (startIndex < 0 || pivotIndex < 0 || startIndex >= pivotIndex)
        {
            continue; // The pair does not describe a region: nothing to turn.
        }

        // The pivot is carried by the rotations before this one, exactly like the objects of its region are.
        QPointF pivot;
        try
        {
            pivot = data->GeometricObject<VPointF>(rotation.pivotId)->toQPointF();
        }
        catch (const VExceptionBadId &)
        {
            continue; // The pivot is gone: this rotation has no centre left.
        }

        // The same two readings, in one pass over the rotations already resolved: the position the pivot is
        // turned from, and the steps the start point has been through when this rotation begins.
        OSpreadCut cut{.startId = rotation.startId};
        for (const Resolved &earlier : std::as_const(resolved))
        {
            if (earlier.startIndex <= pivotIndex && pivotIndex < earlier.pivotIndex)
            {
                pivot = VPointF::RotatePF(earlier.pivot, pivot, earlier.angle);
            }

            if (earlier.startIndex <= startIndex && startIndex < earlier.pivotIndex)
            {
                cut.carried.append({.pivot = earlier.pivot, .angle = earlier.angle});
            }
        }

        const qreal angle = (k < anglesDeg.size()) ? anglesDeg.at(k) : 0.;
        resolved.append({.startIndex = startIndex, .pivotIndex = pivotIndex, .pivot = pivot, .angle = angle});
        plan.cuts[k] = cut;
    }

    for (int i = 0; i < source.size(); ++i)
    {
        for (const Resolved &rotation : std::as_const(resolved))
        {
            if (rotation.startIndex <= i && i < rotation.pivotIndex)
            {
                plan.steps[i].append({.pivot = rotation.pivot, .angle = rotation.angle});
            }
        }
    }

    return plan;
}
