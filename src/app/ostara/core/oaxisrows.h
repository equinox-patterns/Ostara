/************************************************************************
 **
 **  @file   oaxisrows.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/10/2026
 **
 **  @brief  What the Options dock says about an axis: the direction it gives, and the button that sets it.
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

#ifndef OAXISROWS_H
#define OAXISROWS_H

#include <QString>

#include "../vgeometry/oaxis.h"

/** @brief The direction a tool takes from its axis, said in one line.
 *
 *  The two points are deliberately NOT named: they are visible on the canvas, the row has no use for their
 *  labels, and a long axis made two short labels read like a formula nobody asked for. */
auto AxisSummaryText(const OAxis &axis) -> QString;

/** @brief What the row's button offers to do, which is not the same thing once an axis is in place. */
auto AxisButtonText(const OAxis &axis) -> QString;

#endif // OAXISROWS_H
