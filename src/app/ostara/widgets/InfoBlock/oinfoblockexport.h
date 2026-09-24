/************************************************************************
 **
 **  @file   oinfoblockexport.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   03 September, 2026
 **
 **  @brief  Manual flat-SVG writer for the Details export (mono size) with
 **          faithful information blocks (see svg spec §6-§7).
 **
 **          QSvgGenerator cannot inline the logo markup nor fully control the
 **          block text, so the Details SVG is written by hand, mirroring the
 **          Val Fork graded writer (paths flat, text editable by default,
 **          logo SVG inlined). The block content is laid out by the shared
 **          OCartouchePainter::Layout so the SVG matches the canvas exactly.
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

#ifndef OINFOBLOCKEXPORT_H
#define OINFOBLOCKEXPORT_H

#include <QHash>
#include <QList>
#include <QMarginsF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVector>

#include "vwidgets/ocartouchepainter.h"

class QGraphicsItem;

namespace OInfoBlockSvg
{
/**
 * @brief One block to draw on a piece, in piece-local pixels.
 *
 * rect is the final box (top-left = block position, size = natural ×
 * fontScale), rotation around the box top-left (same sign convention as the
 * canvas item: content rotated by -rotation), lines the rendered content.
 */
struct Block
{
    /// The owner's own block number. It orders the blocks on screen — the canvas item takes it as
    /// its z-value — and it is the only thing the writers do NOT need: they walk the vector.
    int index = 0;

    QVector<OCartoucheRenderLine> lines;
    qreal fontScale = 1.0;
    QRectF rect;
    qreal rotation = 0.0;
};

/**
 * @brief One floating annotation to draw on a piece, in piece-local pixels.
 *
 * rect is the final box (top-left = resolved annotation position, size = the
 * canvas box, at least the natural content size × fontScale), rotation around
 * the box top-left (same sign convention as the canvas item: content rotated
 * by -rotation). The font fields carry the canvas-resolved render style; the
 * writer rebuilds the pixel font and centers the multi-line text on the box.
 */
struct Annotation
{
    /// The owner's own annotation index, for the same reason as @ref Block::index.
    int index = 0;

    QString text;
    QString fontFamily;     // empty = default
    int fontWeight{0};      // Qt weight (CSS 100-900), 0 = default
    int fontSizePx{0};      // 0 = base font pixel
    QString textColor{QLatin1String("#000000")};
    QString backgroundColor;  // empty = transparent
    qreal fontScale{1.0};
    qreal rotation{0.0};
    QRectF rect;            // local box (top-left already resolved)
};

/**
 * @brief Write the flat Details SVG: pieces geometry + information blocks.
 *
 * @param pieces the piece root items, already positioned in their scene
 *        (same items the current QSvgGenerator export would render).
 * @param blocksByPiece one entry per piece, aligned by index.
 * @param annotationsByPiece one entry per piece, aligned by index.
 * @param pieceNames display names used to build <g> ids.
 * @param imageRect scene content rect (paper rect in the flat export).
 */
bool WriteSvg(const QString &fileName, const QList<QGraphicsItem *> &pieces,
              const QList<QVector<Block>> &blocksByPiece,
              const QList<QVector<Annotation>> &annotationsByPiece, const QStringList &pieceNames,
              const QRectF &imageRect, const QMarginsF &margins, qreal xScale, qreal yScale,
              const QString &title, const QString &description, bool textAsPaths,
              bool showGrainline, const QHash<int, QVector<int>> &groupMembers = {});
}  // namespace OInfoBlockSvg

#endif // OINFOBLOCKEXPORT_H
