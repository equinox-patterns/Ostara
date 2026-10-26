/************************************************************************
 **
 **  @file   oexportprojector.cpp
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

#include "oexportprojector.h"

#include "oexportfootprint.h"

#include "../vmisc/def.h"

#include <QCoreApplication>
#include <QLineF>
#include <QPolygonF>

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
/// Les paliers de largeur, en millimètres — **150, 200, … 500 cm**, et pas un de plus : le dernier
/// est le plafond de la page, donc une largeur au-delà ne pourrait de toute façon pas être publiée.
constexpr std::array<int, 8> kWidthPaliersMm{1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000};

/// La tolérance des comparaisons de longueur : un demi-pixel. Le plafond se mesure en points dans le
/// PDF et en pixels ici ; refuser un plan qui le dépasse de 0,2 px serait refuser un plan qui tient.
constexpr double kPageTolerancePx = 0.5;

/// Ce que les pièces occupent au minimum, et la plus étroite d'entre elles.
///
/// ⚠️ **L'aire est celle des EMPREINTES**, pas la somme des aires de toutes les tailles :
/// `OExportNesting::FootprintAreaPx` fait l'union que le moteur fera. Additionner chaque taille
/// donnerait sept fois la surface d'une seule — un plan de sept tailles aurait alors l'air de tenir
/// dans aucune, et le format refuserait des plans parfaitement possibles. Mesuré : c'est exactement
/// ce qui a fait démarrer une recherche au palier 350 alors que 150 était le premier à essayer.
struct Pieces
{
    double areaPx{0.0};
    double widestPx{0.0};
};

auto MeasurePieces(const QVector<OExportSizeLayer> &layers) -> Pieces
{
    Pieces pieces;
    pieces.areaPx = OExportNesting::FootprintAreaPx(layers);

    for (const OExportSizeLayer &layer : layers)
    {
        for (const OExportPieceGeometry &piece : layer.pieces)
        {
            const QVector<VLayoutPoint> ring = piece.piece.GetMappedExternalContourPoints();

            if (ring.isEmpty())
            {
                continue;
            }

            QRectF box(ring.first(), QSizeF(0.0, 0.0));
            for (const VLayoutPoint &point : ring)
            {
                box.setLeft(std::min(box.left(), point.x()));
                box.setRight(std::max(box.right(), point.x()));
                box.setTop(std::min(box.top(), point.y()));
                box.setBottom(std::max(box.bottom(), point.y()));
            }

            // Le côté le plus étroit du cadre : une pièce peut se poser dans le sens qui l'avantage,
            // et c'est ce côté-là qui dit si un bac peut la recevoir. Prendre le plus large
            // demanderait un palier de plus pour rien, jamais un palier de moins.
            pieces.widestPx = std::max(pieces.widestPx, std::min(box.width(), box.height()));
        }
    }

    return pieces;
}

/// Le cadre serré des placements, en pixels du bac.
auto TightBox(const QVector<OExportPlacement> &placements) -> QRectF
{
    QRectF box;
    for (const OExportPlacement &placement : placements)
    {
        const QRectF one = OExportFootprint::Rect(placement);
        box = box.isNull() ? one : box.united(one);
    }

    return box;
}

/// Un nombre de centimètres : **entier quand il l'est**, sinon à un dixième.
///
/// Un refus se lit, et « 500.0 × 500.0 cm » se lit moins bien que « 500 × 500 cm » — c'est la
/// formulation de la spec, et la seule chose que ce formateur ait à faire.
auto Cm(double millimetres) -> QString
{
    const double centimetres = millimetres / 10.0;
    if (qAbs(centimetres - std::round(centimetres)) < 1e-6)
    {
        return QString::number(centimetres, 'f', 0);
    }

    return QString::number(centimetres, 'f', 1);
}

/// Un nombre de mètres carrés, à un dixième.
auto SquareMetres(double millimetresSquared) -> QString
{
    return QString::number(millimetresSquared / 1000000.0, 'f', 1);
}

// **Pas d'aide `Tr()` ici.** Un raccourci maison — `Tr(text)` appelant `QCoreApplication::translate` —
// rend les littéraux invisibles à `lupdate` : le message sort du `.ts`, donc du `.qm`, et s'affiche en
// anglais. Chaque littéral s'écrit donc en clair, à son appel.

/// « The plan does not fit in 500 × 500 cm » — la première ligne, la même dans les deux refus.
auto Headline(double pageMaxMm) -> QString
{
    return QCoreApplication::translate("OExportProjector", "The plan does not fit in %1 × %1 cm").arg(Cm(pageMaxMm));
}

/// Le refus d'avant toute nidification : la surface des pièces ne tient dans aucun bac.
auto AreaRefusal(const OExportProjector::Limits &limits, const Pieces &pieces, double marginMm) -> QString
{
    const double capacityMm = limits.pageMaxMm - 2.0 * marginMm;
    const double aMinMm2 = pieces.areaPx * std::pow(FromPixel(1.0, Unit::Mm), 2.0);

    QStringList lines;
    lines << Headline(limits.pageMaxMm);
    lines << QCoreApplication::translate("OExportProjector",
                                         "The pieces take %1 m². With an edge margin of %2 cm, only %3 m² "
                                         "remain in a %4 × %4 cm plan.")
                 .arg(SquareMetres(aMinMm2), Cm(marginMm), SquareMetres(capacityMm * capacityMm), Cm(limits.pageMaxMm));

    // **La marge qui suffirait, et elle se calcule.** La surface utilisable d'un plan carré de côté
    // maximal est `(C − 2m)²` : elle doit couvrir l'aire des pièces, donc `m ≤ (C − √A) / 2`. C'est
    // une estimation — la forme du plan n'est pas carrée — et le message le dit (« would be enough »).
    const double needed = (limits.pageMaxMm - std::sqrt(aMinMm2)) / 2.0;
    const double rounded = std::max(0.0, std::floor(needed / 5.0) * 5.0);

    if (needed > 0.0 && rounded < marginMm)
    {
        lines << QCoreApplication::translate("OExportProjector", "Reducing the edge margin to %1 cm would be enough.")
                     .arg(Cm(rounded));
    }
    else
    {
        // Aucune marge n'y suffit : les pièces elles-mêmes remplissent le plus grand plan possible.
        lines << QCoreApplication::translate("OExportProjector",
                                             "No edge margin can make it fit: the pieces themselves fill the "
                                             "largest plan this format can hold.");
    }

    return lines.join(QStringLiteral("\n\n"));
}

/// Le refus d'après le dernier palier : la surface tient, la forme non.
auto ShapeRefusal(const OExportProjector::Limits &limits, const QRectF &pagePx, double marginMm,
                  double gapMm) -> QString
{
    const double mmPerPx = FromPixel(1.0, Unit::Mm);
    const double widthMm = pagePx.width() * mmPerPx;
    const double heightMm = pagePx.height() * mmPerPx;
    const double overflow = std::max(widthMm, heightMm) - limits.pageMaxMm;

    QStringList lines;
    lines << Headline(limits.pageMaxMm);
    lines << QCoreApplication::translate("OExportProjector",
                                         "The most compact plan obtained is %1 × %2 cm, with an edge margin of "
                                         "%3 cm and a space between pieces of %4 cm.")
                 .arg(Cm(widthMm), Cm(heightMm), Cm(marginMm), Cm(gapMm));

    const double needed = marginMm - overflow / 2.0;
    const double rounded = std::max(0.0, std::floor(needed / 5.0) * 5.0);
    if (needed >= 0.0 && rounded < marginMm)
    {
        lines << QCoreApplication::translate("OExportProjector", "Reducing the edge margin to %1 cm would be enough.")
                     .arg(Cm(rounded));
    }
    else
    {
        lines << QCoreApplication::translate("OExportProjector", "Reduce the space between pieces, or split the plan.");
    }

    return lines.join(QStringLiteral("\n\n"));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportProjector::Nest(const QVector<OExportSizeLayer> &layers, const OExportNestingParams &params,
                            const Limits &limits) -> Outcome
{
    Outcome outcome;

    const double pageMaxPx = ToPixel(limits.pageMaxMm, Unit::Mm);
    const double marginPx = params.marginPx;
    const double marginMm = FromPixel(marginPx, Unit::Mm);
    const double usableMaxPx = pageMaxPx - 2.0 * marginPx;

    const Pieces pieces = MeasurePieces(layers);

    // ── Le refus d'avant le calcul ────────────────────────────────────────────────────────────────
    //
    // Une seule question, et elle se répond sans nidifier : les pièces tiennent-elles, nues, dans le
    // plus grand plan que ce format sache publier ? Si non, aucun palier ne les fera tenir, et faire
    // tourner le moteur huit fois pour l'apprendre coûterait une minute à l'utilisatrice.
    if (usableMaxPx <= 0.0 || pieces.areaPx > usableMaxPx * usableMaxPx)
    {
        outcome.error = AreaRefusal(limits, pieces, marginMm);
        return outcome;
    }

    // ── L'espacement, puis le plancher ────────────────────────────────────────────────────────────
    //
    // 10 cm sur TOUS les paliers d'abord, 5 cm seulement si rien ne tient : un plan plus large à
    // 10 cm vaut mieux qu'un plan étroit à 5 cm (`Projector.md` §10.3).
    // ⚠️ **Le plancher est tenu ICI aussi**, pas seulement dans l'onglet : une recette rejouée, un
    // favori d'une version d'avant, ou un appel direct pourraient demander moins de 5 cm — et un plan
    // projecteur ne descend jamais sous son plancher, quel que soit celui qui le demande. Demander
    // moins donne le plancher, pas un refus : c'est un format, pas une saisie à corriger.
    const qreal askedGapMm = std::max(limits.gapMm, limits.gapFloorMm);

    QVector<qreal> gaps{askedGapMm};
    if (limits.gapFloorMm > 0.0 && limits.gapFloorMm < askedGapMm)
    {
        gaps.append(limits.gapFloorMm);
    }

    // Le premier palier qui PUISSE tenir : une largeur plus petite ne peut pas recevoir les pièces.
    const double narrowestUseful = std::max(pieces.areaPx / usableMaxPx + 2.0 * marginPx,
                                            pieces.widestPx + 2.0 * marginPx);

    QRectF best;
    double bestGapMm = gaps.first();
    QString engineError;

    for (const qreal gapMm : gaps)
    {
        const double gapPx = ToPixel(gapMm, Unit::Mm);

        for (const int widthMm : kWidthPaliersMm)
        {
            const double binWidthPx = ToPixel(widthMm, Unit::Mm);

            // ⚠️ Un palier dont la surface utile ne couvre pas les pièces ne peut rien recevoir :
            // le sauter est une déduction, pas un pari — l'aire mesurée est un minorant.
            const double usableWidthPx = binWidthPx - 2.0 * marginPx;
            if (usableWidthPx <= 0.0 || pieces.areaPx > usableWidthPx * usableMaxPx)
            {
                continue;
            }

            // Et celui qui est sous la largeur minimale non plus.
            if (binWidthPx + kPageTolerancePx < narrowestUseful)
            {
                continue;
            }

            OExportNestingParams attempt = params;
            attempt.binWidthPx = binWidthPx;
            attempt.gapPx = gapPx;

            const OExportNestingResult result = OExportNesting::Run(layers, attempt);
            if (result.abandoned)
            {
                outcome.abandoned = true;
                return outcome;
            }

            if (!result.ok)
            {
                // **Un refus du moteur n'arrête pas la recherche** : c'est le palier qui est trop
                // étroit, et le suivant a le droit d'exister. Le message est gardé au cas où aucun
                // ne passerait.
                if (engineError.isEmpty())
                {
                    engineError = result.error;
                }
                continue;
            }

            const QRectF box = TightBox(result.placements);
            if (box.isNull())
            {
                continue;
            }

            // **La page, c'est la boîte serrée plus l'écart au bord** — jamais le bac. Un palier de
            // largeur n'est pas une taille de page (`Projector.md` §3.2), et c'est justement ce qui
            // permet à un palier de 300 de sortir une page de 287.
            const QRectF page(box.left() - marginPx, box.top() - marginPx,
                              box.width() + 2.0 * marginPx, box.height() + 2.0 * marginPx);
            if (page.width() <= pageMaxPx + kPageTolerancePx
                && page.height() <= pageMaxPx + kPageTolerancePx)
            {
                // ── Retenu : le plan est TRANSLATÉ dans le repère de sa page ─────────────────────
                //
                // Tout ce qui le lit ensuite — le canvas, le PDF, l'image — prend les placements
                // tels quels et la page dans `binWidthPx` × `usedHeightPx`. Faire la translation
                // ici, une fois, évite que trois lecteurs la refassent, ou l'oublient.
                outcome.result = result;
                for (OExportPlacement &placement : outcome.result.placements)
                {
                    placement.x -= box.left() - marginPx;
                    placement.y -= box.top() - marginPx;
                }

                outcome.result.binWidthPx = page.width();
                outcome.result.usedHeightPx = page.height();
                outcome.result.ok = true;
                outcome.ok = true;
                outcome.widthMm = widthMm;
                outcome.gapMm = gapMm;
                return outcome;
            }

            // Le plus compact jusqu'ici : celui dont le plus grand côté dépasse le moins.
            if (best.isNull()
                || std::max(page.width(), page.height()) < std::max(best.width(), best.height()))
            {
                best = page;
                bestGapMm = gapMm;
            }
        }
    }

    // ── Le refus d'après le dernier palier ────────────────────────────────────────────────────────
    if (!best.isNull())
    {
        outcome.error = ShapeRefusal(limits, best, marginMm, bestGapMm);
        return outcome;
    }

    // ── Et quand rien n'a même pu être posé ──────────────────────────────────────────────────────
    //
    // Deux causes, et elles ne se disent pas de la même façon. Le moteur a refusé tous les bacs :
    // son message est vrai, mais il parle de largeur alors que la question est la page — on le cite
    // sous le constat, jamais à sa place. Ou bien aucun bac n'a même été essayé : la pièce la plus
    // large ne passe dans aucun, et il n'y a rien à essayer.
    if (!engineError.isEmpty())
    {
        outcome.error = Headline(limits.pageMaxMm) + QStringLiteral("\n\n") + engineError;
        return outcome;
    }

    outcome.error = Headline(limits.pageMaxMm) + QStringLiteral("\n\n")
                    + QCoreApplication::translate("OExportProjector",
                                                  "The pieces are wider than the largest plan this format can hold, "
                                                  "with the edge margin asked for. Reduce the edge margin.");
    return outcome;
}
