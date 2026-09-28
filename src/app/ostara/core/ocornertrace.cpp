/************************************************************************
 **
 **  @file   ocornertrace.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28/09/2026
 **
 **  @brief  A dash pattern restarts at every corner of the trace — see the header.
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

#include "ocornertrace.h"

#include "../vgeometry/vgeometrydef.h"

#include <QGraphicsPathItem>
#include <QLineF>
#include <QPointF>
#include <QRectF>
#include <QVector>

#include <algorithm>
#include <limits>

#include <cmath>

namespace
{

/// VGObject::PainterPath closes a sub-path on the very point it started from — these are the same.
constexpr qreal kSamePoint = 1e-6;

//---------------------------------------------------------------------------------------------------------------------
auto Distance(const QPointF &one, const QPointF &other) -> qreal
{
    return std::hypot(one.x() - other.x(), one.y() - other.y());
}

//---------------------------------------------------------------------------------------------------------------------
auto SamePoint(const QPointF &first, const QPointF &second) -> bool
{
    return QLineF(first, second).length() < kSamePoint;
}

//---------------------------------------------------------------------------------------------------------------------
// How sharply the line turns at @p vertex, in degrees. A curve flattened into samples turns by a
// fraction of a degree at each of them; a corner turns by much more.
auto TurnDegrees(const QPointF &before, const QPointF &vertex, const QPointF &after) -> qreal
{
    const QPointF one = vertex - before;
    const QPointF other = after - vertex;
    const qreal lengthOne = std::hypot(one.x(), one.y());
    const qreal lengthOther = std::hypot(other.x(), other.y());
    if (lengthOne < 1e-9 || lengthOther < 1e-9)
    {
        return 0.0; // a doubled point turns nowhere
    }
    const qreal cosine = (one.x() * other.x() + one.y() * other.y()) / (lengthOne * lengthOther);
    return std::acos(std::clamp(cosine, -1.0, 1.0)) * 180.0 / M_PI;
}

//---------------------------------------------------------------------------------------------------------------------
void AppendSubpath(QPainterPath &result, const QVector<QPointF> &points, bool closed)
{
    if (points.isEmpty())
    {
        return;
    }
    result.moveTo(points.constFirst());
    for (qsizetype k = 1; k < points.size(); ++k)
    {
        result.lineTo(points.at(k));
    }
    if (closed && points.size() > 1)
    {
        result.lineTo(points.constFirst());
    }
}


//---------------------------------------------------------------------------------------------------------------------
// Where a closed or open polyline turns: the vertices where the line changes.
auto CornerVertices(const QVector<QPointF> &points, qreal turnDegrees, bool closed) -> QVector<qsizetype>
{
    QVector<qsizetype> corners;
    const qsizetype count = points.size();
    for (qsizetype k = 0; k < count; ++k)
    {
        if (!closed && (k == 0 || k == count - 1))
        {
            continue; // the ends of an open line are ends, not corners
        }
        const qsizetype before = (k == 0) ? count - 1 : k - 1;
        const qsizetype after = (k + 1) % count;
        if (TurnDegrees(points.at(before), points.at(k), points.at(after)) >= turnDegrees)
        {
            corners.append(k);
        }
    }
    return corners;
}

//---------------------------------------------------------------------------------------------------------------------
// The sub-paths of @p path, as the polylines they are drawn as.
auto LinesOf(const QPainterPath &path) -> QVector<QVector<QPointF>>
{
    QVector<QVector<QPointF>> lines;
    const int count = path.elementCount();

    int start = 0;
    while (start < count)
    {
        int end = start + 1;
        while (end < count && !path.elementAt(end).isMoveTo())
        {
            ++end;
        }
        QVector<QPointF> line;
        line.reserve(end - start);
        for (int k = start; k < end; ++k)
        {
            line.append(QPointF(path.elementAt(k).x, path.elementAt(k).y));
        }
        if (line.size() > 1)
        {
            lines.append(line);
        }
        start = end;
    }
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
// How long the polyline is up to each of its points, measured from its first one.
auto LengthsOf(const QVector<QPointF> &points) -> QVector<qreal>
{
    QVector<qreal> lengths;
    lengths.reserve(points.size());
    lengths.append(0.0);
    for (qsizetype k = 0; k + 1 < points.size(); ++k)
    {
        lengths.append(lengths.constLast() + Distance(points.at(k), points.at(k + 1)));
    }
    return lengths;
}

/// One place where a dash pattern starts again: how far it stands from the first point of the line,
/// and the point itself.
struct Cut
{
    qreal along{0};
    QPointF point;
};

//---------------------------------------------------------------------------------------------------------------------
// Where @p point stands on the polyline, and how far it is from it.
auto PlaceOn(const QVector<QPointF> &points, const QVector<qreal> &lengths, const QPointF &point, qreal *distance)
    -> Cut
{
    Cut best;
    *distance = std::numeric_limits<qreal>::max();
    for (qsizetype k = 0; k + 1 < points.size(); ++k)
    {
        const QPointF from = points.at(k);
        const QPointF to = points.at(k + 1);
        const QPointF edge = to - from;
        const qreal length2 = edge.x() * edge.x() + edge.y() * edge.y();
        qreal t = 0;
        if (length2 > 1e-12)
        {
            t = std::clamp(((point.x() - from.x()) * edge.x() + (point.y() - from.y()) * edge.y()) / length2, 0.0,
                           1.0);
        }
        const QPointF candidate = from + t * edge;
        const qreal candidateDistance = Distance(point, candidate);
        if (candidateDistance < *distance)
        {
            *distance = candidateDistance;
            best.along = lengths.at(k) + t * (lengths.at(k + 1) - lengths.at(k));
            best.point = candidate;
        }
    }
    return best;
}

//---------------------------------------------------------------------------------------------------------------------
// The line that runs from @p from to @p to, through the points of the polyline that stand between
// them. A closed line read this way can pass its own first point without being cut there.
auto Stretch(const QVector<QPointF> &points, const QVector<qreal> &lengths, qreal total, bool loop,
             const Cut &from, const Cut &to) -> QVector<QPointF>
{
    QVector<QPointF> edge;
    edge.append(from.point);

    qreal cursor = from.along;
    while (true)
    {
        qreal nearest = to.along;
        const QPointF *candidate = nullptr;
        for (qsizetype k = 0; k < points.size(); ++k)
        {
            for (int pass = 0; pass < 2; ++pass)
            {
                if (pass == 1 && !loop)
                {
                    break;
                }
                const qreal at = lengths.at(k) + (pass == 1 ? total : 0.0);
                if (at > cursor + 1e-9 && at < nearest - 1e-9)
                {
                    nearest = at;
                    candidate = &points.at(k);
                }
            }
        }
        if (candidate == nullptr)
        {
            break;
        }
        edge.append(*candidate);
        cursor = nearest;
    }

    edge.append(to.point);
    return edge;
}

//---------------------------------------------------------------------------------------------------------------------
// The sub-paths of @p points cut at @p cuts: one line per stretch between two cuts, so every stretch
// starts at a cut and its dash pattern leaves from there. A closed line is read from its first cut on,
// which keeps the place its own list happens to start from out of the way — the two members of a group
// do not start their lists in the same place, and a boundary there would put their patterns out of
// step.
auto CutAtBreaks(const QVector<QPointF> &points, const QVector<qreal> &lengths, qreal total, bool loop,
                 QVector<Cut> cuts) -> QVector<QVector<QPointF>>
{
    std::sort(cuts.begin(), cuts.end(), [](const Cut &one, const Cut &other) { return one.along < other.along; });

    QVector<Cut> kept;
    kept.reserve(cuts.size());
    for (const Cut &one : cuts)
    {
        if (kept.isEmpty() || !SamePoint(kept.constLast().point, one.point))
        {
            kept.append(one); // two cuts at the very same place would cut a stretch of no length
        }
    }

    QVector<QVector<QPointF>> edges;
    if (loop)
    {
        if (kept.isEmpty())
        {
            return edges;
        }
        for (qsizetype k = 0; k < kept.size(); ++k)
        {
            Cut to = kept.at((k + 1) % kept.size());
            if (k + 1 == kept.size())
            {
                to.along += total; // the last stretch closes the loop through the first cut
            }
            edges.append(Stretch(points, lengths, total, loop, kept.at(k), to));
        }
        return edges;
    }

    // An open line starts where it starts and ends where it ends: the cuts divide what is between.
    if (kept.isEmpty() || kept.constFirst().along > 1e-9)
    {
        Cut start;
        start.point = points.constFirst();
        kept.prepend(start);
    }
    if (kept.constLast().along < total - 1e-9)
    {
        Cut end;
        end.along = total;
        end.point = points.constLast();
        kept.append(end);
    }
    for (qsizetype k = 0; k + 1 < kept.size(); ++k)
    {
        edges.append(Stretch(points, lengths, total, loop, kept.at(k), kept.at(k + 1)));
    }
    return edges;
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OCornerTrace::CutAtCorners(const QPainterPath &path, qreal turnDegrees) -> std::optional<QPainterPath>
{
    const int count = path.elementCount();
    if (count == 0)
    {
        return std::nullopt;
    }

    QPainterPath result;
    result.setFillRule(path.fillRule());
    bool cut = false;

    int start = 0;
    while (start < count)
    {
        int end = start + 1;
        while (end < count && !path.elementAt(end).isMoveTo())
        {
            ++end;
        }

        QVector<QPointF> points;
        points.reserve(end - start);
        for (int k = start; k < end; ++k)
        {
            points.append(QPointF(path.elementAt(k).x, path.elementAt(k).y));
        }

        const bool closed = points.size() > 2 && SamePoint(points.constFirst(), points.constLast());
        if (closed)
        {
            points.removeLast(); // the closing point is the first one over again
        }
        if (points.size() < 3)
        {
            AppendSubpath(result, points, closed);
            start = end;
            continue;
        }

        // Where the line turns: those are the corners, and a dash pattern will leave from each.
        QVector<qsizetype> corners = CornerVertices(points, turnDegrees, closed);

        if (corners.size() < 2)
        {
            AppendSubpath(result, points, closed);
            start = end;
            continue;
        }

        if (!closed)
        {
            // An open line starts where it starts and ends where it ends: the corners in between cut
            // it, the ends are not corners.
            if (corners.constFirst() != 0)
            {
                corners.prepend(0);
            }
            if (corners.constLast() != points.size() - 1)
            {
                corners.append(points.size() - 1);
            }
        }

        for (qsizetype k = 0; k + 1 < corners.size(); ++k)
        {
            const qsizetype from = corners.at(k);
            const qsizetype to = corners.at(k + 1);
            QVector<QPointF> edge;
            edge.reserve(to - from + 1);
            for (qsizetype at = from; at <= to; ++at)
            {
                edge.append(points.at(at));
            }
            AppendSubpath(result, edge, false);
            cut = true;
        }
        if (closed)
        {
            const qsizetype from = corners.constLast();
            QVector<QPointF> edge;
            for (qsizetype at = from; at < points.size(); ++at)
            {
                edge.append(points.at(at));
            }
            for (qsizetype at = 0; at <= corners.constFirst(); ++at)
            {
                edge.append(points.at(at));
            }
            AppendSubpath(result, edge, false);
            cut = true;
        }

        start = end;
    }

    if (!cut)
    {
        return std::nullopt;
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
void OCornerTrace::RestartAtCorners(QGraphicsPathItem *item, qreal turnDegrees)
{
    if (item == nullptr)
    {
        return;
    }
    if (const auto cut = CutAtCorners(item->path(), turnDegrees); cut.has_value())
    {
        item->setPath(cut.value());
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OCornerTrace::RestartAtSharedStretches(const QVector<QGraphicsPathItem *> &members) -> QVector<QPointF>
{
    QVector<QPointF> restarts;
    if (members.size() < 2)
    {
        return restarts;
    }

    /// How far a point may stand from an outline and still be read as a place of the stretch the two
    /// traces have in common. The members of a group are made to coincide before they are drawn, so
    /// what they share, they share exactly — and a tight reading puts the beginning of a shared
    /// stretch where the traces really leave each other, not at the edge of a wide allowance.
    const qreal onLine = MmToPixel(0.2);
    /// A start found on one member is carried over to the other at the nearest place of ITS outline,
    /// which can stand away from the start by the allowance above: it still has to be taken.
    const qreal cutReach = MmToPixel(1.0);
    /// Two starts nearer than this are the same place: cutting twice there would cut nothing.
    const qreal sameStart = MmToPixel(1.0);
    /// How finely an outline is walked to find where it leaves another one.
    const qreal step = MmToPixel(0.1);

    // A member draws its outline (closed) and its pass marks (short, open). Only the outline can be
    // shared with another member, so only it is looked at and only it is cut.
    struct Member
    {
        QGraphicsPathItem *item{nullptr};
        QVector<QVector<QPointF>> lines;
        QVector<bool> outline;
        QPointF offset;
    };

    QVector<Member> drawn;
    drawn.reserve(members.size());
    for (QGraphicsPathItem *member : members)
    {
        Member one;
        one.item = member;
        if (member != nullptr)
        {
            one.offset = member->pos();
            for (const QVector<QPointF> &line : LinesOf(member->path()))
            {
                QVector<QPointF> points = line;
                const bool closed = points.size() > 3 && SamePoint(points.constFirst(), points.constLast());
                if (closed)
                {
                    points.removeLast(); // the closing point is the first one over again
                }
                one.lines.append(points);
                one.outline.append(closed);
            }
        }
        drawn.append(one);
    }

    // Whether @p point stands on the closed @p line, which the item draws where @p offset says.
    const auto onOutline = [onLine](const QVector<QPointF> &line, const QPointF &offset, const QPointF &point)
    {
        const qsizetype count = line.size();
        for (qsizetype k = 0; k < count; ++k)
        {
            const QPointF from = line.at(k) + offset;
            const QPointF to = line.at((k + 1) % count) + offset;
            const QPointF edge = to - from;
            const qreal length2 = edge.x() * edge.x() + edge.y() * edge.y();
            qreal t = 0;
            if (length2 > 1e-12)
            {
                t = std::clamp(((point.x() - from.x()) * edge.x() + (point.y() - from.y()) * edge.y()) / length2,
                               0.0, 1.0);
            }
            if (Distance(point, from + t * edge) <= onLine)
            {
                return true;
            }
        }
        return false;
    };

    // The outline is walked by length, not by its vertices: a stretch two members have in common does
    // not have to begin at a vertex of either of them, and reading only the vertices put the cut too
    // late — leaving the beginning of the stretch out of step, which is exactly what was seen.
    const auto Walked = [step](const QVector<QPointF> &line) -> QVector<QPointF>
    {
        QVector<QPointF> walked;
        const qsizetype count = line.size();
        for (qsizetype k = 0; k < count; ++k)
        {
            const QPointF from = line.at(k);
            const QPointF to = line.at((k + 1) % count);
            const int pieces = std::max(1, static_cast<int>(std::ceil(Distance(from, to) / step)));
            for (int piece = 0; piece < pieces; ++piece)
            {
                walked.append(from + (static_cast<qreal>(piece) / pieces) * (to - from));
            }
        }
        return walked;
    };

    // Where each member's outlines stand, to turn away at once from a place that is nowhere near.
    QVector<QRectF> reach;
    reach.reserve(drawn.size());
    for (const Member &member : drawn)
    {
        qreal minX = 0;
        qreal minY = 0;
        qreal maxX = 0;
        qreal maxY = 0;
        bool any = false;
        for (qsizetype l = 0; l < member.lines.size(); ++l)
        {
            if (!member.outline.at(l))
            {
                continue;
            }
            for (const QPointF &point : member.lines.at(l))
            {
                const QPointF at = point + member.offset;
                if (!any)
                {
                    minX = maxX = at.x();
                    minY = maxY = at.y();
                    any = true;
                }
                else
                {
                    minX = std::min(minX, at.x());
                    maxX = std::max(maxX, at.x());
                    minY = std::min(minY, at.y());
                    maxY = std::max(maxY, at.y());
                }
            }
        }
        reach.append(QRectF(QPointF(minX - onLine, minY - onLine), QPointF(maxX + onLine, maxY + onLine)));
    }

    const auto remember = [&restarts, sameStart](const QPointF &point)
    {
        for (const QPointF &known : restarts)
        {
            if (Distance(known, point) < sameStart)
            {
                return;
            }
        }
        restarts.append(point);
    };

    // The start of every stretch two members have in common: a place of an outline that stands on
    // another member's outline while the place just before it on the same outline does not.
    for (qsizetype i = 0; i < drawn.size(); ++i)
    {
        for (qsizetype l = 0; l < drawn.at(i).lines.size(); ++l)
        {
            if (!drawn.at(i).outline.at(l))
            {
                continue;
            }
            const QVector<QPointF> walked = Walked(drawn.at(i).lines.at(l));
            const qsizetype count = walked.size();
            if (count < 2)
            {
                continue;
            }

            QVector<bool> shared(count, false);
            for (qsizetype k = 0; k < count; ++k)
            {
                const QPointF at = walked.at(k) + drawn.at(i).offset;
                for (qsizetype j = 0; j < drawn.size() && !shared.at(k); ++j)
                {
                    if (j == i || !reach.at(j).contains(at))
                    {
                        continue;
                    }
                    for (qsizetype m = 0; m < drawn.at(j).lines.size(); ++m)
                    {
                        if (drawn.at(j).outline.at(m) && onOutline(drawn.at(j).lines.at(m), drawn.at(j).offset, at))
                        {
                            shared[k] = true;
                            break;
                        }
                    }
                }
            }

            for (qsizetype k = 0; k < count; ++k)
            {
                if (shared.at(k) && !shared.at((k + count - 1) % count))
                {
                    // Halfway between the last place off the other trace and the first place on it.
                    remember((walked.at(k) + walked.at((k + count - 1) % count)) / 2.0 + drawn.at(i).offset);
                }
            }
        }
    }

    if (restarts.isEmpty())
    {
        return restarts;
    }

    // Each member is cut at the very same places: both patterns then leave from the same points, and
    // the stretch that follows is drawn by both over the same length.
    for (const Member &member : drawn)
    {
        if (member.item == nullptr)
        {
            continue;
        }

        QPainterPath result;
        result.setFillRule(member.item->path().fillRule());
        bool changed = false;

        for (qsizetype l = 0; l < member.lines.size(); ++l)
        {
            const QVector<QPointF> &line = member.lines.at(l);
            const bool loop = member.outline.at(l);
            if (!loop)
            {
                AppendSubpath(result, line, false);
                continue;
            }

            const QVector<qreal> lengths = LengthsOf(line);
            const qreal total = lengths.constLast() + Distance(line.constLast(), line.constFirst());

            QVector<Cut> cuts;
            for (const QPointF &restart : restarts)
            {
                qreal distance = 0;
                const Cut cut = PlaceOn(line, lengths, restart - member.offset, &distance);
                if (distance <= cutReach)
                {
                    cuts.append(cut);
                }
            }
            if (cuts.isEmpty())
            {
                AppendSubpath(result, line, true);
                continue;
            }
            for (const QVector<QPointF> &edge : CutAtBreaks(line, lengths, total, loop, cuts))
            {
                AppendSubpath(result, edge, false);
                changed = true;
            }
        }

        if (changed)
        {
            member.item->setPath(result);
        }
    }

    return restarts;
}
