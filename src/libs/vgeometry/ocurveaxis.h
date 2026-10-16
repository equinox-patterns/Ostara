/************************************************************************
 **
 **  @file   ocurveaxis.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/10/2026
 **
 **  @brief  The axis a curve handle takes its angle from.
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

#ifndef OCURVEAXIS_H
#define OCURVEAXIS_H

#include <QPointF>
#include <QString>
#include <QTypeInfo>
#include <QtGlobal>

#include "../vmisc/typedef.h"

/**
 * @brief The four directions a curve handle can take from its axis.
 *
 * Flipping the sense and flipping the handle are the same 180 degrees, so four states are enough for the two
 * handles of a path point: there is no need for a per-handle sense.
 */
enum class CurveAxisSense : quint8
{
    Line = 0,
    LineReverse,
    Perpendicular,
    PerpendicularReverse
};

/**
 * @brief The axis a curve handle takes its angle from: two points, and one of four directions.
 *
 * The axis is held as two point identifiers, never as a formula token: it is a dependency, not a name, so it
 * needs no alias and adds nothing to the variables table. The angle itself is computed and stored as a plain
 * number, which keeps the file readable and the handle usable by everything that reads an angle.
 */
class OCurveAxis
{
public:
    OCurveAxis() = default;
    OCurveAxis(quint32 p1, quint32 p2, CurveAxisSense sense);

    auto P1() const -> quint32;
    auto P2() const -> quint32;
    auto Sense() const -> CurveAxisSense;
    void SetP1(quint32 id);
    void SetP2(quint32 id);
    void SetSense(CurveAxisSense sense);
    void Set(quint32 p1, quint32 p2, CurveAxisSense sense);
    void Clear();

    /** @brief An axis is usable only when both of its points are known. */
    auto IsValid() const -> bool;

    /**
     * @brief The angle, in degrees 0-360, the handle takes from the axis A->B.
     * @param p1 coordinates of the point P1()
     * @param p2 coordinates of the point P2()
     */
    auto HandleAngle(const QPointF &p1, const QPointF &p2) const -> qreal;

    /** @brief The opposite angle, in the same 0-360 range. 350 degrees comes back as 170, not 530. */
    static auto OppositeAngle(qreal angle) -> qreal;

    auto SenseName() const -> QString;
    static auto SenseFromName(const QString &name) -> CurveAxisSense;

private:
    quint32 m_p1{NULL_ID};
    quint32 m_p2{NULL_ID};
    CurveAxisSense m_sense{CurveAxisSense::Line};
};

Q_DECLARE_TYPEINFO(OCurveAxis, Q_MOVABLE_TYPE); // NOLINT

/**
 * @brief The axis to keep when an angle is written by hand: none as soon as the formula changes.
 *
 * The angle is either taken from the axis or written in the field, never composed of the two — so a formula the
 * user replaced abandons the delegation, while a field merely shown and left alone keeps it. One rule for the
 * three places that can give an axis up: the Options dock, the curve dialog and the path dialog.
 *
 * @param axis the axis the handle carries now
 * @param before the formula the tool held
 * @param after the formula the field holds
 */
inline auto AxisAfterHandWrittenAngle(const OCurveAxis &axis, const QString &before, const QString &after)
    -> OCurveAxis
{
    return before == after ? axis : OCurveAxis();
}

#endif // OCURVEAXIS_H
