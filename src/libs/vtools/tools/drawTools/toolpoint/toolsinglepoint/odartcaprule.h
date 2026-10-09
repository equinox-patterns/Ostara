/************************************************************************
 **
 **  @file   odartcaprule.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  the cap of a dart: the rule, written once
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

#ifndef ODARTCAPRULE_H
#define ODARTCAPRULE_H

#include <QPointF>

//! The cap of a dart, in one place.
//!
//! A dart is the triangle P1-P-P2, isosceles by trade: P is its point and P1P2 its opening. Capping it
//! means adding the point C of the triangle P1-C-P2, isosceles on the same opening, so that the dart
//! becomes a shape pointed at both ends.
//!
//! Equal arms alone do not place C: every point of the perpendicular bisector of P1P2 has them, and the
//! axis (P, midpoint of P1P2) is that very bisector when the dart is isosceles. What places C is the line
//! of the pattern that the opening sits on — the shoulder line, the side curve — carried by the point of
//! the line to extend:
//!
//!     C = intersection( (point of the line to extend, the closer of P1/P2) , (P, midpoint of P1P2) )
//!
//! The rule lives here and nowhere else: the dialog refuses with it, the tool builds with it, and the two
//! therefore cannot disagree about what a dart cap is.
//!
//! A dart is a pleat whose copy is already there: the two points of the opening are the point and its own
//! copy, given by the user instead of being carried there by a move. So nothing is created to build the cap,
//! and the line to extend is not a point the user has to click any more: it is the edge the pattern carries
//! at one end of the opening — read by OEdgeReader, then turned into a point of that line here, so that the
//! dialog, the tool and the preview all build the same cap from the same input. A point the user designates
//! stays the way out when no edge can be read.
class ODartCapRule
{
public:
    //! Why a cap point cannot be built, from the input to the construction.
    enum class Failure
    {
        None,              //!< the cap point exists and was written to the output
        DegenerateBase,    //!< P1 and P2 are the same point: no opening, no midpoint, no axis
        DegenerateApex,    //!< the point of the dart sits on the midpoint: the axis has no direction
        DegenerateExtend,  //!< the point of the line to extend is the base point it pairs with
        NoIntersection,    //!< the line to extend and the axis are parallel, or confounded
        NoEdge             //!< nothing was read at that end of the opening: the user has to designate a point
    };

    /** @brief How far along the edge the point of the line to extend is placed: a fraction of the opening,
     *  small enough that the line through it and its base point **is** the edge. */
    static constexpr qreal CarrierFraction = 1e-3;

    //! Whether the failure means the cap point could not be built.
    static auto IsFailure(Failure failure) -> bool;

    //! The middle of the opening, the point the axis goes through.
    static auto Midpoint(const QPointF &firstPoint, const QPointF &secondPoint) -> QPointF;

    //! The base point the line to extend is carried by: 0 for the first point, 1 for the second. On an
    //! exact tie the first point wins — with an isosceles dart both give the same cap point anyway.
    static auto NearestBasePoint(const QPointF &extendPoint, const QPointF &firstPoint,
                                 const QPointF &secondPoint) -> int;

    //! The cap point of the dart P1-P-P2, given the point of the line to extend. Written to capPoint only
    //! when the result is Failure::None.
    static auto CapPoint(const QPointF &apex, const QPointF &firstPoint, const QPointF &secondPoint,
                         const QPointF &extendPoint, QPointF *capPoint) -> Failure;

    /** @brief A point of the line an edge carries, built from the direction read at @a basePoint.
     *
     *  The line to extend is described by a point of it and its direction: the point is taken a fraction of
     *  the opening along that direction from the base point the edge was read at, and that is what makes the
     *  rule anchor the line on **that** base point rather than on the other one. A null answer means the edge
     *  carries no direction at all — nothing was read — and the caller then asks the user for a point. */
    static auto CarrierPoint(const QPointF &basePoint, const QPointF &edgeDirection, qreal opening) -> QPointF;

    /** @brief The cap point of the dart P1-P-P2, read on the edge the pattern carries at one end of the
     *  opening — the way the pleat builds its own caps.
     *
     *  @a baseIndex is 0 for the first point of the opening, 1 for the second: it is the end the edge is read
     *  at, which is what tells one sense from the other. @a edgeDirection is the direction OEdgeReader answered
     *  there, and it may be null: the answer is then Failure::NoEdge, and nothing is written. */
    static auto CapPointFromEdge(const QPointF &apex, const QPointF &firstPoint, const QPointF &secondPoint,
                                 int baseIndex, const QPointF &edgeDirection, QPointF *capPoint) -> Failure;

    /** @brief The value of the crank: **half the opening**.
     *
     *  A pleat folds its fabric over its own value, so the copy the move lays down stands **two** values away, and
     *  its notch is counted **one** value along the edge from the point. A dart is that very pleat — its two points
     *  of the opening are the departure line and the copy the move already made — so the value of its crank is half
     *  the opening. Measured on the pleat: `|copy - departure| / 2` is its own `value` to the last digit. */
    static auto CrankValue(const QPointF &firstPoint, const QPointF &secondPoint) -> qreal;

    /** @brief The point the crank is counted from: **the same** point of the opening the cap reads.
     *
     *  The two senses of a dart cap name the two curves that meet the mouth of the dart — the piece of the outline
     *  before the first point of the opening, and the curve the copy wears after the second one — and each crank
     *  belongs to the curve of its own sense: it is counted from the point that sense reads, so that it stands ON
     *  that curve. Counting it from the other point — the pleat's own rule, which measures its notch on the far side
     *  of its cap — is what made the crank of one sense fall where the other one belongs, and left both of them
     *  beside the edge instead of on it. 0 for the first point of the opening, 1 for the second. */
    static auto CrankBaseIndex(bool secondSense) -> int;
};

#endif // ODARTCAPRULE_H
