/************************************************************************
 **
 **  @file   oexportplottersvg.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24/09/2026
 **
 **  @brief  The file a domestic plotter cuts: one object per part, no bac,
 **          no grid, and not one word.
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

#ifndef OEXPORTPLOTTERSVG_H
#define OEXPORTPLOTTERSVG_H

#include <QString>

#include "../core/oexportoutput.h"

class VAbstractPattern;
class VContainer;

/**
 * @brief Le fichier d'un plan pour plotter domestique.
 *
 * Ce que la machine reçoit est un **SVG**, et il a trois exigences, toutes les trois dures :
 *
 *   1. **Un objet par morceau.** Un plotter refuse une forme plus grande que son tapis, donc une
 *      pièce découpée doit lui arriver en autant d'objets que de morceaux — chacun nommé, pour que
 *      son logiciel les liste séparément.
 *   2. **Ni bac, ni grille, ni repères, ni page de garde.** Tout ce que le fichier porte est
 *      découpé : un rectangle de bac ferait découper un rectangle de bac.
 *   3. **Aucun texte.** Le logiciel l'ignore au mieux, il découpe les glyphes au pire.
 *
 * Le chemin PDF a déjà résolu les deux derniers points, et de la seule façon qui tienne : en
 * **parcourant le contenu** au lieu de peindre la scène. *La scène peut porter le bac, le fichier
 * non.* C'est ce chemin-là qui est repris ici.
 */
namespace OExportPlotterSvg
{
    /// @brief Write the plotter plan of @p output to @p path. False when nothing could be written.
    auto Write(const QString &path, const OExportOutput &output, const VContainer *pattern,
               VAbstractPattern *doc) -> bool;
} // namespace OExportPlotterSvg

#endif // OEXPORTPLOTTERSVG_H
