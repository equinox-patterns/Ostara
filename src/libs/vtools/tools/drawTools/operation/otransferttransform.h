/************************************************************************
 **
 **  @file   otransferttransform.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   29/09/2026
 **
 **  @brief  The transformation of a transfer: the translation that carries the merge point of the source
 **          part onto the merge point of the host part. It is written as a length and an angle — the
 **          convention every Move of the geometry already answers to — so that points, arcs, splines and
 **          the three curve-with-segments types are served by one template, exactly as a rotation is.
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

#ifndef OTRANSFERTTRANSFORM_H
#define OTRANSFERTTRANSFORM_H

#include <QLineF>
#include <QPointF>
#include <QString>
#include <QtGlobal>

/**
 * @brief The transformation a transfer applies to one object.
 *
 * The two points are in pixels: @a origin is the merge point in the source part (the point the portion is
 * anchored on), @a target the merge point in the host part (the point it must land on). Translating by that
 * vector is what places the origin exactly on the target, which is why the operation needs no angle of its
 * own: the copies are the sources moved, and the merged point of the copy is the host point itself.
 *
 * A length and an angle rather than a displacement: it is the pair every geometric object already accepts
 * (`Move(length, angle, name)`), and the same convention as OMoveTransform and ORotationTransform.
 */
class OTransfertTransform
{
public:
    OTransfertTransform(const QPointF &origin, const QPointF &target)
      : m_length(QLineF(origin, target).length()),
        m_angle(QLineF(origin, target).angle())
    {
    }

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        return item.Move(m_length, m_angle, name);
    }

private:
    qreal m_length;
    qreal m_angle;
};

#endif // OTRANSFERTTRANSFORM_H
