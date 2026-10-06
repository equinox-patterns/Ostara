/************************************************************************
 **
 **  @file   o-pleatrule.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The rule of a pleat: how much fabric the fold takes, where the fold line runs, and where the
 **          piece's edge has to be cut for the fold to close.
 **
 **          A pleat is fabric folded back on itself. The user gives the prise — the line the pleat is taken
 **          on, that is its two points — and the value the pleat shows once folded. The rule derives the rest:
 **
 **          - the move: twice the value, perpendicular to the prise, on the side the objects that travel are
 **            on. It is never typed by the user, and that is the whole point: a move that is not perpendicular
 **            to the prise does not send the prise back onto its own copy, and the fold then closes on a
 **            mismatch measured in tenths of a millimetre. This rule exists to make that impossible.
 **          - the fold line: the prise translated by half the move, which is also the perpendicular bisector
 **            of every point of the prise and its copy.
 **          - the centre of each end: the middle of the point and its copy. It lies on the piece's edge when
 **            that edge is parallel to the move, and it is then the whole mark.
 **          - the cap of each end: the V whose apex is where the piece's edge crosses the fold line, drawn
 **            from the edge itself (first sense) or from the same edge carried by the move (second sense).
 **            Folding sends one side of the V onto the other, so the edge continues instead of breaking.
 **
 **          A double pleat is two pleats of the same value side by side, the second one reversed: the same
 **          arithmetic, run twice, with a total move of four times the value. A table of two marks per pleat
 **          is what comes out, and no special case anywhere.
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

#ifndef OPLEATRULE_H
#define OPLEATRULE_H

#include <QPointF>
#include <QVector>
#include <QtGlobal>

#include "../../toolsdef.h"

class VContainer;

/**
 * @brief One end of the prise, as the tool resolved it.
 *
 * @a position and @a edgeDirection are in pixels. @a edgeDirection is a vector along the piece's edge at that
 * point, the edge the fold has to follow; an empty vector means the tool found no edge there, and the end is
 * then marked like a parallel edge — the centre alone, no V.
 *
 * @a showCap and @a secondSense are what the user asked for that end: whether the cap is drawn at all, and
 * which of the two edges it follows. They are kept per end, because the two ends are independent.
 *
 * @a edgePoint is the point the user designated to carry the edge when the tool could not read one by itself: the
 * line from the prise point to it is then the edge, and it wins over anything the tool would have deduced. It is
 * NULL_ID when the edge is read from the point's own tool — see OPleatSupport — or when it could not be found at
 * all, in which case the caller asks the user for one.
 */
struct OPleatEnd
{
    quint32 pointId{NULL_ID};
    QPointF position{};
    QPointF edgeDirection{};
    quint32 edgePointId{NULL_ID};
    bool showCap{true};
    bool secondSense{false};
};

/** @brief One pleat, or two of the same value side by side with the second one reversed. */
enum class OPleatKind : int
{
    Simple = 1,
    Double = 2
};

/** @brief The side of the prise a pleat is built on: the side the objects that travel are on.
 *
 *  The two values are the two orientations of the prise's normal: @a First is the side of the vector
 *  `(-u.y, u.x)`, built on the prise's own direction `u`. @a SideOf answers in those terms, and @a Compose
 *  reads them back, so that the tool never has to know which way round a normal is. */
enum class OPleatSide : int
{
    Undetermined = 0,
    First = 1,
    Second = -1
};

/** @brief What the rule refuses, and the tool has to say out loud rather than build half a pleat. */
enum class OPleatFailure : int
{
    None = 0,
    DegeneratePrise, ///< the two points of the prise are the same point
    DegenerateValue, ///< the value is null or negative: there is no fabric to fold
    NoSide,          ///< nothing tells which side of the prise is folded: all of it, or both of its sides
    UnresolvedEdge,  ///< an end has a cap but no edge to follow: the user has to designate one
    EdgeAlongFold    ///< an edge runs along the fold line: it never meets it, so no cap can be built there
};

/**
 * @brief One mark: the point that materializes an end of a pleat.
 *
 * @a pleat is 0 for the pleat itself and 1 for the second one of a double; @a end is 0 for the first point of
 * the prise and 1 for the other. @a flat means the edge is parallel to the move: the mark is the centre
 * alone, and the V has nothing to draw — that is the case of a hem, and it is a case in its own right, not a
 * degenerate cap.
 */
struct OPleatMark
{
    int pleat{0};
    int end{0};
    bool flat{false};
    QPointF point{};
};

/**
 * @brief What a pleat does, as the rule resolves it.
 *
 * @a move is the translation applied to every object that travels, in pixels: twice the value for a simple
 * pleat, four times for a double. @a step is that translation for one pleat, which is what places the prise
 * of the second one and the copy of the first. @a marks holds the resolved ends, pleat after pleat and, in
 * each pleat, first end then second: the fold line of a pleat joins its two marks.
 */
struct OPleatPlan
{
    OPleatFailure failure{OPleatFailure::None};
    QPointF move{};
    QPointF step{};
    QVector<OPleatMark> marks{};

    /** @brief The mark of one end of one pleat, or nullptr when the plan holds none. */
    auto Mark(int pleat, int end) const -> const OPleatMark *;
};

/**
 * @brief The rule of a pleat.
 *
 * No state: every method is a reading or a computation, and none of them touches the pattern.
 */
class OPleatRule
{
public:
    /** @brief The side of the prise the objects that travel are on.
     *
     *  Only the sources that are points are read, and the two points of the prise are left out: they lie on
     *  the line, so they carry no side. Sources on both sides — or none of them on either side — leave the
     *  side undetermined, and the tool then refuses rather than folding a pleat the wrong way. */
    static auto SideOf(const QVector<SourceItem> &source, quint32 firstId, quint32 secondId,
                       const VContainer *data) -> OPleatSide;

    /** @brief The plan of a pleat: the move, and the mark of each end of each pleat.
     *
     *  @a value is the pleat as it will show once folded, in pixels; @a side says which side of the prise it
     *  is built on. The two ends are read in the order the user gave them, so mark 0 of a pleat is the mark of
     *  @a first and mark 1 the mark of @a second. */
    static auto Compose(const OPleatEnd &first, const OPleatEnd &second, qreal value, OPleatKind kind,
                        OPleatSide side) -> OPleatPlan;

private:
    Q_DISABLE_COPY_MOVE(OPleatRule) // NOLINT
    OPleatRule() = default;
};

#endif // OPLEATRULE_H
