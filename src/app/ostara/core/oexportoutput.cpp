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
auto RequestOf(const OExportOutput &output) -> OExportRunRequest
{
    OExportRunRequest request;
    request.documentWidthMm = output.documentWidthMm;
    request.unit = output.unit;
    request.rangeName = output.rangeName;
    request.singleSize = output.singleSize;
    request.baseA = output.baseA;
    request.baseB = output.baseB;
    request.baseC = output.baseC;
    request.marginMm = output.marginMm;
    request.gapMm = output.gapMm;
    request.printModelToken = output.printModelToken;
    request.columns = output.columns;
    request.frameHeightMm = output.frameHeightMm;
    request.marksTemplateName = output.marksTemplateName;
    request.coverPageTemplateName = output.coverPageTemplateName;
    request.pinnedAxes = output.pinnedAxes;
    request.groupMemberChoices = output.groupMemberChoices;
    return request;
}

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
void OExportOutputStore::SetName(quint32 id, const QString &name)
{
    for (OExportOutput &output : m_outputs)
    {
        if (output.id == id)
        {
            output.name = name;
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportOutputStore::SetPlacement(quint32 id, int index, const OExportPlacement &placement)
{
    for (OExportOutput &output : m_outputs)
    {
        if (output.id != id)
        {
            continue;
        }

        // A gesture is aimed at a footprint of the generation on screen. An undo command can
        // outlive both — the user refreshed the generation, or closed the pattern — and must
        // then do nothing at all rather than write where the index no longer points.
        if (index < 0 || index >= output.result.placements.size())
        {
            return;
        }

        output.result.placements[index] = placement;
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportOutputStore::Replace(quint32 id, OExportOutput output)
{
    for (OExportOutput &existing : m_outputs)
    {
        if (existing.id != id)
        {
            continue;
        }

        // The identity and the name belong to the row, not to the run: the refresh must
        // not rename the generation, and the dock addresses it by the id it was given.
        output.id = existing.id;
        output.name = existing.name;
        existing = std::move(output);
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportOutputStore::Remove(quint32 id)
{
    for (auto it = m_outputs.begin(); it != m_outputs.end(); ++it)
    {
        if (it->id != id)
        {
            continue;
        }

        m_outputs.erase(it);

        if (m_activeId == id)
        {
            m_activeId = 0;
        }
        return;
    }
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
