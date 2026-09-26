/************************************************************************
 **
 **  @file   orotationtransform.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The transformation of a rotation: every source object is turned around the pivot by the same
 **          angle. It lived inside vtoolrotation.cpp, which was enough while Rotation was the only tool
 **          that turned anything; the tool that closes a dart turns the very same way, the only difference
 **          being where its angle comes from — a field for one, the three points of a dart for the other.
 **          Written once here, so the two cannot turn differently.
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

#ifndef OROTATIONTRANSFORM_H
#define OROTATIONTRANSFORM_H

#include <QPointF>
#include <QString>
#include <QtGlobal>

/**
 * @brief The transformation a rotation applies to one object.
 *
 * Points, arcs, splines and the three curve-with-segments types all answer to Rotate(origin, angle, name) with
 * the same signature, so one template serves every shape and OOperationDestination needs nothing else from a
 * transformation than this Image method.
 *
 * The pivot is in pixels and the angle in degrees, counter clockwise: the convention of VPointF::RotatePF and
 * of the whole application.
 */
class ORotationTransform
{
public:
    ORotationTransform(const QPointF &origin, qreal angle) : m_origin(origin), m_angle(angle) {}

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        return item.Rotate(m_origin, m_angle, name);
    }

private:
    QPointF m_origin;
    qreal m_angle;
};

#endif // OROTATIONTRANSFORM_H
