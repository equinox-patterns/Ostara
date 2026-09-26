/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   16 9, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#ifndef VABSTRACTFLIPPING_H
#define VABSTRACTFLIPPING_H

#include <QtGlobal>

#include <QLineF>
#include <QPointF>

#include "../vabstractoperation.h"
#include "../ooperationdestination.h"

/** @brief The transformation of a flip: every source object is mirrored across the axis given by two points.
 *  Points and curves all answer to Flip(axis, name), so one template serves every shape. */
class OFlippingTransform
{
public:
    OFlippingTransform(const QPointF &first, const QPointF &second) : m_axis(first, second) {}

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        return item.Flip(m_axis, name);
    }

private:
    QLineF m_axis;
};

class VAbstractFlipping : public VAbstractOperation
{
    Q_OBJECT // NOLINT
public:
    ~VAbstractFlipping() override = default;

protected:
    explicit VAbstractFlipping(const VAbstractOperationInitData &initData, QGraphicsItem *parent = nullptr);

    static void CreateDestination(VAbstractOperationInitData &initData, const QPointF &fPoint, const QPointF &sPoint);

    /** @brief The images of a flip, across the axis its two points describe. */
    static auto MakeDestination(const QPointF &fPoint, const QPointF &sPoint)
        -> OOperationDestination<OFlippingTransform>;

private:
    Q_DISABLE_COPY_MOVE(VAbstractFlipping) // NOLINT
};

#endif // VABSTRACTFLIPPING_H
