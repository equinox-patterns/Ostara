/************************************************************************
 **
 **  @file   ocoverpageresolver.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  One information of a cover page, turned into the lines the page draws.
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

#include "ocoverpageresolver.h"

#include "ocoverpagecatalogue.h"
#include "ologolibrary.h"
#include "ologorender.h"

//---------------------------------------------------------------------------------------------------------------------

auto OCoverPageResolver::Resolve(const OCoverPageTemplate::Section &section, const PatternData &data,
                                 const QFont &baseFont) -> QVector<OCartoucheRenderLine>
{
    // An information no catalogue declares is one that cannot be shown, let alone drawn: it was
    // written by a build that knew it, or by hand.
    if (!OCoverPageCatalogue::Section(section.typeId).IsValid())
    {
        return {};
    }

    const QFont font = OCartouchePainter::SectionFont(section.options, baseFont);
    const int lineSpacing = section.options.value(QStringLiteral("lineSpacing"), 0).toInt();

    if (section.typeId == QLatin1String("patternName"))
    {
        // Nothing to draw while the editor is drawing the template — no pattern is loaded, so the
        // name the pattern would give is not there to be written.
        return data.name.isEmpty() ? QVector<OCartoucheRenderLine>()
                                   : OCartouchePainter::TextLines(data.name, font, lineSpacing);
    }

    if (section.typeId == QLatin1String("logo"))
    {
        const QString stored = section.options.value(QStringLiteral("logo")).toString();
        const double sizeCm = section.options.value(QStringLiteral("logoSize"), 2.0).toDouble();

        const OLogoRender::Drawing drawing = OLogoRender::Draw(stored, sizeCm);
        if (drawing.pixmap.isNull())
        {
            // Nothing chosen yet, or a file that has gone since. Both come to the same page: one
            // with no logo on it.
            return {};
        }

        // The path travels with the line: whoever draws the page at print resolution rasterises
        // the logo itself rather than scaling what was drawn for a screen.
        return {OCartouchePainter::LogoLine(drawing.pixmap, drawing.heightPx, lineSpacing,
                                            OLogoLibrary::Directory() + QLatin1Char('/') + stored)};
    }

    if (section.typeId == QLatin1String("freeText"))
    {
        // Two sources, and the template says which: the words written in it, or the ones the
        // pattern's output carries. Nothing is drawn while the second has not been filled in.
        const bool writtenHere = section.options.value(QStringLiteral("textSource")).toString()
                                 == OTemplateTextSource::kHere;
        const QString text = writtenHere ? section.options.value(QStringLiteral("text")).toString()
                                         : data.freeText;
        if (text.isEmpty())
        {
            return {};
        }

        return OCartouchePainter::TextLines(text, font, lineSpacing);
    }

    // An information the catalogue declares and nothing here draws — the ones reading a size table,
    // a measurement or a piece, none of which a cover page has yet.
    return {};
}
