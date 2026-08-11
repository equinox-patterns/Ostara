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

#include <QGraphicsSceneHoverEvent>
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
void VScaledLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPen lPen = pen();

    if (m_role != VColorRole::CustomColor)
    {
        lPen.setColor(VSceneStylesheet::CorrectToolColor(this, VSceneStylesheet::Color(m_role)));
    }

    const qreal width = ScaleWidth(m_isBoldLine ? VAbstractApplication::VApp()->Settings()->WidthMainLine()
                                                : VAbstractApplication::VApp()->Settings()->WidthHairLine(),
                                   SceneScale(scene()));
    lPen.setWidthF(qRound(width * 100.) / 100.);
    setPen(lPen);

    PaintWithFixItemHighlightSelected<QGraphicsLineItem>(this, painter, option, widget);

    // Draw direction arrows when bold (hovered or active with visualization)
    // Skip if line style is NoPen (invisible/transparent)
    if (m_showArrows && m_isBoldLine && lPen.style() != Qt::NoPen)
    {
        const QLineF l = line();
        const qreal len = l.length();
        if (len > 0)
        {
            const QPointF mid = l.pointAt(0.5);
            const qreal angle = l.angle();
            const qreal arrowLen = ScaleWidth(
                VAbstractApplication::VApp()->Settings()->GetLineWidth() * 8.0, SceneScale(scene()));

            QPainterPath arrowPath;
            // Arrow fans open backward (toward A), tip points forward (toward B)
            // l.angle() = A→B direction. Add 180° so branches fan toward A.
            const qreal fanBase = angle + 180;
            auto addBranch = [mid, &arrowPath](qreal a, qreal length) {
                QLineF branch(mid, QPointF(mid.x() + 1, mid.y())); // non-zero init for setAngle
                branch.setAngle(a);
                branch.setLength(length);
                arrowPath.moveTo(branch.p1());
                arrowPath.lineTo(branch.p2());
            };
            addBranch(fanBase - 35, arrowLen);
            addBranch(fanBase + 35, arrowLen);

            QPainterStateGuard const guard(painter);

            QPen arrowPen(lPen);
            arrowPen.setStyle(Qt::SolidLine);
            painter->setPen(arrowPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawPath(arrowPath);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
VScaledEllipse::VScaledEllipse(VColorRole role, QGraphicsItem *parent)
  : QGraphicsEllipseItem(parent),
    m_role(role)
{
}

//---------------------------------------------------------------------------------------------------------------------
void VScaledEllipse::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    const qreal scale = SceneScale(scene());
    const qreal width = ScaleWidth(VAbstractApplication::VApp()->Settings()->WidthMainLine(), scale);

    QPen visPen = pen();

    if (m_role != VColorRole::CustomColor)
    {
        visPen.setColor(VSceneStylesheet::Color(m_role));
    }

    visPen.setWidthF(width);

    setPen(visPen);
    if (m_pointMode)
    {
        ScaleCircleSize(this, scale);
    }

    PaintWithFixItemHighlightSelected<QGraphicsEllipseItem>(this, painter, option, widget);
}
