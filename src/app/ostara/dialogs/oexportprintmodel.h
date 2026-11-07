/************************************************************************
 **
 **  @file   oexportprintmodel.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  The print models of the Export mode — the sheet the nesting is
 **          laid out on. A table of data, and nothing else.
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

#ifndef OEXPORTPRINTMODEL_H
#define OEXPORTPRINTMODEL_H

#include <QString>
#include <QVector>

#include "../oexportcoverpage.h"

/// @brief The two shapes a model can take. What the widget shows follows from this,
///        and nothing else does.
enum class OExportPrintShape : qint8
{
    /// One continuous sheet: a fixed width, and a height the pattern decides. Nothing
    /// is assembled, so there is no sheet height to set and no column to multiply.
    Roll,

    /// Sheets of a fixed size, cut out of the plan one beside the other. The sheet's
    /// small side, times the column count, is the width the nesting works on.
    Tiled
};

/**
 * @brief One printable output: the sheet the nesting is laid out on.
 *
 * The models differ by their shape and by whether the plan is cut into a framed sheet, and
 * by nothing else — which is why they are a table rather than eight classes. None of them is
 * typed into: every dimension here is the sheet the user prints on, and a sheet she could
 * edit would be one no printer has. What the table does NOT carry is a behaviour: there is no
 * pointer, no virtual, no callback. A model that needs its own code gets its own shape class,
 * and this file stays a list of values.
 *
 * The dimensions are in millimetres, always. They are the one fixed point of the unit
 * combo: millimetres go in, whatever unit the user reads comes out, and converting the
 * display never touches what is stored.
 */
struct OExportPrintModel
{
    const char *label{nullptr}; ///< what the combo shows — translatable
    const char *token{nullptr}; ///< what the generated name carries — never translated

    OExportPrintShape shape{OExportPrintShape::Tiled};

    qreal widthMm{0.0};  ///< the sheet's small side — the one the columns multiply
    qreal heightMm{0.0}; ///< the sheet's long side; irrelevant to a Roll

    /// Whether the plan is cut into a framed sheet — the two framed formats, and them alone.
    ///
    /// This is what the decor reads: those two get the frame and the scissors, every other
    /// format gets a faint stand-in outline. It is a property of the format and not of its
    /// size — a frame is a decision, and guessing it from the dimensions would mean two
    /// formats of equal size could not differ.
    bool hasFrame{false};

    /// Where this format's cover page stands — see OExportCoverPage.
    ///
    /// A DECLARED field and not a consequence of the two dimensions above, for the same reason
    /// `hasFrame` is declared rather than guessed. The deduction "does the sheet admit
    /// 210 × 297?" answers six formats out of eight and fails on the two that matter: **A4**
    /// admits the block with no room left for a plan at all, and a **roll** has no sheet
    /// height to be tested against. A threshold nobody can state is not a rule.
    OExportCoverPage::Seat coverSeat{OExportCoverPage::Seat::None};

    /// Whether the model offers a column count. False for the A0 sheet, which is not
    /// assembled after printing: its sheets are whole pages stacked, never placed side
    /// by side, so a column count would have nothing to mean.
    bool hasColumns{true};

    /// Whether the combo lets the model be picked.
    bool selectable{true};

    /// Whether the format draws a decor at all — the frame, the scissors, the marks, the numbering.
    ///
    /// **DECLARED, and not deduced from the sheet's height.** The plan is cut into sheets as soon
    /// as the model has a sheet height, and that same height used to mean "draw the decor" too: the
    /// renderer gates both on `artboards.valid`. A format that wants sheets and NO decor breaks that
    /// equivalence, and the A0 sheet is exactly that one — its sheets are whole pages, a folding
    /// machine turns each into four A4s, and there is nothing to assemble, so nothing to number.
    ///
    /// Deduction has no honest form here: an empty marks template does not mean "no decor", because
    /// `OExportTemplateStorage::Resolve` falls back on "Default" for an empty name. So the format
    /// says it.
    ///
    /// False for the roll too, although the roll gets its absence from the zero height anyway: one
    /// flag that means one thing, rather than a zero that means two.
    bool hasDecor{true};

    /// Whether the sheet may be laid either way, the RUN choosing by page count.
    ///
    /// False everywhere but the A0 sheet: every other tiled format is portrait by construction — its
    /// small side is the one the columns multiply, and the plan's width follows. A single A0 page has
    /// no columns to multiply, so both orientations are printable, and neither is better in advance:
    /// measured on two patterns, one gives a page in portrait and two in landscape, the other exactly
    /// the reverse.
    ///
    /// **Nothing is asked of the user**, and nothing is decided here: the nesting nests the plan in
    /// both orientations and keeps the one with the fewest pages (a tie keeps portrait). The flag is
    /// what tells it to. See OExportNestingParams::sheetBothWays.
    bool hasOrientation{false};

    /// Whether the sheets are CLOSED — no piece may straddle two of them.
    ///
    /// False for every format that is ASSEMBLED: a tiled run cuts the plan, and a piece crossing a
    /// cut line is printed on both sheets and glued back together, which is the whole point of
    /// tiling. The A0 sheet is not assembled — a folding machine turns each sheet into four A4s —
    /// so a piece on two sheets could not be printed at all, and the nesting has to keep every
    /// piece whole inside one.
    ///
    /// DECLARED, and not read off `hasColumns`: today the two flags happen to agree, and they
    /// answer two different questions — one is about a field, the other about a hard constraint
    /// handed to the engine.
    bool closedSheets{false};
};

/// @brief Every model, in the order the combo shows them — the standard sheets largest first,
///        and the two framed formats after them, together.
auto OExportPrintModels() -> const QVector<OExportPrintModel> &;

/// @brief The model carrying @p token, or nullptr. Used to replay a generation, which
///        remembers the token and nothing else about how it was printed.
auto FindPrintModel(const QString &token) -> const OExportPrintModel *;

#endif // OEXPORTPRINTMODEL_H
