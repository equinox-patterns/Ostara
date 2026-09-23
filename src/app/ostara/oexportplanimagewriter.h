/************************************************************************
 **
 **  @file   oexportplanimagewriter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  Export mode — writing the plan the canvas shows into an image file.
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

#ifndef OEXPORTPLANIMAGEWRITER_H
#define OEXPORTPLANIMAGEWRITER_H

#include "oexportplanimage.h"

#include <QString>

class QGraphicsScene;

/**
 * @brief Writes the plan the Export canvas is showing into an image file.
 *
 * The scene is the CANVAS'S OWN, handed over rather than rebuilt. §13.5 of the mode's record
 * forbids a second place that would decide what is drawn, and a scene built here would have to
 * repeat ShowExportOutput's preamble line by line — the artboards, the cover page, the sheets,
 * the marks — and would drift from it in silence the day one of them changes.
 *
 * ⚠️ Handing over the screen's scene means handing over the ONE thing it carries that an image
 * must not: the correction's holder, which is up whenever a footprint is selected. Hidden items
 * are not painted, so it is out of the picture as long as nothing is selected — which is not a
 * guarantee. The caller takes it out before calling: see OExportCorrection::WithHolderHidden.
 *
 * The plan is PAINTED, not serialised. Half of what the canvas shows — the paper, the bin, the
 * board grid, the assembly marks, the cover page — are items that draw themselves rather than
 * paths the graded writer could walk, so re-playing their paint() is the only way to capture
 * all of it. It is what makes an image of the plan possible at all.
 */
class OExportPlanImageWriter
{
public:
    /// The two syntaxes the mode offers — the dialog's answer as much as this class's input.
    enum class Format : qint8
    {
        Svg, ///< a page in MILLIMETRES, whose drawing is vector
        Png  ///< pixels, whose count the resolution decides
    };

    /// @brief Write @p plan into @p path, in @p format, at @p target.
    ///
    /// @param title written into an SVG's own header, so that the file says what it is; a PNG
    ///        has nowhere to carry it and ignores it.
    ///
    /// @return false when nothing was written. NO diagnostic is emitted: every qWarning of
    ///         Ostara opens a modal box, and the one message the user is owed belongs to the
    ///         caller — the only place that can name the generation.
    static auto Write(const QString &path, QGraphicsScene *scene, const OExportPlanImage::Plan &plan,
                      const OExportPlanImage::Target &target, Format format,
                      const QString &title = QString()) -> bool;
};

#endif // OEXPORTPLANIMAGEWRITER_H
