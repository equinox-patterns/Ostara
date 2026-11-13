/************************************************************************
 **
 **  @file   tst_linerolenames.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/10/2026
 **
 **  @brief  The reported failure, reduced to the container: a name left behind by a deleted pair holds its
 **          suffix forever, and naming a live line with it is refused in silence. Pruning frees it, and the
 **          live pair can take it.
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

#include "tst_linerolenames.h"

#include "vgeometry/vpointf.h"
#include "vgeometry/vsplinepath.h"
#include "vgeometry/vsplinepoint.h"
#include "vpatterndb/variables/vinternalvariable.h"
#include "vmisc/testvapplication.h"
#include "vpatterndb/vcontainer.h"
#include "vpatterndb/vtranslatevars.h"

#include <QtTest>

namespace
{
auto AddPoint(VContainer &data, const QString &name) -> quint32
{
    return data.AddGObject(QSharedPointer<VPointF>::create(0, 0, name));
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_LineRoleNames::TST_LineRoleNames(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LineRoleNames::OrphanNameIsDroppedAndTheNameIsFreed()
{
    VTranslateVars trVars;
    Unit unit = Unit::Cm;
    VContainer data(&trVars, &unit, VContainer::UniqueNamespace());

    AddPoint(data, QStringLiteral("A"));
    AddPoint(data, QStringLiteral("B"));

    // The live line takes its name; the other pair is a leftover whose points do not exist — the state a deleted
    // piece leaves in `<lineNames>`, `TailleDos -> Tm Mb` being the reported one.
    data.SetLineRoleName(QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("Carrure"));
    data.SetLineRoleName(QStringLiteral("Tm"), QStringLiteral("Mb"), QStringLiteral("TailleDos"));
    QCOMPARE(data.LineRoleNames().size(), 2);

    // The leftover holds the suffix: the live line cannot take it, and nothing says so.
    data.SetLineRoleName(QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("TailleDos"));
    QCOMPARE(data.LineRoleName(QStringLiteral("A_B")), QStringLiteral("Carrure"));
    QVERIFY(data.IsLineRoleNameTaken(QStringLiteral("TailleDos")));

    // The parse drops the pair whose points are gone...
    QCOMPARE(data.PruneOrphanLineRoleNames(), 1);
    QCOMPARE(data.LineRoleNames().size(), 1);
    QCOMPARE(data.LineRoleName(QStringLiteral("A_B")), QStringLiteral("Carrure"));
    QVERIFY(data.LineRoleName(QStringLiteral("Tm_Mb")).isEmpty());

    // ...and the name is free again.
    QVERIFY(not data.IsLineRoleNameTaken(QStringLiteral("TailleDos")));
    data.SetLineRoleName(QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("TailleDos"));
    QCOMPARE(data.LineRoleName(QStringLiteral("A_B")), QStringLiteral("TailleDos"));

    // Nothing else to drop: a second pass is quiet.
    QCOMPARE(data.PruneOrphanLineRoleNames(), 0);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LineRoleNames::NameNoLineBearsIsDropped()
{
    VTranslateVars trVars;
    Unit unit = Unit::Cm;
    VContainer data(&trVars, &unit, VContainer::UniqueNamespace());

    const quint32 a = AddPoint(data, QStringLiteral("A"));
    const quint32 b = AddPoint(data, QStringLiteral("B"));

    // Both points are there, and no line joins them: the name survives the point test, and only the bearer
    // question can let it go. This is the reported case — a deleted line whose ends stayed in the piece.
    data.SetLineRoleName(QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("Carrure"));
    QCOMPARE(data.PruneOrphanLineRoleNames(true), 1);
    QVERIFY(data.LineRoleName(QStringLiteral("A_B")).isEmpty());
    QVERIFY(not data.IsLineRoleNameTaken(QStringLiteral("Carrure")));

    // A tool takes the role for the pair: the name is borne, and the bearer test keeps it.
    data.SetLineRoleName(QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("Carrure"));
    data.AddLine(a, b, 0, true);
    QCOMPARE(data.PruneOrphanLineRoleNames(true), 0);
    QCOMPARE(data.LineRoleName(QStringLiteral("A_B")), QStringLiteral("Carrure"));

    // And the old question still answers on its own: the pair lives, so its name is not an orphan.
    QCOMPARE(data.PruneOrphanLineRoleNames(), 0);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LineRoleNames::UnreadablePairIsKept()
{
    VTranslateVars trVars;
    Unit unit = Unit::Cm;
    VContainer data(&trVars, &unit, VContainer::UniqueNamespace());

    AddPoint(data, QStringLiteral("A"));

    // Declared from the key alone, with no object to recover the second half: the pattern cannot read the pair,
    // so it keeps what it cannot read instead of guessing it away. `A_B` and `A_B_C` would otherwise both be
    // plausible readings of a name holding an underscore.
    data.SetLineRoleName(QStringLiteral("A_B"), QStringLiteral("Carrure"));
    QCOMPARE(data.PruneOrphanLineRoleNames(), 0);
    QCOMPARE(data.LineRoleName(QStringLiteral("A_B")), QStringLiteral("Carrure"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_LineRoleNames::NameOfAnotherLineIsTaken()
{
    VTranslateVars trVars;
    Unit unit = Unit::Cm;
    VContainer data(&trVars, &unit, VContainer::UniqueNamespace());

    AddPoint(data, QStringLiteral("A"));
    AddPoint(data, QStringLiteral("B"));
    data.SetLineRoleName(QStringLiteral("A"), QStringLiteral("B"), QStringLiteral("Carrure"));

    QVERIFY(data.IsLineRoleNameTaken(QStringLiteral("Carrure")));
    QVERIFY(not data.IsLineRoleNameTaken(QStringLiteral("Carrure"), QStringLiteral("A_B")));
    QVERIFY(not data.IsLineRoleNameTaken(QStringLiteral("Autre")));
    QVERIFY(not data.IsLineRoleNameTaken(QString()));
}

//---------------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    TestVApplication app(argc, argv);
    TST_LineRoleNames tests;
    return QTest::qExec(&tests, argc, argv);
}


//---------------------------------------------------------------------------------------------------------------------
// A drag re-reads its block on every mouse move without clearing the variables. Each segment of a spline path must
// take back the route it holds (`Curve_A_B`) instead of a new one beside it: otherwise the route stays on the
// variable of the first move — a formula reads that move's length — and five keys are added per segment and per move.
void TST_LineRoleNames::SegmentTakesItsRouteBackWhenReadAgain()
{
    VTranslateVars trVars;
    Unit unit = Unit::Cm;
    VContainer data(&trVars, &unit, VContainer::UniqueNamespace());

    const auto Point = [](qreal x, qreal y, const QString &name) -> VSplinePoint
    {
        return {VPointF(x, y, name, 0, 0), 0, QStringLiteral("0"), 180, QStringLiteral("180"), 20,
                QStringLiteral("20"), 20, QStringLiteral("20")};
    };
    const auto Path = [&Point](qreal shift) -> QSharedPointer<VSplinePath>
    {
        return QSharedPointer<VSplinePath>::create(QVector<VSplinePoint>{
            Point(0, 0, QStringLiteral("A")), Point(100 + shift, 30, QStringLiteral("B")),
            Point(200, 0, QStringLiteral("C"))});
    };

    QSharedPointer<VSplinePath> path = Path(0);
    const quint32 id = data.AddGObject(path);
    data.AddCurveWithSegments(path, id);

    const auto *variables = data.DataVariables();
    const qsizetype registered = variables->size();
    const QString route = QStringLiteral("Curve_A_B");
    QVERIFY(variables->contains(route));

    // The same path, moved, registered again: the state of a drag's second mouse move.
    path = Path(50);
    data.UpdateGObject(id, path);
    data.AddCurveWithSegments(path, id);

    QCOMPARE(variables->size(), registered); // no new key
    const QSharedPointer<VInternalVariable> byRoute = variables->value(route);
    QVERIFY(not byRoute.isNull());
    QCOMPARE(variables->value(byRoute->GetName()), byRoute); // the route reads the variable of this move
    QVERIFY(not variables->contains(route + QStringLiteral("_1")));
}
