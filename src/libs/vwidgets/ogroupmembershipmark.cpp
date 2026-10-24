/************************************************************************
 **
 **  @file   ogroupmembershipmark.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 October, 2026
 **
 **  @brief  The mark a group member carries: a dashed frame and "Member of <group>".
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
#include "ogroupmembershipmark.h"

#include <QCoreApplication>
#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPen>

#include <algorithm>

namespace
{
const QColor kMarkGrey(128, 128, 128, 170);
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OGroupMark::Caption(const QString &groupName) -> QString
{
    // The strings keep the context of the Details tool: the two carriers say the same thing, so
    // they must resolve to the same translation, not to two entries with one wording.
    return groupName.isEmpty() ? QCoreApplication::translate("VToolSeamAllowance", "Member of a group")
                               : QCoreApplication::translate("VToolSeamAllowance", "Member of %1")
                                     .arg(groupName);
}

//---------------------------------------------------------------------------------------------------------------------
auto OGroupMark::FontPixelSize(const Unit &unit) -> qreal
{
    return std::max(8, qRound(ToPixel(0.4, unit)));
}

//---------------------------------------------------------------------------------------------------------------------
void OGroupMark::Paint(QPainter *painter, const QRectF &rect, const QString &caption, const Unit &unit)
{
    if (painter == nullptr || rect.isNull())
    {
        return;
    }

    painter->save();
    QPen framePen(kMarkGrey);
    framePen.setStyle(Qt::DashLine);
    framePen.setWidthF(0); // cosmetic pen: 1 px on screen, whatever the zoom
    painter->setPen(framePen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(rect.adjusted(-2, -2, 2, 2));

    QFont font = painter->font();
    font.setPixelSize(qRound(FontPixelSize(unit)));
    painter->setFont(font);
    painter->setPen(kMarkGrey);
    painter->drawText(QPointF(rect.left(), rect.top() - font.pixelSize() * 0.45), caption);
    painter->restore();
}

//---------------------------------------------------------------------------------------------------------------------
OGroupMembershipMarkItem::OGroupMembershipMarkItem(QGraphicsItem *parent)
    : QGraphicsItem(parent)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setFlag(QGraphicsItem::ItemIsMovable, false);
    setFlag(QGraphicsItem::ItemIsFocusable, false);
    setAcceptedMouseButtons(Qt::NoButton); // the piece under the mark keeps every gesture
}

//---------------------------------------------------------------------------------------------------------------------
void OGroupMembershipMarkItem::SetMark(const QRectF &rect, const QString &caption, const Unit &unit)
{
    prepareGeometryChange();
    m_rect = rect;
    m_caption = caption;
    m_unit = unit;
    update();
}

//---------------------------------------------------------------------------------------------------------------------
auto OGroupMembershipMarkItem::boundingRect() const -> QRectF
{
    if (m_rect.isNull())
    {
        return {};
    }

    // The frame, plus the caption that stands above the top-left corner: a repaint of the piece
    // must reach both, or the caption would be left behind when the piece moves or is renamed.
    QFont font;
    font.setPixelSize(qRound(OGroupMark::FontPixelSize(m_unit)));
    const QFontMetricsF metrics(font);
    const QRectF frame = m_rect.adjusted(-2, -2, 2, 2);
    const QRectF caption(m_rect.left(), m_rect.top() - font.pixelSize() * 0.45 - metrics.ascent(),
                         metrics.horizontalAdvance(m_caption), metrics.height());
    return frame.united(caption);
}

//---------------------------------------------------------------------------------------------------------------------
void OGroupMembershipMarkItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                                     QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    OGroupMark::Paint(painter, m_rect, m_caption, m_unit);
}
