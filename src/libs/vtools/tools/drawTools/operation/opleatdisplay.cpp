/************************************************************************
 **
 **  @file   opleatdisplay.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The objects the pleats draw: the departure lines and their copies, the fold lines, and the cap of
 **          each end of each pleat.
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

#include "../../vgeometry/vgeometrydef.h"
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
const QString OPleatDisplay::AttrDeparture = QStringLiteral("departure");
const QString OPleatDisplay::AttrPlace = QStringLiteral("place");
const QString OPleatDisplay::AttrEnd = QStringLiteral("end");
const QString OPleatDisplay::AttrPoint = QStringLiteral("point");
const QString OPleatDisplay::AttrSide1 = QStringLiteral("side1");
const QString OPleatDisplay::AttrSide2 = QStringLiteral("side2");

namespace
{
/** The label offsets a point of the display is given when the point of the departure line it comes from is gone. */
constexpr qreal kDefaultLabelOffsetX = 0.4;
constexpr qreal kDefaultLabelOffsetY = 0.65;

/** How close two places have to be for the figure to call them the same one: the carries are sums of steps, so
 *  a place is where the arithmetic put it, and this only forgives the rounding of that arithmetic. */
constexpr qreal kPlaceTolerance = 1e-6;

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
/** @brief The name of a point of a departure line, or a plain one when the container no longer holds it: a name is
 *  a label, and a missing one must not cost the figure its objects. */
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

/** @brief One end of a departure line, the way every other question of the figure asks for it. */
auto EndOf(const QVector<OPleatDeparture> &departures, int departure, int end) -> OPleatEnd
{
    const OPleatDeparture line = departures.value(departure, OPleatDeparture{});
    return (end == 0) ? line.first : line.second;
}

/** @brief Where the endpoint of one place comes from.
 *
 *  @a Prise is the point the user drew — the departure line as it is, which the figure never creates; @a Image is
 *  the copy the operation already made of that point, which stands at the last place of its own departure line;
 *  @a Own is a place no other object stands for, and the figure then holds a point of its own there. */
enum class OPleatEndpoint
{
    Prise,
    Image,
    Own
};

//---------------------------------------------------------------------------------------------------------------------
auto EndpointKind(const OPleatPlan &plan, const QVector<OPleatDeparture> &departures, int departure, int line,
                  int end, const QVector<SourceItem> &source, const QVector<DestinationItem> &destination)
    -> OPleatEndpoint
{
    const OPleatEnd prise = EndOf(departures, departure, end);
    const QPointF place = plan.Place(prise, departure, line);

    if (VFuzzyComparePoints(place, prise.position, kPlaceTolerance))
    {
        return OPleatEndpoint::Prise; // the line as the user drew it: there is nothing to create
    }

    if (VFuzzyComparePoints(place, prise.position + plan.Move(prise.pointId), kPlaceTolerance)
        && OOperationLines::PointImage(prise.pointId, source, destination, {}) != NULL_ID)
    {
        // The place is where the operation carries that point: making a second point there would leave two of
        // them where the pattern needs one (see Endpoint).
        return OPleatEndpoint::Image;
    }

    return OPleatEndpoint::Own;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::PleatCount(const OPleatDeparture &departure) -> int
{
    return (departure.kind == OPleatKind::Double) ? 2 : 1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::LineCount(const OPleatDeparture &departure) -> int
{
    return PleatCount(departure) + 1;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::LineCount(const QVector<OPleatDeparture> &departures) -> int
{
    int count = 0;
    for (const OPleatDeparture &departure : departures)
    {
        count += LineCount(departure);
    }

    return count;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::CopyCount(const QVector<OPleatDeparture> &departures) -> int
{
    return 2 * LineCount(departures);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::LineIndex(const QVector<OPleatDeparture> &departures, int departure, int line) -> int
{
    int index = line;
    for (int i = 0; i < departure && i < departures.size(); ++i)
    {
        index += LineCount(departures.at(i));
    }

    return index;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::CopyIndex(const QVector<OPleatDeparture> &departures, int departure, int line, int end) -> int
{
    return 2 * LineIndex(departures, departure, line) + end;
}

//---------------------------------------------------------------------------------------------------------------------
auto OPleatDisplay::Endpoint(const OPleatDisplay &display, const OPleatPlan &plan,
                             const QVector<OPleatDeparture> &departures, int departure, int line, int end,
                             const QVector<SourceItem> &source, const QVector<DestinationItem> &destination) -> quint32
{
    switch (EndpointKind(plan, departures, departure, line, end, source, destination))
    {
        case OPleatEndpoint::Prise:
            return EndOf(departures, departure, end).pointId;
        case OPleatEndpoint::Image:
            return OOperationLines::PointImage(EndOf(departures, departure, end).pointId, source, destination, {});
        case OPleatEndpoint::Own:
            break;
    }

    return display.copies.value(CopyIndex(departures, departure, line, end)).point;
}

//---------------------------------------------------------------------------------------------------------------------
void OPleatDisplay::EnsureIds(OPleatDisplay &display, const QVector<OPleatDeparture> &departures, bool foldLine,
                              const OPleatPlan &plan, const QVector<SourceItem> &source,
                              const QVector<DestinationItem> &destination, VContainer *data)
{
    if (not display.show || plan.failure != OPleatFailure::None)
    {
        // Nothing is kept while the figure is off, and nothing exists when the rule refused the pleats: switching
        // the figure on allocates the objects again, which is what "the objects are created again" means.
        display.lines.clear();
        display.foldLines.clear();
        display.copies.clear();
        display.marks.clear();
        return;
    }

    // An entry of the file whose place no longer exists — a departure line the user took away — describes a figure
    // that is gone: nothing tells which of the ids that are left stood for which new place, so every one of them
    // is given again rather than handed to the wrong point.
    for (const OPleatCopy &copy : std::as_const(display.copies))
    {
        if (copy.departure < 0 || copy.departure >= departures.size() || copy.line < 0
            || copy.line >= LineCount(departures.at(copy.departure)) || copy.end < 0 || copy.end > 1)
        {
            display.copies.clear();
            break;
        }
    }

    display.lines.resize(LineCount(departures));
    // The fold line is the pleat itself: with none drawn there is no object to keep, and its id must not stay in
    // the file as the id of a line that will never exist.
    display.foldLines.resize(foldLine ? plan.PleatCount() : 0);
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

    // The copies the file held are read back by the place they name, and every entry is then stamped with its own:
    // the entries are rebuilt whole, so that a place the file does not hold is an empty entry — the figure creates
    // nothing there — and never the entry of another place.
    QVector<OPleatCopy> ordered(CopyCount(departures));
    for (const OPleatCopy &copy : std::as_const(display.copies))
    {
        ordered[CopyIndex(departures, copy.departure, copy.line, copy.end)] = copy;
    }
    display.copies = ordered;

    for (int departure = 0; departure < departures.size(); ++departure)
    {
        for (int line = 0; line < LineCount(departures.at(departure)); ++line)
        {
            for (int end = 0; end < 2; ++end)
            {
                OPleatCopy &copy = display.copies[CopyIndex(departures, departure, line, end)];
                copy.departure = departure;
                copy.line = line;
                copy.end = end;

                const OPleatEnd prise = EndOf(departures, departure, end);
                if (EndpointKind(plan, departures, departure, line, end, source, destination) != OPleatEndpoint::Own)
                {
                    // The place is one the operation carries by itself: an id left by another state would leave
                    // a second point where the pattern needs one. Whatever changes this asks for a full parse,
                    // so the point that id named is rebuilt from the file, without it.
                    copy.point = NULL_ID;
                    copy.name.clear();
                    continue;
                }

                if (copy.point == NULL_ID)
                {
                    copy.point = data->getNextId();
                }

                if (copy.name.isEmpty())
                {
                    copy.name = UniqueName(PriseName(prise.pointId, data) + "__m"_L1 + QString::number(line), data);
                }
            }
        }
    }

    for (int i = 0; i < display.marks.size() && i < plan.marks.size(); ++i)
    {
        OPleatMarkDisplay &mark = display.marks[i];
        const OPleatMark &resolved = plan.marks.at(i);
        const OPleatEnd prise = EndOf(departures, resolved.departure, resolved.end);

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
            const QString suffix = (plan.PleatCount() == 1) ? "__pli"_L1
                                                           : "__pli"_L1 + QString::number(resolved.pleat + 1);
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
void OPleatDisplay::Create(quint32 idTool, OPleatDisplay &display, const OPleatPlan &plan,
                           const QVector<OPleatDeparture> &departures, VContainer *data)
{
    if (not display.show || plan.failure != OPleatFailure::None)
    {
        return;
    }

    for (int departure = 0; departure < departures.size(); ++departure)
    {
        for (int line = 0; line < LineCount(departures.at(departure)); ++line)
        {
            for (int end = 0; end < 2; ++end)
            {
                const OPleatCopy &copy = display.copies.value(CopyIndex(departures, departure, line, end));
                if (copy.point == NULL_ID)
                {
                    continue;
                }

                const OPleatEnd prise = EndOf(departures, departure, end);
                const QSharedPointer<VPointF> source = PointOf(prise.pointId, data);
                const QPointF offset = source.isNull() ? QPointF(kDefaultLabelOffsetX, kDefaultLabelOffsetY)
                                                       : QPointF(source->mx(), source->my());
                PlacePoint(copy.point, plan.Place(prise, departure, line), copy.name, offset, idTool, data);
            }
        }
    }

    for (int i = 0; i < display.marks.size() && i < plan.marks.size(); ++i)
    {
        const OPleatMarkDisplay &mark = display.marks.at(i);
        const OPleatMark &resolved = plan.marks.at(i);
        const OPleatEnd prise = EndOf(departures, resolved.departure, resolved.end);

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
        // The place is written with the point: the places of the figure are read back by what they name, and an
        // entry that named nothing would be handed to whichever place comes first.
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        doc->SetAttribute(item, AttrDeparture, static_cast<quint32>(copy.departure));
        doc->SetAttribute(item, AttrPlace, static_cast<quint32>(copy.line));
        doc->SetAttribute(item, AttrEnd, static_cast<quint32>(copy.end));
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

        // The copies of a file written before a pleat could be taken on several departure lines name no place: they
        // are read in their own order, which is the order the figure held them in — first end then second, place
        // after place, on the first line of all.
        int unnamed = 0;

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
            else if (item.hasAttribute(AttrLine))
            {
                display.lines.append(VAbstractPattern::GetParametrUInt(item, AttrLine, NULL_ID_STR));
            }
            else
            {
                // Every place holds an entry, even one with nothing in it: a place the operation carries by itself
                // is not the display's to create, and the place must still be counted.
                OPleatCopy copy;
                copy.point = VAbstractPattern::GetParametrUInt(item, AttrCopy, NULL_ID_STR);
                copy.name = VDomDocument::GetParametrEmptyString(item, AttrName);
                if (item.hasAttribute(AttrDeparture))
                {
                    copy.departure = static_cast<int>(VAbstractPattern::GetParametrUInt(item, AttrDeparture, u"0"_s));
                    copy.line = static_cast<int>(VAbstractPattern::GetParametrUInt(item, AttrPlace, u"0"_s));
                    copy.end = static_cast<int>(VAbstractPattern::GetParametrUInt(item, AttrEnd, u"0"_s));
                }
                else
                {
                    copy.departure = 0;
                    copy.line = unnamed / 2 + 1;
                    copy.end = unnamed % 2;
                }
                ++unnamed;
                display.copies.append(copy);
            }
        }

        break; // Only the first element is read, exactly as the operation lines are: an operation has one.
    }

    return display;
}
