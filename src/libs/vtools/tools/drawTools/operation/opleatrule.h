/************************************************************************
 **
 **  @file   o-pleatrule.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The rule of a pleat: how much fabric each fold takes, where its fold line runs, and where the
 **          piece's edge has to be cut for the fold to close.
 **
 **          A pleat is fabric folded back on itself. The user gives the departure lines — the lines the pleats
 **          are taken on, each of them a pair of points — the value each one shows once folded, and the objects
 **          that travel. The rule derives the rest:
 **
 **          - the move: twice each value, perpendicular to the departure lines, on the side the objects that
 **            travel are on. It is never typed by the user, and that is the whole point: a move that is not
 **            perpendicular to the departure line does not send that line back onto its own copy, and the fold
 **            then closes on a mismatch measured in tenths of a millimetre. This rule exists to make that
 **            impossible.
 **          - how far each departure line has been carried: a pleat takes the fabric beyond its line and moves
 **            it, so a second line lying beyond the first one travels with it. The carry is read from the
 **            geometry — where the lines lie along the normal — and never from the order of the clicks.
 **          - the fold line of each pleat: its departure line, carried and translated by half a step, which is
 **            also the perpendicular bisector of every point of that line and its copy.
 **          - the centre of each end: the middle of the point and its copy. It lies on the piece's edge when
 **            that edge is parallel to the move, and it is then the whole mark.
 **          - the cap of each end: the V whose apex is where the piece's edge crosses the fold line, drawn
 **            from the edge itself (first sense) or from the same edge carried by the move (second sense).
 **            Folding sends one side of the V onto the other, so the edge continues instead of breaking.
 **
 **          The edge is always read where the user drew it — at the point of the departure line — and never on a
 **          copy: a pleat only translates, so a copy carries the same edge as its origin, and nothing else could
 **          be read there (see OPleatSupport).
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
#include <QString>
#include <QVector>
#include <QtGlobal>

#include "../../toolsdef.h"
#include "opleatend.h"

class VContainer;

/** @brief One pleat, or two of the same value side by side with the second one reversed. */
enum class OPleatKind : int
{
    Simple = 1,
    Double = 2
};

/** @brief The side of the departure line a pleat is built on: the side the objects that travel are on.
 *
 *  The two values are the two orientations of the departure line's normal: @a First is the side of the vector
 *  `(-u.y, u.x)`, built on that line's own direction `u`. @a SideOf answers in those terms, and @a Compose
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
    DegeneratePrise, ///< a departure line is degenerate: its two points are the same point
    DegenerateValue, ///< a value is null or negative: there is no fabric to fold
    NoSide,          ///< nothing tells which side of the lines is folded: all of it, or both of its sides
    UnresolvedEdge,  ///< an end has a cap but no edge to follow: the user has to designate one
    EdgeAlongFold    ///< an edge runs along the fold line: it never meets it, so no cap can be built there
};

/** @brief One departure line of the operation: the pair of points the fold is taken on, the value it shows once
 *  folded, and whether it is one fold or two side by side.
 *
 *  The line is given **flat**, as the user drew it, and its points are the ones the edge of the piece is read
 *  at: what the pleats before it do to that line is the rule's business, never the user's (see OPleatPlace). */
struct OPleatDeparture
{
    OPleatEnd first{};
    OPleatEnd second{};
    QString value{};
    OPleatKind kind{OPleatKind::Simple};
};

/** @brief Where one departure line is folded, and what one of its pleats takes.
 *
 *  Every distance is a vector, and every line keeps its own direction: a pleat takes the fabric beyond **its** line
 *  and moves it along **its** normal, and two translations compose whatever their directions — the departure lines
 *  of one operation therefore have no reason to be parallel, and a modéliste may take two pleats along two different
 *  axes. What a line has in common with the others is only the half-plane the fabric goes to, which is what orients
 *  its normal (see @a normal).
 *
 *  @a carry is what the pleats taken on the OTHER lines did to this one before its own fold: a line lying beyond
 *  another one travels with it, and it is then folded where it arrived. @a step is one pleat of this line, twice its
 *  value, along @a normal. @a tolerance is how close to the line a point still counts as lying on it.
 *
 *  @a firstPleat is the index of the departure's first pleat among all the pleats of the operation, which is what
 *  the marks and the fold lines are numbered by. */
struct OPleatPlace
{
    QPointF origin{};
    QPointF normal{};
    QPointF carry{};
    QPointF step{};
    int pleats{1};
    int firstPleat{0};
    qreal tolerance{0.};
};

/**
 * @brief One mark: the point that materializes an end of a pleat.
 *
 * @a departure is the departure line the pleat was taken on, @a pleat is the index of that pleat among all the
 * pleats of the operation, and @a end is 0 for the first point of the departure line and 1 for the other.
 * @a flat means the edge is parallel to the move: the mark is the centre alone, and the V has nothing to draw —
 * that is the case of a hem, and it is a case in its own right, not a degenerate cap.
 */
struct OPleatMark
{
    int departure{0};
    int pleat{0};
    int end{0};
    bool flat{false};
    QPointF point{};
};

/**
 * @brief What the pleats do, as the rule resolves them.
 *
 * @a places holds one entry per departure line, **in the order the user gave them**, and @a marks the resolved
 * ends, pleat after pleat and, in each pleat, first end then second: the fold line of a pleat joins its two
 * marks. Every distance is a vector, and a move is read by asking the plan (see Move).
 */
struct OPleatPlan
{
    OPleatFailure failure{OPleatFailure::None};
    QVector<OPleatPlace> places{};
    QVector<OPleatMark> marks{};

    /** @brief The mark of one end of one pleat, or nullptr when the plan holds none. */
    auto Mark(int pleat, int end) const -> const OPleatMark *;

    /** @brief Where a point of the pattern lands: the sum of the pleats that point lies on or beyond.
     *
     *  A point on a departure line belongs to the fabric that travels — the fold carries the line itself onto
     *  its own copy — so it is carried by that pleat as well, and the edge of the tolerance is what says a
     *  point drawn on the line is on it.
     *
     *  The steps are vectors, so the sum is a vector: two pleats taken along two different axes compose as any two
     *  translations do, and the order they were given in changes nothing. */
    auto Move(const QPointF &point) const -> QPointF;

    /** @brief Where one end of a departure line is, once the pleats before it have carried it.
     *
     *  @a line is 0 for the line as the user drew it, 1 for its first copy, and so on: a pleat of that
     *  departure is laid between two consecutive places. */
    auto Place(const QPointF &prisePoint, int departure, int line) const -> QPointF;

    /** @brief How many pleats the operation holds: one per simple departure, two per double one. */
    auto PleatCount() const -> int;
};

/**
 * @brief The rule of the pleats.
 *
 * No state: every method is a reading or a computation, and none of them touches the pattern.
 */
class OPleatRule
{
public:
    /** @brief The side of a departure line the objects that travel are on.
     *
     *  Only the sources that are points are read, and the two points of the line are left out: they lie on the
     *  line, so they carry no side. Sources on both sides — or none of them on either side — leave the side
     *  undetermined, and the tool then refuses rather than folding a pleat the wrong way. */
    static auto SideOf(const QVector<SourceItem> &source, quint32 firstId, quint32 secondId,
                       const VContainer *data) -> OPleatSide;

    /** @brief The plan of the pleats: how far each line was carried, and the mark of each end of each pleat.
     *
     *  @a values holds the value of each departure line, in pixels and in the same order as @a departures; the
     *  caller evaluates the formulas, since a formula is the tool's business and not the rule's. The side is
     *  read here, of every departure line that can answer for it, so that a line the selection straddles says
     *  nothing instead of refusing the whole operation. */
    static auto Compose(const QVector<OPleatDeparture> &departures, const QVector<qreal> &values,
                        const QVector<SourceItem> &source, const VContainer *data) -> OPleatPlan;

private:
    Q_DISABLE_COPY_MOVE(OPleatRule) // NOLINT
    OPleatRule() = default;
};

#endif // OPLEATRULE_H
