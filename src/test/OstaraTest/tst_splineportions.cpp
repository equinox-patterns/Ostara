/************************************************************************
 **
 **  @file   tst_splineportions.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  Les morceaux d'un chemin de cinq points, coupe a un, deux ou trois de ses noeuds.
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

#include "tst_splineportions.h"

#include "vtools/tools/drawTools/toolcurve/osplinepathportions.h"

#include <QtTest>

//---------------------------------------------------------------------------------------------------------------------
TST_SplinePortions::TST_SplinePortions(QObject *parent) : QObject(parent) {}

//---------------------------------------------------------------------------------------------------------------------
void TST_SplinePortions::NoCutIsOneRun()
{
    // A path nobody cut is one run, from its first node to its last: the whole curve, as it was drawn.
    const QVector<OSplinePathPortions::Run> runs = OSplinePathPortions::Runs({}, 5);

    QCOMPARE(runs.size(), qsizetype(1));
    QCOMPARE(runs.constFirst().from, 0);
    QCOMPARE(runs.constFirst().to, 4);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_SplinePortions::ThreeCutsMakeFourRuns()
{
    // Five points, three interior boxes: four pieces, each starting where the previous one ended.
    const QVector<OSplinePathPortions::Run> runs = OSplinePathPortions::Runs({1, 2, 3}, 5);

    QCOMPARE(runs.size(), qsizetype(4));

    const QVector<QPair<int, int>> expected{{0, 1}, {1, 2}, {2, 3}, {3, 4}};
    for (qsizetype index = 0; index < expected.size(); ++index)
    {
        QCOMPARE(runs.at(index).from, expected.at(index).first);
        QCOMPARE(runs.at(index).to, expected.at(index).second);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void TST_SplinePortions::EndsAreNeverCut()
{
    // A box on the first or the last node would separate an empty piece: it is ignored, never honoured.
    const QVector<OSplinePathPortions::Run> runs = OSplinePathPortions::Runs({0, 4}, 5);

    QCOMPARE(runs.size(), qsizetype(1));
    QCOMPARE(runs.constFirst().from, 0);
    QCOMPARE(runs.constFirst().to, 4);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_SplinePortions::BoxOrderDoesNotChangeTheRuns()
{
    // The boxes are a set, not a sequence: checking them in another order, or twice, gives the same four pieces.
    const QVector<OSplinePathPortions::Run> ordered = OSplinePathPortions::Runs({1, 2, 3}, 5);
    const QVector<OSplinePathPortions::Run> shuffled = OSplinePathPortions::Runs({3, 1, 2, 2}, 5);

    QCOMPARE(ordered, shuffled);
}

//---------------------------------------------------------------------------------------------------------------------
void TST_SplinePortions::RunsRoundTripThroughTheFile()
{
    // What the file says is what comes back: the nodes, and one alias per run with the empty ones kept in
    // place, so a name stays on its piece even when the one before it is unnamed.
    const QVector<int> nodes = OSplinePathPortions::DecodeNodes(OSplinePathPortions::EncodeNodes({1, 3}));
    QCOMPARE(nodes, (QVector<int>{1, 3}));

    // The id each run got from the container travels the same way, keyed by its two nodes: a reparse then finds the
    // same object instead of allocating a new one beside it. A piece kept for its users carries its name too.
    const QMap<QString, OSplinePathPortions::StoredPiece> pieces = OSplinePathPortions::DecodePieces(
        OSplinePathPortions::EncodePieces({{QStringLiteral("0-2"), {101, QString()}},
                                           {QStringLiteral("2-5"), {103, QStringLiteral("A18__p2")}}}));
    QCOMPARE(pieces.size(), qsizetype(2));
    QCOMPARE(pieces.value(QStringLiteral("0-2")).id, quint32(101));
    QVERIFY(pieces.value(QStringLiteral("0-2")).name.isEmpty());
    QCOMPARE(pieces.value(QStringLiteral("2-5")).id, quint32(103));
    QCOMPARE(pieces.value(QStringLiteral("2-5")).name, QStringLiteral("A18__p2"));

    // A pattern written before the boxes existed has neither attribute: it reads back as an uncut path.
    QVERIFY(OSplinePathPortions::DecodeNodes(QString()).isEmpty());
    QVERIFY(OSplinePathPortions::DecodePieces(QString()).isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void TST_SplinePortions::RunKeysNameBothEnds()
{
    // A run is named by its two ends, so unticking the box before it cannot hand its id to the longer run that takes
    // its place: the merged run is a new key.
    QCOMPARE(OSplinePathPortions::RunKey({2, 5}), QStringLiteral("2-5"));

    OSplinePathPortions::Run run;
    QVERIFY(OSplinePathPortions::ParseRunKey(QStringLiteral("2-5"), run));
    QCOMPARE(run.from, 2);
    QCOMPARE(run.to, 5);

    // The key of a cut half, and keys that describe no run, are not runs.
    QVERIFY(not OSplinePathPortions::ParseRunKey(QStringLiteral("c90-1"), run));
    QVERIFY(not OSplinePathPortions::ParseRunKey(QStringLiteral("5"), run));
    QVERIFY(not OSplinePathPortions::ParseRunKey(QStringLiteral("5-2"), run));

    const QVector<OSplinePathPortions::Run> before = OSplinePathPortions::Runs({1, 3}, 6);
    const QVector<OSplinePathPortions::Run> after = OSplinePathPortions::Runs({3}, 6);
    QCOMPARE(OSplinePathPortions::RunKey(before.at(1)), QStringLiteral("1-3"));
    QCOMPARE(OSplinePathPortions::RunKey(after.at(0)), QStringLiteral("0-3"));
}

//---------------------------------------------------------------------------------------------------------------------
void TST_SplinePortions::LegacyIdsAreStillRead()
{
    // A file written before the runs had two ends keyed them by the node they end on, `3=102`: read as the legacy key
    // the tool matches against its runs once.
    const QMap<QString, OSplinePathPortions::StoredPiece> pieces =
        OSplinePathPortions::DecodePieces(QStringLiteral("3=102;5=103"));
    QCOMPARE(pieces.value(OSplinePathPortions::LegacyRunKey(3)).id, quint32(102));
    QCOMPARE(pieces.value(OSplinePathPortions::LegacyRunKey(5)).id, quint32(103));

    // Anything that is not an id is left out rather than read as one.
    QVERIFY(OSplinePathPortions::DecodePieces(QStringLiteral("0-2=;x;=7")).isEmpty());
}

QTEST_MAIN(TST_SplinePortions)
