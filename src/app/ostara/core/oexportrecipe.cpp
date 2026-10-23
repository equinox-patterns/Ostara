/************************************************************************
 **
 **  @file   oexportrecipe.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/10/2026
 **
 **  @brief  The format and the decor of a generation, written for Rabnest.
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

#include "oexportrecipe.h"

#include "oexportoutput.h"
#include "oexporttemplate.h"

#include <QDomDocument>
#include <QDomNamedNodeMap>
#include <QList>
#include <QString>
#include <QXmlStreamWriter>

namespace
{
/// The recipe's own version, which is not the template's and not the payload's.
///
/// Three markers for three lifetimes: the payload's says how the file is carried, the template's
/// how a decor is written, and this one what a generation was asked for. They move for different
/// reasons, so they are not the same number.
constexpr auto kRecipeVersion = "1";

constexpr auto kTagMetadata = "metadata";
constexpr auto kTagRecipe = "exportRecipe";
constexpr auto kTagFormat = "format";
constexpr auto kTagDecor = "decor";
constexpr auto kTagName = "name";

constexpr auto kAttrVersion = "version";
constexpr auto kAttrToken = "token";
constexpr auto kAttrFramed = "framed";
constexpr auto kAttrColumns = "columns";
constexpr auto kAttrMarginMm = "marginMm";
constexpr auto kAttrGapMm = "gapMm";
constexpr auto kAttrFrameHeightMm = "frameHeightMm";
constexpr auto kAttrDocumentWidthMm = "documentWidthMm";
constexpr auto kAttrTemplate = "template";

void WriteElement(QXmlStreamWriter &writer, const QDomElement &element)
{
    writer.writeStartElement(element.tagName());

    const QDomNamedNodeMap attributes = element.attributes();
    for (int index = 0; index < attributes.count(); ++index)
    {
        const QDomAttr attribute = attributes.item(index).toAttr();
        if (!attribute.isNull())
        {
            writer.writeAttribute(attribute.name(), attribute.value());
        }
    }

    for (QDomNode child = element.firstChild(); !child.isNull(); child = child.nextSibling())
    {
        if (child.isElement())
        {
            WriteElement(writer, child.toElement());
        }
        else if (child.isText())
        {
            writer.writeCharacters(child.toText().data());
        }
    }

    writer.writeEndElement();
}
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
auto OExportRecipe::Of(QDomDocument &doc, const OExportOutput &output, const OExportTemplate *marks,
                       bool framed) -> QDomElement
{
    QDomElement metadata = doc.createElement(QLatin1String(kTagMetadata));
    QDomElement recipe = doc.createElement(QLatin1String(kTagRecipe));
    recipe.setAttribute(QLatin1String(kAttrVersion), QLatin1String(kRecipeVersion));
    metadata.appendChild(recipe);

    // The format, in the words the print model is named by and in millimetres — the units every
    // consumer of this works in, and the ones a change of format is reasoned about in.
    //
    // `framed` is stated rather than derived, because it is the FORMAT that has a frame and not the
    // template: the same template draws a frame on the framed formats and none on the others, so a
    // reader that only saw the template would not know which of the two it was looking at.
    QDomElement format = doc.createElement(QLatin1String(kTagFormat));
    format.setAttribute(QLatin1String(kAttrToken), output.printModelToken);
    format.setAttribute(QLatin1String(kAttrFramed), framed);
    format.setAttribute(QLatin1String(kAttrColumns), QString::number(output.columns));
    format.setAttribute(QLatin1String(kAttrMarginMm), QString::number(output.marginMm));
    format.setAttribute(QLatin1String(kAttrGapMm), QString::number(output.gapMm));
    format.setAttribute(QLatin1String(kAttrFrameHeightMm), QString::number(output.frameHeightMm));
    format.setAttribute(QLatin1String(kAttrDocumentWidthMm), QString::number(output.documentWidthMm));
    recipe.appendChild(format);

    // The decor, in the template's own words: its three blocks are copied verbatim out of the very
    // element a `.oamt` file holds, so a template has one grammar and Rabnest reads the same one
    // the template editor writes. Only `<name>` is left out — the name is the attribute above, and
    // writing it twice would make two answers to one question.
    if (marks != nullptr)
    {
        const QDomElement source = marks->ToXml(doc);

        QDomElement decor = doc.createElement(QLatin1String(kTagDecor));
        decor.setAttribute(QLatin1String(kAttrTemplate), marks->name);
        decor.setAttribute(QLatin1String(kAttrVersion),
                           source.attribute(QLatin1String(kAttrVersion)));

        QList<QDomNode> blocks;
        for (QDomElement child = source.firstChildElement(); !child.isNull();
             child = child.nextSiblingElement())
        {
            if (child.tagName() != QLatin1String(kTagName))
            {
                blocks.append(child);
            }
        }
        for (const QDomNode &block : blocks)
        {
            decor.appendChild(block);
        }

        recipe.appendChild(decor);
    }

    return metadata;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportRecipe::Write(QXmlStreamWriter &writer, const QDomElement &element)
{
    if (!element.isNull())
    {
        WriteElement(writer, element);
    }
}
