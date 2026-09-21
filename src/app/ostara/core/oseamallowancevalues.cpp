/************************************************************************
 **
 **  @file   oseamallowancevalues.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/09/2026
 **
 **  @brief  The widths of margin a loaded pattern actually carries.
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

#include "oseamallowancevalues.h"

#include "oseamallowancelibrary.h"

#include "../vmisc/def.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"
#include "xml/vabstractpattern.h"

namespace
{
/**
 * @brief Adds @p width to @p values, which is given in the pattern's own unit.
 *
 * **The conversion happens here and nowhere else**, and it happens on the way in: what the rest of
 * the page works with is millimetres, so a library shared by patterns written in different units
 * still means one thing. A width of zero, or the sentinel a point uses to say it has none of its
 * own, is dropped before it can be counted.
 */
void Keep(QVector<qreal> &values, qreal width, const Unit &unit)
{
    if (width <= 0.0)
    {
        return;
    }

    const qreal mm = OSeamAllowanceLibrary::Rounded(UnitConvertor(width, unit, Unit::Mm));
    if (mm > 0.0 && !values.contains(mm))
    {
        values.append(mm);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OSeamAllowanceValues::Collect(const VAbstractPattern *doc) -> QVector<qreal>
{
    QVector<qreal> values;
    if (doc == nullptr)
    {
        return values;
    }

    // **Kept, not taken by reference.** The container is built on demand and hands out a pointer
    // into itself, so the copy has to outlive the walk — reaching through a temporary would be
    // reading a container that has already gone.
    const VContainer data = doc->GetCompleteData();
    const Unit *unit = data.GetPatternUnit();
    if (unit == nullptr)
    {
        return values;
    }

    const QHash<quint32, VPiece> *pieces = data.DataPieces();
    if (pieces == nullptr)
    {
        return values;
    }

    for (const VPiece &piece : *pieces)
    {
        // A piece cut without a seam allowance has no margin to name, whatever its outline says.
        if (!piece.IsSeamAllowance())
        {
            continue;
        }

        Keep(values, piece.GetSAWidth(), *unit);

        // **The path the engine offsets**, which is the piece's own outline with the groups of
        // custom margin worked into it: reading the raw path would miss a hem written as one.
        const QVector<VPieceNode> path = piece.GetUnitedPath(&data);
        for (const VPieceNode &node : path)
        {
            // A point that overrides nothing answers with the sentinel, and the piece's own width
            // has already been read — so only what is genuinely local is kept.
            Keep(values, node.GetSABefore(&data), *unit);
            Keep(values, node.GetSAAfter(&data), *unit);
        }
    }

    return values;
}
