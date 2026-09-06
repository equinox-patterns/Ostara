/************************************************************************
 **
 **  @file   ogradedsvgdecor.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Graded SVG export — reference decor resolution for the "Layout"
 **          layer. Reproduces the live Graded canvas behaviour on a headless
 **          temporary scene: group members' own decor is masked in favour of
 **          the group's (like the classic Details SVG export), and every child
 **          (grainline, fold, info blocks, annotations) is placed at its RANGE
 **          position (overrides / resolved centres), never at the default
 **          Details-mode position.
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

#ifndef OGRADEDSVGDECOR_H
#define OGRADEDSVGDECOR_H

#include "ogradedsvgwriter.h"  // OGradedSvg::LayoutObject

#include <QList>

class QGraphicsItem;
class QGraphicsScene;
class OSizerange;
class VAbstractPattern;
class VContainer;

namespace OGradedDecor
{
/**
 * @brief Build the Layout-layer decor objects of a graded SVG.
 *
 * @param scene   the temporary scene whose item positions are FINAL (bases +
 *                alignment + range offsets already applied by OGraded::ApplyOffsets).
 * @param range   the exported range (content overrides are the source of truth
 *                for the child positions).
 * @param pattern the pattern (pieces + groups).
 * @param doc     the document (needed by the info-block resolver).
 * @param lonePieceIds ordered lone (non-grouped) pieces to decorate.
 * @param groupIds     ordered groups (>= 2 present members) to decorate.
 *
 * The caller owns the returned items/records; no group host is left on @p scene
 * except the grainline path items added there so the writer can map them.
 */
QList<OGradedSvg::LayoutObject> BuildReferenceDecor(QGraphicsScene *scene, const OSizerange &range,
                                                    const VContainer *pattern, VAbstractPattern *doc,
                                                    const QList<quint32> &lonePieceIds,
                                                    const QList<quint32> &groupIds);
}  // namespace OGradedDecor

#endif  // OGRADEDSVGDECOR_H
