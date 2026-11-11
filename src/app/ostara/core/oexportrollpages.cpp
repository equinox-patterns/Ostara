/************************************************************************
 **
 **  @file   oexportrollpages.cpp
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

#include "oexportrollpages.h"

#include "../oexportcoverpage.h"
#include "../oexportfootprint.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace
{
/// La tolérance des comparaisons de longueur : un demi-pixel, la même que la découpe du
/// projecteur (`oexportprojector.cpp`). Refuser une page qui dépasse de 0,2 px serait refuser
/// une page qui tient.
constexpr qreal kTolerancePx = 0.5;

/// Le plafond dur du PDF, en pixels — la même valeur que `PdfPageMaxMm`, convertie une fois.
auto PdfLimitPx() -> qreal
{
    return ToPixel(OExportRollPages::PdfPageMaxMm, Unit::Mm);
}

/// La page est-elle au-delà du plafond du PDF, en hauteur ou en largeur ?
auto PastPdfLimit(const QRectF &page) -> bool
{
    return page.height() > PdfLimitPx() + kTolerancePx || page.width() > PdfLimitPx() + kTolerancePx;
}

/// Le bloc de la page de garde, au coin de la première page — `InSheet`, le seul siège d'un
/// rouleau. Il est posé à `(marge, marge)` et mesure 190 × 259 mm : la page 1 doit le contenir
/// entier, même quand le plan s'arrête avant lui.
auto CoverBlockPx(qreal marginPx) -> QRectF
{
    return OExportCoverPage::BlockPx(OExportCoverPage::Seat::InSheet, marginPx);
}

/// Les pièces d'une page, rangées dans l'ordre du plan.
///
/// **Les indices sont ceux du vecteur d'origine**, jamais ceux du tri : le résultat de la
/// génération porte ses placements et ses clés dans le même ordre, et rendre des placements
/// triés les désalignerait en silence.
struct Grouped
{
    QVector<QVector<int>> pages{};
};

auto GroupPlacements(const QVector<OExportPlacement> &placements, qreal contentMaxPx) -> Grouped
{
    QVector<int> order(placements.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(),
              [&placements](int left, int right)
              {
                  const QRectF a = OExportFootprint::Rect(placements.at(left));
                  const QRectF b = OExportFootprint::Rect(placements.at(right));

                  if (std::abs(a.top() - b.top()) > kTolerancePx)
                  {
                      return a.top() < b.top();
                  }

                  return a.left() < b.left();
              });

    Grouped grouped;
    QRectF currentBox;
    QVector<int> current;

    for (const int index : order)
    {
        const QRectF one = OExportFootprint::Rect(placements.at(index));
        const QRectF grown = currentBox.isNull() ? one : currentBox.united(one);

        if (!currentBox.isNull() && grown.height() > contentMaxPx + kTolerancePx)
        {
            grouped.pages.append(current);
            current.clear();
            currentBox = one;
            current.append(index);
            continue;
        }

        currentBox = grown;
        current.append(index);
    }

    if (!current.isEmpty())
    {
        grouped.pages.append(current);
    }

    return grouped;
}

/// La boîte de contenu d'une page : l'union des empreintes de ses pièces.
auto BoxOf(const QVector<OExportPlacement> &placements, const QVector<int> &members) -> QRectF
{
    QRectF box;
    for (const int member : members)
    {
        const QRectF one = OExportFootprint::Rect(placements.at(member));
        box = box.isNull() ? one : box.united(one);
    }
    return box;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportRollPages::SettingsOf(const OExportOutput &output) -> Settings
{
    Settings settings;
    settings.binWidthPx = output.result.binWidthPx;
    settings.usedHeightPx = output.result.usedHeightPx;
    settings.marginPx = ToPixel(output.marginMm, Unit::Mm);
    settings.maxPageLengthMm = output.maxPageLengthMm;
    settings.trimSideMargins = output.trimSideMargins;
    // Un rouleau n'a qu'un siège possible, `InSheet` (`oexportprintmodel.cpp`) : le nom de la page
    // de garde suffit donc à dire qu'il y a un bloc à ne pas couper.
    settings.coverPage = !output.coverPageTemplateName.isEmpty();
    return settings;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRollPages::Compute(const QVector<OExportPlacement> &placements, const Settings &settings) -> Result
{
    Result result;

    const qreal margin = std::max<qreal>(0.0, settings.marginPx);
    const qreal sideTrim = settings.trimSideMargins ? margin : 0.0;
    const qreal pageWidth = std::max<qreal>(0.0, settings.binWidthPx - 2.0 * sideTrim);

    // **Zéro veut dire 500 cm, pas « pas de limite ».** Un champ vide garde donc le comportement
    // d'hier tant que le plan tient, et coupe au plafond du PDF quand il ne tient plus — c'est le
    // seul plafond qu'on ne peut pas demander de dépasser.
    const qreal askedMm =
        settings.maxPageLengthMm > 0.0 ? std::min(settings.maxPageLengthMm, PdfPageMaxMm) : PdfPageMaxMm;
    const qreal pageMaxPx = ToPixel(askedMm, Unit::Mm);
    const qreal contentMaxPx = std::max<qreal>(0.0, pageMaxPx - 2.0 * margin);

    const auto makePage = [&](qreal top, qreal bottom) -> QRectF
    {
        const qreal clampedTop = std::max<qreal>(0.0, top);
        const qreal clampedBottom = std::max(clampedTop, std::min(settings.usedHeightPx, bottom));
        return {sideTrim, clampedTop, pageWidth, clampedBottom - clampedTop};
    };

    const auto finish = [&](const QRectF &first)
    {
        QRectF page = first;
        if (settings.coverPage)
        {
            // Le bloc plus SA marge : une page ne se termine pas sur le coin de la page de garde.
            // La largeur est celle du rouleau : seule la hauteur s'allonge ici.
            page.setBottom(std::max(page.bottom(), CoverBlockPx(margin).bottom() + margin));
        }

        result.pageBeyondMax = page.height() > pageMaxPx + kTolerancePx;
        result.pastPdfLimit = PastPdfLimit(page);
        result.pages.append(page);
        return result;
    };

    // **Le plan tient sur une page** : rien à découper. C'est le cas de figure de tous les runs
    // d'aujourd'hui, et il doit rester exactement le même — le rectangle du bac, marges comprises.
    if (placements.isEmpty() || settings.usedHeightPx <= pageMaxPx + kTolerancePx)
    {
        return finish(makePage(0.0, settings.usedHeightPx));
    }

    const Grouped grouped = GroupPlacements(placements, contentMaxPx);

    // ── Les pages, dans le repère du plan ────────────────────────────────────────────────────────
    //
    // Une page est la boîte de son contenu plus la marge, en haut et en bas. **Et deux pages ne se
    // chevauchent jamais** : le haut d'une page ne remonte pas dans celle d'avant, même quand
    // l'espacement entre les deux pièces n'y suffirait pas. C'est ce qui garantit qu'aucune pièce
    // n'apparaît sur deux pages — un plan non réagencé (correction à la main) garde la règle, avec
    // une marge haute plus courte, plutôt que de laisser une pièce à cheval.
    qreal previousBottom = 0.0;
    for (int index = 0; index < grouped.pages.size(); ++index)
    {
        const QRectF box = BoxOf(placements, grouped.pages.at(index));

        qreal top = index == 0 ? 0.0 : box.top() - margin;
        if (index > 0)
        {
            top = std::max(top, previousBottom);
        }

        QRectF page = makePage(top, box.bottom() + margin);

        if (index == 0 && settings.coverPage)
        {
            page.setBottom(std::max(page.bottom(), CoverBlockPx(margin).bottom() + margin));
        }

        if (page.height() > pageMaxPx + kTolerancePx)
        {
            result.pageBeyondMax = true;
        }
        if (PastPdfLimit(page))
        {
            result.pastPdfLimit = true;
        }

        previousBottom = page.bottom();
        result.pages.append(page);
    }

    if (result.pages.isEmpty())
    {
        // Ne peut pas arriver — un plan non vide a au moins une boîte — mais un résultat qui ne
        // rend aucune page ferait tomber le PDF sur `pages.first()`.
        return finish(makePage(0.0, settings.usedHeightPx));
    }

    return result;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportRollPages::Layout(const QVector<OExportPlacement> &placements, const Settings &settings) -> LayoutResult
{
    LayoutResult result;
    result.placements = placements;
    result.usedHeightPx = settings.usedHeightPx;

    const qreal margin = std::max<qreal>(0.0, settings.marginPx);
    const qreal askedMm =
        settings.maxPageLengthMm > 0.0 ? std::min(settings.maxPageLengthMm, PdfPageMaxMm) : PdfPageMaxMm;
    const qreal pageMaxPx = ToPixel(askedMm, Unit::Mm);
    const qreal contentMaxPx = std::max<qreal>(0.0, pageMaxPx - 2.0 * margin);

    // Une seule page : rien à réagencer, et le plan reste exactement celui du nesting.
    if (placements.isEmpty() || settings.usedHeightPx <= pageMaxPx + kTolerancePx)
    {
        result.pages = Compute(placements, settings);
        return result;
    }

    const Grouped grouped = GroupPlacements(placements, contentMaxPx);

    // ── Le réagencement : chaque page posée EXACTEMENT sur le bas de la précédente ──────────────
    //
    // L'espacement du nesting est une distance entre deux PIÈCES ; une marge de page est une
    // distance entre une pièce et le BORD. Entre deux pages il en faut `2 × marge`. Quand le plan
    // n'en offre pas assez, la page suivante descend de ce qui manque ; quand il en offre
    // **davantage**, elle remonte du trop-plein. Dans les deux cas la page suivante commence sur le
    // bas de la précédente : le plan devient la somme des pages, et le trou qu'un grand espacement
    // laissait au milieu du plan — un blanc que le PDF n'imprime jamais — disparaît.
    //
    // Le déplacement peut donc être NÉGATIF. Aucune pièce ne change de page ; ce sont les mêmes
    // pages, posées les unes sur les autres.
    qreal shift = 0.0;
    qreal previousBottom = 0.0;

    for (int index = 0; index < grouped.pages.size(); ++index)
    {
        const QVector<int> &members = grouped.pages.at(index);

        for (const int member : members)
        {
            result.placements[member].y += shift;
        }

        QRectF box = BoxOf(result.placements, members);

        if (index > 0)
        {
            const qreal delta = previousBottom - (box.top() - margin);
            if (std::abs(delta) > kTolerancePx)
            {
                shift += delta;

                for (const int member : members)
                {
                    result.placements[member].y += delta;
                }

                box.translate(0.0, delta);
            }
        }

        qreal bottom = box.bottom() + margin;
        if (index == 0 && settings.coverPage)
        {
            bottom = std::max(bottom, CoverBlockPx(margin).bottom() + margin);
        }

        previousBottom = bottom;
    }

    result.usedHeightPx = previousBottom;

    Settings adjusted = settings;
    adjusted.usedHeightPx = result.usedHeightPx;
    result.pages = Compute(result.placements, adjusted);
    return result;
}
