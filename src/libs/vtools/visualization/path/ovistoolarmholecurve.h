/************************************************************************
 **
 **  @file   ovistoolarmholecurve.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/08/2026
 **
 **  @brief  Visualization for the ArmholeCurve tool — progressive preview
 **          during 4-point selection with auto-computed handles.
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

#ifndef OVISTOOLARMHOLECURVE_H
#define OVISTOOLARMHOLECURVE_H

#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QtGlobal>

#include "../vgeometry/vcubicbezierpath.h"
#include "../vmisc/def.h"
#include "vispath.h"

class OVisToolArmholeCurve : public VisPath
{
    Q_OBJECT // NOLINT
public:
    explicit OVisToolArmholeCurve(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolArmholeCurve() override;

    void RefreshGeometry() override;
    void VisualMode(quint32 id) override;

    void SetPoint1Id(quint32 value); // armhole
    void SetPoint2Id(quint32 value); // chest/back width
    void SetPoint3Id(quint32 value); // perpendicular
    void SetPoint4Id(quint32 value); // shoulder

    /// The final curve drawn in Show mode. It must be the STORED path: recomputing it from the
    /// 4 points would be wrong when the perpendicular was skipped.
    void SetPath(const VCubicBezierPath &path);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolArmholeCurve)
    };

private:
    Q_DISABLE_COPY_MOVE(OVisToolArmholeCurve) // NOLINT
    VCubicBezierPath m_path{};
    bool m_hasPath{false};
    quint32 m_point1Id{NULL_ID}; // armhole
    quint32 m_point2Id{NULL_ID}; // chest/back width
    quint32 m_point3Id{NULL_ID}; // perpendicular
    quint32 m_point4Id{NULL_ID}; // shoulder
    VScaledEllipse *m_point1{nullptr};
    VScaledEllipse *m_point2{nullptr};
    VScaledEllipse *m_point3{nullptr};
    VScaledEllipse *m_point4{nullptr};
    VScaledLine *m_helpLine1{nullptr}; // armhole → chest/back width (or cursor)
    VScaledLine *m_helpLine2{nullptr}; // chest/back width → perpendicular (or cursor)
};

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolArmholeCurve::SetPoint1Id(quint32 value)
{
    m_point1Id = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolArmholeCurve::SetPoint2Id(quint32 value)
{
    m_point2Id = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolArmholeCurve::SetPoint3Id(quint32 value)
{
    m_point3Id = value;
}

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolArmholeCurve::SetPoint4Id(quint32 value)
{
    m_point4Id = value;
}

#endif // OVISTOOLARMHOLECURVE_H
