/************************************************************************
 **
 **  @file   ovistoolrectangle.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  preview of a rectangle, with its two lengths told apart
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

#include "ovistoolrectangle.h"

#include <QLineF>
#include <QPointF>
#include <QSharedPointer>

#include "../../tools/drawTools/toolpoint/tooldoublepoint/orectanglerule.h"
#include "../vgeometry/vpointf.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtheme/vscenestylesheet.h"
#include "visline.h"

//---------------------------------------------------------------------------------------------------------------------
OVisToolRectangle::OVisToolRectangle(const VContainer *data, QGraphicsItem *parent)
  : VisLine(data, parent)
{
    SetColorRole(VColorRole::VisSupportColor);

    m_pointA = InitPoint(VColorRole::VisSupportColor, this);
    m_pointB = InitPoint(VColorRole::VisSupportColor, this);
    m_pointC = InitPoint(VColorRole::VisSupportColor, this);
    m_pointD = InitPoint(VColorRole::VisSupportColor, this);

    m_lineWidth = InitItem<VScaledLine>(VColorRole::VisSupportColor2, this);
    m_lineHeight = InitItem<VScaledLine>(VColorRole::VisSupportColor4, this);
    m_lineCD = InitItem<VScaledLine>(VColorRole::VisSupportColor, this);
    m_lineDA = InitItem<VScaledLine>(VColorRole::VisSupportColor, this);

    // The arrows stay: a side of a rectangle is read from one end to the other, like every other line of the
    // pattern, and the preview is where that is learnt.
    for (VScaledLine *line : {m_lineWidth, m_lineHeight, m_lineCD, m_lineDA})
    {
        line->SetBoldLine(false);
        line->RefreshPen();
    }

    // Set after the loop, so the bold flag they finally keep is the one the pen was refreshed with.
    for (VScaledLine *line : {m_lineWidth, m_lineHeight})
    {
        line->SetBoldLine(true);
        line->RefreshPen();
    }

    ApplyPens();
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolRectangle::RefreshGeometry()
{
    if (m_basePointId <= NULL_ID)
    {
        return;
    }

    const QSharedPointer<VPointF> baseP = GetData()->GeometricObject<VPointF>(m_basePointId);
    const QPointF a = static_cast<QPointF>(*baseP);
    DrawPoint(m_pointA, a);

    if (GetMode() != Mode::Show)
    {
        return;
    }

    // A flat rectangle is a rectangle: the two lengths are drawn as soon as they are known, so the user sees
    // the shape settle while typing. Only a null side makes the whole thing unreadable.
    if (qFuzzyIsNull(m_width) && qFuzzyIsNull(m_height))
    {
        return;
    }

    const ORectangleRule::Corners corners = ORectangleRule::Build(a, m_width, m_height);

    DrawPoint(m_pointB, corners.b);
    DrawPoint(m_pointC, corners.c);
    DrawPoint(m_pointD, corners.d);

    // The pen each side carries is the one ApplyPens() set — style and colour alike. DrawLine is not used
    // here: it would take the style as an argument and overwrite the one that was chosen.
    auto DrawSide = [](VScaledLine *line, const QLineF &segment) -> void
    {
        line->setLine(segment);
        line->setVisible(not segment.isNull());
    };

    DrawSide(m_lineWidth, QLineF(corners.a, corners.b));
    DrawSide(m_lineHeight, QLineF(corners.b, corners.c));
    DrawSide(m_lineCD, QLineF(corners.c, corners.d));
    DrawSide(m_lineDA, QLineF(corners.d, corners.a));
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolRectangle::SetTypeLine(const QString &typeLine)
{
    m_typeLine = typeLine;
    ApplyPens();
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolRectangle::SetLineColor(const QString &color)
{
    m_lineColor = color;
    ApplyPens();
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolRectangle::ApplyPens()
{
    // The width and the height carry the two colours of the pastilles; the two other sides carry the one the
    // user chose. All four follow the chosen style, so the preview and the created rectangle look alike.
    const Qt::PenStyle style = LineStyleToPenStyle(m_typeLine);

    auto ApplyPen = [style](VScaledLine *line, const QColor &color) -> void
    {
        QPen linePen = line->pen();
        linePen.setColor(color);
        linePen.setStyle(style);
        line->setPen(linePen);
    };

    const QColor chosen = VSceneStylesheet::CorrectToolColorForDarkTheme(QColor(m_lineColor));

    ApplyPen(m_lineWidth, VSceneStylesheet::Color(VColorRole::VisSupportColor2));
    ApplyPen(m_lineHeight, VSceneStylesheet::Color(VColorRole::VisSupportColor4));
    ApplyPen(m_lineCD, chosen);
    ApplyPen(m_lineDA, chosen);
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolRectangle::SetFormulaWidth(const QString &expression)
{
    m_width = FindLengthFromUser(expression, GetData()->DataVariables());
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolRectangle::SetFormulaHeight(const QString &expression)
{
    m_height = FindLengthFromUser(expression, GetData()->DataVariables());
}

//---------------------------------------------------------------------------------------------------------------------
void OVisToolRectangle::VisualMode(quint32 id)
{
    m_basePointId = id;
    StartVisualMode();
}
