/************************************************************************
 **
 **  @file   ocopypieces.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/11/2026
 **
 **  @brief  The pieces a copy inherits from the curve it was made of.
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

#ifndef OCOPYPIECES_H
#define OCOPYPIECES_H

#include <QSet>
#include <QString>
#include <QVector>
#include <QtGlobal>

class VAbstractPattern;
class VContainer;

/**
 * @brief What a copy made by an operation inherits from the curve it was made of: the way that curve is cut.
 *
 * The curve an operation copies can be cut three ways, and its copy follows each of them, in that direction only:
 *
 *  - the boxes of a path, ticked on its interior nodes — the copy is cut on the same nodes;
 *  - the chest/back width of an armhole curve shown as its two pieces — the copy is cut there too;
 *  - a cut tool placed on the curve at a length — the copy is cut at the place that point was carried to.
 *
 * A copy of a copy follows the curve at the top of its family, and the cuts placed on every curve of that family.
 * Nothing goes the other way: a copy has no box of its own, and cutting a copy leaves the curve it comes from whole.
 *
 * The pieces are objects of the pattern: an operation copies them, a tool is placed on them, a formula reads their
 * length. They therefore keep their ids from one parse to the next — written on the destination item of their copy,
 * `pieceIds` — and a piece something is built on stays in the pattern when the cut that made it goes away: unticking
 * a box takes the pieces off a copy of the whole curve, never off the objects built on them (PruneUnused).
 *
 * The operation draws them (VAbstractOperation::RefreshCopyPieces); this class builds them, and keeps the file and
 * the dependency graph in step.
 */
class OCopyPieces
{
public:
    /// The attribute of a `<destination><item>` that holds the pieces of that copy: `n0-1=101;c90-2=103`.
    static const QString AttrPieceIds;

    /** @brief One piece of one copy. */
    struct Piece
    {
        quint32 id{0};
        quint32 copyId{0};
        /// Drawn on the canvas: the pieces of the boxes, the halves of a cut shown as its two segments, and the
        /// prolongation a cut past an end draws. A piece kept for its users is not, nor a half its cut keeps hidden.
        bool shown{false};
    };

    /** @brief What the pieces of one operation are, once built. */
    struct Pieces
    {
        QVector<Piece> pieces{};
        /// The copies their pieces replace on the canvas, exactly as the curve they come from is replaced by its own.
        QSet<quint32> replacedCopies{};
    };

    /**
     * @brief Build the pieces of every copy @a operationId made, from what the file says now.
     *
     * Writes them into @a data under the ids the file gave them (new ones get theirs written back), and gives them
     * their place in the dependency graph — a vertex each, and an edge from their copy. Called on every parse, from
     * the moment the copies exist (VAbstractOperation::CreateDestinationLines), and again when a cut placed later in
     * the file comes into being (FollowCut) or when the operation refreshes its drawing.
     */
    static auto Build(VAbstractPattern *doc, VContainer *data, quint32 operationId) -> Pieces;

    /** @brief A cut tool on @a curveId has just been built: every copy of that curve, and of its copies, is cut too.
     *  The copy may come before the cut in the file — it was made before the curve was cut — and was then built
     *  with nothing to follow. */
    static void FollowCut(VAbstractPattern *doc, VContainer *data, quint32 curveId);

    /**
     * @brief The boxes of @a curveId are about to become @a cutNodes: forget, on every copy of the curve, the pieces
     * that will stop being runs and that nothing is built on.
     *
     * Asked at the moment the box changes, while the graph knows every user. The parse that follows rebuilds every
     * copy from the new boxes; a piece left here is kept for the objects built on it.
     */
    static void PruneUnused(VAbstractPattern *doc, VContainer *data, quint32 curveId, const QVector<int> &cutNodes);

    /** @brief The halves a cut tool gave the copies of its curve. A cut whose halves are in use on a copy cannot be
     *  deleted, exactly as one whose own halves are (see VToolCut::IsRemovable). */
    static auto HalvesOfCut(VAbstractPattern *doc, quint32 cutId) -> QVector<quint32>;

private:
    static auto CutKey(quint32 cutId, int half) -> QString;
    static auto NodeKey(const QString &runKey) -> QString;
};

#endif // OCOPYPIECES_H
