/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   21 6, 2017
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

#include "scalesceneitems.h"
#include "vscenestylesheet.h"
#include "../vmisc/vabstractapplication.h"
#include "../vbackport/qpainterstateguard.h"
#include "global.h"

#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainterPath>
#include <QPen>

//---------------------------------------------------------------------------------------------------------------------
VScaledLine::VScaledLine(VColorRole role, QGraphicsItem *parent)
  : QGraphicsLineItem(parent),
    m_role(role)
{
}

//---------------------------------------------------------------------------------------------------------------------
VScaledLine::VScaledLine(const QLineF &line, VColorRole role, QGraphicsItem *parent)
  : QGraphicsLineItem(line, parent),
    m_role(role)
{
}

//---------------------------------------------------------------------------------------------------------------------
auto VScaledLine::ScaledPenWidth() const -> qreal
{
    const qreal width = ScaleWidth(m_isBoldLine ? VAbstractApplication::VApp()->Settings()->WidthMainLine()
                                                : VAbstractApplication::VApp()->Settings()->WidthHairLine(),
                                   SceneScale(scene()));
    return qRound(width * 100.) / 100.;
}

//---------------------------------------------------------------------------------------------------------------------
auto VScaledLine::ArrowsPath() const -> QPainterPath
{
    QPainterPath arrowPath;

    if (!m_showArrows || !m_isBoldLine || pen().style() == Qt::NoPen)
    {
        return arrowPath;
    }

    const QLineF l = line();
    const qreal len = l.length();
    if (len <= 0)
    {
        return arrowPath;
    }

    // Dual arrows at 25% and 75% — avoids collision with a midpoint created by the user
    const QPointF mid1 = l.pointAt(0.25);
    const QPointF mid2 = l.pointAt(0.75);
    const qreal angle = l.angle();
    const qreal arrowLen = ScaleWidth(
        VAbstractApplication::VApp()->Settings()->GetLineWidth() * 8.0, SceneScale(scene()));

    // Arrow fans open backward (toward A), tip points forward (toward B)
    // l.angle() = A→B direction. Add 180° so branches fan toward A.
    const qreal fanBase = angle + 180;
    auto addBranch = [&arrowPath](const QPointF &origin, qreal a, qreal length) {
        QLineF branch(origin, QPointF(origin.x() + 1, origin.y())); // non-zero init for setAngle
        branch.setAngle(a);
        branch.setLength(length);
        arrowPath.moveTo(branch.p1());
        arrowPath.lineTo(branch.p2());
    };
    addBranch(mid1, fanBase - 35, arrowLen);
    addBranch(mid1, fanBase + 35, arrowLen);
    addBranch(mid2, fanBase - 35, arrowLen);
    addBranch(mid2, fanBase + 35, arrowLen);

    return arrowPath;
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::RefreshPen()
{
    QPen lPen = pen();

    if (m_role != VColorRole::CustomColor)
    {
        lPen.setColor(VSceneStylesheet::CorrectToolColor(this, VSceneStylesheet::Color(m_role)));
    }

    lPen.setWidthF(ScaledPenWidth());
    setPen(lPen);
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::SetHighlighted(bool on)
{
    prepareGeometryChange();
    SetBoldLine(on);
    RefreshPen();
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    PaintWithFixItemHighlightSelected<QGraphicsLineItem>(this, painter, option, widget);

    // Draw direction arrows when bold (hovered or active with visualization)
    const QPainterPath arrowPath = ArrowsPath();
    if (!arrowPath.isEmpty())
    {
        QPainterStateGuard const guard(painter);

        QPen arrowPen(pen());
        arrowPen.setStyle(Qt::SolidLine);
        painter->setPen(arrowPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(arrowPath);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VScaledLine::shape() const -> QPainterPath
{
    if (pen().style() == Qt::NoPen)
    {
        // Invisible line (type "none"): not hit-testable, so it neither captures hover nor
        // clicks. The parent point stays fully interactive.
        return {};
    }

    QPainterPath itemPath;
    itemPath.moveTo(line().p1());
    itemPath.lineTo(line().p2());

    // Comfortable hit area: at least 6 screen pixels wide regardless of zoom, so a hairline
    // (WidthHairLine) is still easy to hover/click. Same philosophy as VScenePoint::shape().
    QPen shapePen = pen();
    const qreal scale = SceneScale(scene());
    constexpr qreal minScreenPixels = 6.0;
    const qreal minWidth = minScreenPixels / qMax(scale, 0.01);
    if (shapePen.widthF() < minWidth)
    {
        shapePen.setWidthF(minWidth);
    }

    return ItemShapeFromPath(itemPath, shapePen);
}

//---------------------------------------------------------------------------------------------------------------------
auto VScaledLine::boundingRect() const -> QRectF
{
    QRectF rect = QGraphicsLineItem::boundingRect();

    const QPainterPath arrowPath = ArrowsPath();
    if (!arrowPath.isEmpty())
    {
        QPainterPathStroker stroker;
        stroker.setWidth(ScaledPenWidth());
        rect = rect.united(stroker.createStroke(arrowPath).boundingRect());
    }

    return rect;
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    SetHighlighted(true);
    QGraphicsLineItem::hoverEnterEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    SetHighlighted(false);
    QGraphicsLineItem::hoverLeaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Clicking a child line acts like clicking its parent (e.g. a point tool's
    // child line): select and focus the parent so it reacts (PropertyBrowser +
    // Delete key). A line with no selectable parent (e.g. a visualization
    // preview, or a top-level line tool) falls through to the default handler.
    if (QGraphicsItem *parent = parentItem())
    {
        if (parent->flags() & QGraphicsItem::ItemIsSelectable)
        {
            parent->setSelected(true);
        }
        if (parent->flags() & QGraphicsItem::ItemIsFocusable)
        {
            parent->setFocus(Qt::MouseFocusReason);
        }
        event->accept();
        return;
    }
    QGraphicsLineItem::mousePressEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto VScaledLine::itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant
{
    if (change == ItemSceneHasChanged && value.value<QGraphicsScene *>() != nullptr)
    {
        RefreshPen();
    }
    else if (change == ItemEnabledHasChanged)
    {
        RefreshPen();
    }
    return QGraphicsLineItem::itemChange(change, value);
}

//---------------------------------------------------------------------------------------------------------------------
VScaledEllipse::VScaledEllipse(VColorRole role, QGraphicsItem *parent)
  : QGraphicsEllipseItem(parent),
    m_role(role)
{
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledEllipse::RefreshScale()
{
    const qreal scale = SceneScale(scene());
    const qreal width = ScaleWidth(VAbstractApplication::VApp()->Settings()->WidthMainLine(), scale);

    QPen visPen = pen();

    if (m_role != VColorRole::CustomColor)
    {
        const QColor color = VSceneStylesheet::Color(m_role);
        visPen.setColor(color);
        setBrush(QBrush(color)); // Fill follows the outline (active state = whole point, not just contour)
    }

    visPen.setWidthF(width);

    setPen(visPen);
    if (m_pointMode)
    {
        ScaleCircleSize(this, scale);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledEllipse::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    PaintWithFixItemHighlightSelected<QGraphicsEllipseItem>(this, painter, option, widget);
}

//---------------------------------------------------------------------------------------------------------------------
auto VScaledEllipse::itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant
{
    if (change == ItemSceneHasChanged && value.value<QGraphicsScene *>() != nullptr)
    {
        RefreshScale();
    }
    return QGraphicsEllipseItem::itemChange(change, value);
}
