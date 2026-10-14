/************************************************************************
 **
 **  @file   ovistoolpleat.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The preview of the pleats: the objects that travel where the folds put them, and the folds themselves.
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

#include "ovistoolpleat.h"

#include <QSharedPointer>

#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "../vwidgets/scalesceneitems.h"

namespace
{
/** How bright the ends of a departure line the dialog is not editing are drawn: enough to be seen, dim enough for
 *  the two ends of the line being edited to be the only ones the eye follows. */
constexpr qreal kDimmedLight = 0.25;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OVisToolPleat::OVisToolPleat(const VContainer *data, QGraphicsItem *parent)
  : VisOperation(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor2);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::SetPleat(const QVector<OPleatDeparture> &departures, const OPleatEnd &pending, int active,
                             OPleatOrigin origin)
{
    m_departures = departures;
    m_pending = pending;
    m_active = active;
    m_origin = origin;
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolPleat::Light(int departure) const -> qreal
{
    // One line is being edited: it is the one the dialog shows, and its two ends have to be told from the ends of the
    // other lines, which wear the very same green and red. The ends of the others are dimmed.
    return (m_active < 0 || m_active == departure) ? 1. : kDimmedLight;
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolPleat::ValueInPixels(const QString &value, const VContainer *data) -> qreal
{
    if (value.isEmpty())
    {
        return 0.;
    }

    // A formula that does not hold answers nothing: the preview draws no fold at all while the user is typing one,
    // and never a fold built on a value the tool would refuse.
    const qreal read = FindValFromUser(value, data->DataVariables());
    return (read > 0. && !qIsNaN(read) && !qIsInf(read)) ? VAbstractValApplication::VApp()->toPixel(read) : 0.;
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolPleat::Values() const -> QVector<qreal>
{
    QVector<qreal> values;
    values.reserve(m_departures.size());

    for (const OPleatDeparture &departure : m_departures)
    {
        values.append(ValueInPixels(departure.value, GetData()));
    }

    return values;
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolPleat::Source() const -> QVector<SourceItem>
{
    QVector<SourceItem> source;
    source.reserve(Objects().size());

    for (const quint32 id : Objects())
    {
        source.append(SourceItem{.id = id});
    }

    return source;
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::RefreshGeometry()
{
    if (Objects().isEmpty() && !HasPreviewLines())
    {
        return;
    }

    m_plan = OPleatPlan{};

    int iPoint = -1;
    int iCurve = -1;

    const QVector<QGraphicsItem *> originObjects = CreateOriginObjects(iPoint, iCurve);
    if (originObjects.isEmpty() || m_departures.isEmpty())
    {
        DrawWithoutAPlan();
        return;
    }

    const OPleatPlan plan = OPleatRule::Compose(m_departures, Values(), Source(), GetData(), m_origin);
    if (plan.failure != OPleatFailure::None)
    {
        // A pleat the rule refuses is no pleat: the objects that travel are shown as they are, and only the gesture
        // keeps its dots. The dialog never opens on such a plan, so this is a state the canvas alone can show.
        DrawWithoutAPlan();
        return;
    }

    m_plan = plan;

    const OPleatTransform transform(m_plan);
    CreateTransformedObjects(transform);

    // And the copy of every line the user selected, drawn from end to end like the objects: the images of the two
    // ends are the line's image, exactly as the operation will build it.
    RefreshPreviewLines(true);

    // The pleat is in the pattern: every object it carries is drawn there already, and the second drawing this
    // preview lays over it — the trace of the origin in orange, the image in magenta — would show the same objects
    // twice. Only the ends of the gesture are kept (see SetInPattern).
    if (m_inPattern)
    {
        HideOperatedObjects();
        HideAttachedLines();
    }

    // Last, because RefreshPreviewLines hides every line it does not use of the pool it shares with this call.
    DrawDeparturePoints();
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::SetInPattern(bool inPattern)
{
    m_inPattern = inPattern;
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::DrawWithoutAPlan()
{
    RefreshPreviewLines(false);

    // This path drew the source side of every line attached to the pleat; where the pattern draws those lines itself,
    // that drawing goes away with the objects of the origin (see SetInPattern).
    if (m_inPattern)
    {
        HideOperatedObjects();
        HideAttachedLines();
    }

    DrawDeparturePoints();
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolPleat::PointImage(quint32 id, const QPointF &point) const -> QPointF
{
    // A pleat takes the run of the ordered selection between the two points of its departure line, so two objects
    // do not travel the same distance: the object is asked, exactly as the operation asks it (see OPleatPlan::Move).
    return point + m_plan.Move(id);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::DrawDeparturePoints()
{
    const auto dot = [this](QVector<VScaledEllipse *> &pool, int index, VColorRole role, const OPleatEnd &end,
                            qreal light)
    {
        VScaledEllipse *item = nullptr;
        if (index < pool.size())
        {
            item = pool.at(index);
        }
        else
        {
            // The dots are what the gesture asks the eye to read — green for the first end, red for the second — so
            // they take the Z of a mark and leave the pass the echo of the objects is not in (see InitMark).
            item = InitMark<VScaledEllipse>(role, this);
            pool.append(item);
        }

        item->setOpacity(light);

        if (end.pointId == NULL_ID)
        {
            item->setVisible(false);
            return;
        }

        DrawPoint(item, end.position);
        item->setVisible(true);
    };

    // Green for the first point of a departure line, red for the second: the colour IS the name, and the dialog and
    // the dock use the very same two.
    int first = 0;
    int second = 0;
    for (int departure = 0; departure < m_departures.size(); ++departure)
    {
        // The dot marks the end of the departure line WHERE THE PLEAT IS, not where the point was drawn: the pleats
        // before it carried that line away, and a mark left on the original point would name a place the fold never
        // passes through (see OPleatPlan::Place).
        OPleatEnd one = m_departures.at(departure).first;
        OPleatEnd two = m_departures.at(departure).second;
        one.position = m_plan.Place(one, departure, 0);
        two.position = m_plan.Place(two, departure, 0);

        dot(m_firstDots, first++, VColorRole::VisSupportColor2, one, Light(departure));
        dot(m_secondDots, second++, VColorRole::VisSupportColor4, two, Light(departure));
    }

    // The line being picked: its first point is already a dot of the gesture, its second one is still to come.
    dot(m_firstDots, first++, VColorRole::VisSupportColor2, m_pending, 1.);

    for (int i = first; i < m_firstDots.size(); ++i)
    {
        m_firstDots.at(i)->setVisible(false);
    }
    for (int i = second; i < m_secondDots.size(); ++i)
    {
        m_secondDots.at(i)->setVisible(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::CreateTransformedObjects(const OPleatTransform &transform)
{
    int iPoint = -1;
    int iCurve = -1;

    // The origin objects have already taken their place in the two lists; the images carry on after them.
    for (const quint32 id : Objects())
    {
        const QSharedPointer<VGObject> obj = GetData()->GetGObject(id);

        Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects were handled.");

        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wswitch-default")
        QT_WARNING_DISABLE_CLANG("-Wswitch-default")

        switch (obj->getType())
        {
            case GOType::Point:
            {
                const QSharedPointer<VPointF> point = GetData()->GeometricObject<VPointF>(id);
                ++iPoint;
                VScaledEllipse *image = GetPoint(static_cast<quint32>(iPoint), VColorRole::VisSupportColor);
                DrawPoint(image, static_cast<QPointF>(transform.Image(*point, QString())));
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
                qCDebug(vVis, "Object %u cannot be folded, skipped", id);
                break;
        }

        QT_WARNING_POP
    }
}

//---------------------------------------------------------------------------------------------------------------------
template <class Item>
auto OVisToolPleat::AddTransformedCurve(quint32 id, int i, const OPleatTransform &transform) -> int
{
    const QSharedPointer<Item> curve = GetData()->template GeometricObject<Item>(id);

    ++i;
    VCurvePathItem *path = GetCurve(static_cast<quint32>(i), VColorRole::VisSupportColor);
    const Item transformed = transform.Image(*curve, QString());
    DrawPath(path, transformed.GetPath(), transformed.DirectionArrows(), Qt::SolidLine, Qt::RoundCap);

    return i;
}
