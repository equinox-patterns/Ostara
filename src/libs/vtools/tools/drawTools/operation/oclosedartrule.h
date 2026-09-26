/************************************************************************
 **
 **  @file   oclosedartrule.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The rule that closes a dart: the rotation one leg has to travel to land on the other one, and
 **          the share of that rotation the user asks for.
 **          A dart is an isosceles triangle P1-P-P2, so the angle between its two legs is what has to be
 **          turned around P to bring P2 onto P1 — the very angle the formula AngleLine_P_P1 - AngleLine_P_P2
 **          gives, but read from the three points, so that the legs do not have to exist as lines in the
 **          pattern. A dart is not always closed all the way: dividing that angle leaves the legs apart,
 **          which is what a partly closed dart looks like.
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

#ifndef OCLOSEDARTRULE_H
#define OCLOSEDARTRULE_H

#include <QPointF>
#include <QString>
#include <QtGlobal>

/**
 * @brief The angle that closes a dart, and whether such an angle exists at all.
 *
 * The tool that closes a dart asks for three points rather than for an angle: where the rotation turns (the
 * pivot, the apex of the dart), which point travels, and which point it lands on. The angle follows from them,
 * so the user never types the difference of two line angles — and the two legs do not have to be drawn.
 *
 * The angle is signed, and a negative one is not a mistake: it only says the dart opens the other way round.
 *
 * No state: every method is a reading or a test, and none of them touches the pattern.
 */
class OCloseDartRule
{
public:
    /** @brief The divisor a dart is closed with when the user asks for nothing: 1, the whole opening, which
     *  is a dart closed all the way. Offered as a formula, the way every value of a tool is held. */
    static auto DefaultDivisorFormula() -> QString;

    /** @brief Direction of the line going from @a from to @a to, in degrees, counter clockwise, 0° at three
     *  o'clock — the reading of the formula variable AngleLine_<A>_<B>. */
    static auto LineAngle(const QPointF &from, const QPointF &to) -> qreal;

    /** @brief Whether the three points give two directions from the pivot and a point that has somewhere to go,
     *  which is what an angle is made of.
     *
     * False as soon as the pivot sits on one of the other two points — a direction with no length has no angle
     * to speak of, and Qt answers 0 for it instead of refusing, which would turn a misclick into a silent
     * rotation — and false as soon as the two leg points are the same point, which leaves nothing to close. */
    static auto IsDefined(const QPointF &pivot, const QPointF &moving, const QPointF &target) -> bool;

    /** @brief Whether a divisor can divide an angle at all: zero and infinity cannot. */
    static auto IsDivisorUsable(qreal divisor) -> bool;

    /** @brief The signed angle, in degrees, that brings @a moving onto @a target once @a divisor of the
     *  opening has been travelled — 1 for a dart closed all the way, 2 for half of it.
     *
     * NaN when the angle does not exist (see IsDefined) or cannot be divided (see IsDivisorUsable), so that no
     * caller can turn an impossible dart into a rotation without noticing. The result is brought back into
     * ]-180, 180], like every other rotation angle of the application. */
    static auto ClosingAngle(const QPointF &pivot, const QPointF &moving, const QPointF &target, qreal divisor)
        -> qreal;

private:
    Q_DISABLE_COPY_MOVE(OCloseDartRule) // NOLINT
    OCloseDartRule() = default;
};

#endif // OCLOSEDARTRULE_H
