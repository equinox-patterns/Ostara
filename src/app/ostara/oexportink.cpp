/************************************************************************
 **
 **  @file   oexportink.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Export mode — l'encre du format Projecteur.
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

#include "oexportink.h"

#include "osvgutils.h"

#include "../vlayout/vgraphicsfillitem.h"

#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QPen>

//---------------------------------------------------------------------------------------------------------------------
void OExportInk::ApplyToSubtree(QGraphicsItem *root)
{
    if (root == nullptr)
    {
        return;
    }

    // ── Le trait, et rien d'autre ────────────────────────────────────────────────────────────────
    //
    // Ce qui n'est pas un chemin — une légende, une image, un cadre — n'a pas d'encre à recevoir :
    // la lui imposer la rendrait invisible ou noire sans raison.
    // ⚠️ **`OSvg::AsPathItem`, jamais `qgraphicsitem_cast` seul.** Le droit-fil et la ligne de pli
    // sont des `VGraphicsFillItem`/`VGraphicsFoldLineItem`, qui redéfinissent `type()` : un cast
    // générique les REFUSE, et c'est précisément sur eux que l'épaisseur compte. Le helper du projet
    // connaît les deux types, et c'est la même lecture que fait l'écrivain SVG.
    if (auto *path = OSvg::AsPathItem(root))
    {
        const bool grainOrFold = root->type() == VGraphicsFillItem::Type
                                 || root->type() == VGraphicsFoldLineItem::Type;

        QPen pen = path->pen();
        pen.setColor(Ink());
        pen.setStyle(Qt::SolidLine);
        // ⚠️ Un motif de tirets se vide en repassant en `SolidLine` ? Non — `setDashPattern` reste
        // en mémoire et `Qt::CustomDashLine` reviendrait avec lui. C'est donc explicitement qu'on
        // le vide, avant de choisir le style.
        pen.setDashPattern({});
        pen.setWidthF(grainOrFold ? kGrainlinePx : kGeometryPx);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        path->setPen(pen);
    }

    const QList<QGraphicsItem *> children = root->childItems();
    for (QGraphicsItem *child : children)
    {
        ApplyToSubtree(child);
    }
}
