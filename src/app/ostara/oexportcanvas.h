/************************************************************************
 **
 **  @file   oexportcanvas.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode — draw the bin and the nested content.
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

#ifndef OEXPORTCANVAS_H
#define OEXPORTCANVAS_H

#include <QVector>

class VMainGraphicsScene;
class VContainer;
class OSizerange;
struct OExportNestingResult;
struct OExportSizeLayer;

/// @brief Draws the result of a nesting run into the Export scene.
///
/// The nesting places footprints, not sizes: one footprint covers every size of
/// the range. The canvas therefore draws each size layer with the transform its
/// footprint received — the same thing the nested SVG contains, where every size
/// layer is moved by the transform of the piece it belongs to.
class OExportCanvas
{
public:
    /// @param range   the range being nested, or nullptr when the run is a single size.
    ///                Its per-owner overrides are where the decor of the reference layer
    ///                takes its position from — never the Details-mode placement.
    /// @param pattern the pattern, source of the grainlines and of the groups. Decor is
    ///                skipped when it is nullptr.
    static void Build(VMainGraphicsScene *scene,
                      const QVector<OExportSizeLayer> &layers,
                      const OExportNestingResult &result,
                      const OSizerange *range,
                      const VContainer *pattern);
};

#endif // OEXPORTCANVAS_H
