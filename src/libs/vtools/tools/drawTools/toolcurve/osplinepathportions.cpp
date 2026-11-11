/************************************************************************
 **
 **  @file   osplinepathportions.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  The runs of a spline path, once the user has cut it at some of its own nodes.
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

#include "osplinepathportions.h"

#include "vabstractcubicbezierpath.h"

#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"

#include <algorithm>

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::Runs(const QVector<int> &cutNodes, vsizetype points) -> QVector<Run>
{
    QVector<Run> runs;

    if (points < 2)
    {
        return runs; // a path of one point carries no run at all
    }

    // The frontiers: both ends, and every node the user cut on. Sorted and without duplicates, so two runs can
    // never overlap and the order of the boxes cannot change the answer.
    QVector<int> frontiers{0};

    for (const int node : cutNodes)
    {
        if (node > 0 && node < points - 1)
        {
            frontiers.append(node);
        }
    }

    frontiers.append(static_cast<int>(points) - 1);

    std::sort(frontiers.begin(), frontiers.end());
    frontiers.erase(std::unique(frontiers.begin(), frontiers.end()), frontiers.end());

    for (qsizetype index = 1; index < frontiers.size(); ++index)
    {
        runs.append(Run{frontiers.at(index - 1), frontiers.at(index)});
    }

    return runs;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::RunEndingAt(const QVector<Run> &runs, int node) -> QVector<Run>
{
    for (const Run &run : runs)
    {
        if (run.to == node)
        {
            return {run};
        }
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::Take(const VAbstractCubicBezierPath &path, const Run &run) -> VSplinePath
{
    QVector<VSpline> parts;
    parts.reserve(run.to - run.from);

    for (int index = run.from + 1; index <= run.to; ++index)
    {
        parts.append(path.GetSpline(index));
    }

    // A path is built from two sub-splines at the least; one alone is not a path and would come out empty.
    if (parts.size() < 2)
    {
        return VSplinePath(); // a path of one sub-spline has no geometry: the caller draws nothing
    }

    VSplinePath portion(parts);
    portion.SetApproximationScale(path.GetApproximationScale());
    return portion;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::EncodeNodes(const QVector<int> &cutNodes) -> QString
{
    QStringList parts;
    parts.reserve(cutNodes.size());

    for (const int node : cutNodes)
    {
        parts.append(QString::number(node));
    }

    return parts.join(QLatin1Char(' '));
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::DecodeNodes(const QString &value) -> QVector<int>
{
    QVector<int> nodes;

    const QStringList parts = value.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    for (const QString &part : parts)
    {
        bool ok = false;
        const int node = part.toInt(&ok);

        if (ok && node > 0)
        {
            nodes.append(node);
        }
    }

    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::EncodeIds(const QMap<int, quint32> &ids) -> QString
{
    QStringList parts;

    for (auto it = ids.cbegin(); it != ids.cend(); ++it)
    {
        parts.append(QStringLiteral("%1=%2").arg(it.key()).arg(it.value()));
    }

    return parts.join(QLatin1Char(';'));
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::DecodeIds(const QString &value) -> QMap<int, quint32>
{
    QMap<int, quint32> ids;

    for (const QString &part : value.split(QLatin1Char(';'), Qt::SkipEmptyParts))
    {
        const qsizetype equal = part.indexOf(QLatin1Char('='));

        if (equal <= 0)
        {
            continue;
        }

        bool okNode = false;
        bool okId = false;
        const int node = part.left(equal).toInt(&okNode);
        const quint32 id = part.mid(equal + 1).toUInt(&okId);

        if (okNode && okId && node > 0 && id != 0)
        {
            ids.insert(node, id);
        }
    }

    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
void OSplinePathPortions::SyncGraph(VAbstractPattern *doc, quint32 motherId, const QVector<quint32> &before,
                                    const QVector<quint32> &after)
{
    if (doc == nullptr)
    {
        return;
    }

    VPatternGraph *const graph = doc->PatternGraph();

    if (graph == nullptr)
    {
        return;
    }

    // A piece that is gone takes its vertex and its edge with it: a graph that kept them would hold an id no
    // object answers to, and everything that walks the graph — the dependency tree, a deletion — would read it.
    for (const quint32 id : before)
    {
        if (id == NULL_ID || after.contains(id))
        {
            continue;
        }

        graph->RemoveEdge(motherId, id);
        graph->RemoveVertex(id);
    }

    const int blockId = doc->PatternBlockMapper()->GetActiveId();

    for (const quint32 id : after)
    {
        if (id == NULL_ID)
        {
            continue;
        }

        // The vertex is added only when it is missing: a rebuild happens on every parse, and a curve read again
        // with the same piece ids would otherwise log "vertex already exists" on each one.
        if (not graph->HasVertex(id))
        {
            graph->AddVertex(id, VNodeType::OBJECT, blockId);
        }

        graph->AddEdge(motherId, id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OSplinePathPortions::PortionSuffix(int index) -> QString
{
    return QStringLiteral("__p%1").arg(index);
}
