/************************************************************************
 **
 **  @file   oexportpdf.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Export mode — write a generation as a PDF.
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

#ifndef OEXPORTPDF_H
#define OEXPORTPDF_H

#include <QString>

class VAbstractPattern;
class VContainer;
struct OExportOutput;
struct OExportTemplate;

/**
 * @brief The PDF of a generation.
 *
 * What it writes is what the canvas shows: the same items, built by the same code, so the
 * paper cannot disagree with the screen. The layers come from that same walk — one per
 * size, plus the reference size's decor — and Trame turns them into optional content
 * groups, which is what lets a printer switch a size off in Acrobat.
 *
 * The page is the bin when the generation has no sheets to cut — a roll, the A0 sheet:
 * the width the print model imposed, the height the nesting used, the very rectangle the
 * canvas draws. Nothing is cropped and nothing is left blank.
 *
 * When the run is tiled, the plan is cut into its artboards and each artboard becomes a
 * page — the same grid the canvas draws, so the paper cannot disagree with the screen.
 * Nothing is asked of the user: the print model already said what she prints on, and its
 * frame height is the whole of what tells the two cases apart.
 *
 * The drawing is written ONCE whatever the page count: it goes into the file as one Form
 * XObject per layer, and every page references it, offset so that its own artboard lands
 * on the paper. The plan is in the PDF a single time rather than once per sheet, and it is
 * rendered a single time as well. What a page holds of its own is that offset — and, in
 * time, the decor that belongs to a sheet rather than to the plan: the numbering, the
 * frame, the registration marks. Leaving that room is half the reason the drawing is
 * shared.
 *
 * A single-size run has nothing to switch between, so it goes out flat — no layers, one
 * stream. There is no layer panel to offer, and an entry with no choice is not one.
 */
// The cover page the generation carries, resolved against the live document by
// OExportCoverPage::Of. Only its name is needed here: it is handed over, never read.
namespace OExportCoverPage
{
struct Drawing;
}

namespace OExportPdf
{
/// @param output  the generation, as it was filed: its resolved layers and its placements.
/// @param pattern the pattern it was nested from — the decor is read from it live, exactly
///                as the canvas reads it, so what is exported is what is on screen now.
/// @param doc     the document, which the cartouche resolver reads the template from.
/// @param marks   the assembly marks template the generation is drawn with, or nullptr when
///                none resolved — the canvas's own convention, and the same value it is
///                given. Null means no underlay at all, which is a state and not an error:
///                a folder the user has emptied leaves every plan without marks.
/// @param framed  whether the format is one of the framed ones. The underlay carries it and
///                the layers do not, because the frame belongs to the sheet rather than to
///                what is drawn on it.
/// @param addLayoutLayer  whether the layout is written as one of the reader's layers, and so
///                can be switched off there. The dialog asks, and it opens on NO.
///
///                False leaves the layout out of the optional content entirely — it is drawn
///                exactly as always, and nothing in the reader's panel reaches it. That is the
///                safer answer and the reason it is the default: a plan whose layout can be
///                switched off is a plan that can be printed without its information block, its
///                folds and its annotations, and it looks perfectly normal that way.
///
///                It governs the assembly marks too, since they are drawn on the paper the
///                layout describes. With no layout layer they have no marker and stand alone,
///                which is what they have always done.
/// @param cover   the cover page the generation carries, or nullptr when it carries none. The
///                canvas's own value, resolved by the same call it is drawn with, so the paper
///                cannot carry a page the screen did not show.
///
///                **It is written outside every layer.** Nothing in the reader's panel switches it
///                off, and no combination of layer states hides it: a plan delivered without its
///                nomenclature looks perfectly normal, and the reader has no way to tell. That is
///                why it is not optional content — the caller decides nothing, and `addLayoutLayer`
///                does not reach it.
///
///                Its SEAT belongs to the format and decides where the page stands: inside the
///                plan's own sheet, or on one of its own — and that one becomes the file's first
///                page, because a cover page opens what it covers.
/// @return false when the run cannot be reproduced; the reason has been logged.
bool Write(const OExportOutput &output, const QString &pdfPath,
           const VContainer *pattern, VAbstractPattern *doc,
           const OExportTemplate *marks, bool framed, bool addLayoutLayer,
           const OExportCoverPage::Drawing *cover = nullptr);
}  // namespace OExportPdf

#endif  // OEXPORTPDF_H
