/************************************************************************
 **
 **  @file   obezierextension.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  Prolongement d'une courbe de Bezier cubique au-dela de ses extremites.
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

#ifndef OBEZIEEXTENSION_H
#define OBEZIEEXTENSION_H

#include <QPointF>
#include <QtGlobal>

/**
 * @brief Prolongs a cubic Bezier curve past one of its ends.
 *
 * The prolongation leaves the curve in the direction the curve is travelling in when it gets
 * there, and runs straight for the requested length. The curve itself is not modified.
 */
class OBezierExtension
{
public:
    struct Piece
    {
        QPointF p1{};
        QPointF p2{};
        QPointF p3{};
        QPointF p4{};
    };

    // Piece prolonging the curve past p4, oriented from p4 towards its far end.
    static auto After(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, qreal length)
        -> Piece;

    // Piece prolonging the curve before p1, oriented towards the curve, from its far end to p1.
    static auto Before(const QPointF &p1, const QPointF &p2, const QPointF &p3, const QPointF &p4, qreal length)
        -> Piece;

    // A negative length is measured from the end of the curve instead of its start.
    static auto Normalize(qreal length, qreal curveLength) -> qreal;
};

#endif // OBEZIEEXTENSION_H
