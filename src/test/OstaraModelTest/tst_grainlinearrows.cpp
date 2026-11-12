/************************************************************************
 **
 **  @file   tst_grainlinearrows.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/11/2026
 **
 **  @brief  The twelve grainline arrow states and the six buttons the options dock offers
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

#include "tst_grainlinearrows.h"
#include "vpatterndb/floatItemData/floatitemdef.h"

#include <QtTest>

#include <QSet>

namespace
{
auto AsInt(GrainlineArrowDirection arrowType) -> int
{
    return static_cast<int>(arrowType);
}

auto AsInt(GrainlineEnds ends) -> int
{
    return static_cast<int>(ends);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
TST_GrainlineArrows::TST_GrainlineArrows(QObject *parent)
  : QObject(parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
// Every value a file can hold must come back unchanged through the dock's two three-way choices.
// A value that survives decoding but not re-encoding is a state the panel would silently rewrite.
void TST_GrainlineArrows::TestEveryValueSurvivesADecode()
{
    const QList<GrainlineArrowDirection> everyValue = {
        GrainlineArrowDirection::twoWaysUpDown,
        GrainlineArrowDirection::oneWayUp,
        GrainlineArrowDirection::oneWayDown,
        GrainlineArrowDirection::fourWays,
        GrainlineArrowDirection::twoWaysUpLeft,
        GrainlineArrowDirection::twoWaysUpRight,
        GrainlineArrowDirection::twoWaysDownLeft,
        GrainlineArrowDirection::twoWaysDownRight,
        GrainlineArrowDirection::threeWaysUpDownLeft,
        GrainlineArrowDirection::threeWaysUpDownRight,
        GrainlineArrowDirection::threeWaysUpLeftRight,
        GrainlineArrowDirection::threeWaysDownLeftRight,
    };

    QCOMPARE(everyValue.size(), 12);

    for (const GrainlineArrowDirection arrowType : everyValue)
    {
        const GrainlineArrows arrows = DecodeGrainlineArrows(arrowType);
        QCOMPARE(AsInt(EncodeGrainlineArrows(arrows)), AsInt(arrowType));
    }
}

//---------------------------------------------------------------------------------------------------------
// The other direction: the six buttons give twelve states, no two of them encoded the same, and each
// one reads back as itself. This is what guarantees the panel can reach every existing value.
void TST_GrainlineArrows::TestEveryCombinationIsDistinct()
{
    const GrainlineEnds everyEnds[] = {GrainlineEnds::Both, GrainlineEnds::First, GrainlineEnds::Second};
    QSet<int> seen;

    for (const bool crossed : {false, true})
    {
        for (const GrainlineEnds main : everyEnds)
        {
            for (const GrainlineEnds cross : everyEnds)
            {
                // A single line has no crossed line, so only one of its three choices exists.
                if (!crossed && cross != GrainlineEnds::Both)
                {
                    continue;
                }

                GrainlineArrows arrows;
                arrows.crossed = crossed;
                arrows.main = main;
                arrows.cross = cross;

                const int value = AsInt(EncodeGrainlineArrows(arrows));
                QVERIFY2(!seen.contains(value), "two button states encode to the same arrow type");
                seen.insert(value);

                const GrainlineArrows back = DecodeGrainlineArrows(static_cast<GrainlineArrowDirection>(value));
                QVERIFY(back.crossed == crossed);
                QCOMPARE(AsInt(back.main), AsInt(main));
                QCOMPARE(AsInt(back.cross), AsInt(cross));
            }
        }
    }

    QCOMPARE(seen.size(), 12);
}

//---------------------------------------------------------------------------------------------------------
// The point of the feature: one arrow instead of two, at the end the user asked for.
void TST_GrainlineArrows::TestOneArrowUsesTheRightEnd()
{
    const GrainlineArrows up = DecodeGrainlineArrows(GrainlineArrowDirection::oneWayUp);
    QVERIFY(!up.crossed);
    QCOMPARE(AsInt(up.main), AsInt(GrainlineEnds::Second));

    const GrainlineArrows down = DecodeGrainlineArrows(GrainlineArrowDirection::oneWayDown);
    QVERIFY(!down.crossed);
    QCOMPARE(AsInt(down.main), AsInt(GrainlineEnds::First));

    const GrainlineArrows both = DecodeGrainlineArrows(GrainlineArrowDirection::twoWaysUpDown);
    QVERIFY(!both.crossed);
    QCOMPARE(AsInt(both.main), AsInt(GrainlineEnds::Both));

    // The crossed line is drawn only by the cross values: the plain ones must not show it.
    const GrainlineArrows crossed = DecodeGrainlineArrows(GrainlineArrowDirection::fourWays);
    QVERIFY(crossed.crossed);
    QCOMPARE(AsInt(crossed.main), AsInt(GrainlineEnds::Both));
    QCOMPARE(AsInt(crossed.cross), AsInt(GrainlineEnds::Both));
}

//---------------------------------------------------------------------------------------------------------
// The parser casts the attribute without validating it, so a hand-edited file can carry 12 or more.
void TST_GrainlineArrows::TestUnknownValueFallsBackToThePlainGrainline()
{
    const GrainlineArrows arrows = DecodeGrainlineArrows(static_cast<GrainlineArrowDirection>(42));
    QVERIFY(!arrows.crossed);
    QCOMPARE(AsInt(arrows.main), AsInt(GrainlineEnds::Both));
    QCOMPARE(AsInt(EncodeGrainlineArrows(arrows)), AsInt(GrainlineArrowDirection::twoWaysUpDown));
}
