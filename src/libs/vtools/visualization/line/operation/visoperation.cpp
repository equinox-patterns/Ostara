/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 9, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "visoperation.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vgobject.h"
#include "../vexception/vexception.h"
#include "../vgeometry/vpointf.h"

//---------------------------------------------------------------------------------------------------------------------
VisOperation::VisOperation(const VContainer *data, QGraphicsItem *parent)
  : VisLine(data, parent)
{
}

//---------------------------------------------------------------------------------------------------------------------
VisOperation::~VisOperation()
{
    qDeleteAll(m_points);
    qDeleteAll(m_curves);
    qDeleteAll(m_lines);
}

//---------------------------------------------------------------------------------------------------------------------
void VisOperation::SetObjects(const QVector<quint32> &objects)
{
    m_objects = objects;

    qDeleteAll(m_points);
    qDeleteAll(m_curves);

    m_points.clear();
    m_curves.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void VisOperation::SetPreviewLines(const QVector<QPair<quint32, quint32>> &lines)
{
    m_previewLines = lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto VisOperation::GetLine(quint32 i, VColorRole role) -> VScaledLine *
{
    if (!m_lines.isEmpty() && static_cast<quint32>(m_lines.size() - 1) >= i)
    {
        return m_lines.at(static_cast<int>(i));
    }

    auto *line = InitItem<VScaledLine>(role, this);
    line->SetShowArrows(false); // Arrows belong to the tool aids, not to a highlighted line
    m_lines.append(line);
    return line;
}

//---------------------------------------------------------------------------------------------------------------------
auto VisOperation::PointPosition(const VContainer *data, quint32 id) -> QPointF
{
    return static_cast<QPointF>(*data->GeometricObject<VPointF>(id));
}

//---------------------------------------------------------------------------------------------------------------------
void VisOperation::RefreshPreviewLines(bool drawImages)
{
    int i = 0;

    for (const auto &attached : m_previewLines)
    {
        QPointF firstPoint;
        QPointF secondPoint;

        // A preview must never be able to crash the application: an unexpected id simply drops the line.
        try
        {
            firstPoint = PointPosition(GetData(), attached.first);
            secondPoint = PointPosition(GetData(), attached.second);
        }
        catch (const VException &e)
        {
            // A line whose endpoint is gone is dropped, nothing more: warning here would raise a modal box on
            // every mouse move, since this runs from RefreshGeometry (see VDrawTool::GetItemType).
            qCDebug(vVis, "Attached line %u - %u is not drawable: %s", attached.first, attached.second,
                    e.ErrorMessage().toUtf8().constData());
            continue;
        }

        const bool firstHasImage = m_objects.contains(attached.first) || IsFixedPoint(attached.first);
        const bool secondHasImage = m_objects.contains(attached.second) || IsFixedPoint(attached.second);

        VScaledLine *source = GetLine(static_cast<quint32>(i), VColorRole::ActivePointColor);
        ++i;
        DrawLine(source, QLineF(firstPoint, secondPoint), Qt::SolidLine);

        VScaledLine *image = GetLine(static_cast<quint32>(i), VColorRole::VisSupportColor);
        ++i;

        if (drawImages && firstHasImage && secondHasImage)
        {
            const QPointF first = PointImage(attached.first, firstPoint);
            const QPointF second = PointImage(attached.second, secondPoint);
            DrawLine(image, QLineF(first, second), Qt::SolidLine);
        }
        else
        {
            // One endpoint has no image: the operation will not create this line, so the preview must
            // not show it either.
            image->setVisible(false);
        }
    }

    for (int j = i; j < m_lines.size(); ++j)
    {
        m_lines.at(j)->setVisible(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VisOperation::VisualMode(quint32 id)
{
    Q_UNUSED(id)
    StartVisualMode();
}

//---------------------------------------------------------------------------------------------------------------------
auto VisOperation::GetPoint(quint32 i, VColorRole role) -> VScaledEllipse *
{
    return GetPointItem(m_points, i, role, this);
}

//---------------------------------------------------------------------------------------------------------------------
auto VisOperation::GetCurve(quint32 i, VColorRole role) -> VCurvePathItem *
{
    if (!m_curves.isEmpty() && static_cast<quint32>(m_curves.size() - 1) >= i)
    {
        return m_curves.at(static_cast<int>(i));
    }

    auto *curve = InitItem<VCurvePathItem>(role, this);
    m_curves.append(curve);
    return curve;
}

//---------------------------------------------------------------------------------------------------------------------
auto VisOperation::ObjectSelected() const -> bool
{
    return m_startAction;
}

//---------------------------------------------------------------------------------------------------------------------
auto VisOperation::AddOriginCurve(quint32 id, int &i) -> QGraphicsPathItem *
{
    const QSharedPointer<VAbstractCurve> curve = GetData()->template GeometricObject<VAbstractCurve>(id);

    ++i;
    VCurvePathItem *path = GetCurve(static_cast<quint32>(i), VColorRole::ActivePointColor);
    DrawPath(path, curve->GetPath(), curve->DirectionArrows(), Qt::SolidLine, Qt::RoundCap);

    return path;
}

//---------------------------------------------------------------------------------------------------------------------
auto VisOperation::AddOriginPoint(quint32 id, int &i) -> VScaledEllipse *
{
    const QSharedPointer<VPointF> p = GetData()->GeometricObject<VPointF>(id);

    ++i;
    VScaledEllipse *point = GetPoint(static_cast<quint32>(i), VColorRole::ActivePointColor);
    DrawPoint(point, static_cast<QPointF>(*p));

    return point;
}

//---------------------------------------------------------------------------------------------------------------------
auto VisOperation::CreateOriginObjects(int &iPoint, int &iCurve) -> QVector<QGraphicsItem *>
{
    QVector<QGraphicsItem *> originObjects;
    originObjects.reserve(Objects().size());

    for (auto id : Objects())
    {
        // The ids come from the pattern, and one of them can be gone by the time the preview is drawn. An
        // exception escaping here would end the application: this runs from the Qt event loop, on every mouse
        // move (see VDrawTool::GetItemType). The object is simply left out of the preview.
        try
        {
            const QSharedPointer<VGObject> obj = GetData()->GetGObject(id);

            // This check helps to find missed objects in the switch
            Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects were handled.");

            QT_WARNING_PUSH
            QT_WARNING_DISABLE_GCC("-Wswitch-default")
            QT_WARNING_DISABLE_CLANG("-Wswitch-default")

            switch (obj->getType())
            {
                case GOType::Point:
                    originObjects.append(AddOriginPoint(id, iPoint));
                    break;
                case GOType::Arc:
                case GOType::EllipticalArc:
                case GOType::Spline:
                case GOType::SplinePath:
                case GOType::CubicBezier:
                case GOType::CubicBezierPath:
                    originObjects.append(AddOriginCurve(id, iCurve));
                    break;
                case GOType::PlaceLabel:
                case GOType::Unknown:
                    // Nothing to draw for either of them: a place label is not an operated object, and an
                    // unknown type has no geometry to show.
                    qCDebug(vVis, "Object %u has no origin to draw, skipped", id);
                    break;
            }

            QT_WARNING_POP
        }
        catch (const VException &e)
        {
            qCDebug(vVis, "Skipping object %u in the preview: %s", id, e.ErrorMessage().toUtf8().constData());
        }
    }

    return originObjects;
}
