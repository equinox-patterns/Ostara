/************************************************************************
 **
 **  @file   opleatend.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/10/2026
 **
 **  @brief  One end of a departure line, as the pleat tool resolved it: where it is, which way the piece's edge
 **          runs there, and what the user asked for at that end.
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

#ifndef OPLEATEND_H
#define OPLEATEND_H

#include <QPointF>
#include <QtGlobal>

#include "../../toolsdef.h"
#include "../vmisc/def.h"

/**
 * @brief One end of the departure line, as the tool resolved it.
 *
 * @a pointId is the point the user gave, and it is the point the edge is read at: a pleat only translates, so a
 * copy of that point carries the same edge and nothing else could be read on one (see OPleatSupport).
 *
 * @a position and @a edgeDirection are in pixels. @a edgeDirection is a vector along the piece's edge at that
 * point, the edge the fold has to follow; an empty vector means the tool found no edge there, and the end is
 * then marked like a parallel edge — the centre alone, no V.
 *
 * @a showCap and @a secondSense are what the user asked for that end: whether the cap is drawn at all, and
 * which of the two edges it follows. They are kept per end, because the two ends are independent.
 *
 * @a showNotch asks for the notch of that end as well: the mark the fitter reads on the piece's edge, a value
 * away from the line the fold is measured on, on the side the sense sends it to (see OPleatNotch). There is no
 * notch where there is no cap: a pleat whose edge is not followed has nothing to mark.
 *
 * @a edgePoint is the point the user designated to carry the edge when the tool could not read one by itself: the
 * line from the end of the departure line to it is then the edge, and it wins over anything the tool would have
 * deduced. It is NULL_ID when the edge is read from the point's own tool — see OPleatSupport — or when it could
 * not be found at all, in which case the caller asks the user for one.
 */
struct OPleatEnd
{
    quint32 pointId{NULL_ID};
    QPointF position{};
    QPointF edgeDirection{};
    quint32 edgePointId{NULL_ID};
    bool showCap{true};
    bool secondSense{false};
    bool showNotch{false};
};

#endif // OPLEATEND_H
