/************************************************************************
 **
 **  @file   opleatnotch.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/10/2026
 **
 **  @brief  Where a notch falls on the piece's edge, and the two halves the edge is cut into there.
 **
 **          A notch is the mark a fitter reads on the edge of a piece: the point the folded edge of a pleat comes
 **          back onto the outline once the fabric is laid down. It is placed by walking along the edge itself — a
 **          value away from the line the pleat is measured on — and never along the move, because what it marks is
 **          a place on the outline.
 **
 **          On a straight edge that is a point and nothing else. On a curve it is a **cut**: the edge is split in
 **          two there, exactly as the cut tools split one, so that the point belongs to the curve and the piece the
 **          user builds later can be traced through it. The two halves take the names their ends carry, which is
 **          what makes them read as the pieces of that very edge.
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

#ifndef OPLEATNOTCH_H
#define OPLEATNOTCH_H

#include <QPointF>
#include <QSharedPointer>
#include <QString>
#include <QtGlobal>

#include "../../vgeometry/vabstractcurve.h"

class VContainer;
class VPointF;

/**
 * @brief Where a notch stands on the edge it marks.
 *
 * @a point is the place itself. @a length is where it falls on the curve that carries the edge, measured from that
 * curve's start the way the cut tools measure a cut: the length is negative when the walk runs past the far end of
 * the edge, which is a notch the pleat cannot have.
 */
struct OPleatNotchPlacement
{
    QPointF point{};
    qreal length{0.};
};

/**
 * @brief The two halves a notch makes of the edge it cuts, and the names their ends give them.
 *
 * Both are empty when the curve is of a kind no cut can split, and the caller then has nothing to place.
 */
struct OPleatNotchHalves
{
    QSharedPointer<VAbstractCurve> first{};
    QSharedPointer<VAbstractCurve> second{};
    QString firstName{};
    QString secondName{};
};

/**
 * @brief The rule of a notch: the walk along the edge, and the cut it asks for.
 *
 * No state: every method is a reading or a computation, and none of them touches the pattern.
 */
class OPleatNotchRule
{
public:
    /** @brief Where a notch stands on @a curve when it is walked from @a from, a value away.
     *
     *  @a from is an end of that curve, and the walk goes from it **into** the curve: the answer is the value itself
     *  when that end is the curve's start, and the curve's length less the value when it is its end — which is the
     *  very arithmetic the cut tools ask of their user, said in the curve's own terms.
     *
     *  A negative @a length in the answer means the walk runs past the other end of the edge: the edge is shorter
     *  than the value, and no notch of that pleat lies on it. */
    static auto Place(const QSharedPointer<VAbstractCurve> &curve, const QPointF &from, qreal value)
        -> OPleatNotchPlacement;

    /** @brief Split @a curve at @a length, and name each half after the two ends it joins.
     *
     *  @a notchName is the name of the notch, which is the point the two halves share: the first half joins the
     *  start of the curve to it, the second one it to the end of the curve, exactly as a cut names its pieces.
     *
     *  @a startName and @a endName are the names of the points the curve runs between, such as the pattern shows
     *  them. The curve handed over here is the one the operation carried, whose own ends are the control points of a
     *  moved spline — `X1` and `X4` — and a half named after those would answer to names no point of the pattern
     *  bears. A name left empty is one the curve cannot give: an arc is read from its centre, and each half then
     *  keeps the name its own end gives it. */
    static auto Cut(const QSharedPointer<VAbstractCurve> &curve, qreal length, const QString &notchName,
                    const QString &startName, const QString &endName) -> OPleatNotchHalves;

private:
    Q_DISABLE_COPY_MOVE(OPleatNotchRule) // NOLINT
    OPleatNotchRule() = default;
};

#endif // OPLEATNOTCH_H
