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
};

/**
 * @brief Cherche la plus petite largeur dont la page tient sous le plafond.
 *
 * L'échelle des paliers est 150, 200, … 500 cm. L'espace demandé est tenté **sur tous les paliers
 * d'abord**, puis le plancher de 5 cm sur tous les paliers : l'espacement passe avant la compacité
 * (`Projector.md` §10.3).
 */
auto Nest(const QVector<OExportSizeLayer> &layers, const OExportNestingParams &params,
          const Limits &limits) -> Outcome;
} // namespace OExportProjector

#endif // OEXPORTPROJECTOR_H
