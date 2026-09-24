/************************************************************************
 **
 **  @file   othumbnailcache.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  Drawings of the piece miniatures at the reference size (see
 **          othumbnailcache.h).
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

#include "othumbnailcache.h"

#include <QHash>

namespace
{
// One drawing per piece id. Written while the document stands on the reference size, read back
// after it returned to the active size — the parse in between destroyed every tool, so this cannot
// live on them.
QHash<quint32, QPainterPath> g_thumbnails;

bool g_frozenReference = false;
}  // namespace

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailCache::Store(quint32 pieceId, const QPainterPath &content)
{
    if (content.isEmpty())
    {
        g_thumbnails.remove(pieceId);
        return;
    }
    g_thumbnails.insert(pieceId, content);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailCache::Clear()
{
    g_thumbnails.clear();
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailCache::Content(quint32 pieceId) -> QPainterPath
{
    return g_thumbnails.value(pieceId);
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailCache::Has(quint32 pieceId) -> bool
{
    return g_thumbnails.contains(pieceId);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailCache::SetFrozenReference(bool frozen)
{
    g_frozenReference = frozen;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailCache::IsFrozenReference() -> bool
{
    return g_frozenReference;
}
