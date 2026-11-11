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

#include "../vgeometry/vsplinepath.h"

class VAbstractCubicBezierPath;
class VAbstractCurve;
class VAbstractPattern;
class VContainer;

#include <QSharedPointer>

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

    /** @brief One piece as the pattern writes it down: its object, and — for a piece kept for its users — the name
     *  it had when it stopped being a run. */
    struct StoredPiece
    {
        quint32 id{NULL_ID};
        /// Empty for a piece the current runs name. A piece that outlives its run (see IsInUse) has no index left
        /// in the runs to be named after, and its name is what a formula cites: it is written beside its id.
        QString name{};

        auto operator==(const StoredPiece &other) const -> bool = default;
    };

    /** @brief `2-5`: the key of a run, written by its two ends. A run keyed by the node it ends on alone changed
     *  identity when a box before it was unticked: the merged run inherited the id of the run on its right, and an
     *  operation that copied that piece silently copied a longer one. */
    static auto RunKey(const Run &run) -> QString;

    /** @brief The run a key names. False for a key that is not a run (`c90-1`, the key of a cut half). */
    static auto ParseRunKey(const QString &key, Run &run) -> bool;

    /** @brief The key a run had in a file written before runs were keyed by both ends: `5`, the node it ends on.
     *  Read once, matched against the runs of the moment, never written again. */
    static auto LegacyRunKey(int to) -> QString;

    /** @brief `0-2=101;2-5=A18__p2=103` — every piece of one curve, keyed: `key=id`, or `key=name=id` for a piece
     *  kept with its own name. The id is always last, so the counter of the pattern reads it like any list of ids
     *  (see VPattern::MaxMentionedId). */
    static auto EncodePieces(const QMap<QString, StoredPiece> &pieces) -> QString;

    /** @brief The pieces of such a value. A legacy `node=id` list is read too: its keys are LegacyRunKey. */
    static auto DecodePieces(const QString &value) -> QMap<QString, StoredPiece>;

    /**
     * @brief Whether something of the pattern is built on @a id: a tool, a piece, an operation that copies it, a
     * formula that reads it.
     *
     * Asked at the moment a piece is about to stop being a run — a box unticked — and the answer decides whether it
     * leaves the pattern or stays for the objects built on it. A graph not complete yet, or a lock not obtained, is
     * answered "in use": keeping a piece nobody needs costs an object, dropping one somebody needs costs a pattern
     * that no longer opens.
     */
    static auto IsInUse(const VAbstractPattern *doc, quint32 id) -> bool;

    /** @brief The run @a run of @a path, as the curve it is: one sub-spline is a spline — a path is built from two at
     *  the least and refuses fewer —, two or more a path. Null when the run holds no geometry. */
    static auto RunCurve(const VAbstractCubicBezierPath &path, const Run &run) -> QSharedPointer<VAbstractCurve>;

    /** @brief What every piece is: a derivative of the curve it comes from, owned by @a ownerId, drawn with that
     *  curve's pen — same colour, same style, same approximation: it IS that curve, piece by piece. Named like the
     *  halves a cut makes, the curve's name plus its own mark, never after the points it joins: two objects answering
     *  to one name are one object for every reader of the pattern. */
    static void MarkPiece(VAbstractCurve &piece, const VAbstractCurve &from, quint32 ownerId, const QString &suffix);

    /**
     * @brief Store @a piece in @a data under @a id, and give it the variables a curve of its kind has.
     *
     * A known id is the identity of the piece: the container adopts an object handed to it under that id even when it
     * holds nothing there yet — the state a parse leaves. It is reused only when what the container holds there is
     * this very piece: the same kind of curve, made by the same owner. An id that names something else — a file
     * edited by hand, two pieces given one id — would have the piece overwrite another object of the pattern, or the
     * container refuse the cast and close the file: the piece takes a new id instead. Returns the id it is stored
     * under, NULL_ID for a kind no piece is made of.
     */
    static auto StorePiece(VContainer *data, const QSharedPointer<VAbstractCurve> &piece, quint32 id) -> quint32;

    /** @brief The name a piece kept for its users falls back on when nothing else names it: its two nodes, which no
     *  run of the boxes is ever named after (`__p2_5`). */
    static auto KeptSuffix(const QString &curveName, const Run &run) -> QString;

    /** @brief The curve @a id names, or null when it names nothing or something that is not a curve. */
    static auto CurveOf(const VContainer *data, quint32 id) -> QSharedPointer<VAbstractCurve>;
};

#endif // OSPLINEPATHPORTIONS_H
