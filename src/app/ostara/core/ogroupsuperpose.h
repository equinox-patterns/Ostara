/************************************************************************
 **
 **  @file   ogroupsuperpose.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28/09/2026
 **
 **  @brief  The stretch two members of a piece group share is made to coincide.
 **
 **          Members of a group are laid on one another, so the stretch they have in common is traced
 **          twice. The two traces do not sit exactly on each other — a seam allowance here, a
 **          millimetre there — and a dashed style then shows two lines instead of one.
 **
 **          Nothing is taken away: every point of a member that already stands on the biggest
 **          member's line (within a millimetre) is brought exactly onto it. The two traces become
 **          the same line, and the dash pattern restarting at every corner of that line (OCornerTrace)
 **          gives them the same dashes. What the drawing shows is what the pattern means.
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

#ifndef OGROUPSUPERPOSE_H
#define OGROUPSUPERPOSE_H

#include <QVector>
#include <QtGlobal>

class VContainer;
class VLayoutPiece;
struct DetailForLayout;

/**
 * @brief The traces a piece group shares, brought onto one line.
 */
namespace OGroupSuperpose
{

/**
 * @brief Bring every member of a group onto the biggest member's line.
 *
 * @p details and @p pieces are parallel: pieces.at(k) is details.at(k).piece, laid out.
 */
void Snap(const QVector<DetailForLayout> &details, QVector<VLayoutPiece> &pieces, const VContainer *data);

} // namespace OGroupSuperpose

#endif // OGROUPSUPERPOSE_H
