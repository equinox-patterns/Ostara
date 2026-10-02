/************************************************************************
 **
 **  @file   ospreadrule.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The rule of a slash-and-spread: which rotation turns each object, and around which pivot.
 **
 **          A slash-and-spread is a chain of rotations. Each rotation is a "cut line" from a start point
 **          (the hem, where the spread opens) up to a pivot (the apex the piece turns around). The objects
 **          between the start and the pivot of a rotation, in the order the user picked them, travel with
 **          that rotation; an object between the starts and pivots of several rotations travels with all of
 **          them, each one around its own pivot, in order.
 **
 **          The pivot of a rotation is not a fixed point of the whole chain: it stays put in its own rotation
 **          but is carried by every earlier one. Its position for rotation K is therefore its original
 **          position turned by rotations 1..K-1. The rule reads that position once per rotation and hands the
 **          result to the transform, which only turns objects.
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

#ifndef OSPREADRULE_H
#define OSPREADRULE_H

#include <QPointF>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../toolsdef.h"

class VContainer;

/**
 * @brief One rotation of a slash-and-spread, as the user wrote it.
 *
 * @a startId and @a pivotId are points of the ordered source list: the start (départ) is the hem point the
 * spread opens at, the pivot the apex the piece turns around. They are stored by id and resolved to their
 * position in the list by the rule, so the pair can be picked in any order: the role follows the position,
 * not the click.
 *
 * @a angle is the formula as the user typed it; it is evaluated to degrees by the tool before the rule reads it.
 */
struct OSpreadRotation
{
    quint32 startId{NULL_ID};
    quint32 pivotId{NULL_ID};
    QString name{};
    QString angle{};
};

/**
 * @brief One applied rotation for one object: turn by @a angle degrees around @a pivot (pixels).
 */
struct OSpreadStep
{
    QPointF pivot{};
    qreal angle{0.};
};

/**
 * @brief The rule of a slash-and-spread.
 *
 * No state: every method is a reading or a computation, and none of them touches the pattern.
 */
class OSpreadRule
{
public:
    /** @brief The angle a rotation is built with when the pattern does not give one. */
    static const QString DefaultAngle;

    /** @brief The name of the K-th rotation, for a dialog that has not read a name from the pattern. */
    static auto DefaultRotationName(int index) -> QString;

    /** @brief Position of @a id in the ordered source list, -1 when it is not there. */
    static auto IndexOf(const QVector<SourceItem> &source, quint32 id) -> int;

    /** @brief The rotation steps every source object goes through, in source order: result[i] is the ordered
     *  list of steps for source[i], empty for an object no rotation touches.
     *
     *  @a anglesDeg is aligned with @a rotations: the evaluated angle of each, in degrees. A rotation whose
     *  start or pivot is not in the source list, or whose pivot is gone from the container, is skipped: its
     *  two points describe no region to turn. */
    static auto Compose(const QVector<SourceItem> &source, const QVector<OSpreadRotation> &rotations,
                        const QVector<qreal> &anglesDeg, const VContainer *data) -> QVector<QVector<OSpreadStep>>;

private:
    Q_DISABLE_COPY_MOVE(OSpreadRule) // NOLINT
    OSpreadRule() = default;
};

#endif // OSPREADRULE_H
