/************************************************************************
 **
 **  @file   opleattransform.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The transformation of a pleat: one translation, the same for every object that travels.
 **
 **          A pleat does not turn anything and does not mirror anything: the fabric beyond the prise is carried
 **          over the fold line, which is a translation by twice the value the pleat shows — four times it for a
 **          double. The direction is the normal of the prise, and the rule resolves it; this class only applies
 **          it, so the destination factory can use it exactly as it uses OMoveTransform.
 **
 **          Every shape answers to Move(length, angle, name) with the same signature, so one template serves a
 **          point, an arc, a spline and the three curve-with-segments types.
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

#ifndef OPLEATTRANSFORM_H
#define OPLEATTRANSFORM_H

#include <QLineF>
#include <QPointF>
#include <QString>

#include "opleatrule.h"

/**
 * @brief The translation a pleat applies to every object that travels.
 *
 * The move is a vector; Move(length, angle, name) is the shape every pattern object answers to, so the vector is
 * turned into the couple the geometry expects, with the very convention the rest of the application uses
 * (QLineF, as VPointF::MovePF does).
 */
class OPleatTransform
{
public:
    OPleatTransform(const OPleatEnd &first, const OPleatEnd &second, qreal value, OPleatKind kind, OPleatSide side)
      : m_move(QPointF(), OPleatRule::Compose(first, second, value, kind, side).move)
    {
    }

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        return item.Move(m_move.length(), m_move.angle(), name);
    }

private:
    /** The move, read as a line from the origin: its length is the fabric consumed, its angle what the objects
     *  of the pattern answer to. A pleat the rule refused leaves a null line, which moves nothing. */
    QLineF m_move;
};

#endif // OPLEATTRANSFORM_H
