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
#include "../toolpoint/toolsinglepoint/toollinepoint/vtoollinepoint.h"
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
 * @brief ImplicitLine describes the line a point tool draws towards its base point.
 * @param pointId id of the source point.
 * @param obj geometric object of that point.
 * @param data container with variables.
 * @return descriptor, or a descriptor with NULL_ID endpoints when the point tool draws no line, or when
 *         its base is not a point (the intersect-axis tools draw towards a curve or a line).
 */
auto ImplicitLine(quint32 pointId, const QSharedPointer<VGObject> &obj, const VContainer *data)
    -> OOperationLine
{
    OOperationLine descriptor;

    auto *linePoint = qobject_cast<VToolLinePoint *>(VAbstractPattern::FindTool(obj->getIdTool()));
    if (linePoint == nullptr || linePoint->BasePointId() == NULL_ID)
    {
        return descriptor;
    }

    try
    {
        if (data->GetGObject(linePoint->BasePointId())->getType() != GOType::Point)
        {
            return descriptor;
        }
    }
    catch (const VExceptionBadId &e)
    {
        Q_UNUSED(e)
        return descriptor;
    }

    descriptor.source = pointId;
    descriptor.point1 = pointId;
    descriptor.point2 = linePoint->BasePointId();
    descriptor.typeLine = linePoint->getLineType();
    descriptor.lineColor = linePoint->GetLineColor();
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

            const OOperationLine descriptor = ImplicitLine(sItem.id, obj, data);
            if (descriptor.point1 != NULL_ID && descriptor.point2 != NULL_ID)
            {
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

    for (auto &line : lines)
    {
        // A line still attached keeps the id of the transformed copy that already exists.
        const auto old = std::find_if(oldLines.cbegin(),
                                      oldLines.cend(),
                                      [&line](const OOperationLine &oldLine) -> bool
                                      { return oldLine.source == line.source; });
        if (old != oldLines.cend())
        {
            line.result = old->result;
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
                                 const QHash<quint32, quint32> &referencedPoints)
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

        if (VAbstractPattern::FindTool(line.result) != nullptr)
        {
            continue; // Already created (lite parse, or a second call on the same data)
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
        initData.parse = Document::FullParse;
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
