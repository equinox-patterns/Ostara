/************************************************************************
 **
 **  @file   vpattern_garbagecollector.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Garbage collection methods extracted from vpattern.cpp
 **
 **  @note   Extracted from vpattern.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
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

#include "vpattern.h"

#include "../ifc/xml/vpatterngraph.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/def.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/tools/vtooluniondetails.h"
#include "vpattern_helpers.h"

#include <QFileInfo>

using namespace Qt::Literals::StringLiterals;
using namespace VPatternHelpers;

//---------------------------------------------------------------------------------------------------------------------
void VPattern::CollectGarbage()
{
    if (!m_garbageCollected)
    {
        GarbageCollector();
        m_garbageCollected = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// MODELING_TOOL (union tools) cannot be assessed via BFS from the tool itself because
// their children keep the BFS path alive even when the resulting piece is gone.
// They are handled separately in GCStaleUnionTools.
auto VPattern::GCStaleModelingObjects(const VPatternGraph *graph) -> QSet<vidtype>
{
    QSet<vidtype> candidates = ConvertToSet(graph->GetVerticesByType(VNodeType::MODELING_OBJECT));
    EraseIf(candidates,
            [graph, this](vidtype node) -> bool
            {
                auto const PieceFilter = [](const VNode &n) -> bool { return n.type == VNodeType::PIECE; };
                return !graph->GetDependentNodes(node, PieceFilter).isEmpty() || FindElementById(node).isNull();
            });
    return candidates;
}

//---------------------------------------------------------------------------------------------------------------------
// A union tool is stale when ALL its XML-recorded children have no reachable PIECE descendants.
auto VPattern::GCStaleUnionTools(const VPatternGraph *graph) -> QSet<vidtype>
{
    auto const PieceFilter = [](const VNode &n) -> bool { return n.type == VNodeType::PIECE; };

    QSet<vidtype> candidates;
    for (vidtype const toolId : graph->GetVerticesByType(VNodeType::MODELING_TOOL))
    {
        const QDomElement toolElement = FindElementById(toolId);
        if (toolElement.isNull() || toolElement.attribute(AttrType) != VToolUnionDetails::ToolType)
        {
            continue;
        }

        const QVector<vidtype> childIds = CollectUnionChildIds(toolElement);
        if (childIds.isEmpty())
        {
            continue;
        }

        if (!std::ranges::all_of(childIds,
                                  [graph, &PieceFilter](vidtype childId) -> bool
                                  { return graph->GetDependentNodes(childId, PieceFilter).isEmpty(); }))
        {
            continue;
        }

        candidates.insert(toolId);
    }
    return candidates;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::GCRemoveFromModeling(const QSet<vidtype> &candidates) -> QSet<vidtype>
{
    QSet<vidtype> removed;
    removed.reserve(candidates.size());

    for (vidtype const node : candidates)
    {
        const QDomElement domElement = FindElementById(node);
        if (domElement.isNull())
        {
            continue;
        }

        if (QDomElement parent = domElement.parentNode().toElement();
            !parent.isNull() && parent.tagName() == TagModeling)
        {
            parent.removeChild(domElement);
            removed.insert(node);
        }
    }
    return removed;
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::GCUpdateHistory(const QSet<vidtype> &removed)
{
    QVector<VToolRecord> newHistory;
    newHistory.reserve(history.size());
    for (const auto &record : std::as_const(history))
    {
        if (!removed.contains(record.GetId()))
        {
            newHistory.append(record);
        }
    }
    history = newHistory;
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::GarbageCollector()
{
    VPatternGraph const *graph = PatternGraph();

    QSet<vidtype> candidates = GCStaleModelingObjects(graph);
    candidates += GCStaleUnionTools(graph);

    if (candidates.isEmpty())
    {
        return;
    }

    BackupBeforeGarbageCollector();

    const QSet<vidtype> removed = GCRemoveFromModeling(candidates);
    if (removed.isEmpty())
    {
        return;
    }

    GCUpdateHistory(removed);
    RefreshElementIdCache();
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::BackupBeforeGarbageCollector() const
{
    if (!m_garbageCollectBackupFilePath.isEmpty())
    {
        QString error;
        QFileInfo const info(m_garbageCollectBackupFilePath);
#if defined(Q_OS_UNIX) || defined(Q_OS_MACOS)
        const QString hidden = QChar('.');
#else
        const QString hidden;
#endif
        const QString backupFileName = u"%1/%2%3.gb.bak"_s.arg(info.absoluteDir().absolutePath(),
                                                               hidden,
                                                               info.fileName());
        if (VDomDocument::SafeCopy(m_garbageCollectBackupFilePath, backupFileName, error))
        {
#if defined(Q_OS_WIN)
            SetFileAttributesW(backupFileName.toStdWString().c_str(), FILE_ATTRIBUTE_HIDDEN);
#endif
        }
    }
}
