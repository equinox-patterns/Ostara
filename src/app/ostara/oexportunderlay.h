/************************************************************************
 **
 **  @file   oexportunderlay.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  Export mode — write a template's assembly marks as the document the PDF
 **          draws under the plan, on every one of its pages.
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

#ifndef OEXPORTUNDERLAY_H
#define OEXPORTUNDERLAY_H

#include <QString>

#include "oexportartboards.h"
#include "oexportsheets.h"

struct OExportTemplate;

/**
 * @brief The sheet's own frame, numbering and registration marks, as a document of their own.
 *
 * It is written as a SECOND document rather than as part of the plan's SVG, and that is the
 * whole of its reason. Trame draws it on every page, under the layers and — this is the point —
 * OUTSIDE every optional content group, so that nothing in the reader's layer panel can switch
 * it off. A sheet whose frame and numbering can be turned off is a sheet that can be assembled
 * wrongly, and it is a silent error: the print looks right. The décor of the plan is a layer
 * because it belongs to what is drawn ON the paper; this belongs to the paper.
 *
 * It is drawn ONCE, in the plan's own frame, and each page shows of it exactly what the page
 * shows of the plan — the same crop, by the same translation. Nothing here knows which sheet it
 * is drawing for, and that is not a shortcut: a seam between two sheets must carry ONE line
 * drawn from the plan's anchor, or its dashes restart at every edge and the two halves never
 * meet. Reading a sheet off the crop is what makes the join invisible, and it is why no page
 * identity has to cross into Trame.
 *
 * What it must declare is therefore the plan's own page, to the plan's own figures. Trame
 * compares the two documents before it will lay one on the other and REFUSES the whole underlay
 * if they differ — see the writer, which does not let the SVG generator decide that.
 */
namespace OExportUnderlay
{
/**
 * @param svgPath     where to write the document.
 * @param planSvgPath the plan's own SVG, already written: its page is the one to declare.
 * @param marks       the template — frame, assembly marks and pagination — as it stands now.
 * @param artboards   the tiling, which is also the page of the document.
 * @param sheets      which of those sheets is printed; the marks follow the paper.
 * @param framed      whether the format is one of the framed ones. On any other tiled format
 *                    the frame and the scissors are not drawn, exactly as on screen.
 * @return false when the document could not be written; the reason has been logged.
 */
bool Write(const QString &svgPath, const QString &planSvgPath, const OExportTemplate &marks,
           const OExportArtboards::Grid &artboards, const OExportSheets::Set &sheets, bool framed);
}  // namespace OExportUnderlay

#endif  // OEXPORTUNDERLAY_H
