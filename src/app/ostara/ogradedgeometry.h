/************************************************************************
 **
 **  @file   ogradedgeometry.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Graded SVG export: geometry helpers shared with the live Graded
 **          canvas. `OGraded::ApplyOffsets` is the reusable "positions" pass of
 **          MainWindow::ApplyGradedLayoutOffsets (reset to the frozen base,
 **          per-piece alignment, rigid-group alignment, per-range content
 **          offsets) — used both by the canvas and by the headless export so the
 **          exported SVG stays pixel-faithful to what is displayed.
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

#ifndef OGRADEDGEOMETRY_H
#define OGRADEDGEOMETRY_H

#include <QtGlobal>

class QGraphicsItem;
class VContainer;
class OSizerange;

namespace OGraded
{
/**
 * @brief Align and offset a set of graded items exactly like the live canvas
 *        does (positions pass of ApplyGradedLayoutOffsets).
 *
 * @param items   the graded silhouette roots (each tagged with kGradedPieceIdRole
 *                and kGradedBasePosRole), normally all the items of one range.
 * @param range   the exported range (content dx/dy applied on top).
 * @param pattern the pattern (pieces + groups, alignment source of truth).
 */
void ApplyOffsets(const QList<QGraphicsItem *> &items, const OSizerange &range,
                  const VContainer *pattern);
}  // namespace OGraded

#endif  // OGRADEDGEOMETRY_H
