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
#include <QtGlobal>

#include "onormalizedanchor.h"

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
 * @brief Per-piece thumbnail state.
 *
 * A piece is either "unset" (the file has no <thumbnail> element: the user
 * never configured a miniature) or "set" (the piece carries explicit data).
 * Activation is a plain flag: the miniature may be configured while hidden.
 */
class OThumbnailData
{
public:
    OThumbnailData() = default;

    auto IsSet() const -> bool;
    void SetSet(bool set);

    auto IsActive() const -> bool;
    void SetActive(bool active);

    /** @return Stored geometry, or a default (auto-size, auto-placement) when none stored. */
    auto Geom() const -> OThumbnailGeom;
    /** @return Whether the block has explicit stored geometry (moved or resized). */
    auto HasGeometry() const -> bool;
    void SetGeom(const OThumbnailGeom &geom);

    void Clear();

private:
    bool m_set{false};
    bool m_active{false};
    OThumbnailGeom m_geom{};
    bool m_hasGeom{false}; // A (0,0) position is a real move, not an "unset" marker.
};

#endif // OTHUMBNAILDATA_H
