/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   11 9, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "tst_vtooluniondetails.h"
#include "../vgeometry/vpointf.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vtools/tools/vtooluniondetails.h"

#include <QtTest>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

using UnitedPathNode = QPair<bool, VPieceNode>;

Q_DECLARE_METATYPE(UnitedPathNode)

//---------------------------------------------------------------------------------------------------------------------
TST_VToolUnionDetails::TST_VToolUnionDetails(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VToolUnionDetails::TestUnitingMainPaths_data()
{
    QTest::addColumn<VPiecePath>("d1");
    QTest::addColumn<VPiecePath>("d2");
    QTest::addColumn<quint32>("indexD2");
    QTest::addColumn<quint32>("pRotate");
    QTest::addColumn<QVector<UnitedPathNode>>("result");

    VPieceNode const d1p0(1205, Tool::NodePoint);
    VPieceNode const d1p1(1206, Tool::NodePoint);
    VPieceNode const d1p2(1207, Tool::NodeSpline);
    VPieceNode const d1p3(1208, Tool::NodePoint);
    VPieceNode const d1p4(1209, Tool::NodePoint);
    VPieceNode const d1p5(1204, Tool::NodeSpline);

    VPieceNode const d2p0(1211, Tool::NodePoint);
    VPieceNode const d2p1(1212, Tool::NodePoint);
    VPieceNode const d2p2(1213, Tool::NodeSpline);
    VPieceNode const d2p3(1214, Tool::NodePoint);
    VPieceNode const d2p4(1215, Tool::NodePoint);

    QVector<UnitedPathNode> result;
    result.append(qMakePair(true, d1p0));
    result.append(qMakePair(true, d1p1));
    result.append(qMakePair(true, d1p2));
    result.append(qMakePair(true, d1p3));
    result.append(qMakePair(false, d2p2));
    result.append(qMakePair(false, d2p3));
    result.append(qMakePair(false, d2p4));
    // The second piece contributes neither end of its own seam edge: the start of it is one point with the end of
    // the first edge, and the end of it one point with the start of the first. What is left of the first piece on
    // both sides of the seam stays.
    result.append(qMakePair(true, d1p4));
    result.append(qMakePair(true, d1p5));

    VPiecePath d1;
    d1.Append(d1p0);
    d1.Append(d1p1);
    d1.Append(d1p2);
    d1.Append(d1p3);
    d1.Append(d1p4);
    d1.Append(d1p5);

    VPiecePath d2;
    d2.Append(d2p0);
    d2.Append(d2p1);
    d2.Append(d2p2);
    d2.Append(d2p3);
    d2.Append(d2p4);

    quint32 const indexD2 = 0;
    quint32 const pRotate = 1208;

    // See the file <root>/src/app/share/collection/Issue_#752.val
    QTest::newRow("Case 1") << d1 << d2 << indexD2 << pRotate << result;

    result.clear();
    result.append(qMakePair(true, d1p1));
    result.append(qMakePair(true, d1p2));
    result.append(qMakePair(true, d1p3));
    result.append(qMakePair(false, d2p2));
    result.append(qMakePair(false, d2p3));
    result.append(qMakePair(false, d2p4));
    result.append(qMakePair(true, d1p4));
    result.append(qMakePair(true, d1p5));
    result.append(qMakePair(true, d1p0));

    d1.Clear();
    d1.Append(d1p1);
    d1.Append(d1p2);
    d1.Append(d1p3);
    d1.Append(d1p4);
    d1.Append(d1p5);
    d1.Append(d1p0);

    QTest::newRow("Case 2") << d1 << d2 << indexD2 << pRotate << result;

    result.clear();
    result.append(qMakePair(true, d1p2));
    result.append(qMakePair(true, d1p3));
    result.append(qMakePair(false, d2p2));
    result.append(qMakePair(false, d2p3));
    result.append(qMakePair(false, d2p4));
    result.append(qMakePair(true, d1p4));
    result.append(qMakePair(true, d1p5));
    result.append(qMakePair(true, d1p0));
    result.append(qMakePair(true, d1p1));

    d1.Clear();
    d1.Append(d1p2);
    d1.Append(d1p3);
    d1.Append(d1p4);
    d1.Append(d1p5);
    d1.Append(d1p0);
    d1.Append(d1p1);

    QTest::newRow("Case 3") << d1 << d2 << indexD2 << pRotate << result;

    // The rotation point sits on the second to last node: the block goes in before the last node, and the last node
    // is the end of the seam — it stays, since the point it is one with belongs to the second piece.
    result.clear();
    result.append(qMakePair(true, d1p0));
    result.append(qMakePair(true, d1p1));
    result.append(qMakePair(true, d1p2));
    result.append(qMakePair(true, d1p3));
    result.append(qMakePair(true, d1p4));
    result.append(qMakePair(false, d2p2));
    result.append(qMakePair(false, d2p3));
    result.append(qMakePair(false, d2p4));
    result.append(qMakePair(true, d1p5));

    d1.Clear();
    d1.Append(d1p0);
    d1.Append(d1p1);
    d1.Append(d1p2);
    d1.Append(d1p3);
    d1.Append(d1p4);
    d1.Append(d1p5);

    QTest::newRow("Seam ends on the second to last node") << d1 << d2 << indexD2 << quint32(1209) << result;

    // The seam is the closing edge of the first piece: the block goes in after the very last node, and the node that
    // follows it is the first one of the contour. That is the shape of the union this was reported on.
    result.clear();
    result.append(qMakePair(true, d1p0));
    result.append(qMakePair(true, d1p1));
    result.append(qMakePair(true, d1p2));
    result.append(qMakePair(true, d1p3));
    result.append(qMakePair(true, d1p4));
    result.append(qMakePair(true, d1p5));
    result.append(qMakePair(false, d2p2));
    result.append(qMakePair(false, d2p3));
    result.append(qMakePair(false, d2p4));

    QTest::newRow("Seam closes the contour") << d1 << d2 << indexD2 << quint32(1204) << result;
}

//---------------------------------------------------------------------------------------------------------------------
void TST_VToolUnionDetails::TestUnitingMainPaths()
{
    QFETCH(VPiecePath, d1);
    QFETCH(VPiecePath, d2);
    QFETCH(quint32, indexD2);
    QFETCH(quint32, pRotate);
    QFETCH(QVector<UnitedPathNode>, result);

    const QVector<UnitedPathNode> out = VToolUnionDetails::CalcUnitedPath(d1, d2, indexD2, pRotate);

    QVERIFY(result.size() == out.size());

    // **Four points make two.** Both seam edges are in neither outline, and both their ends are named once: the
    // contour holds two nodes fewer than the sum of both outlines — always, whatever the names or the places of the
    // four points that were joined.
    QCOMPARE(out.size(), d1.CountNodes() + d2.CountNodes() - 2);

    for (int i = 0; i < out.size(); ++i)
    {
        const VPieceNode resP = result.at(i).second;
        const VPieceNode outP = out.at(i).second;

        // cppcheck-suppress unreadVariable
        const QString msg = u"Index: %1. Got item with id = %2, type = %3; Expected item with id = %4, "
                            u"type = %5."_s.arg(i)
                                .arg(outP.GetId())
                                .arg(static_cast<ToolVisHolderType>(outP.GetTypeTool()))
                                .arg(resP.GetId())
                                .arg(static_cast<ToolVisHolderType>(resP.GetTypeTool()));

        QVERIFY2(out.at(i).first == result.at(i).first && outP.GetId() == resP.GetId() &&
                     outP.GetTypeTool() == resP.GetTypeTool(),
                 qUtf8Printable(msg));
    }
}


//---------------------------------------------------------------------------------------------------------------------
void TST_VToolUnionDetails::TestFusionPoints()
{
    // **The four points the user picked become two.** The contour keeps the two points of the first piece — a point
    // belongs to a piece and shows its name — and drops the two of the second one, which give what they held
    // instead of a place: the margin of the segment that arrives at one end, of the segment that leaves at the
    // other. What they also held is a notch, and a notch on a seam that is no longer there marks nothing.
    VPieceNode b1(1205, Tool::NodePoint); // the seam ends here, at the start of the contour
    VPieceNode inside(1206, Tool::NodePoint);
    VPieceNode a1(1207, Tool::NodePoint); // the seam starts here, at the last node
    VPieceNode a2(1211, Tool::NodePoint); // the start of the second piece's edge: one point with b1
    VPieceNode b2(1212, Tool::NodePoint); // its end: one point with a1
    VPieceNode d2inside(1213, Tool::NodePoint);

    b1.SetPassmark(true);
    b1.SetPassmarkLineType(PassmarkLineType::TMark);
    a1.SetPassmark(true);
    a1.SetPassmarkLineType(PassmarkLineType::OneLine);
    inside.SetPassmark(true); // a notch inside the first piece: it has nothing to do with the seam
    a2.SetFormulaSABefore(QStringLiteral("4.5"));
    a2.SetPassmark(true);
    b2.SetFormulaSAAfter(QStringLiteral("7"));
    b2.SetPassmark(true);

    VPiecePath d1;
    d1.Append(b1);
    d1.Append(inside);
    d1.Append(a1);

    VPiecePath d2;
    d2.Append(a2);
    d2.Append(b2);
    d2.Append(d2inside);

    // The seam is the closing edge of the first piece and the first edge of the second one.
    const QVector<UnitedPathNode> out = VToolUnionDetails::CalcUnitedPath(d1, d2, 0, a1.GetId());

    QCOMPARE(out.size(), d1.CountNodes() + d2.CountNodes() - 2);
    QCOMPARE(out.at(0).second.GetId(), b1.GetId());
    QCOMPARE(out.at(1).second.GetId(), inside.GetId());
    QCOMPARE(out.at(2).second.GetId(), a1.GetId());
    QCOMPARE(out.at(3).second.GetId(), d2inside.GetId());

    // What the two points of the second piece carried.
    QCOMPARE(out.at(0).second.GetFormulaSABefore(), a2.GetFormulaSABefore());
    QCOMPARE(out.at(2).second.GetFormulaSAAfter(), b2.GetFormulaSAAfter());

    // And what they must not carry any more.
    QVERIFY(not out.at(0).second.IsPassmark());
    QVERIFY(not out.at(2).second.IsPassmark());
    QVERIFY(out.at(1).second.IsPassmark());

    // A union saved before the four points became two was written with one node more; its children are mapped on
    // the sequence as it was, and that sequence has to stay reachable.
    const QVector<UnitedPathNode> before = VToolUnionDetails::CalcUnitedPath(d1, d2, 0, a1.GetId(), false);
    QCOMPARE(before.size(), d1.CountNodes() + d2.CountNodes() - 1);
    QCOMPARE(before.constLast().second.GetId(), a2.GetId());
    QCOMPARE(out.at(0).second.GetFormulaSABefore(), a2.GetFormulaSABefore());
    QVERIFY(before.at(0).second.IsPassmark());
}
