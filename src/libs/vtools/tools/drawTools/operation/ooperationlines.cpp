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

//---------------------------------------------------------------------------------------------------------------------
auto OOperationLines::LineDescriptor(VToolLine *line) -> OOperationLine
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

//---------------------------------------------------------------------------------------------------------------------
auto OOperationLines::Collect(const QVector<OOperationLine> &selected) -> QVector<OOperationLine>
{
    QVector<OOperationLine> lines;
    lines.reserve(selected.size());

    for (const OOperationLine &line : selected)
    {
        // A line exists through its two endpoints and nothing else: a descriptor missing one of them, or naming
        // the same point twice, describes no line at all. Dropped here, so no caller has to check again.
        if (line.point1 == NULL_ID || line.point2 == NULL_ID || line.point1 == line.point2)
        {
            continue;
        }

        // The result id belongs to the operation that builds the line, never to the selection: it is allocated
        // when the tool is created (AllocateResultIds) or taken over from the operation being reconciled
        // (SyncLines). A selection restored from a file still carries the ids of an earlier operation.
        OOperationLine descriptor = line;
        descriptor.result = NULL_ID;
        lines.append(descriptor);
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
                                const QVector<OOperationLine> &selected) -> QVector<OOperationLine>
{
    QVector<OOperationLine> lines = Collect(selected);

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

        BuildLine(doc, data, scene, line, first, second, parse);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OOperationLines::CreateLine(VAbstractPattern *doc, VContainer *data, VMainGraphicsScene *scene,
                                 const OOperationLine &line, quint32 first, quint32 second, const Document &parse)
{
    // Same refusal as above: two endpoints that name no object, or the same one twice, describe no line.
    if (first == NULL_ID || second == NULL_ID || first == second)
    {
        return;
    }

    BuildLine(doc, data, scene, line, first, second, parse);
}

//---------------------------------------------------------------------------------------------------------------------
void OOperationLines::BuildLine(VAbstractPattern *doc, VContainer *data, VMainGraphicsScene *scene,
                                const OOperationLine &line, quint32 first, quint32 second, const Document &parse)
{
    // The full parse is the one that builds a line; every other parse only refreshes the ones already built.
    const bool build = parse == Document::FullParse;

    // A line that is already in the scene is never rebuilt — VToolLine::Create only ever adds a scene item
    // under a full parse — so it is left alone, and its pen is taken from the operation here, because
    // nothing else does. A transformed line has no element of its own: a lite parse, the one every option
    // edit triggers, never re-reads it, and its type and colour would otherwise stay those of the gesture
    // that produced it until the file is reopened.
    VDataTool *const existing = VAbstractPattern::FindTool(line.result);

    if (existing != nullptr)
    {
        if (auto *lineTool = qobject_cast<VToolLine *>(existing); lineTool != nullptr)
        {
            lineTool->RefreshPenFromParse(line.typeLine.isEmpty() ? VToolLineInitData{}.typeLine : line.typeLine,
                                          line.lineColor.isEmpty() ? VToolLineInitData{}.lineColor
                                                                   : line.lineColor);

            // The pen is not all that moves: a line whose endpoints the operation has just moved — a pleat cap
            // whose cap point follows the value, a copy line whose endpoints travel — would otherwise stay where
            // the parse before it put it, and the figure would come apart on screen. A derived line has no element
            // of its own, so nothing else re-reads its geometry: this is the call, and VToolLine knows it.
            lineTool->FullUpdateFromFile();
        }
        return;
    }

    // Missing outside of the full parse: nothing builds a scene item but the full parse, so a lite parse
    // has nothing to do here — and calling Create for a line that does not exist would refresh nothing.
    if (!build)
    {
        return;
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
