/************************************************************************
 **
 **  @file   odartcapedge.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/10/2026
 **
 **  @brief  the line of the pattern a dart's cap prolongs, read at a point of its opening
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

#include "odartcapedge.h"

#include <algorithm>
#include <utility>

#include <QLineF>
#include <QList>
#include <QMap>
#include <QSharedPointer>
#include <QVector>

#include "../ifc/xml/vpatterngraph.h"
#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vmisc/typedef.h"
#include "../../vpatterndb/variables/vlinelength.h"
#include "../../vpatterndb/vcontainer.h"
#include "oedgereader.h"
#include "opleatrule.h"

namespace
{
/** Two directions describe one line when the angle between them is below this: the same edge, whichever way along
 *  it each was read. The segments of a pattern are drawn to a pixel, so the tolerance has only to absorb the noise
 *  of the arithmetic, not a real disagreement. */
constexpr qreal kSameLineSine = 1e-6;

auto Position(const VContainer *data, quint32 id) -> QPointF
{
    return static_cast<QPointF>(*data->GeometricObject<VPointF>(id));
}

auto Normalized(const QPointF &vector) -> QPointF
{
    const qreal length = QLineF(QPointF(), vector).length();
    return (length < kPleatTolerance) ? QPointF() : vector / length;
}

auto SameLine(const QPointF &one, const QPointF &other) -> bool
{
    const QPointF first = Normalized(one);
    const QPointF second = Normalized(other);
    if (first.isNull() || second.isNull())
    {
        return false;
    }

    return qAbs(first.x() * second.y() - first.y() * second.x()) <= kSameLineSine;
}

auto HoldsLine(const QVector<QPointF> &lines, const QPointF &direction) -> bool
{
    return std::any_of(lines.cbegin(), lines.cend(),
                       [&direction](const QPointF &one) { return SameLine(one, direction); });
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto ODartCapEdge::Direction(quint32 pointId, quint32 apexId, quint32 otherPointId, quint32 ownerId,
                             const VContainer *data, const VPatternGraph *graph) -> QPointF
{
    if (data == nullptr || not data->HasObject(pointId)
        || qSharedPointerDynamicCast<VPointF>(data->GetGObject(pointId)).isNull())
    {
        return {};
    }

    const QPointF position = Position(data, pointId);

    // The two lines that can never be the edge: the arm, which joins the point to the point of the dart, and the
    // opening, which joins the two points the dart is taken between.
    const QPointF arm =
        (apexId != NULL_ID && data->HasObject(apexId)) ? Position(data, apexId) - position : QPointF();
    const QPointF opening = (otherPointId != NULL_ID && data->HasObject(otherPointId))
                                ? Position(data, otherPointId) - position
                                : QPointF();

    const auto refused = [&arm, &opening](const QPointF &direction) -> bool
    { return (not arm.isNull() && SameLine(direction, arm))
             || (not opening.isNull() && SameLine(direction, opening)); };

    QVector<QPointF> lines;
    const auto take = [&lines, &refused](const QPointF &direction) -> void
    {
        // One line answers once, whichever of its segments or its halves brought it here.
        if (Normalized(direction).isNull() || refused(direction) || HoldsLine(lines, direction))
        {
            return;
        }

        lines.append(direction);
    };

    // 1. What the container records as a line of the pattern: every `<line>`, every side of a rectangle. Such a
    //    segment names its two ends, which is all this question needs — and it is exactly what the pattern DRAWING a
    //    line means: the tool of a `<line>` carries no drawn segment, which is why the edge of a dart whose mouth is
    //    a `<line>` is invisible to every other reader. The halves a construction registers (an intersection, a point
    //    of contact, a bisector) come through the same table and are dropped just below — they are not edges.
    const QMap<QString, QSharedPointer<VLengthLine>> recorded = data->DataLengthLines();
    for (const QSharedPointer<VLengthLine> &line : recorded)
    {
        if (line.isNull())
        {
            continue;
        }

        const quint32 first = line->GetP1Id();
        const quint32 second = line->GetP2Id();
        const quint32 other = (first == pointId) ? second : (second == pointId ? first : NULL_ID);
        if (other == NULL_ID || other == pointId || not data->HasObject(other))
        {
            continue; // that segment does not hold the point
        }

        // The halves an intersection, a point of contact or a bisector register are NOT edges. Each of those tools
        // records the segments that join ITS OWN point to the points of the lines it was built on — so the tool that
        // owns the segment is one of its two ends. Such a segment says only that a construction runs through the
        // point; it never says the pattern draws an edge there. Without this, P3 — a `lineIntersectAxis` cut on the
        // line P3-P2 — gave the dart a second candidate at P2, the reader refused ("two edges hold the point"), and
        // the cap went flat with its crank on its own cap point.
        const QList<quint32> references = line->GetReferences();
        if (references.contains(first) || references.contains(second))
        {
            continue;
        }

        // The figure of the tool being built is a line like any other — the sides of a cap join its two opening
        // points to its own cap point — and a cap that read its own sides would find two edges where there is one,
        // refuse itself, and flatten the very shape it drew on the parse before.
        if (ownerId != NULL_ID && data->GetGObject(other)->getIdObject() == ownerId)
        {
            continue;
        }

        take(Position(data, other) - position);
    }

    // 2. And what the tools draw: a point on a line declares the segment of its own line, which no line of the
    //    container records. Every tool of the pattern is asked, not only the one that made this point.
    for (const QPointF &direction : OEdgeReader::SegmentsAt(pointId, data))
    {
        take(direction);
    }

    if (lines.size() == 1)
    {
        return lines.first();
    }

    if (lines.size() > 1)
    {
        return {}; // two edges hold the point: nothing here decides, the user has to say which one
    }

    // 3. Nothing passes through the point: what made it may still name the line it was built on. A point built on a
    //    line names the two points of that line — drawn or not, the line is theirs.
    if (graph == nullptr)
    {
        return {};
    }

    const QSharedPointer<VGObject> object = data->GetGObject(pointId);
    const quint32 sourceId = (object->getIdObject() != NULL_ID) ? object->getIdObject() : pointId;

    QVector<quint32> sources;
    for (const vidtype source : graph->GetPredecessors(sourceId))
    {
        if (source != NULL_ID && data->HasObject(source)
            && not qSharedPointerDynamicCast<VPointF>(data->GetGObject(source)).isNull())
        {
            sources.append(source);
        }
    }

    if (sources.size() != 2)
    {
        return {}; // a line needs two points; anything else says nothing about one
    }

    const QPointF first = Position(data, sources.at(0));
    const QPointF second = Position(data, sources.at(1));
    const QPointF direction = second - first;

    // The two sources give A line; it has to be the one the point stands on, or it says nothing about the edge.
    if (Normalized(direction).isNull() || not SameLine(direction, position - first))
    {
        return {};
    }

    return refused(direction) ? QPointF() : direction;
}
