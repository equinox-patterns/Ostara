/************************************************************************
 **
 **  @file   opleatsupport.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The piece's edge at one point of a pleat's prise: the direction the fold has to follow.
 **
 **          The cap of a pleat is the V that keeps the piece's edge continuous across the fold. Its two sides
 **          follow the edge itself where it leaves the prise, so the tool has to know which way the edge runs
 **          there — and the user, who gave nothing but the prise, the objects that travel and a value, must not
 **          be asked for it.
 **
 **          Nothing new is needed to answer. Every point of a pattern was drawn by a tool, and that tool knows
 **          the lines it draws:
 **
 **          - a point that lies on a line — an alongLine, an endLine, a shoulder point, a height — sits on the
 **            one segment its tool draws towards its base point, and that segment *is* the edge;
 **          - a point cut out of a curve (cutSpline, cutArc) sits at the end of one of the two sub-curves the
 **            cut produced: the tangent there is the edge, and for a spline it is read from the handle, which is
 **            the same value the Angle1Curve_ / Angle2Curve_ variables of the pattern carry;
 **          - anything else — a corner, the crossing of two lines, a copy an operation made without drawing
 **            anything through it — carries no edge of its own. The answer is then empty, and the caller asks
 **            the user to designate one point, whose line with the prise point gives the direction (see
 **            Pleat.md, "Ce que l'outil déduit").
 **
 **          Only a *direction* is ever read, never an orientation: the fold cares about the line, and a vector
 **          and its opposite describe the same edge. Whoever reads the answer must therefore expect no sign.
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

#ifndef OPLEATSUPPORT_H
#define OPLEATSUPPORT_H

#include <QPointF>
#include <QtGlobal>

#include "../../toolsdef.h"
#include "opleatrule.h"

class VContainer;

/**
 * @brief The piece's edge where it leaves the prise.
 *
 * No state, and no pattern: the question is asked of the objects a container already holds, and the answer is a
 * plain vector in pixels.
 */
class OPleatSupport
{
public:
    /** @brief A vector along the edge of the piece at @a pointId, or a null vector when that point carries no
     *  edge the tool can read.
     *
     *  The answer is never normalized, and its orientation carries no meaning: the caller reads a line. A null
     *  answer is the signal that the user has to designate a point instead — it is not a failure of the tool. */
    static auto EdgeDirection(quint32 pointId, const VContainer *data) -> QPointF;

    /** @brief Where one end of the prise is, and which way the piece's edge runs there.
     *
     *  A point the user designated wins over anything the tool would read: it is the way out of an edge no tool can
     *  describe — a corner, a crossing, a copy nothing draws through. With no designated point the edge is read from
     *  the point's own tool, and an empty direction means the user has to designate one. */
    static void Resolve(OPleatEnd &end, const VContainer *data);

private:
    Q_DISABLE_COPY_MOVE(OPleatSupport) // NOLINT
    OPleatSupport() = default;
};

#endif // OPLEATSUPPORT_H
