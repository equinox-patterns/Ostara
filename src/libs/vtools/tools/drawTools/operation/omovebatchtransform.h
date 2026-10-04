/************************************************************************
 **
 **  @file   omovebatchtransform.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/10/2026
 **
 **  @brief  The transformation of a move by batch: the composition of the moves that carry one object.
 **
 **          One step is what the Move tool does, once: a translation of a length along an angle, then a rotation
 **          of an angle around a pivot. Unlike a plain move, which applies the very same couple to every source,
 **          a move by batch applies each object the sequence of steps its position in the ordered selection
 **          earns it. The rule resolves that sequence once, into one list of steps per source id; this class only
 **          replays it, so the destination factory can use it exactly as it uses OMoveTransform.
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

#ifndef OMOVEBATCHTRANSFORM_H
#define OMOVEBATCHTRANSFORM_H

#include <QHash>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "omovebatchrule.h"

/**
 * @brief The transformation a move by batch applies to one object.
 *
 * Points, arcs, splines and the three curve-with-segments types all answer to Move(length, angle, name) and
 * Rotate(origin, angle, name) with the same signature, so one template serves every shape, exactly as a move
 * does. The steps are read from a table keyed by source id: each object has its own list of steps, empty when
 * no move carries it, in which case the copy stays in place and only takes the new name.
 */
class OMoveBatchTransform
{
public:
    OMoveBatchTransform(const QVector<SourceItem> &source, const QVector<OMoveBatchMove> &moves,
                        const QVector<qreal> &lengths, const QVector<qreal> &angles,
                        const QVector<qreal> &rotationAngles, const VContainer *data)
    {
        const QVector<QVector<OMoveBatchStep>> steps =
            OMoveBatchRule::Compose(source, moves, lengths, angles, rotationAngles, data);
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
            // No move carries this object: a copy in place, renamed. A move of nothing moves nothing, whatever
            // the pivot.
            return result.Move(0., 0., name);
        }

        const QVector<OMoveBatchStep> &steps = it.value();
        for (int k = 0; k < steps.size(); ++k)
        {
            // Only the last step carries the name: the intermediate positions are never materialized, so their
            // names do not matter, and handing each one a name would stack suffixes on top of each other.
            const QString stepName = (k == steps.size() - 1) ? name : QString();
            result = result.Move(steps.at(k).length, steps.at(k).angle, QString())
                         .Rotate(steps.at(k).pivot, steps.at(k).rotationAngle, stepName);
        }

        return result;
    }

private:
    QHash<quint32, QVector<OMoveBatchStep>> m_stepsById{};
};

#endif // OMOVEBATCHTRANSFORM_H
