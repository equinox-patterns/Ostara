/************************************************************************
 **
 **  @file   oexportcanvas.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — draw the bin and the nested content.
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

#include "oexportcanvas.h"

#include "oexportgeometry.h"
#include "oexportnesting.h"

#include "../vlayout/vgraphicsfillitem.h"
#include "../vwidgets/vmaingraphicsscene.h"

#include <QBrush>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <QTransform>

namespace
{
/// Grainline, fold mark and its label are drawn by GetItem on every size, and only the
/// smallest one may show them — that is the rule the Graded canvas applies to its ghost
/// layers, and the layers of a range are no different. Same three types, same hiding.
void HidePerSizeDecor(QGraphicsItem *item)
{
    if (item == nullptr)
    {
        return;
    }

    const QList<QGraphicsItem *> children = item->childItems();
    for (QGraphicsItem *child : children)
    {
        if (child->type() == VGraphicsFillItem::Type || child->type() == VGraphicsFoldLineItem::Type
            || child->type() == QGraphicsSimpleTextItem::Type)
        {
            child->hide();
        }
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void OExportCanvas::Build(VMainGraphicsScene *scene,
                          const QVector<OExportSizeLayer> &layers,
                          const OExportNestingResult &result)
{
    if (scene == nullptr || !result.ok)
    {
        return;
    }

    scene->clear();

    // The bin: fixed width, height given by the nesting.
    auto *bin = new QGraphicsRectItem(0, 0, result.binWidthPx, result.usedHeightPx);
    bin->setPen(QPen(Qt::black, 1));
    bin->setBrush(QBrush(Qt::white));
    bin->setZValue(-1);
    scene->addItem(bin);

    for (int i = 0; i < result.placements.size() && i < result.keys.size(); ++i)
    {
        const OExportPlacement &placement = result.placements.at(i);
        const QString &key = result.keys.at(i);

        for (int l = 0; l < layers.size(); ++l)
        {
            const OExportSizeLayer &layer = layers.at(l);
            // The layers come in the order of the range's sizes, smallest first: the
            // first one is the reference, the only one allowed to carry per-piece decor.
            const bool isReference = (l == 0);

            for (const OExportPieceGeometry &geometry : layer.pieces)
            {
                if (geometry.key != key)
                {
                    continue;
                }

                // The engine places a footprint by the min corner of its unrotated polygon,
                // so the piece has to be brought back to that origin before being moved —
                // exactly what the nested SVG writes as translate(x, y) rotate(r)
                // translate(-ox, -oy). The piece's alignment shift belongs to that origin,
                // not after the rotation: it is part of where the piece sits in the
                // document, and the engine measured the footprint with it already applied.
                QTransform placementMatrix;
                placementMatrix.translate(placement.x, placement.y);
                placementMatrix.rotate(placement.rotation);
                placementMatrix.translate(-placement.originX + geometry.alignDelta.x(),
                                          -placement.originY + geometry.alignDelta.y());

                // The placement is baked into the piece's own matrix rather than into a
                // transform on the item: an item transform would drag the bounding rect,
                // the selection and the rubber band along with it.
                VLayoutPiece placed = geometry.piece;
                placed.SetMatrix(placementMatrix);

                QGraphicsItem *item = placed.GetItem(false, true, false);
                if (item != nullptr)
                {
                    if (!isReference)
                    {
                        HidePerSizeDecor(item);
                    }
                    scene->addItem(item);
                }
            }
        }
    }
}
