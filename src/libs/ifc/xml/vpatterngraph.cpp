/************************************************************************
 **
 **  @file   vpatterngraph.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   18 12, 2025
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2025 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 30/09/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "vpatterngraph.h"
#include "../graaflib/properties/vertex_properties.h"
#include "../vmisc/defglobal.h"

#include <QDebug>
#include <QQueue>
#include <QSet>

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Add a vertex to the graph
 * @param id Unique node ID
 * @param type Node type (TOOL or OBJECT)
 * @param index of node's pattern block name
 * @return true if vertex was added, false if ID already exists
 */
auto VPatternGraph::AddVertex(vidtype id, VNodeType type, int index) -> bool
{
    return AddVertex({.id = id, .type = type, .indexPatternBlock = index});
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Add a vertex to the graph
 * @param node Unique node
 * @return true if vertex was added, false if ID already exists
 */
auto VPatternGraph::AddVertex(const VNode &node) -> bool
{
    QWriteLocker const locker(&m_lock);

    if (!m_graphRebuildEnabled)
    {
        return false;
    }

    // Check if ID already exists
    if (m_idToVertex.contains(node.id))
    {
        qWarning() << "VPatternGraph::AddVertex: vertex already exists. id=" << node.id;
        return false;
    }

    vertex_id_t const vertexId = m_graph.add_vertex(node);
    m_idToVertex.insert(node.id, vertexId);
    m_vertexToId.insert(vertexId, node.id);

    // Edges recorded before this vertex existed can be completed now (see AddEdge).
    FlushPendingEdgesLocked(node.id);

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void VPatternGraph::FlushPendingEdgesLocked(vidtype id)
{
    for (auto i = m_pendingEdges.begin(); i != m_pendingEdges.end();)
    {
        if (i->fromId != id && i->toId != id)
        {
            ++i;
            continue;
        }

        // The other end can itself still be missing — it is what the edge was waiting for first. The edge
        // keeps waiting for it: this vertex will not be announced a second time.
        if (!m_idToVertex.contains(i->fromId) || !m_idToVertex.contains(i->toId))
        {
            ++i;
            continue;
        }

        const vidtype fromId = i->fromId;
        const vidtype toId = i->toId;
        i = m_pendingEdges.erase(i);
        AddEdgeLocked(fromId, toId);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Add an edge between two vertices by their IDs
 * @param fromId Source node ID
 * @param toId Target node ID
 * @return true if edge was added successfully
 */
auto VPatternGraph::AddEdge(vidtype fromId, vidtype toId) -> bool
{
    QWriteLocker const locker(&m_lock);

    if (!m_graphRebuildEnabled)
    {
        return false;
    }

    if (!m_idToVertex.contains(fromId) || !m_idToVertex.contains(toId))
    {
        // The objects are read in file order, and a tool can depend on one the file defines further down —
        // another pattern block, in particular. The edge is not lost: it waits for the missing vertex, which
        // AddVertex adds and flushes as soon as the object is read. Dropping it would leave the dependents of
        // that object unknown — nothing would refresh them when it moves, the delete checks would call a used
        // object removable, the Dependencies dock would not show them and the garbage collector would delete
        // a used object. What is genuinely missing is reported by Clear, once the build is over: at this
        // point the object may still be on its way.
        m_pendingEdges.append({fromId, toId, m_build});
        return false;
    }

    return AddEdgeLocked(fromId, toId);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPatternGraph::AddEdgeLocked(vidtype fromId, vidtype toId) -> bool
{
    auto const fromVertex = m_idToVertex.value(fromId);
    auto const toVertex = m_idToVertex.value(toId);

    if (!m_graph.has_vertex(fromVertex) || !m_graph.has_vertex(toVertex))
    {
        qWarning() << "VPatternGraph::AddEdge: vertex id maps to invalid graph vertex. fromId=" << fromId
                   << "toId=" << toId;
        return false;
    }

    m_graph.add_edge(fromVertex, toVertex, VEdge{});
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Remove a vertex from the graph by its ID
 * @param id Node ID to remove
 * @return true if vertex was removed
 */
auto VPatternGraph::RemoveVertex(vidtype id) -> bool
{
    QWriteLocker const locker(&m_lock);

    if (!m_idToVertex.contains(id))
    {
        return false;
    }

    auto const vertexId = m_idToVertex.value(id);
    if (!m_graph.has_vertex(vertexId))
    {
        return false;
    }

    m_graph.remove_vertex(vertexId);
    m_idToVertex.remove(id);
    m_vertexToId.remove(vertexId);

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Remove an edge from the graph by node IDs
 * @param fromId Source node ID
 * @param toId Target node ID
 * @return true if edge was removed
 */
auto VPatternGraph::RemoveEdge(vidtype fromId, vidtype toId) -> bool
{
    QWriteLocker const locker(&m_lock);

    if (!m_idToVertex.contains(fromId) || !m_idToVertex.contains(toId))
    {
        return false;
    }

    auto const fromVertex = m_idToVertex.value(fromId);
    auto const toVertex = m_idToVertex.value(toId);

    if (!m_graph.has_edge(fromVertex, toVertex))
    {
        return false;
    }

    m_graph.remove_edge(fromVertex, toVertex);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Check if a vertex exists in the graph by its ID
 * @param id Node ID to check
 * @return true if vertex exists
 */
auto VPatternGraph::HasVertex(vidtype id) const -> bool
{
    QReadLocker const locker(&m_lock);
    return m_idToVertex.contains(id);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Check if an edge exists in the graph by node IDs
 * @param fromId Source node ID
 * @param toId Target node ID
 * @return true if edge exists
 */
auto VPatternGraph::HasEdge(vidtype fromId, vidtype toId) const -> bool
{
    QReadLocker const locker(&m_lock);

    if (!m_idToVertex.contains(fromId) || !m_idToVertex.contains(toId))
    {
        return false;
    }

    auto const fromVertex = m_idToVertex.value(fromId);
    auto const toVertex = m_idToVertex.value(toId);

    return m_graph.has_edge(fromVertex, toVertex);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get the node data by ID
 * @param id Node ID
 * @return Optional containing the node if it exists
 */
auto VPatternGraph::GetVertex(vidtype id) const -> std::optional<VNode>
{
    QReadLocker const locker(&m_lock);

    if (!m_idToVertex.contains(id))
    {
        return std::nullopt;
    }

    auto const vertexId = m_idToVertex.value(id);
    if (!m_graph.has_vertex(vertexId))
    {
        return std::nullopt;
    }

    return m_graph.get_vertex(vertexId);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get all node IDs in the graph
 * @return List of node IDs
 */
auto VPatternGraph::GetVertices() const -> QList<vidtype>
{
    QReadLocker const locker(&m_lock);
    return m_idToVertex.keys();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get the number of vertices in the graph
 * @return Vertex count
 */
auto VPatternGraph::VertexCount() const -> std::size_t
{
    QReadLocker const locker(&m_lock);
    return m_graph.vertex_count();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get the number of edges in the graph
 * @return Edge count
 */
auto VPatternGraph::EdgeCount() const -> std::size_t
{
    QReadLocker const locker(&m_lock);
    return m_graph.edge_count();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get all neighbor IDs (outgoing edges) of a node
 * @param id Node ID
 * @return Vector of neighbor node IDs
 */
auto VPatternGraph::GetNeighbors(vidtype id) const -> QVector<vidtype>
{
    QReadLocker const locker(&m_lock);

    if (!m_idToVertex.contains(id))
    {
        return {};
    }

    auto const vertexId = m_idToVertex.value(id);
    if (!m_graph.has_vertex(vertexId))
    {
        return {};
    }

    QVector<vidtype> neighbors;
    auto const nodeNeighbors = m_graph.get_neighbors(vertexId);
    neighbors.reserve(static_cast<vsizetype>(nodeNeighbors.size()));

    for (const auto &neighborVertexId : nodeNeighbors)
    {
        if (m_vertexToId.contains(neighborVertexId))
        {
            neighbors.append(m_vertexToId.value(neighborVertexId));
        }
    }
    return neighbors;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get all predecessors (incoming edges) of a node
 * @param id Node ID
 * @return Vector of predecessor node IDs
 */
auto VPatternGraph::GetPredecessors(vidtype id) const -> QVector<vidtype>
{
    QReadLocker const locker(&m_lock);

    if (!m_idToVertex.contains(id))
    {
        return {};
    }

    auto const vertexId = m_idToVertex.value(id);
    if (!m_graph.has_vertex(vertexId))
    {
        return {};
    }

    QVector<vidtype> predecessors;

    for (const auto &[vid, node] : m_graph.get_vertices())
    {
        if (m_graph.has_edge(vid, vertexId) && m_vertexToId.contains(vid))
        {
            predecessors.append(m_vertexToId.value(vid));
        }
    }
    return predecessors;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get all node IDs of a specific type
 * @param type Node type to filter by
 * @return Vector of node IDs matching the type
 */
auto VPatternGraph::GetVerticesByType(VNodeType type) const -> QVector<vidtype>
{
    QReadLocker const locker(&m_lock);
    QVector<vidtype> result;

    for (const auto &[vertexId, node] : m_graph.get_vertices())
    {
        if (node.type == type && m_vertexToId.contains(vertexId))
        {
            result.append(m_vertexToId.value(vertexId));
        }
    }
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get the degree of a node (number of all edges)
 * @param id Node ID
 * @return Degree count
 */
auto VPatternGraph::GetDegree(vidtype id) const -> std::size_t
{
    QReadLocker const locker(&m_lock);

    if (!m_idToVertex.contains(id))
    {
        return 0;
    }

    auto const vertexId = m_idToVertex.value(id);
    if (!m_graph.has_vertex(vertexId))
    {
        return 0;
    }

    return graaf::properties::vertex_degree(m_graph, vertexId);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get the in-degree of a node (number of incoming edges)
 * @param id Node ID
 * @return In-degree count
 */
auto VPatternGraph::GetInDegree(vidtype id) const -> std::size_t
{
    QReadLocker const locker(&m_lock);

    if (!m_idToVertex.contains(id))
    {
        return 0;
    }

    auto const vertexId = m_idToVertex.value(id);
    if (!m_graph.has_vertex(vertexId))
    {
        return 0;
    }

    return graaf::properties::vertex_indegree(m_graph, vertexId);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get the out-degree of a node (number of outgoing edges)
 * @param id Node ID
 * @return Out-degree count
 */
auto VPatternGraph::GetOutDegree(vidtype id) const -> std::size_t
{
    QReadLocker const locker(&m_lock);

    if (!m_idToVertex.contains(id))
    {
        return 0;
    }

    auto const vertexId = m_idToVertex.value(id);
    if (!m_graph.has_vertex(vertexId))
    {
        return 0;
    }

    return graaf::properties::vertex_outdegree(m_graph, vertexId);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Remove all incoming edges to a node
 * @param id Node ID whose incoming edges should be removed
 * @return Number of edges removed
 * 
 * Example: If A->C, B->C exist, RemoveIncomingEdges(C) removes both edges
 */
auto VPatternGraph::RemoveIncomingEdges(vidtype id) -> size_t
{
    QWriteLocker const m_locker(&m_lock);

    if (!m_idToVertex.contains(id))
    {
        return 0;
    }

    auto vertexId = m_idToVertex[id];
    if (!m_graph.has_vertex(vertexId))
    {
        return 0;
    }

    size_t removedCount = 0;

    // Collect all predecessors first to avoid iterator invalidation
    std::vector<vertex_id_t> predecessors;
    for (const auto &[vid, node] : m_graph.get_vertices())
    {
        if (m_graph.has_edge(vid, vertexId))
        {
            predecessors.push_back(vid);
        }
    }

    // Remove all incoming edges
    for (const auto &pred : predecessors)
    {
        m_graph.remove_edge(pred, vertexId);
        ++removedCount;
    }

    return removedCount;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Clear all vertices and edges from the graph
 */
void VPatternGraph::Clear()
{
    QWriteLocker const locker(&m_lock);

    // An edge recorded during an earlier build and still waiting has had a whole parse to find its missing
    // endpoint: the object it names is not in the pattern. Say so, then let it go. An edge recorded during
    // the build that is ending is kept for the next one: the graph was cleared while a formula-dependency
    // worker was still walking the tools, and that build is where its vertex will appear.
    QVector<PendingEdge> stillWaiting;
    for (const PendingEdge &edge : m_pendingEdges)
    {
        if (edge.build < m_build)
        {
            qWarning() << "VPatternGraph: an object depends on one the pattern does not define. fromId="
                       << edge.fromId << "toId=" << edge.toId;
            continue;
        }
        stillWaiting.append(edge);
    }
    m_pendingEdges = stillWaiting;

    ++m_build;

    m_graph = Graph{};
    m_idToVertex.clear();
    m_vertexToId.clear();
    m_graphRebuildEnabled = true;
}

//---------------------------------------------------------------------------------------------------------------------
void VPatternGraph::SetGraphRebuildEnabled(bool enabled)
{
    QWriteLocker const locker(&m_lock);
    m_graphRebuildEnabled = enabled;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPatternGraph::IsGraphRebuildEnabled() const -> bool
{
    QReadLocker const locker(&m_lock);
    return m_graphRebuildEnabled;
}

//---------------------------------------------------------------------------------------------------------------------
/**
     * @brief Get internal vertex_id_t for a node ID (for advanced use)
     * @param id Node ID
     * @return Optional containing vertex_id_t if node exists
     * 
     * This is useful when you need to use graaf algorithms directly
     * that require vertex_id_t.
     */
auto VPatternGraph::GetInternalVertexId(vidtype id) const -> std::optional<vertex_id_t>
{
    QReadLocker const locker(&m_lock);

    if (!m_idToVertex.contains(id))
    {
        return std::nullopt;
    }
    return m_idToVertex.value(id);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Get node ID from internal vertex_id_t (for advanced use)
 * @param vertexId Internal vertex ID
 * @return Optional containing node ID if mapping exists
 */
auto VPatternGraph::GetNodeId(vertex_id_t vertexId) const -> std::optional<vidtype>
{
    QReadLocker const locker(&m_lock);

    if (!m_vertexToId.contains(vertexId))
    {
        return std::nullopt;
    }

    return m_vertexToId.value(vertexId);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPatternGraph::TryGetDependentBlocks(int indexPatternBlock, int timeout) const -> std::optional<QSet<int>>
{
    if (!m_lock.tryLockForRead(timeout))
    {
        return std::nullopt;
    }

    auto Unlock = qScopeGuard([this]() -> auto { m_lock.unlock(); });

    // Every node of the block is a starting point: a tool of another block can read any one of them. One walk
    // from all of them together, not one walk each — this runs while a piece is being dragged.
    QSet<vidtype> seen;
    QQueue<vidtype> queue;
    for (auto it = m_idToVertex.constBegin(); it != m_idToVertex.constEnd(); ++it)
    {
        if (m_graph.has_vertex(it.value()) && m_graph.get_vertex(it.value()).indexPatternBlock == indexPatternBlock)
        {
            seen.insert(it.key());
            queue.enqueue(it.key());
        }
    }

    QSet<int> blocks;
    while (!queue.isEmpty())
    {
        const vidtype id = queue.dequeue();
        if (!m_idToVertex.contains(id))
        {
            continue;
        }

        const auto vertexId = m_idToVertex.value(id);
        if (!m_graph.has_vertex(vertexId))
        {
            continue;
        }

        for (const auto &neighbor : m_graph.get_neighbors(vertexId))
        {
            if (!m_vertexToId.contains(neighbor))
            {
                continue;
            }

            const vidtype neighborId = m_vertexToId.value(neighbor);
            if (seen.contains(neighborId))
            {
                continue;
            }
            seen.insert(neighborId);

            const VNode &node = m_graph.get_vertex(neighbor);
            if (node.indexPatternBlock != indexPatternBlock)
            {
                blocks.insert(node.indexPatternBlock);
            }
            queue.enqueue(neighborId);
        }
    }

    return blocks;
}
