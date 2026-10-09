/************************************************************************
 **
 **  @file   ooffsetstations.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  Where a variable-width offset acts: for every width, the point
 **          of the base curve it is measured from and the point it lands on.
 **          The stations live inside the sampling of VSplinePath::Outline(),
 **          so this reads the same arc length tables and the same normal,
 **          and a preview can name the width of a station without rebuilding
 **          the offset engine.
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

#ifndef OOFFSETSTATIONS_H
#define OOFFSETSTATIONS_H

#include <QPointF>
#include <QVector>

class VAbstractCurve;

/**
 * @brief One width of a variable-width offset, placed on the base curve.
 */
struct OOffsetStation
{
    int index{0};     ///< rank of the width in the list handed over, in the direction of the curve
    QPointF base{};   ///< point of the base curve the width is measured from
    QPointF target{}; ///< where the width lands: base + width * the normal of the base there
    qreal width{0.0}; ///< width really applied, signed, in the unit the caller gave
};

namespace OOffsetStations
{
/**
 * @brief Stations of a variable-width offset on @p curve.
 *
 * Station i of n sits at i/(n-1) of the arc length of the base, which is where VSplinePath::Outline() puts it, and
 * lands at its width along the normal of the base at that point. A single width is not a set of stations — the offset
 * is the same along the whole curve — so it is reported by both of its ends.
 *
 * @return an empty vector for a curve no variable width is defined on, or a degenerate one.
 */
auto Stations(const VAbstractCurve &curve, const QVector<qreal> &widths) -> QVector<OOffsetStation>;
} // namespace OOffsetStations

#endif // OOFFSETSTATIONS_H
