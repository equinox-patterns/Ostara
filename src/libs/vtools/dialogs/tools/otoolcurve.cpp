/************************************************************************
 **
 **  @file   otoolcurve.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/08/2026
 **
 **  @brief  Frontend collector for the unified "Curve" tool.
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

#include "otoolcurve.h"

#include <QString>
#include <QtGlobal>

#include "../../visualization/path/vistoolsplinepath.h"
#include "../../visualization/visualization.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vsplinepath.h"
#include "../vgeometry/vsplinepoint.h"
#include "../vmisc/vabstractapplication.h"
#include "../vwidgets/vmaingraphicsscene.h"

//---------------------------------------------------------------------------------------------------------------------
OToolCurve::OToolCurve(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent)
{
    vis = new VisToolSplinePath(&this->data);
    auto *path = qobject_cast<VisToolSplinePath *>(vis);
    SCASSERT(path != nullptr)

    auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
    SCASSERT(scene != nullptr)
    connect(scene, &VMainGraphicsScene::MouseLeftPressed, path, &VisToolSplinePath::MouseLeftPressed);
    connect(scene, &VMainGraphicsScene::MouseLeftReleased, path, &VisToolSplinePath::MouseLeftReleased);
    connect(path, &VisToolSplinePath::PathChanged, this, &OToolCurve::PathUpdated);
}

//---------------------------------------------------------------------------------------------------------------------
OToolCurve::~OToolCurve() = default;

//---------------------------------------------------------------------------------------------------------------------
auto OToolCurve::GetPointIds() const -> QVector<quint32>
{
    return m_pointIds;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCurve::GetPath() const -> VSplinePath
{
    return m_path;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurve::PathUpdated(const VSplinePath &path)
{
    m_path = path;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurve::CheckDependencyTreeComplete()
{
    // Headless collector: no UI to enable/disable.
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurve::ChosenObject(quint32 id, const SceneObject &type)
{
    if (type != SceneObject::Point || m_pointIds.contains(id))
    {
        return;
    }

    m_pointIds.append(id);

    VSplinePoint p;
    p.SetP(*data.GeometricObject<VPointF>(id));
    m_path.append(p);

    auto *visPath = qobject_cast<VisToolSplinePath *>(vis);
    SCASSERT(visPath != nullptr)
    visPath->SetPath(m_path);

    if (m_pointIds.size() == 1)
    {
        visPath->VisualMode(NULL_ID);
    }
    else
    {
        visPath->RefreshGeometry();
    }

    if (m_pointIds.size() == 1)
    {
        emit ToolTip(tr("Select the second point of the curve"));
    }
    else
    {
        emit ToolTip(tr("Select an additional point or press Enter to finish"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurve::ShowDialog(bool click)
{
    if (click)
    {
        return;
    }

    if (m_pointIds.size() >= 2)
    {
        emit ToolTip(QString());
        DialogAccepted();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCurve::ShowVisualization()
{
    AddVisualization<VisToolSplinePath>();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCurve::IsValid() const -> bool
{
    return m_pointIds.size() >= 2;
}
