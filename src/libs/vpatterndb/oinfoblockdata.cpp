/************************************************************************
 **
 **  @file   oinfoblockdata.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   31 August, 2026
 **
 **  @brief  Per-piece information block state (see oinfoblockdata.h).
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

#include "oinfoblockdata.h"

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockData::IsSet() const -> bool
{
    return m_set;
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockData::SetSet(bool set)
{
    m_set = set;
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockData::IsActive() const -> bool
{
    return m_active;
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockData::SetActive(bool active)
{
    m_active = active;
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockData::Template() const -> QString
{
    return m_template;
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockData::SetTemplate(const QString &name)
{
    m_template = name;
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockData::Dispatch() const -> const QHash<QString, int> &
{
    return m_dispatch;
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockData::BlockOfSection(const QString &type) const -> int
{
    return m_dispatch.value(type, 0);
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockData::SetSectionBlock(const QString &type, int block)
{
    m_dispatch.insert(type, block);
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockData::SetDispatch(const QHash<QString, int> &dispatch)
{
    m_dispatch = dispatch;
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockData::ClearDispatch()
{
    m_dispatch.clear();
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockData::BlockGeom(int blockIndex) const -> OInfoBlockGeom
{
    if (!m_storedGeoms.contains(blockIndex) || blockIndex > m_blockGeoms.size())
    {
        return OInfoBlockGeom();
    }
    return m_blockGeoms.at(blockIndex - 1);
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockData::HasStoredGeometry(int blockIndex) const -> bool
{
    return m_storedGeoms.contains(blockIndex);
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockData::SetBlockGeom(int blockIndex, const OInfoBlockGeom &geom)
{
    if (blockIndex < 1 || blockIndex > 11)
    {
        return;
    }
    if (m_blockGeoms.size() < blockIndex)
    {
        m_blockGeoms.resize(blockIndex);
    }
    m_blockGeoms[blockIndex - 1] = geom;
    m_storedGeoms.insert(blockIndex);
}

//---------------------------------------------------------------------------------------------------------------------
auto OInfoBlockData::StoredBlockIndexes() const -> QList<int>
{
    QList<int> indexes;
    indexes.reserve(m_storedGeoms.size());
    for (int index : m_storedGeoms)
    {
        indexes.append(index);
    }
    std::sort(indexes.begin(), indexes.end());
    return indexes;
}

//---------------------------------------------------------------------------------------------------------------------
void OInfoBlockData::Clear()
{
    m_set = false;
    m_active = false;
    m_template.clear();
    m_dispatch.clear();
    m_blockGeoms.clear();
    m_storedGeoms.clear();
}
