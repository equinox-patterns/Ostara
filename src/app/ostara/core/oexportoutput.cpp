/************************************************************************
 **
 **  @file   oexportoutput.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/09/2026
 **
 **  @brief  Implementation of the Export-mode generation store.
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

#include "oexportoutput.h"

#include <utility>

//---------------------------------------------------------------------------------------------------------------------
void OExportOutputStore::Clear()
{
    m_outputs.clear();
    m_activeId = 0;
    m_stale = false;
    // m_nextId is deliberately NOT reset: an id is never handed out twice, so a row
    // that outlived its generation can never point at a different one.
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportOutputStore::IsEmpty() const -> bool
{
    return m_outputs.isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportOutputStore::Append(OExportOutput output) -> quint32
{
    output.id = m_nextId++;
    m_activeId = output.id;
    m_outputs.append(std::move(output));
    return m_activeId;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportOutputStore::Outputs() const -> const QVector<OExportOutput> &
{
    return m_outputs;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportOutputStore::Find(quint32 id) const -> const OExportOutput *
{
    for (const OExportOutput &output : m_outputs)
    {
        if (output.id == id)
        {
            return &output;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportOutputStore::ActiveId() const -> quint32
{
    return m_activeId;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportOutputStore::SetActiveId(quint32 id)
{
    m_activeId = id;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportOutputStore::IsStale() const -> bool
{
    return m_stale;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportOutputStore::SetStale(bool stale)
{
    m_stale = stale;
}
