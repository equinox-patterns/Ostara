/************************************************************************
 **
 **  @file   tst_rollpages.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/11/2026
 **
 **  @brief  La découpe d'un rouleau en pages, mesurée sur des placements construits à la main.
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

#include "tst_rollpages.h"

#include "core/oexportrollpages.h"

#include <QtTest>

#include <cmath>

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto Px(qreal mm) -> qreal
{
    return ToPixel(mm, Unit::Mm);
}

//---------------------------------------------------------------------------------------------------------------------
/// Une pièce sans contour : `OExportFootprint::Rect` prend alors son rectangle, ce qui suffit à
/// décrire un placement pour la découpe.
auto Piece(qreal topMm, qreal leftMm, qreal widthMm, qreal heightMm) -> OExportPlacement
{
    OExportPlacement placement;
    placement.x = Px(leftMm);
    placement.y = Px(topMm);
    placement.extentX = Px(widthMm);
    placement.extentY = Px(heightMm);
    return placement;
}

//---------------------------------------------------------------------------------------------------------------------
auto Settings(qreal binWidthMm, qreal usedHeightMm, qreal marginMm, qreal maxPageMm, bool trim = false,
              bool cover = false) -> OExportRollPages::Settings
{
    OExportRollPages::Settings settings;
    settings.binWidthPx = Px(binWidthMm);
    settings.usedHeightPx = Px(usedHeightMm);
    settings.marginPx = Px(marginMm);
    settings.maxPageLengthMm = maxPageMm;
    settings.trimSideMargins = trim;
    settings.coverPage = cover;
    return settings;
}

//---------------------------------------------------------------------------------------------------------------------
auto Near(qreal actualPx, qreal expectedMm) -> bool
{
    return std::abs(actualPx - Px(expectedMm)) < 1.0;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_RollPages::TST_RollPages(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_RollPages::FitsOnOnePage()
{
    const QVector<OExportPlacement> pieces{Piece(5.0, 5.0, 400.0, 300.0)};
    const OExportRollPages::Result result =
        OExportRollPages::Compute(pieces, Settings(841.0, 1000.0, 5.0, 0.0));

    QCOMPARE(result.pages.size(), 1);
    QVERIFY(Near(result.pages.first().x(), 0.0));
    QVERIFY(Near(result.pages.first().y(), 0.0));
    QVERIFY(Near(result.pages.first().width(), 841.0));
    QVERIFY(Near(result.pages.first().height(), 1000.0));
    QVERIFY(!result.pageBeyondMax);
    QVERIFY(!result.pastPdfLimit);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_RollPages::TrimmedWidth()
{
    const QVector<OExportPlacement> pieces{Piece(5.0, 5.0, 400.0, 300.0)};
    const OExportRollPages::Result result =
        OExportRollPages::Compute(pieces, Settings(841.0, 1000.0, 5.0, 0.0, /*trim*/ true));

    QCOMPARE(result.pages.size(), 1);
    // La marge est retirée à GAUCHE et à DROITE, et nulle part ailleurs : 841 - 2 × 5 = 831.
    QVERIFY(Near(result.pages.first().x(), 5.0));
    QVERIFY(Near(result.pages.first().width(), 831.0));
    QVERIFY(Near(result.pages.first().height(), 1000.0));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_RollPages::SplitAtMaxLength()
{
    // Deux pièces de 600 mm, séparées de 5 mm, sur une page limitée à 1 000 mm : la deuxième ne
    // tient pas sous la première (1 205 mm de contenu), donc deux pages, aucune pièce coupée.
    //
    // **Et les pages sont INDÉPENDANTES** : `Layout` pousse la seconde de 5 mm pour que chacune ait
    // sa marge haute et basse, au lieu de partager la même bande blanche.
    const QVector<OExportPlacement> pieces{
        Piece(5.0, 5.0, 400.0, 600.0),
        Piece(610.0, 5.0, 400.0, 600.0),
    };
    const OExportRollPages::LayoutResult layout =
        OExportRollPages::Layout(pieces, Settings(841.0, 1215.0, 5.0, 1000.0));

    QCOMPARE(layout.pages.pages.size(), 2);
    // Page 1 : de 0 à 610 mm (la pièce 1 finit à 605, plus sa marge basse).
    QVERIFY(Near(layout.pages.pages.at(0).y(), 0.0));
    QVERIFY(Near(layout.pages.pages.at(0).height(), 610.0));
    // Page 2 : elle COMMENCE où la première finit, et sa pièce a 5 mm de marge au-dessus.
    QVERIFY(Near(layout.pages.pages.at(1).y(), 610.0));
    QVERIFY(Near(layout.pages.pages.at(1).height(), 610.0));
    QVERIFY(Near(layout.placements.at(1).y, 615.0));
    QVERIFY(layout.pages.pages.at(0).bottom() <= layout.pages.pages.at(1).top() + 1.0);
    QVERIFY(!layout.pages.pageBeyondMax);
    QVERIFY(!layout.pages.pastPdfLimit);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_RollPages::SplitAndTrim()
{
    const QVector<OExportPlacement> pieces{
        Piece(5.0, 5.0, 400.0, 600.0),
        Piece(610.0, 5.0, 400.0, 600.0),
    };
    const OExportRollPages::LayoutResult layout =
        OExportRollPages::Layout(pieces, Settings(841.0, 1215.0, 5.0, 1000.0, /*trim*/ true));

    QCOMPARE(layout.pages.pages.size(), 2);
    for (const QRectF &page : layout.pages.pages)
    {
        QVERIFY(Near(page.x(), 5.0));
        QVERIFY(Near(page.width(), 831.0));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_RollPages::PieceLongerThanMax()
{
    // Une pièce ne se coupe pas : sa page la dépasse, et c'est ce que la barre d'état doit dire.
    const QVector<OExportPlacement> pieces{Piece(5.0, 5.0, 400.0, 600.0)};
    const OExportRollPages::Result result =
        OExportRollPages::Compute(pieces, Settings(841.0, 610.0, 5.0, 500.0));

    QCOMPARE(result.pages.size(), 1);
    QVERIFY(Near(result.pages.first().height(), 610.0));
    QVERIFY(result.pageBeyondMax);
    QVERIFY(!result.pastPdfLimit); // 610 mm est loin du plafond de 5 000 mm
}

//---------------------------------------------------------------------------------------------------------------------
void TST_RollPages::CompactsALargeGap()
{
    // Un grand espace entre les deux pages : la page 2 doit REMONTER pour se poser sur la page 1,
    // au lieu de laisser un trou que le PDF n'imprime pas.
    const QVector<OExportPlacement> pieces{
        Piece(5.0, 5.0, 400.0, 600.0),
        Piece(700.0, 5.0, 400.0, 600.0),
    };
    const OExportRollPages::LayoutResult layout =
        OExportRollPages::Layout(pieces, Settings(841.0, 1305.0, 5.0, 1000.0));

    QCOMPARE(layout.pages.pages.size(), 2);
    QVERIFY(Near(layout.pages.pages.at(0).height(), 610.0));
    QVERIFY(Near(layout.pages.pages.at(1).top(), 610.0));
    QVERIFY(Near(layout.pages.pages.at(1).height(), 610.0));
    QVERIFY(Near(layout.placements.at(1).y, 615.0));
    QVERIFY(Near(layout.usedHeightPx, 1220.0));
    QVERIFY(layout.pages.pages.at(0).bottom() <= layout.pages.pages.at(1).top() + 1.0);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_RollPages::ThreePagesWhenNeeded()
{
    const QVector<OExportPlacement> pieces{
        Piece(5.0, 5.0, 400.0, 300.0),
        Piece(310.0, 5.0, 400.0, 300.0),
        Piece(615.0, 5.0, 400.0, 300.0),
    };
    const OExportRollPages::LayoutResult layout =
        OExportRollPages::Layout(pieces, Settings(841.0, 920.0, 5.0, 400.0));

    QCOMPARE(layout.pages.pages.size(), 3);
    qreal previousBottom = 0.0;
    for (const QRectF &page : layout.pages.pages)
    {
        QVERIFY(Near(page.height(), 310.0));
        QVERIFY(page.top() >= previousBottom - 1.0);
        previousBottom = page.bottom();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_RollPages::CoverPageIsNotCut()
{
    // Le bloc de page de garde fait 190 × 259 mm, posé à la marge : la première page le contient
    // entier, même si le plan s'arrête bien avant lui.
    const QVector<OExportPlacement> pieces{Piece(5.0, 5.0, 400.0, 100.0)};
    const OExportRollPages::Result result =
        OExportRollPages::Compute(pieces, Settings(841.0, 110.0, 5.0, 200.0, /*trim*/ false,
                                                   /*cover*/ true));

    QCOMPARE(result.pages.size(), 1);
    QVERIFY(result.pages.first().height() >= Px(259.0));
    QVERIFY(result.pageBeyondMax); // 264 mm de page pour 200 demandés
    QVERIFY(!result.pastPdfLimit);
}

QTEST_MAIN(TST_RollPages)
