/************************************************************************
 **
 **  @file   ocornertrace.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28/09/2026
 **
 **  @brief  A dash pattern restarts at every corner of the trace.
 **
 **          Two members of a piece group are laid on one another, so the stretch they have in common
 **          is traced twice, once by each, with the same style. A dash pattern runs along a contour
 **          from wherever it starts: the two pieces start theirs at different places, so on the
 **          shared stretch the dashes of one fall in the gaps of the other and the eye reads a plain
 **          line instead of a dashed one.
 **
 **          Nothing has to be taken away. A dash pattern restarts at every sub-path (Qt does, and so
 **          does the SVG), so cutting the trace at its own CORNERS — the angles of the drawn line,
 **          not the pattern's construction points — gives every stretch between two corners its own
 **          pattern, starting at the corner it leaves. Two traces that lie on each other have the
 **          same corners and the same lengths between them: their dashes then coincide, exactly.
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

#ifndef OCORNERTRACE_H
#define OCORNERTRACE_H

#include <QPainterPath>
#include <QVector>
#include <QtGlobal>

#include <optional>

class QGraphicsPathItem;

/**
 * @brief Where a trace restarts its dash pattern: at every corner of the line itself.
 */
namespace OCornerTrace
{

/**
 * @brief Cut every sub-path of @p path at its corners, so each stretch carries its own dash pattern
 *        from the corner it leaves.
 *
 * A pure re-parameterisation of what is drawn: the shape is the same, edge for edge, and only the
 * place where each dash pattern begins changes. Nothing is added, nothing is taken away.
 *
 * @param turnDegrees how sharp a turn has to be to count as a corner. The flattening of a curve
 *                    turns by a fraction of a degree at every sample; a real corner turns much more.
 */
auto CutAtCorners(const QPainterPath &path, qreal turnDegrees = 10.0) -> std::optional<QPainterPath>;

/**
 * @brief The same cut, applied to the contour an item draws.
 */
void RestartAtCorners(QGraphicsPathItem *item, qreal turnDegrees = 10.0);

/**
 * @brief Cut each member of a group at the one place that puts both patterns back in step.
 *
 * Two members of a group are laid on one another and share stretches of their outline. Each trace
 * runs its dash pattern from wherever its own contour began, and the two contours do not begin in the
 * same place: on a shared stretch the dashes of one fall in the gaps of the other, and the eye reads a
 * plain line where a dashed one is meant.
 *
 * Cutting the trace makes a dash pattern start again (Qt restarts it at every sub-path, and so does
 * the SVG). But a cut is not free: where it falls in a gap, the two dashes on either side of it are
 * soldered into one, so the more cuts, the more solid places — which is why cutting everywhere fixes
 * one stretch and spoils another.
 *
 * What is needed is the least possible: one cut at the START of each shared stretch, at the same
 * point of the outline on both members. From there the two traces draw the same line over the same
 * length — measured: the difference between their two lengths is constant along a shared stretch —
 * so both patterns stay in step to the end of that stretch, and nothing else on either trace moves.
 *
 * @return the places where a pattern was made to start again, so a caller can tell how much the
 *         members have in common. Empty when there is nothing to align.
 */
auto RestartAtSharedStretches(const QVector<QGraphicsPathItem *> &members) -> QVector<QPointF>;

} // namespace OCornerTrace

#endif // OCORNERTRACE_H
