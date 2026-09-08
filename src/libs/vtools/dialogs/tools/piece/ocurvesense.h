/************************************************************************
 **
 **  @file   ocurvesense.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   8 September, 2026
 **
 **  @brief  Shared curve sense helpers (widget-free): end points of a curve
 **          object and the auto-reverse decision from the entry point.
 **          Single source of truth used by OPieceAutomation (piece contour)
 **          and DialogPiecePath (internal path).
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

#ifndef OCURVESENSE_H
#define OCURVESENSE_H

#include <QPointF>
#include <QtGlobal>

class VContainer;
class VPieceNode;

/**
 * @brief Curve sense helpers shared by the piece contour and the internal path tools.
 *
 * Reverse semantics (stored on a curve VPieceNode): false = intrinsic start-to-end
 * traversal, true = end-to-start. All functions are read-only and widget-free.
 */
namespace OCurveSense
{
    /**
     * @brief Returns the stored start and end points of a curve object.
     * @return True for Arc/EllipticalArc/Spline/SplinePath/CubicBezier/CubicBezierPath.
     */
    bool CurveEndPoints(const VContainer *data, quint32 curveId, QPointF &start, QPointF &end);

    /**
     * @brief Decides the traversal sense of a curve entered at @p entry.
     * @return True when the sense is determined (entry == stored end → reversed).
     */
    bool ReverseFromEntry(const QPointF &entry, const VContainer *data, quint32 curveId, bool &reverse);

    /**
     * @brief Entry point of a curve given the previous node of the path.
     * @return False when undecidable (no shared endpoint / interior point).
     */
    bool EntryFromLast(const VContainer *data, const VPieceNode &last, quint32 curveId, QPointF &entry);
} // namespace OCurveSense

#endif // OCURVESENSE_H
