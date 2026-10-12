/************************************************************************
 **
 **  @file   opiecebuilder.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/10/2026
 **
 **  @brief  Cutting a piece out of a pattern: what travels with it, and what it will need elsewhere.
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

#ifndef OPIECEBUILDER_H
#define OPIECEBUILDER_H

#include "opiecedocument.h"

#include <QString>

class VAbstractPattern;
class VContainer;

/**
 * @brief What a piece takes with it when it leaves its pattern.
 *
 * The block goes whole — its objects, its **layers**, their colours and their visibility are all inside
 * the `<draw>`, so nothing has to be gathered separately. What does have to be gathered is what the block
 * does *not* carry:
 *
 * - the **increments** its formulas use, and the increments those use in turn, because an increment is a
 *   definition shared by the whole pattern and a piece without it cannot build;
 * - the **values it reads from elsewhere** — a line or a curve of another piece. Those are found by asking
 *   the container what each cited value is built on: a value whose object belongs to the block is the
 *   block's own, and a value that leans on an object outside it belongs to whoever declares that object.
 */
namespace OPieceBuilder
{
/**
 * @brief Fills @p piece with the block @p blockName of @p doc, under the name @p name.
 *
 * @p error says why when the block is not there. The caller owns @p piece and may reuse it afterwards.
 */
auto Build(VAbstractPattern *doc,
           VContainer *data,
           const QString &blockName,
           const QString &name,
           OPieceDocument *piece,
           QString &error) -> bool;
} // namespace OPieceBuilder

#endif // OPIECEBUILDER_H
