/************************************************************************
 **
 **  @file   othumbnaildata.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  Per-piece thumbnail state (see othumbnaildata.h).
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

#include "othumbnaildata.h"

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::IsSet() const -> bool
{
    return m_set;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetSet(bool set)
{
    m_set = set;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::IsActive() const -> bool
{
    return m_active;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetActive(bool active)
{
    m_active = active;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::Geom() const -> OThumbnailGeom
{
    return m_geom;
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailData::HasGeometry() const -> bool
{
    return m_hasGeom;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::SetGeom(const OThumbnailGeom &geom)
{
    m_geom = geom;
    m_hasGeom = true;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailData::Clear()
{
    m_set = false;
    m_active = false;
    m_geom = OThumbnailGeom();
    m_hasGeom = false;
}
