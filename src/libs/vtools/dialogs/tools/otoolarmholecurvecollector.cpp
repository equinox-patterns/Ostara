/************************************************************************
 **
 **  @file   otoolarmholecurvecollector.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   10/09/2026
 **
 **  @brief  Headless creation collector for the ArmholeCurve tool — gathers
 **          the clicked points and builds the 7-point path, without any UI.
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

#include "otoolarmholecurvecollector.h"

#include <QString>
#include <QtGlobal>

#include "../../tools/drawTools/toolcurve/otoolarmholecurve.h"
#include "../../visualization/path/ovistoolarmholecurve.h"
#include "../vgeometry/vpointf.h"
#include "../vpatterndb/vcontainer.h"
#include "dialogtool.h"

//---------------------------------------------------------------------------------------------------------------------
OToolArmholeCurveCollector::OToolArmholeCurveCollector(const VContainer *data, VAbstractPattern *doc, quint32 toolId,
                                                       QWidget *parent)
  : DialogTool(data, doc, toolId, parent)
{
    // Headless collector: the visualization enters the scene through VisualMode() on the first pick,
    // so ShowVisualization() is never reached. Never call InitOkCancel*() or CheckState() here — the
    // dialog has no buttons, so bOk stays null and CheckState() would assert.
    vis = new OVisToolArmholeCurve(&this->data);
}

//---------------------------------------------------------------------------------------------------------------------
OToolArmholeCurveCollector::~OToolArmholeCurveCollector() = default;

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurveCollector::GetPath() const -> VCubicBezierPath
{
    return m_path;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurveCollector::SetDefPenStyle(const QString &value)
{
    m_defPenStyle = value;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurveCollector::SetDefColor(const QString &value)
{
    m_defColor = value;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurveCollector::CheckDependencyTreeComplete()
{
    // Headless collector: no UI to enable/disable.
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurveCollector::PointId(PointSlot slot) const -> quint32
{
    return m_pointIds.value(static_cast<int>(slot), NULL_ID);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurveCollector::PointAt(PointSlot slot) const -> QSharedPointer<VPointF>
{
    return data.GeometricObject<VPointF>(PointId(slot));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurveCollector::NextToolTip() const -> QString
{
    switch (m_pointIds.size())
    {
        case Carrure:
            return tr("Select the chest/back width point");
        case Perpendicular:
            return tr("Select the perpendicular point — or click the shoulder point to skip the "
                      "perpendicular.");
        case Shoulder:
            return tr("Select the shoulder point. If you skipped the perpendicular, click the shoulder "
                      "point again.");
        default:
            return QString();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurveCollector::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare || type != SceneObject::Point)
    {
        return;
    }

    auto *visPath = qobject_cast<OVisToolArmholeCurve *>(vis);
    SCASSERT(visPath != nullptr)
    if (Q_UNLIKELY(visPath == nullptr))
    {
        return;
    }

    switch (m_pointIds.size())
    {
        case Armhole:
            m_pointIds.append(id);
            // The only path that puts the visualization on the scene: the dialog is never shown, so
            // ShowVisualization() is not called.
            visPath->VisualMode(id);
            break;
        case Carrure:
            m_pointIds.append(id);
            visPath->SetPoint2Id(id);
            visPath->RefreshGeometry();
            break;
        case Perpendicular:
            if (id == PointId(Armhole) || id == PointId(Carrure))
            {
                return;
            }
            m_pointIds.append(id);
            visPath->SetPoint3Id(id);
            visPath->RefreshGeometry();
            break;
        case Shoulder:
            // The perpendicular may be skipped by picking the shoulder point twice, so the only
            // duplicate allowed here is the perpendicular itself — see IsPerpendicularSkipped().
            if (id == PointId(Armhole) || id == PointId(Carrure))
            {
                return;
            }
            m_pointIds.append(id);
            visPath->SetPoint4Id(id);
            visPath->RefreshGeometry();
            prepare = true;
            break;
        default:
            return;
    }

    emit ToolTip(NextToolTip());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurveCollector::ShowDialog(bool click)
{
    // The fourth click closes the tool. Unlike OToolCurve, Enter is not a finishing gesture here:
    // the curve needs its four points, and the order ChoosedObject() then ShowDialog(true) is the
    // one that already drives the creation.
    if (!click || m_pointIds.size() != PointCount)
    {
        return;
    }

    Finalize();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurveCollector::IsValid() const -> bool
{
    return m_pointIds.size() == PointCount;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurveCollector::Finalize()
{
    m_path = OToolArmholeCurve::BuildPath(*PointAt(Armhole), *PointAt(Carrure), *PointAt(Perpendicular),
                                          *PointAt(Shoulder));

    if (!m_defPenStyle.isEmpty())
    {
        m_path.SetPenStyle(m_defPenStyle);
    }
    if (!m_defColor.isEmpty())
    {
        m_path.SetColor(m_defColor);
    }

    if (not data.IsUnique(m_path.name()))
    {
        m_path.SetDuplicate(DNumber(m_path.name()));
    }

    emit ToolTip(QString());
    DialogAccepted();
}
