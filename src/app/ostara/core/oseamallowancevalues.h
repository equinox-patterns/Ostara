/************************************************************************
 **
 **  @file   oseamallowancevalues.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  The widths of margin a loaded pattern actually carries, read
 **          off its pieces.
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

#ifndef OSEAMALLOWANCEVALUES_H
#define OSEAMALLOWANCEVALUES_H

#include <QVector>

class VAbstractPattern;

/**
 * @brief The widths of seam allowance a pattern holds, as a page has to state them.
 *
 * A cover page says which margins a pattern is cut with, and the pattern is the only thing that
 * knows: the widths live on its pieces, and nowhere else in the file is there a list of them.
 *
 * **A margin is measured in two places, and both are read.** A piece carries the width its outline
 * is offset by; a point of that outline may carry a width of its own, which is how a hem running
 * along one edge is written. A pattern is what it is at both levels, so a width found at either is
 * a width the page has to name.
 *
 * **Zero is not a margin, and neither is the absence of one.** A point may be given no seam
 * allowance at all — the outline is offset by nothing there — and the engine writes that as a zero.
 * A page that listed it would be naming a margin nobody sews. A width a point does not override is
 * not a zero either: it says "as the piece is", and the piece has already been read.
 *
 * **What comes back is what the page will print**: millimetres, each width once however many times
 * the pattern uses it, brought to the step comparisons are made on so that two routes to the same
 * length cannot come out as two widths.
 *
 * **And smallest first, because the walk has no order of its own.** Pieces are read from a hash,
 * so the sequence they are met in is not one the pattern owns and not one that survives a run. A
 * page that named them in the order they were found said something different each time it was
 * drawn.
 */
namespace OSeamAllowanceValues
{
/**
 * @brief Every distinct width @p doc is cut with, in millimetres, smallest first.
 *
 * An empty answer is the honest one for a pattern with no seam allowance anywhere, and for no
 * pattern at all: @p doc may be null, which is what a page opened with nothing loaded hands over.
 */
auto Collect(const VAbstractPattern *doc) -> QVector<qreal>;
} // namespace OSeamAllowanceValues

#endif // OSEAMALLOWANCEVALUES_H
