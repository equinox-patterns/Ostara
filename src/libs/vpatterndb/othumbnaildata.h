/************************************************************************
 **
 **  @file   othumbnaildata.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  Per-piece thumbnail state: activation and geometry of the
 **          piece miniature block. The thumbnail is an independent entity,
 **          outside the cartouche: its own element in the file, its own
 **          activation flag, its own block on the canvas.
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

#ifndef OTHUMBNAILDATA_H
#define OTHUMBNAILDATA_H

#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

#include <functional>

#include "onormalizedanchor.h"

class VPiece;
class VAbstractPattern;
class VContainer;

/**
 * @brief Geometry of the thumbnail block, in piece-local scene pixels.
 *
 * width/height of 0 mean auto-size: the drawing is sized from the reference
 * bounding box (30 % of it, clamped between 5 cm and 15 cm). A null position
 * means auto-placement (centered on the piece).
 */
struct OThumbnailGeom
{
    QPointF pos{};   // Piece-local position (scene px).
    qreal width{0.0};  // Width in px; 0 = auto-size.
    qreal height{0.0}; // Height in px; 0 = auto-size.
    ONormalizedAnchor anchor{}; // Normalized placement, keeps the block following the active size.
};

/**
 * @brief One printed seam-allowance value of the miniature.
 *
 * The miniature says what margins the piece is cut with, and the same width may be printed
 * several times — three independent runs of 1 cm are three labels, not one label reading "3".
 *
 * @note The count the panel shows is **not** stored. It is the number of entries carrying that
 * width. A stored count beside a stored list of placements is two places counting the same thing,
 * and that is how a panel and a drawing come to disagree.
 *
 * @note **A word is placed on the drawing, never in the block.**
 *
 * `offset` is a point of the traced piece — the very coordinates @ref ThumbnailContent() returns —
 * and not a distance to the block's corner. The block is resized, and in Graded a size range gives
 * it another frame altogether: a distance to a corner would slide the word off the seam it names
 * as soon as the frame changed, while a point of the drawing follows it, whichever canvas draws it.
 * Only the words the user never moved are laid out against the block (the row above it, the legend
 * centred under it), and those are derived at paint time, never stored.
 */
struct OThumbnailLabel
{
    qreal mm{0.0};      // The seam-allowance width this label prints, in millimetres.
    QPointF offset{};   // Placed position, in the drawing's own coordinates.
    bool placed{false}; // The "moved by hand" mark: a (0,0) offset is a real move, not an absence.
};

/**
 * @brief The words the miniature prints, ready to be drawn.
 *
 * **The miniature owns its geometry; the application owns its words.** Units, decimal separator,
 * translation and the languages a page is written in all live above this library, so a pattern
 * measured in inches and a pattern read in French are the same miniature with different wording —
 * not a second miniature.
 */
struct OThumbnailTexts
{
    QStringList values; // One per stored label, in the same order.
    QString legend;     // Empty when no unit is asked for.
};

/**
 * @brief Per-piece thumbnail state.
 *
 * A piece is either "unset" (the file has no <thumbnail> element: the user
 * never configured a miniature) or "set" (the piece carries explicit data).
 * Activation is a plain flag: the miniature may be configured while hidden.
 *
 * The unit flags and the label list are the *content* of the miniature, as the geometry is its
 * frame: what the drawing says, and where each word of it sits. The words themselves are not
 * stored — they are formatted from @ref OThumbnailLabel::mm and these two flags, so switching to
 * inches reformats every label instead of rewriting it.
 */
class OThumbnailData
{
public:
    /**
     * @brief A miniature's content, born with the units the application prefers.
     *
     * ⚠️ **The unit flags start from the preference, not from a constant of this class.** The
     * Thumbnail page of the block editor decides what a miniature is created with, and a piece drawn
     * after that decision must obey it without being saved and reopened first — so the default is
     * read here, where a miniature is born, and the two flags then belong to the piece: once written,
     * a miniature keeps the unit it was given, whatever the preference becomes.
     */
    OThumbnailData();

    auto IsSet() const -> bool;
    void SetSet(bool set);

    auto IsActive() const -> bool;
    void SetActive(bool active);

    /** @return Stored geometry, or a default (auto-size, auto-placement) when none stored. */
    auto Geom() const -> OThumbnailGeom;
    /** @return Whether the block has explicit stored geometry (moved or resized). */
    auto HasGeometry() const -> bool;
    void SetGeom(const OThumbnailGeom &geom);

    /** @return Whether the values are printed in centimetres. On by default. */
    auto ShowCentimeters() const -> bool;
    void SetShowCentimeters(bool show);

    /** @return Whether the values are printed in inches — beside the centimetres when both are on. */
    auto ShowInches() const -> bool;
    void SetShowInches(bool show);

    /** @return One entry per printed value, in the order the user added them. */
    auto Labels() const -> const QVector<OThumbnailLabel> &;
    void SetLabels(const QVector<OThumbnailLabel> &labels);

    /** @return Where the legend sits, in the drawing's own coordinates — the labels' frame, and
     *  for the same reason: a caption belongs to the drawing it captions, not to the block's corner. */
    auto LegendOffset() const -> QPointF;
    /** @return Whether the legend was moved by hand (same mark, and same reason, as the labels). */
    auto HasLegendOffset() const -> bool;
    void SetLegendOffset(const QPointF &offset);

    void Clear();

private:
    bool m_set{false};
    bool m_active{false};
    OThumbnailGeom m_geom{};
    bool m_hasGeom{false}; // A (0,0) position is a real move, not an "unset" marker.

    bool m_showCentimeters{true}; // The unit a pattern's margins are read in by default.
    bool m_showInches{false};
    QVector<OThumbnailLabel> m_labels{};
    QPointF m_legendOffset{};
    bool m_hasLegendOffset{false};
};

/**
 * @brief Builds the miniature's wording — registered once at application startup.
 *
 * The data to print is handed over rather than read off the piece: a miniature is not always the
 * piece's own. A group carries one per member, and it is that one — its units, its counts — that the
 * block shows.
 */
using OThumbnailTextResolver = std::function<OThumbnailTexts(const VPiece &piece, const OThumbnailData &thumbnail,
                                                             VAbstractPattern *doc)>;

#endif // OTHUMBNAILDATA_H
