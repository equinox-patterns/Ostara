/************************************************************************
 **
 **  @file   ovistoolpleat.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  What a pleat previews: the objects that travel, and where the fold carries them.
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

//---------------------------------------------------------------------------------------------------------------------
OVisToolPleat::OVisToolPleat(const VContainer *data, QGraphicsItem *parent)
  : VisOperation(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor2);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::SetPleat(const OPleatEnd &first, const OPleatEnd &second, const QString &value, OPleatKind kind,
                             bool foldLine)
{
    m_first = first;
    m_second = second;
    m_value = value;
    m_kind = kind;
    m_foldLine = foldLine;
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolPleat::ValueInPixels() const -> qreal
{
    if (m_value.isEmpty())
    {
        return 0.;
    }

    return VAbstractValApplication::VApp()->toPixel(FindValFromUser(m_value, GetData()->DataVariables()));
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::RefreshGeometry()
{
    if (Objects().isEmpty() && !HasPreviewLines())
    {
        return;
    }

    int iPoint = -1;
    int iCurve = -1;

    const QVector<QGraphicsItem *> originObjects = CreateOriginObjects(iPoint, iCurve);
    if (originObjects.isEmpty())
    {
        m_move = QPointF();
        RefreshPreviewLines(false);
        DrawDeparturePoints();
        return;
    }

    const qreal value = ValueInPixels();
    if (value <= 0. || m_first.pointId == NULL_ID || m_second.pointId == NULL_ID)
    {
        // The prise or the value is not picked yet: only the objects that travel are drawn, as they are.
        m_move = QPointF();
        RefreshPreviewLines(false);
        DrawDeparturePoints();
        return;
    }

    QVector<SourceItem> source;
    source.reserve(Objects().size());
    for (const quint32 id : Objects())
    {
        source.append(SourceItem{.id = id});
    }

    const OPleatSide side = OPleatRule::SideOf(source, m_first.pointId, m_second.pointId, GetData());
    if (side == OPleatSide::Undetermined)
    {
        m_move = QPointF();
        RefreshPreviewLines(false);
        DrawDeparturePoints();
        return;
    }

    const OPleatPlan plan = OPleatRule::Compose(m_first, m_second, value, m_kind, side);
    m_move = plan.move;

    const OPleatTransform transform(m_first, m_second, value, m_kind, side);
    CreateTransformedObjects(transform);

    // And the copy of every line the user selected, drawn from end to end like the objects: the images of the two
    // ends are the line's image, exactly as the operation will build it.
    RefreshPreviewLines(true);

    // Last, because RefreshPreviewLines hides every line it does not use of the pool it shares with this call.
    DrawFigure(plan);
    DrawDeparturePoints();
}

//---------------------------------------------------------------------------------------------------------------------
auto OVisToolPleat::PointImage(const QPointF &point) const -> QPointF
{
    // A pleat is a translation: every object of the selection, and every line that joins two of them, goes by the
    // same vector (see m_move).
    return point + m_move;
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::DrawDeparturePoints()
{
    // Green for the first point of the departure line, red for the second: the colour IS the name, and the dialog and
    // the dock use the very same two.
    if (m_firstDot == nullptr)
    {
        m_firstDot = InitItem<VScaledEllipse>(VColorRole::VisSupportColor2, this);
    }
    if (m_secondDot == nullptr)
    {
        m_secondDot = InitItem<VScaledEllipse>(VColorRole::VisSupportColor4, this);
    }

    if (m_first.pointId != NULL_ID)
    {
        DrawPoint(m_firstDot, m_first.position);
        m_firstDot->setVisible(true);
    }
    else
    {
        m_firstDot->setVisible(false);
    }

    if (m_second.pointId != NULL_ID)
    {
        DrawPoint(m_secondDot, m_second.position);
        m_secondDot->setVisible(true);
    }
    else
    {
        m_secondDot->setVisible(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolPleat::DrawFigure(const OPleatPlan &plan)
{
    int usedLines = 0;
    int usedPoints = 0;

    const auto segment = [this, &usedLines](const QPointF &from, const QPointF &to, VColorRole role,
                                            Qt::PenStyle style)
    {
        VScaledLine *item = nullptr;
        if (usedLines < m_figureLines.size())
        {
            item = m_figureLines.at(usedLines);
            item->SetColorRole(role);
        }
        else
        {
            item = InitItem<VScaledLine>(role, this);
            item->SetShowArrows(false);
            m_figureLines.append(item);
        }
        ++usedLines;

        const QLineF line(from, to);
        item->setVisible(line.length() > 0.);
        if (line.length() > 0.)
        {
            DrawLine(item, line, style);
        }
    };

    const auto dot = [this, &usedPoints](const QPointF &position)
    {
        VScaledEllipse *item = nullptr;
        if (usedPoints < m_figurePoints.size())
        {
            item = m_figurePoints.at(usedPoints);
        }
        else
        {
            item = InitItem<VScaledEllipse>(VColorRole::VisSupportColor3, this);
            m_figurePoints.append(item);
        }
        ++usedPoints;

        DrawPoint(item, position);
        item->setVisible(true);
    };

    const auto hideLeftovers = [this, usedLines, usedPoints]()
    {
        for (int i = usedLines; i < m_figureLines.size(); ++i)
        {
            m_figureLines.at(i)->setVisible(false);
        }
        for (int i = usedPoints; i < m_figurePoints.size(); ++i)
        {
            m_figurePoints.at(i)->setVisible(false);
        }
    };

    if (plan.failure != OPleatFailure::None)
    {
        hideLeftovers();
        return;
    }

    const int pleats = (m_kind == OPleatKind::Double) ? 2 : 1;

    // The prise, and every place the fold carries it to: the same line, one pleat at a time.
    for (int position = 0; position <= pleats; ++position)
    {
        const QPointF shift = plan.step * position;
        segment(m_first.position + shift, m_second.position + shift, VColorRole::VisSupportColor3, Qt::SolidLine);
    }

    // What the pleat is pressed along, drawn only when both caps are: without them it has nothing to join.
    if (m_foldLine && m_first.showCap && m_second.showCap)
    {
        for (int pleat = 0; pleat < pleats; ++pleat)
        {
            const QPointF shift = plan.step * pleat + plan.step / 2.;
            segment(m_first.position + shift, m_second.position + shift, VColorRole::VisSupportColor3, Qt::DotLine);
        }
    }

    // Each cap: its point, and the two sides that make the V — or the point alone when the edge lies along the move.
    for (const OPleatMark &mark : std::as_const(plan.marks))
    {
        dot(mark.point);

        const OPleatEnd &prise = (mark.end == 1) ? m_second : m_first;
        const QPointF origin = prise.position + plan.step * mark.pleat;
        const QPointF copy = origin + plan.step;

        if (!mark.flat)
        {
            segment(mark.point, origin, VColorRole::VisSupportColor3, Qt::SolidLine);
            segment(mark.point, copy, VColorRole::VisSupportColor3, Qt::SolidLine);
        }
    }

    hideLeftovers();
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
