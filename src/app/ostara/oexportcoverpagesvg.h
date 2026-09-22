/************************************************************************
 **
 **  @file   oexportcoverpagesvg.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/09/2026
 **
 **  @brief  Export mode — the cover page as an SVG document of its own.
 **
 **          The page is the feature's (19 × 25.9 cm, print size), the bands
 **          are placed by the shared layout, and the items are serialized by
 **          the shared writer: this file adds nothing but the root and the
 **          frame the page stands in.
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

#ifndef OEXPORTCOVERPAGESVG_H
#define OEXPORTCOVERPAGESVG_H

#include "oexportcoverpage.h"

#include <QRectF>
#include <QString>

/// @brief Writing a cover page out as a document of its own.
///
/// **The PDF's path to a cover page**, and the only one: the canvas paints the page, the SVG
/// writers of a piece know nothing of bands, and a page handed to Trame as pixels would be a
/// page nobody could check.
namespace OExportCoverPageSvg
{
/// @brief Write @p drawing as one SVG whose page is @p pageRect.
///
/// One page, one document — and the page is the cover page's own: 19 × 25.9 cm at print size
/// wherever its format puts it, which is what makes it the same page on an A4 as on a rouleau.
///
/// The root's `viewBox` is @p pageRect **in the plan's own coordinates**, and that is the whole of
/// what places the page: the PDF draws this document through the plan's translation, exactly as it
/// draws the assembly marks, so a page laid out in a frame of its own would land nowhere near its
/// sheet. The document's own page is its declared height, and Trame re-places it against the plan's
/// — the two are not the same page and are not meant to be.
///
/// **The words stay words.** They are written as `<text>`, with their font, and it is Trame that
/// turns them into traced glyphs — in a scope of its own, which is the only place a form without a
/// `/Font` can carry them. That is the layout layer's own route, and the cover page takes it for
/// the same reason: the document keeps what it says, and a reader of the SVG reads the page rather
/// than a drawing of it.
///
/// @param pageRect where the page stands, from `OExportCoverPage::PagePx` — the same call, with the
///                 same figures, that the canvas draws the page with.
/// @return false when there is nothing to write, or when the file cannot be written.
auto Write(const QString &fileName, const OExportCoverPage::Drawing &drawing, const QRectF &pageRect)
    -> bool;
}  // namespace OExportCoverPageSvg

#endif // OEXPORTCOVERPAGESVG_H
