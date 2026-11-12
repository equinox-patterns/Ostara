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

#include "vgeometry/vabstractcurve.h"
#include "vgeometry/vcubicbezierpath.h"
#include "vgeometry/vpointf.h"
#include "vgeometry/vspline.h"
#include "vgeometry/vsplinepath.h"
#include "vmisc/testvapplication.h"
#include "vpatterndb/vcontainer.h"
#include "vpatterndb/vtranslatevars.h"
#include "vtools/tools/drawTools/operation/opleatcaprule.h"
#include "vtools/tools/drawTools/operation/oedgereader.h"
#include "vtools/tools/drawTools/operation/opleatnotch.h"
#include "vtools/tools/drawTools/operation/opleatrule.h"

#include <QtTest>

namespace
{
constexpr quint32 kDepartureFirst = 10;
constexpr quint32 kDepartureSecond = 11;

/** @brief Les variables du patron tel que l'application les nomme : un conteneur ne peut pas s'en passer, et les
 *  objets qu'on y range les lisent pour se nommer (`VGObject`, via `VAbstractApplication::VApp()->TrVars()`). */
VTranslateVars *g_trVars = nullptr;

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

/** @brief L'endroit où deux morceaux d'une courbe segmentée se joignent, dans un patron qui les porte tous les trois.
 *
 *  C'est l'état que la segmentation laisse dans le conteneur (§11), et le seul où la lecture d'arête a un choix à
 *  faire : le chemin entier et les deux morceaux qu'il contient ont **la même tangente** au nœud, donc rien dans la
 *  géométrie ne dit lequel répond — seul le côté, puis le fait qu'un morceau aboutit au point et que le chemin non.
 */
struct SegmentedEdge
{
    quint32 node{NULL_ID};
    quint32 up{NULL_ID};
    quint32 right{NULL_ID};
    quint32 path{NULL_ID};
};

/** @brief Les deux morceaux : l'un monte vers la gauche depuis le nœud, l'autre part vers la droite. */
constexpr qreal kNodeX = 0.;
constexpr qreal kNodeY = 9.;

auto AddJointPoint(VContainer &data) -> quint32
{
    return data.AddGObject(QSharedPointer<VPointF>::create(kNodeX, kNodeY, QStringLiteral("A21")));
}

/** @brief Le chemin qui porte les deux morceaux, fait des mêmes splines : c'est ce que la segmentation laisse. */
auto AddPath(VContainer &data, const VSpline &up, const VSpline &right) -> quint32
{
    return data.AddGObject(QSharedPointer<VSplinePath>::create(QVector<VSpline>{up, right}));
}

auto BuildSegmentedEdge(VContainer &data) -> SegmentedEdge
{
    SegmentedEdge edge;

    const VPointF start(0., 0., QStringLiteral("P0"));
    const VPointF node(kNodeX, kNodeY, QStringLiteral("P1"));
    const VPointF finish(9., 9., QStringLiteral("P2"));

    const VSpline up(start, QPointF(0., 3.), QPointF(0., 6.), node);
    const VSpline right(node, QPointF(3., 9.), QPointF(6., 9.), finish);

    edge.path = AddPath(data, up, right);
    edge.up = data.AddGObject(QSharedPointer<VSpline>::create(up));
    edge.right = data.AddGObject(QSharedPointer<VSpline>::create(right));
    edge.node = AddJointPoint(data);

    return edge;
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

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::TheSelectionTravelsWhenTheLineBoundsNothing()
{
    Unit unit = Unit::Px;
    VContainer data(g_trVars, &unit, VContainer::UniqueNamespace());

    const quint32 first = data.AddGObject(QSharedPointer<VPointF>::create(0., 0., QStringLiteral("B1")));
    const quint32 second = data.AddGObject(QSharedPointer<VPointF>::create(0., 10., QStringLiteral("B2")));
    const quint32 fabric = data.AddGObject(QSharedPointer<VPointF>::create(5., 5., QStringLiteral("B3")));

    // **La paire cliquée d'abord, le tissu ensuite** : le tronçon de la sélection entre les deux points ne contient
    // alors que la paire, et le pli n'emportait qu'elle — le tissu qu'on venait de sélectionner restait sur place, sa
    // copie posée exactement sur son original. C'est ce que Sophie a vu sur `A57__mp1` superposé à `A57`.
    const QVector<SourceItem> source{SourceItem{first}, SourceItem{second}, SourceItem{fabric}};

    OPleatDeparture departure;
    departure.first.pointId = first;
    departure.first.position = QPointF(0., 0.);
    departure.first.showCap = false;
    departure.second.pointId = second;
    departure.second.position = QPointF(0., 10.);
    departure.second.showCap = false;

    const OPleatPlan plan = OPleatRule::Compose({departure}, {3.}, source, &data, OPleatOrigin::FirstArm);

    QVERIFY2(plan.failure == OPleatFailure::None, qPrintable(OPleatFailureText(plan.failure)));

    // Le côté est lu sur toute la sélection (le tissu est à x > 0, la ligne est verticale) : le pas vaut deux fois
    // la valeur, le long de la normale — et il emporte le tissu, qui n'était dans aucun tronçon.
    QCOMPARE(plan.Move(fabric), QPointF(6., 0.));
    QCOMPARE(plan.Move(first), QPointF(6., 0.));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::CapOnACoordinatePointFollowsItsAxis()
{
    Unit unit = Unit::Px;
    VContainer data(g_trVars, &unit, VContainer::UniqueNamespace());

    // Le bout est un point **posé par coordonnées** : il n'est sur aucune courbe, son outil ne déclare aucun segment,
    // et le patron n'a pas de pièce. Rien ne dit donc de quel côté court l'arête — c'est le cas de A22 sur
    // `TestPlis.ostara`, où le pli demandait un point de plus.
    const quint32 first = data.AddGObject(QSharedPointer<VPointF>::create(0., 0., QStringLiteral("C1")));
    const quint32 second = data.AddGObject(QSharedPointer<VPointF>::create(10., 0., QStringLiteral("C2")));
    const quint32 witness = data.AddGObject(QSharedPointer<VPointF>::create(0., 5., QStringLiteral("C3")));
    const quint32 fabric = data.AddGObject(QSharedPointer<VPointF>::create(5., 5., QStringLiteral("C4")));

    const QVector<SourceItem> source{SourceItem{first}, SourceItem{second}, SourceItem{witness}, SourceItem{fabric}};

    OPleatDeparture departure;
    departure.first.pointId = first;
    departure.first.position = QPointF(0., 0.);
    departure.first.showCap = true;
    departure.second.pointId = second;
    departure.second.position = QPointF(10., 0.);
    departure.second.showCap = false; // le second bout ne demande rien : c'est le premier qui est mesuré

    const OPleatPlan plan = OPleatRule::Compose({departure}, {3.}, source, &data, OPleatOrigin::FirstArm);

    // C3 partage le X du bout : le contour court donc **droit**, verticalement. La ligne de départ est horizontale,
    // donc le bord est parallèle au déplacement : le chapeau est **plat** — le point au centre, rien à tracer.
    QVERIFY2(plan.failure == OPleatFailure::None, qPrintable(OPleatFailureText(plan.failure)));

    const OPleatMark *mark = plan.Mark(0, 0);
    QVERIFY(mark != nullptr);
    QVERIFY(mark->flat);
    // Le point d'un chapeau plat est le **milieu de l'ouverture**, donc sur la ligne de pli elle-même : le pas vaut
    // 2 × 3 depuis la ligne de départ, le milieu est à une valeur.
    QCOMPARE(mark->point, QPointF(0., 3.));
    QVERIFY(mark->edgeId == NULL_ID); // rien à couper : le chapeau plat ne trace aucun côté
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::CapReadsThePieceAndNeverThePath()
{
    Unit unit = Unit::Px;
    VContainer data(g_trVars, &unit, VContainer::UniqueNamespace());

    const SegmentedEdge edge = BuildSegmentedEdge(data);

    // Le côté est la seule chose qui dit de quel morceau le chapeau est lu : l'un monte vers la gauche du nœud,
    // l'autre part vers la droite. Et c'est un **morceau** qui répond, jamais le chemin, dont le nœud n'est pas un
    // bout — c'est ce que le cran exige, et ce qui manquait quand le chapeau gardait le morceau de l'autre sens.
    const OEdgeCarrier up = OEdgeReader::Carrier(edge.node, QPointF(0., -1.), &data);
    QCOMPARE(up.objectId, edge.up);

    const OEdgeCarrier right = OEdgeReader::Carrier(edge.node, QPointF(1., 0.), &data);
    QCOMPARE(right.objectId, edge.right);

    // Les deux sens ne peuvent pas rendre le même morceau : c'est tout le défaut que ce cas surveille.
    QVERIFY(up.objectId != right.objectId);
    QVERIFY(up.objectId != edge.path);
    QVERIFY(right.objectId != edge.path);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::CarrierFallsBackToThePathWhenNoPieceEndsThere()
{
    Unit unit = Unit::Px;
    VContainer data(g_trVars, &unit, VContainer::UniqueNamespace());

    // Un patron qui ne garde que le chemin, sans les morceaux de sa segmentation : le chemin répond encore, sinon
    // le chapeau n'existerait plus du tout là où la coupe n'a laissé aucune portion.
    const VPointF start(0., 0., QStringLiteral("P0"));
    const VPointF joint(kNodeX, kNodeY, QStringLiteral("P1"));
    const VPointF finish(9., 9., QStringLiteral("P2"));
    const quint32 path = AddPath(data, VSpline(start, QPointF(0., 3.), QPointF(0., 6.), joint),
                                 VSpline(joint, QPointF(3., 9.), QPointF(6., 9.), finish));
    const quint32 node = AddJointPoint(data);

    // La tangente d'un chemin à son nœud est orientée vers le milieu du chemin : c'est la géométrie qui dit de quel
    // côté elle tombe, pas le sens. Les deux côtés sont donc demandés, et l'un des deux doit rendre le chemin —
    // c'est le repli que la correction ne doit pas avoir supprimé.
    const OEdgeCarrier up = OEdgeReader::Carrier(node, QPointF(0., -1.), &data);
    const OEdgeCarrier down = OEdgeReader::Carrier(node, QPointF(0., 1.), &data);
    QVERIFY2(up.objectId == path or down.objectId == path, "the path must still answer when no piece carries it");
    QVERIFY(up.objectId != NULL_ID or down.objectId != NULL_ID);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::NotchCannotWalkAPathFromItsNode()
{
    Unit unit = Unit::Px;
    VContainer data(g_trVars, &unit, VContainer::UniqueNamespace());

    const SegmentedEdge edge = BuildSegmentedEdge(data);
    const QPointF node(kNodeX, kNodeY);

    // Le cran marche le long de l'arête depuis un bout : le nœud n'est pas un bout du chemin, donc il n'a nulle part
    // où aller et la règle refuse — ce refus est global, il emportait le chapeau avec lui. Sur le morceau, il tombe.
    const OPleatNotchPlacement onPath
        = OPleatNotchRule::Place(data.GeometricObject<VAbstractCurve>(edge.path), node, 3.);
    QVERIFY(onPath.length < 0.);

    const OPleatNotchPlacement onPiece
        = OPleatNotchRule::Place(data.GeometricObject<VAbstractCurve>(edge.up), node, 3.);
    QVERIFY(onPiece.length >= 0.);
    QVERIFY(qAbs(onPiece.length - 6.) < 0.01); // le morceau fait 9, le nœud est son bout : la valeur se compte à reculons
}

//---------------------------------------------------------------------------------------------------------------------
void TST_Pleat::CapSidesAreCutOnThePieceThePlanMeasured()
{
    Unit unit = Unit::Px;
    VContainer data(g_trVars, &unit, VContainer::UniqueNamespace());

    const SegmentedEdge edge = BuildSegmentedEdge(data);
    const QPointF node(kNodeX, kNodeY);

    // Le plan a mesuré l'apex sur le morceau qui part vers la droite, et c'est cette direction qu'il transmet à la
    // figure. Sans elle, `EdgeAt` prend la première sous-courbe du chemin — celle qui monte — et le côté du chapeau
    // est découpé sur l'autre morceau : c'est le défaut signalé, un chapeau qui garde le sens de l'autre.
    OPleatMark mark;
    mark.edgeId = edge.path;
    mark.length = 3.;
    mark.fromStart = true;
    mark.inward = QPointF(1., 0.);

    OPleatEnd prise;
    prise.pointId = edge.node;
    prise.position = node;

    const OPleatCapSides sides = OPleatCapRule::Sides(mark, prise, node, QLineF(QPointF(2., 0.), QPointF(2., 20.)),
                                                      QStringLiteral("capApex"), &data);

    QVERIFY(not sides.edge.isNull());
    QVERIFY(not sides.image.isNull());

    // Le côté part du point de la ligne de départ : c'est le début du morceau mesuré, et non celui de l'autre.
    const QVector<QPointF> points = sides.edge->GetPoints();
    QVERIFY(not points.isEmpty());
    QVERIFY(QLineF(points.constFirst(), node).length() < 0.01);
}

int main(int argc, char *argv[])
{
    TestVApplication app(argc, argv);

    VTranslateVars trVars;
    app.SetTrVars(&trVars);
    g_trVars = &trVars;

    TST_Pleat tests;
    return QTest::qExec(&tests, argc, argv);
}