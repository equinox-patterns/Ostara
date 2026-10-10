/************************************************************************
 **
 **  @file   oflouncespiraldisplay.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  The figure of a flounce spiral: ids, formulas and the one
 **          pen of the whole figure, written under one element of the
 **          operation.
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

#include "oflouncespiraldisplay.h"

#include <QDomElement>
#include <QSharedPointer>

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vgobject.h"
#include "../vmisc/def.h"
#include "../vpatterndb/vcontainer.h"
#include "vexception.h"

const QString OFlounceSpiralDisplay::TagDisplay = QStringLiteral("flounceSpiral");
const QString OFlounceSpiralDisplay::AttrInner = QStringLiteral("inner");
const QString OFlounceSpiralDisplay::AttrOuter = QStringLiteral("outer");
const QString OFlounceSpiralDisplay::AttrLine = QStringLiteral("line");
const QString OFlounceSpiralDisplay::AttrPoint = QStringLiteral("point");
const QString OFlounceSpiralDisplay::AttrRole = QStringLiteral("role");
const QString OFlounceSpiralDisplay::AttrWidth = QStringLiteral("width");
const QString OFlounceSpiralDisplay::AttrLength = QStringLiteral("length");
const QString OFlounceSpiralDisplay::AttrAngle = QStringLiteral("angle");
const QString OFlounceSpiralDisplay::AttrTypeLine = QStringLiteral("typeLine");
const QString OFlounceSpiralDisplay::AttrLineColor = QStringLiteral("lineColor");
const QString OFlounceSpiralDisplay::AttrAlias = QStringLiteral("alias");
const QString OFlounceSpiralDisplay::AttrPenStyle = QStringLiteral("penStyle");

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/// The three roles, written as stable keys so that a file stays readable and a role stays recognisable.
auto RoleKey(OFlounceSpiralPoint::Role role) -> QString
{
    switch (role)
    {
        case OFlounceSpiralPoint::Role::OuterStart:
            return QStringLiteral("outerStart");
        case OFlounceSpiralPoint::Role::InnerEnd:
            return QStringLiteral("innerEnd");
        case OFlounceSpiralPoint::Role::OuterEnd:
            return QStringLiteral("outerEnd");
    }
    return QStringLiteral("outerStart");
}

//---------------------------------------------------------------------------------------------------------------------
auto RoleOfKey(const QString &key) -> OFlounceSpiralPoint::Role
{
    if (key == QStringLiteral("innerEnd"))
    {
        return OFlounceSpiralPoint::Role::InnerEnd;
    }
    if (key == QStringLiteral("outerEnd"))
    {
        return OFlounceSpiralPoint::Role::OuterEnd;
    }
    return OFlounceSpiralPoint::Role::OuterStart;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiralDisplay::PointId(const OFlounceSpiralDisplay &display, OFlounceSpiralPoint::Role role) -> quint32
{
    for (const OFlounceSpiralPoint &point : display.points)
    {
        if (point.role == role)
        {
            return point.id;
        }
    }
    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
void OFlounceSpiralDisplay::EnsureIds(OFlounceSpiralDisplay &display, VContainer *data)
{
    if (data == nullptr)
    {
        return;
    }

    // The ids of the figure are not known to the parser while it rebuilds them, so the counter of free ids is
    // told about every one of them before anything new is asked for.
    for (const quint32 id : OwnedIds(display))
    {
        if (id != NULL_ID)
        {
            data->UpdateId(id);
        }
    }

    if (display.points.isEmpty())
    {
        for (const auto role : {OFlounceSpiralPoint::Role::OuterStart, OFlounceSpiralPoint::Role::InnerEnd,
                                OFlounceSpiralPoint::Role::OuterEnd})
        {
            OFlounceSpiralPoint point;
            point.role = role;
            display.points.append(point);
        }
    }

    for (OFlounceSpiralPoint &point : display.points)
    {
        if (point.id == NULL_ID)
        {
            point.id = data->getNextId();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiralDisplay::OwnedIds(const OFlounceSpiralDisplay &display) -> QSet<quint32>
{
    QSet<quint32> ids;
    for (const quint32 id : {display.inner, display.outer, display.line1, display.line2})
    {
        if (id != NULL_ID)
        {
            ids.insert(id);
        }
    }
    for (const OFlounceSpiralPoint &point : display.points)
    {
        if (point.id != NULL_ID)
        {
            ids.insert(point.id);
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiralDisplay::ObjectIds(const QDomElement &element) -> QSet<quint32>
{
    QSet<quint32> ids;
    const QDomElement display = element.firstChildElement(TagDisplay);
    for (QDomElement item = display.firstChildElement(QStringLiteral("item")); not item.isNull();
         item = item.nextSiblingElement(QStringLiteral("item")))
    {
        for (const QString &attribute : {AttrInner, AttrOuter, AttrLine, AttrPoint})
        {
            if (const quint32 id = item.attribute(attribute).toUInt(); id != NULL_ID)
            {
                ids.insert(id);
            }
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiralDisplay::PointIds(const QDomElement &element) -> QVector<quint32>
{
    QVector<quint32> ids;
    const QDomElement display = element.firstChildElement(TagDisplay);
    for (QDomElement item = display.firstChildElement(QStringLiteral("item")); not item.isNull();
         item = item.nextSiblingElement(QStringLiteral("item")))
    {
        if (const quint32 id = item.attribute(AttrPoint).toUInt(); id != NULL_ID)
        {
            ids.append(id);
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiralDisplay::CurveIds(const QDomElement &element) -> QVector<quint32>
{
    QVector<quint32> ids;
    const QDomElement display = element.firstChildElement(TagDisplay);
    for (QDomElement item = display.firstChildElement(QStringLiteral("item")); not item.isNull();
         item = item.nextSiblingElement(QStringLiteral("item")))
    {
        for (const QString &attribute : {AttrInner, AttrOuter})
        {
            if (const quint32 id = item.attribute(attribute).toUInt(); id != NULL_ID)
            {
                ids.append(id);
            }
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
void OFlounceSpiralDisplay::Save(VAbstractPattern *doc, QDomElement &tag, const OFlounceSpiralDisplay &display)
{
    if (doc == nullptr)
    {
        return;
    }

    // The figure is written whole every time, so the one already there goes first.
    for (QDomElement old = tag.firstChildElement(TagDisplay); not old.isNull();)
    {
        const QDomElement next = old.nextSiblingElement(TagDisplay);
        tag.removeChild(old);
        old = next;
    }

    QDomElement element = doc->createElement(TagDisplay);

    doc->SetAttribute(element, AttrWidth, display.width);
    doc->SetAttribute(element, AttrLength, display.length);
    doc->SetAttribute(element, AttrAngle, display.angle);
    doc->SetAttribute(element, AttrTypeLine, display.typeLine);
    doc->SetAttribute(element, AttrLineColor, display.lineColor);

    // One item per object, so that a reader never has to guess which id belongs to what.
    auto AppendObject = [&doc, &element](const QString &attribute, quint32 id, const QString &alias,
                                         const QString &penStyle, const QString &color)
    {
        if (id == NULL_ID)
        {
            return;
        }
        QDomElement item = doc->createElement(QStringLiteral("item"));
        doc->SetAttribute(item, attribute, id);
        doc->SetAttribute(item, AttrPenStyle, penStyle);
        doc->SetAttribute(item, AttrLineColor, color);
        if (not alias.isEmpty())
        {
            doc->SetAttribute(item, AttrAlias, alias);
        }
        element.appendChild(item);
    };

    // One pen for the whole figure, so every item carries the same pair: the edge and the segments are the
    // objects of one figure, and nothing tells their styles apart any more.
    AppendObject(AttrInner, display.inner, display.innerAlias, display.typeLine, display.lineColor);
    AppendObject(AttrOuter, display.outer, display.outerAlias, display.typeLine, display.lineColor);

    for (const quint32 id : {display.line1, display.line2})
    {
        AppendObject(AttrLine, id, QString(), display.typeLine, display.lineColor);
    }

    for (const OFlounceSpiralPoint &point : display.points)
    {
        if (point.id == NULL_ID)
        {
            continue;
        }
        QDomElement item = doc->createElement(QStringLiteral("item"));
        doc->SetAttribute(item, AttrPoint, point.id);
        doc->SetAttribute(item, AttrRole, RoleKey(point.role));
        if (not point.name.isEmpty())
        {
            doc->SetAttribute(item, AttrName, point.name);
        }
        element.appendChild(item);
    }

    tag.appendChild(element);
}

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiralDisplay::Extract(const QDomElement &domElement) -> OFlounceSpiralDisplay
{
    OFlounceSpiralDisplay display;
    const QDomElement element = domElement.firstChildElement(TagDisplay);
    if (element.isNull())
    {
        return display;
    }

    display.width = VAbstractPattern::GetParametrString(element, AttrWidth, QChar('0'));
    display.length = VAbstractPattern::GetParametrString(element, AttrLength, QChar('0'));
    display.angle = VAbstractPattern::GetParametrString(element, AttrAngle, QStringLiteral("180"));

    // The figure has ONE style, and the file keeps it in three places: the attributes of the element, the
    // penStyle and lineColor of every item, and the <lines> element the base reads. The element's attributes are
    // read first — that is the figure's own place, and it is the only one that can be right when a file written
    // before the merge carries a pen per object: two different pens cannot both survive, and the one that
    // describes the whole figure is the honest choice. The items are then the fallback for a file whose element
    // does not carry them.
    display.typeLine = VAbstractPattern::GetParametrString(element, AttrTypeLine, TypeLineLine);
    display.lineColor = VAbstractPattern::GetParametrString(element, AttrLineColor, ColorBlack);
    const bool styleOnElement = element.hasAttribute(AttrTypeLine) || element.hasAttribute(AttrLineColor);

    for (QDomElement item = element.firstChildElement(QStringLiteral("item")); not item.isNull();
         item = item.nextSiblingElement(QStringLiteral("item")))
    {
        // The pen of one object of the figure, read only when the element carries none — see above.
        auto ItemStyle = [&styleOnElement, &item, &display]()
        {
            if (not styleOnElement)
            {
                display.typeLine = VAbstractPattern::GetParametrString(item, AttrPenStyle, display.typeLine);
                display.lineColor = VAbstractPattern::GetParametrString(item, AttrLineColor, display.lineColor);
            }
        };

        if (const quint32 id = item.attribute(AttrInner).toUInt(); id != NULL_ID)
        {
            display.inner = id;
            display.innerAlias = item.attribute(AttrAlias);
            ItemStyle();
        }
        else if (const quint32 outer = item.attribute(AttrOuter).toUInt(); outer != NULL_ID)
        {
            display.outer = outer;
            display.outerAlias = item.attribute(AttrAlias);
            ItemStyle();
        }
        else if (const quint32 line = item.attribute(AttrLine).toUInt(); line != NULL_ID)
        {
            if (display.line1 == NULL_ID)
            {
                display.line1 = line;
            }
            else
            {
                display.line2 = line;
            }
            ItemStyle();
        }
        else if (const quint32 point = item.attribute(AttrPoint).toUInt(); point != NULL_ID)
        {
            OFlounceSpiralPoint entry;
            entry.id = point;
            entry.role = RoleOfKey(item.attribute(AttrRole));
            entry.name = item.attribute(AttrName);
            display.points.append(entry);
        }
    }

    return display;
}

//---------------------------------------------------------------------------------------------------------------------
auto OFlounceSpiralDisplay::Same(const OFlounceSpiralDisplay &first, const OFlounceSpiralDisplay &second) -> bool
{
    // The figure's own element and its one style, nothing else: whether the name of a point is shown is not part
    // of that element — the file carries the switch in the <destination> items — so it is not compared here.
    return first.inner == second.inner && first.outer == second.outer && first.line1 == second.line1
           && first.line2 == second.line2 && first.width == second.width && first.length == second.length
           && first.angle == second.angle && first.typeLine == second.typeLine
           && first.lineColor == second.lineColor && first.innerAlias == second.innerAlias
           && first.outerAlias == second.outerAlias && first.points.size() == second.points.size()
           && [&first, &second]()
    {
        for (int i = 0; i < first.points.size(); ++i)
        {
            if (first.points.at(i).id != second.points.at(i).id || first.points.at(i).role != second.points.at(i).role
                || first.points.at(i).name != second.points.at(i).name)
            {
                return false;
            }
        }
        return true;
    }();
}
