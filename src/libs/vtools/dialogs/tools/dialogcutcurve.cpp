/************************************************************************
 **
 **  @file   dialogcutcurve.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   13/08/2026
 **
 **  @brief  Thin collector for the unified "Segment a curve" tool.
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

#include "dialogcutcurve.h"

//---------------------------------------------------------------------------------------------------------------------
DialogCutCurve::DialogCutCurve(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
DialogCutCurve::~DialogCutCurve() = default;

//---------------------------------------------------------------------------------------------------------------------
void DialogCutCurve::ChosenObject(quint32 id, const SceneObject &type)
{
    if (type == SceneObject::Spline || type == SceneObject::SplinePath)
    {
        m_curveId = id;
        m_isSplinePath = (type == SceneObject::SplinePath);
        DialogAccepted();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutCurve::getCurveId() const -> quint32
{
    return m_curveId;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutCurve::IsSplinePath() const -> bool
{
    return m_isSplinePath;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutCurve::CheckDependencyTreeComplete()
{
    // Headless collector: no UI to enable/disable.
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutCurve::IsValid() const -> bool
{
    return m_curveId != NULL_ID;
}
