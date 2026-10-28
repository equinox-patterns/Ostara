/************************************************************************
 **
 **  @file   omovebatchrule.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/10/2026
 **
 **  @brief  The rule of a move by batch: which move carries each object, and around which pivot it turns.
 **
 **          A move by batch is a chain of moves. Each move is a "cut line" from a start point (the hem, where
 **          the part opens) up to a pivot (the point the part turns around). The objects between the start and
 **          the pivot of a move, in the order the user picked them, travel with that move; an object between the
 **          starts and pivots of several moves travels with all of them, in order.
 **
 **          Unlike a slash-and-spread, the region is closed at BOTH ends: the pivot travels too. A rotation has
 **          to leave its pivot behind -- it is the hinge -- but a translation has no fixed point at all, so
 **          nothing has to stay put, and a cut line belongs to the part it opens.
 **
 **          Each move is the pair of things the Move tool does: a translation of a length along an angle, then a
 **          rotation of an angle around the pivot. The pivot is not a fixed point of the whole chain -- it stays
 **          put in its own move but is carried by every earlier one, so its position for move K is its original
 **          position after moves 1..K-1. The rule reads that position once per move and hands the result to the
 **          transform, which only moves objects.
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

#ifndef OMOVEBATCHRULE_H
#define OMOVEBATCHRULE_H

#include <QPointF>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../toolsdef.h"

class VContainer;

/**
 * @brief One move of a batch, as the user wrote it.
 *
 * @a startId and @a pivotId are points of the ordered source list: they delimit the region of the move -- the
 * objects from the start to the pivot, both INCLUDED, travel with it. They are stored by id and resolved to
 * their position in the list by the rule, so the pair can be picked in any order: the role follows the
 * position, not the click. The pivot is also the centre the move turns its part around.
 *
 * @a length, @a angle and @a rotationAngle are the formulas as the user typed them; the tool evaluates them to
 * numbers before the rule reads them.
 */
struct OMoveBatchMove
{
    quint32 startId{NULL_ID};
    quint32 pivotId{NULL_ID};
    QString name{};
    QString length{};
    QString angle{};
    QString rotationAngle{};
};

/**
 * @brief One applied move for one object: translate by @a length along @a angle, then turn by @a rotationAngle
 * degrees around @a pivot (pixels).
 */
struct OMoveBatchStep
{
    QPointF pivot{};
    qreal length{0.};
    qreal angle{0.};
    qreal rotationAngle{0.};
};

/**
 * @brief The rule of a move by batch.
 *
 * No state: every method is a reading or a computation, and none of them touches the pattern.
 */
class OMoveBatchRule
{
public:
    /** @brief The length a move is built with when the pattern does not give one. */
    static const QString DefaultLength;

    /** @brief The angle a move is built with when the pattern does not give one. */
    static const QString DefaultAngle;

    /** @brief The rotation angle a move is built with when the pattern does not give one. */
    static const QString DefaultRotationAngle;

    /** @brief The name of the K-th move, for a dialog that has not read a name from the pattern. */
    static auto DefaultMoveName(int index) -> QString;

    /** @brief Position of @a id in the ordered source list, -1 when it is not there. */
    static auto IndexOf(const QVector<SourceItem> &source, quint32 id) -> int;

    /** @brief The moves every source object goes through, in source order: result[i] is the ordered list of
     *  steps for source[i], empty for an object no move touches.
     *
     *  @a lengths, @a angles and @a rotationAngles are aligned with @a moves: the evaluated value of each, in
     *  pixels and degrees. A move whose start or pivot is not in the source list, or whose pivot is gone from
     *  the container, is skipped: its two points describe no region to move. */
    static auto Compose(const QVector<SourceItem> &source, const QVector<OMoveBatchMove> &moves,
                        const QVector<qreal> &lengths, const QVector<qreal> &angles,
                        const QVector<qreal> &rotationAngles, const VContainer *data)
        -> QVector<QVector<OMoveBatchStep>>;

    /** @brief One state of one point: the point, and how many moves it has been through at that state. */
    struct Intermediate
    {
        quint32 sourceId{NULL_ID}; //!< the point whose state is materialized
        int state{0};              //!< the number of moves it has been through there
    };

    /** @brief The intermediate states the points of the chain need, in source order.
     *
     *  A point a portion of the contour stops on must exist at the state that portion stops at. A cut line
     *  divides the chain, so the object before it and the object after it do not travel the same number of
     *  times: the point is the end of one of them, and the copy of that one stops beside a point already
     *  further away — the move is then incomplete (a curve with no point at its end), and no later state can
     *  close it. So a point receives one copy more than its final one, at each state where a neighbour of the
     *  ordered chain stops: `<source>__M<state>`, the final copy keeping `<source>__M`.
     *
     *  The counts do not depend on the VALUES of the moves — only on which regions hold which object — which
     *  is why the caller passes none. The neighbours are the previous and the next object of the ordered
     *  selection: that order is the contour, and it is the contract of the whole tool. */
    static auto IntermediateStates(const QVector<SourceItem> &source, const QVector<OMoveBatchMove> &moves)
        -> QVector<Intermediate>;

private:
    Q_DISABLE_COPY_MOVE(OMoveBatchRule) // NOLINT
    OMoveBatchRule() = default;
};

#endif // OMOVEBATCHRULE_H
