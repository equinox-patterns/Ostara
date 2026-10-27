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
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 18/10/2026.
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
    // "Bold" also while marked for a layer. Read from the flag rather than written into m_isBoldLine, so that
    // dropping the mark gives back exactly the weight the line had before.
    const bool bold = m_isBoldLine || m_layerSelected || m_inLayer;
    const qreal width = ScaleWidth(bold ? VAbstractApplication::VApp()->Settings()->WidthBoldLine()
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
void VScaledLine::SetAuthoredLineStyle(Qt::PenStyle style)
{
    if (m_authoredStyle != style)
    {
        prepareGeometryChange(); // the style drives shape() and boundingRect()
        m_authoredStyle = style;
    }

    // Always converge the pen, even when the remembered style did not change: the owner has just written the
    // style it asked for (VToolLinePoint::RefreshAuthoredPen does), and a "none" line has to be shown instead
    // while it is emphasized — the emphasis may well have been set before this call, and skipping the refresh
    // here is what left the line invisible after a click.
    RefreshPen();
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::SetAuthoredColorProvider(std::function<QColor()> provider)
{
    m_authoredColorProvider = std::move(provider);
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::RefreshPen()
{
    QPen lPen = pen();

    if (m_layerSelected || m_inLayer)
    {
        // Selected: orange, the colour that says what Enter will act on. Otherwise the layer's content: magenta,
        // the colour of a transient multi-selection. A line can be both, and the selection is what has to be
        // readable then, so it is tested first.
        const VColorRole markRole = m_layerSelected ? VColorRole::ActivePointColor : VColorRole::VisSupportColor;
        lPen.setColor(VSceneStylesheet::CorrectToolColor(this, VSceneStylesheet::Color(markRole)));
    }
    else if (m_role != VColorRole::CustomColor)
    {
        lPen.setColor(VSceneStylesheet::CorrectToolColor(this, VSceneStylesheet::Color(m_role)));
    }
    else if (m_colorBeforeLayer.isValid())
    {
        // A CustomColor line carries its own colour — the one of the object that draws through it — so there is
        // no role to fall back on. Without this the orange would stay in the pen for good.
        lPen.setColor(m_colorBeforeLayer);
        m_colorBeforeLayer = QColor();
    }
    else if (m_authoredColorProvider)
    {
        // No mark, no role, nothing saved: the only one who knows what this line is supposed to look like is the
        // object that draws through it. Asking it is what gives the pen a colour it can trust after a mark has
        // been dropped — the mark's colour used to stay in the pen, and nothing could recompute it.
        lPen.setColor(m_authoredColorProvider());
    }

    // A "none" line is invisible, but it has to show while it is emphasized: the point it comes from is
    // hovered or picked, and the line is what says where that point comes from. Same intent as
    // Visualization::LineStyle(), which draws "none" solid while a tool is being created. Writing a visible
    // style into the pen is also what keeps the item honest — QGraphicsLineItem::boundingRect() derives from
    // the virtual shape(), which is empty while the style is "none", and an empty rect makes the scene clip
    // the painting to the arrows. The authored style gives the invisibility back once the emphasis is gone.
    if (m_authoredStyle == Qt::NoPen)
    {
        const bool emphasized = m_isBoldLine || m_layerSelected || m_inLayer;
        const Qt::PenStyle style = emphasized ? Qt::SolidLine : Qt::NoPen;
        if (lPen.style() != style)
        {
            prepareGeometryChange(); // the style drives shape() and boundingRect()
            lPen.setStyle(style);
        }
    }

    lPen.setWidthF(ScaledPenWidth());
    setPen(lPen);
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::SetLayerSelected(bool selected)
{
    if (m_layerSelected == selected)
    {
        return;
    }

    m_layerSelected = selected;
    ApplyLayerMark();
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::SetInLayer(bool inLayer)
{
    if (m_inLayer == inLayer)
    {
        return;
    }

    m_inLayer = inLayer;
    ApplyLayerMark();
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::ApplyLayerMark()
{
    prepareGeometryChange();

    // A CustomColor line carries its own colour — the one of the object that draws through it — so there is no
    // role to fall back on once both marks are gone. Save it, and only on the way in: reading pen() again while
    // already marked would save the mark itself. RefreshPen() hands it back and drops it on the way out.
    if ((m_layerSelected || m_inLayer) && m_role == VColorRole::CustomColor && not m_colorBeforeLayer.isValid())
    {
        m_colorBeforeLayer = pen().color();
    }

    RefreshPen();
    update();
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

    return LineHitPath();
}

//---------------------------------------------------------------------------------------------------------------------
auto VScaledLine::LineHitPath() const -> QPainterPath
{
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
    // A child line belongs to the object drawn through it — a point tool's line, a rectangle's side, a cap's tracé —
    // and its look is that object's business: passing over it changes nothing. The click is what answers for it (it
    // selects the parent), and the parent is what brings its tracés out. A top-level line, the Line tool's own
    // object, keeps its hover feedback: there is no parent to speak for it.
    if (parentItem() == nullptr)
    {
        SetHighlighted(true);
    }
    QGraphicsLineItem::hoverEnterEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    // Symmetrical to hoverEnterEvent: a child line was never highlighted on the way in, so nothing may be dropped on
    // the way out — the drop would take back the bold that the click had asked for.
    if (parentItem() == nullptr)
    {
        SetHighlighted(false);
    }
    QGraphicsLineItem::hoverLeaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Clicking a child line acts like clicking its parent (e.g. a point tool's
    // child line): select and focus the parent so it reacts (PropertyBrowser +
    // Delete key). A line with no selectable parent (e.g. a visualization
    // preview, a top-level line tool, or a construction point the layers keep
    // out of reach) falls through to the default handler: swallowing the click
    // there would take the keyboard focus without selecting anything, and the
    // Delete key would then act on an object the user never picked — instead of
    // the line drawn underneath, which the scene then tries.
    if (QGraphicsItem *parent = parentItem(); parent != nullptr && (parent->flags() & QGraphicsItem::ItemIsSelectable))
    {
        parent->setSelected(true);
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
void VScaledLine::SetSegmentClickHandler(std::function<void()> handler)
{
    m_segmentClickHandler = std::move(handler);
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // A drawn segment answers its own click: the tool that draws it is told, and picks that segment. The test is
    // the one every other pick uses — left button, no double click, release inside the line — so a drag that ends
    // away from the line is not a pick. A line with no handler keeps its old course.
    if (m_segmentClickHandler && IsSelectedByReleaseEvent(this, event))
    {
        m_segmentClickHandler();
        event->accept();
        return;
    }

    QGraphicsLineItem::mouseReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledLine::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    // The right-click counterpart of mousePressEvent above, and it was missing: a line a tool draws for itself —
    // the main line of a line point, the second tracé of a dart cap, a side of a rectangle — is a child of that
    // tool and not an object of its own, so the menu belongs to the tool (Options, add to layer, remove from
    // layer, Delete). Doing nothing here does not hand the event back down to the tool either: the scene accepts
    // it before asking the item (QGraphicsScene::contextMenuEvent), so the right click simply died on the line.
    // VToolLine, a tool in its own right, overrides this and keeps its own menu.
    //
    // The event goes through the scene: QGraphicsItem::contextMenuEvent is protected, so a sibling item cannot
    // be asked directly, and QGraphicsScene::sendEvent is the public door to that handler — measured, not
    // assumed (a handler reached this way really does run).
    if (QGraphicsItem *owner = parentItem(); owner != nullptr && scene() != nullptr)
    {
        scene()->sendEvent(owner, event);
        return;
    }

    QGraphicsLineItem::contextMenuEvent(event);
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
    const qreal width = ScaleWidth(VAbstractApplication::VApp()->Settings()->WidthBoldLine(), scale);

    QPen visPen = pen();

    if (m_role != VColorRole::CustomColor)
    {
        const QColor color = VSceneStylesheet::Color(m_role);
        visPen.setColor(color);
        setBrush(m_pointMode ? QBrush(color) : QBrush(Qt::NoBrush)); // Points filled; construction circles stay outlined
    }

    visPen.setWidthF(width);

    setPen(visPen);
    if (m_pointMode)
    {
        ScaleCircleSize(this, scale);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief SetColorRole repaints the disc in another role's colour — the point being edited turns to the orange of the
 *  active point while the rest of the path keeps the magenta of a visualization. */
void VScaledEllipse::SetColorRole(VColorRole role)
{
    if (m_role == role)
    {
        return;
    }

    m_role = role;
    RefreshScale();
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
