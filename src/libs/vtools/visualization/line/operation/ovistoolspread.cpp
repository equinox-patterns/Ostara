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
#include "../../../vtheme/vscenestylesheet.h"
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
#include "../vwidgets/scalesceneitems.h"
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

    // Two moments, two answers. WHILE the tool is being used — it is not in the pattern yet — what has to be read is
    // the SELECTION: the objects picked so far are echoed, and that echo is the orange the gesture relies on.
    // The echo of the selection is built ONLY while the tool is being used, and not at all once it exists in the
    // pattern: there the pattern draws those very objects itself, and what the eye needs is what the rotations
    // GENERATED — the `__R` of the points, of the curves and of the lines. Hiding the echo after building it was not
    // enough: the items were still there, under the images.
    QVector<QGraphicsItem *> originObjects;
    if (not m_inPattern)
    {
        originObjects = CreateOriginObjects(iPoint, iCurve);
    }

    if (m_inPattern)
    {
        HideOperatedObjects();
    }

    if ((m_inPattern ? Objects().isEmpty() : originObjects.isEmpty()) || m_rotations.isEmpty())
    {
        // Nothing to spread: the objects resolve to nothing, or the chain of rotations is not picked yet. Only the
        // source side of the attached lines is drawn — and the dots of the pair being picked, which is all there is
        // to read at that moment.
        DrawAttachedLines();
        DrawPairPoints(nullptr);
        return;
    }

    QVector<SourceItem> source;
    source.reserve(m_sourceIds.size());
    for (const quint32 id : std::as_const(m_sourceIds))
    {
        source.append(SourceItem{.id = id});
    }

    QVector<qreal> angles = m_angles;
    if (angles.size() != m_rotations.size())
    {
        // No angles handed over — a dialog still being filled in: the formulas are all there is to read.
        angles.clear();
        angles.reserve(m_rotations.size());
        for (const auto &rotation : std::as_const(m_rotations))
        {
            angles.append(FindValFromUser(rotation.angle, GetData()->DataVariables()));
        }
    }

    const OSpreadTransform transform(source, m_rotations, angles, GetData(), m_smooth);
    CreateTransformedObjects(iPoint, iCurve, transform);

    // The spread images of the line endpoints are already drawn as points; the source lines stay as reference.
    DrawAttachedLines();

    // The dots come last, and above: they are what the gesture asks the eye to read — which end of the pair is
    // which — and the copies of the points would otherwise cover them.
    DrawPairPoints(m_inPattern ? &transform : nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolSpread::DrawPairPoints(const OSpreadTransform *transform)
{
    const auto dot = [this, transform](QVector<VScaledEllipse *> &pool, int index, VColorRole role,
                                       quint32 id) -> void
    {
        VScaledEllipse *item = (index < pool.size()) ? pool.at(index) : nullptr;
        if (item == nullptr)
        {
            // A dot is a mark, not an object of the figure: it takes the pass of the gesture (see InitMark), so the
            // echo of the objects never hides it.
            item = InitMark<VScaledEllipse>(role, this);
            pool.append(item);
        }

        if (id == NULL_ID || GetData() == nullptr || not GetData()->HasObject(id))
        {
            item->setVisible(false); // the second point of the pair is not picked yet
            return;
        }

        // The dot marks the object the operation GENERATED — `B35__R`, `B33__R` — and not the point that was picked:
        // the highlight belongs to what the rotations made, the picked points are already drawn by the pattern.
        const QSharedPointer<VPointF> point = GetData()->GeometricObject<VPointF>(id);
        DrawPoint(item, transform != nullptr ? static_cast<QPointF>(transform->Image(*point, QString()))
                                             : static_cast<QPointF>(*point));
        item->setVisible(true);
    };

    // Green for the start point of a cut, red for its pivot: the very two colours a pleat gives the ends of its
    // departure line, and the dialog uses no other.
    int start = 0;
    int pivot = 0;
    for (const OSpreadRotation &rotation : std::as_const(m_rotations))
    {
        dot(m_startDots, start++, VColorRole::VisSupportColor2, rotation.startId);
        dot(m_pivotDots, pivot++, VColorRole::VisSupportColor4, rotation.pivotId);
    }

    for (int i = start; i < m_startDots.size(); ++i)
    {
        m_startDots.at(i)->setVisible(false);
    }
    for (int i = pivot; i < m_pivotDots.size(); ++i)
    {
        m_pivotDots.at(i)->setVisible(false);
    }
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
    // A point no rotation carries is not copied by the operation: it is the point itself, already drawn as the
    // pivot of the rotation whose region stops at it. The preview shows what will be created, so it draws no
    // second dot on top of it.
    const QVector<quint32> referenced = transform.ReferencedPoints();

    for (const quint32 id : Objects())
    {
        if (referenced.contains(id))
        {
            continue;
        }

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

    // The colour of the curve is its own: the item is built in the custom role, which is the one that lets the pen
    // written below decide both the path AND its direction arrows (see VCurvePathItem::paint). Left in the support
    // role, the arrows alone would stay magenta on a curve painted red.
    VCurvePathItem *path = GetCurve(static_cast<quint32>(i), VColorRole::CustomColor);
    const Item transformed = transform.Image(*curve, QString());
    DrawPath(path, transformed.GetPath(), transformed.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);

    // The copy carries the colour its line has in the report, so a line of the list can be found on the drawing.
    // A curve the chain did not rebuild has no rank and keeps the support colour of the preview.
    QPen pen = path->pen();
    pen.setColor(transform.Rank(id) > 0 ? OSpreadColors::Color(transform.Rank(id))
                                        : VSceneStylesheet::Color(VColorRole::VisSupportColor));
    path->setPen(pen);

    return i;
}
