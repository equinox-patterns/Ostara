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
 **          Reading that edge is `OEdgeReader`'s job (the contour of the piece, the lines the pattern records
 **          through the point, the segment the point's own tool draws, the tangent of a curve a cut produced).
 **          What is left here is the way out: a point the user designates by hand wins over anything read, for
 **          the points no source can describe — a corner, a crossing, a copy nothing draws through.
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
 * @brief Where one end of a pleat's prise is, and which way the piece's edge runs there.
 *
 * No state, and no pattern: the question is asked of the objects a container already holds, and the answer is a
 * plain vector in pixels.
 */
class OPleatSupport
{
public:
    /** @brief Where one end of the prise is, and which way the piece's edge runs there.
     *
     *  A point the user designated wins over anything that could be read: it is the way out of an edge no source
     *  can describe — a corner, a crossing, a copy nothing draws through. With no designated point the edge is
     *  read by `OEdgeReader`, and an empty direction means the user has to designate one. */
    static void Resolve(OPleatEnd &end, const VContainer *data);

private:
    Q_DISABLE_COPY_MOVE(OPleatSupport) // NOLINT
    OPleatSupport() = default;
};

#endif // OPLEATSUPPORT_H
