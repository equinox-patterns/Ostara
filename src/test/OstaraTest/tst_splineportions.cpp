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

    // The id each run got from the container travels the same way, keyed by the node the run ends on: a reparse
    // then finds the same object instead of allocating a new one beside it.
    const QMap<int, quint32> ids
        = OSplinePathPortions::DecodeIds(OSplinePathPortions::EncodeIds({{2, 101}, {4, 103}}));
    QCOMPARE(ids.size(), qsizetype(2));
    QCOMPARE(ids.value(2), quint32(101));
    QCOMPARE(ids.value(4), quint32(103));

    // A pattern written before the boxes existed has neither attribute: it reads back as an uncut path.
    QVERIFY(OSplinePathPortions::DecodeNodes(QString()).isEmpty());
    QVERIFY(OSplinePathPortions::DecodeIds(QString()).isEmpty());
}

QTEST_MAIN(TST_SplinePortions)
