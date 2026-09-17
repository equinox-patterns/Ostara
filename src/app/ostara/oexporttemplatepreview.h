/************************************************************************
 **
 **  @file   oexporttemplatepreview.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  Preview of a tiled plan — six white artboards on a contrasting background.
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

#ifndef OEXPORTTEMPLATEPREVIEW_H
#define OEXPORTTEMPLATEPREVIEW_H

#include "core/oexportframesettings.h"
#include "core/oexportmarksettings.h"
#include "core/oexportpaginationsettings.h"

#include <QSize>
#include <QSizeF>
#include <QVector>
#include <QWidget>

class QPainter;
class QSvgRenderer;

/**
 * @brief Draws six artboards — three columns, two rows — as white sheets on a contrasting
 * background, with the template's decor on each of them.
 *
 * This is the preview pane of the Export template editor. Six sheets rather than one: the
 * marks are placed per sheet and the pagination runs along the rows and the columns, so a
 * single sheet would show neither. What is previewed is the rendering of the settings.
 *
 * The sheets tile edge to edge, the way the plan glues its artboards.
 *
 * The sheet size is the one of the print model being previewed, in millimetres — A4, A3,
 * Letter, or the free size of « Tiled Framed ». Everything the template draws on a sheet is
 * drawn at the same scale as the sheet itself, which is what makes the preview truthful:
 * a mark keeps its absolute size from one format to the next, so it occupies a smaller share
 * of a big sheet than of a small one, and the preview shows exactly that.
 *
 * **The zoom is the answer to the one thing that scale cannot show.** A whole 190 mm sheet in
 * a pane a few hundred pixels wide puts a one-point line under a quarter of a pixel: at 1:1
 * the decor is *there*, it simply cannot be seen. Above 1:1 the scale is frozen at what the
 * fit was, and the widget grows to the size of the grid it now draws — the containing
 * QScrollArea then scrolls it. Nothing is ever drawn thicker than it is: what changes is how
 * much of the sheet one pixel covers.
 *
 * See Modifications/Ostara/ExportMode/PDFTemplateEditor.md §3.4 and §3.5.
 */
class OExportTemplatePreview : public QWidget
{
    Q_OBJECT

public:
    explicit OExportTemplatePreview(QWidget *parent = nullptr);

    /** @brief Size of one artboard in millimetres, as the print model defines it. */
    void SetSheetSizeMm(const QSizeF &sizeMm);

    /** @brief The decor the template draws on each sheet — the frame and the scissors. */
    void SetFrameSettings(const OExportFrameSettings &frame);

    /** @brief The assembly marks — what they look like, and where they are set. */
    void SetMarkSettings(const OExportMarkSettings &marks);

    /** @brief The page number, and how it is written. */
    void SetPaginationSettings(const OExportPaginationSettings &pagination);

    /**
     * @brief Whether the sheet being previewed is the one the decor belongs to — « Tiled
     * Framed » alone.
     *
     * Every other tiled model has its artboard boundary along the paper's own edge, which is
     * why the frame is ignored for them rather than drawn twice. Their sheets still have to be
     * told apart on screen, so a faint outline stands in for the decor — and only for them: on
     * the framed model a second line would sit under the user's own frame and fight the opacity
     * they set (§3.4.1).
     */
    void SetSheetFramed(bool framed);

    /** @brief 1.0 shows the whole grid at the fit scale; 8.0 shows a corner closely. */
    void SetZoomFactor(qreal factor);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * @brief The frame, drawn as the grid's own lines and not as six independent rectangles.
     *
     * A boundary between two sheets belongs to both, and is drawn once here — which is what
     * makes the sheets fuse. Every vertical line runs from the top of the grid downwards and
     * every horizontal one from its left edge rightwards, so the dash pattern has a single
     * anchor for the whole plan: a sheet's right edge and its neighbour's left edge are the
     * same line, and the two carry the same dashes at the same places. Drawn sheet by sheet,
     * each rectangle would restart its own pattern at its own corner, and the seams would show.
     *
     * That last point is a rule the export must keep too: on paper each sheet draws its four
     * borders separately, so each border has to be drawn from the plan's anchor — not from the
     * sheet's — and always in the same direction.
     */
    void DrawFrame(QPainter &painter, const QPointF &origin, qreal sheetWidth, qreal sheetHeight,
                   qreal pxPerMm) const;

    /** @brief The boundaries of the tiling, each drawn once, with whatever pen is set. */
    void DrawGridLines(QPainter &painter, const QPointF &origin, qreal sheetWidth,
                       qreal sheetHeight) const;

    /**
     * @brief The assembly marks, centred on the corners and the mid-edges of the tiling.
     *
     * Each one is drawn ONCE, where the sheets meet — half of it belongs to the sheet on either
     * side, a quarter to each of four at a corner. Drawing it per sheet would put two marks
     * where one belongs, and they would not agree; the export must therefore place each mark
     * from the plan as well, and let every page print its own part.
     *
     * Corners where two sheets meet are marked as well as those where four do, and there the
     * mark is cut by the plan's edge: the part that falls outside belongs to no sheet, is not
     * drawn, and is not exported either.
     */
    void DrawMarks(QPainter &painter, const QPointF &origin, qreal sheetWidth, qreal sheetHeight,
                   qreal pxPerMm) const;

    /** @brief The two scissors of one sheet, flat against the frame and inside it. */
    void DrawScissors(QPainter &painter, const QRectF &sheet, qreal pxPerMm) const;

    /**
     * @brief The page number, at the centre of every sheet.
     *
     * The one thing of the decor that is drawn **per sheet** rather than once for the plan: a
     * number belongs to the sheet it is printed on, and no two sheets carry the same. That is
     * also why the row and the column are read here and nowhere else — the frame, the scissors
     * and the marks are placed from the plan and would have no use for them.
     *
     * Drawn whatever the format, as the marks are: a sheet of A4 is glued to its neighbours just
     * as a sheet of « Tiled Framed » is, and needs to say which one it is.
     */
    void DrawCentreNumber(QPainter &painter, const QPointF &origin, qreal sheetWidth,
                          qreal sheetHeight, qreal pxPerMm) const;

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
    void DrawCornerNumbers(QPainter &painter, const QPointF &origin, qreal sheetWidth,
                           qreal sheetHeight, qreal pxPerMm) const;

    /**
     * @brief One orientation of the edge-centre numbers — the middle of the seams between sheets.
     *
     * Called once for the seams running down the plan and once for those running across it, each
     * with its own settings and its own box. The two differ in nothing but orientation: which
     * seams are walked, which sheets meet there, and which way « across the seam » points.
     *
     * A vertical seam runs **down** the plan, so it is told apart by its column and the number
     * crosses it **sideways**; a horizontal seam runs **across** the plan, is told apart by its
     * row, and the number crosses it **up or down**. The marks' vocabulary is the same in both
     * cases: « Vertical centre » for the seams down the plan, at the middle of a sheet's height.
     *
     * A number reads the marks' **size** and not their visibility, and is held between two rails
     * — never inside the printer's margin, never off the sheet. The whole rule is written out in
     * OExportPaginationEdgeNumber.
     */
    void DrawEdgeNumbers(QPainter &painter, const QPointF &origin, qreal sheetWidth,
                         qreal sheetHeight, qreal pxPerMm, bool vertical) const;

    /**
     * @brief Sets the painter's font and colour from one placement's text style.
     *
     * Shared by the two placements, which differ in where they draw and in nothing else: a second
     * copy of the absolute-size conversion is a second place for it to drift.
     */
    void ApplyTextStyle(QPainter &painter, const OExportPaginationTextStyle &style,
                        qreal pxPerMm) const;

    /** @brief The grid at the scale in force, margins included — empty before the first paint. */
    QSize GridSizePx() const;

    /** A4, the model used until the format row above hands over another one. */
    QSizeF m_sheetSizeMm{210.0, 297.0};

    OExportFrameSettings m_frame{};
    OExportMarkSettings m_marks{};
    OExportPaginationSettings m_pagination{};

    /**
     * One renderer per row of the shape table, in the table's own order — which is also the
     * order of OExportMarkShape's values, and the order the combo offers. The three must stay
     * in step; the drawing checks the index before reaching for one.
     */
    QVector<QSvgRenderer *> m_shapeRenderers;

    /** True until the format row says otherwise; « Tiled Framed » is what it shows first. */
    bool m_sheetFramed{true};

    /** The two scissors marks, drawn at the size their files declare. */
    QSvgRenderer *m_scissorsVertical{nullptr};
    QSvgRenderer *m_scissorsHorizontal{nullptr};

    qreal m_zoom{1.0};

    /**
     * The fit scale, remembered from the last time the whole grid was on screen. It is what a
     * zoom above 1:1 multiplies. Recomputing the fit while zoomed would read it off a widget
     * already grown to the zoomed grid, and the zoom would chase its own tail.
     */
    qreal m_frozenPxPerMm{0.0};
};

#endif // OEXPORTTEMPLATEPREVIEW_H
