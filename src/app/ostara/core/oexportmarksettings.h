/************************************************************************
 **
 **  @file   oexportmarksettings.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  The assembly marks a template scatters over the sheets of a tiled plan.
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

#ifndef OEXPORTMARKSETTINGS_H
#define OEXPORTMARKSETTINGS_H

#include "../vmisc/def.h"

#include <QVector>
#include <QtGlobal>

/** The geometric shapes a mark can take. The custom SVG is not built yet. */
enum class OExportMarkShape : qint8
{
    Circle,
    Square,
    Octagon
};

/**
 * @brief One row of the mark-shape table: what the combo shows, what a saved template would
 * carry, and which file draws it.
 */
struct OExportMarkShapeModel
{
    const char *label{nullptr}; ///< what the combo shows — translatable
    const char *token{nullptr}; ///< what a saved template carries — never translated
    const char *icon{nullptr};  ///< the file name, inside the `:/export/` resource folder
};

/** @brief The shapes, in the order the combo offers them. */
auto OExportMarkShapes() -> const QVector<OExportMarkShapeModel> &;

/**
 * @brief What the assembly marks look like, and where they go.
 *
 * A mark is meant to be **shared by the sheets it registers**: its centre sits on a corner or on
 * the middle of an edge, so the corner mark is divided into four equal parts and the edge marks
 * into two, one per sheet. Each sheet prints its own part, and the whole reappears once the
 * sheets are glued — which is why a mark is positioned from the plan and never from the sheet.
 *
 * Only « Tiled Framed » has room inside the sheet for them; the other tiled models are ignored,
 * as the frame is (§3.4.1).
 */
struct OExportMarkSettings
{
    OExportMarkShape shape{OExportMarkShape::Circle};

    /** The mark's side, in millimetres — a square box; the shape is drawn inside it. */
    qreal sizeMm{40.0};

    /** The unit the size is shown in. Always millimetres here, whatever this says. */
    Unit sizeUnit{Unit::Cm};

    /** Opacity of the marks, in percent. */
    int opacity{100};

    /** The three placements, chosen independently. Corner only, to begin with. */
    bool onCorner{true};
    bool onVerticalCentre{false};
    bool onHorizontalCentre{false};
};

#endif // OEXPORTMARKSETTINGS_H
