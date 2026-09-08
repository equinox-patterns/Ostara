/************************************************************************
 **
 **  @file   opattern_graded.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  <sizeRanges> persistence helpers of the Graded mode.
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

#ifndef OPATTERN_GRADED_H
#define OPATTERN_GRADED_H

class QDomElement;
class VAbstractPattern;
class VContainer;

/// Rewrite the <sizeRanges> root element of the pattern DOM from the ranges
/// stored in @p data (replace any stale copy first). No-op when there is no range.
void AddSizeRanges(VAbstractPattern *doc, QDomElement &rootElement, const VContainer *data);

#endif // OPATTERN_GRADED_H
