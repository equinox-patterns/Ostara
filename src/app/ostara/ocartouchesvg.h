/************************************************************************
 **
 **  @file   ocartouchesvg.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/09/2026
 **
 **  @brief  Serializing a placed cartouche item, written once for every
 **          document that carries one.
 **
 **          The items come from OCartouchePainter::Layout — the single source
 **          of the placement formula, shared by the screen painter and the
 **          writers. What those items become as SVG was transcribed twice,
 **          once per writer; it is written here once, and both go through it.
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

#ifndef OCARTOUCHESVG_H
#define OCARTOUCHESVG_H

#include "vwidgets/ocartouchepainter.h"

#include <QVector>

class QXmlStreamWriter;

/// @brief Serializing placed cartouche items into an SVG document.
///
/// **One definition, because several documents carry the same item.** A mark, a word, a trace
/// sample, a shape's outline and an inlined logo are what a block is made of, and every writer
/// that draws a block draws exactly these — the graded sheet's decor, a piece's information
/// block, and the cover page. A second transcription of them is a second answer to the same
/// question, and the two would drift the day one of them learned a new kind.
namespace OCartoucheSvg
{
/// @brief Write @p items as children of the element @p writer currently has open.
///
/// The caller has already written the `<g>` that places them: each item carries its own target
/// rect, pen origin and baseline, in the coordinates of that group.
///
/// @param scale       the block's font scale. **Only the two widths that are lines rather than
///                    content follow it** — a shape's outline and a mark's stroke. A word scales
///                    through its font and a logo through its target rect, both already resolved
///                    by Layout.
/// @param textAsPaths whether the words go out as glyph outlines, or as `<text>` carrying a font.
///                    **The PDF path wants the outlines**: a Form XObject carries no `/Font`, so a
///                    `<text>` that reaches one is a word no reader can resolve. The editable SVG
///                    path wants `<text>`, and is the reason the flag exists at all.
void WriteItems(QXmlStreamWriter &writer, const QVector<OCartoucheRenderItem> &items, qreal scale,
                bool textAsPaths);
}  // namespace OCartoucheSvg

#endif // OCARTOUCHESVG_H
