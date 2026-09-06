/************************************************************************
 **
 **  @file   osvgutils.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Implementation of the shared SVG emission primitives (see header).
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

#include "osvgutils.h"

#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QPainterPath>
#include <QPen>
#include <QStringList>
#include <QTextStream>
#include <QTransform>

#include "../vlayout/vgraphicsfillitem.h"

#include <algorithm>

namespace OSvg
{
//---------------------------------------------------------------------------------------------------------------------
QString PainterPathToSvgD(const QPainterPath &path)
{
    QString d;
    QTextStream stream(&d);
    stream.setRealNumberNotation(QTextStream::FixedNotation);
    stream.setRealNumberPrecision(3);

    for (int i = 0; i < path.elementCount(); ++i)
    {
        const QPainterPath::Element e = path.elementAt(i);
        switch (e.type)
        {
            case QPainterPath::MoveToElement:
                stream << "M " << e.x << ' ' << e.y << ' ';
                break;
            case QPainterPath::LineToElement:
                stream << "L " << e.x << ' ' << e.y << ' ';
                break;
            case QPainterPath::CurveToElement:
                stream << "C " << e.x << ' ' << e.y << ' ';
                break;
            case QPainterPath::CurveToDataElement:
                stream << e.x << ' ' << e.y << ' ';
                break;
            default:
                break;
        }
    }
    return d.trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
QString SanitizeSvgId(const QString &name)
{
    QString id;
    id.reserve(name.size());
    for (const QChar ch : name)
    {
        id += (ch.isLetterOrNumber() || ch == u'_' || ch == u'-') ? ch : u'_';
    }
    if (!id.isEmpty() && id.front().isDigit())
    {
        id.prepend(u'_');
    }
    if (id.isEmpty())
    {
        id = QStringLiteral("piece");
    }
    return id;
}

//---------------------------------------------------------------------------------------------------------------------
QString MatrixAttr(const QTransform &m)
{
    return QStringLiteral("matrix(%1,%2,%3,%4,%5,%6)")
        .arg(m.m11(), 0, 'g', 8)
        .arg(m.m12(), 0, 'g', 8)
        .arg(m.m21(), 0, 'g', 8)
        .arg(m.m22(), 0, 'g', 8)
        .arg(m.dx(), 0, 'g', 8)
        .arg(m.dy(), 0, 'g', 8);
}

//---------------------------------------------------------------------------------------------------------------------
QString DashArray(const QPen &pen)
{
    if (pen.style() == Qt::SolidLine)
    {
        return {};
    }

    const qreal w = std::max(pen.widthF(), static_cast<qreal>(1.0));

    // A custom dash pattern is expressed in multiples of the pen width
    // (Qt QPen doc). The graded style stores the pattern already divided by
    // the pen width, so multiplying back gives the true pixel lengths.
    if (pen.style() == Qt::CustomDashLine)
    {
        const QVector<qreal> pattern = pen.dashPattern();
        if (pattern.isEmpty())
        {
            return {};
        }
        // The pattern is stored in multiples of the pen width (Qt QPen doc). Multiply
        // back by the REAL width (not the clamped one used by the named styles) so a
        // px-accurate dash (px/width stored) comes out as its true pixel length.
        const qreal realW = pen.widthF() > 0.0 ? pen.widthF() : 1.0;
        QStringList parts;
        parts.reserve(pattern.size());
        for (qreal v : pattern)
        {
            parts.append(QString::number(v * realW));
        }
        return parts.join(QLatin1Char(' '));
    }

    switch (pen.style())
    {
        case Qt::DashLine:
            return QStringLiteral("%1 %2").arg(w * 4.0).arg(w * 2.0);
        case Qt::DotLine:
            return QStringLiteral("%1 %2").arg(w).arg(w * 2.0);
        case Qt::DashDotLine:
            return QStringLiteral("%1 %2 %3 %2").arg(w * 4.0).arg(w * 2.0).arg(w);
        case Qt::DashDotDotLine:
            return QStringLiteral("%1 %2 %3 %2 %3 %2").arg(w * 4.0).arg(w * 2.0).arg(w);
        default:
            return {};
    }
}

//---------------------------------------------------------------------------------------------------------------------
QGraphicsPathItem *AsPathItem(QGraphicsItem *item)
{
    const int t = item->type();
    if (t == VGraphicsFillItem::Type || t == VGraphicsFoldLineItem::Type)
    {
        return static_cast<QGraphicsPathItem *>(item);
    }
    return qgraphicsitem_cast<QGraphicsPathItem *>(item);
}

//---------------------------------------------------------------------------------------------------------------------
bool IsInfoChild(const QGraphicsItem *child)
{
    if (child == nullptr || !child->isVisible())
    {
        return false;
    }
    const int t = child->type();
    if (t == QGraphicsSimpleTextItem::Type || t == VGraphicsFillItem::Type
        || t == VGraphicsFoldLineItem::Type)
    {
        return true;
    }
    QGraphicsPathItem *const p = AsPathItem(const_cast<QGraphicsItem *>(child));
    return p != nullptr && p->brush().style() != Qt::NoBrush;
}
}  // namespace OSvg
