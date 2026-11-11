/************************************************************************
 **
 **  @file   tst_pleat.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  Le pli, mesure sur des plans construits a la main : le deplacement d'un objet de la
 **          region, l'immobilite de celui qui n'y est pas, la place de la ligne de depart, et les
 **          refus qui doivent tous dire quoi faire.
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

#include "tst_pleat.h"

#include "vtools/tools/drawTools/operation/opleatrule.h"

#include <QtTest>

namespace
{
constexpr quint32 kDepartureFirst = 10;
constexpr quint32 kDepartureSecond = 11;

/** @brief The plan of one simple pleat: the fabric a value above the line, the line's two objects taken. */
auto OneSimplePleat(const QPointF &step) -> OPleatPlan
{
    OPleatPlan plan;

    OPleatPlace place;
    place.start = 0;
    place.last = 1;
    place.pleats = 1;
    place.firstPleat = 0;
    place.step = step;
    plan.places.append(place);

    // Every object the region holds travels one pleat, the two points of the line included: what keeps those two
    // in place is the place the rule asks for them, not a zero here (see OPleatPlan::Place).
    plan.sources = {kDepartureFirst, kDepartureSecond};
    plan.moves.insert(kDepartureFirst, step);
    plan.moves.insert(kDepartureSecond, step);
    plan.moves.insert(20, step);

    return plan;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_Pleat::TST_Pleat(QObject *parent) : QObject(parent) {}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::ObjectInTheRegionTravels()
{
    // Un pli vaut deux fois sa valeur : le pas est le deplacement de tout ce que la region contient.
    const OPleatPlan plan = OneSimplePleat(QPointF(0., -2.));
    QCOMPARE(plan.Move(20), QPointF(0., -2.));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::ObjectOutOfTheRegionStays()
{
    // Hors de toutes les regions, un objet ne bouge pas : la copie est posee sur l'original, et rien ne le dit.
    const OPleatPlan plan = OneSimplePleat(QPointF(0., -2.));
    QCOMPARE(plan.Move(99), QPointF());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::DepartureLineStandsOnItself()
{
    // La ligne de depart n'est pas emportee par son propre pli : placee a son rang, elle tombe sur elle-meme.
    const OPleatPlan plan = OneSimplePleat(QPointF(0., -2.));

    OPleatEnd prise;
    prise.pointId = kDepartureFirst;
    prise.position = QPointF(5., 5.);

    QCOMPARE(plan.Place(prise, 0, 0), QPointF(5., 5.));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::SecondLineIsOneStepOn()
{
    // La copie de la ligne est un pas plus loin : c'est la ou le tissu arrive apres le pli.
    const OPleatPlan plan = OneSimplePleat(QPointF(0., -2.));

    OPleatEnd prise;
    prise.pointId = kDepartureFirst;
    prise.position = QPointF(5., 5.);

    QCOMPARE(plan.Place(prise, 0, 1), QPointF(5., 3.));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::PleatCountAddsTheFolds()
{
    // Un pli double compte deux plis : la figure les numerote dans l'ordre des lignes donnees.
    OPleatPlan plan = OneSimplePleat(QPointF(0., -2.));
    plan.places[0].pleats = 2;

    QCOMPARE(plan.PleatCount(), 2);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::EveryRefusalSaysWhatToDo()
{
    // A failure the user cannot name is a failure they cannot correct: none of them may be silent.
    const QVector<OPleatFailure> failures{OPleatFailure::DegeneratePrise,
                                          OPleatFailure::DegenerateValue,
                                          OPleatFailure::NoSide,
                                          OPleatFailure::UnresolvedEdge,
                                          OPleatFailure::EdgeAlongFold,
                                          OPleatFailure::NotchOffEdge,
                                          OPleatFailure::SplitDeparture,
                                          OPleatFailure::EmptyRegion};

    for (const OPleatFailure failure : failures)
    {
        QVERIFY2(not OPleatFailureText(failure).isEmpty(), qPrintable(QString::number(static_cast<int>(failure))));
    }

    QVERIFY(OPleatFailureText(OPleatFailure::None).isEmpty());
}

QTEST_MAIN(TST_Pleat)
