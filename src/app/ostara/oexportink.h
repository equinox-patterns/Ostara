/************************************************************************
 **
 **  @file   oexportink.h
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

#ifndef OEXPORTINK_H
#define OEXPORTINK_H

#include <QColor>
#include <QtGlobal>

class QGraphicsItem;

/**
 * @brief Le tracé du format Projecteur : noir, plein, et trois épaisseurs.
 *
 * **Une seule encre pour toutes les tailles** — décision de Sophie : c'est elle qui rend les calques
 * interchangeables. Aucune couleur, aucun tiret, **aucun style de taille** : les styles de taille de
 * Graded court-circuitent le tracé des pièces partout ailleurs, et le projecteur est le seul format
 * qui n'en veuille pas.
 *
 * | Élément | Épaisseur | Relevé sur le fichier de référence |
 * |---|---|---|
 * | Contour, chemins internes | 7 pt = 2,47 mm | oui |
 * | Droit-fil, ligne de pli | 5,25 pt = 1,85 mm | oui |
 * | Grilles de calibration | 4 pt = 1,41 mm | oui |
 *
 * ⚠️ **L'unité est le millimètre réel, jamais une proportion de la page.** À 100 % avec défilement,
 * 1 mm de page = 1 mm sur le mur : le millimètre par pixel dépend de la **distance de projection**,
 * pas de la taille du document. Une règle proportionnelle serait fondée sur une hypothèse fausse.
 *
 * Les valeurs ci-dessous sont en **pixels du fichier** — l'unité du `viewBox`, qui vaut 96 dpi : le
 * SVG déclare sa taille en millimètres, donc un pixel y est exactement 25,4/96 mm, et 7 pt y pèsent
 * 9,333 px. C'est cette valeur-là que `WriteTrace` écrit telle quelle.
 *
 * ⚠️ **L'encre est posée sur les items, une fois, par le canvas.** Le fichier n'a donc aucune règle
 * de trait à lui : il écrit ce qu'il voit, comme pour tous les autres formats. C'est ce qui garde
 * l'écran et le papier d'accord par construction, au lieu de deux réglages à tenir ensemble.
 *
 * @see Projector.md §5
 */
namespace OExportInk
{
/// 96 dpi / 72 pt : le facteur entre le point typographique et le pixel du fichier.
constexpr qreal kPtToPx = 96.0 / 72.0;

constexpr qreal kGeometryPx = 7.0 * kPtToPx;   ///< 7 pt — 2,47 mm
constexpr qreal kGrainlinePx = 5.25 * kPtToPx; ///< 5,25 pt — 1,85 mm
constexpr qreal kGridPx = 4.0 * kPtToPx;       ///< 4 pt — 1,41 mm

/// L'encre, unique : du noir pur. Le fichier de référence est en CMJN, hérité d'Illustrator ; la
/// projection, elle, n'a qu'un noir à offrir.
inline const QColor &Ink()
{
    static const QColor ink(Qt::black);
    return ink;
}

/// @brief Le pas de la grille en centimètres, et son nom de calque.
constexpr int kGridStepCm = 5;

/// @brief Le pas de la grille en pouces, et son nom de calque.
constexpr int kGridStepInch = 3;

/// @brief Pose l'encre du format sur @p root et sur tous ses enfants.
///
/// Le droit-fil et la ligne de pli sont reconnus **par leur type d'item** — la même lecture que
/// `WriteTrace` fait déjà pour leur donner leurs attributs — jamais par leur position ni leur nom.
void ApplyToSubtree(QGraphicsItem *root);
} // namespace OExportInk

#endif // OEXPORTINK_H
