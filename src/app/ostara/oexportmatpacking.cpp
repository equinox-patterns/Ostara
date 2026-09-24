/************************************************************************
 **
 **  @file   oexportmatpacking.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  Fitting the parts of a plotter run onto mats — as few mats
 **          as their boxes allow.
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

#include "oexportmatpacking.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
/// L'indulgence d'une somme qui tombe pile. Sans elle, un morceau dont la largeur vaut EXACTEMENT le
/// tapis ouvrirait une étagère de plus parce qu'un flottant a rendu 1e-13 de trop — le même piège
/// que `OExportPlotter::CellCount` écarte de son côté, à la même tolérance.
constexpr double kTolerance = 1e-6;

/// Une orientation d'un item : ses dimensions, et si c'est le quart de tour.
struct Facing
{
    double widthPx{0.0};
    double heightPx{0.0};
    bool turned{false};
    bool fits{false}; ///< tient dans un tapis, dans cette orientation
};

/// Un item, ses deux orientations, et celle qu'on retient d'abord.
struct Candidate
{
    int index{0};
    Facing chosen{};
    Facing other{};
};

/// L'orientation retenue : la **plus basse** de celles qui tiennent dans un tapis — une étagère
/// basse porte plus d'items. À hauteur égale l'item reste tel quel : un morceau tourné pour rien se
/// relit moins bien contre le modèle, qui n'est pas tourné.
auto Facings(const OExportMatPacking::Item &item, const OExportPlotter::Mat &mat) -> Candidate
{
    Candidate candidate;
    candidate.chosen = {item.widthPx, item.heightPx, false,
                        item.widthPx <= mat.widthPx && item.heightPx <= mat.heightPx};
    candidate.other = {item.heightPx, item.widthPx, true,
                       item.heightPx <= mat.widthPx && item.widthPx <= mat.heightPx};

    if (candidate.other.fits
        && (!candidate.chosen.fits || candidate.other.heightPx < candidate.chosen.heightPx))
    {
        std::swap(candidate.chosen, candidate.other);
    }

    return candidate;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportMatPacking::Pack(const QVector<Item> &items, const OExportPlotter::Mat &mat) -> Packing
{
    Packing packing;

    if (!mat.IsValid() || items.isEmpty())
    {
        return packing;
    }

    QVector<Candidate> order;
    order.reserve(items.size());
    for (int i = 0; i < items.size(); ++i)
    {
        Candidate candidate = Facings(items.at(i), mat);
        candidate.index = i;
        order.append(candidate);
    }

    // Le plus haut d'abord, et c'est ce qui rend l'étagère sûre : elle prend sa hauteur au premier
    // item qu'elle reçoit, et tous les suivants sont plus bas — aucun ne peut donc la dépasser. À
    // hauteur égale, le plus large d'abord, puis le rang : l'ordre est TOTAL, donc reproductible, et
    // un rangement qui bougerait d'un build à l'autre serait un plan qui change sous l'utilisatrice.
    std::stable_sort(order.begin(), order.end(),
                     [](const Candidate &a, const Candidate &b)
                     {
                         if (a.chosen.heightPx > b.chosen.heightPx)
                         {
                             return true;
                         }
                         if (a.chosen.heightPx < b.chosen.heightPx)
                         {
                             return false;
                         }
                         if (a.chosen.widthPx > b.chosen.widthPx)
                         {
                             return true;
                         }
                         if (a.chosen.widthPx < b.chosen.widthPx)
                         {
                             return false;
                         }
                         return a.index < b.index;
                     });

    packing.seats.resize(items.size());

    auto Lay = [&packing](int index, int mat, double x, double y, const Facing &facing)
    {
        Seat seat;
        seat.mat = mat;
        seat.x = x;
        seat.y = y;
        seat.turned = facing.turned;
        packing.seats[index] = seat;
    };

    int matIndex = 0;
    double shelfY = 0.0;      // le haut de l'étagère ouverte
    double shelfHeight = 0.0; // sa hauteur, prise au premier item qu'elle a reçu
    double cursor = 0.0;      // et la largeur déjà prise dessus
    bool shelfOpen = false;

    for (const Candidate &candidate : std::as_const(order))
    {
        const Facing *facing = nullptr;

        if (shelfOpen)
        {
            const double free = mat.widthPx - cursor + kTolerance;

            if (candidate.chosen.widthPx <= free
                && candidate.chosen.heightPx <= shelfHeight + kTolerance)
            {
                facing = &candidate.chosen;
            }
            // L'autre orientation n'est essayée qu'ICI. Sur une étagère ouverte elle peut tenir
            // couchée là où elle ne tenait pas debout ; sur une étagère neuve il n'y a rien à
            // gagner, puisque c'est elle qui donnerait sa hauteur.
            else if (candidate.other.fits && candidate.other.widthPx <= free
                     && candidate.other.heightPx <= shelfHeight + kTolerance)
            {
                facing = &candidate.other;
            }
        }

        if (facing != nullptr)
        {
            Lay(candidate.index, matIndex, cursor, shelfY, *facing);
            cursor += facing->widthPx;
            continue;
        }

        // L'étagère est pleine, ou l'item est plus haut qu'elle : on la ferme et on en ouvre une.
        if (shelfOpen)
        {
            shelfY += shelfHeight;
        }

        shelfHeight = candidate.chosen.heightPx;
        cursor = 0.0;

        // Les étagères ne tiennent plus dans le tapis : c'est un tapis de plus — le seul endroit du
        // rangement où le compte augmente, et donc le seul endroit qui ait à être juste.
        if (shelfOpen && shelfY + shelfHeight > mat.heightPx + kTolerance)
        {
            ++matIndex;
            shelfY = 0.0;
        }

        shelfOpen = true;
        Lay(candidate.index, matIndex, cursor, shelfY, candidate.chosen);
        cursor += candidate.chosen.widthPx;
    }

    packing.mats = matIndex + 1;

    return packing;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMatPacking::Arrangement(int mats) -> QSize
{
    if (mats <= 0)
    {
        return QSize();
    }

    // La racine donne le carré, l'arrondi au-dessus donne la colonne qui le dépasse le moins. Les
    // rangées se comptent ensuite par excès : c'est ce qui fait qu'une dernière rangée incomplète
    // reste possible, et c'est voulu — forcer un rectangle plein dessinerait des tapis en trop.
    const int columns = static_cast<int>(std::ceil(std::sqrt(static_cast<double>(mats))));
    const int rows = (mats + columns - 1) / columns;

    return QSize(columns, rows);
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportMatPacking::TileRect(int index, const QSize &arrangement, double matWidthPx, double matHeightPx)
    -> QRectF
{
    if (index < 0 || arrangement.isEmpty())
    {
        return QRectF();
    }

    const int column = index % arrangement.width();
    const int row = index / arrangement.width();

    return QRectF(column * matWidthPx, row * matHeightPx, matWidthPx, matHeightPx);
}
