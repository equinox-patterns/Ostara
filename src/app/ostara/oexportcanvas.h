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

#include "oexportartboards.h"  // OExportArtboards::Grid
#include "ogradedsvgwriter.h"  // OGradedSvg::LayoutObject

#include <QList>
#include <QPair>
#include <QString>
#include <QVector>

class QGraphicsItem;
class QGraphicsScene;
class VAbstractPattern;
class VContainer;
class OSizerange;
struct OExportNestingResult;
struct OExportSizeLayer;

/// @brief What the canvas drew, in the shape a writer needs.
///
/// Collected by Build itself rather than rebuilt beside it: the decor has one definition
/// — the canvas's — and a second one written for the export would drift from the screen.
/// Everything here points into the scene the canvas filled, so it lives no longer than it.
struct OExportCanvasContent
{
    /// One drawn object: a lone piece, or the members a group was nested as.
    struct Object
    {
        QString name;
        bool isGroup{false};
        QGraphicsItem *loneItem{nullptr};
        /// Member name and silhouette, in the group's own member order.
        QVector<QPair<QString, QGraphicsItem *>> members;
    };

    /// One size layer, in the order the canvas drew them — reference first.
    struct Layer
    {
        /// The size itself: what the role, the styles and the trace all key on.
        QString label;

        /// What the layer is to be called, when the Size Styles give it a name. Empty when
        /// they do not, and the label is then the name.
        QString displayName;

        QVector<Object> objects;
    };

    QVector<Layer> layers;

    /// The decor of the reference layer, one entry per owner that carries any.
    QList<OGradedSvg::LayoutObject> decor;
};

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
    /// @param doc     the document, which the cartouche resolver reads the template from.
    ///                Blocks are skipped when it is nullptr.
    /// @param content when not nullptr, filled with what was drawn — see the note on
    ///                OExportCanvasContent. Drawing is unaffected either way.
    /// @param artboards the sheets a tiled plan is cut into. An invalid grid — the default,
    ///                and what every caller with no use for it leaves here — changes nothing
    ///                at all; a valid one makes the sheets the paper, and the bin the run's
    ///                own extent instead of what is drawn around it. The PDF writer leaves it
    ///                at its default deliberately: a page IS a sheet there, so its edge is not
    ///                drawn on it.
    static void Build(QGraphicsScene *scene,
                      const QVector<OExportSizeLayer> &layers,
                      const OExportNestingResult &result,
                      const OSizerange *range,
                      const VContainer *pattern,
                      VAbstractPattern *doc,
                      OExportCanvasContent *content = nullptr,
                      const OExportArtboards::Grid &artboards = {});
};

#endif // OEXPORTCANVAS_H
