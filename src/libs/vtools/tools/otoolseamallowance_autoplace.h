/************************************************************************
 **
 **  @file   otoolseamallowance_autoplace.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27 August, 2026
 **
 **  @brief  Auto-placement for the InsertNode tool: compute where a
 **          forgotten node goes in the piece MainPath (closed clockwise
 **          loop), with auto-reverse of curves and cut-point splitting.
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

#ifndef OTOOLSEAMALLOWANCE_AUTOPLACE_H
#define OTOOLSEAMALLOWANCE_AUTOPLACE_H

#include <QVector>

#include "../vpatterndb/vpiecenode.h"

class VContainer;
class VAbstractPattern;

/**
 * @brief ONodePlacement — where an inserted node goes in the current path.
 *
 * index is the insertion index in the current node list. hasCurvePortion
 * means a curve portion (same curve object) must also be inserted right
 * after the node — used when a point cuts a curve.
 */
struct ONodePlacement
{
    bool placed{false};
    int index{-1};
    VPieceNode curvePortion;
    bool hasCurvePortion{false};
    bool reverse{false}; // for a whole-curve insertion
};

/**
 * @brief ComputeNodePlacement — determine where `node` (an InsertNode
 * selection, original object id) goes in `path` (the current MainPath,
 * tool ids). Returns false if no placement can be determined (caller
 * should skip the node, never crash).
 */
auto ComputeNodePlacement(const QVector<VPieceNode> &path, const VPieceNode &node, VContainer *data,
                          VAbstractPattern *doc, ONodePlacement &placement) -> bool;

#endif // OTOOLSEAMALLOWANCE_AUTOPLACE_H
