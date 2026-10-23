/************************************************************************
 **
 **  @file   opattern_graded.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  <sizeRanges> persistence of the Graded mode: parse (VPattern member)
 **          and DOM rewrite (AddSizeRanges).
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

#include "opattern_graded.h"

#include "vpattern.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"

#include <QDomElement>
#include <QHash>

namespace
{
const QString kTagSizeRanges = QStringLiteral("sizeRanges");
const QString kTagSizeRange  = QStringLiteral("sizeRange");
const QString kTagAxisValue  = QStringLiteral("axisValue");
const QString kTagPiece      = QStringLiteral("piece");
const QString kTagGroup      = QStringLiteral("group");

const QString kAttrMeasurement = QStringLiteral("measurement");
const QString kAttrSizeAxis    = QStringLiteral("sizeAxis");
const QString kAttrMin         = QStringLiteral("min");
const QString kAttrMax         = QStringLiteral("max");
const QString kAttrExportPath         = QStringLiteral("exportPath");
const QString kAttrExportFileName     = QStringLiteral("exportFileName");
const QString kAttrExportLayoutLayer  = QStringLiteral("exportLayoutLayerName");
const QString kAttrExportXScale       = QStringLiteral("exportXScale");
const QString kAttrExportYScale       = QStringLiteral("exportYScale");
const QString kAttrAxis        = QStringLiteral("axis");
const QString kAttrValue       = QStringLiteral("value");
const QString kAttrDx          = QStringLiteral("dx");
const QString kAttrDy          = QStringLiteral("dy");

// The referenced piece/group ids are deliberately NOT stored in the global "id" attribute:
// tool AddToFile() does a whole-document uniqueness scan on the "id" attribute and would
// flag these references as duplicate ids (see VToolSeamAllowance::AddToFile). Same scheme
// as group members (<member piece="…">), which use a dedicated attribute name.
const QString kAttrPieceId = QStringLiteral("pieceId");
const QString kAttrGroupId = QStringLiteral("groupId");

// Per-range grainline override stored inside a <piece>/<group> content element.
const QString kTagGrainline = QStringLiteral("grainline");
const QString kAttrMx       = QStringLiteral("mx");
const QString kAttrMy       = QStringLiteral("my");
const QString kAttrLength   = QStringLiteral("length");

// Per-range info-block overrides stored inside a <piece>/<group> content element.
const QString kTagBlock       = QStringLiteral("block");
const QString kAttrIndex      = QStringLiteral("index");
const QString kAttrFontScale  = QStringLiteral("fontScale");

// Per-range annotation overrides stored inside a <piece>/<group> content element.
const QString kTagAnnotation = QStringLiteral("annotation");
const QString kAttrWidth     = QStringLiteral("width");
const QString kAttrHeight    = QStringLiteral("height");

// Per-range miniature override stored inside a <piece> content element. Only its frame is stored:
// the drawing is always the piece at the reference size of the pattern, whichever the range.
const QString kTagThumbnail = QStringLiteral("thumbnail");

void ReadGrainlineOverride(const QDomElement &ownerElement, OSizerange::GrainlineOverride &gl)
{
    for (QDomNode node = ownerElement.firstChild(); not node.isNull(); node = node.nextSibling())
    {
        if (not node.isElement())
        {
            continue;
        }
        const QDomElement child = node.toElement();
        if (child.tagName() != kTagGrainline)
        {
            continue;
        }
        gl.valid = true;
        gl.pos = QPointF(child.attribute(kAttrMx).toDouble(), child.attribute(kAttrMy).toDouble());
        gl.lengthPx = child.attribute(kAttrLength).toDouble();
        return;
    }
}

void AppendGrainlineOverride(VAbstractPattern *doc, QDomElement &ownerElement, const OSizerange::GrainlineOverride &gl)
{
    if (!gl.valid)
    {
        return;
    }
    QDomElement glElement = doc->createElement(kTagGrainline);
    doc->SetAttribute(glElement, kAttrMx, gl.pos.x());
    doc->SetAttribute(glElement, kAttrMy, gl.pos.y());
    doc->SetAttribute(glElement, kAttrLength, gl.lengthPx);
    ownerElement.appendChild(glElement);
}

void ReadBlockOverrides(const QDomElement &ownerElement, QHash<int, OSizerange::BlockOverride> &out)
{
    for (QDomNode node = ownerElement.firstChild(); not node.isNull(); node = node.nextSibling())
    {
        if (not node.isElement())
        {
            continue;
        }
        const QDomElement child = node.toElement();
        if (child.tagName() != kTagBlock)
        {
            continue;
        }
        const int index = child.attribute(kAttrIndex).toInt();
        if (index < 1 || index > 11)
        {
            continue;
        }
        OSizerange::BlockOverride override;
        override.valid = true;
        override.geom.pos = QPointF(child.attribute(kAttrMx).toDouble(), child.attribute(kAttrMy).toDouble());
        override.geom.fontScale = child.attribute(kAttrFontScale).toDouble();
        if (override.geom.fontScale <= 0.0)
        {
            override.geom.fontScale = 1.0;
        }
        out.insert(index, override);
    }
}

void AppendBlockOverrides(VAbstractPattern *doc, QDomElement &ownerElement,
                          const QHash<int, OSizerange::BlockOverride> &blocks)
{
    for (auto it = blocks.cbegin(); it != blocks.cend(); ++it)
    {
        if (!it.value().valid)
        {
            continue;
        }
        QDomElement blockElement = doc->createElement(kTagBlock);
        doc->SetAttribute(blockElement, kAttrIndex, it.key());
        doc->SetAttribute(blockElement, kAttrMx, it.value().geom.pos.x());
        doc->SetAttribute(blockElement, kAttrMy, it.value().geom.pos.y());
        if (it.value().geom.fontScale > 0.0)
        {
            doc->SetAttribute(blockElement, kAttrFontScale, it.value().geom.fontScale);
        }
        ownerElement.appendChild(blockElement);
    }
}

void ReadAnnotationOverrides(const QDomElement &ownerElement, QHash<int, OSizerange::AnnotationOverride> &out)
{
    for (QDomNode node = ownerElement.firstChild(); not node.isNull(); node = node.nextSibling())
    {
        if (not node.isElement())
        {
            continue;
        }
        const QDomElement child = node.toElement();
        if (child.tagName() != kTagAnnotation)
        {
            continue;
        }
        OSizerange::AnnotationOverride override;
        override.valid = true;
        override.geom.pos = QPointF(child.attribute(kAttrMx).toDouble(), child.attribute(kAttrMy).toDouble());
        override.geom.boxWidth = child.attribute(kAttrWidth).toDouble();
        override.geom.boxHeight = child.attribute(kAttrHeight).toDouble();
        override.geom.fontScale = child.attribute(kAttrFontScale).toDouble();
        if (override.geom.fontScale <= 0.0)
        {
            override.geom.fontScale = 1.0;
        }
        const int index = child.attribute(kAttrIndex).toInt();
        if (index < 0)
        {
            continue; // annotation indices are 0-based — reject negatives (mirror block 1..11 bound)
        }
        out.insert(index, override);
    }
}

void AppendAnnotationOverrides(VAbstractPattern *doc, QDomElement &ownerElement,
                               const QHash<int, OSizerange::AnnotationOverride> &annotations)
{
    for (auto it = annotations.cbegin(); it != annotations.cend(); ++it)
    {
        if (!it.value().valid)
        {
            continue;
        }
        QDomElement annotationElement = doc->createElement(kTagAnnotation);
        doc->SetAttribute(annotationElement, kAttrIndex, it.key());
        doc->SetAttribute(annotationElement, kAttrMx, it.value().geom.pos.x());
        doc->SetAttribute(annotationElement, kAttrMy, it.value().geom.pos.y());
        if (it.value().geom.boxWidth > 0.0)
        {
            doc->SetAttribute(annotationElement, kAttrWidth, it.value().geom.boxWidth);
        }
        if (it.value().geom.boxHeight > 0.0)
        {
            doc->SetAttribute(annotationElement, kAttrHeight, it.value().geom.boxHeight);
        }
        if (it.value().geom.fontScale > 0.0)
        {
            doc->SetAttribute(annotationElement, kAttrFontScale, it.value().geom.fontScale);
        }
        ownerElement.appendChild(annotationElement);
    }
}

void ReadThumbnailOverride(const QDomElement &ownerElement, OSizerange::ThumbnailOverride &thumbnail)
{
    for (QDomNode node = ownerElement.firstChild(); not node.isNull(); node = node.nextSibling())
    {
        if (not node.isElement())
        {
            continue;
        }
        const QDomElement child = node.toElement();
        if (child.tagName() != kTagThumbnail)
        {
            continue;
        }
        thumbnail.valid = true;
        thumbnail.pos = QPointF(child.attribute(kAttrMx).toDouble(), child.attribute(kAttrMy).toDouble());
        thumbnail.width = child.attribute(kAttrWidth).toDouble();
        thumbnail.height = child.attribute(kAttrHeight).toDouble();
        return;
    }
}

void ReadMemberThumbnailOverrides(const QDomElement &ownerElement,
                                  QHash<quint32, OSizerange::ThumbnailOverride> &out)
{
    for (QDomNode node = ownerElement.firstChild(); not node.isNull(); node = node.nextSibling())
    {
        if (not node.isElement())
        {
            continue;
        }
        const QDomElement child = node.toElement();
        if (child.tagName() != kTagThumbnail)
        {
            continue;
        }
        const quint32 pieceId = child.attribute(kAttrPieceId).toUInt(); // malformed → 0 = NULL
        if (pieceId == NULL_ID)
        {
            continue;
        }
        OSizerange::ThumbnailOverride override;
        override.valid = true;
        override.pos = QPointF(child.attribute(kAttrMx).toDouble(), child.attribute(kAttrMy).toDouble());
        override.width = child.attribute(kAttrWidth).toDouble();
        override.height = child.attribute(kAttrHeight).toDouble();
        out.insert(pieceId, override);
    }
}

void AppendMemberThumbnailOverrides(VAbstractPattern *doc, QDomElement &ownerElement,
                                    const QHash<quint32, OSizerange::ThumbnailOverride> &thumbnails)
{
    for (auto it = thumbnails.cbegin(); it != thumbnails.cend(); ++it)
    {
        if (!it.value().valid)
        {
            continue;
        }
        QDomElement thumbnailElement = doc->createElement(kTagThumbnail);
        doc->SetAttribute(thumbnailElement, kAttrPieceId, it.key());
        doc->SetAttribute(thumbnailElement, kAttrMx, it.value().pos.x());
        doc->SetAttribute(thumbnailElement, kAttrMy, it.value().pos.y());
        doc->SetAttribute(thumbnailElement, kAttrWidth, it.value().width);
        doc->SetAttribute(thumbnailElement, kAttrHeight, it.value().height);
        ownerElement.appendChild(thumbnailElement);
    }
}

void AppendThumbnailOverride(VAbstractPattern *doc, QDomElement &ownerElement,
                             const OSizerange::ThumbnailOverride &thumbnail)
{
    if (!thumbnail.valid)
    {
        return;
    }
    QDomElement thumbnailElement = doc->createElement(kTagThumbnail);
    doc->SetAttribute(thumbnailElement, kAttrMx, thumbnail.pos.x());
    doc->SetAttribute(thumbnailElement, kAttrMy, thumbnail.pos.y());
    doc->SetAttribute(thumbnailElement, kAttrWidth, thumbnail.width);
    doc->SetAttribute(thumbnailElement, kAttrHeight, thumbnail.height);
    ownerElement.appendChild(thumbnailElement);
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseSizeRanges(const QDomElement &domElement)
{
    QHash<QString, OSizerange> ranges;

    for (QDomNode node = domElement.firstChild(); not node.isNull(); node = node.nextSibling())
    {
        if (not node.isElement())
        {
            continue;
        }

        const QDomElement rangeElement = node.toElement();
        if (rangeElement.tagName() != kTagSizeRange)
        {
            continue;
        }

        OSizerange range;
        range.name = GetParametrEmptyString(rangeElement, VAbstractPattern::AttrName);
        range.measurement = GetParametrEmptyString(rangeElement, kAttrMeasurement);
        range.sizeAxis = GetParametrEmptyString(rangeElement, kAttrSizeAxis);
        range.minValue = GetParametrEmptyString(rangeElement, kAttrMin);
        range.maxValue = GetParametrEmptyString(rangeElement, kAttrMax);

        // Export settings of the graded SVG export (optional on <sizeRange>).
        range.exportPath = GetParametrEmptyString(rangeElement, kAttrExportPath);
        range.exportFileName = GetParametrEmptyString(rangeElement, kAttrExportFileName);
        range.exportLayoutLayerName = GetParametrEmptyString(rangeElement, kAttrExportLayoutLayer);
        {
            bool ok = false;
            const double x = rangeElement.attribute(kAttrExportXScale).toDouble(&ok);
            range.exportXScale = ok && x > 0.0 ? x : 1.0;
        }
        {
            bool ok = false;
            const double y = rangeElement.attribute(kAttrExportYScale).toDouble(&ok);
            range.exportYScale = ok && y > 0.0 ? y : 1.0;
        }

        for (QDomNode child = rangeElement.firstChild(); not child.isNull(); child = child.nextSibling())
        {
            if (not child.isElement())
            {
                continue;
            }

            const QDomElement childElement = child.toElement();
            const QString tag = childElement.tagName();

            if (tag == kTagAxisValue)
            {
                const QString axis = GetParametrEmptyString(childElement, kAttrAxis);
                const QString value = GetParametrEmptyString(childElement, kAttrValue);
                if (!axis.isEmpty() && !value.isEmpty())
                {
                    range.axisValues.insert(axis, value);
                }
            }
            else if (tag == kTagPiece)
            {
                OSizerange::PieceContent content;
                content.dx = childElement.attribute(kAttrDx).toDouble(); // tolerant: malformed → 0
                content.dy = childElement.attribute(kAttrDy).toDouble();
                ReadGrainlineOverride(childElement, content.grainline);
                ReadBlockOverrides(childElement, content.blocks);
                ReadAnnotationOverrides(childElement, content.annotations);
                ReadThumbnailOverride(childElement, content.thumbnail);
                const quint32 pieceId = childElement.attribute(kAttrPieceId).toUInt(); // malformed → 0 = NULL
                if (pieceId != NULL_ID)
                {
                    range.pieceContent.insert(pieceId, content);
                }
            }
            else if (tag == kTagGroup)
            {
                OSizerange::GroupContent content;
                content.dx = childElement.attribute(kAttrDx).toDouble(); // tolerant: malformed → 0
                content.dy = childElement.attribute(kAttrDy).toDouble();
                ReadGrainlineOverride(childElement, content.grainline);
                ReadBlockOverrides(childElement, content.blocks);
                ReadAnnotationOverrides(childElement, content.annotations);
                ReadMemberThumbnailOverrides(childElement, content.memberThumbnails);
                const quint32 groupId = childElement.attribute(kAttrGroupId).toUInt(); // malformed → 0 = NULL
                if (groupId != NULL_ID)
                {
                    range.groupContent.insert(groupId, content);
                }
            }
        }

        if (not range.name.isEmpty())
        {
            ranges.insert(range.name, range);
        }
    }

    data->SetSizeRanges(ranges);
}

//---------------------------------------------------------------------------------------------------------------------
void WriteSizeRanges(VAbstractPattern *doc, QDomElement &rootElement, const VContainer *data)
{
    if (doc == nullptr || data == nullptr)
    {
        return; // nothing to write — and no container is not the same thing as "no ranges"
    }

    // <sizeRanges> is a single sibling element under the pattern root — replace any stale copy.
    for (QDomElement stale = rootElement.firstChildElement(kTagSizeRanges); not stale.isNull();
         stale = rootElement.firstChildElement(kTagSizeRanges))
    {
        rootElement.removeChild(stale);
    }

    const auto *ranges = data->SizeRanges();
    if (ranges == nullptr || ranges->isEmpty())
    {
        return; // the stale element is gone: "no ranges" is written by its absence
    }

    QDomElement rangesElement = doc->createElement(kTagSizeRanges);
    for (auto it = ranges->cbegin(); it != ranges->cend(); ++it)
    {
        const OSizerange &range = it.value();

        QDomElement rangeElement = doc->createElement(kTagSizeRange);
        doc->SetAttribute(rangeElement, VAbstractPattern::AttrName, range.name);
        doc->SetAttribute(rangeElement, kAttrMeasurement, range.measurement);
        doc->SetAttribute(rangeElement, kAttrSizeAxis, range.sizeAxis);
        doc->SetAttribute(rangeElement, kAttrMin, range.minValue);
        doc->SetAttribute(rangeElement, kAttrMax, range.maxValue);

        // Export settings of the graded SVG export (only the overrides/non-defaults).
        if (not range.exportPath.isEmpty())
        {
            doc->SetAttribute(rangeElement, kAttrExportPath, range.exportPath);
        }
        if (not range.exportFileName.isEmpty())
        {
            doc->SetAttribute(rangeElement, kAttrExportFileName, range.exportFileName);
        }
        if (not range.exportLayoutLayerName.isEmpty())
        {
            doc->SetAttribute(rangeElement, kAttrExportLayoutLayer, range.exportLayoutLayerName);
        }
        if (range.exportXScale != 1.0)
        {
            doc->SetAttribute(rangeElement, kAttrExportXScale, range.exportXScale);
        }
        if (range.exportYScale != 1.0)
        {
            doc->SetAttribute(rangeElement, kAttrExportYScale, range.exportYScale);
        }

        for (auto av = range.axisValues.cbegin(); av != range.axisValues.cend(); ++av)
        {
            if (av.key().isEmpty() || av.value().isEmpty())
            {
                continue; // an empty axisValue would make the file unreadable
            }
            QDomElement axisElement = doc->createElement(kTagAxisValue);
            doc->SetAttribute(axisElement, kAttrAxis, av.key());
            doc->SetAttribute(axisElement, kAttrValue, av.value());
            rangeElement.appendChild(axisElement);
        }

        for (auto pc = range.pieceContent.cbegin(); pc != range.pieceContent.cend(); ++pc)
        {
            QDomElement pieceElement = doc->createElement(kTagPiece);
            doc->SetAttribute(pieceElement, kAttrPieceId, pc.key());
            doc->SetAttribute(pieceElement, kAttrDx, pc.value().dx);
            doc->SetAttribute(pieceElement, kAttrDy, pc.value().dy);
            AppendGrainlineOverride(doc, pieceElement, pc.value().grainline);
            AppendBlockOverrides(doc, pieceElement, pc.value().blocks);
            AppendAnnotationOverrides(doc, pieceElement, pc.value().annotations);
            AppendThumbnailOverride(doc, pieceElement, pc.value().thumbnail);
            rangeElement.appendChild(pieceElement);
        }

        for (auto gc = range.groupContent.cbegin(); gc != range.groupContent.cend(); ++gc)
        {
            QDomElement groupElement = doc->createElement(kTagGroup);
            doc->SetAttribute(groupElement, kAttrGroupId, gc.key());
            doc->SetAttribute(groupElement, kAttrDx, gc.value().dx);
            doc->SetAttribute(groupElement, kAttrDy, gc.value().dy);
            AppendGrainlineOverride(doc, groupElement, gc.value().grainline);
            AppendBlockOverrides(doc, groupElement, gc.value().blocks);
            AppendAnnotationOverrides(doc, groupElement, gc.value().annotations);
            AppendMemberThumbnailOverrides(doc, groupElement, gc.value().memberThumbnails);
            rangeElement.appendChild(groupElement);
        }

        rangesElement.appendChild(rangeElement);
    }

    // **Placed rather than appended.** Where <sizeRanges> belongs depends on what the document
    // already carries — groups, and the draws before them — and that question has one answer, asked
    // in one place. See VAbstractPattern::InsertRootElement.
    doc->InsertRootElement(rangesElement);
}

//---------------------------------------------------------------------------------------------------------------------
void AddSizeRanges(VAbstractPattern *doc, QDomElement &rootElement, const VContainer *data)
{
    if (doc == nullptr || data == nullptr)
    {
        return;
    }

    WriteSizeRanges(doc, rootElement, data);

    // **A write made here is a write made outside the document's own setters**, so the pattern has
    // to be told — the title, the Save action and the save prompt all read it. Done here, once:
    // every writer of <sizeRanges> goes through this function. The save itself does not: it calls
    // WriteSizeRanges, because a file being written must not come out looking dirty.
    doc->NotifyPatternChanged();
}
