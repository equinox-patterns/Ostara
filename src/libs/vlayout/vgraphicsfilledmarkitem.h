/************************************************************************
 **
 **  @file   vgraphicsfilledmarkitem.h
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

#ifndef VGRAPHICSFILLEDMARKITEM_H
#define VGRAPHICSFILLEDMARKITEM_H

#include <QGraphicsPathItem>
#include <QPainter>

#include "../vmisc/def.h"

/**
 * @brief A path whose inside is painted with the same ink as its outline.
 *
 * The colour is not stored: it is the PEN's, read at paint time. That is the whole reason this class exists
 * rather than a brush set once — a size style, the ink pass of an export and the theme all recolour the pen
 * of the items they walk, and none of them knows this item exists. A brush frozen at creation would keep the
 * black it was born with while the outline turned red.
 *
 * Same rule as the grainline's arrows (see VGraphicsFillItem), for the same reason; the SVG writer needs to
 * be told too, since it reads brushes rather than painters (see OSvg::WriteTrace).
 */
class VGraphicsFilledMarkItem : public QGraphicsPathItem
{
public:
    explicit VGraphicsFilledMarkItem(QGraphicsItem *parent = nullptr);
    explicit VGraphicsFilledMarkItem(const QPainterPath &path, QGraphicsItem *parent = nullptr);
    ~VGraphicsFilledMarkItem() override = default;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Layout::FilledMarkItem)
    };

private:
    Q_DISABLE_COPY_MOVE(VGraphicsFilledMarkItem) // NOLINT
};

#endif // VGRAPHICSFILLEDMARKITEM_H
