/************************************************************************
 **
 **  @file   oexportpaginationsettings.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  The page number a template writes on the sheets of a tiled plan — what it is made
 **          of, and how it is written.
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

#ifndef OEXPORTPAGINATIONSETTINGS_H
#define OEXPORTPAGINATIONSETTINGS_H

#include <QString>
#include <QVector>
#include <QtGlobal>

/**
 * @brief What a page number is made of.
 *
 * Two forms and no third: letters alone are not offered. Letters run along the columns, so they
 * give out at twenty-six of them — a plan that long is the ordinary case, and a form that stops
 * counting halfway is not a form. See Modifications/Ostara/ExportMode/PDFTemplateEditor.md
 * §3.4.3.
 */
enum class OExportPaginationForm : qint8
{
    /** The sheet's rank in reading order — left to right, then top to bottom. */
    Number,

    /**
     * The column's letter and the row's number. The letter is the column and the number is the
     * row, so a sheet is *registered* rather than counted: A1, B1, C1 across, A2, B2, C2 below.
     */
    NumberAndLetter
};

/**
 * @brief The most columns a run may be given — and, for the same reason, the most a page number
 *        can name.
 *
 * Twenty-six, because the two-axis form writes a column as a letter and there is none past 'Z'.
 * The tiling itself would take more: this is a limit of what a sheet can be **called**, not of
 * how many can be laid side by side. Capping it where the count is entered is what keeps a plan
 * from being built whose sheets carry numbers no one can read — a number that names nothing is
 * worse than a count that cannot be asked for.
 */
constexpr int kPaginationMaxColumns = 26;

/** @brief One row of the form table: what the combo shows, and a saved template carries. */
struct OExportPaginationFormModel
{
    const char *label{nullptr}; ///< what the combo shows — translatable
    const char *token{nullptr}; ///< what a saved template carries — never translated
};

/** @brief The forms, in the order the combo offers them — which is also the enum's. */
auto OExportPaginationForms() -> const QVector<OExportPaginationFormModel> &;

/**
 * @brief One weight of the weight combo: what it shows, and the value a template carries.
 *
 * The same nine the Template Information block lists for a text section (§3.4.3): a weight is a
 * weight, and a second list of them would only be a place for the two to drift apart.
 */
struct OExportPaginationWeightModel
{
    const char *label{nullptr};
    int weight{0};
};

/** @brief The weights, heaviest last, without the « Default » entry the combo puts first. */
auto OExportPaginationWeights() -> const QVector<OExportPaginationWeightModel> &;

/**
 * @brief How far a corner number is set back from each of the two edges it hugs, in millimetres.
 *
 * Half a centimetre — Sophie's figure, 17-09-2026, and it is not a taste: a printer cannot put
 * ink on the last few millimetres of a sheet, so a number closer to the edge than this would be
 * the one mark of the decor that a print could lose.
 */
constexpr qreal kPaginationCornerInsetMm = 5.0;

/**
 * @brief How one page number is written.
 *
 * Four settings and no more — the same ones the Template Information block offers for a text
 * section, with the line spacing left out and the opacity put in its place (§3.4.3). No colour:
 * the number is black, and there is nothing to configure. That is not a restriction added here,
 * it is the state of the model — the Information block exposes no colour either, and draws its
 * text with the theme's.
 *
 * **And no « Default » entry, unlike the Information block's.** A section there belongs to a
 * template and inherits its style, so it needs a way of saying « whatever the template uses »; a
 * page number is the whole of what it is, and nothing above it to inherit from. Every setting
 * therefore carries a real value, and the four below are what the number is until another one is
 * chosen — a decision of Sophie's, 17-09-2026.
 */
struct OExportPaginationTextStyle
{
    /** Empty leaves the default face in place. */
    QString fontFamily;

    /** One of OExportPaginationWeights(). Regular until another is chosen. */
    int weight{400};

    /**
     * The size, in pixels. An absolute length, as every size of the decor is: 96 pixels make an
     * inch whatever the sheet, so the number keeps its size from a small artboard to a large one
     * (§3.5).
     */
    int sizePx{16};

    /** Opacity of the number, in percent. */
    int opacity{100};
};

/**
 * @brief The page number shown at the centre of each sheet.
 *
 * A possibility of its own, carrying its own text settings. Ticking its box is what brings them
 * onto the screen, and their being here rather than on the group is what will let a second
 * placement — a corner, an edge — write the number another way.
 */
struct OExportPaginationCentreNumber
{
    bool enabled{false};
    OExportPaginationTextStyle style;
};

/**
 * @brief The page number shown at the corners of the sheets, where sheets meet.
 *
 * **A junction is what earns a corner its number, and the whole of what earns it.** A corner
 * where sheets meet — two along an edge, four inside — carries one; a corner of the plan itself
 * carries none, since a sheet alone in a corner has no neighbour to be registered against. That
 * is the same rule the corner marks follow (§3.4.2), and it is why the four corners of the plan
 * are passed over here as they are there.
 *
 * **All the sheets meeting at a junction print the same number**, and each prints it in its own
 * corner, inside itself. The number is that of the sheet **above and to the left** of the
 * junction — among the sheets present, the first in reading order. So on a three-column plan the
 * top edge reads A1 between the first two sheets and B1 between the next two, the centre junction
 * reads A1 for all four sheets around it, and the second row repeats with A2 and B2. Gluing is
 * then a matching game: two sheets that meet carry the same number at the place where they meet.
 *
 * Drawn per sheet and not once on the junction, unlike the marks: a text laid on the junction
 * would be cut into four quarters by the page edges, and four quarters of a number name nothing.
 * Being inside the sheet, it is never cut at all — which is also why the number is set back from
 * the two edges it hugs, by kPaginationCornerInsetMm, clear of the printer's own margin.
 */
struct OExportPaginationCornerNumber
{
    bool enabled{false};
    OExportPaginationTextStyle style;
};

/**
 * @brief The page number shown where two sheets meet along a seam — at the middle of the edge.
 *
 * The same principle as the corners, read on a seam instead of on a crossing: one number per
 * junction, printed by both sheets that meet there, each inside itself. The number is again that
 * of the sheet **above and to the left** of the junction, so the seam between the first two
 * sheets of a row reads A1, and gluing is a matching game there too.
 *
 * **Two of these, activated independently** — one for the seams running down the plan, one for
 * those running across it — as the marks offer their « Vertical centre » and « Horizontal centre »
 * separately (§3.4.2). A template may well want the numbers on one axis and not the other.
 *
 * **The number sits inside the mark, and follows its size.** It is placed a quarter of the mark's
 * side from the seam — the centre of the half of the mark that belongs to the sheet — so that
 * the two read as one thing: the mark registers the sheets, and the number says which ones. The
 * mark's **size is read, never its visibility**: the number aligns with where the mark would be,
 * so switching the visual mark off moves nothing and switching it on moves nothing either. A
 * template can carry the number alone.
 *
 * **Two rails keep that rule honest, and they are what make the number valid on its own.**
 * A quarter of a small mark would fall inside the printer's margin, and a quarter of a mark
 * larger than the sheet would fall off the page. So the ink is held at least kPaginationCornerInsetMm
 * away from the seam — the same figure, and the same reason, as the corner number — and never
 * further than the middle of the sheet. Between the two the mark decides.
 */
struct OExportPaginationEdgeNumber
{
    bool enabled{false};
    OExportPaginationTextStyle style;
};

/**
 * @brief What the pagination writes, and where.
 *
 * The number depends on nothing but the sheet's own place in the tiling, which is why one
 * template serves every tiled format and nothing here names one.
 */
struct OExportPaginationSettings
{
    OExportPaginationForm form{OExportPaginationForm::Number};
    OExportPaginationCentreNumber centre;
    OExportPaginationCornerNumber corner;

    /** The seams running down the plan, at the middle of a sheet's height — the marks' « Vertical
     *  centre ». */
    OExportPaginationEdgeNumber verticalCentre;

    /** The seams running across the plan, at the middle of a sheet's width — the marks'
     *  « Horizontal centre ». */
    OExportPaginationEdgeNumber horizontalCentre;
};

#endif // OEXPORTPAGINATIONSETTINGS_H
