/************************************************************************
 **
 **  @file   osmoothpath.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/10/2026
 **
 **  @brief  Draw a contour held as points as the smooth curve those points describe.
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

#ifndef OSMOOTHPATH_H
#define OSMOOTHPATH_H

#include <QPointF>
#include <QVector>

class QPainterPath;

namespace OSmoothPath
{
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Path passing exactly through every point, smooth everywhere but at the real corners.
 *
 * Some contours are only ever held as points: the seam line of a piece and its seam allowance are built by
 * walking a node list and cutting the curves into a polyline, and the allowance is a point-set offset of
 * that polyline. Drawn as segments, such a contour is a good drawing of the curve at 100 % and a visibly
 * faceted one as soon as one zooms in, because the segments are cut once and for all in pattern units and
 * nothing ever cuts them again for the current zoom.
 *
 * Handing the same points over as a cubic chain, cut by the paint engine at the fineness of the screen,
 * gives a line that stays smooth at any zoom. The chain goes through every point, so nothing the model
 * decided — a corner, a dart point, a prong — moves; only the small kinks the cutting itself introduced
 * between the points disappear. A knot the contour really turns at (see kCornerAngle in the
 * implementation) is left as a corner: it is where two runs of the chain meet, and the two runs keep their
 * own directions there.
 *
 * A point list and not a curve: this is the last resort for contours whose exact curve is not available
 * anymore. When the curve is known, VAbstractCurve::GetPath() hands over the cubic itself.
 *
 * @param points knots of the contour, in order.
 * @param close  true when the contour comes back to its first point (a piece outline), false when it does
 *               not (a seam allowance line, which the fill closes on its own).
 * @return an empty path when there are fewer than two points.
 */
auto Path(const QVector<QPointF> &points, bool close) -> QPainterPath;
} // namespace OSmoothPath

#endif // OSMOOTHPATH_H
