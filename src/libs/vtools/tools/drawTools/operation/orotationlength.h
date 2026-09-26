/************************************************************************
 **
 **  @file   orotationlength.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The rule that ties the angle of a rotation to the length it draws: the chord of the
 **          reference point. It was written twice — once for the dialog field, once for the options
 **          dock — with two tolerances and two ways of refusing a length, and the two drifted apart
 **          until a half-turn was refused in one place and silently rewritten in the other.
 **          Everything here is in the unit the user reads and rounded the way a pattern displays a
 **          number, since both callers hand the result straight to a field.
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

#ifndef OROTATIONLENGTH_H
#define OROTATIONLENGTH_H

#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../vmisc/def.h"

class VContainer;

/**
 * @brief The length of a rotation, and the angle that length means.
 *
 * A rotation is a circle drawn around its pivot: it can be told either by an angle or by the chord of its
 * reference point, and the two say the same thing. The rule lives here so that the dialog field and the
 * options dock cannot answer it differently.
 *
 * No state: every method is a conversion or a test.
 */
class ORotationLength
{
public:
    /** @brief Half of the third decimal: a value a field already shows, to the precision it shows, is the
     *  same value. Comparing with that tolerance is what lets the two fields describe one rotation without
     *  rewriting what the user is typing. */
    static constexpr qreal DisplayTolerance = 0.0005;

    /** @brief The point a length is measured on: the first point among the sources of the operation, or
     *  NULL_ID when they hold none — a rotation built only from curves has no length to speak of. */
    static auto ReferencePoint(const VContainer *data, const QVector<quint32> &sourceIds) -> quint32;

    /** @brief Distance from the pivot to the reference point, in pixels. 0 as soon as one of them is gone,
     *  which is exactly what "no radius, hence no length" means. */
    static auto Radius(const VContainer *data, quint32 pivotId, quint32 referenceId) -> qreal;

    /** @brief Whether a radius can carry a length at all. */
    static auto HasRadius(qreal radius) -> bool;

    /** @brief The chord of the reference point, signed like the rotation it comes from, in the unit the user
     *  reads and rounded the way it is displayed: what an angle means as a length. */
    static auto LengthOfAngle(qreal radius, qreal angle, const Unit &unit) -> qreal;

    /** @brief What a length means as an angle, rounded the way it is displayed, or NaN when the length
     *  cannot belong to any rotation.
     *
     * A length within DisplayTolerance of the diameter is taken as the diameter: the field shows three
     * decimals, so the diameter it displays can pass the exact one, and that is the field's own precision
     * rather than a mistake to refuse. */
    static auto AngleOfLength(qreal radius, qreal length, const Unit &unit) -> qreal;

    /** @brief The circle diameter — the longest length a rotation can show — as it is displayed. */
    static auto DiameterText(qreal radius, const Unit &unit) -> QString;

private:
    Q_DISABLE_COPY_MOVE(ORotationLength) // NOLINT
    ORotationLength() = default;
};

#endif // OROTATIONLENGTH_H
