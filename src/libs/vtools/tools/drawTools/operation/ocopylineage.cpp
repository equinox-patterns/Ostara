/************************************************************************
 **
 **  @file   ocopylineage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  Where a copy an operation made comes from, and what has been copied from a curve.
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

#include "ocopylineage.h"

#include <QDomNodeList>
#include <QHash>
#include <QSet>

#include "../../../tools/toolsdef.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../toolcurve/osplinepathportions.h"
#include "../toolcurve/otoolarmholecurve.h"
#include "../toolpoint/toolsinglepoint/toolcut/vtoolcutarc.h"
#include "../toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vpatterndb/vcontainer.h"
#include "vabstractoperation.h"

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/** @brief The two lists of one operation element, paired by rank. */
auto ImagesOf(const QDomElement &operation) -> QVector<OCopyLineage::Image>
{
    QVector<OCopyLineage::Image> images;

    const QVector<SourceItem> source = VAbstractOperation::ExtractSourceData(operation);
    const QVector<DestinationItem> destination = VAbstractOperation::ExtractDestinationData(operation);
    const quint32 operationId = VDomDocument::GetParametrId(operation);

    const qsizetype count = qMin(source.size(), destination.size());
    images.reserve(count);

    for (qsizetype i = 0; i < count; ++i)
    {
        // A referenced source — the merge point of a transfer — keeps its rank with no image of its own.
        if (destination.at(i).id == NULL_ID || source.at(i).id == NULL_ID)
        {
            continue;
        }

        images.append({.operationId = operationId, .copyId = destination.at(i).id, .sourceId = source.at(i).id});
    }

    return images;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OCopyLineage::OperationOf(VAbstractPattern *doc, const VContainer *data, quint32 copyId) -> QDomElement
{
    if (doc == nullptr || data == nullptr || copyId == NULL_ID || not data->HasObject(copyId))
    {
        return {};
    }

    const quint32 owner = data->GetGObject(copyId)->getIdObject();

    if (owner == NULL_ID || owner == copyId)
    {
        return {}; // an object of its own tool: no operation made it
    }

    // Looked up without a tag, then told by its own: the owner of a half or of a portion is a cut or a curve, and a
    // lookup that names the wrong tag rebuilds the whole id cache of the document before saying no.
    const QDomElement element = doc->FindElementById(owner);
    return element.tagName() == VAbstractPattern::TagOperation ? element : QDomElement();
}

//---------------------------------------------------------------------------------------------------------------------
auto OCopyLineage::SourceOf(VAbstractPattern *doc, const VContainer *data, quint32 copyId) -> quint32
{
    const QDomElement operation = OperationOf(doc, data, copyId);

    if (operation.isNull())
    {
        return NULL_ID;
    }

    // The figure of a pleat, the point a dart cap adds, the pieces of a copy: objects an operation draws besides its
    // copies are not in its destination list, and are nobody's image.
    for (const Image &image : ImagesOf(operation))
    {
        if (image.copyId == copyId)
        {
            return image.sourceId;
        }
    }

    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCopyLineage::Ancestors(VAbstractPattern *doc, const VContainer *data, quint32 copyId) -> QVector<quint32>
{
    QVector<quint32> ancestors;
    QSet<quint32> seen{copyId};

    for (quint32 source = SourceOf(doc, data, copyId); source != NULL_ID; source = SourceOf(doc, data, source))
    {
        // A file can be written by hand: a loop in the family would never end otherwise.
        if (seen.contains(source))
        {
            break;
        }

        seen.insert(source);
        ancestors.append(source);
    }

    return ancestors;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCopyLineage::Descendants(VAbstractPattern *doc, quint32 objectId) -> QVector<Image>
{
    QVector<Image> descendants;

    if (doc == nullptr || objectId == NULL_ID)
    {
        return descendants;
    }

    // Every image of the pattern, by the object it was made from: read once, then walked.
    QHash<quint32, QVector<Image>> imagesBySource;

    const QDomNodeList operations = doc->elementsByTagName(VAbstractPattern::TagOperation);
    for (int i = 0; i < operations.size(); ++i)
    {
        const QDomElement operation = operations.at(i).toElement();
        if (operation.isNull())
        {
            continue;
        }

        for (const Image &image : ImagesOf(operation))
        {
            imagesBySource[image.sourceId].append(image);
        }
    }

    QVector<quint32> pending{objectId};
    QSet<quint32> seen{objectId};

    while (not pending.isEmpty())
    {
        const quint32 current = pending.takeFirst();

        for (const Image &image : imagesBySource.value(current))
        {
            if (seen.contains(image.copyId))
            {
                continue;
            }

            seen.insert(image.copyId);
            descendants.append(image);
            pending.append(image.copyId);
        }
    }

    return descendants;
}

//---------------------------------------------------------------------------------------------------------------------
auto OCopyLineage::CutNodesOf(VAbstractPattern *doc, quint32 curveId) -> QVector<int>
{
    if (doc == nullptr || curveId == NULL_ID)
    {
        return {};
    }

    // Without a tag, for the reason OperationOf gives: a copy's family can end on a half, a portion, an arc.
    const QDomElement element = doc->FindElementById(curveId);

    if (element.isNull() || element.tagName() != VAbstractPattern::TagSpline)
    {
        return {};
    }

    if (element.attribute(AttrType) == OToolArmholeCurve::ToolType)
    {
        return VDomDocument::GetParametrBool(element, AttrSegmentAtCarrure, falseStr)
                   ? QVector<int>{OToolArmholeCurve::CarrureNode}
                   : QVector<int>{};
    }

    return OSplinePathPortions::DecodeNodes(VDomDocument::GetParametrEmptyString(element, AttrCutNodes));
}

//---------------------------------------------------------------------------------------------------------------------
auto OCopyLineage::CutsOn(VAbstractPattern *doc, quint32 curveId) -> QVector<QDomElement>
{
    return CutsByCurve(doc).value(curveId);
}

//---------------------------------------------------------------------------------------------------------------------
auto OCopyLineage::CutsByCurve(VAbstractPattern *doc) -> QHash<quint32, QVector<QDomElement>>
{
    QHash<quint32, QVector<QDomElement>> cuts;

    if (doc == nullptr)
    {
        return cuts;
    }

    const QDomNodeList points = doc->elementsByTagName(VAbstractPattern::TagPoint);
    for (int i = 0; i < points.size(); ++i)
    {
        const QDomElement point = points.at(i).toElement();
        if (point.isNull())
        {
            continue;
        }

        const QString type = point.attribute(AttrType);

        QString base;
        if (type == VToolCutSpline::ToolType)
        {
            base = point.attribute(VToolCutSpline::AttrSpline);
        }
        else if (type == VToolCutSplinePath::ToolType)
        {
            base = point.attribute(VToolCutSplinePath::AttrSplinePath);
        }
        else if (type == VToolCutArc::ToolType)
        {
            base = point.attribute(AttrArc);
        }
        else
        {
            continue;
        }

        bool ok = false;
        const quint32 curveId = base.toUInt(&ok);
        if (ok && curveId != NULL_ID)
        {
            cuts[curveId].append(point);
        }
    }

    return cuts;
}
