/************************************************************************
 **
 **  @file   ogradedsvgwriter.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   06/09/2026
 **
 **  @brief  Graded SVG writer: one Illustrator/Inkscape layer per size (grand →
 **          petit in document order, so the smallest ends up on top of the
 **          layers panel) + a final decor layer holding the shared decor
 **          (grainline, fold, info blocks, annotations) of the reference size.
 **          Size layers carry geometry only; the decor layer carries no
 **          contour. Text is ALWAYS emitted as editable <text> (never paths).
 **          The decor layer's <g> is marked ostara:layout="yes" — a backend
 **          marker (namespace "ostara", see ogradedsvgwriter.cpp) that Rabnest
 **          uses to identify the layer. No editor displays it, so the visible
 **          name is free: id = sanitized display name, inkscape:label / <title>
 **          = display name (localized or user-edited). Each decor object wrapper
 **          of the decor layer is marked i:layer="yes" so Rabnest moves it
 **          together with its piece (see WriteGradedSvg).
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

#ifndef OGRADEDSVGWRITER_H
#define OGRADEDSVGWRITER_H

#include "widgets/InfoBlock/oinfoblockexport.h"  // OInfoBlockSvg::Block / Annotation

#include <QList>
#include <QMarginsF>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

class QGraphicsItem;

namespace OGradedSvg
{
/** @brief One piece inside a group wrapper (geometry of a group member). */
struct GradedMember
{
    QString name;
    QGraphicsItem *item = nullptr;
};

/** @brief One top-level object drawn in a size layer (lone piece or group). */
struct GradedObject
{
    QString name;
    bool isGroup = false;
    QGraphicsItem *loneItem = nullptr;         // when !isGroup: the piece silhouette root
    QVector<GradedMember> members;             // when isGroup: the member silhouettes
};

/** @brief One size layer: label + its objects, already in grand → petit order. */
struct SizeLayer
{
    QString label;
    QVector<GradedObject> objects;
};

/** @brief One object of the Layout layer (shared decor of the reference size). */
struct LayoutObject
{
    QString name;
    QPointF origin;                            // scene origin used to place blocks/annotations
    QList<QGraphicsItem *> decorChildren;      // grainline / fold / text items (already in scene coords)
    QVector<OInfoBlockSvg::Block> blocks;
    QVector<OInfoBlockSvg::Annotation> annotations;
};

/**
 * @brief Write the graded multi-layer SVG.
 *
 * @param sizeLayers  layers grand → petit (smallest last, = reference).
 * @param layoutLayerName  display name of the final decor layer (topmost) — used
 *                         for its id, inkscape:label and <title>.
 * @param layoutObjects    decor per top-level object, taken from the reference size.
 * @param imageRect   scene content rect (all sizes, after alignment/offsets).
 */
bool WriteGradedSvg(const QString &fileName, const QList<SizeLayer> &sizeLayers,
                    const QString &layoutLayerName, const QList<LayoutObject> &layoutObjects,
                    const QRectF &imageRect, const QMarginsF &margins, qreal xScale, qreal yScale,
                    const QString &title, const QString &description);
}  // namespace OGradedSvg

#endif  // OGRADEDSVGWRITER_H
