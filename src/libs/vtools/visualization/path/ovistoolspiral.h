/************************************************************************
 **
 **  @file   ovistoolspiral.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Visualization for the Spiral tool: marks the point the spiral
 **          starts at while it is being created, and draws the curve that
 **          was built.
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

#ifndef OVISTOOLSPIRAL_H
#define OVISTOOLSPIRAL_H

#include <QGraphicsItem>
#include <QMetaObject>
#include <QObject>
#include <QtGlobal>

#include "../vmisc/def.h"
#include "../vgeometry/vsplinepath.h"
#include "vispath.h"

class VScaledEllipse;

class OVisToolSpiral : public VisPath
{
    Q_OBJECT // NOLINT

public:
    explicit OVisToolSpiral(const VContainer *data, QGraphicsItem *parent = nullptr);
    ~OVisToolSpiral() override;

    void RefreshGeometry() override;
    void VisualMode(quint32 id) override;

    void SetStartPointId(quint32 value);

    /// The curve as it was built. There is no law to recompute here: the length the user asked for is solved
    /// inside the geometry, so the visualization only ever shows the stored result.
    void SetPath(const VSplinePath &path);

    auto type() const -> int override { return Type; }
    enum
    {
        Type = UserType + static_cast<int>(Vis::ToolSpiral)
    };

private:
    Q_DISABLE_COPY_MOVE(OVisToolSpiral) // NOLINT

    quint32 m_startPointId{NULL_ID};
    VSplinePath m_path{};
    bool m_hasPath{false};
    VScaledEllipse *m_startPoint{nullptr};
};

//---------------------------------------------------------------------------------------------------------------------
inline void OVisToolSpiral::SetStartPointId(quint32 value)
{
    m_startPointId = value;
}

#endif // OVISTOOLSPIRAL_H
