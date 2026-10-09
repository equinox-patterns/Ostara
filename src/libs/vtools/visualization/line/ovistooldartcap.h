/************************************************************************
 **
 **  @file   ovisooldartcap.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  preview of the cap of a dart
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

#ifndef OVISTOOLDARTCAP_H
#define OVISTOOLDARTCAP_H

#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "visline.h"

class VScaledEllipse;

/**
 * @brief The preview of a dart cap, drawn as the user picks the points: the dart itself (its axis), the line of
 * the pattern the cap is read on, and — as soon as the construction exists — the cap point and the two tracés
 * that go from it to the opening, green for C→P1 and red for C→P2, the colours the dock shows on their names.
 *
 * The line of the pattern is not clicked any more: it is the edge of the piece, read by OEdgeReader where the
 * sense says, and given here as a direction — the preview has no pattern of its own to ask. A point the user
 * designated stays the way out, and the edge is then the line from it to the nearer end of the opening.
 */
class OVisToolDartCap : public VisLine
{
    Q_OBJECT // NOLINT

public:
    explicit OVisToolDartCap(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolDartCap() override = default;

    void RefreshGeometry() override;
    void VisualMode(quint32 id) override;

    void SetApexId(quint32 value);
    void SetFirstPointId(quint32 value);
    void SetSecondPointId(quint32 value);
    void SetExtendPointId(quint32 value);

    /** @brief The point C the tool made, when the figure carries one: the axis of the dart runs from its apex to it.
     *
     *  Null while the cap is only being placed — the dialog shows the aids of the placement, and there is no point C
     *  yet — and the axis then stops at the middle of the opening, where the fold line crosses it. */
    void SetCapPointId(quint32 value);

    //! Whether the cap follows the edge read at the second point of the opening: the sense of the cap.
    void SetSecondSense(bool value);

    //! The direction of the edge read at that end, and nothing when none could be read.
    void SetEdgeDirection(const QPointF &value);

    //! Forget the points: nothing is left on the scene.
    void Clear();

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolDartCap)
    };

private:
    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(OVisToolDartCap) // NOLINT

    quint32 m_apexId{NULL_ID};
    quint32 m_firstPointId{NULL_ID};
    quint32 m_secondPointId{NULL_ID};
    quint32 m_extendPointId{NULL_ID};
    quint32 m_capPointId{NULL_ID};

    bool m_secondSense{false};
    QPointF m_edgeDirection{};

    VScaledEllipse *m_apex{nullptr};
    VScaledEllipse *m_firstPoint{nullptr};
    VScaledEllipse *m_secondPoint{nullptr};
    VScaledEllipse *m_extendPoint{nullptr};

    /** @brief m_axis the axis of the dart, apex → middle of the opening. Drawn on `this`, which is a line. */
    VScaledLine *m_axis{nullptr};

    /** @brief m_extendLine the line of the pattern the cap point is looked for on, dotted: it is the one the
     *  user must imagine prolonged. */
    VScaledLine *m_extendLine{nullptr};

    VScaledLine *m_capLine1{nullptr};
    VScaledLine *m_capLine2{nullptr};
    VScaledEllipse *m_capPoint{nullptr};
};

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolDartCap::SetApexId(quint32 value)
{
    m_apexId = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolDartCap::SetFirstPointId(quint32 value)
{
    m_firstPointId = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolDartCap::SetSecondPointId(quint32 value)
{
    m_secondPointId = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolDartCap::SetExtendPointId(quint32 value)
{
    m_extendPointId = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolDartCap::SetCapPointId(quint32 value)
{
    m_capPointId = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolDartCap::SetSecondSense(bool value)
{
    m_secondSense = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolDartCap::SetEdgeDirection(const QPointF &value)
{
    m_edgeDirection = value;
}

#endif // OVISTOOLDARTCAP_H
