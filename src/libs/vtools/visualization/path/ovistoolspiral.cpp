/************************************************************************
 **
 **  @file   ovistoolspiral.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Visualization for the Spiral tool.
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

#include "ovistoolspiral.h"

#include <QPointF>
#include <QSharedPointer>
#include <new>

#include "../vgeometry/vpointf.h"
#include "../visualization.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/scalesceneitems.h"
#include "vispath.h"

//---------------------------------------------------------------------------------------------------------------------
OVisToolSpiral::OVisToolSpiral(const VContainer *data, QGraphicsItem *parent)
  : VisPath(data, parent)
{
    m_startPoint = InitPoint(VColorRole::VisSupportColor, this);
}

//---------------------------------------------------------------------------------------------------------------------
OVisToolSpiral::~OVisToolSpiral() = default;

//---------------------------------------------------------------------------------------------------------------------
void OVisToolSpiral::RefreshGeometry()
{
    if (GetMode() == Mode::Show)
    {
        m_startPoint->setVisible(false);

        if (!m_hasPath)
        {
            return;
        }

        VSplinePath path = m_path;
        path.SetApproximationScale(ApproximationScale());
        DrawPath(this, path.GetPath(), path.DirectionArrows(), LineStyle(), Qt::RoundCap);
        return;
    }

    // Creation: the spiral does not exist yet — the length that stops it is not known until the dialog is
    // applied. All there is to show is the point it will start at.
    if (m_startPointId <= NULL_ID)
    {
        m_startPoint->setVisible(false);
        return;
    }

    const QSharedPointer<VPointF> start = GetData()->GeometricObject<VPointF>(m_startPointId);
    DrawPoint(m_startPoint, static_cast<QPointF>(*start), Qt::DashLine);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolSpiral::VisualMode(quint32 id)
{
    m_startPointId = id;
    StartVisualMode();
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolSpiral::SetPath(const VSplinePath &path)
{
    m_path = path;
    m_hasPath = true;
}
