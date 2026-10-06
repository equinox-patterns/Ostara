/************************************************************************
 **
 **  @file   opleatsupport.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The piece's edge at one point of a pleat's prise: the direction the fold has to follow.
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

#include "opleatsupport.h"

#include <QLineF>
#include <QPointF>
#include <QSharedPointer>

#include "../../vgeometry/vpointf.h"
#include "../../vpatterndb/vcontainer.h"
#include "oedgereader.h"

namespace
{
/** Below this length a vector carries no direction at all: a degenerate segment, or two points that the pattern
 *  put on top of each other, answer as little as a point that has no edge. */
constexpr qreal kDirectionTolerance = 1e-7;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OPleatSupport::Resolve(OPleatEnd &end, const VContainer *data)
{
    end.position = OEdgeReader::Position(end.pointId, data);

    if (end.edgePointId != NULL_ID)
    {
        const QPointF designated = OEdgeReader::Position(end.edgePointId, data);
        if (QLineF(QPointF(), designated).length() > kDirectionTolerance)
        {
            end.edgeDirection = designated - end.position;
            return;
        }
    }

    end.edgeDirection = OEdgeReader::Direction(end.pointId, data);
}
