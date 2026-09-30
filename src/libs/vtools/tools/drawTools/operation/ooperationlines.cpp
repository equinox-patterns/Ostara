/************************************************************************
 **
 **  @file   ooperationlines.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  Lines attached to an operation tool: collect, transform, persist.
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

#include "ooperationlines.h"

#include <algorithm>

#include <QSharedPointer>

#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vgobject.h"
#include "../vtoolline.h"
#include "../vpatterndb/vcontainer.h"
#include "vabstractoperation.h"

using namespace Qt::Literals::StringLiterals;

const QString OOperationLines::TagLines = QStringLiteral("lines");
const QString OOperationLines::AttrIdSource = QStringLiteral("idSource");
const QString OOperationLines::AttrIdPoint1 = QStringLiteral("idPoint1");
const QString OOperationLines::AttrIdPoint2 = QStringLiteral("idPoint2");
const QString OOperationLines::AttrIdResult = QStringLiteral("idResult");

namespace
{
/**
 * @brief ExplicitLine describes a line created by the Line tool.
 * @param line line tool.
 * @return descriptor, or a descriptor with NULL_ID endpoints when the line is unknown.
 */
auto ExplicitLine(VToolLine *line) -> OOperationLine
{
    OOperationLine descriptor;
    if (line == nullptr)
    {
        return descriptor;
    }

    descriptor.source = line->getId();
    descriptor.point1 = line->FirstPointId();
    descriptor.point2 = line->SecondPointId();
    descriptor.typeLine = line->getLineType();
    descriptor.lineColor = line->GetLineColor();
    return descriptor;
}

/**
 * @brief DrawnLine describes one segment a tool draws, exactly as that tool registered it.
 * @param source id of the tool that draws the segment.
 * @param segment the segment itself.
 * @return descriptor, or a descriptor with NULL_ID endpoints when the segment is degenerate.
 */
auto DrawnLine(quint32 source, const VDrawnSegment &segment) -> OOperationLine
{
    OOperationLine descriptor;

    if (segment.point1 == NULL_ID || segment.point2 == NULL_ID || segment.point1 == segment.point2)
    {
        return descriptor;
    }

    descriptor.source = source;
    descriptor.point1 = segment.point1;
    descriptor.point2 = segment.point2;
    descriptor.typeLine = segment.typeLine;
    descriptor.lineColor = segment.lineColor;
    return descriptor;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OOperationLines::Collect(const VContainer *data, const QVector<SourceItem> &source,
                              const QVector<quint32> &explicitLines) -> QVector<OOperationLine>
{
    QVector<OOperationLine> lines;
    lines.reserve(explicitLines.size() + source.size());

    for (auto id : explicitLines)
    {
        const OOperationLine descriptor = ExplicitLine(qobject_cast<VToolLine *>(VAbstractPattern::FindTool(id)));
        if (descriptor.point1 != NULL_ID && descriptor.point2 != NULL_ID)
        {
            lines.append(descriptor);
        }
    }

    for (const auto &sItem : source)
    {
        try
        {
            const QSharedPointer<VGObject> obj = data->GetGObject(sItem.id);
            if (obj->getType() != GOType::Point)
            {
                continue;
            }

            VAbstractTool *tool = qobject_cast<VAbstractTool *>(VAbstractPattern::FindTool(obj->getIdTool()));
            if (tool == nullptr)
            {
                continue;
            }

            // Every segment the tool draws: the one towards its base point for a line point, the four sides of
            // a rectangle, the two tracés of a dart cap. They are all carried by that same tool, so the pair of
            // points is what tells them apart (see SyncLines). A segment a click has already taken stays the
            // line the user chose; the two descriptors would be the very same line twice.
            for (const VDrawnSegment &segment : tool->DrawnSegments())
            {
                const OOperationLine descriptor = DrawnLine(tool->getId(), segment);
                if (descriptor.point1 == NULL_ID || descriptor.point2 == NULL_ID)
                {
                    continue;
                }

                const bool alreadyTaken = std::any_of(
                    lines.cbegin(),
                    lines.cend(),
                    [&descriptor](const OOperationLine &line) -> bool
                    {
                        return (line.point1 == descriptor.point1 && line.point2 == descriptor.point2)
                               || (line.point1 == descriptor.point2 && line.point2 == descriptor.point1);
                    });
                if (alreadyTaken)
                {
                    continue;
                }

                lines.append(descriptor);
            }
        }
        catch (const VExceptionBadId &e)
        {
            Q_UNUSED(e)
            continue;
        }
    }

    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
void OOperationLines::AllocateResultIds(VContainer *data, QVector<OOperationLine> &lines)
{
    for (auto &line : lines)
    {
        if (line.result == NULL_ID)
        {
            line.result = data->getNextId();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OOperationLines::SyncLines(VContainer *data, const QVector<OOperationLine> &oldLines,
                                const QVector<SourceItem> &source,
                                const QVector<quint32> &explicitLines) -> QVector<OOperationLine>
{
    QVector<OOperationLine> lines = Collect(data, source, explicitLines);

    // A line still attached keeps the id of the transformed copy that already exists. The carrying tool alone
    // cannot tell two lines apart: a tool that draws several segments — the four sides of a rectangle, the two
    // tracés of a dart cap — carries them all under its own id, and matching on that id alone would give every
    // one of them the same result id, so only the first would ever be built. The two endpoints come first.
    for (auto &line : lines)
    {
        const auto sameSegment = [&line](const OOperationLine &oldLine) -> bool
        {
            return oldLine.source == line.source && oldLine.point1 == line.point1 && oldLine.point2 == line.point2;
        };

        if (const auto old = std::find_if(oldLines.cbegin(), oldLines.cend(), sameSegment); old != oldLines.cend())
        {
            line.result = old->result;
        }
    }

    // What is left has no exact counterpart: a segment whose endpoints moved — the base point of a point tool,
    // or of a rectangle, was re-pointed — is still the same line, and takes the next free id of the tool that
    // carries it, in the order both lists give them. Taking it over rather than allocating a new one is what
    // keeps the transformed line it describes from being left behind as a second line nobody owns.
    for (auto &line : lines)
    {
        if (line.result != NULL_ID)
        {
            continue;
        }

        for (const auto &old : oldLines)
        {
            if (old.source != line.source || old.result == NULL_ID)
            {
                continue;
            }

            const bool taken = std::any_of(lines.cbegin(),
                                           lines.cend(),
                                           [&old](const OOperationLine &other) -> bool
                                           { return other.result == old.result; });
            if (!taken)
            {
                line.result = old.result;
                break;
            }
        }
    }

    AllocateResultIds(data, lines);
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto OOperationLines::IsExplicit(const OOperationLine &line) -> bool
{
    return qobject_cast<VToolLine *>(VAbstractPattern::FindTool(line.source)) != nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OOperationLines::PointImage(quint32 pointId, const QVector<SourceItem> &source,
                                 const QVector<DestinationItem> &destination,
                                 const QVector<quint32> &fixedPoints,
                                 const QHash<quint32, quint32> &referencedPoints) -> quint32
{
    if (pointId == NULL_ID)
    {
        return NULL_ID;
    }

    // A referenced point has no copy of its own: its image is the point it was anchored on — the merge
    // point of the host part. Read before the positional lookup, which knows the point as a source but has
    // no destination object to answer with.
    if (const auto referenced = referencedPoints.constFind(pointId); referenced != referencedPoints.cend())
    {
        return referenced.value();
    }

    const qsizetype count = std::min(source.size(), destination.size());
    for (qsizetype i = 0; i < count; ++i)
    {
        if (source.at(i).id == pointId)
        {
            return destination.at(i).id;
        }
    }

    // A fixed point of the transformation is its own image: rotating around it, or taking it on the
    // mirror axis, leaves it in place.
    if (fixedPoints.contains(pointId))
    {
        return pointId;
    }

    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
void OOperationLines::CreateLines(VAbstractPattern *doc, VContainer *data, VMainGraphicsScene *scene,
                                 const QVector<OOperationLine> &lines, const QVector<SourceItem> &source,
                                 const QVector<DestinationItem> &destination, const QVector<quint32> &fixedPoints,
                                 const Document &parse, const QHash<quint32, quint32> &referencedPoints)
{
    // The full parse is the one that builds a line; every other parse refreshes the ones already built.
    const bool build = parse == Document::FullParse;

    for (const auto &line : lines)
    {
        const quint32 first = PointImage(line.point1, source, destination, fixedPoints, referencedPoints);
        const quint32 second = PointImage(line.point2, source, destination, fixedPoints, referencedPoints);

        // No image for one endpoint: the line cannot exist, and nothing is created. The preview shows
        // the same thing, so what is displayed is what will be built.
        if (first == NULL_ID || second == NULL_ID || first == second)
        {
            continue;
        }

        // Built by the full parse — including a second call on the same data — or missing outside of it:
        // both are left alone. VToolLine::Create only ever adds a scene item under a full parse, so calling
        // it for a line that does not exist elsewhere would refresh nothing at all.
        if ((VAbstractPattern::FindTool(line.result) != nullptr) == build)
        {
            continue;
        }

        VToolLineInitData initData;
        initData.id = line.result;
        initData.firstPoint = first;
        initData.secondPoint = second;
        initData.typeLine = line.typeLine.isEmpty() ? VToolLineInitData{}.typeLine : line.typeLine;
        initData.lineColor = line.lineColor.isEmpty() ? VToolLineInitData{}.lineColor : line.lineColor;
        initData.scene = scene;
        initData.doc = doc;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;
        initData.derived = true;

        // A transformed line must never make the operation creation fail: if one cannot be built, it is
        // reported and skipped, the operation itself is still created.
        try
        {
            VToolLine::Create(initData);
        }
        catch (const VException &e)
        {
            // Reported and skipped, as announced above — but in the log only: this runs from the creation
            // itself, and a warning would raise a modal box in the middle of it.
            qCDebug(vTool,
                    "Transformed line %u between points %u and %u (source %u) could not be created: %s",
                    line.result,
                    first,
                    second,
                    line.source,
                    e.ErrorMessage().toUtf8().constData());
        }
    }
}
//---------------------------------------------------------------------------------------------------------------------
void OOperationLines::SaveLines(VAbstractPattern *doc, QDomElement &tag, const QVector<OOperationLine> &lines)
{
    if (lines.isEmpty())
    {
        return;
    }

    QDomElement tagLines = doc->createElement(TagLines);
    for (const auto &line : std::as_const(lines))
    {
        QDomElement item = doc->createElement(VAbstractOperation::TagItem);
        doc->SetAttribute(item, AttrIdSource, line.source);
        doc->SetAttribute(item, AttrIdPoint1, line.point1);
        doc->SetAttribute(item, AttrIdPoint2, line.point2);
        doc->SetAttribute(item, AttrIdResult, line.result);
        doc->SetAttribute(item, AttrTypeLine, line.typeLine);
        doc->SetAttribute(item, AttrLineColor, line.lineColor);
        tagLines.appendChild(item);
    }
    tag.appendChild(tagLines);
}

//---------------------------------------------------------------------------------------------------------------------
auto OOperationLines::ExtractLines(const QDomElement &domElement) -> QVector<OOperationLine>
{
    QVector<OOperationLine> lines;

    const QDomNodeList nodeList = domElement.childNodes();
    for (int i = 0; i < nodeList.size(); ++i)
    {
        const QDomElement element = nodeList.at(i).toElement();
        if (element.isNull() || element.tagName() != TagLines)
        {
            continue;
        }

        const QDomNodeList items = element.childNodes();
        for (int j = 0; j < items.size(); ++j)
        {
            const QDomElement item = items.at(j).toElement();
            if (item.isNull() || item.tagName() != VAbstractOperation::TagItem)
            {
                continue;
            }

            OOperationLine line;
            line.source = VAbstractPattern::GetParametrUInt(item, AttrIdSource, NULL_ID_STR);
            line.point1 = VAbstractPattern::GetParametrUInt(item, AttrIdPoint1, NULL_ID_STR);
            line.point2 = VAbstractPattern::GetParametrUInt(item, AttrIdPoint2, NULL_ID_STR);
            line.result = VAbstractPattern::GetParametrUInt(item, AttrIdResult, NULL_ID_STR);
            line.typeLine = VAbstractPattern::GetParametrString(item, AttrTypeLine, TypeLineDefault);
            line.lineColor = VAbstractPattern::GetParametrString(item, AttrLineColor, ColorBlack);
            lines.append(line);
        }

        break;
    }

    return lines;
}
