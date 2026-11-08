/************************************************************************
 **
 **  @file   oexportprojector.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/10/2026
 **
 **  @brief  Export mode — la recherche de largeur du format Projecteur.
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

#ifndef OEXPORTPROJECTOR_H
#define OEXPORTPROJECTOR_H

#include "core/oexportoutput.h"
#include "oexportgeometry.h"
#include "oexportnesting.h"

#include <QString>
#include <QVector>

/**
 * @brief The Projector format's page — one page, at 1:1, and never more than 500 × 500 cm.
 *
 * The engine needs a WIDTH and produces a height; a projector has no laize, so its width is
 * **searched**. This file is that search and nothing else: it calls the nesting with a ladder of
 * widths, measures what came back, and keeps the first plan whose page fits.
 *
 * **Why a ladder and not a formula.** The height does not decrease monotonically with the width —
 * the engine is a heuristic — so a dichotomy could step over a width that works. The ladder is
 * climbed in order, and the first success is the smallest width that fits.
 *
 * **Why the page is measured, not the bin.** A width palier is not the page: the engine keeps the
 * margin on both sides and the pieces do not fill the bin, so the page is the **tight box of the
 * placements, plus the edge margin on four sides** (`Projector.md` §3.2). That box is what the run
 * publishes as its bin, which is what the canvas draws and what the PDF page measures.
 *
 * ⚠️ **The cap is held, not patched up afterwards.** Past 5080 mm (14400 pt) a PDF page has to carry
 * a `/UserUnit`, and Trame writes none: the plan would come out **shrunk, and looking right**. So a
 * plan that does not fit is refused with its numbers, and no file is written.
 *
 * @see Projector.md §10 (la recherche), §11 (le refus)
 */
namespace OExportProjector
{
/// @brief Ce que le format supporte, et ce que l'utilisatrice a demandé.
struct Limits
{
    /// Le plafond de la page, en millimètres — **5000**, et pas un chiffre rond : c'est le plus grand
    /// entier sous les deux limites d'Acrobat (200 in = 5080 mm) et de Qt (14400 pt), qui tombent au
    /// même endroit. Voir `Projector.md` §3.1.
    qreal pageMaxMm{5000.0};

    /// L'espace demandé entre deux pièces, en millimètres — **10 cm par défaut**.
    qreal gapMm{100.0};

    /// Le plancher absolu de cet espace, en millimètres — **5 cm**. En dessous, ce n'est plus un plan
    /// projecteur, c'est un plan de coupe.
    qreal gapFloorMm{50.0};

    /// Dans quel sens chercher la largeur du bac — le seul réglage de la recherche.
    ///
    /// **Ajouté en fin de struct, et pas au milieu** : l'initialisation est positionnelle, et un champ
    /// inséré avant un autre décalerait silencieusement tous les suivants.
    OExportPageShape shape{OExportPageShape::Square};
};

/// @brief Ce qu'une recherche a donné.
struct Outcome
{
    bool ok{false};

    /// L'utilisatrice a fermé le dialogue : tout ce qui suit est à jeter, et rien ne se dit.
    bool abandoned{false};

    /// Le plan retenu, **déjà translaté dans le repère de sa page** et portant la page dans
    /// `binWidthPx` × `usedHeightPx` : tout ce qui le lit ensuite — canvas, PDF, image — n'a rien à
    /// savoir de la recherche.
    OExportNestingResult result{};

    /// Le refus, déjà rédigé et chiffré. Vide quand @ref ok.
    QString error{};

    /// La largeur du palier retenu, en millimètres — pour l'information de la barre d'état.
    int widthMm{0};

    /// L'espace entre pièces du plan retenu, en millimètres.
    qreal gapMm{0.0};

    /// Les pages du PDF, en pixels du plan — **vide quand le plan tient sur une seule page**.
    ///
    /// Chaque rectangle est la boîte serrée de sa page plus la marge au bord, donc les pages n'ont
    /// pas toutes la même taille. Elles sont exprimées dans le repère du plan publié
    /// (`result.placements`, déjà translatés), et c'est le writer qui découpe : il n'a pas à savoir
    /// qu'un projecteur a découpé quoi que ce soit.
    ///
    /// @see OExportOutput::projectorPagesPx — le même vecteur, de l'autre côté du run.
    QVector<QRectF> pagesPx{};

    /// Ce qui est vrai du plan sans empêcher de l'écrire — vide quand il n'y a rien à dire.
    ///
    /// **Un avertissement, jamais un refus.** Un plan qui sort plus haut que large en mode paysage,
    /// ou qui n'arrive pas à être carré, reste un plan utilisable : le refuser priverait
    /// l'utilisatrice d'un fichier qu'elle peut projeter. Le refus, lui, ne sert qu'à ce qui ne peut
    /// pas exister (`error`).
    QString warning{};
};

/**
 * @brief Cherche la plus petite largeur dont la page tient sous le plafond.
 *
 * L'échelle des paliers est 150, 200, … 500 cm. L'espace demandé est tenté **sur tous les paliers
 * d'abord**, puis le plancher de 5 cm sur tous les paliers : l'espacement passe avant la compacité
 * (`Projector.md` §10.3).
 *
 * **Le sens de la recherche est le seul réglage** (`Limits::shape`), et il ne change rien d'autre :
 *
 * - `Portrait` — les paliers du plus étroit au plus large, et **le premier qui tient est le bon** :
 *   la page la plus étroite, donc la plus haute. C'est ce que le format faisait avant.
 * - `Landscape` — les paliers du plus large au plus étroit, même règle : la page la plus large.
 * - `Square` — les paliers du plus étroit au plus large, mais on ne s'arrête qu'au premier dont le
 *   rapport grand côté / petit côté est **≤ 1,25** : une page à peu près carrée. Si aucun palier n'y
 *   arrive, on garde le plus proche **et on le dit** dans `Outcome::warning`.
 *
 * **Et quand aucune page ne tient**, le plan n'est pas refusé : il est **coupé en pages** d'au plus
 * 500 × 500 cm, sans qu'aucune pièce ne soit à cheval sur deux d'entre elles, et `Outcome::pagesPx`
 * porte la découpe. Le refus est réservé à ce qui ne peut pas exister — une pièce plus grande que la
 * page elle-même.
 */
auto Nest(const QVector<OExportSizeLayer> &layers, const OExportNestingParams &params,
          const Limits &limits) -> Outcome;
} // namespace OExportProjector

#endif // OEXPORTPROJECTOR_H
