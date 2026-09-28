/************************************************************************
 **
 **  @file   ogroupsuperpose.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28/09/2026
 **
 **  @brief  The stretch two members of a piece group share is made to coincide — see the header.
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

#include "ogroupsuperpose.h"

#include "../vgeometry/vgeometrydef.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vlayout/vlayoutpoint.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "mainwindowsnogui.h"

#include <QHash>
#include <QPointF>

#include <cmath>

namespace
{

/// How far a point may stand from the other piece's line and still be brought onto it. Beyond that
/// it is not the same line, and moving it would move the pattern.
const qreal kReach = MmToPixel(1.0);

//---------------------------------------------------------------------------------------------------------------------
auto Distance(const QPointF &one, const QPointF &other) -> qreal
{
    return std::hypot(one.x() - other.x(), one.y() - other.y());
}

//---------------------------------------------------------------------------------------------------------------------
// The point of the segment @p from–@p to that stands nearest @p point, and how far it is.
auto NearestOnSegment(const QPointF &point, const QPointF &from, const QPointF &to, qreal *distance) -> QPointF
{
    const qreal dx = to.x() - from.x();
    const qreal dy = to.y() - from.y();
    const qreal length2 = dx * dx + dy * dy;
    if (length2 < 1e-12)
    {
        *distance = Distance(point, from);
        return from;
    }
    const qreal t = std::clamp(((point.x() - from.x()) * dx + (point.y() - from.y()) * dy) / length2, 0.0, 1.0);
    const QPointF nearest(from.x() + t * dx, from.y() + t * dy);
    *distance = Distance(point, nearest);
    return nearest;
}

//---------------------------------------------------------------------------------------------------------------------
auto Length(const QVector<VLayoutPoint> &points) -> qreal
{
    qreal total = 0;
    for (qsizetype k = 0; k + 1 < points.size(); ++k)
    {
        total += Distance(points.at(k), points.at(k + 1));
    }
    return total;
}

//---------------------------------------------------------------------------------------------------------------------
// The shift that SUPERIMPOSES two outlines: the difference between a point of one and a point of the
// other is the same all along the stretch they share, so it is the value that comes back most often.
auto Superposition(const QVector<VLayoutPoint> &one, const QVector<VLayoutPoint> &other) -> QPointF
{
    const qreal step = MmToPixel(0.25);
    QHash<QString, int> votes;
    QHash<QString, QPointF> sums;

    for (const VLayoutPoint &a : one)
    {
        for (const VLayoutPoint &b : other)
        {
            const QPointF shift = QPointF(a) - QPointF(b);
            const QString key =
                QStringLiteral("%1|%2").arg(qRound(shift.x() / step)).arg(qRound(shift.y() / step));
            votes[key] += 1;
            sums[key] += shift;
        }
    }

    QString best;
    int bestVotes = 0;
    for (auto at = votes.cbegin(); at != votes.cend(); ++at)
    {
        if (at.value() > bestVotes)
        {
            bestVotes = at.value();
            best = at.key();
        }
    }
    return bestVotes == 0 ? QPointF() : sums.value(best) / bestVotes;
}

//---------------------------------------------------------------------------------------------------------------------
// @p points, each one brought onto @p line when it already stands on it. The count of what moved is
// added to @p moved. Nothing is dropped, nothing is added.
auto SnappedOnto(const QVector<VLayoutPoint> &points, const QVector<QPointF> &line, qsizetype *moved)
    -> QVector<VLayoutPoint>
{
    QVector<VLayoutPoint> snapped;
    snapped.reserve(points.size());

    for (const VLayoutPoint &point : points)
    {
        qreal nearest = kReach;
        QPointF onto;
        bool found = false;
        for (qsizetype k = 0; k + 1 < line.size(); ++k)
        {
            qreal distance = 0;
            const QPointF candidate = NearestOnSegment(QPointF(point), line.at(k), line.at(k + 1), &distance);
            if (distance < nearest)
            {
                nearest = distance;
                onto = candidate;
                found = true;
            }
        }

        VLayoutPoint result(point);
        if (found && nearest > 1e-6)
        {
            result = VLayoutPoint(onto);
            result.SetTurnPoint(point.TurnPoint());
            result.SetCurvePoint(point.CurvePoint());
            *moved += 1;
        }
        snapped.append(result);
    }
    return snapped;
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OGroupSuperpose::Snap(const QVector<DetailForLayout> &details, QVector<VLayoutPiece> &pieces,
                           const VContainer *data)
{
    if (data == nullptr || details.isEmpty() || details.size() != pieces.size())
    {
        return;
    }

    const auto *groups = data->DataPieceGroups();
    if (groups == nullptr || groups->isEmpty())
    {
        return;
    }

    QHash<quint32, qsizetype> indexOf;
    for (qsizetype k = 0; k < details.size(); ++k)
    {
        indexOf.insert(details.at(k).id, k);
    }

    for (auto group = groups->cbegin(); group != groups->cend(); ++group)
    {
        QVector<quint32> members;
        for (const quint32 pieceId : group.value().pieceIds)
        {
            if (indexOf.contains(pieceId))
            {
                members.append(pieceId);
            }
        }
        if (members.size() < 2)
        {
            continue;
        }

        // The BIGGEST piece is the one the others are brought onto: a short is cut out of a trouser.
        const auto outlineLength = [&](quint32 pieceId) -> qreal {
            const VLayoutPiece &piece = pieces.at(indexOf.value(pieceId));
            const QVector<VLayoutPoint> seam = piece.GetSeamAllowancePoints();
            return Length(seam.isEmpty() ? piece.GetContourPoints() : seam);
        };
        quint32 carrierId = members.constFirst();
        for (const quint32 pieceId : members)
        {
            if (outlineLength(pieceId) > outlineLength(carrierId))
            {
                carrierId = pieceId;
            }
        }

        const VLayoutPiece &carrier = pieces.at(indexOf.value(carrierId));

        for (const quint32 otherId : members)
        {
            if (otherId == carrierId)
            {
                continue;
            }

            VLayoutPiece &other = pieces[indexOf.value(otherId)];
            qsizetype moved = 0;

            // The outline is the seam allowance when the piece has one — that is what is drawn — and
            // the contour otherwise. Only those two lists are touched: the internal paths a piece
            // carries live elsewhere and cannot be reached from here.
            if (!carrier.GetSeamAllowancePoints().isEmpty() && !other.GetSeamAllowancePoints().isEmpty())
            {
                const QVector<VLayoutPoint> &theirs = carrier.GetSeamAllowancePoints();
                const QPointF shift = Superposition(theirs, other.GetSeamAllowancePoints());
                QVector<QPointF> line;
                line.reserve(theirs.size());
                for (const VLayoutPoint &point : theirs)
                {
                    line.append(QPointF(point) + shift);
                }
                other.SetSeamAllowancePoints(SnappedOnto(other.GetSeamAllowancePoints(), line, &moved),
                                             details.at(indexOf.value(otherId)).piece.IsSeamAllowance());
            }
            if (!carrier.GetContourPoints().isEmpty() && !other.GetContourPoints().isEmpty())
            {
                const QVector<VLayoutPoint> &theirs = carrier.GetContourPoints();
                const QPointF shift = Superposition(theirs, other.GetContourPoints());
                QVector<QPointF> line;
                line.reserve(theirs.size());
                for (const VLayoutPoint &point : theirs)
                {
                    line.append(QPointF(point) + shift);
                }
                other.SetContourPoints(SnappedOnto(other.GetContourPoints(), line, &moved),
                                       details.at(indexOf.value(otherId)).piece.IsHideMainPath());
            }
        }
    }
}
