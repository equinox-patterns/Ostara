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
        NoIntersection     //!< the line to extend and the axis are parallel, or confounded
    };

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
};

#endif // ODARTCAPRULE_H
