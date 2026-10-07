/************************************************************************
 **
 **  @file   o-pleatrule.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/10/2026
 **
 **  @brief  The rule of a pleat: how much fabric each fold takes, where its fold line runs, and where the
 **          piece's edge has to be cut for the fold to close.
 **
 **          A pleat is fabric folded back on itself. The user gives the objects that travel, in the order of the
 **          contour, then the departure lines — the lines the pleats are taken on, each of them a pair of points
 **          of that selection — each with the value it shows once folded. The rule derives the rest:
 **
 **          - the region of a pleat: the run of the ordered selection that lies **between the two points of its
 **            departure line**, both included. That is what the pleat takes, exactly as a slash-and-spread takes
 **            the run between the start and the pivot of a cut, and it is why the order of the clicks is the
 **            contract: an object lies in a region or it does not.
 **          - the move: twice the value, perpendicular to the departure line, on the side the fabric of that
 **            region is on. It is never typed by the user, and that is the whole point: a move that is not
 **            perpendicular to the departure line does not send that line back onto its own copy, and the fold
 **            then closes on a mismatch measured in tenths of a millimetre.
 **          - where each departure line has been carried: its two points are objects of the selection like any
 **            other, so they travel with the pleats whose region holds them, and the line is then folded where it
 **            arrived. Nothing is deduced from the geometry — only from the regions.
 **          - the fold line of each pleat: its departure line, carried, translated by half a step, which is also
 **            the perpendicular bisector of every point of that line and its copy.
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

#include <QHash>
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

/** @brief The side of the departure line a pleat is built on: the side the fabric of its region is on.
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
    NoSide,          ///< nothing tells which side of a line is folded: all of it, or both of its sides
    UnresolvedEdge,  ///< an end has a cap but no edge to follow: the user has to designate one
    EdgeAlongFold,   ///< an edge runs along the fold line: it never meets it, so no cap can be built there
    NotchOffEdge     ///< a notch falls past the end of the edge it should mark: the edge is shorter than the value
};

/** @brief One departure line of the operation: the pair of points the fold is taken on, the value it shows once
 *  folded, and whether it is one fold or two side by side.
 *
 *  The two points are **of the ordered selection**, and they are the bounds of the pleat's region: what the rule
 *  takes is the run of objects that lies between them. The line itself is given as the user drew it, and it is
 *  then carried by the pleats whose region holds its points (see OPleatPlace). */
struct OPleatDeparture
{
    OPleatEnd first{};
    OPleatEnd second{};
    QString value{};
    OPleatKind kind{OPleatKind::Simple};
};

/** @brief Where one departure line is folded, and what one of its pleats takes.
 *
 *  @a start and @a last are the bounds of the **region**: the run of the ordered selection this pleat takes, the
 *  two points of the departure line included. An empty region — a line whose points are not in the selection,
 *  which only a file edited by hand can hold — is marked by a negative @a start, and the pleat then moves nothing.
 *
 *  @a normal is the unit normal of the departure line, oriented towards the fabric of that region, and @a step is
 *  one pleat of the line: twice its value, along @a normal. @a firstPleat is the index of its first pleat among
 *  all the pleats of the operation, which is what the marks and the fold lines are numbered by. */
struct OPleatPlace
{
    QPointF normal{};
    QPointF step{};
    int pleats{1};
    int firstPleat{0};
    int start{-1};
    int last{-1};
};

/**
 * @brief One mark: the point that materializes an end of a pleat, and the edge the cap was read on.
 *
 * @a departure is the departure line the pleat was taken on, @a pleat is the index of that pleat among all the
 * pleats of the operation, and @a end is 0 for the first point of the departure line and 1 for the other.
 * @a flat means the edge is parallel to the move: the mark is the centre alone, and the V has nothing to draw —
 * that is the case of a hem, and it is a case in its own right, not a degenerate cap.
 *
 * @a point is the apex of the V: where the edge crosses the fold line. When a **curve** carries the edge there, the
 * two sides of the cap follow that curve and its image by the fold, and @a edgeId, @a length and @a fromStart are
 * what lets the figure cut them (see OPleatCapRule). They stay empty otherwise — a line carries the edge, and the
 * two sides are then the straight tracings, which are exact on a straight edge — and the apex is the one the cap
 * rule found on the tangent.
 */
struct OPleatMark
{
    int departure{0};
    int pleat{0};
    int end{0};
    bool flat{false};
    QPointF point{};

    /** @brief The curve the edge was read on, or NULL_ID when a line carries it. */
    quint32 edgeId{NULL_ID};

    /** @brief Where the apex falls on that curve, measured from its start the way the cut tools measure. */
    qreal length{0.};

    /** @brief Whether the point of the departure line is the start of that curve. */
    bool fromStart{false};

    /** @brief The place of the departure line the cap is drawn at: its own place when the pleat follows the edge as
     *  drawn, the one the move carried it to when it follows the edge the move carries — the two sides of the V then
     *  start from the two places, and the order of the two is what tells one sense from the other. */
    int place{0};
};

/**
 * @brief The notch of one end of one pleat: the mark the fitter reads on the piece's edge.
 *
 * Once the fabric is folded and laid down, the edge the fold was taken on comes back onto the piece's outline a
 * value away from the line the pleat is measured on. That point is what a notch marks, so that the pleat can be
 * laid the right way round and its depth checked — and it is the reason it is read per **sense**: the two senses
 * send the folded edge to two different places, and each place has its own mark.
 *
 * The line the value is counted from is the one the sense designates — the pleat's own departure line when it
 * follows the edge as drawn, its copy when it follows the edge the move carries. The value is then walked **along
 * the piece's edge**, never along the move, and on the side the fold leaves free.
 *
 * @a point is where the notch stands in the pattern. @a edgeId is the curve the notch cuts when a curve carries
 * the edge there — the cut is what makes the notch a point *of* that curve, and what lets the piece be built
 * through it later — and NULL_ID when a plain segment carries it, the notch being then a point and nothing else.
 * @a length is where the cut falls on that curve, measured from its start the way the cut tools measure it: a
 * notch placed on an edge that runs towards the point is at its total length less the value.
 */
struct OPleatNotch
{
    int departure{0};
    int pleat{0};
    int end{0};
    QPointF point{};
    quint32 edgeId{NULL_ID};
    qreal length{0.};
};

/**
 * @brief What the pleats do, as the rule resolves them.
 *
 * @a places holds one entry per departure line, **in the order the user gave them**, and @a marks the resolved
 * ends, pleat after pleat and, in each pleat, first end then second: the fold line of a pleat joins its two
 * marks.
 *
 * @a moves is what every object of the selection travels: the sum of the steps of the pleats whose region holds
 * it, keyed by the object's own id. An object in no region — outside every pair — does not travel at all. Two
 * pleats taken on two different lines compose as any two translations do, and the order of the departure lines
 * changes nothing.
 */
struct OPleatPlan
{
    OPleatFailure failure{OPleatFailure::None};
    QVector<OPleatPlace> places{};

    /** The ordered selection the rule read, which is what tells where a region begins and ends. */
    QVector<quint32> sources{};

    /** What each object of the selection travels, by its own id. */
    QHash<quint32, QPointF> moves{};

    QVector<OPleatMark> marks{};

    /** @brief One entry per notch the pleats ask for, pleat after pleat and, in each pleat, first end then
     *  second: a pleat the user left without a notch holds none. */
    QVector<OPleatNotch> notches{};

    /** @brief What one object of the selection travels, or a null vector when no pleat takes it. */
    auto Move(quint32 objectId) const -> QPointF;

    /** @brief The mark of one end of one pleat, or nullptr when the plan holds none. */
    auto Mark(int pleat, int end) const -> const OPleatMark *;

    /** @brief The notch of one end of one pleat, or nullptr when that end asks for none. */
    auto Notch(int pleat, int end) const -> const OPleatNotch *;

    /** @brief Where one end of a departure line is, at one of the places that line is drawn at.
     *
     *  @a line is 0 for the line once the pleats before it have carried it, 1 for its first copy, and so on: a
     *  pleat of that departure is laid between two consecutive places. The answer is the carried point of the
     *  line — what the other pleats did to it — plus its own step, one place at a time. */
    auto Place(const OPleatEnd &prise, int departure, int line) const -> QPointF;

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
    /** @brief The side of a departure line the fabric of a region is on.
     *
     *  Only the sources that are points are read, and the two points of the line are left out: they lie on the
     *  line, so they carry no side. Fabric on both sides — or none of it on either side — leaves the side
     *  undetermined, and the tool then refuses rather than folding a pleat the wrong way.
     *
     *  @a region is what is read, and it is the run of the ordered selection the pleat takes (see OPleatPlace). */
    static auto SideOf(const QVector<SourceItem> &region, quint32 firstId, quint32 secondId,
                       const VContainer *data) -> OPleatSide;

    /** @brief The plan of the pleats: the region of each one, and what every object travels.
     *
     *  @a values holds the value of each departure line, in pixels and in the same order as @a departures; the
     *  caller evaluates the formulas, since a formula is the tool's business and not the rule's. @a source is the
     *  ordered selection: its order is the contract, and the region of a pleat is the run that lies between the
     *  two points of its line. */
    static auto Compose(const QVector<OPleatDeparture> &departures, const QVector<qreal> &values,
                        const QVector<SourceItem> &source, const VContainer *data) -> OPleatPlan;

private:
    Q_DISABLE_COPY_MOVE(OPleatRule) // NOLINT
    OPleatRule() = default;
};

#endif // OPLEATRULE_H
