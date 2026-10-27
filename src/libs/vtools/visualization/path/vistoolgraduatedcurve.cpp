/************************************************************************
 **
 **  @file   vistoolgraduatedcurve.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 12, 2025
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2025 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 09/10/2026.
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
#include "vistoolgraduatedcurve.h"

#include <QGraphicsScene>
#include <QLineF>
#include <QPainterPath>
#include <QtGlobal>

#include <cmath>
#include <utility>

#include "../../tools/toolsdef.h"
#include "../vgeometry/ooffsetstations.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vsplinepath.h"
#include "../vmisc/vmodifierkey.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "../vwidgets/scalesceneitems.h"
#include "../vwidgets/vgraphicssimpletextitem.h"

namespace
{
// How far a station label hangs off the point it names, in scene units, clear of the disc.
constexpr qreal kStationLabelGap = 20.0;
// The marks of the stations read on top of both curves: they say where a width acts, they are not part of the curve.
constexpr qreal kStationZ = 2.0;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
VisToolGraduatedCurve::VisToolGraduatedCurve(const VContainer *data, QGraphicsItem *parent)
  : VisPath(data, parent)
{
    m_originalCurve = InitItem<VCurvePathItem>(VColorRole::VisSupportColor, this);
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolGraduatedCurve::RefreshGeometry()
{
    if (m_curveId == NULL_ID)
    {
        return;
    }

    const QSharedPointer<VAbstractCurve> curve = GetData()->GeometricObject<VAbstractCurve>(m_curveId);
    DrawPath(m_originalCurve, curve->GetPath(), curve->DirectionArrows(), LineStyle(), Qt::RoundCap);

    QVector<qreal> widths = m_offsets;
    int activeOffset = m_activeOffset;

    if (GetMode() == Mode::Show && !m_offsets.isEmpty())
    {
        VSplinePath splPath = curve->Outline(m_offsets);
        splPath.SetApproximationScale(ApproximationScale());

        DrawPath(this, splPath.GetPath(), splPath.DirectionArrows(), LineStyle(), Qt::RoundCap);
    }
    else if (GetMode() == Mode::Creation)
    {
        QPointF const p = curve->ClosestPoint(ScenePos());

        qreal width = QLineF(p, ScenePos()).length();
        if (QGuiApplication::keyboardModifiers() == Qt::ShiftModifier)
        {
            width = -width;
        }

        widths.append(width);
        // The width under the cursor is the one being placed: it is the one to read on the canvas.
        activeOffset = static_cast<int>(widths.size()) - 1;

        VSplinePath splPath = curve->Outline(widths);
        splPath.SetApproximationScale(ApproximationScale());

        DrawPath(this, splPath.GetPath(), splPath.DirectionArrows(), LineStyle(), Qt::RoundCap);

        static const QString suffix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

        auto PrepareOffsetValues = [](const QVector<qreal> &offsets) -> QString
        {
            QStringList values;

            for (const auto &offset : offsets)
            {
                values.append(LengthToUser(offset) + suffix);
            }

            QString str = values.join(';');

            if (values.size() > 1)
            {
                str.prepend('(');
                str.append(')');
            }

            return str;
        };

        SetToolTip(tr("<b>Graduated curve</b>: offsets = %1; "
                      "<b>Mouse click</b> - selecting next width, "
                      "<b>%2</b> - negative width, "
                      "<b>%3</b> - finish")
                       .arg(PrepareOffsetValues(widths), VModifierKey::Shift(), VModifierKey::EnterKey()));
    }
    else
    {
        // No width at all is not an offset: leaving the path of the previous refresh would keep a curve on screen
        // that nothing holds any more.
        DrawPath(this, QPainterPath(), QVector<DirectionArrow>(), LineStyle(), Qt::RoundCap);
        RefreshStations(*curve, {}, -1);
        return;
    }

    RefreshStations(*curve, widths, activeOffset);
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolGraduatedCurve::VisualMode(quint32 id)
{
    m_curveId = id;
    StartVisualMode();
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolGraduatedCurve::SetOffsets(const QVector<VRawGraduatedCurveOffset> &offsets)
{
    m_offsets.clear();
    m_offsets.reserve(offsets.size());

    VContainer localData = *GetData();
    // Own table first: the copy shares the live pattern's variables, and the offsets added below are working
    // values of this visualization.
    localData.DetachVariables();

    for (const auto &offset : offsets)
    {
        qreal const width = FindLengthFromUser(offset.formula, localData.DataVariables());
        m_offsets.append(width);

        auto *offsetVal = new VIncrement(&localData, offset.name);
        offsetVal->SetFormula(width, offset.formula, true);

        localData.AddVariable(offsetVal);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief SetActiveOffset paints the marks of the width being edited — the rank of the row in the dialog's table —
 *  in the orange of the active point, and gives every other one back the green of an auxiliary construction. */
void VisToolGraduatedCurve::SetActiveOffset(int index)
{
    m_activeOffset = index;
    ApplyStationColors();
}

//---------------------------------------------------------------------------------------------------------------------
void VisToolGraduatedCurve::ApplyStationColors()
{
    for (const StationItems &items : std::as_const(m_stations))
    {
        const VColorRole role =
            items.index == m_activeOffset ? VColorRole::ActivePointColor : VColorRole::VisSupportColor2;

        items.anchor->SetColorRole(role);
        items.target->SetColorRole(role);

        if (items.normal->GetColorRole() != role)
        {
            items.normal->SetColorRole(role); // the pen of a line is only rewritten on a refresh
            items.normal->RefreshPen();
        }

        items.rank->SetTextColor(role);
        items.rank->RefreshColor();
        items.value->SetTextColor(role);
        items.value->RefreshColor();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RefreshStations marks, on the base curve, every point a width is measured from, and the normal that carries
 * that width to the result curve. The rank of the width and its value are written beside, so that a row of the
 * dialog's table and a place on the curve are the same thing.
 */
void VisToolGraduatedCurve::RefreshStations(const VAbstractCurve &curve, const QVector<qreal> &widths, int activeOffset)
{
    m_activeOffset = activeOffset;

    const QVector<OOffsetStation> stations = OOffsetStations::Stations(curve, widths);

    while (m_stations.size() < stations.size())
    {
        StationItems items;
        // The green of an auxiliary construction, not the magenta of a point: the base curve is magenta already, and
        // a mark of the same colour on top of it would be the one thing not to be read.
        items.anchor = InitPoint(VColorRole::VisSupportColor2, this, kStationZ);
        items.target = InitPoint(VColorRole::VisSupportColor2, this, kStationZ);
        items.normal = InitItem<VScaledLine>(VColorRole::VisSupportColor2, this);
        // A normal is the answer to "where does this width go": behind the curves it would be the one thing hidden.
        items.normal->setFlag(QGraphicsItem::ItemStacksBehindParent, false);
        items.normal->setZValue(kStationZ);
        items.normal->SetBoldLine(false);
        items.rank = MakeStationLabel();
        items.value = MakeStationLabel();
        m_stations.append(items);
    }

    for (qsizetype i = 0; i < stations.size(); ++i)
    {
        const OOffsetStation &station = stations.at(i);
        StationItems &items = m_stations[i];

        const QPointF direction = station.target - station.base;
        const qreal length = std::hypot(direction.x(), direction.y());
        // A width of nil leaves no direction at all: the label still needs a side to hang from.
        const QPointF outside = qFuzzyIsNull(length) ? QPointF(0, -1) : direction / length;

        items.index = station.index;
        items.anchor->setPos(station.base);
        items.target->setPos(station.target);
        items.normal->setLine(QLineF(station.base, station.target));

        // The rank stays on the side the width comes from, the value on the far side of where it lands: the two
        // never meet, whatever the width, even nil.
        items.rank->setText(QString::number(station.index + 1));
        PlaceStationLabel(items.rank, station.base, -outside);
        items.value->setText(LengthToUser(widths.at(station.index)));
        PlaceStationLabel(items.value, station.target, outside);

        items.anchor->setVisible(true);
        items.target->setVisible(true);
        items.normal->setVisible(true);
        items.rank->setVisible(true);
        items.value->setVisible(true);
    }

    for (qsizetype i = stations.size(); i < m_stations.size(); ++i)
    {
        const StationItems &items = m_stations.at(i);
        items.anchor->setVisible(false);
        items.target->setVisible(false);
        items.normal->setVisible(false);
        items.rank->setVisible(false);
        items.value->setVisible(false);
    }

    ApplyStationColors();
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief A label of the canvas: read-only, so that a click goes through it to the curve underneath. */
auto VisToolGraduatedCurve::MakeStationLabel() -> VGraphicsSimpleTextItem *
{
    auto *label = new VGraphicsSimpleTextItem(VColorRole::VisSupportColor2, VColorRole::VisSupportColor2, this);
    label->setAcceptedMouseButtons(Qt::NoButton);
    label->setFlag(QGraphicsItem::ItemIsMovable, false);
    label->setFlag(QGraphicsItem::ItemIsSelectable, false);
    label->setFlag(QGraphicsItem::ItemIsFocusable, false);
    label->setAcceptHoverEvents(false);
    label->setZValue(kStationZ);
    label->setVisible(false);
    return label;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief PlaceStationLabel hangs a label off a point, along a direction, clear of the disc it names. The item is
 *  anchored by its top-left corner, so it is the centre that is placed. */
void VisToolGraduatedCurve::PlaceStationLabel(VGraphicsSimpleTextItem *label, const QPointF &from, const QPointF &away)
{
    label->SetDestination(from);
    label->RefreshScale(); // a label created under a parent already in the scene never hears about the scale

    const QSizeF size = label->boundingRect().size() * label->scale();
    const qreal gap = ScaledRadius(SceneScale(scene())) + kStationLabelGap;
    const QPointF centre = from + away * (gap + qMax(size.width(), size.height()) / 2);

    label->SetRealPos(centre - QPointF(size.width() / 2, size.height() / 2));
}
