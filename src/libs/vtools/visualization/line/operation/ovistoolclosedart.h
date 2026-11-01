/************************************************************************
 **
 **  @file   ovistoolclosedart.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The preview of the tool that closes a dart.
 **
 **          The ghost it draws is the ghost of a rotation — the same objects, turned around the same pivot —
 **          and the code that builds it is inherited from VisToolRotation. Only the angle changes hands: a
 **          typed field there, the three points of the dart here. While the point the dart closes onto is
 **          still being chosen, the cursor stands in for it: the ghost closes and opens with the mouse, and
 **          the click then lands on a real point and makes the angle exact.
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

#ifndef OVISTOOLCLOSEDART_H
#define OVISTOOLCLOSEDART_H

#include <QGraphicsItem>
#include <QtGlobal>

#include "vistoolrotation.h"

class OVisToolCloseDart : public VisToolRotation
{
    Q_OBJECT // NOLINT

public:
    explicit OVisToolCloseDart(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolCloseDart() override = default;

    void RefreshGeometry() override;

    /** @brief The point of the dart that travels, NULL_ID until it has been chosen. */
    void SetMovingPointId(quint32 value);

    /** @brief The point of the dart the moving one lands on, NULL_ID while the cursor stands in for it. */
    void SetTargetPointId(quint32 value);

    /** @brief How much of the opening is closed: 1 all the way, 2 half of it. */
    void SetDivisor(qreal value);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolCloseDart)
    };

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OVisToolCloseDart) // NOLINT

    quint32 m_movingPointId{NULL_ID};
    quint32 m_targetPointId{NULL_ID};
    qreal m_divisor{1.};

    /** @brief Draw the pivot with the operated objects, in their own colour.
     *
     *  The pivot is never an operated object: the dialogue takes it out of the list, because it is what the
     *  others turn around. It is still the point the reader looks for — the one that says what the dart is
     *  closed around — so it is echoed along with them. That covers the whole gesture (from the click that
     *  picks it, while the two other points are chosen) and the pattern itself, once the tool is selected.
     *
     *  @param iPoint index of the last point taken from the pool; the pivot takes the next one, so the ghost
     *  drawn afterwards never lands on it. */
    void DrawPivotEcho(int &iPoint);

    /** @brief The angle the dart is closed with as far as it is known: NaN while the pivot or the moving
     *  point is missing, or when the divisor cannot divide. The cursor stands for the target until the user
     *  chooses it. */
    auto ClosingAngle() const -> qreal;
};

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolCloseDart::SetMovingPointId(quint32 value)
{
    m_movingPointId = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolCloseDart::SetTargetPointId(quint32 value)
{
    m_targetPointId = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolCloseDart::SetDivisor(qreal value)
{
    m_divisor = value;
}

#endif // OVISTOOLCLOSEDART_H
