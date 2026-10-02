/************************************************************************
 **
 **  @file   ospreadtransform.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The transformation of a slash-and-spread: the composition of the rotations that carry one object.
 **
 **          Unlike a rotation, which turns every source around the same pivot, a slash-and-spread turns each
 **          object around a sequence of pivots that depends on where it sits in the ordered selection. The rule
 **          resolves that sequence once, into one list of steps per source id; this class only replays it, so
 **          the destination factory can use it exactly as it uses ORotationTransform.
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

#ifndef OSPREADTRANSFORM_H
#define OSPREADTRANSFORM_H

#include <QHash>
#include <QPointF>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "ospreadrule.h"

/**
 * @brief The transformation a slash-and-spread applies to one object.
 *
 * Points, arcs, splines and the three curve-with-segments types all answer to Rotate(origin, angle, name) with
 * the same signature, so one template serves every shape, exactly as a rotation does. The steps are read from a
 * table keyed by source id: each object has its own list of (pivot, angle), empty when no rotation carries it,
 * in which case the copy stays in place and only takes the new name.
 */
class OSpreadTransform
{
public:
    OSpreadTransform(const QVector<SourceItem> &source, const QVector<OSpreadRotation> &rotations,
                     const QVector<qreal> &anglesDeg, const VContainer *data)
    {
        const QVector<QVector<OSpreadStep>> steps = OSpreadRule::Compose(source, rotations, anglesDeg, data);
        m_stepsById.reserve(source.size());
        for (int i = 0; i < source.size(); ++i)
        {
            m_stepsById.insert(source.at(i).id, steps.at(i));
        }
    }

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        Item result = item;

        // The key is the object's own id in the container (item.id()), not the id of the tool that drew it
        // (getIdObject()): the table is built from the source ids the operation was given.
        const auto it = m_stepsById.constFind(item.id());
        if (it == m_stepsById.cend() || it->isEmpty())
        {
            // No rotation carries this object: a copy in place, renamed. An angle of 0 turns nothing, whatever
            // the pivot.
            return result.Rotate(QPointF(0, 0), 0., name);
        }

        const QVector<OSpreadStep> &steps = it.value();
        for (int k = 0; k < steps.size(); ++k)
        {
            // Only the last step carries the name: the intermediate positions are never materialized, so their
            // names do not matter, and handing each one a name would stack suffixes on top of each other.
            const QString stepName = (k == steps.size() - 1) ? name : QString();
            result = result.Rotate(steps.at(k).pivot, steps.at(k).angle, stepName);
        }

        return result;
    }

private:
    QHash<quint32, QVector<OSpreadStep>> m_stepsById{};
};

#endif // OSPREADTRANSFORM_H
