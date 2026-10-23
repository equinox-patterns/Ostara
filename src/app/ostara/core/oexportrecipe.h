/************************************************************************
 **
 **  @file   oexportrecipe.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/10/2026
 **
 **  @brief  What a generation was made OF, written beside the drawing for Rabnest to read.
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

#ifndef OEXPORTRECIPE_H
#define OEXPORTRECIPE_H

#include <QDomElement>

class QDomDocument;
class QXmlStreamWriter;

struct OExportOutput;
struct OExportTemplate;

/**
 * @brief The recipe of one export — the choices the generation was made WITH.
 *
 * Rabnest does not read the paper, it rebuilds it. A frame drawn into the PDF is a frame of the
 * format it was exported in: give the client A3 and the drawing is a lie, because the columns,
 * the seams and the page numbers all move. What survives a change of format is not the drawing
 * but the RECIPE — which format, framed or not, how many columns, how much margin, and which
 * template drew the frame, the marks and the numbering, with the settings that template held the
 * day this file was written.
 *
 * So the recipe is a statement of intent, not a copy of the ink: a few dozen bytes that let the
 * same code draw the paper again at another size. The pieces stay the drawing, and only the
 * pieces — they are the pattern, and nothing about them changes with the sheet.
 *
 * **The decor is written in the template's own words.** Its three blocks come straight from
 * `OExportTemplate::ToXml`, so there is one grammar for a template and not two: what a `.oamt`
 * file states is what the recipe states, and neither can drift away from the reader of the other.
 */
namespace OExportRecipe
{
/**
 * @brief The generation as a `<metadata>` element, ready to be written into the export's SVG.
 *
 * It goes into the SVG rather than beside it because the SVG IS the payload: one file, one
 * version marker, and the contract Rabnest was given — a PDF that carries a drawing — does not
 * grow a second medium to be told the same thing twice.
 *
 * @param doc     the document the element is created in; the caller owns it
 * @param output  the generation — its format, in millimetres, as it was filed
 * @param marks   the template that drew the decor, or nullptr when none resolved. Absent is a
 *                fact and not a default: a plan exported without a template carries no frame and
 *                no numbering, and Rabnest must be able to tell that from a plan whose template
 *                happened to draw nothing.
 * @param framed  whether the format is one of the framed ones. It is the format that decides it,
 *                not the template — a template draws a frame only on a format that has one.
 */
auto Of(QDomDocument &doc, const OExportOutput &output, const OExportTemplate *marks, bool framed)
    -> QDomElement;

/**
 * @brief Writes a recipe element — and what it contains — into an open XML stream.
 *
 * QDomNode has no writer overload, and a recipe spliced in as text would be escaped and lost, so
 * the tree is walked here: elements, their attributes, and the text inside them. That is all a
 * recipe is, and all this will ever be asked to write.
 */
void Write(QXmlStreamWriter &writer, const QDomElement &element);
}  // namespace OExportRecipe

#endif  // OEXPORTRECIPE_H
