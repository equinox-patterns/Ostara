/************************************************************************
 **
 **  @file   vistoolrotation.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   11 4, 2016
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

#include "vistoolrotation.h"

#include <QGuiApplication>
#include <QLineF>
#include <QSharedPointer>
#include <climits>

#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vmisc/vmodifierkey.h"
#include "../vpatterndb/vcontainer.h"
#include "../vexception/vexception.h"
#include "../vwidgets/global.h"
#include "visoperation.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
// A preview is built from ids read in the pattern, and one of them can be gone by the time it is drawn. Letting
// VExceptionBadId out of here would end the application: an exception raised from the Qt event loop — and this
// runs on every mouse move — is not handled, see VDrawTool::GetItemType. A missing object simply draws nothing.
template <class T> auto FindObject(const VContainer *data, quint32 id) -> QSharedPointer<T>
{
    if (data == nullptr || id == NULL_ID)
    {
        return {};
    }

    try
    {
        return data->GeometricObject<T>(id);
    }
    catch (const VException &e)
    {
        qCDebug(vVis, "Skipping object %u in the rotation preview: %s", id, e.ErrorMessage().toUtf8().constData());
        return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto FindGObject(const VContainer *data, quint32 id) -> QSharedPointer<VGObject>
{
    if (data == nullptr || id == NULL_ID)
    {
        return {};
    }

    try
    {
        return data->GetGObject(id);
    }
    catch (const VException &e)
    {
        qCDebug(vVis, "Skipping object %u in the rotation preview: %s", id, e.ErrorMessage().toUtf8().constData());
        return {};
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
VisToolRotation::VisToolRotation(const VContainer *data, QGraphicsItem *parent)
  : VisOperation(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor2);

    m_point = InitPoint(VColorRole::VisSupportColor2, this, kMarkZ);
    // The aides of the rotation — the arc and the reference axis — take a colour of their own, so that the
    // pivot is not painted like the circle it is turned around (same split as VisToolMove).
    m_angleArc = InitItem<VCurvePathItem>(VColorRole::VisSupportColor3, this);
    m_xAxis = InitItem<VScaledLine>(VColorRole::VisSupportColor3, this);
    m_xAxis->SetShowArrows(false); // an axis of reference, not a direction
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolRotation::RefreshGeometry()
{
    // An operation can be started from a line alone: the operated-object list is empty then, but the
    // attached line still has a source side to draw.
    if (Objects().isEmpty() && !HasPreviewLines())
    {
        return;
    }

    int iPoint = -1;
    int iCurve = -1;

    if (const QVector<QGraphicsItem *> originObjects = CreateOriginObjects(iPoint, iCurve);
        originObjects.isEmpty() || (GetMode() == Mode::Creation && !ObjectSelected()))
    {
        // Stage 1: the transformation is not known yet, so only the source side of the lines is drawn.
        RefreshPreviewLines(false);
        return;
    }

    qreal tempAngle = 0;

    if (m_originPointId != NULL_ID)
    {
        const QSharedPointer<VPointF> origin = FindObject<VPointF>(GetData(), m_originPointId);
        if (origin.isNull())
        {
            return; // the pivot is gone: there is no circle left to draw the rotation around
        }

        DrawPoint(m_point, static_cast<QPointF>(*origin));

        QLineF rLine;
        if (VFuzzyComparePossibleNulls(m_angle, INT_MIN))
        {
            rLine = QLineF(static_cast<QPointF>(*origin), ScenePos());

            if (QGuiApplication::keyboardModifiers() == Qt::ShiftModifier)
            {
                rLine.setAngle(CorrectAngle(rLine.angle()));
            }

            rLine.setP2(Ray(static_cast<QPointF>(*origin), rLine.angle()));
            tempAngle = rLine.angle();
        }
        else
        {
            rLine = QLineF(static_cast<QPointF>(*origin), Ray(static_cast<QPointF>(*origin), m_angle));
            tempAngle = m_angle;
        }

        // Drawn and read the short way round, exactly like the length it goes with: a rotation of 270° is a
        // rotation of -90°, and it is shown as such here as it is in the dialog.
        const qreal sweep = NormalizeRotation(tempAngle);

        DrawLine(this, rLine, Qt::DashLine);
        DrawLine(m_xAxis, QLineF(static_cast<QPointF>(*origin), Ray(static_cast<QPointF>(*origin), 0)), Qt::DashLine);

        if (qIsNaN(sweep) || qFuzzyIsNull(sweep))
        {
            // Nothing is turned: VArc reads an empty sweep as a whole circle, so the arc would lie.
            m_angleArc->setVisible(false);
        }
        else
        {
            VArc const arc(*origin, ScaledRadius(SceneScale(VAbstractValApplication::VApp()->getCurrentScene())) * 2,
                           0, sweep);
            DrawPath(m_angleArc, arc.GetPath(), Qt::SolidLine, Qt::RoundCap);
        }

        // The status bar reads the length out too: the chord of the reference point — the first source point
        // — signed like the rotation it comes from, so what is read here can be typed in the dialog. No point
        // among the sources means no reference, hence nothing to say about a length.
        const quint32 referenceId = GetData()->FirstPointId(Objects());
        if (referenceId != NULL_ID)
        {
            SetToolTip(tr("Rotating angle = %1°, length = %2%3, <b>%4</b> - sticking angle, "
                          "<b>Mouse click</b> - finish creation")
                           .arg(AngleToUser(sweep),
                                LengthToUser(RotationChord(GetData()->PointsDistance(m_originPointId, referenceId),
                                                           sweep)),
                                UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true),
                                VModifierKey::Shift()));
        }
        else
        {
            SetToolTip(tr("Rotating angle = %1°, <b>%2</b> - sticking angle, "
                          "<b>Mouse click</b> - finish creation")
                           .arg(AngleToUser(sweep), VModifierKey::Shift()));
        }
    }

    CreateRotatedObjects(iPoint, iCurve, tempAngle);
    RefreshPreviewLines(true);
}

//---------------------------------------------------------------------------------------------------------------------
auto VisToolRotation::PointImage(const QPointF &point) const -> QPointF
{
    if (m_originPointId == NULL_ID)
    {
        return point;
    }

    const QSharedPointer<VPointF> origin = FindObject<VPointF>(GetData(), m_originPointId);
    if (origin.isNull())
    {
        return point; // no pivot left to turn around: the point is its own image
    }

    // Same angle as the one shown in the status bar: the live one while the mouse drives it, the locked
    // one once the angle is set.
    const qreal angle = VFuzzyComparePossibleNulls(m_angle, INT_MIN)
                            ? QLineF(static_cast<QPointF>(*origin), ScenePos()).angle()
                            : m_angle;
    return VPointF::RotatePF(static_cast<QPointF>(*origin), point, angle);
}

//---------------------------------------------------------------------------------------------------------------------
auto VisToolRotation::IsFixedPoint(quint32 id) const -> bool
{
    // The centre of rotation stays where it is: it is its own image.
    return m_originPointId != NULL_ID && id == m_originPointId;
}

//---------------------------------------------------------------------------------------------------------------------
auto VisToolRotation::Angle() const -> QString
{
    return QString::number(RoundForDisplay(line().angle()));
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolRotation::SetAngle(const QString &expression)
{
    m_angle = FindValFromUser(expression, GetData()->DataVariables());
}

//---------------------------------------------------------------------------------------------------------------------
auto VisToolRotation::AddRotatedPoint(qreal angle, quint32 id, int i) -> int
{
    if (m_originPointId == NULL_ID)
    {
        return i;
    }

    const QSharedPointer<VPointF> p = FindObject<VPointF>(GetData(), id);
    const QSharedPointer<VPointF> origin = FindObject<VPointF>(GetData(), m_originPointId);
    if (p.isNull() || origin.isNull())
    {
        return i; // one of the two is gone: the image of this point simply is not drawn
    }

    ++i;
    VScaledEllipse *point = GetPoint(static_cast<quint32>(i), VColorRole::VisSupportColor);
    DrawPoint(point, static_cast<QPointF>(p->Rotate(static_cast<QPointF>(*origin), angle)));

    return i;
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolRotation::CreateRotatedObjects(int &iPoint, int &iCurve, qreal angle)
{
    for (auto id : Objects())
    {
        const QSharedPointer<VGObject> obj = FindGObject(GetData(), id);
        if (obj.isNull())
        {
            continue; // the object is gone: it has no image to draw
        }

        // This check helps to find missed objects in the switch
        Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects was handled.");

        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wswitch-default")
        QT_WARNING_DISABLE_CLANG("-Wswitch-default")

        switch (obj->getType())
        {
            case GOType::Point:
                iPoint = AddRotatedPoint(angle, id, iPoint);
                break;
            case GOType::Arc:
                iCurve = AddRotatedCurve<VArc>(angle, id, iCurve);
                break;
            case GOType::EllipticalArc:
                iCurve = AddRotatedCurve<VEllipticalArc>(angle, id, iCurve);
                break;
            case GOType::Spline:
                iCurve = AddRotatedCurve<VSpline>(angle, id, iCurve);
                break;
            case GOType::SplinePath:
                iCurve = AddRotatedCurve<VSplinePath>(angle, id, iCurve);
                break;
            case GOType::CubicBezier:
                iCurve = AddRotatedCurve<VCubicBezier>(angle, id, iCurve);
                break;
            case GOType::CubicBezierPath:
                iCurve = AddRotatedCurve<VCubicBezierPath>(angle, id, iCurve);
                break;
            case GOType::PlaceLabel:
            case GOType::Unknown:
                // A place label cannot be operated on, and an unknown type has nothing to draw: the source
                // list is read from the pattern, so neither is impossible in a hand-edited file.
                qCDebug(vVis, "Object %u cannot be rotated, skipped", id);
                break;
        }

        QT_WARNING_POP
    }
}

//---------------------------------------------------------------------------------------------------------------------
template<class Item>
auto VisToolRotation::AddRotatedCurve(qreal angle, quint32 id, int i) -> int
{
    if (m_originPointId == NULL_ID)
    {
        return i;
    }

    const QSharedPointer<Item> curve = FindObject<Item>(GetData(), id);
    const QSharedPointer<VPointF> origin = FindObject<VPointF>(GetData(), m_originPointId);
    if (curve.isNull() || origin.isNull())
    {
        return i; // one of the two is gone: this curve has no image to draw
    }

    ++i;
    VCurvePathItem *path = GetCurve(static_cast<quint32>(i), VColorRole::VisSupportColor);
    const Item rotated = curve->Rotate(static_cast<QPointF>(*origin), angle);
    DrawPath(path, rotated.GetPath(), rotated.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);

    return i;
}
