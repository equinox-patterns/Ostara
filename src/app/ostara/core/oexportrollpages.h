/************************************************************************
 **
 **  @file   oexportrollpages.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  The pages of a roll plan: where the continuous band is cut.
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

#ifndef OEXPORTROLLPAGES_H
#define OEXPORTROLLPAGES_H

#include <QRectF>
#include <QVector>

#include "oexportoutput.h"

/**
 * @brief Les pages d'un plan de rouleau : la bande continue découpée.
 *
 * Un rouleau n'a pas de feuille : sa longueur est celle que le patron demande, et le PDF la
 * portait d'un seul tenant. Deux raisons de la couper, et une seule règle pour les deux :
 *
 *   - **la longueur demandée par l'utilisatrice** — les machines à plier ne prennent pas toutes
 *     la même longueur, et un plan qui les dépasse se plie à la main, donc se paie ;
 *   - **le plafond du PDF** — 500 cm, la plus grande page qu'un PDF puisse porter et rester
 *     lisible par un autre logiciel (`OExportProjector` le dit déjà pour le projecteur).
 *
 * **Aucune pièce n'est jamais à cheval sur deux pages** : ce n'est pas une préférence, c'est ce
 * qui rend le plan imprimable. La découpe se fait donc entre deux pièces, et la page est la boîte
 * de son contenu plus la marge — la dernière est plus courte que les autres, elle n'est jamais
 * complétée comme une feuille A0.
 *
 * Le calcul est une fonction PURE des placements et des réglages : il n'est stocké nulle part.
 * C'est ce qui le garde juste après une correction à la main — une pièce déplacée déplace la
 * découpe avec elle, sans qu'un second état puisse mentir sur le premier.
 */
namespace OExportRollPages
{
/// @brief Le plafond dur du PDF, en millimètres : 500 cm.
///
/// C'est la même valeur que `OExportProjector::Limits::pageMaxMm`. Trame n'écrit aucun
/// `/UserUnit` : au-delà, Qt divise la page et le PDF sort **rétréci sans le dire**.
constexpr qreal PdfPageMaxMm = 5000.0;

/// @brief Ce que la découpe a besoin de savoir, et rien de plus.
struct Settings
{
    qreal binWidthPx{0.0};  ///< la largeur du bac, marges comprises
    qreal usedHeightPx{0.0}; ///< la hauteur du plan, marges comprises
    qreal marginPx{0.0};     ///< la marge du document, sur les quatre côtés

    /// La longueur maximale d'une PAGE, en millimètres. **Zéro veut dire 500 cm** — le plafond du
    /// PDF — et non « pas de limite » : un PDF de plus de 500 cm n'est pas un PDF utilisable.
    qreal maxPageLengthMm{0.0};

    /// @brief Exporter le PDF sans les marges droite et gauche.
    ///
    /// Seules les deux largeurs sont retirées : le haut et le bas restent, parce qu'une imprimante
    /// a besoin de sa marge d'avance et que le pliage les paie de toute façon.
    bool trimSideMargins{false};

    /// @brief Une page de garde est posée au coin de la première page (siège `InSheet`).
    ///
    /// Elle ne se découpe pas : sa première page est donc au moins aussi haute que son bloc.
    bool coverPage{false};
};

/// @brief Le résultat de la découpe.
struct Result
{
    /// @brief Les pages, dans l'ordre du plan, en pixels du plan. Jamais vide.
    QVector<QRectF> pages{};

    /// @brief Une pièce est plus longue que la longueur demandée : sa page la dépasse.
    ///
    /// Ce n'est pas une erreur — la pièce ne peut pas être coupée, et le PDF reste imprimable —
    /// mais c'est exactement ce que la longueur demandée existe pour éviter (le pliage manuel).
    /// La barre d'état le dit.
    bool pageBeyondMax{false};

    /// @brief Une page dépasse le plafond du PDF : le fichier sortirait rétréci.
    ///
    /// Le run est refusé, plutôt que de livrer un PDF qui a l'air juste et qui ne l'est pas.
    bool pastPdfLimit{false};
};

/// @brief Le plan RÉAGENCÉ pour ses pages, et les pages qui en sortent.
struct LayoutResult
{
    /// @brief Les placements décalés pour que chaque page commence où la précédente finit.
    ///
    /// **Les pages sont indépendantes ET contiguës** : deux pages ne se chevauchent pas, et la
    /// marge de l'une n'est jamais la marge de l'autre. Quand l'espacement entre deux pièces ne
    /// donne pas `2 × marge`, la page suivante descend de ce qui manque ; quand il en donne
    /// **davantage**, elle remonte du trop-plein. Le plan se resserre donc au lieu de garder un
    /// trou que le PDF n'imprime pas : sa longueur devient la somme des pages.
    QVector<OExportPlacement> placements{};

    /// @brief La hauteur du plan après le réagencement, en pixels — plus grande que l'originale.
    qreal usedHeightPx{0.0};

    /// @brief Les pages du plan réagencé — voir @ref Compute.
    Result pages{};
};

/// @brief Les réglages que @p output porte pour sa découpe — le seul endroit qui les assemble.
///
/// A appeler **seulement sur un run d'impression de forme Roll**. Les autres formats n'ont pas de
/// page à découper de cette façon : les tuilés ont un cadre, le projecteur a ses propres pages.
auto SettingsOf(const OExportOutput &output) -> Settings;

/**
 * @brief Découpe la bande de @p placements en pages, entre les pièces.
 *
 * L'ordre est celui du plan — de haut en bas, puis de gauche à droite — pour que les pages se
 * suivent et que la dernière soit la fin du plan, jamais un patchwork. Une page se ferme quand la
 * pièce suivante la ferait dépasser : elle est donc au plus la longueur demandée, et souvent un
 * peu moins quand une grande pièce n'entre pas dans ce qui reste.
 */
auto Compute(const QVector<OExportPlacement> &placements, const Settings &settings) -> Result;

/**
 * @brief Réagence le plan pour que ses pages soient indépendantes, puis les découpe.
 *
 * **C'est cette fonction que la génération appelle**, et c'est elle qui écrit les placements de la
 * sortie : `Compute` ne fait que relire le résultat. Sans ce passage, deux pages voisines
 * partageraient leur marge — le bas de l'une et le haut de l'autre seraient le MÊME blanc du plan,
 * et une pièce posée dans cet espace apparaîtrait sur les deux pages. Avec lui, chaque page a son
 * blanc à elle, et aucune pièce ne peut être à cheval.
 *
 * Chaque page est posée **exactement sur le bas de la précédente** : les pièces descendent quand
 * l'espacement est trop court, remontent quand il est trop long, et aucune ne change de page. Le
 * plan devient la somme des pages — le trou qu'un grand espacement laissait entre deux pages n'est
 * plus dans le plan, et la surface du plan rejoint celle du PDF.
 */
auto Layout(const QVector<OExportPlacement> &placements, const Settings &settings) -> LayoutResult;
} // namespace OExportRollPages

#endif // OEXPORTROLLPAGES_H
