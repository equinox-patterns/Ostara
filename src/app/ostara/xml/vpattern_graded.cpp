/************************************************************************
 **
 **  @file   vpattern_graded.cpp
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

#include "vpattern_graded.h"

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
void AddSizeRanges(VAbstractPattern *doc, QDomElement &rootElement, const VContainer *data)
{
    // <sizeRanges> is a single sibling element under the pattern root — replace any stale copy.
    for (QDomElement stale = rootElement.firstChildElement(kTagSizeRanges); not stale.isNull();
         stale = rootElement.firstChildElement(kTagSizeRanges))
    {
        rootElement.removeChild(stale);
    }

    const auto *ranges = data != nullptr ? data->SizeRanges() : nullptr;
    if (ranges == nullptr || ranges->isEmpty())
    {
        return;
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
            rangeElement.appendChild(groupElement);
        }

        rangesElement.appendChild(rangeElement);
    }

    rootElement.appendChild(rangesElement);
}
