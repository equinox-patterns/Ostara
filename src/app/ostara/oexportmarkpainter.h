/************************************************************************
 **
 **  @file   oexportmarkpainter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  Draws the assembly marks of a template — frame, scissors, marks and pagination.
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

#ifndef OEXPORTMARKPAINTER_H
#define OEXPORTMARKPAINTER_H

#include "core/oexportframesettings.h"
#include "core/oexportmarksettings.h"
#include "core/oexportpaginationsettings.h"

#include <QPointF>

#include <memory>
#include <vector>

class QPainter;
class QSvgRenderer;

/**
 * @brief The tiling the assembly marks are drawn on, as the drawn plan sees it.
 *
 * One sheet's size in pixels, how many of them side by side and how many stacked, and the scale
 * that puts millimetres into pixels — everything a decor length needs to become a drawn length.
 * The sheets tile edge to edge, so the plan is exactly this many cells and nothing else.
 *
 * This is the geometry the rendering side works in, and it is not `OExportArtboards::Grid`: that
 * one carries the plan's rectangle and its own validity rules, and belongs to the export. They
 * describe the same tiling seen from two sides, and the canvas maps one onto the other — which is
 * the only place the two meet.
 */
struct OExportMarkGrid
{
    int columns{0};
    int rows{0};

    /** One sheet, across and down, as drawn. */
    qreal sheetWidthPx{0.0};
    qreal sheetHeightPx{0.0};

    /** The scale in force. A decor length is absolute: it is never a fraction of the sheet. */
    qreal pxPerMm{0.0};

    auto IsValid() const -> bool
    {
        return columns > 0 && rows > 0 && sheetWidthPx > 0.0 && sheetHeightPx > 0.0 && pxPerMm > 0.0;
    }
};

/**
 * @brief The one renderer of a template's assembly marks.
 *
 * **One renderer, several outputs.** The same decor is drawn in the template editor's preview, on
 * the Export canvas, and — later — written into the exported PDF. Drawing it once here is what
 * keeps the three from drifting apart: a mark that is placed from the plan in one place and from
 * the sheet in another is a mark that glues wrong, and no test would catch it. This is the
 * cartouche's pattern (`OCartouchePainter`), applied to the tiling.
 *
 * Three rules are the whole of what the decor is, and they are not preferences:
 *
 * - **Placed from the plan, not from the sheet.** Every line of the frame, every mark and every
 *   scissors is positioned against the tiling's own origin. A boundary between two sheets is one
 *   line, drawn once; a mark at a junction is one mark, drawn once. Drawing per sheet would put
 *   two lines where one belongs, restart the dash pattern at every edge, and double every mark.
 * - **Absolute sizes.** A length is in millimetres, points or pixels and stays that size on a
 *   small artboard as on a large one; `pxPerMm` is the only conversion.
 * - **The frame and its scissors belong to « Tiled Framed » alone.** Every other tiled model has
 *   its boundary along the paper's own edge, where a frame has nowhere to go. The marks and the
 *   pagination are drawn whatever the format: a sheet of A4 is glued to its neighbours too.
 *
 * The painter owns the SVG renderers of the three mark shapes and of the two scissors, so an
 * instance is neither copyable nor assignable. Construct one and keep it — building it per paint
 * would re-parse five SVG files on every repaint.
 *
 * See Modifications/Ostara/ExportMode/PDFTemplateEditor.md §3.4, §11 and §12.
 */
class OExportMarkPainter
{
public:
    OExportMarkPainter();
    ~OExportMarkPainter();

    OExportMarkPainter(const OExportMarkPainter &other) = delete;
    auto operator=(const OExportMarkPainter &other) -> OExportMarkPainter & = delete;

    /** @brief The frame and the scissors — the decor of « Tiled Framed ». */
    void SetFrameSettings(const OExportFrameSettings &frame);

    /** @brief The assembly marks — what they look like, and where they are set. */
    void SetMarkSettings(const OExportMarkSettings &marks);

    /** @brief The page number, and how it is written. */
    void SetPaginationSettings(const OExportPaginationSettings &pagination);

    /**
     * @brief Draws the whole decor of the template onto @p grid, from @p origin.
     *
     * @param framed     whether the format is the one the frame belongs to — « Tiled Framed »
     *                   alone. On every other tiled format the frame and the scissors are not
     *                   drawn at all.
     * @param sheetGuide whether to stand in for a frame that is not drawn, with a faint outline
     *                   along the tiling's boundaries. The editor's preview says yes: it has
     *                   nothing else to tell its sheets apart, and there the outline is the only
     *                   thing between six white rectangles and one white block.
     *
     *                   The Export canvas says no, and the reason is the mirror image: it draws
     *                   its own cut lines over the pieces, which is where they are readable. It
     *                   keeps them exactly where the frame is absent, and stands them down where
     *                   the frame is drawn — a second line under the user's own would fight the
     *                   opacity they set, which is the defect the preview was cured of.
     */
    void Paint(QPainter &painter, const QPointF &origin, const OExportMarkGrid &grid, bool framed,
               bool sheetGuide) const;

private:
    /**
     * @brief The frame, drawn as the grid's own lines and not as one rectangle per sheet.
     *
     * A boundary between two sheets belongs to both, and is drawn once here — which is what makes
     * the sheets fuse. Every vertical line runs from the top of the grid downwards and every
     * horizontal one from its left edge rightwards, so the dash pattern has a single anchor for
     * the whole plan: a sheet's right edge and its neighbour's left edge are the same line, and
     * the two carry the same dashes at the same places. Drawn sheet by sheet, each rectangle
     * would restart its own pattern at its own corner, and the seams would show in print.
     */
    void DrawFrame(QPainter &painter, const QPointF &origin, const OExportMarkGrid &grid) const;

    /** @brief The boundaries of the tiling, each drawn once, with whatever pen is set. */
    void DrawGridLines(QPainter &painter, const QPointF &origin, const OExportMarkGrid &grid) const;

    /**
     * @brief The assembly marks, centred on the corners and the mid-edges of the tiling.
     *
     * Each one is drawn ONCE, where the sheets meet — half of it belongs to the sheet on either
     * side, a quarter to each of four at a corner. Corners where two sheets meet are marked as
     * well as those where four do, and there the mark is cut by the plan's edge: the part that
     * falls outside belongs to no sheet, and is not drawn.
     */
    void DrawMarks(QPainter &painter, const QPointF &origin, const OExportMarkGrid &grid) const;

    /** @brief The two scissors of one sheet, flat against the frame and inside it. */
    void DrawScissors(QPainter &painter, const QRectF &sheet, const OExportMarkGrid &grid) const;

    /**
     * @brief The page number, at the centre of every sheet.
     *
     * The one thing of the decor that is drawn **per sheet** rather than once for the plan: a
     * number belongs to the sheet it is printed on, and no two sheets carry the same. That is
     * also why the row and the column are read here and nowhere else — the frame, the scissors
     * and the marks are placed from the plan and would have no use for them.
     */
    void DrawCentreNumber(QPainter &painter, const QPointF &origin,
                          const OExportMarkGrid &grid) const;

    /**
     * @brief The page number at the corners of the sheets, where sheets meet.
     *
     * One number per junction, printed by every sheet that meets there, each in its own corner
     * and inside itself. Two sheets that touch therefore carry the same number at the place where
     * they touch, and gluing them is a matching game rather than a reckoning.
     *
     * Where a junction has no number to give — a corner of the plan — nothing is drawn, and the
     * sheet keeps that corner empty. The whole rule is written out in
     * OExportPaginationCornerNumber.
     */
    void DrawCornerNumbers(QPainter &painter, const QPointF &origin,
                           const OExportMarkGrid &grid) const;

    /**
     * @brief One orientation of the edge-centre numbers — the middle of the seams between sheets.
     *
     * Called once for the seams running down the plan and once for those running across it, each
     * with its own settings and its own box. The two differ in nothing but orientation: which
     * seams are walked, which sheets meet there, and which way « across the seam » points.
     *
     * A number reads the marks' **size** and not their visibility, and is held between two rails
     * — never inside the printer's margin, never off the sheet. The whole rule is written out in
     * OExportPaginationEdgeNumber.
     */
    void DrawEdgeNumbers(QPainter &painter, const QPointF &origin, const OExportMarkGrid &grid,
                         bool vertical) const;

    /**
     * @brief Sets the painter's font and colour from one placement's text style.
     *
     * Shared by the placements, which differ in where they draw and in nothing else: a second
     * copy of the absolute-size conversion is a second place for it to drift.
     */
    void ApplyTextStyle(QPainter &painter, const OExportPaginationTextStyle &style,
                        const OExportMarkGrid &grid) const;

    OExportFrameSettings m_frame{};
    OExportMarkSettings m_marks{};
    OExportPaginationSettings m_pagination{};

    /**
     * One renderer per row of the shape table, in the table's own order — which is also the
     * order of OExportMarkShape's values, and the order the combo offers. The three must stay
     * in step; the drawing checks the index before reaching for one.
     */
    std::vector<std::unique_ptr<QSvgRenderer>> m_shapeRenderers;

    /** The two scissors marks, drawn at the size their files declare. */
    std::unique_ptr<QSvgRenderer> m_scissorsVertical;
    std::unique_ptr<QSvgRenderer> m_scissorsHorizontal;
};

#endif // OEXPORTMARKPAINTER_H
