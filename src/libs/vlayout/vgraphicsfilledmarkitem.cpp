/************************************************************************
 **
 **  @file   vgraphicsfilledmarkitem.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/11/2026
 **
 **  @brief  Un repère dont l'encre remplit la forme — le cercle plein.
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

#include "vgraphicsfilledmarkitem.h"

#include "../vmisc/vabstractapplication.h"

//---------------------------------------------------------------------------------------------------------------------
VGraphicsFilledMarkItem::VGraphicsFilledMarkItem(QGraphicsItem *parent)
  : QGraphicsPathItem(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
VGraphicsFilledMarkItem::VGraphicsFilledMarkItem(const QPainterPath &path, QGraphicsItem *parent)
  : QGraphicsPathItem(path, parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void VGraphicsFilledMarkItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    QPainterStateGuard const guard(painter);

    // The item's own pen, not whatever the scene left in the painter: QGraphicsPathItem::paint sets it itself,
    // and a subclass that skips that step paints with the scene's default pen instead of its own.
    const QPen ink = pen();
    painter->setPen(ink);
    painter->setBrush(ink.color());
    painter->drawPath(path());
}
