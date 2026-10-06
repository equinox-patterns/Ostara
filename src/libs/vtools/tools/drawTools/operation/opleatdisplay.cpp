/************************************************************************
 **
 **  @file   opleatdisplay.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The objects a pleat draws: the prise and its copies, the fold line, and the cap of each end.
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

#include "opleatdisplay.h"

#include <QPointF>
#include <QSharedPointer>

#include "../../vgeometry/vgobject.h"
#include "../../vgeometry/vpointf.h"
#include "../../vmisc/literals.h"
#include "../../vpatterndb/vcontainer.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "ooperationlines.h"
#include "vabstractoperation.h"

using namespace Qt::Literals::StringLiterals;

const QString OPleatDisplay::TagDisplay = QStringLiteral("pleatDisplay");
const QString OPleatDisplay::AttrShow = QStringLiteral("show");
const QString OPleatDisplay::AttrTypeLine = QStringLiteral("typeLine");
const QString OPleatDisplay::AttrLineColor = QStringLiteral("lineColor");
const QString OPleatDisplay::AttrFoldTypeLine = QStringLiteral("foldTypeLine");
const QString OPleatDisplay::AttrFoldColor = QStringLiteral("foldColor");
const QString OPleatDisplay::AttrLine = QStringLiteral("line");
const QString OPleatDisplay::AttrFoldLine = QStringLiteral("foldLine");
const QString OPleatDisplay::AttrCopy = QStringLiteral("copy");
const QString OPleatDisplay::AttrPoint = QStringLiteral("point");
const QString OPleatDisplay::AttrSide1 = QStringLiteral("side1");
const QString OPleatDisplay::AttrSide2 = QStringLiteral("side2");

namespace
{
/** The label offsets a point of the display is given when the prise point it comes from is gone. */
constexpr qreal kDefaultLabelOffsetX = 0.4;
constexpr qreal kDefaultLabelOffsetY = 0.65;

//---------------------------------------------------------------------------------------------------------------------
/** @brief The name of a point of the display, made unique against everything the pattern already holds. */
auto UniqueName(const QString &base, const VContainer *data) -> QString
{
    if (base.isEmpty())
    {
        return base;
    }

    if (data->IsUnique(base))
    {
        return base;
    }

    return base + '_'_L1 + QString::number(data->UniqueDuplicate(base));
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The point of the container under an id, empty when the id names no point. */
auto PointOf(quint32 pointId, const VContainer *data) -> QSharedPointer<VPointF>
{
    if (data == nullptr || not data->HasObject(pointId))
    {
        return {};
    }

    return qSharedPointerDynamicCast<VPointF>(data->GetGObject(pointId));
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The name of a prise point, or a plain one when the container no longer holds it: a name is a label, and a
 *  missing one must not cost the figure its objects. */
auto PriseName(quint32 pointId, const VContainer *data) -> QString
{
    const QSharedPointer<VPointF> point = PointOf(pointId, data);
    return point.isNull() ? QStringLiteral("pli") : point->name();
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Place one point of the display under the id it carries, creating it or moving the one already there.
 *
 *  The point is a construction mark: its name is read in the dependency tree, where it names the object, and not on
 *  the canvas, where it would sit under the lines of the figure. */
void PlacePoint(quint32 id, const QPointF &position, const QString &name, const QPointF &labelOffset, quint32 idTool,
                VContainer *data)
{
    const auto point = QSharedPointer<VPointF>::create(position, name, labelOffset.x(), labelOffset.y());
    point->setIdObject(idTool);
    point->SetShowLabel(false);

    data->UpdateGObject(id, point);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::PleatCount(OPleatKind kind) -> int
{
    return (kind == OPleatKind::Double) ? 2 : 1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::PositionCount(OPleatKind kind) -> int
{
    return PleatCount(kind) + 1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::Endpoint(const OPleatDisplay &display, int position, int end, const OPleatEnd &first,
                             const OPleatEnd &second, const QVector<SourceItem> &source,
                             const QVector<DestinationItem> &destination) -> quint32
{
    const quint32 priseId = (end == 0) ? first.pointId : second.pointId;
    if (position <= 0)
    {
        return priseId; // the prise itself: a point the user drew, which the display never creates
    }

    const quint32 own = display.copies.value(2 * (position - 1) + end).point;
    if (own != NULL_ID)
    {
        return own;
    }

    // The point travels with the other objects: where it lands is the copy the operation made of it, and making a
    // second point there would leave two of them where the pattern needs one. That is true of the LAST place only
    // — the copy line, where the fabric lands. Every place before it is a line the fold draws in the middle of the
    // travel, which no other object stands for: the display holds a point of its own there, and without it the line
    // has nothing to be drawn on (see EnsureIds, which gives it one).
    if (position != static_cast<int>(display.lines.size()) - 1)
    {
        return NULL_ID;
    }

    return OOperationLines::PointImage(priseId, source, destination, {});
}

//---------------------------------------------------------------------------------------------------------------------
void OPleatDisplay::EnsureIds(OPleatDisplay &display, const OPleatEnd &first, const OPleatEnd &second, OPleatKind kind,
                              bool foldLine, const OPleatPlan &plan, const QVector<SourceItem> &source,
                              const QVector<DestinationItem> &destination, VContainer *data)
{
    const int pleats = PleatCount(kind);
    const int positions = PositionCount(kind);

    if (not display.show || plan.failure != OPleatFailure::None)
    {
        // Nothing is kept while the figure is off, and nothing exists when the rule refused the pleat: switching the
        // figure on allocates the objects again, which is what "the objects are created again" means.
        display.lines.clear();
        display.foldLines.clear();
        display.copies.clear();
        display.marks.clear();
        return;
    }

    display.lines.resize(positions);
    // The fold line is the pleat itself: with none drawn there is no object to keep, and its id must not stay in the
    // file as the id of a line that will never exist.
    display.foldLines.resize(foldLine ? pleats : 0);
    display.copies.resize(pleats * 2);
    display.marks.resize(plan.marks.size());

    for (quint32 &line : display.lines)
    {
        if (line == NULL_ID)
        {
            line = data->getNextId();
        }
    }

    for (quint32 &line : display.foldLines)
    {
        if (line == NULL_ID)
        {
            line = data->getNextId();
        }
    }

    for (int k = 1; k < positions; ++k)
    {
        for (int end = 0; end < 2; ++end)
        {
            OPleatCopy &copy = display.copies[2 * (k - 1) + end];
            const OPleatEnd &prise = (end == 0) ? first : second;

            // The copy the operation makes of the prise point is one place only — the last one, where the fabric
            // lands. Every place before it is a line of the travel that no other object stands for, and it needs a
            // point of its own: two lines drawn on the same copy would leave the second pleat of a double without a
            // second side, and the line between the two pleats would sit on the copy line (see Endpoint).
            const bool carried = (k == positions - 1)
                                 && OOperationLines::PointImage(prise.pointId, source, destination, {}) != NULL_ID;

            if (carried)
            {
                // An id left by another state — the place was a line of the display in a double, and a simple pleat
                // carries it: keeping it would leave a second point on the copy line. Whatever changes this asks for
                // a full parse, so the point that id named is rebuilt from the file, without it.
                copy.point = NULL_ID;
                copy.name.clear();
            }
            else if (copy.point == NULL_ID)
            {
                copy.point = data->getNextId();
            }

            if (copy.point != NULL_ID && copy.name.isEmpty())
            {
                copy.name = UniqueName(PriseName(prise.pointId, data) + "__m"_L1 + QString::number(k), data);
            }
        }
    }

    for (int i = 0; i < display.marks.size() && i < plan.marks.size(); ++i)
    {
        OPleatMarkDisplay &mark = display.marks[i];
        const OPleatMark &resolved = plan.marks.at(i);
        const OPleatEnd &prise = (resolved.end == 0) ? first : second;

        if (mark.pleat != resolved.pleat || mark.end != resolved.end)
        {
            // The mark describes another end than it did: its name described that other end too.
            mark.name.clear();
            mark.pleat = resolved.pleat;
            mark.end = resolved.end;
        }

        if (mark.point == NULL_ID)
        {
            mark.point = data->getNextId();
        }

        if (mark.name.isEmpty())
        {
            const QString suffix = (pleats == 1) ? "__pli"_L1 : "__pli"_L1 + QString::number(resolved.pleat + 1);
            mark.name = UniqueName(PriseName(prise.pointId, data) + suffix, data);
        }

        if (resolved.flat)
        {
            // The edge is parallel to the move: the mark is the middle of the opening, and there is no side to draw.
            // The ids of the sides a previous state had must go, or the objects would stay in the file.
            mark.side1 = NULL_ID;
            mark.side2 = NULL_ID;
            continue;
        }

        if (mark.side1 == NULL_ID)
        {
            mark.side1 = data->getNextId();
        }
        if (mark.side2 == NULL_ID)
        {
            mark.side2 = data->getNextId();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OPleatDisplay::Create(quint32 idTool, OPleatDisplay &display, const OPleatPlan &plan, OPleatKind kind,
                           const OPleatEnd &first, const OPleatEnd &second, VContainer *data)
{
    if (not display.show || plan.failure != OPleatFailure::None)
    {
        return;
    }

    const int positions = PositionCount(kind);

    for (int k = 1; k < positions; ++k)
    {
        for (int end = 0; end < 2; ++end)
        {
            const OPleatCopy &copy = display.copies.value(2 * (k - 1) + end);
            if (copy.point == NULL_ID)
            {
                continue;
            }

            const OPleatEnd &prise = (end == 0) ? first : second;
            const QSharedPointer<VPointF> source = PointOf(prise.pointId, data);
            const QPointF offset = source.isNull() ? QPointF(kDefaultLabelOffsetX, kDefaultLabelOffsetY)
                                                   : QPointF(source->mx(), source->my());
            PlacePoint(copy.point, prise.position + plan.step * k, copy.name, offset, idTool, data);
        }
    }

    for (int i = 0; i < display.marks.size() && i < plan.marks.size(); ++i)
    {
        const OPleatMarkDisplay &mark = display.marks.at(i);
        const OPleatMark &resolved = plan.marks.at(i);
        const OPleatEnd &prise = (resolved.end == 0) ? first : second;

        if (mark.point == NULL_ID)
        {
            continue;
        }

        const QSharedPointer<VPointF> source = PointOf(prise.pointId, data);
        const QPointF offset = source.isNull() ? QPointF(kDefaultLabelOffsetX, kDefaultLabelOffsetY)
                                               : QPointF(source->mx(), source->my());
        PlacePoint(mark.point, resolved.point, mark.name, offset, idTool, data);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::PointImages(const OPleatDisplay &display, const VContainer *data) -> QVector<quint32>
{
    QVector<quint32> ids;
    ids.reserve(display.copies.size() + display.marks.size());

    for (const OPleatCopy &copy : std::as_const(display.copies))
    {
        // A point the container does not know is one that was never placed: it has no image to offer, and handing its
        // id over would have the caller ask the container for an object that is not there.
        if (copy.point != NULL_ID && data->HasObject(copy.point))
        {
            ids.append(copy.point);
        }
    }

    for (const OPleatMarkDisplay &mark : std::as_const(display.marks))
    {
        if (mark.point != NULL_ID && data->HasObject(mark.point))
        {
            ids.append(mark.point);
        }
    }

    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
void OPleatDisplay::Save(VAbstractPattern *doc, QDomElement &tag, const OPleatDisplay &display)
{
    SCASSERT(doc != nullptr)

    // The element is written whole: a cap that is turned off loses its mark, and the ids of a mark that is gone must
    // not stay behind in the file.
    for (QDomElement old = tag.firstChildElement(TagDisplay); not old.isNull();)
    {
        const QDomElement next = old.nextSiblingElement(TagDisplay);
        tag.removeChild(old);
        old = next;
    }

    QDomElement element = doc->createElement(TagDisplay);
    doc->SetAttribute(element, AttrShow, display.show);
    doc->SetAttribute(element, AttrTypeLine, display.lineTypeLine);
    doc->SetAttribute(element, AttrLineColor, display.lineColor);
    doc->SetAttribute(element, AttrFoldTypeLine, display.foldTypeLine);
    doc->SetAttribute(element, AttrFoldColor, display.foldColor);

    for (const quint32 line : std::as_const(display.lines))
    {
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        doc->SetAttribute(item, AttrLine, line);
        element.appendChild(item);
    }

    for (const quint32 line : std::as_const(display.foldLines))
    {
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        doc->SetAttribute(item, AttrFoldLine, line);
        element.appendChild(item);
    }

    for (const OPleatCopy &copy : std::as_const(display.copies))
    {
        // Every place holds an entry, even an empty one: the two lists are read side by side, and dropping one would
        // pair every following copy with the wrong place on the prise.
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        if (copy.point != NULL_ID)
        {
            doc->SetAttribute(item, AttrCopy, copy.point);
            if (not copy.name.isEmpty())
            {
                doc->SetAttribute(item, AttrName, copy.name);
            }
        }
        element.appendChild(item);
    }

    for (const OPleatMarkDisplay &mark : std::as_const(display.marks))
    {
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        if (mark.point != NULL_ID)
        {
            doc->SetAttribute(item, AttrPoint, mark.point);
            if (not mark.name.isEmpty())
            {
                doc->SetAttribute(item, AttrName, mark.name);
            }
        }
        if (mark.side1 != NULL_ID)
        {
            doc->SetAttribute(item, AttrSide1, mark.side1);
        }
        if (mark.side2 != NULL_ID)
        {
            doc->SetAttribute(item, AttrSide2, mark.side2);
        }
        element.appendChild(item);
    }

    tag.appendChild(element);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::Extract(const QDomElement &domElement) -> OPleatDisplay
{
    OPleatDisplay display;

    for (QDomElement element = domElement.firstChildElement(TagDisplay); not element.isNull();
         element = element.nextSiblingElement(TagDisplay))
    {
        // The figure of a pleat is always drawn: a file written while it could be turned off opens with it on,
        // since nothing in the tool would turn it back on (see Pleat.md).
        display.show = true;
        display.lineTypeLine = VAbstractPattern::GetParametrString(element, AttrTypeLine, TypeLineLine);
        display.lineColor = VAbstractPattern::GetParametrString(element, AttrLineColor, ColorOrange);
        display.foldTypeLine = VAbstractPattern::GetParametrString(element, AttrFoldTypeLine, TypeLineDotLine);
        display.foldColor = VAbstractPattern::GetParametrString(element, AttrFoldColor, ColorLightGreen);

        for (QDomElement item = element.firstChildElement(VAbstractOperation::TagItem); not item.isNull();
             item = item.nextSiblingElement(VAbstractOperation::TagItem))
        {
            if (item.hasAttribute(AttrPoint))
            {
                OPleatMarkDisplay mark;
                mark.point = VAbstractPattern::GetParametrUInt(item, AttrPoint, NULL_ID_STR);
                mark.side1 = VAbstractPattern::GetParametrUInt(item, AttrSide1, NULL_ID_STR);
                mark.side2 = VAbstractPattern::GetParametrUInt(item, AttrSide2, NULL_ID_STR);
                mark.name = VDomDocument::GetParametrEmptyString(item, AttrName);
                display.marks.append(mark);
            }
            else if (item.hasAttribute(AttrFoldLine))
            {
                display.foldLines.append(VAbstractPattern::GetParametrUInt(item, AttrFoldLine, NULL_ID_STR));
            }
            else if (item.hasAttribute(AttrCopy))
            {
                OPleatCopy copy;
                copy.point = VAbstractPattern::GetParametrUInt(item, AttrCopy, NULL_ID_STR);
                copy.name = VDomDocument::GetParametrEmptyString(item, AttrName);
                display.copies.append(copy);
            }
            else if (item.hasAttribute(AttrLine))
            {
                display.lines.append(VAbstractPattern::GetParametrUInt(item, AttrLine, NULL_ID_STR));
            }
            else
            {
                // An entry with nothing in it is a place the operation carries by itself: the copy of a prise point
                // that travels is not the display's to create, and the place must still be counted.
                display.copies.append(OPleatCopy{});
            }
        }

        break; // Only the first element is read, exactly as the operation lines are: an operation has one.
    }

    return display;
}
