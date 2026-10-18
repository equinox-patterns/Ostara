/************************************************************************
 **
 **  @file   ohitshapepathitem.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18 October, 2026
 **
 **  @brief  A path item that draws one path and is grabbed on another.
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

#include "ohitshapepathitem.h"

OHitShapePathItem::OHitShapePathItem(QGraphicsItem *parent)
    : QGraphicsPathItem(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void OHitShapePathItem::SetHitShape(const QPainterPath &shape)
{
    m_hitShape = shape;
}

//---------------------------------------------------------------------------------------------------------------------
void OHitShapePathItem::ClearHitShape()
{
    m_hitShape = QPainterPath();
}

//---------------------------------------------------------------------------------------------------------------------
auto OHitShapePathItem::shape() const -> QPainterPath
{
    QPainterPath shape = QGraphicsPathItem::shape();
    if (!m_hitShape.isEmpty())
    {
        // The stroked drawing keeps its own area — a click on the outline must still land, even
        // where the contour stands outside the surface (a notch, a pass mark, a curve overshoot).
        shape.addPath(m_hitShape);
    }
    return shape;
}
