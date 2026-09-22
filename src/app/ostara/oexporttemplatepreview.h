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
#include "oexportmarkpainter.h"

#include <QSize>
#include <QSizeF>
#include <QWidget>

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
 * The sheet size is the one of the print model being previewed, in millimetres — A3, A4, Letter,
 * Tabloid, or either of the two framed formats. Everything the template draws on a sheet is
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
 * **The decor itself is not drawn here.** It comes from OExportMarkPainter, the same renderer
 * the Export canvas draws its plan with — one renderer, several outputs, as the cartouche
 * already does it. This widget decides only what a preview adds on top: how large the sheets are
 * drawn, and that a frame which the format does not carry is stood in for by a faint outline.
 *
 * See Modifications/Ostara/ExportMode/PDFTemplateEditor.md §3.4, §3.5 and §11.
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
    /** @brief The grid at the scale in force, margins included — empty before the first paint. */
    QSize GridSizePx() const;

    /** A4, the model used until the format row above hands over another one. */
    QSizeF m_sheetSizeMm{210.0, 297.0};

    /**
     * The decor — frame, scissors, marks and pagination — with the two rules that are its own:
     * everything is placed from the plan rather than from the sheet, and every length is
     * absolute. Held by value and kept: building it would re-parse five SVG files per repaint.
     */
    OExportMarkPainter m_decor;

    /** True until the format row says otherwise; « A4/Letter framed » is what it shows first. */
    bool m_sheetFramed{true};

    qreal m_zoom{1.0};

    /**
     * The fit scale, remembered from the last time the whole grid was on screen. It is what a
     * zoom above 1:1 multiplies. Recomputing the fit while zoomed would read it off a widget
     * already grown to the zoomed grid, and the zoom would chase its own tail.
     */
    qreal m_frozenPxPerMm{0.0};
};

#endif // OEXPORTTEMPLATEPREVIEW_H
