/************************************************************************
 **
 **  @file   oexportframesettings.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   16/09/2026
 **
 **  @brief  What a template draws on the sheets of a tiled plan — the frame, and the
 **          scissors that say where to cut.
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

#ifndef OEXPORTFRAMESETTINGS_H
#define OEXPORTFRAMESETTINGS_H

#include <QVector>
#include <QtGlobal>

/**
 * @brief The frame drawn on each sheet of a tiled plan, and the two scissors marks.
 *
 * There is no inset to configure: an artboard *is* the print frame, so the line sits on the
 * sheet's own boundary. Only « Tiled Framed » shows it — for A3, A4 and Letter that boundary is
 * the paper's edge, and drawing it there would trace a second line over the first. See
 * Modifications/Ostara/ExportMode/PDFTemplateEditor.md §3.4.1.
 *
 * Lengths are in points, as the size styles are, and converted at render time by
 * `PrintDPI / 72`. Nothing here is a fraction of the sheet: a mark keeps its size from one
 * format to the next, which is the whole contract of the preview (§3.5).
 */
struct OExportFrameSettings
{
    /** Thickness of the frame's line, in points. */
    qreal widthPt{1.0};

    /**
     * Solid or dashed, and it is an explicit choice rather than the size styles' « empty
     * pattern means solid » — the pattern below is remembered while the line is solid, so that
     * trying one and coming back does not lose what was typed.
     */
    bool dashed{false};

    /**
     * The pattern, in points: one dash, one space. Held as a list because that is what a pen
     * pattern is — Qt alternates the values — but only the one pair is offered, and the line
     * shows it typed over as soon as it stops fitting.
     *
     * In points, as the size styles are: a Qt pattern counts in multiples of the pen width, so
     * a length of n pt is converted at render time by `n × (px per pt) / width`.
     */
    QVector<qreal> dashes{10.0, 5.0};

    /** Opacity of the frame's line, in percent. */
    int frameOpacity{100};

    /** Which cuts the scissors hang on. Both are drawn by default. */
    bool scissorsHorizontal{true};
    bool scissorsVertical{true};

    /** Opacity of the two scissors marks, in percent. */
    int scissorsOpacity{100};
};

#endif // OEXPORTFRAMESETTINGS_H
