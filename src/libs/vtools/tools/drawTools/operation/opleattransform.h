/************************************************************************
 **
 **  @file   opleattransform.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The translation the pleats apply to the objects that travel.
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
 * @brief The translation the pleats apply to the objects that travel.
 *
 *  A pleat takes the run of the ordered selection that lies between the two points of its departure line, so the
 *  answer is read **per object**: every object of the selection has its own displacement, and an object no pleat
 *  takes does not move at all. That is what makes the portions of a cut curve travel apart — each portion is an
 *  object of its own, so each one answers for itself — and what makes a curve that spans two pleats follow the
 *  one that took it, without splitting anything.
 *
 *  Move(length, angle, name) is the shape every pattern object answers to, so the vector is turned into the
 *  couple the geometry expects, with the very convention the rest of the application uses (QLineF, as
 *  VPointF::MovePF does). An object the rule did not move leaves a null vector, which moves nothing but renames
 *  the copy, exactly as any other operation does.
 */
class OPleatTransform
{
public:
    explicit OPleatTransform(const OPleatPlan &plan) : m_plan(plan) {}

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        const QLineF move(QPointF(), m_plan.Move(item.id()));
        return item.Move(move.length(), move.angle(), name);
    }

private:
    /** The rule, run once: the region of each pleat, and what every object of the selection travels. */
    OPleatPlan m_plan;
};

#endif // OPLEATTRANSFORM_H
