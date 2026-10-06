/************************************************************************
 **
 **  @file   ovistoolspread.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  The preview of a slash-and-spread: the operated objects and their spread images.
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

#include "ovistoolspread.h"

#include <QSharedPointer>

#include "../../../tools/drawTools/operation/ospreadtransform.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "visoperation.h"

//---------------------------------------------------------------------------------------------------------------------
OVisToolSpread::OVisToolSpread(const VContainer *data, QGraphicsItem *parent)
  : VisOperation(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor2);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolSpread::SetRotations(const QVector<quint32> &sourceIds, const QVector<OSpreadRotation> &rotations)
{
    m_sourceIds = sourceIds;
    m_rotations = rotations;
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolSpread::SetInPattern(bool inPattern)
{
    m_inPattern = inPattern;
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolSpread::RefreshGeometry()
{
    if (Objects().isEmpty() && !HasPreviewLines())
    {
        return;
    }

    int iPoint = -1;
    int iCurve = -1;

    const QVector<QGraphicsItem *> originObjects = CreateOriginObjects(iPoint, iCurve);

    // The spread is in the pattern: the objects it spread stand there, drawn by the pattern, and the trace of their
    // origin laid over them says nothing it does not already say (see SetInPattern). The images are drawn after this
    // point and stay: they are what shows what the spread does while its dialog is open.
    if (m_inPattern)
    {
        HideOperatedObjects();
    }

    if (originObjects.isEmpty() || m_rotations.isEmpty())
    {
        // Nothing to spread: the objects resolve to nothing, or the chain of rotations is not picked yet. Only the
        // source side of the attached lines is drawn.
        DrawAttachedLines();
        return;
    }

    QVector<SourceItem> source;
    source.reserve(m_sourceIds.size());
    for (const quint32 id : std::as_const(m_sourceIds))
    {
        source.append(SourceItem{.id = id});
    }

    QVector<qreal> angles;
    angles.reserve(m_rotations.size());
    for (const auto &rotation : std::as_const(m_rotations))
    {
        angles.append(FindValFromUser(rotation.angle, GetData()->DataVariables()));
    }

    const OSpreadTransform transform(source, m_rotations, angles, GetData());
    CreateTransformedObjects(iPoint, iCurve, transform);

    // The spread images of the line endpoints are already drawn as points; the source lines stay as reference.
    DrawAttachedLines();
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolSpread::DrawAttachedLines()
{
    RefreshPreviewLines(false);

    // This draws the source side of every line attached to the spread, which is the trace of the origin of those lines
    // too: where the pattern draws them itself, it goes away with the objects of the origin (see SetInPattern).
    if (m_inPattern)
    {
        HideAttachedLines();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolSpread::CreateTransformedObjects(int &iPoint, int &iCurve, const OSpreadTransform &transform)
{
    for (const quint32 id : Objects())
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
            {
                const QSharedPointer<VPointF> p = GetData()->GeometricObject<VPointF>(id);
                ++iPoint;
                VScaledEllipse *point = GetPoint(static_cast<quint32>(iPoint), VColorRole::VisSupportColor);
                DrawPoint(point, static_cast<QPointF>(transform.Image(*p, QString())));
                break;
            }
            case GOType::Arc:
                iCurve = AddTransformedCurve<VArc>(id, iCurve, transform);
                break;
            case GOType::EllipticalArc:
                iCurve = AddTransformedCurve<VEllipticalArc>(id, iCurve, transform);
                break;
            case GOType::Spline:
                iCurve = AddTransformedCurve<VSpline>(id, iCurve, transform);
                break;
            case GOType::SplinePath:
                iCurve = AddTransformedCurve<VSplinePath>(id, iCurve, transform);
                break;
            case GOType::CubicBezier:
                iCurve = AddTransformedCurve<VCubicBezier>(id, iCurve, transform);
                break;
            case GOType::CubicBezierPath:
                iCurve = AddTransformedCurve<VCubicBezierPath>(id, iCurve, transform);
                break;
            case GOType::PlaceLabel:
            case GOType::Unknown:
                qCDebug(vVis, "Object %u cannot be spread, skipped", id);
                break;
        }

        QT_WARNING_POP
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Item>
auto OVisToolSpread::AddTransformedCurve(quint32 id, int i, const OSpreadTransform &transform) -> int
{
    const QSharedPointer<Item> curve = GetData()->template GeometricObject<Item>(id);

    ++i;
    VCurvePathItem *path = GetCurve(static_cast<quint32>(i), VColorRole::VisSupportColor);
    const Item transformed = transform.Image(*curve, QString());
    DrawPath(path, transformed.GetPath(), transformed.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);

    return i;
}
