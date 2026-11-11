/************************************************************************
 **
 **  @file   osplinepathportions.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  The runs of a spline path, once the user has cut it at some of its own nodes.
 **
 **          A path carries its nodes; checking one of them does not cut a curve with a length, it names a
 **          frontier the geometry already has. n frontiers make n + 1 runs, and each run is exactly the
 **          sub-splines between two of them — no approximation, nothing recomputed.
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

#ifndef OSPLINEPATHPORTIONS_H
#define OSPLINEPATHPORTIONS_H

#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

#include "vsplinepath.h"

class VAbstractCubicBezierPath;
class VAbstractPattern;

/**
 * @brief What a set of cut nodes makes of a path: the runs, and how they are written down.
 *
 * No state and no widget: the rule alone, so the tool that owns the path and the dock that shows the boxes
 * cannot answer it differently.
 */
class OSplinePathPortions
{
public:
    /** @brief A run of the path: the node it starts on and the node it ends on, both inclusive. */
    struct Run
    {
        int from{0};
        int to{0};

        auto operator==(const Run &other) const -> bool = default;
    };

    /**
     * @brief The runs a path is cut into, in the order the path runs.
     *
     * The first and the last node are always frontiers — they end the curve — and a node the user cut on is
     * one too. Nodes outside 1 .. points-2 are ignored: neither end can be cut, since the piece before it or
     * after it would be empty.
     */
    static auto Runs(const QVector<int> &cutNodes, vsizetype points) -> QVector<Run>;

    /** @brief The run that ends on @a node, or an empty list when no run does. */
    static auto RunEndingAt(const QVector<Run> &runs, int node) -> QVector<Run>;

    /** @brief @a path between two node indices, as a path of two sub-splines or more. Empty when there is less. */
    static auto Take(const VAbstractCubicBezierPath &path, const Run &run) -> VSplinePath;

    /** @brief `2 3 5` — the cut nodes as the pattern writes them, empty when there are none. */
    static auto EncodeNodes(const QVector<int> &cutNodes) -> QString;

    /** @brief The cut nodes of `2 3 5`, ignoring anything that is not a number. */
    static auto DecodeNodes(const QString &value) -> QVector<int>;

    /** @brief `2=101;4=103` — the id the container gave each run, keyed by the node it ends on. */
    static auto EncodeIds(const QMap<int, quint32> &ids) -> QString;

    /** @brief The run ids of `2=101;4=103`. */
    static auto DecodeIds(const QString &value) -> QMap<int, quint32>;

    /**
     * @brief The suffix a portion carries after the name of the curve it comes from: `__p1`, `__p2`…
     *
     * A portion is a derivative of its curve, exactly as the two halves a cut makes are: it keeps the curve's
     * name and adds its own mark. Named from the points it happens to join instead, a portion took the name of
     * an existing curve of the pattern — the same points give the same name — and two objects answering to one
     * name is what a formula, a layer and an operation read as one object.
     */
    static auto PortionSuffix(int index) -> QString;

    /**
     * @brief Keep the dependency graph in step with the pieces of one curve.
     *
     * A piece is an object of the pattern, so the graph has to hold it as one: without a vertex and an edge
     * from the curve it comes from, nothing that walks the graph — the dependency tree, the deletion of a
     * curve with what it made, an operation asking what a source depends on — ever sees it. The pieces of a
     * tool that rebuilds its images are rebuilt from the file without a `Create` of their own, so nothing
     * else will register them.
     *
     * @param before the ids the pieces had before the rebuild
     * @param after the ids they have now: the ones that are gone leave the graph with this call
     */
    static void SyncGraph(VAbstractPattern *doc, quint32 motherId, const QVector<quint32> &before,
                          const QVector<quint32> &after);
};

#endif // OSPLINEPATHPORTIONS_H
