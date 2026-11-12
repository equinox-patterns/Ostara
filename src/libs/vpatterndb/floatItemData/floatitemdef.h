/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   23 2, 2017
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

#ifndef FLOATITEMDEF_H
#define FLOATITEMDEF_H

#include <QtGlobal>

// denotes the type of arrow for the grainline
enum class GrainlineArrowDirection : qint8
{
    twoWaysUpDown = 0,
    oneWayUp = 1,
    oneWayDown = 2,
    fourWays = 3,
    twoWaysUpLeft = 4,
    twoWaysUpRight = 5,
    twoWaysDownLeft = 6,
    twoWaysDownRight = 7,
    threeWaysUpDownLeft = 8,
    threeWaysUpDownRight = 9,
    threeWaysUpLeftRight = 10,
    threeWaysDownLeftRight = 11
};

// Which end(s) of ONE grainline line carry an arrowhead. First is the line's p1, Second its p2:
// on the main line that is bottom/top, on the crossed line left/right.
enum class GrainlineEnds : qint8
{
    Both = 0,
    First = 1,
    Second = 2
};

// The three choices the options dock offers, together: the line count (one line or the crossed
// one) and, per line, which ends carry an arrowhead. Twelve combinations exist and they are
// exactly the twelve GrainlineArrowDirection values — this is the single place that translates
// between the two, so the widget never re-implements the table.
struct GrainlineArrows
{
    bool crossed{false};
    GrainlineEnds main{GrainlineEnds::Both};
    GrainlineEnds cross{GrainlineEnds::Both};
};

inline auto EncodeGrainlineArrows(const GrainlineArrows &arrows) -> GrainlineArrowDirection
{
    const bool mainBoth = arrows.main == GrainlineEnds::Both;
    const bool mainFirst = arrows.main == GrainlineEnds::First;

    if (!arrows.crossed)
    {
        if (mainBoth)
        {
            return GrainlineArrowDirection::twoWaysUpDown;
        }
        return mainFirst ? GrainlineArrowDirection::oneWayDown : GrainlineArrowDirection::oneWayUp;
    }

    const bool crossBoth = arrows.cross == GrainlineEnds::Both;
    const bool crossFirst = arrows.cross == GrainlineEnds::First;

    if (mainBoth)
    {
        if (crossBoth)
        {
            return GrainlineArrowDirection::fourWays;
        }
        return crossFirst ? GrainlineArrowDirection::threeWaysUpDownLeft
                          : GrainlineArrowDirection::threeWaysUpDownRight;
    }
    if (mainFirst)
    {
        if (crossBoth)
        {
            return GrainlineArrowDirection::threeWaysDownLeftRight;
        }
        return crossFirst ? GrainlineArrowDirection::twoWaysDownLeft : GrainlineArrowDirection::twoWaysDownRight;
    }
    if (crossBoth)
    {
        return GrainlineArrowDirection::threeWaysUpLeftRight;
    }
    return crossFirst ? GrainlineArrowDirection::twoWaysUpLeft : GrainlineArrowDirection::twoWaysUpRight;
}

inline auto DecodeGrainlineArrows(GrainlineArrowDirection arrowType) -> GrainlineArrows
{
    switch (arrowType)
    {
        case GrainlineArrowDirection::twoWaysUpDown:
            return {false, GrainlineEnds::Both, GrainlineEnds::Both};
        case GrainlineArrowDirection::oneWayUp:
            return {false, GrainlineEnds::Second, GrainlineEnds::Both};
        case GrainlineArrowDirection::oneWayDown:
            return {false, GrainlineEnds::First, GrainlineEnds::Both};
        case GrainlineArrowDirection::fourWays:
            return {true, GrainlineEnds::Both, GrainlineEnds::Both};
        case GrainlineArrowDirection::twoWaysUpLeft:
            return {true, GrainlineEnds::Second, GrainlineEnds::First};
        case GrainlineArrowDirection::twoWaysUpRight:
            return {true, GrainlineEnds::Second, GrainlineEnds::Second};
        case GrainlineArrowDirection::twoWaysDownLeft:
            return {true, GrainlineEnds::First, GrainlineEnds::First};
        case GrainlineArrowDirection::twoWaysDownRight:
            return {true, GrainlineEnds::First, GrainlineEnds::Second};
        case GrainlineArrowDirection::threeWaysUpDownLeft:
            return {true, GrainlineEnds::Both, GrainlineEnds::First};
        case GrainlineArrowDirection::threeWaysUpDownRight:
            return {true, GrainlineEnds::Both, GrainlineEnds::Second};
        case GrainlineArrowDirection::threeWaysUpLeftRight:
            return {true, GrainlineEnds::Second, GrainlineEnds::Both};
        case GrainlineArrowDirection::threeWaysDownLeftRight:
            return {true, GrainlineEnds::First, GrainlineEnds::Both};
    }
    // A value outside 0-11 (hand-edited file, future version) falls back to the plain grainline
    // rather than leaving the dock on an undefined state.
    return {false, GrainlineEnds::Both, GrainlineEnds::Both};
}

#endif // FLOATITEMDEF_H

