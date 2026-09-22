/************************************************************************
 **
 **  @file   oexportcoverpage.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   22/09/2026
 **
 **  @brief  Export mode — where the cover page stands on a given format.
 **
 **          One rule, read by everything that needs it: the nesting, which
 **          must keep the reserved block clear, and the canvas, which must
 **          draw the page where the paper will carry it. A second answer
 **          written for the screen would drift from the first.
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

#ifndef OEXPORTCOVERPAGE_H
#define OEXPORTCOVERPAGE_H

#include "core/ocoverpagerender.h"
#include "core/ocoverpagetemplate.h"

#include <QMap>
#include <QRectF>
#include <QVector>

struct OExportPrintModel;
struct OExportOutput;
class VAbstractPattern;

/// @brief Where the cover page of a format stands.
///
/// A property of the FORMAT and of nothing else. The page is 19 × 25.9 cm wherever it goes,
/// and whether that fits beside a plan or needs a sheet of its own is a fact about the sheet
/// the plan is printed on — never about the run, or two runs of one format would put the same
/// page in two places.
namespace OExportCoverPage
{
/// Where a cover page stands on a format.
enum class Seat : qint8
{
    None,        ///< no cover page — nothing reserved, nothing drawn, no page added
    InSheet,     ///< a block reserved at the plan's top-left corner; the page centred in it
    ExtraSheet   ///< a sheet of its own, above row 1, column 1
};

/// @brief The seat @p model gives a cover page.
///
/// A null model answers None: a generation whose format has left the table draws no cover
/// page rather than a guessed one.
auto SeatOf(const OExportPrintModel *model) -> Seat;

/// @brief The block a reserved cover page occupies — **the page's own rectangle**, at the
///        document margin from the plan's top-left corner. Empty unless the seat is InSheet.
///
/// ⚠️ **The page's size and not an A4's**, decided by Sophie on 22-09-2026: *« le rectangle doit
/// faire la taille de la cover page »*. The block was 210 × 297 first — an A4's own rectangle at
/// the absolute corner — and that was wrong: **the page already carries its own margin**, and a
/// block larger than the page invented a second one, giving the nesting away for nothing.
///
/// And the margin is the DOCUMENT's, the one the user set: the block sits at `(margin, margin)`,
/// so the page is off the top and left edges by exactly the figure she chose. Hence the parameter:
/// the rule needs a length the format does not carry.
auto BlockPx(Seat seat, qreal marginPx) -> QRectF;

/// @brief The rectangle the page stands on, in PIXELS.
///
/// InSheet: the block, at the plan's corner, off the edges by the document margin. ExtraSheet: one
/// sheet above row 1, column 1 — built from the grid's own cell size, never from a second reading
/// of the format table, so a run and its canvas cannot disagree about how wide a sheet is.
///
/// ⚠️ **Paper only for ExtraSheet.** For InSheet this is a rectangle on a sheet that already
/// exists, and filling it white would paint over the plan's own paper. The caller asks the
/// seat before it lays anything down.
auto PaperPx(Seat seat, qreal marginPx, const QRectF &planRect, qreal sheetWidthPx,
             qreal sheetHeightPx) -> QRectF;

/// @brief The page itself, at PRINT SIZE — 19 × 25.9 cm in pixels.
///
/// Centred on its paper, which for a reserved block is the same rectangle: the block IS the page
/// there, so the centring is the identity and nothing is trimmed. On an extra sheet the page is
/// centred on a sheet that is a whole format — and on A4/Letter framed, whose sheet is 190 × 259
/// exactly, the two coincide again.
auto PagePx(Seat seat, qreal marginPx, const QRectF &planRect, qreal sheetWidthPx,
            qreal sheetHeightPx) -> QRectF;

/// @brief What a generation's cover page is, ready to draw.
///
/// Carries the seat and the resolved bands — and **nothing the canvas has to think about**. The
/// canvas places it and paints it; it resolves no template, reads no document and loads no
/// library, which is the same rule the assembly marks already follow.
struct Drawing
{
    Seat seat{Seat::None};

    /// The document margin the run was made with, in PIXELS — what places a reserved block off the
    /// plan's edges. Carried here rather than passed beside, because it belongs to the run the page
    /// came from: a block placed with today's margin on a plan nested with another would sit where
    /// the paper does not expect it.
    qreal marginPx{0.0};

    QMap<OCoverPageTemplate::Placement, QVector<OCartoucheRenderLine>> bands{};

    /// True when there is nothing to draw at all — the one test every consumer makes.
    auto IsEmpty() const -> bool { return seat == Seat::None || bands.isEmpty(); }
};

/// @brief The cover page @p output carries, resolved against the live document and **filled from
///        the run**.
///
/// The template is resolved by NAME, afresh on every rebuild, for the marks' own reason: a template
/// is a file the user edits, and a run that had copied it would go on drawing what the file said
/// the day it was nested.
///
/// **Filled from the RUN, and that is the whole point of the third parameter** — the nomenclature
/// spec's §6: the preview can only state the model's values, and a page announcing a size, a range
/// or a frozen axis value the paper does not carry is a page that lies. The pattern supplies what
/// belongs to the document — its name, the free text, the legend, the margins — and those are re-read
/// on every rebuild, which is what makes "editing the cover page needs no re-nesting" structurally
/// true rather than a promise.
///
/// An empty name — "No cover page" — and a name whose file has gone both answer an empty Drawing.
auto Of(const OExportOutput &output, VAbstractPattern *doc) -> Drawing;
} // namespace OExportCoverPage

#endif // OEXPORTCOVERPAGE_H
