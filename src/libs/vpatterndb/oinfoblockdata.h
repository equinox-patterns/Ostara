/************************************************************************
 **
 **  @file   oinfoblockdata.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Per-piece information block state: activation, template
 **          override, section dispatch (info -> block 1..11) and block
 **          geometry. Absence of the per-piece element in the file means
 **          the piece inherits the pattern-level seed (IsSet() == false).
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

#ifndef OINFOBLOCKDATA_H
#define OINFOBLOCKDATA_H

#include <QHash>
#include <QList>
#include <QPointF>
#include <QSet>
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "onormalizedanchor.h"

/**
 * @brief Geometry of one information block, in piece-local scene pixels.
 *
 * width/height of 0 mean auto-size to the natural content size; a null
 * position means auto-placement (piece center). A block is serialized
 * only once it has been moved or resized.
 */
struct OInfoBlockGeom
{
    QPointF pos{};        // Piece-local position (scene px).
    qreal width{0.0};     // Width in px; 0 = auto-size.
    qreal height{0.0};    // Height in px; 0 = auto-size.
    qreal rotation{0.0};  // Rotation in degrees.
    qreal fontScale{1.0}; // Content scale factor applied on resize.
    ONormalizedAnchor anchor{}; // Normalized placement, keeps the block following the active size.
};

/**
 * @brief Per-piece information block state.
 *
 * A piece is either "unset" (legacy / never edited -> inherits the
 * pattern-level seed) or "set" (carries explicit per-piece data). The
 * set flag is the presence of the <cartoucheBlocks> element in the file.
 */
class OInfoBlockData
{
public:
    OInfoBlockData() = default;

    auto IsSet() const -> bool;
    void SetSet(bool set);

    auto IsActive() const -> bool;
    void SetActive(bool active);

    /** @return Template override (base name); empty = inherit the pattern template. */
    auto Template() const -> QString;
    void SetTemplate(const QString &name);

    auto Dispatch() const -> const QHash<QString, int> &;
    /** @return Block (1..11) of a section type, 0 when not dispatched. */
    auto BlockOfSection(const QString &type) const -> int;
    void SetSectionBlock(const QString &type, int block);
    void SetDispatch(const QHash<QString, int> &dispatch);
    void ClearDispatch();

    /** @return Stored geometry of a block, or a default (auto) when none stored. */
    auto BlockGeom(int blockIndex) const -> OInfoBlockGeom;
    /** @return Whether a block has explicit stored geometry (moved or resized). */
    auto HasStoredGeometry(int blockIndex) const -> bool;
    void SetBlockGeom(int blockIndex, const OInfoBlockGeom &geom);
    /** @return Block indexes (1..11) that carry stored geometry. */
    auto StoredBlockIndexes() const -> QList<int>;

    void Clear();

private:
    bool m_set{false};
    bool m_active{false};
    QString m_template;
    QHash<QString, int> m_dispatch;
    QVector<OInfoBlockGeom> m_blockGeoms; // index = block number - 1 (only blocks with stored geometry)
    QSet<int> m_storedGeoms;              // block indexes with explicit geometry (a (0,0) position is a real move)
};

#endif // OINFOBLOCKDATA_H
