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

#include <QFont>
#include <QHash>
#include <QList>
#include <QMarginsF>
#include <QPainterPath>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVector>

#include "vwidgets/ocartouchepainter.h"

class QGraphicsItem;
class OThumbnailData;
class VPiece;
struct OThumbnailGeom;

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
 * @brief One word of a miniature, placed: the text and the top-left of its box.
 *
 * The anchor is in the BLOCK's own frame — the rectangle @ref Thumbnail::rect describes but whose
 * origin is its top-left corner, exactly the frame the item paints the words in. The caller has
 * already put a hand-placed word through the fit that maps the drawing into that block; the writer
 * places the word, it never lays it out.
 */
struct ThumbnailWord
{
    QString text;
    QPointF origin;
};

/**
 * @brief One miniature to draw: the piece at its reference size, with the values it is cut with.
 *
 * `content` is the drawing in the piece's OWN coordinates and `rect` is the block in the owner's
 * local frame — the two frames @ref OThumbnailItem keeps apart too, so the writer redoes the same
 * fit the canvas paint does and nothing else. The words sit in the block's frame, one translation
 * away from the owner's.
 *
 * A group carries one miniature per member: the caller emits them under the group, in the group's
 * frame, which is what the canvas holder does as well.
 */
struct Thumbnail
{
    QPainterPath content;
    QRectF contentBounds;
    QRectF rect;
    QVector<ThumbnailWord> words;
    /// The render font of the words. Carried here so the document shows the typography the screen
    /// shows — the miniature editor's font settings will move it, and the writer follows.
    QFont font;
};

/**
 * @brief Build the record of one miniature, from the very sources the canvas reads.
 *
 * The record is assembled here, once, for both SVG writers: the flat Details sheet and the graded
 * sheet's decor layer draw the same miniature, and a second transcription of the placing rules would
 * be a second answer to the same question — the two would drift the day one of them learned a new
 * kind of word.
 *
 * @param geom      the frame the caller has settled on — a range's override when there is one, the
 *                  piece's own stored geometry otherwise.
 * @param pieceBox  the box @p geom is measured against when nobody ever chose one: the piece's own
 *                  rectangle, in whichever frame the block is written in.
 * @param geomChosen whether that frame was ever placed by hand. A frame nobody placed is sized from
 *                  @p pieceBox and centred on it; a placed one is left exactly as it was stored.
 *
 * @return a record whose @ref Thumbnail::content is empty when the canvas draws nothing: the tool is
 *         gone, or the reference drawing was never captured. The caller drops it — there is nothing
 *         to write, and nothing missing either.
 */
auto BuildThumbnail(quint32 pieceId, const VPiece &piece, const OThumbnailData &data,
                    const OThumbnailGeom &geom, const QRectF &pieceBox, bool geomChosen) -> Thumbnail;

/**
 * @brief The miniature's stroke width, in document pixels.
 *
 * The canvas never draws it heavier than the piece it draws, and never more than one pixel: its law
 * is `min(1, ScaleWidth(hairline, zoom) × zoom)`, and a document has no zoom — the export stands at
 * 100 %, where `ScaleWidth()` leaves the hairline alone. The floor is this file's own: every other
 * path that reaches it with no width is given the same 0.1 px the rest of this writer falls back to,
 * and a miniature drawn thinner than a hair would simply not be there. The document's scale is
 * deliberately NOT applied: no other stroke in either writer compensates for it, so a scaled sheet
 * shrinks every line by the same amount instead of thinning only this one.
 */
auto MiniatureStroke() -> qreal;

/**
 * @brief Write the flat Details SVG: pieces geometry + information blocks.
 *
 * @param pieces the piece root items, already positioned in their scene
 *        (same items the current QSvgGenerator export would render).
 * @param blocksByPiece one entry per piece, aligned by index.
 * @param annotationsByPiece one entry per piece, aligned by index.
 * @param thumbnailsByPiece one entry per piece, aligned by index — the miniatures the canvas
 *        shows, a piece's own or the ones a group carries for its members.
 * @param pieceNames display names used to build <g> ids.
 * @param imageRect scene content rect (paper rect in the flat export).
 */
bool WriteSvg(const QString &fileName, const QList<QGraphicsItem *> &pieces,
              const QList<QVector<Block>> &blocksByPiece,
              const QList<QVector<Annotation>> &annotationsByPiece,
              const QList<QVector<Thumbnail>> &thumbnailsByPiece, const QStringList &pieceNames,
              const QRectF &imageRect, const QMarginsF &margins, qreal xScale, qreal yScale,
              const QString &title, const QString &description, bool textAsPaths,
              bool showGrainline, const QHash<int, QVector<int>> &groupMembers = {});
}  // namespace OInfoBlockSvg

#endif // OINFOBLOCKEXPORT_H
