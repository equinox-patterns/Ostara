/************************************************************************
 **
 **  @file   othumbnailcache.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  Drawings of the piece miniatures as they are at the reference
 **          size of the pattern.
 **
 **          A piece's geometry is memoized at parse time, so the only way to
 **          obtain the drawing of another size is to put the whole document on
 **          that size, let it parse, and read the result. The drawings are kept
 **          here rather than on the tools because that parse destroys and
 **          recreates every tool: whatever the tools hold would not survive the
 **          trip back to the active size.
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

#ifndef OTHUMBNAILCACHE_H
#define OTHUMBNAILCACHE_H

#include <QPainterPath>
#include <QtGlobal>

/**
 * @brief Drawings of the piece miniatures at the reference size.
 *
 * Empty whenever the active size already IS the reference size: the live
 * geometry is then the right one and nothing has to be remembered.
 */
namespace OThumbnailCache
{
/** @brief Remember one piece's drawing, in that piece's own coordinates. */
void Store(quint32 pieceId, const QPainterPath &content);

/** @brief Forget every stored drawing (document switched, or the geometry to show changed). */
void Clear();

/** @brief The stored drawing of a piece, or an empty path. */
auto Content(quint32 pieceId) -> QPainterPath;

/** @brief Whether a piece has a stored drawing. */
auto Has(quint32 pieceId) -> bool;

/**
 * @brief Tell the blocks that the drawing to show is NOT the live geometry.
 *
 * True as soon as the pattern has axes and the active size is not the reference size. While it is
 * true and a piece's drawing has not been captured yet, the block must show nothing: painting the
 * live geometry would show the active size for a moment and then jump to the reference size, right
 * under the user's eyes.
 */
void SetFrozenReference(bool frozen);
auto IsFrozenReference() -> bool;
}  // namespace OThumbnailCache

#endif // OTHUMBNAILCACHE_H
