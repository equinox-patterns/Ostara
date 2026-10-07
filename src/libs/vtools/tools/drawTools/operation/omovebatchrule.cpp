/************************************************************************
 **
 **  @file   omovebatchrule.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/10/2026
 **
 **  @brief  The rule of a move by batch: which move carries each object, and around which pivot it turns.
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

#include "omovebatchrule.h"

#include <QSharedPointer>

#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vpatterndb/vcontainer.h"
#include "../ifc/exception/vexceptionbadid.h"

using namespace Qt::Literals::StringLiterals;

const QString OMoveBatchRule::DefaultLength = QStringLiteral("0");
const QString OMoveBatchRule::DefaultAngle = QStringLiteral("0");
const QString OMoveBatchRule::DefaultRotationAngle = QStringLiteral("0");

//---------------------------------------------------------------------------------------------------------------------
auto OMoveBatchRule::DefaultMoveName(int index) -> QString
{
    return QStringLiteral("Move %1").arg(index + 1);
}

//---------------------------------------------------------------------------------------------------------------------
auto OMoveBatchRule::IndexOf(const QVector<SourceItem> &source, quint32 id) -> int
{
    return SourceIndexOf(source, id); // the question is the operations', and its answer is written once
}

//---------------------------------------------------------------------------------------------------------------------
auto OMoveBatchRule::Compose(const QVector<SourceItem> &source, const QVector<OMoveBatchMove> &moves,
                             const QVector<qreal> &lengths, const QVector<qreal> &angles,
                             const QVector<qreal> &rotationAngles, const VContainer *data)
    -> QVector<QVector<OMoveBatchStep>>
{
    QVector<QVector<OMoveBatchStep>> steps(source.size());

    // One resolved move: its region as positions in the source list, its pivot position (already carried by the
    // earlier moves), and the three values it applies.
    struct Resolved
    {
        int startIndex{-1};
        int pivotIndex{-1};
        QPointF pivot{};
        qreal length{0.};
        qreal angle{0.};
        qreal rotationAngle{0.};
    };

    QVector<Resolved> resolved;
    resolved.reserve(moves.size());

    for (int k = 0; k < moves.size(); ++k)
    {
        const OMoveBatchMove &move = moves.at(k);

        const int startIndex = IndexOf(source, move.startId);
        const int pivotIndex = IndexOf(source, move.pivotId);
        if (startIndex < 0 || pivotIndex < 0 || startIndex >= pivotIndex)
        {
            continue; // The pair does not describe a region: nothing to move.
        }

        // The pivot is carried by the moves before this one, exactly like the objects of its region are.
        QPointF pivot;
        try
        {
            pivot = data->GeometricObject<VPointF>(move.pivotId)->toQPointF();
        }
        catch (const VExceptionBadId &)
        {
            continue; // The pivot is gone: this move has no centre left.
        }

        for (const Resolved &earlier : std::as_const(resolved))
        {
            if (earlier.startIndex <= pivotIndex && pivotIndex <= earlier.pivotIndex)
            {
                // The very pair of operations the transform applies to an object, so the pivot lands where the
                // object that carries it lands.
                pivot = VPointF::MovePF(pivot, earlier.length, earlier.angle);
                pivot = VPointF::RotatePF(earlier.pivot, pivot, earlier.rotationAngle);
            }
        }

        resolved.append({.startIndex = startIndex,
                         .pivotIndex = pivotIndex,
                         .pivot = pivot,
                         .length = (k < lengths.size()) ? lengths.at(k) : 0.,
                         .angle = (k < angles.size()) ? angles.at(k) : 0.,
                         .rotationAngle = (k < rotationAngles.size()) ? rotationAngles.at(k) : 0.});
    }

    for (int i = 0; i < source.size(); ++i)
    {
        for (const Resolved &move : std::as_const(resolved))
        {
            if (move.startIndex <= i && i <= move.pivotIndex)
            {
                steps[i].append({.pivot = move.pivot,
                                 .length = move.length,
                                 .angle = move.angle,
                                 .rotationAngle = move.rotationAngle});
            }
        }
    }

    return steps;
}
