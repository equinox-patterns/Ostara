/************************************************************************
 **
 **  @file   odartcapedge.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/10/2026
 **
 **  @brief  the line of the pattern a dart's cap prolongs, read at a point of its opening
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

#ifndef ODARTCAPEDGE_H
#define ODARTCAPEDGE_H

#include <QPointF>
#include <QtGlobal>

class VContainer;
class VPatternGraph;

//! The line of the pattern a dart's cap prolongs, read at one point of its opening.
//!
//! The cap is C = intersection( the line the opening sits on , the fold line ), so the tool has to know which
//! line the piece's edge follows at the opening point. It cannot be asked of the tool that MADE the point: a
//! point of an opening is very often a copy, an intersection, a point alone — nothing that says what it lies on.
//! It is therefore asked of the pattern itself, in two questions that complete each other:
//!
//!  1. **what passes through the point** — every segment the container records (a `<line>`, a side of a rectangle,
//!     the pair a point of intersection registers) and every segment a tool draws (the base of a point on a line,
//!     which no line of the container holds);
//!  2. **what made it** — when nothing passes through it, the sources its own tool consumed: a point built on a
//!     line names that line, drawn or not.
//!
//! Two lines of the pattern are never the edge: the OPENING itself (through the two points of the opening) and
//! the ARM of the dart (through the point and its apex). What is left is the edge when it is a single line; when
//! more than one is left, nothing decides, and the caller asks the user for a point — the way out the dialog
//! already offers.
//!
//! The piece is deliberately not read: a cap is drawn before the piece exists, and a figure that changed its mind
//! the day the piece is created would move under the user's hands.
class ODartCapEdge
{
public:
    /** @brief Which way the edge runs at @a pointId, or a null vector when no single line answers.
     *
     *  @a apexId is the point of the dart (NULL_ID when unknown): the segment joining the point to it is the arm,
     *  never the edge. @a otherPointId is the other point of the opening (NULL_ID when unknown): the line through
     *  the two is the opening, never the edge either. @a ownerId is the tool whose own figure must be ignored
     *  (NULL_ID when none): the sides of a cap are lines of the pattern like any other, and the cap must not read
     *  itself. @a graph may be null — the second question is then simply not asked. The answer is a direction,
     *  never an orientation: the caller reads a line. */
    static auto Direction(quint32 pointId, quint32 apexId, quint32 otherPointId, quint32 ownerId,
                          const VContainer *data, const VPatternGraph *graph) -> QPointF;

private:
    Q_DISABLE_COPY_MOVE(ODartCapEdge) // NOLINT
    ODartCapEdge() = default;
};

#endif // ODARTCAPEDGE_H
