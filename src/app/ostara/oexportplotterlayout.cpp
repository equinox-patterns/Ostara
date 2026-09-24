/************************************************************************
 **
 **  @file   oexportplotterlayout.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Laying a plotter run out: the parts packed onto mats, and the
 **          mats laid out to be read.
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

#include "oexportplotterlayout.h"

#include "oexportmatpacking.h"

#include "../vlayout/vlayoutpoint.h"

#include <QPolygonF>
#include <QSize>

//---------------------------------------------------------------------------------------------------------------------
auto OExportPlotterLayout::Run(const QVector<OExportSizeLayer> &layers, const OExportPlotter::Mat &mat)
    -> OExportNestingResult
{
    OExportNestingResult result;

    if (!mat.IsValid() || layers.isEmpty())
    {
        // Rien à poser. `ok` reste faux : un plan qu'on ne peut pas poser doit se refuser, pas se
        // dessiner vide — un fichier sans un objet ne dit rien à personne.
        return result;
    }

    const QVector<OExportPieceGeometry> &pieces = layers.constFirst().pieces;

    // ── Ce qu'il y a à ranger ───────────────────────────────────────────────────────────────────
    //
    // Un morceau se range par son RECTANGLE DE COUPE. `MappedDetailBoundingRect` mesure le contour
    // du morceau — marge de couture et crans compris, puisque c'est ce contour-là qui a été posé sur
    // la pièce — et c'est exactement ce que la machine découpe. Mesurer sur autre chose ferait
    // dépendre le nombre de tapis d'un réglage d'affichage : `ceil` transforme une demi-plume en
    // tapis de plus, et le même patron donnerait deux plans sur deux postes.
    QVector<OExportMatPacking::Item> items;
    items.reserve(pieces.size());
    for (const OExportPieceGeometry &geometry : pieces)
    {
        const QRectF box = geometry.piece.MappedDetailBoundingRect();
        items.append({box.width(), box.height()});
    }

    // ── Le rangement ────────────────────────────────────────────────────────────────────────────
    //
    // Le morceau est l'unité, plus la pièce : c'est ce qui permet à deux pièces de partager un tapis,
    // et c'est le seul levier du nombre de tapis. Un morceau n'est jamais renvoyé à sa pièce — une
    // pièce coupée en trois peut voir ses morceaux se poser sur trois tapis différents, et c'est le
    // NOM qui les réunit au recollage, pas l'endroit où ils ont été rangés.
    const OExportMatPacking::Packing packing = OExportMatPacking::Pack(items, mat);

    if (packing.seats.size() != pieces.size())
    {
        return result;
    }

    const QSize arrangement = OExportMatPacking::Arrangement(packing.mats);

    if (arrangement.isEmpty())
    {
        return result;
    }

    // ── Les tapis sur le canvas ─────────────────────────────────────────────────────────────────
    //
    // Le tapis 0, 1, 2… se pose en LIGNES sur le canvas, en rectangle le plus proche du carré. C'est
    // de l'affichage et rien d'autre : le fichier porte un objet par morceau, et le logiciel du
    // plotter lit le CONTENU, jamais les tapis. Aucun ordre de lecture n'existe donc ici, et l'ordre
    // de remplissage est libre — c'est ce qui permet de prendre le plus compact.
    for (int i = 0; i < pieces.size(); ++i)
    {
        const OExportPieceGeometry &geometry = pieces.at(i);
        const OExportMatPacking::Seat &seat = packing.seats.at(i);
        const QRectF box = geometry.piece.MappedDetailBoundingRect();
        const QRectF tile = OExportMatPacking::TileRect(seat.mat, arrangement, mat.widthPx, mat.heightPx);

        OExportPlacement placement;

        // ⚠️ **Le quart de tour part vers la GAUCHE.** `PieceMatrix` tourne autour de l'ancre du
        // rectangle — `translate(x,y)·rotate(θ)·translate(-origin)` — et Qt tourne dans le sens des
        // aiguilles : l'image du rectangle s'étend alors de toute sa hauteur vers la gauche du point
        // (x, y). Le siège, lui, donne le coin du rectangle OCCUPÉ, donc `x` doit avancer de la
        // hauteur du morceau pour que la forme retombe dessus. Sans ce décalage, un morceau tourné
        // sort de son tapis de toute sa hauteur — et un morceau à cheval sur deux tapis ne se découpe
        // pas.
        placement.x = tile.left() + seat.x + (seat.turned ? box.height() : 0.0);
        placement.y = tile.top() + seat.y;

        // Le tour est dit ici, et pas cuit dans la géométrie : les morceaux sont construits AVANT
        // que quoi que ce soit soit rangé, donc leur orientation ne peut pas être connue là-bas.
        placement.rotation = seat.turned ? 90.0 : 0.0;

        // Le coin du rectangle du morceau, dans son propre repère : c'est lui que la matrice de pose
        // ramène au point (x, y). La convention du mode, reprise telle quelle.
        placement.originX = box.left();
        placement.originY = box.top();
        placement.extentX = box.width();
        placement.extentY = box.height();

        QPolygonF outline;
        for (const VLayoutPoint &point : geometry.piece.GetMappedContourPoints())
        {
            outline.append(point);
        }
        placement.outline = outline;

        result.keys.append(geometry.key);
        result.placements.append(placement);
    }

    // Le canvas fait exactement la grille des tapis, dernière rangée incomplète comprise : c'est lui
    // qui dit combien de tapis le run demande, et un canevas qui arrondirait au rectangle plein
    // annoncerait des tapis que personne ne chargera.
    result.binWidthPx = arrangement.width() * mat.widthPx;
    result.usedHeightPx = arrangement.height() * mat.heightPx;
    result.matCount = packing.mats;
    result.ok = true;

    return result;
}
