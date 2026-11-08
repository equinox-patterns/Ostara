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
#include <limits>

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
    /// Le plus GRAND côté du cadre de la pièce — et c'est lui, pas `widestPx`, qui dit si une pièce
    /// tient sur une page. Une pièce de 300 × 5200 mm a un petit côté de 300 : elle passe le test de
    /// largeur du bac, et pourtant aucun sens ne la fait tenir sous 5000. Les deux mesures servent,
    /// et à deux questions différentes.
    double longestPx{0.0};
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
            pieces.longestPx = std::max(pieces.longestPx, std::max(box.width(), box.height()));
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

// **Pas d'aide `Tr()` ici.** Un raccourci maison — `Tr(text)` appelant `QCoreApplication::translate` —
// rend les littéraux invisibles à `lupdate` : le message sort du `.ts`, donc du `.qm`, et s'affiche en
// anglais. Chaque littéral s'écrit donc en clair, à son appel.

/// « The plan does not fit in 500 × 500 cm » — la première ligne, la même dans les deux refus.
auto Headline(double pageMaxMm) -> QString
{
    return QCoreApplication::translate("OExportProjector", "The plan does not fit in %1 × %1 cm").arg(Cm(pageMaxMm));
}

/// **Le « à peu près carré » du mode `Square`** : 1,25 de rapport entre le grand côté et le petit.
///
/// Mesuré sur la PAGE publiée, jamais sur le numéro du palier : un palier de 300 peut sortir une page
/// de 287 (`Projector.md` §3.2), et c'est la page que l'utilisatrice projette.
///
/// 1,25 est la valeur de Sophie : « 200 × 250 ici sera considéré comme un carré ». Le rapport y vaut
/// exactement 1,25, donc la comparaison est **inclusive** — 200 × 251 n'est plus un carré.
constexpr double kAspectTolerance = 1.25;

/// Le rapport grand côté / petit côté d'une page. 1,0 pour un carré.
auto AspectOf(const QRectF &page) -> double
{
    const double longSide = std::max(page.width(), page.height());
    const double shortSide = std::min(page.width(), page.height());

    // Une page nulle n'a pas de forme ; elle ne doit simplement jamais gagner un concours d'aspect.
    return shortSide > 0.0 ? longSide / shortSide : std::numeric_limits<double>::max();
}

/// Le seul refus qui reste : **une pièce plus grande que la page elle-même.**
///
/// Tout le reste est découpable : un plan trop grand se coupe en pages, une page trop haute se coupe
/// en largeur. Une pièce qui ne tient pas dans 500 × 500 cm ne tiendra sur aucune page d'aucune
/// découpe, et rien dans le format ne peut la sauver.
auto PieceRefusal(const OExportProjector::Limits &limits, const Pieces &pieces, double marginMm) -> QString
{
    const double widestMm = pieces.widestPx * FromPixel(1.0, Unit::Mm);
    const double capacityMm = limits.pageMaxMm - 2.0 * marginMm;

    QStringList lines;
    lines << Headline(limits.pageMaxMm);
    lines << QCoreApplication::translate("OExportProjector",
                                         "One piece is %1 cm wide, and a page holds %2 cm with an edge margin of "
                                         "%3 cm. No page can hold it.")
                 .arg(Cm(widestMm), Cm(capacityMm), Cm(marginMm));

    return lines.join(QStringLiteral("\n\n"));
}

/// Ce que le sens promettait et que la forme n'a pas donné — **un avertissement, jamais un refus**.
///
/// Un plan qui sort plus haut que large en mode paysage reste un plan projetable : on le dit, et on
/// l'écrit quand même. Le refus (`error`) est réservé à ce qui ne peut pas exister.
auto ShapeWarning(const OExportProjector::Limits &limits, const QRectF &pagePx, double aspect) -> QString
{
    const double widthMm = pagePx.width() * FromPixel(1.0, Unit::Mm);
    const double heightMm = pagePx.height() * FromPixel(1.0, Unit::Mm);

    if (limits.shape == OExportPageShape::Square && aspect > kAspectTolerance)
    {
        return QCoreApplication::translate("OExportProjector",
                                           "No width gives a square plan: the closest is %1 × %2 cm. It is written "
                                           "as it is.")
            .arg(Cm(widthMm), Cm(heightMm));
    }

    if (limits.shape == OExportPageShape::Landscape && widthMm < heightMm)
    {
        return QCoreApplication::translate("OExportProjector",
                                           "The widest plan this format can hold is still taller than wide: %1 × %2 "
                                           "cm. It is written as it is.")
            .arg(Cm(widthMm), Cm(heightMm));
    }

    return QString();
}

/// La découpe en pages, quand aucune largeur ne donne une page qui contient le plan.
///
/// **Ce n'est pas un tuilage, et la différence est le prix.** Un tuilage paie du papier : il impose
/// des feuilles de taille fixe, identiques, parce qu'on les imprime. Un projecteur ne paie que de la
/// surface à projeter — donc chaque page fait **sa propre boîte serrée plus la marge**, et la
/// dernière est plus petite que les autres. C'est voulu, et c'est ce que Sophie a demandé.
///
/// La coupe est **horizontale et jamais au travers d'une pièce** : les placements sont pris dans
/// l'ordre du plan, et une page se ferme quand la pièce suivante ferait dépasser le plafond. Une
/// pièce entière par page, toujours — personne n'aligne deux projections au millimètre sur la même
/// pièce.
auto Paginate(const QVector<OExportSizeLayer> &layers, const OExportNestingParams &params,
              const OExportProjector::Limits &limits, const QVector<qreal> &gaps, bool descending,
              double pageMaxPx, double marginPx) -> OExportProjector::Outcome
{
    OExportProjector::Outcome outcome;

    const double usableMaxPx = pageMaxPx - 2.0 * marginPx;
    const Pieces pieces = MeasurePieces(layers);

    // Le bac de la découpe. **Il n'y a plus de palier à chercher** : le plan ne tient sur aucune page,
    // donc ce n'est plus la largeur qui décide de quoi que ce soit, c'est le sens demandé. Carré et
    // paysage prennent la largeur maximale — les pages sont alors aussi larges que possible, et la
    // coupe les rend carrées ou basses. Le portrait garde la bande la plus étroite où les pièces
    // entrent, pour que ses pages restent hautes et étroites, comme le mode le promet.
    double binWidthPx = pageMaxPx;
    if (!descending && limits.shape == OExportPageShape::Portrait)
    {
        const double narrowestUseful = std::max(pieces.areaPx / usableMaxPx + 2.0 * marginPx,
                                                pieces.widestPx + 2.0 * marginPx);
        for (const int widthMm : kWidthPaliersMm)
        {
            const double candidatePx = ToPixel(widthMm, Unit::Mm);
            if (candidatePx + kPageTolerancePx >= narrowestUseful)
            {
                binWidthPx = candidatePx;
                break;
            }
        }
    }

    OExportNestingResult result;
    QString engineError;
    qreal usedGapMm = gaps.isEmpty() ? limits.gapFloorMm : gaps.first();

    for (const qreal gapMm : gaps)
    {
        OExportNestingParams attempt = params;
        attempt.binWidthPx = binWidthPx;
        attempt.gapPx = ToPixel(gapMm, Unit::Mm);

        const OExportNestingResult one = OExportNesting::Run(layers, attempt);
        if (one.abandoned)
        {
            outcome.abandoned = true;
            return outcome;
        }

        if (one.ok && !TightBox(one.placements).isNull())
        {
            result = one;
            usedGapMm = gapMm;
            break;
        }

        if (engineError.isEmpty())
        {
            engineError = one.error;
        }
    }

    const QRectF box = TightBox(result.placements);
    if (!result.ok || box.isNull())
    {
        // Le moteur n'a rien pu poser : son message est vrai, et il est le seul à savoir pourquoi.
        outcome.error = engineError.isEmpty()
                            ? Headline(limits.pageMaxMm) + QStringLiteral("\n\n")
                                  + QCoreApplication::translate("OExportProjector",
                                                                "The pieces are wider than the largest plan this "
                                                                "format can hold, with the edge margin asked for.")
                            : Headline(limits.pageMaxMm) + QStringLiteral("\n\n") + engineError;
        return outcome;
    }

    // Les placements dans l'ordre du plan : de haut en bas, et de gauche à droite dans une ligne.
    // Sans cet ordre, une page se fermerait sur des pièces qui ne se suivent pas, et la dernière
    // page serait un patchwork au lieu d'être la fin du plan.
    QVector<OExportPlacement> placements = result.placements;
    std::sort(placements.begin(), placements.end(),
              [](const OExportPlacement &left, const OExportPlacement &right)
              {
                  const QRectF a = OExportFootprint::Rect(left);
                  const QRectF b = OExportFootprint::Rect(right);

                  if (std::abs(a.top() - b.top()) > kPageTolerancePx)
                  {
                      return a.top() < b.top();
                  }

                  return a.left() < b.left();
              });

    // ── Le remplissage : une page se ferme quand la pièce suivante la ferait déborder ───────────
    QVector<QRectF> sheetBoxes;
    QRectF current;
    for (const OExportPlacement &placement : placements)
    {
        const QRectF one = OExportFootprint::Rect(placement);
        const QRectF grown = current.isNull() ? one : current.united(one);

        // ⚠️ **La page, c'est la boîte PLUS la marge** (`Projector.md` §3.2) : c'est donc
        // `usableMaxPx`, et non le plafond, qui décide de la hauteur d'une page.
        if (!current.isNull() && grown.height() > usableMaxPx + kPageTolerancePx)
        {
            sheetBoxes.append(current);
            current = one;
            continue;
        }

        current = grown;
    }

    if (!current.isNull())
    {
        sheetBoxes.append(current);
    }

    // ── Le plan est TRANSLATÉ dans son repère, une fois, comme sur le chemin d'une seule page ────
    outcome.result = result;
    outcome.result.placements = placements;
    for (OExportPlacement &placement : outcome.result.placements)
    {
        placement.x -= box.left() - marginPx;
        placement.y -= box.top() - marginPx;
    }

    outcome.result.binWidthPx = box.width() + 2.0 * marginPx;
    outcome.result.usedHeightPx = box.height() + 2.0 * marginPx;
    outcome.result.ok = true;
    outcome.ok = true;
    outcome.widthMm = qRound(FromPixel(binWidthPx, Unit::Mm));
    outcome.gapMm = usedGapMm;

    // **Une seule page n'est pas une découpe.** Si le remplissage n'en a produit qu'une, le plan
    // tient : on laisse `pagesPx` vide, et tout ce qui suit lit la page unique comme il l'a toujours
    // fait. Un vecteur d'un élément serait un second chemin pour le même résultat.
    if (sheetBoxes.size() > 1)
    {
        QVector<QRectF> pages;
        pages.reserve(sheetBoxes.size());
        for (const QRectF &sheet : sheetBoxes)
        {
            pages.append(QRectF(sheet.left() - (box.left() - marginPx) - marginPx,
                                sheet.top() - (box.top() - marginPx) - marginPx,
                                sheet.width() + 2.0 * marginPx, sheet.height() + 2.0 * marginPx));
        }

        outcome.pagesPx = pages;
        outcome.warning = QCoreApplication::translate(
                              "OExportProjector",
                              "The plan is larger than %1 × %1 cm, the biggest page a PDF can hold and still be read "
                              "by other software. It is written on %2 pages, and no piece is across two of them.")
                              .arg(Cm(limits.pageMaxMm))
                              .arg(pages.size());
    }

    return outcome;
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

    // ── Le seul refus, et il se répond sans nidifier ──────────────────────────────────────────────
    //
    // Une pièce plus longue que la page elle-même ne tiendra sur aucune, dans aucun sens : ce n'est
    // pas un plan trop grand, c'est une pièce impossible, et aucune découpe ne la sauvera.
    //
    // **Tout le reste est découpable.** C'est la différence avec l'ancien format, qui refusait dès
    // que la surface des pièces dépassait celle d'une page — et qui privait donc l'utilisatrice d'un
    // fichier parfaitement projetable, en deux pages.
    if (usableMaxPx <= 0.0 || pieces.longestPx > usableMaxPx + kPageTolerancePx)
    {
        outcome.error = PieceRefusal(limits, pieces, marginMm);
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

    // **Le sens de la recherche n'est qu'un ordre**, et il ne change rien d'autre que cet ordre :
    // `Portrait` et `Square` montent du plus étroit au plus large, `Landscape` descend du plus large
    // au plus étroit. Aucune autre ligne du moteur ne connaît le sens.
    const bool descending = limits.shape == OExportPageShape::Landscape;

    // Ce que la recherche a trouvé. **Et ce n'est pas rendu tout de suite** : en mode carré, un
    // palier qui tient n'est pas encore le bon — il faut avoir vu les suivants pour savoir lequel
    // donne la page la plus proche du carré.
    OExportNestingResult winnerResult;
    QRectF winnerPage;
    int winnerWidthMm = 0;
    qreal winnerGapMm = 0.0;
    double winnerAspect = std::numeric_limits<double>::max();
    bool haveWinner = false;

    QString engineError;

    for (const qreal gapMm : gaps)
    {
        // Le premier espacement qui donne un plan a gagné : le plancher de 5 cm n'est tenté que si
        // l'espace demandé n'a rien donné, sur AUCUN palier (`Projector.md` §10.3).
        if (haveWinner)
        {
            break;
        }

        const double gapPx = ToPixel(gapMm, Unit::Mm);

        for (int slotIndex = 0; slotIndex < static_cast<int>(kWidthPaliersMm.size()); ++slotIndex)
        {
            const int slot = descending ? static_cast<int>(kWidthPaliersMm.size()) - 1 - slotIndex : slotIndex;
            const int widthMm = kWidthPaliersMm.at(static_cast<std::size_t>(slot));
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
            if (page.width() > pageMaxPx + kPageTolerancePx || page.height() > pageMaxPx + kPageTolerancePx)
            {
                // Il ne tient pas : le palier suivant a le droit d'exister, et le refus du moteur
                // n'arrête pas la recherche non plus.
                continue;
            }

            // ── Il tient. Reste à savoir s'il est le BON ─────────────────────────────────────────
            const double aspect = AspectOf(page);

            if (limits.shape != OExportPageShape::Square || aspect <= kAspectTolerance)
            {
                // Portrait et paysage : **le premier qui tient est le bon** — l'ordre des paliers le
                // dit, et rien d'autre n'a à en décider. Carré : il tient ET il est assez carré.
                winnerResult = result;
                winnerPage = page;
                winnerWidthMm = widthMm;
                winnerGapMm = gapMm;
                winnerAspect = aspect;
                haveWinner = true;
                break;
            }

            // Carré, mais trop allongé : on le garde comme pis-aller et on continue. Le palier
            // suivant est plus large, donc sa page plus basse, donc plus près du carré.
            if (!haveWinner || aspect < winnerAspect)
            {
                winnerResult = result;
                winnerPage = page;
                winnerWidthMm = widthMm;
                winnerGapMm = gapMm;
                winnerAspect = aspect;
            }
        }
    }

    // ── Le plan retenu est TRANSLATÉ dans le repère de sa page ───────────────────────────────────
    //
    // Tout ce qui le lit ensuite — le canvas, le PDF, l'image — prend les placements tels quels et la
    // page dans `binWidthPx` × `usedHeightPx`. Faire la translation ici, une fois, évite que trois
    // lecteurs la refassent, ou l'oublient.
    if (haveWinner)
    {
        outcome.result = winnerResult;
        for (OExportPlacement &placement : outcome.result.placements)
        {
            placement.x -= winnerPage.left();
            placement.y -= winnerPage.top();
        }

        outcome.result.binWidthPx = winnerPage.width();
        outcome.result.usedHeightPx = winnerPage.height();
        outcome.result.ok = true;
        outcome.ok = true;
        outcome.widthMm = winnerWidthMm;
        outcome.gapMm = winnerGapMm;
        outcome.warning = ShapeWarning(limits, winnerPage, winnerAspect);
        return outcome;
    }

    // ── Aucune page ne contient le plan : on le DÉCOUPE ──────────────────────────────────────────
    //
    // **Et c'est la seule issue.** Refuser priverait l'utilisatrice d'un fichier qu'elle peut
    // projeter : un plan de 6 m² sort très bien en deux pages, à condition qu'aucune pièce ne soit
    // coupée en deux. Le message qui l'explique est un avertissement, pas une erreur — il voyage
    // dans `warning`, et le fichier s'écrit quand même.
    Outcome paginated = Paginate(layers, params, limits, gaps, descending, pageMaxPx, marginPx);
    if (!paginated.ok && !paginated.abandoned && paginated.error.isEmpty() && !engineError.isEmpty())
    {
        paginated.error = Headline(limits.pageMaxMm) + QStringLiteral("\n\n") + engineError;
    }

    return paginated;
}
