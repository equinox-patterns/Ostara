/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   14 6, 2017
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2017 Valentina project
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

#include "vscenepoint.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/def.h"
#include "vscenestylesheet.h"
#include "../vmisc/vabstractapplication.h"
#include "global.h"
#include "scalesceneitems.h"
#include "vgraphicssimpletextitem.h"

#include <QBrush>
#include <QFont>
#include <QGraphicsScene>
#include <QPainter>
#include <QPen>
#include <QStyle>
#include <QStyleOptionGraphicsItem>

#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
#include "../vmisc/backport/qpainterstateguard.h"
#else
#include <QPainterStateGuard>
#endif

//---------------------------------------------------------------------------------------------------------------------
VScenePoint::VScenePoint(VColorRole role, QGraphicsItem *parent)
  : QGraphicsEllipseItem(parent),
    m_namePoint(new VGraphicsSimpleTextItem(VColorRole::DraftLabelColor, VColorRole::DraftLabelHoverColor, this)),
    m_lineName(new VScaledLine(VColorRole::DraftLabelLineColor, this)),
    m_role(role)
{
    m_lineName->SetBoldLine(false);
    m_lineName->setLine(QLineF(0, 0, 1, 0));
    m_lineName->setVisible(false);

    setBrush(QBrush(Qt::NoBrush));
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsFocusable, true); // For keyboard input focus
    setZValue(1); // Points above curves/lines (Z=0) for hit-test priority
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Do not mutate item/scene state here. Since Qt 6 changing geometry, visibility or
    // selection inside paint() crashes the paint pipeline on Windows. All scale-dependent
    // state lives in RefreshScale(), driven by VMainGraphicsView::ScaleChanged and the tool
    // refresh path.
    if (VAbstractApplication::VApp()->Settings()->GetShowAccuracyRadius())
    {
        QPainterStateGuard const guard(painter);

        QPen pen = painter->pen();
        pen.setWidthF(accuracyPointOnLine / 15);
        pen.setStyle(Qt::DashLine);
        pen.setColor(VSceneStylesheet::ToolStyle().AccuracyRadiusColor());

        painter->setPen(pen);
        painter->drawEllipse(PointRect(accuracyPointOnLine));
    }

    PaintWithFixItemHighlightSelected<QGraphicsEllipseItem>(this, painter, option, widget);
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::RefreshScale()
{
    const qreal scale = SceneScale(scene());

    ScaleMainPenWidth(scale);
    ScaleCircleSize(this, scale);

    if (const VCommonSettings *settings = VAbstractApplication::VApp()->Settings();
        settings->GetPatternLabelFontSize() * scale < minVisibleFontSize)
    {
        m_namePoint->setVisible(false);
        m_lineName->setVisible(false);
    }
    else if (not m_onlyPoint)
    {
        m_namePoint->setVisible(settings->GetShowAllLabels() || m_showLabel);
        RefreshLine();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::RefreshPointGeometry(const VPointF &point)
{
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
    setPos(static_cast<QPointF>(point));
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_showLabel = point.IsShowLabel();

    {
        const QSignalBlocker blocker(m_namePoint);
        m_namePoint->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
        m_namePoint->SetRealPos(QPointF(point.mx(), point.my()));
        m_namePoint->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }

    m_namePoint->setText(point.name());

    RefreshScale();
}

//---------------------------------------------------------------------------------------------------------------------
auto VScenePoint::boundingRect() const -> QRectF
{
    QRectF rect = QGraphicsEllipseItem::boundingRect();

    if (VAbstractApplication::VApp()->Settings()->GetShowAccuracyRadius())
    {
        rect = rect.united(PointRect(accuracyPointOnLine));
    }

    return rect;
}

//---------------------------------------------------------------------------------------------------------------------
auto VScenePoint::shape() const -> QPainterPath
{
    QPainterPath path;
    const qreal scale = SceneScale(scene());
    qreal radius = ScaledRadius(scale);
    // Minimum hit-target radius in screen pixels for comfortable interaction
    constexpr qreal minScreenPixels = 6.0;
    radius = qMax(radius, minScreenPixels / qMax(scale, 0.01));
    path.addEllipse(PointRect(radius));
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::SetOnlyPoint(bool value)
{
    m_onlyPoint = value;
    m_namePoint->setVisible(not m_onlyPoint);
    m_lineName->setVisible(not m_onlyPoint);
}

//---------------------------------------------------------------------------------------------------------------------
auto VScenePoint::IsOnlyPoint() const -> bool
{
    return m_onlyPoint;
}

//---------------------------------------------------------------------------------------------------------------------
auto VScenePoint::itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant
{
    // Size the point for the current view scale as soon as it enters a scene, so it is correct on
    // its first paint (e.g. when interactively creating a tool) instead of flashing at the default
    // size until the next RefreshScale/zoom.
    if (change == ItemSceneHasChanged && value.value<QGraphicsScene *>() != nullptr)
    {
        RefreshScale();
    }
    else if (change == ItemEnabledHasChanged)
    {
        // CorrectToolColor() depends on isEnabled(); re-resolve the pen now that the enabled state
        // is final, otherwise a point disabled at refresh time (e.g. while switching draw/details
        // mode) keeps its DisabledColor until the next hover/zoom.
        ScaleMainPenWidth(SceneScale(scene()));
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    m_isHovered = true;
    RefreshHoverPen();
    QGraphicsEllipseItem::hoverEnterEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_isHovered = false;
    RefreshHoverPen();
    QGraphicsEllipseItem::hoverLeaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::RefreshHoverPen()
{
    // paint() no longer recomputes the pen, so re-apply the hovered/un-hovered main pen width for
    // the current scale here. Otherwise the hover highlight would not appear until the next zoom.
    ScaleMainPenWidth(SceneScale(scene()));
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::SetActivePoint(bool active)
{
    m_selected = active;
    RefreshHoverPen();
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::SetTemporarySelected(bool selected)
{
    m_temporarySelected = selected;
    RefreshHoverPen();
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::SetLayerSelected(bool selected)
{
    if (m_layerSelected == selected)
    {
        return;
    }

    m_layerSelected = selected;
    RefreshHoverPen();
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::SetInLayer(bool inLayer)
{
    if (m_inLayer == inLayer)
    {
        return;
    }

    m_inLayer = inLayer;
    RefreshHoverPen();
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::RefreshLine()
{
    if (m_onlyPoint)
    {
        if (m_lineName != nullptr)
        {
            m_lineName->setVisible(false);
        }
        return;
    }

    QRectF nRec = m_namePoint->sceneBoundingRect();
    nRec.translate(-scenePos());
    if (not rect().intersects(nRec))
    {
        const QRectF nameRec = m_namePoint->sceneBoundingRect();
        QPointF p1;
        QPointF p2;
        VGObject::LineIntersectCircle(QPointF(), ScaledRadius(SceneScale(scene())),
                                      QLineF(QPointF(), nameRec.center() - scenePos()), p1, p2);
        const QPointF pRec = VGObject::LineIntersectRect(nameRec, QLineF(scenePos(), nameRec.center()));

        if (QLineF(p1, pRec - scenePos()).length() <= ToPixel(4 / qMax(1.0, SceneScale(scene())), Unit::Mm))
        {
            m_lineName->setVisible(false);
        }
        else
        {
            m_lineName->setLine(QLineF(p1, pRec - scenePos()));
            m_lineName->setVisible(VAbstractApplication::VApp()->Settings()->GetShowAllLabels() || m_showLabel);
        }
    }
    else
    {
        m_lineName->setVisible(false);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VScenePoint::GetLabelLineColorRole() const -> VColorRole
{
    return m_lineName->GetColorRole();
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::SetLabelLineColorRole(VColorRole role)
{
    m_lineName->SetColorRole(role);
}

//---------------------------------------------------------------------------------------------------------------------
auto VScenePoint::GetLabelTextColorRole() const -> VColorRole
{
    return m_namePoint->GetTextColor();
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::SetLabelTextColorRole(VColorRole role)
{
    m_namePoint->SetTextColor(role);
}

//---------------------------------------------------------------------------------------------------------------------
auto VScenePoint::GetLabelTextHoverColorRole() const -> VColorRole
{
    return m_namePoint->GetTextHoverColor();
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::SetLabelTextHoverColorRole(VColorRole role)
{
    m_namePoint->SetTextHoverColor(role);
}

//---------------------------------------------------------------------------------------------------------------------
void VScenePoint::ScaleMainPenWidth(qreal scale)
{
    // An active point (property browser open) keeps its highlighted look even when not hovered.
    const bool hovered = m_isHovered || m_selected || m_temporarySelected || m_layerSelected || m_inLayer;

    const qreal width = ScaleWidth(hovered ? VAbstractApplication::VApp()->Settings()->WidthBoldLine()
                                           : VAbstractApplication::VApp()->Settings()->WidthHairLine(),
                                   scale);

    QPen pointPen = pen();
    if (m_role != VColorRole::CustomColor)
    {
        // Priority: selected (orange) > in the layer (magenta) > transient multi-selection (magenta) >
        // persistent active (orange) > hover (magenta). The two layer states come first on purpose: a hover would
        // otherwise paint magenta over the mark the user has just set by clicking. Between them the selection
        // wins, because an object can be both — it is in the layer being emptied AND picked to leave it — and
        // what Enter will act on is the one that has to be readable.
        const bool layerSelected = m_layerSelected && not m_alwaysHovered;
        const bool inLayer = m_inLayer && not m_alwaysHovered;
        const bool active = m_selected && not m_alwaysHovered;
        const bool transient = m_temporarySelected && not m_alwaysHovered;
        const bool hoverColorActive = hovered && not m_alwaysHovered;
        VColorRole colorRole = m_role;
        if (layerSelected)
        {
            colorRole = VColorRole::ActivePointColor;
        }
        else if (inLayer)
        {
            colorRole = VColorRole::VisSupportColor;
        }
        else if (transient)
        {
            colorRole = VColorRole::VisSupportColor;
        }
        else if (active)
        {
            colorRole = VColorRole::ActivePointColor;
        }
        else if (hoverColorActive)
        {
            colorRole = VColorRole::VisSupportColor;
        }
        const QColor color = VSceneStylesheet::CorrectToolColor(this, VSceneStylesheet::Color(colorRole));
        pointPen.setColor(color);
        // Hover/active points and Draw-mode points stay filled. Details-mode piece points
        // are outline-only at rest: the fill is removed so only the outline shows.
        if (hoverColorActive || transient || m_fillPoints)
        {
            setBrush(QBrush(color)); // Fill inherits the role color (outline + fill = same color)
        }
        else
        {
            setBrush(Qt::NoBrush);
        }
    }
    pointPen.setWidthF(width);
    setPen(pointPen);
}
