/************************************************************************
 **
 **  @file   ohitshapepathitem.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18 October, 2026
 **
 **  @brief  A path item that draws one path and is grabbed on another.
 **
 **          The Graded canvas cuts a contour into one open stretch per
 **          corner so the trace pattern restarts at every corner
 **          (OCornerTrace). A path made only of open stretches has no
 **          surface left — QGraphicsPathItem::shape() then returns the
 **          stroked path alone — so the piece could only be grabbed on
 **          its very outline, never on its whole surface. The closed
 **          contour is kept aside here and added to shape(). It is never
 **          painted: the drawing is exactly path(), as before.
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

#ifndef OHITSHAPEPATHITEM_H
#define OHITSHAPEPATHITEM_H

#include <QGraphicsPathItem>
#include <QPainterPath>

/**
 * @brief A path item whose grabbable area may be wider than the painted path.
 *
 * Single responsibility: hold the surface the item is grabbed on, separately from the path it
 * draws. Nothing about the drawing changes — paint() and path() keep using the path set with
 * QGraphicsPathItem::setPath(); only shape() (hit testing, rubber band) is enlarged.
 */
class OHitShapePathItem : public QGraphicsPathItem
{
    Q_DISABLE_COPY_MOVE(OHitShapePathItem) // NOLINT

public:
    explicit OHitShapePathItem(QGraphicsItem *parent = nullptr);

    /// The surface to grab on. In item coordinates, like path(). Not painted.
    void SetHitShape(const QPainterPath &shape);
    void ClearHitShape();

    auto shape() const -> QPainterPath override;

private:
    QPainterPath m_hitShape{};
};

#endif // OHITSHAPEPATHITEM_H
