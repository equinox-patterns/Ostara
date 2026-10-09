/************************************************************************
 **
 **  @file   vsimplecurve.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   7 5, 2016
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

#include "vsimplecurve.h"

#include <QBrush>
#include <QFlags>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMessageLogger>
#include <QPainterPath>
#include <QPen>
#include <QtDebug>

#include "../ifc/ifcdef.h"
#include "../vgeometry/vabstractcurve.h"
#include "vscenestylesheet.h"
#include "../vmisc/vabstractapplication.h"
#include "global.h"

template <class T> class QSharedPointer;

//---------------------------------------------------------------------------------------------------------------------
VSimpleCurve::VSimpleCurve(quint32 id, const QSharedPointer<VAbstractCurve> &curve, QObject *parent)
  : VAbstractSimple(id, parent),
    VCurvePathItem(VColorRole::CustomColor),
    m_curve(curve),
    m_isHovered(false)
{
    setBrush(QBrush(Qt::NoBrush));
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsFocusable, true); // For keyboard input focus
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::RefreshGeometry(const QSharedPointer<VAbstractCurve> &curve)
{
    m_curve = curve;

    if (not m_curve.isNull())
    {
        m_isHovered ? SetDirectionArrows(m_curve->DirectionArrows()) : SetDirectionArrows(QVector<DirectionArrow>());
        setPath(m_curve->GetPath());
        SetPoints(m_curve->GetPoints());
        ScalePenWidth();
    }
    else
    {
        qWarning() << "VSimpleCurve::RefreshGeometry: pointer to curve is null.";
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::TriggerChoosed()
{
    emit Choosed(id);
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::CurveChoosed()
{
    emit Choosed(id);
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::CurveSelected(bool selected)
{
    emit Selected(selected, id);
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Special for not selectable item first need to call standard mousePressEvent then accept event
    QGraphicsPathItem::mousePressEvent(event);

    // Somehow clicking on notselectable object do not clean previous selections.
    if (not(flags() & ItemIsSelectable) && scene())
    {
        scene()->clearSelection();
    }

    if (m_selectionType == SelectionType::ByMouseRelease)
    {
        event
            ->accept(); // Special for not selectable item first need to call standard mousePressEvent then accept event
    }
    else
    {
        if (event->button() == Qt::LeftButton && event->type() != QEvent::GraphicsSceneMouseDoubleClick)
        {
            emit Choosed(id);
            event->accept();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (m_selectionType == SelectionType::ByMouseRelease && IsSelectedByReleaseEvent(this, event))
    {
        emit Choosed(id);
    }
    QGraphicsPathItem::mouseReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    m_isHovered = true;
    if (not m_curve.isNull())
    {
        SetDirectionArrows(m_curve->DirectionArrows());
    }
    ScalePenWidth();
    QGraphicsPathItem::hoverEnterEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    m_isHovered = false;
    SetDirectionArrows(QVector<DirectionArrow>());
    ScalePenWidth();
    QGraphicsPathItem::hoverLeaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto VSimpleCurve::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) -> QVariant
{
    if (change == QGraphicsItem::ItemSceneHasChanged && value.value<QGraphicsScene *>() != nullptr)
    {
        ScalePenWidth();
    }
    else if (change == QGraphicsItem::ItemEnabledHasChanged)
    {
        // CorrectToolColor() depends on isEnabled(); re-resolve the pen now that the enabled state
        // is final, otherwise a curve disabled at refresh time (e.g. while switching draw/details
        // mode) keeps its DisabledColor until the next hover/zoom.
        ScalePenWidth();
    }
    else if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        emit Selected(value.toBool(), id);
    }

    return QGraphicsPathItem::itemChange(change, value);
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    emit ShowContextMenu(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Delete:
            emit Delete();
            return; // Leave this method immediately after call!!!
        default:
            break;
    }
    QGraphicsPathItem::keyReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::SetLayerSelected(bool selected)
{
    if (m_layerSelected == selected)
    {
        return;
    }

    m_layerSelected = selected;
    ScalePenWidth();
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::SetInLayer(bool inLayer)
{
    if (m_inLayer == inLayer)
    {
        return;
    }

    m_inLayer = inLayer;
    ScalePenWidth();
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void VSimpleCurve::ScalePenWidth()
{
    // The layer marks are read here rather than written into the pen, so that dropping them gives back exactly the
    // width and the colour the curve had before.
    const bool marked = m_layerSelected || m_inLayer;

    qreal width = (m_isHovered || marked) ? VAbstractApplication::VApp()->Settings()->WidthBoldLine()
                                          : VAbstractApplication::VApp()->Settings()->WidthHairLine();
    width = ScaleWidth(width, SceneScale(scene()));

    QPen curvePen = pen();
    if (m_layerSelected)
    {
        // Selected: orange, the colour that says what Enter will act on.
        curvePen.setColor(VSceneStylesheet::Color(VColorRole::ActivePointColor));
    }
    else if (m_inLayer)
    {
        // The layer's content: magenta, the colour of a transient multi-selection.
        curvePen.setColor(VSceneStylesheet::Color(VColorRole::VisSupportColor));
    }
    else
    {
        curvePen.setColor(VSceneStylesheet::CorrectToolColor(
            this, VSceneStylesheet::CorrectToolColorForDarkTheme(m_curve->GetColor())));
    }
    curvePen.setWidthF(width);
    curvePen.setStyle(LineStyleToPenStyle(m_curve->GetPenStyle()));
    setPen(curvePen);
}
