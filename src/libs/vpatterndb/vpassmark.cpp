/************************************************************************
 **
 **  @file   vpassmark.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   1 5, 2019
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2019 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 20/09/2026.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include <QPainterPath>

#include "../ifc/exception/vexceptioninvalidnotch.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/varc.h"
#include "../vlayout/vrawsapoint.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/testpath.h" // do not remove
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/testpassmark.h" // do not remove
#include "vgeometrydef.h"
#include "vpassmark.h"

#include <QJsonObject>
#include <QtMath>

const qreal VPassmark::passmarkRadiusFactor = 0.45;

namespace
{
const qreal passmarkGap = MmToPixel(1.5);

//---------------------------------------------------------------------------------------------------------------------
auto GetSeamPassmarkSAPoint(const VPiecePassmarkData &passmarkData, const QVector<QPointF> &seamAllowance,
                            QPointF &point) -> PassmarkStatus
{
    bool needRollback = false; // no need for rollback
    QVector<VRawSAPoint> ekvPoints;
    ekvPoints = VAbstractPiece::EkvPoint(ekvPoints,
                                         passmarkData.previousSAPoint,
                                         passmarkData.passmarkSAPoint,
                                         passmarkData.nextSAPoint,
                                         passmarkData.passmarkSAPoint,
                                         passmarkData.saWidth,
                                         &needRollback);

    if (ekvPoints.isEmpty() && needRollback && not seamAllowance.isEmpty())
    {
        ekvPoints += VRawSAPoint(seamAllowance.at(seamAllowance.size() - 1));
    }

    if (ekvPoints.isEmpty())
    {                                 // Just in case
        return PassmarkStatus::Error; // Something wrong
    }

    // The corner of the allowance comes first, in every case. A rollback says where the allowance contour has to
    // start so it can be walked from this notch — it says nothing about the shape of the joint, and taking the last
    // point of the contour as the notch's point put it anywhere on the outline instead of on the corner. That is
    // why the allowance notch of a rollback joint was not the bisector of the corner.
    {
        const QLineF beforeMargin = VAbstractPiece::ParallelLine(passmarkData.previousSAPoint,
                                                                 passmarkData.passmarkSAPoint,
                                                                 passmarkData.saWidth);
        const QLineF afterMargin = VAbstractPiece::ParallelLine(passmarkData.passmarkSAPoint,
                                                                passmarkData.nextSAPoint,
                                                                passmarkData.saWidth);

        QPointF corner;
        if (beforeMargin.intersects(afterMargin, &corner) != QLineF::NoIntersection)
        {
            // A true zero on one side changes what the corner of the allowance is. With two margins the allowance
            // turns at the point where the two offset lines meet — that is the corner. With one margin at zero there
            // is no such turn: the allowance leaves the seam line by a step, and the two offset lines meet *past*
            // that step, off the contour. The corner is then the far end of the step, which EkvPoint already hands
            // back at exactly one margin from the node — the same point the intersection notches pick, and the one
            // that reads as a real corner on screen (client pattern: this is what made A42 and A47 bisectors start
            // beside the corner instead of on it).
            const bool beforeZero =
                qFuzzyIsNull(passmarkData.passmarkSAPoint.GetSABefore(passmarkData.saWidth));
            const bool afterZero = qFuzzyIsNull(passmarkData.passmarkSAPoint.GetSAAfter(passmarkData.saWidth));

            if (beforeZero != afterZero)
            {
                const qreal width = beforeZero ? passmarkData.passmarkSAPoint.GetSAAfter(passmarkData.saWidth)
                                               : passmarkData.passmarkSAPoint.GetSABefore(passmarkData.saWidth);
                const QPointF node = passmarkData.passmarkSAPoint.ToQPointF();

                qreal best = -1;
                for (const auto &ekvPoint : std::as_const(ekvPoints))
                {
                    const QPointF candidate = ekvPoint.ToQPointF();
                    const qreal distance = QLineF(node, candidate).length();
                    if (distance <= accuracyPointOnLine)
                    {
                        continue;
                    }

                    const qreal delta = qAbs(distance - width);
                    if (best < 0 || delta < best)
                    {
                        best = delta;
                        point = candidate;
                    }
                }

                // Only accept a real step end: it stands exactly one margin from the node. If EkvPoint offers no
                // such point, the joint has no step after all and the two offset lines give the corner.
                if (best >= 0 && best <= accuracyPointOnLine)
                {
                    return needRollback ? PassmarkStatus::Rollback : PassmarkStatus::Common;
                }
            }

            point = corner;
            return needRollback ? PassmarkStatus::Rollback : PassmarkStatus::Common;
        }
    }

    if (ekvPoints.size() > 1)
    {
        // The notch stands at the corner of the allowance: the one point both margins agree on. Each margin is a
        // line parallel to its own edge, and the corner is where those two lines meet — nothing else. EkvPoint also
        // hands back the two feet (one allowance from the node on either edge), and anchoring on a foot instead of
        // the corner is what threw the bisector off by tens of degrees as soon as the corner was not square.
        const QLineF beforeMargin = VAbstractPiece::ParallelLine(passmarkData.previousSAPoint,
                                                                 passmarkData.passmarkSAPoint,
                                                                 passmarkData.saWidth);
        const QLineF afterMargin = VAbstractPiece::ParallelLine(passmarkData.passmarkSAPoint,
                                                                passmarkData.nextSAPoint,
                                                                passmarkData.saWidth);

        QPointF corner;
        const auto intersection = beforeMargin.intersects(afterMargin, &corner);
        if (intersection != QLineF::NoIntersection)
        {
            point = corner;
        }
        else
        {
            // Parallel margins: the joint is straight, so any foot is as good as another. Take the farthest one,
            // which is the one the allowance actually reaches.
            point = ekvPoints.constFirst().ToQPointF();
            qreal best = QLineF(passmarkData.passmarkSAPoint, point).length();
            for (const auto &ekvPoint : std::as_const(ekvPoints))
            {
                const QPointF candidate = ekvPoint.ToQPointF();
                const qreal distance = QLineF(passmarkData.passmarkSAPoint, candidate).length();
                if (distance > best)
                {
                    point = candidate;
                    best = distance;
                }
            }
        }
    }
    else
    {
        point = ekvPoints.constFirst().ToQPointF();
    }


    return needRollback ? PassmarkStatus::Rollback : PassmarkStatus::Common;
}

//---------------------------------------------------------------------------------------------------------------------
/// Direction that cuts into the allowance at a corner of its contour.
///
/// A joint with a true zero margin has no corner where the two margin lines meet: the allowance leaves the seam by a
/// step, and its corner is the end of that step. Drawing the notch from that corner towards the point lays it flat on
/// the contour — along the step itself — instead of cutting into the allowance, which is what made A47, PB2 and OT
/// read as notches on the wrong contour. What that corner offers is its own bisector: the two segments meeting there,
/// taken from the corner, and the direction between them.
auto AllowanceCornerBisector(const QVector<QPointF> &contour, const QPointF &corner,
                                 const VPiecePassmarkData &data) -> QLineF
{
    const qsizetype count = contour.size();
    if (count < 3)
    {
        return {};
    }

    qsizetype vertex = -1;
    qreal vertexDistance = -1;
    for (qsizetype i = 0; i < count; ++i)
    {
        const qreal distance = QLineF(corner, contour.at(i)).length();
        if (vertexDistance < 0 || distance < vertexDistance)
        {
            vertexDistance = distance;
            vertex = i;
        }
    }

    QPointF first;
    QPointF second;
    if (vertexDistance <= accuracyPointOnLine)
    {
        // Walk past the points that sit on the corner itself: a closed contour repeats its first point at the end, and
        // the end of a step can also be listed twice. Taking one of those as a neighbour made the corner unusable and
        // the notch fell back to running along the edge (OT, client pattern).
        qsizetype back = vertex;
        for (qsizetype step = 0; step < count; ++step)
        {
            back = (back - 1 + count) % count;
            if (QLineF(corner, contour.at(back)).length() > accuracyPointOnLine)
            {
                break;
            }
        }

        qsizetype forward = vertex;
        for (qsizetype step = 0; step < count; ++step)
        {
            forward = (forward + 1) % count;
            if (QLineF(corner, contour.at(forward)).length() > accuracyPointOnLine)
            {
                break;
            }
        }

        first = contour.at(back);
        second = contour.at(forward);
    }
    else
    {
        qsizetype best = 0;
        qreal bestDistance = -1;
        for (qsizetype i = 0; i < count; ++i)
        {
            const QPointF &a = contour.at(i);
            const QPointF &b = contour.at((i + 1) % count);
            const QPointF ab = b - a;
            const qreal length2 = QPointF::dotProduct(ab, ab);
            QPointF closest = a;
            if (not qFuzzyIsNull(length2))
            {
                closest = a + qBound(0., QPointF::dotProduct(corner - a, ab) / length2, 1.) * ab;
            }

            const qreal distance = QLineF(corner, closest).length();
            if (bestDistance < 0 || distance < bestDistance)
            {
                bestDistance = distance;
                best = i;
            }
        }
        first = contour.at(best);
        second = contour.at((best + 1) % count);
    }

    const QLineF edge1(corner, first);
    const QLineF edge2(corner, second);
    if (edge1.isNull() || edge2.isNull())
    {
        return {};
    }

    if (qAbs(edge1.angleTo(edge2)) >= 5.)
    {
        QLineF bisector(corner, corner + QPointF(1., 1.));
        bisector.setAngle(edge1.angle() + edge1.angleTo(edge2) / 2.);
        return bisector;
    }

    // The contour runs straight through this corner. A step joint at a square corner does exactly that: the step is
    // the continuation of the side that has no margin, so there is no corner to bisect and running towards the point
    // lays the notch flat on the edge, where it reads like an Intersection notch (OT, client pattern). What a notch
    // does on an edge is cut across it: perpendicular to the side that has no margin, towards the side that has one.
    const QPointF node = data.passmarkSAPoint.ToQPointF();
    const bool afterZero = qFuzzyIsNull(data.passmarkSAPoint.GetSAAfter(data.saWidth));
    const QLineF zeroSide(node, afterZero ? data.nextSAPoint.ToQPointF() : data.previousSAPoint.ToQPointF());
    const QLineF wideSide(node, afterZero ? data.previousSAPoint.ToQPointF() : data.nextSAPoint.ToQPointF());
    if (zeroSide.isNull() || wideSide.isNull())
    {
        return {};
    }

    QLineF perpendicular = zeroSide;
    perpendicular.setAngle(zeroSide.angle() + 90.);
    perpendicular.setLength(1.);
    const QPointF firstNormal = perpendicular.p2() - perpendicular.p1();
    const QPointF secondNormal = perpendicular.p1() - perpendicular.p2();
    const QPointF inward = QPointF::dotProduct(firstNormal, wideSide.p2() - wideSide.p1()) >= 0 ? firstNormal
                                                                                              : secondNormal;
    return {corner, corner + inward};
}

//---------------------------------------------------------------------------------------------------------------------
auto PointsToSegments(const QVector<QPointF> &points) -> QVector<QLineF>
{
    QVector<QLineF> lines;
    if (points.size() >= 2)
    {
        lines.reserve(points.size() - 1);
        for (int i = 0; i < points.size() - 1; ++i)
        {
            auto const segment = QLineF(points.at(i), points.at(i + 1));
            if (segment.length() > 0)
            {
                lines.append(segment);
            }
        }
    }
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto PassmarkLength(const VPiecePassmarkData &passmarkData, qreal width, bool &ok) -> qreal
{
    auto ValidateLength = [&passmarkData](qreal length)
    {
        if (length <= accuracyPointOnLine)
        {
            const QString errorMsg = QObject::tr("Found null notch for point '%1' in piece '%2'. Length is less "
                                                 "than minimal allowed.")
                                         .arg(passmarkData.nodeName, passmarkData.pieceName);
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;

            return false;
        }
        return true;
    };

    qreal length = 0;
    if (not passmarkData.passmarkSAPoint.IsManualPasskmarkLength())
    {
        if (passmarkData.globalPassmarkLength > accuracyPointOnLine)
        {
            ok = true;
            return passmarkData.globalPassmarkLength;
        }

        if (width <= accuracyPointOnLine)
        {
            // No seam allowance (0 or disabled) — the notch cannot be half of nothing. Use a fixed default length
            // instead (0.5 cm), like the reference fork.
            length = MmToPixel(5.0);
        }
        else
        {
            length = qMin(width * VSAPoint::passmarkFactor, VSAPoint::maxPassmarkLength);
        }
        if (not ValidateLength(length))
        {
            ok = false;
            return length;
        }

        ok = true;
        return length;
    }

    length = passmarkData.passmarkSAPoint.GetPasskmarkLength();
    if (not ValidateLength(length))
    {
        ok = false;
        return length;
    }

    ok = true;
    return length;
}

//---------------------------------------------------------------------------------------------------------------------
auto PassmarkAngle(const VPiecePassmarkData &passmarkData, qreal angle) -> qreal
{
    return passmarkData.passmarkSAPoint.IsManualPasskmarkAngle() ? passmarkData.passmarkSAPoint.GetPasskmarkAngle()
                                                                 : angle;
}

//---------------------------------------------------------------------------------------------------------------------
// The width a notch would get if it had none of its own, expressed from the length of its baseline.
// Every mark builder comes through here, so the rule exists in one place and a UI asking for the
// automatic value cannot drift from what is drawn.
auto AutomaticNotchWidth(const VPiecePassmarkData &passmarkData, qreal length) -> qreal
{
    switch (passmarkData.passmarkLineType)
    {
        case PassmarkLineType::TwoLines:
        case PassmarkLineType::ThreeLines:
            return passmarkGap; // A fixed gap, independent of the notch length.
        case PassmarkLineType::TMark:
            return length * 0.75;
        case PassmarkLineType::ExternalVMark:
        case PassmarkLineType::InternalVMark:
        {
            constexpr qreal defAngle = 35;
            return 2. * length * qTan(qDegreesToRadians(defAngle));
        }
        case PassmarkLineType::UMark:
        case PassmarkLineType::BoxMark:
            return length * VPassmark::passmarkRadiusFactor * 2.;
        case PassmarkLineType::CheckMark:
        {
            constexpr qreal defAngle = 45;
            const qreal width = qAbs(length * qTan(qDegreesToRadians(defAngle)));
            // The sign is the drawing side, not a magnitude: opening counter-clockwise stores a
            // negative width, and that sign is what the mark builder branches on.
            return passmarkData.passmarkSAPoint.IsPassmarkClockwiseOpening() ? width : -width;
        }
        case PassmarkLineType::OneLine:
        default:
            return 0; // A single line carries no width.
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto PassmarkWidth(const VPiecePassmarkData &passmarkData, qreal width) -> qreal
{
    auto ValidateWidth = [&passmarkData](qreal width)
    {
        if (qAbs(width) <= accuracyPointOnLine)
        {
            const QString errorMsg =
                QObject::tr("Error: notch for point '%1' in piece '%2'. Width is less than minimal allowed.")
                    .arg(passmarkData.nodeName, passmarkData.pieceName);
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
    };

    if (not passmarkData.passmarkSAPoint.IsManualPasskmarkWidth())
    {
        if (qAbs(passmarkData.globalPassmarkWidth) > accuracyPointOnLine)
        {
            return passmarkData.globalPassmarkWidth;
        }

        ValidateWidth(width);
        return width;
    }

    ValidateWidth(passmarkData.passmarkSAPoint.GetPasskmarkWidth());
    return passmarkData.passmarkSAPoint.GetPasskmarkWidth();
}

//---------------------------------------------------------------------------------------------------------------------
// Among several intersections, pick the closest one that lies in the forward direction of the axis (the closest point
// is not always the one we need). On success stores it in notch and returns true.
auto ForwardNotch(const QVector<QPointF> &points, const QLineF &axis, const QPointF &saPoint, QPointF *notch) -> bool
{
    QMap<qreal, int> forward;

    for (qint32 i = 0; i < points.size(); ++i)
    {
        if (points.at(i) == saPoint)
        { // Always seek unique intersection
            continue;
        }

        if (const QLineF length(saPoint, points.at(i)); qAbs(length.angle() - axis.angle()) < 0.1)
        {
            forward.insert(length.length(), i);
        }
    }

    if (not forward.isEmpty())
    {
        *notch = points.at(forward.first());
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto FixNotchPoint(const QVector<QPointF> &seamAllowance, const VPiecePassmarkData &data, QPointF *notch) -> bool
{
    auto axis = QLineF(data.passmarkSAPoint, *notch);

    if (data.passmarkAngleType == PassmarkAngleType::Straightforward)
    {
        axis.setAngle(PassmarkAngle(data, axis.angle()));
    }

    // Point is on seam allowance
    if (VAbstractCurve::IsPointOnCurve(seamAllowance, axis.p2()))
    { // Fixing distortion
        // **La méthode de Valentina, remise telle quelle.** Le point est déjà sur la marge : on rallonge
        // l'axe et on recoupe. La projection « sur le point le plus proche du contour », que j'avais mise
        // à la place, peut choisir un AUTRE morceau du contour — sur une courbe, le point le plus proche
        // n'est pas forcément celui de l'arête — et le cran partait alors se placer sur le point voisin
        // (R6 se posait comme s'il était sur R5, patron de Sophie).
        *notch = axis.p2();
        axis.setLength(axis.length() + accuracyPointOnLine * 10);
        const QVector<QPointF> points = VAbstractCurve::CurveIntersectLine(seamAllowance, axis);
        if (points.size() == 1)
        {
            *notch = points.constFirst();
        }
        else if (points.size() > 1)
        {
            ForwardNotch(points, axis, data.passmarkSAPoint, notch);
        }
        return true;
    }

    // Point is not on seam allowance
    // La marge est DESSINÉE en polyligne, son coin est CALCULÉ analytiquement : les deux diffèrent de la
    // finesse de l'échantillonnage, une fraction de millimètre. Ce n'est pas une ancre perdue — le contour
    // est juste là — et le rayon de 500 cm part alors du point de couture en rasant l'arête au lieu de la
    // traverser : il ne trouve aucun croisement et le cran disparaissait (`BaDvtterM`, Bas Robe Devant,
    // 32/160 : coin analytique à 0,88 px du coin dessiné, tolérance 0,59). On recale l'ancre tant que
    // l'écart reste sous le demi-millimètre ; au-delà, le rayon de Valentina reste seul juge.
    {
        qreal nearest = -1;
        QPointF nearestPoint;
        for (qsizetype i = 0; i + 1 < seamAllowance.size(); ++i)
        {
            const QPointF a = seamAllowance.at(i);
            const QPointF b = seamAllowance.at(i + 1);
            const QPointF ab = b - a;
            const qreal length2 = QPointF::dotProduct(ab, ab);
            QPointF candidate = a;
            if (not qFuzzyIsNull(length2))
            {
                candidate = a + qBound(0., QPointF::dotProduct(axis.p2() - a, ab) / length2, 1.) * ab;
            }
            const qreal distance = QLineF(axis.p2(), candidate).length();
            if (nearest < 0 || distance < nearest)
            {
                nearest = distance;
                nearestPoint = candidate;
            }
        }

        if (nearest >= 0 && nearest <= MmToPixel(0.5))
        {
            *notch = nearestPoint;
            return true;
        }
    }

    axis.setLength(ToPixel(500, Unit::Cm));
    const QVector<QPointF> points = VAbstractCurve::CurveIntersectLine(seamAllowance, axis);

    if (points.empty())
    {
        return false;
    }

    if (points.size() == 1)
    {
        *notch = points.at(0);
        return true;
    }

    // Closest point is not always want we need. First return point in forward direction if exists.
    return ForwardNotch(points, axis, data.passmarkSAPoint, notch);
}

//---------------------------------------------------------------------------------------------------------------------
auto CreateOnePassmarkLines(const VPiecePassmarkData &passmarkData, const QLineF &line) -> QVector<QLineF>
{
    Q_UNUSED(passmarkData);
    return {line};
}

//---------------------------------------------------------------------------------------------------------------------
auto CreateTwoPassmarkLines(const VPiecePassmarkData &passmarkData, const QLineF &line,
                            const QVector<QPointF> &seamAllowance) -> QVector<QLineF>
{
    const qreal width = PassmarkWidth(passmarkData, AutomaticNotchWidth(passmarkData, line.length()));

    QPointF l1p1;
    {
        QLineF line1 = line;
        line1.setAngle(line1.angle() + 90);
        line1.setLength(width / 2.);
        l1p1 = line1.p2();
    }

    QPointF l2p1;
    {
        QLineF line2 = line;
        line2.setAngle(line2.angle() - 90);
        line2.setLength(width / 2.);
        l2p1 = line2.p2();
    }

    QPointF l1p2;
    {
        auto line1 = QLineF(line.p2(), line.p1());
        line1.setAngle(line1.angle() - 90);
        line1.setLength(width / 2.);
        l1p2 = line1.p2();
    }

    QPointF l2p2;
    {
        auto line2 = QLineF(line.p2(), line.p1());
        line2.setAngle(line2.angle() + 90);
        line2.setLength(width / 2.);
        l2p2 = line2.p2();
    }

    QVector<QLineF> lines;
    QLineF seg = VPassmark::FindIntersection(QLineF(l1p2, l1p1), seamAllowance);
    lines.append(QLineF(seg.p2(), seg.p1()));

    seg = VPassmark::FindIntersection(QLineF(l2p2, l2p1), seamAllowance);
    lines.append(QLineF(seg.p2(), seg.p1()));
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto CreateThreePassmarkLines(const VPiecePassmarkData &passmarkData, const QLineF &line,
                              const QVector<QPointF> &seamAllowance) -> QVector<QLineF>
{
    const qreal width = PassmarkWidth(passmarkData, AutomaticNotchWidth(passmarkData, line.length()));

    QPointF l1p1;
    {
        QLineF line1 = line;
        line1.setAngle(line1.angle() + 90);
        line1.setLength(width / 2.);
        l1p1 = line1.p2();
    }

    QPointF l2p1;
    {
        QLineF line2 = line;
        line2.setAngle(line2.angle() - 90);
        line2.setLength(width / 2.);
        l2p1 = line2.p2();
    }

    QPointF l1p2;
    {
        auto line1 = QLineF(line.p2(), line.p1());
        line1.setAngle(line1.angle() - 90);
        line1.setLength(width / 2.);
        l1p2 = line1.p2();
    }

    QPointF l2p2;
    {
        auto line2 = QLineF(line.p2(), line.p1());
        line2.setAngle(line2.angle() + 90);
        line2.setLength(width / 2.);
        l2p2 = line2.p2();
    }

    QVector<QLineF> lines;
    QLineF seg = VPassmark::FindIntersection(QLineF(l1p2, l1p1), seamAllowance);
    lines.append(QLineF(seg.p2(), seg.p1()));

    lines.append(line);

    seg = VPassmark::FindIntersection(QLineF(l2p2, l2p1), seamAllowance);
    lines.append(QLineF(seg.p2(), seg.p1()));
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto CreateTMarkPassmark(const VPiecePassmarkData &passmarkData, const QLineF &line) -> QVector<QLineF>
{
    const qreal width = PassmarkWidth(passmarkData, AutomaticNotchWidth(passmarkData, line.length()));

    QPointF p1;
    {
        auto tmpLine = QLineF(line.p2(), line.p1());
        tmpLine.setAngle(tmpLine.angle() - 90);
        tmpLine.setLength(width / 2.);
        p1 = tmpLine.p2();
    }

    QPointF p2;
    {
        auto tmpLine = QLineF(line.p2(), line.p1());
        tmpLine.setAngle(tmpLine.angle() + 90);
        tmpLine.setLength(width / 2.);
        p2 = tmpLine.p2();
    }

    return {line, {QLineF(p1, p2).center(), p2}, {p2, p1}};
}

//---------------------------------------------------------------------------------------------------------------------
auto CreateExternalVMarkPassmark(const VPiecePassmarkData &passmarkData, const QLineF &line) -> QVector<QLineF>
{
    const qreal width = PassmarkWidth(passmarkData, AutomaticNotchWidth(passmarkData, line.length()));
    const qreal angle = qRadiansToDegrees(qAtan(qAbs(width) / 2. / line.length()));

    QLineF l1 = line;
    l1.setAngle(l1.angle() - angle);

    QLineF l2 = line;
    l2.setAngle(l2.angle() + angle);

    return {{l1.p2(), l1.p1()}, l2};
}

//---------------------------------------------------------------------------------------------------------------------
auto CreateInternalVMarkPassmark(const VPiecePassmarkData &passmarkData, const QLineF &line,
                                 const QVector<QPointF> &seamAllowance) -> QVector<QLineF>
{
    const qreal width = PassmarkWidth(passmarkData, AutomaticNotchWidth(passmarkData, line.length()));
    const qreal angle = qRadiansToDegrees(qAtan(qAbs(width) / 2. / line.length()));

    auto l1 = QLineF(line.p2(), line.p1());
    l1.setAngle(l1.angle() + angle);
    l1 = VPassmark::FindIntersection(l1, seamAllowance);

    auto l2 = QLineF(line.p2(), line.p1());
    l2.setAngle(l2.angle() - angle);

    return {{l1.p2(), l1.p1()}, VPassmark::FindIntersection(l2, seamAllowance)};
}

//---------------------------------------------------------------------------------------------------------------------
auto CreateUMarkPassmark(const VPiecePassmarkData &passmarkData, const QLineF &line,
                         const QVector<QPointF> &seamAllowance) -> QVector<QLineF>
{
    const qreal width = PassmarkWidth(passmarkData, AutomaticNotchWidth(passmarkData, line.length()));
    const qreal radius = width / 2.;

    QVector<QPointF> points;
    QLineF baseLine = line;

    QPointF l1p1;
    {
        QLineF line = baseLine;
        line.setAngle(line.angle() - 90);
        line.setLength(radius);
        l1p1 = line.p2();
    }

    QPointF l2p1;
    {
        QLineF line = baseLine;
        line.setAngle(line.angle() + 90);
        line.setLength(radius);
        l2p1 = line.p2();
    }

    if (baseLine.length() - radius > accuracyPointOnLine)
    {
        baseLine.setLength(baseLine.length() - radius); // keep defined depth

        QPointF l1p2;
        {
            auto line = QLineF(baseLine.p2(), baseLine.p1());
            line.setAngle(line.angle() + 90);
            line.setLength(radius);
            l1p2 = line.p2();
        }

        QPointF l2p2;
        {
            auto line = QLineF(baseLine.p2(), baseLine.p1());
            line.setAngle(line.angle() - 90);
            line.setLength(radius);
            l2p2 = line.p2();
        }

        QLineF seg = VPassmark::FindIntersection(QLineF(l1p2, l1p1), seamAllowance);
        seg = QLineF(seg.p2(), seg.p1());
        points.append(seg.p1());
        points.append(seg.p2());

        const qreal f1 = QLineF(baseLine.p2(), l1p2).angle();
        const qreal f2 = QLineF(baseLine.p2(), l2p2).angle();
        VArc arc(VPointF(baseLine.p2()), radius, f1, f2);
        arc.SetApproximationScale(VArc::OptimalApproximationScale(radius, f1, f2, 0.3));
        points += arc.GetPoints();

        seg = VPassmark::FindIntersection(QLineF(l2p2, l2p1), seamAllowance);
        points.append(seg.p1());
        points.append(seg.p2());
    }
    else
    {
        const qreal f1 = QLineF(baseLine.p1(), l1p1).angle();
        const qreal f2 = QLineF(baseLine.p1(), l2p1).angle();
        VArc arc(VPointF(baseLine.p1()), radius, f1, f2);
        arc.SetApproximationScale(VArc::OptimalApproximationScale(radius, f1, f2, 0.3));
        points += arc.GetPoints();
    }

    return PointsToSegments(points);
}

//---------------------------------------------------------------------------------------------------------------------
auto CreateBoxMarkPassmark(const VPiecePassmarkData &passmarkData, const QLineF &line,
                           const QVector<QPointF> &seamAllowance) -> QVector<QLineF>
{
    const qreal width = PassmarkWidth(passmarkData, AutomaticNotchWidth(passmarkData, line.length()));

    QPointF l1p1;
    {
        QLineF tmp = line;
        tmp.setAngle(tmp.angle() - 90);
        tmp.setLength(width / 2.);
        l1p1 = tmp.p2();
    }

    QPointF l1p2;
    {
        auto tmp = QLineF(line.p2(), line.p1());
        tmp.setAngle(tmp.angle() + 90);
        tmp.setLength(width / 2.);
        l1p2 = tmp.p2();
    }

    QPointF l2p1;
    {
        QLineF tmp = line;
        tmp.setAngle(tmp.angle() + 90);
        tmp.setLength(width / 2.);
        l2p1 = tmp.p2();
    }

    QPointF l2p2;
    {
        auto tmp = QLineF(line.p2(), line.p1());
        tmp.setAngle(tmp.angle() - 90);
        tmp.setLength(width / 2.);
        l2p2 = tmp.p2();
    }

    QVector<QPointF> points;

    QLineF seg = VPassmark::FindIntersection(QLineF(l1p2, l1p1), seamAllowance);
    points.append(seg.p2());
    points.append(seg.p1());

    seg = VPassmark::FindIntersection(QLineF(l2p2, l2p1), seamAllowance);
    points.append(seg.p1());
    points.append(seg.p2());

    return PointsToSegments(points);
}

//---------------------------------------------------------------------------------------------------------------------
auto CreateCheckMarkPassmark(const VPiecePassmarkData &passmarkData, const QLineF &line,
                             const QVector<QPointF> &seamAllowance) -> QVector<QLineF>
{
    const qreal width = PassmarkWidth(passmarkData, AutomaticNotchWidth(passmarkData, line.length()));
    const qreal angle = qRadiansToDegrees(qAtan(qAbs(width) / line.length()));

    if (width > 0)
    { // clockwise
        QLineF l1(line.p2(), line.p1());
        l1.setAngle(l1.angle() + angle);
        l1 = VPassmark::FindIntersection(l1, seamAllowance);

        return {{l1.p2(), l1.p1()}, {line.p2(), line.p1()}};
    }

    QLineF l2(line.p2(), line.p1());
    l2.setAngle(l2.angle() - angle);
    l2 = VPassmark::FindIntersection(l2, seamAllowance);

    return {line, l2};
}

//---------------------------------------------------------------------------------------------------------------------
auto PassmarkToPath(const QVector<QLineF> &passmark) -> QPainterPath
{
    QPainterPath path;
    if (not passmark.isEmpty())
    {
        for (qint32 i = 0; i < passmark.count(); ++i)
        {
            path.moveTo(passmark.at(i).p1());
            path.lineTo(passmark.at(i).p2());
        }

        path.setFillRule(Qt::WindingFill);
    }
    return path;
}
} // namespace

//------------------------------VPiecePassmarkData---------------------------------------------------------------------
auto VPiecePassmarkData::toJson() const -> QJsonObject
{
    QJsonObject dataObject{
        {"previousSAPoint", previousSAPoint.toJson()},
        {"passmarkSAPoint", passmarkSAPoint.toJson()},
        {"nextSAPoint", nextSAPoint.toJson()},
        {"saWidth", saWidth},
        {"nodeName", nodeName},
        {"pieceName", pieceName},
        {"passmarkLineType", static_cast<int>(passmarkLineType)},
        {"passmarkAngleType", static_cast<int>(passmarkAngleType)},
        {"isMainPathNode", isMainPathNode},
        {"isShowSecondPassmark", isShowSecondPassmark},
        {"passmarkIndex", passmarkIndex},
        {"id", static_cast<qint64>(id)},
        {"globalPassmarkLength", globalPassmarkLength},
        {"globalPassmarkWidth", globalPassmarkWidth},
    };

    return dataObject;
}

//---------------------------------------------------------------------------------------------------------------------
VPassmark::VPassmark(const VPiecePassmarkData &data)
  : m_data(data),
    m_null(false)
{
    // Correct distorsion
    if (IsPointOnLineSegment(m_data.passmarkSAPoint, m_data.previousSAPoint, m_data.nextSAPoint))
    {
        const QPointF p =
            VGObject::CorrectDistortion(m_data.passmarkSAPoint, m_data.previousSAPoint, m_data.nextSAPoint);
        m_data.passmarkSAPoint.setX(p.x());
        m_data.passmarkSAPoint.setY(p.y());
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::FullPassmark(const VPiece &piece, const VContainer *data) const -> QVector<QLineF>
{
    if (m_null)
    {
        return {};
    }

    if (not piece.IsSeamAllowance())
    {
        return BuiltInSAPassmark(piece, data);
    }

    QVector<QLineF> lines;
    lines += SAPassmark(piece, data, PassmarkSide::All);
    if (piece.IsSecondPassmarkOnSeamLine() &&
        (VAbstractApplication::VApp()->Settings()->IsPieceShowMainPath() || not piece.IsHideMainPath()) &&
        m_data.isMainPathNode && m_data.passmarkAngleType != PassmarkAngleType::Intersection &&
        m_data.passmarkAngleType != PassmarkAngleType::IntersectionOnlyLeft &&
        m_data.passmarkAngleType != PassmarkAngleType::IntersectionOnlyRight &&
        m_data.passmarkAngleType != PassmarkAngleType::Intersection2 &&
        m_data.passmarkAngleType != PassmarkAngleType::Intersection2OnlyLeft &&
        m_data.passmarkAngleType != PassmarkAngleType::Intersection2OnlyRight && m_data.isShowSecondPassmark)
    {
        lines += BuiltInSAPassmark(piece, data);
    }
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::SAPassmark(const VPiece &piece, const VContainer *data, PassmarkSide side) const -> QVector<QLineF>
{
    if (m_null)
    {
        return {};
    }

    if (not piece.IsSeamAllowance())
    {
        // No seam allowance — draw passmarks on the main path instead.
        QVector<QPointF> mainPath;
        CastTo(piece.FullMainPathPoints(data), mainPath);
        return SAPassmark(mainPath, mainPath, side);
    }

    // Because rollback cannot be calulated if passmark is not first point in main path we rotate it.
    QVector<QPointF> rotatedSeamAllowance;
    CastTo(piece.FullSeamAllowancePointsWithRotation(data, m_data.passmarkIndex), rotatedSeamAllowance);

    QVector<QPointF> seamAllowance;
    CastTo(piece.FullSeamAllowancePoints(data), seamAllowance);

    // **« Tout droit » : le cran de marge est la PROLONGATION du cran du contour principal.**
    // Même ligne, aucune rotation, aucun déplacement. Deux points comptent et j'avais les deux faux :
    //   • le croisement retenu est celui qui est DEVANT le point, dans le sens sortant de la
    //     prolongation — pas celui qui se trouve derrière, sur le contour opposé ;
    //   • le cran renvoyé est COURT (sa longueur de cran), pas le segment entier du point au contour,
    //     qui barrait la pièce d'un trait.
    if (m_data.passmarkAngleType == PassmarkAngleType::Straightforward)
    {
        const QVector<QLineF> mainLines = BuiltInSAPassmarkBaseLine(piece, piece.SeamMirrorLine(data));
        if (not mainLines.isEmpty())
        {
            const QPointF node = m_data.passmarkSAPoint.ToQPointF();
            const QLineF main = mainLines.constFirst();
            // Sens SORTANT : on part du cran du contour principal et on prolonge vers l'extérieur.
            const QPointF outward = main.p1() - main.p2();
            QLineF ray(node, node + outward);
            ray.setLength(ToPixel(500, Unit::Cm));

            const qreal marginWidth = m_data.passmarkSAPoint.MaxLocalSA(m_data.saWidth);
            qreal best = -1;
            QPointF anchor;
            for (const QPointF &candidate : VAbstractCurve::CurveIntersectLine(seamAllowance, ray))
            {
                const QPointF v = candidate - node;
                if (QPointF::dotProduct(v, outward) <= 0.)
                {
                    continue; // derrière le point : ce n'est pas le bord de ce côté
                }
                const qreal distance = std::hypot(v.x(), v.y());
                if (distance <= accuracyPointOnLine)
                {
                    continue;
                }
                const qreal delta = qAbs(distance - marginWidth);
                if (best < 0 || delta < best)
                {
                    best = delta;
                    anchor = candidate;
                }
            }

            if (best >= 0)
            {
                bool ok = false;
                const qreal length = PassmarkLength(m_data, marginWidth, ok);
                if (ok)
                {
                    QLineF notch(anchor, node);
                    notch.setLength(length);
                    return {notch};
                }
            }
        }
    }

    return SAPassmark(seamAllowance, rotatedSeamAllowance, side);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::SAPassmark(const QVector<QPointF> &seamAllowance, const QVector<QPointF> &rotatedSeamAllowance,
                           PassmarkSide side) const -> QVector<QLineF>
{
    if (m_null)
    {
        return {};
    }

    // DumpVector(seamAllowance, QStringLiteral("seamAllowance.json.XXXXXX")); // Uncomment for dumping test data
    // DumpVector(rotatedSeamAllowance,
    //            QStringLiteral("rotatedSeamAllowance.json.XXXXXX"));       // Uncomment for dumping test data
    // DumpPassmarkData(m_data, QStringLiteral("passmarkData.json.XXXXXX")); // Uncomment for dumping test data

    QVector<QLineF> lines = SAPassmarkBaseLine(seamAllowance, rotatedSeamAllowance, side);
    {
        QLineF e1(m_data.passmarkSAPoint.ToQPointF(), m_data.previousSAPoint.ToQPointF());
        QLineF e2(m_data.passmarkSAPoint.ToQPointF(), m_data.nextSAPoint.ToQPointF());
    }


    if (lines.isEmpty())
    {
        return lines;
    }

    lines = CreatePassmarkLines(lines, seamAllowance, side);
    // DumpPassmarkShape(lines, QStringLiteral("passmarkShape.json.XXXXXX")); // Uncomment for dumping test data
    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::IsNull() const -> bool
{
    return m_null;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::Data() const -> VPiecePassmarkData
{
    return m_data;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::AutomaticLength() const -> qreal
{
    if (m_null)
    {
        return 0;
    }

    VPiecePassmarkData data = m_data;
    data.passmarkSAPoint.SetManualPasskmarkLength(false);

    bool ok = false;
    const qreal length = PassmarkLength(data, data.passmarkSAPoint.MaxLocalSA(data.saWidth), ok);
    return ok ? length : 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::AutomaticWidth() const -> qreal
{
    if (m_null)
    {
        return 0;
    }

    VPiecePassmarkData data = m_data;
    data.passmarkSAPoint.SetManualPasskmarkWidth(false);

    // The width follows the length in effect — the baseline the mark builders work on is the resolved one,
    // so this one has to be too. A manual length therefore moves the automatic width, as it does on screen.
    bool ok = false;
    const qreal length = PassmarkLength(data, data.passmarkSAPoint.MaxLocalSA(data.saWidth), ok);

    return AutomaticNotchWidth(data, ok ? length : 0);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::AutomaticAngle(const VPiece &piece, const VContainer *data) const -> qreal
{
    if (m_null)
    {
        return 0;
    }

    VPiecePassmarkData masked = m_data;
    masked.passmarkSAPoint.SetManualPasskmarkAngle(false);

    // Same dispatch as FullPassmark: with the seam allowance disabled the notch is drawn on the main path.
    const VPassmark passmark(masked);
    const QVector<QLineF> baseLine = piece.IsSeamAllowance()
                                         ? passmark.SAPassmarkBaseLine(piece, data, PassmarkSide::All)
                                         : passmark.BuiltInSAPassmarkBaseLine(piece, piece.SeamMirrorLine(data));
    if (baseLine.isEmpty())
    {
        return 0;
    }

    // The baseline runs from the seam-line notch back to the point, while the manual angle is stored as the
    // outward direction — hence the 180° flip, the same convention as BuiltInSAPassmarkBaseLine.
    qreal angle = baseLine.constFirst().angle() + 180.;
    while (angle >= 360.)
    {
        angle -= 360.;
    }
    while (angle < 0.)
    {
        angle += 360.;
    }
    return angle;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::FindIntersection(const QLineF &line, const QVector<QPointF> &seamAllowance) -> QLineF
{
    QLineF testLine = line;
    testLine.setLength(testLine.length() * 10);
    QVector<QPointF> const intersections = VAbstractCurve::CurveIntersectLine(seamAllowance, testLine);
    if (not intersections.isEmpty())
    {
        return {line.p1(), intersections.constLast()};
    }

    return line;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::PassmarkIntersection(const QVector<QPointF> &path, QLineF line, qreal width) const -> QLineF
{
    const QPointF node = m_data.passmarkSAPoint.ToQPointF();

    // A joint with a true zero on one side has no corner where the two margin lines meet: the allowance leaves the
    // seam by a step, and its corner is the end of that step. When this side does carry a margin, that corner is the
    // anchor — the crossing the requested line finds lands just past the step, on the extension of the margin line
    // (A42, client pattern: 38.86 px instead of 37.80, and the notch read as detached from the corner, while the
    // Intersection 2 notch, which happened to land on the step end, was right).
    const bool beforeZero = qFuzzyIsNull(m_data.passmarkSAPoint.GetSABefore(m_data.saWidth));
    const bool afterZero = qFuzzyIsNull(m_data.passmarkSAPoint.GetSAAfter(m_data.saWidth));

    if (beforeZero != afterZero && not qFuzzyIsNull(width))
    {
        QPointF stepEnd;
        if (GetSeamPassmarkSAPoint(m_data, path, stepEnd) != PassmarkStatus::Error)
        {
            // **La direction reste celle de la ligne DEMANDÉE** — celle du côté gauche ou droit choisi
            // par l'utilisateur. La remplacer par la bissectrice du contour écrasait ce choix : un
            // « Intersection droite » sortait en bissectrice (BaDosD', BaDvtter, BaDvtterM sur le
            // patron de Sophie), alors que les mêmes crans avec une marge non nulle étaient justes.
            // Seule l'ANCRE est propre au joint à marge nulle — le bout de la marche, qui est le coin
            // de la marge ; elle reste.
            // **Le sens compte : de la marge vers la couture.** Le cran part du bout de la marche (le
            // coin de la marge) et va VERS le point, donc vers l'intérieur de la pièce — c'est la
            // construction de Valentina (`QLineF(ancre, point)`). Le prendre dans le sens de la ligne
            // demandée le faisait sortir de la pièce (BaDosD', BaDvtter, BaDvtterM).
            QLineF notch(stepEnd, node);

            bool ok = false;
            const qreal length = PassmarkLength(m_data, width, ok);
            if (ok && notch.length() > accuracyPointOnLine)
            {
                notch.setLength(length);
                return notch;
            }
        }
    }

    line.setLength(line.length() * 100); // Hope 100 is enough

    // DumpVector(path, QStringLiteral("points.json.XXXXXX")); // Uncomment for dumping test data

    const QVector<QPointF> intersections = VAbstractCurve::CurveIntersectLine(path, line);

    // DumpVector(intersections, QStringLiteral("intersections.json.XXXXXX")); // Uncomment for dumping test data

    if (not intersections.isEmpty())
    {
        // The notch starts on the allowance edge, one margin away from the node: that distance is the single
        // property the crossing must have. The requested line is infinite, so it crosses the contour anywhere the
        // piece happens to be — the neighbouring node itself, the far end of the edge, or the node again where the
        // allowance runs along the seam. Valentina kept the list's last crossing, which on the client pattern put
        // Col' notch 226 px away at the other end of its edge (it looked like it had disappeared) and dropped PB2's
        // on the node instead of on its allowance. The crossing one margin from the node is the one that belongs to
        // this corner, and it is the only one the allowance edge can offer.
        QPointF anchor;
        qreal bestDelta = -1;
        for (const QPointF &candidate : intersections)
        {
            const qreal delta = qAbs(QLineF(node, candidate).length() - width);
            if (bestDelta < 0 || delta < bestDelta)
            {
                bestDelta = delta;
                anchor = candidate;
            }
        }


        if (QLineF(anchor, node).length() > accuracyPointOnLine)
        {
            line = QLineF(anchor, node);

            bool ok = false;
            const qreal length = PassmarkLength(m_data, width, ok);
            if (not ok)
            {
                return {};
            }
            line.setLength(length);

            return line;
        }

        // The allowance meets the seam line exactly at the node: this side is a true zero, so there is no margin
        // for the notch to cross and the intersection *is* the node. Valentina only ever escaped this through its
        // floor on local widths (minSAWidth, a hundredth of a pixel), which held the allowance a hair away and let
        // the same comparison pass; the true zero keeps it honest, and the notch must not be dropped for it — the
        // mark is still owed to this node.
        //
        // What the notch is missing is the distance the allowance used to give it, not its direction: the line it
        // was asked to stand on still carries that, and the mark keeps it. Start the base line at the node and lay
        // the notch's own length along it, exactly as the built line does on a side that has an allowance, so the
        // crossbar closes it at the same distance and the same angle.
        QLineF unit(line.p1(), line.p2());
        unit.setLength(1.); // A null line keeps its length, so the guard below still holds.
        const QPointF direction = unit.p2() - unit.p1();
        if (not direction.isNull())
        {
            bool ok = false;
            const qreal length = PassmarkLength(m_data, width, ok);
            if (ok)
            {
                return {m_data.passmarkSAPoint, m_data.passmarkSAPoint - direction * length};
            }
        }

        const QString errorMsg =
            QCoreApplication::translate("VPassmark", "Cannot calculate a notch for point '%1' in piece '%2'. Notch "
                                                     "collapse.")
                .arg(m_data.nodeName, m_data.pieceName);
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VExceptionInvalidNotch(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
    }
    else
    {
        const QString errorMsg = QCoreApplication::translate(
                                     "VPassmark", "Cannot calculate a notch for point '%1' in piece '%2'. Cannot find "
                                                  "intersection.")
                                     .arg(m_data.nodeName, m_data.pieceName);
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VExceptionInvalidNotch(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::PassmarkStraightforwardBaseLine(const QPointF &seamPassmarkSAPoint) const -> QVector<QLineF>
{
    bool ok = false;
    const qreal length = PassmarkLength(m_data,
                                        m_data.passmarkSAPoint.MaxLocalSA(m_data.saWidth),
                                        ok);

    if (not ok)
    {
        return {};
    }

    auto line = QLineF(seamPassmarkSAPoint, m_data.passmarkSAPoint);

    // ⚠️ **Un joint presque droit n'a pas de croisement de marges exploitable.** Quand les deux arêtes
    // sont quasi parallèles, les deux lignes de marge se croisent très loin : l'ancre partait à 55 px
    // du point là où la marge vaut 37,8 px (R6, Top V2 Devant — le cran se dessinait comme s'il était
    // sur le point voisin, R5). On ramène l'ancre à la distance de la marge, sur la direction trouvée :
    // la direction reste celle du calcul, seule la distance est remise juste.


    if (line.isNull())
    {
        if (m_data.passmarkSAPoint.IsManualPasskmarkAngle())
        {
            line = QLineF(m_data.passmarkSAPoint.x(),
                          m_data.passmarkSAPoint.y(),
                          m_data.passmarkSAPoint.x() + 100,
                          m_data.passmarkSAPoint.y());
            line.setAngle(m_data.passmarkSAPoint.GetPasskmarkAngle() + 180);
        }
        else if (const QLineF seam(m_data.previousSAPoint.ToQPointF(), m_data.nextSAPoint.ToQPointF());
                 not seam.isNull())
        {
            // No seam allowance on either side: the notch has no distance to run along, its base point being
            // the node itself. Take the direction from the seam and draw the notch across it — a marker is
            // exactly that — instead of leaving an invisible notch and asking for a manual angle.
            const QPointF start = m_data.passmarkSAPoint.ToQPointF();
            line = QLineF(start, start + QPointF(100., 100.));
            line.setAngle(seam.angle() - 90.);
        }
        else
        {
            const QString errorMsg = QObject::tr(
                                         "Piece '%1'. Notch collapse for point '%2'. Provide a manual angle value.")
                                         .arg(m_data.pieceName, m_data.nodeName);
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
    }

    line.setLength(length);
    return {line};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::PassmarkBisectorBaseLine(PassmarkStatus seamPassmarkType, const QPointF &seamPassmarkSAPoint,
                                         const QVector<QPointF> &seamAllowance) const -> QVector<QLineF>
{
    QLineF edge1;
    QLineF edge2;

    if (seamPassmarkType == PassmarkStatus::Common)
    {
        // The joint's own edges: from the node towards the points on either side. The margins must not enter the
        // direction — a bisector bisects the joint, and the squaring of the node or the width of the allowance
        // has no say in where the joint is.
        edge1 = QLineF(m_data.passmarkSAPoint.ToQPointF(), m_data.previousSAPoint.ToQPointF());
        edge2 = QLineF(m_data.passmarkSAPoint.ToQPointF(), m_data.nextSAPoint.ToQPointF());
    }
    else if (seamPassmarkType == PassmarkStatus::Rollback)
    {
        // A rollback only says where the allowance contour starts, not what the joint looks like: same edges.
        edge1 = QLineF(m_data.passmarkSAPoint.ToQPointF(), m_data.previousSAPoint.ToQPointF());
        edge2 = QLineF(m_data.passmarkSAPoint.ToQPointF(), m_data.nextSAPoint.ToQPointF());
    }
    else
    { // Should never happen
        return {};
    }

    bool ok = false;
    const qreal length = PassmarkLength(m_data,
                                        m_data.passmarkSAPoint.MaxLocalSA(m_data.saWidth),
                                        ok);
    if (not ok)
    {
        return {};
    }

    // Direction: from the margin's corner back to the point. That is the joint's own bisector whenever both
    // margins are equal — the corner of the allowance then lies on the bisector of the joint — so the notch still
    // bisects the corner, and it is also exactly what "straightforward" builds, which is why the two rules are one
    // and the same on a corner. When the two margins differ, though, the corner sits off that bisector (measured on
    // the client pattern: 52° off for a true zero on one side, enough for the notch to leave the corner sideways and
    // end without ever reaching the seam), and the only direction that keeps the notch on its corner is the one
    // from the corner to the point.
    const qreal bisectorAngle = edge1.angle() + edge1.angleTo(edge2) / 2.;

    const bool beforeZero = qFuzzyIsNull(m_data.passmarkSAPoint.GetSABefore(m_data.saWidth));
    const bool afterZero = qFuzzyIsNull(m_data.passmarkSAPoint.GetSAAfter(m_data.saWidth));

    QLineF notch(seamPassmarkSAPoint, m_data.passmarkSAPoint.ToQPointF());

    if (beforeZero != afterZero)
    {   // A step joint: the corner of the allowance is the end of the step, and the notch has to cut into the
        // allowance from there — not run back along the step towards the point, which lays it flat on the contour.
        if (const QLineF corner = AllowanceCornerBisector(seamAllowance, seamPassmarkSAPoint, m_data); not corner.isNull())
        {
            notch = corner;
        }
    }

    if (notch.length() <= accuracyPointOnLine)
    {   // No allowance on either side: the corner sits on the point itself, so only the joint can give a direction.
        notch = QLineF(seamPassmarkSAPoint, seamPassmarkSAPoint + QPointF(1., 1.));
        notch.setAngle(bisectorAngle);
    }

    notch.setLength(length);


    return {notch};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::PassmarkIntersectionBaseLine(const QVector<QPointF> &path, PassmarkSide side) const -> QVector<QLineF>
{
    QVector<QLineF> lines;

    if ((m_data.passmarkAngleType == PassmarkAngleType::Intersection ||
         m_data.passmarkAngleType == PassmarkAngleType::IntersectionOnlyLeft) &&
        (side == PassmarkSide::All || side == PassmarkSide::Left))
    {
        // first passmark
        if (QLineF const intersection = PassmarkIntersection(path,
                                                             QLineF(m_data.nextSAPoint, m_data.passmarkSAPoint),
                                                             m_data.passmarkSAPoint.GetSABefore(m_data.saWidth));
            !intersection.isNull())
        {
            lines += intersection;
        }
    }

    if ((m_data.passmarkAngleType == PassmarkAngleType::Intersection ||
         m_data.passmarkAngleType == PassmarkAngleType::IntersectionOnlyRight) &&
        (side == PassmarkSide::All || side == PassmarkSide::Right))
    {
        // second passmark
        if (QLineF const intersection = PassmarkIntersection(path,
                                                             QLineF(m_data.previousSAPoint, m_data.passmarkSAPoint),
                                                             m_data.passmarkSAPoint.GetSAAfter(m_data.saWidth));
            !intersection.isNull())
        {
            lines += intersection;
        }
    }

    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::PassmarkIntersection2BaseLine(const QVector<QPointF> &path, PassmarkSide side) const -> QVector<QLineF>
{
    QVector<QLineF> lines;
    if ((m_data.passmarkAngleType == PassmarkAngleType::Intersection2 ||
         m_data.passmarkAngleType == PassmarkAngleType::Intersection2OnlyLeft) &&
        (side == PassmarkSide::All || side == PassmarkSide::Left))
    {
        // first passmark
        QLineF line(m_data.passmarkSAPoint, m_data.previousSAPoint);
        line.setAngle(line.angle() - 90);

        if (QLineF const intersection = PassmarkIntersection(path,
                                                             line,
                                                             m_data.passmarkSAPoint.GetSABefore(m_data.saWidth));
            !intersection.isNull())
        {
            lines += intersection;
        }
    }

    if ((m_data.passmarkAngleType == PassmarkAngleType::Intersection2 ||
         m_data.passmarkAngleType == PassmarkAngleType::Intersection2OnlyRight) &&
        (side == PassmarkSide::All || side == PassmarkSide::Right))
    {
        // second passmark
        QLineF line(m_data.passmarkSAPoint, m_data.nextSAPoint);
        line.setAngle(line.angle() + 90);
        if (QLineF const intersection
            = PassmarkIntersection(path, line, m_data.passmarkSAPoint.GetSAAfter(m_data.saWidth));
            !intersection.isNull())
        {
            lines += intersection;
        }
    }

    return lines;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::BuiltInSAPassmark(const VPiece &piece, const VContainer *data) const -> QVector<QLineF>
{
    if (m_null)
    {
        return {};
    }

    const QVector<QLineF> lines = BuiltInSAPassmarkBaseLine(piece, piece.SeamMirrorLine(data));
    if (lines.isEmpty())
    {
        return {};
    }

    QVector<QPointF> points;
    CastTo(piece.FullMainPathPoints(data), points);

    return CreatePassmarkLines(lines, points, PassmarkSide::All);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::CalculatePassmarkLength(const VPiece &piece) const -> qreal
{
    qreal length = 0;

    if (!piece.IsSeamAllowance())
    {
        // Seam allowance disabled — the notch is drawn on the main path with a fixed default length (0.5 cm),
        // unless a manual length was provided.
        if (m_data.passmarkSAPoint.IsManualPasskmarkLength())
        {
            length = m_data.passmarkSAPoint.GetPasskmarkLength();
            if (length <= accuracyPointOnLine)
            {
                length = MmToPixel(5.0); // Manual length field empty — fallback to default.
            }
            return length;
        }

        return MmToPixel(5.0);
    }

    bool ok = false;
    length = PassmarkLength(m_data, m_data.passmarkSAPoint.MaxLocalSA(m_data.saWidth), ok);
    if (!ok)
    {
        return 0;
    }

    return length;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::DetermineEdges(const VPiece &piece, const QLineF &mirrorLine) const -> std::pair<QLineF, QLineF>
{
    QLineF edge1;
    QLineF edge2;

    if (!m_data.passmarkSAPoint.IsManualPasskmarkAngle() && !mirrorLine.isNull() && piece.IsShowFullPiece())
    {
        if (VFuzzyComparePoints(m_data.passmarkSAPoint, mirrorLine.p1()))
        {
            edge1 = QLineF(m_data.passmarkSAPoint, m_data.previousSAPoint);
            qreal angle = edge1.angleTo(mirrorLine);
            if (angle > 180)
            {
                angle = (360 - angle) * -1;
            }
            edge2 = edge1;
            edge2.setAngle(edge2.angle() + angle * 2);
        }
        else if (VFuzzyComparePoints(m_data.passmarkSAPoint, mirrorLine.p2()))
        {
            edge2 = QLineF(m_data.passmarkSAPoint, m_data.nextSAPoint);
            qreal angle = edge2.angleTo(QLineF(mirrorLine.p2(), mirrorLine.p1()));
            if (angle > 180)
            {
                angle = (360 - angle) * -1;
            }
            edge1 = edge2;
            edge1.setAngle(edge1.angle() + angle * 2);
        }
        else
        {
            edge1 = QLineF(m_data.passmarkSAPoint, m_data.previousSAPoint);
            edge2 = QLineF(m_data.passmarkSAPoint, m_data.nextSAPoint);
        }
    }
    else
    {
        edge1 = QLineF(m_data.passmarkSAPoint, m_data.previousSAPoint);
        edge2 = QLineF(m_data.passmarkSAPoint, m_data.nextSAPoint);
    }

    return {edge1, edge2};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::BuiltInSAPassmarkBaseLine(const VPiece &piece, const QLineF &mirrorLine) const -> QVector<QLineF>
{
    if (m_null)
    {
        return {};
    }

    const qreal length = CalculatePassmarkLength(piece);
    if (length <= 0)
    {
        return {};
    }

    const auto [firstEdge, secondEdge] = DetermineEdges(piece, mirrorLine);

    QLineF adjustedEdge = firstEdge; // starts at m_data.passmarkSAPoint
    if (m_data.passmarkSAPoint.IsManualPasskmarkAngle())
    {
        // The manual angle is stored as the outward direction (passmarkSAPoint -> seam allowance notch, see
        // PassmarkAngle()/FixNotchPoint). The seam-line notch points into the piece, so use the opposite direction to
        // stay collinear with the primary seam allowance notch.
        adjustedEdge.setAngle(m_data.passmarkSAPoint.GetPasskmarkAngle() + 180);
    }
    else
    {
        adjustedEdge.setAngle(adjustedEdge.angle() + adjustedEdge.angleTo(secondEdge) / 2.);
    }
    adjustedEdge.setLength(length);

    return {adjustedEdge};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::SAPassmarkBaseLine(const VPiece &piece, const VContainer *data, PassmarkSide side) const
    -> QVector<QLineF>
{
    if (m_null)
    {
        return {};
    }

    // Because rollback cannot be calulated if passmark is not first point in main path we rotate it.
    QVector<QPointF> rotatedSeamAllowance;
    CastTo(piece.FullSeamAllowancePointsWithRotation(data, m_data.passmarkIndex), rotatedSeamAllowance);

    QVector<QPointF> seamAllowance;
    CastTo(piece.FullSeamAllowancePoints(data), seamAllowance);

    return SAPassmarkBaseLine(seamAllowance, rotatedSeamAllowance, side);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::SAPassmarkBaseLine(const QVector<QPointF> &seamAllowance, const QVector<QPointF> &rotatedSeamAllowance,
                                   PassmarkSide side) const -> QVector<QLineF>
{
    if (m_null)
    {
        return {};
    }

    if (rotatedSeamAllowance.size() < 2)
    {
        const QString errorMsg =
            QCoreApplication::translate("VPassmark",
                                        "Cannot calculate a notch for point '%1' in piece '%2'. Seam allowance is "
                                        "empty.")
                .arg(m_data.nodeName, m_data.pieceName);
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VExceptionInvalidNotch(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        return {}; // Something wrong
    }

    QPointF seamPassmarkSAPoint;
    const PassmarkStatus seamPassmarkType = GetSeamPassmarkSAPoint(m_data, rotatedSeamAllowance, seamPassmarkSAPoint);
    if (seamPassmarkType == PassmarkStatus::Error)
    {
        const QString errorMsg = QCoreApplication::translate(
                                     "VPassmark", "Cannot calculate a notch for point '%1' in piece '%2'. Cannot find "
                                                  "position for a notch.")
                                     .arg(m_data.nodeName, m_data.pieceName);
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VExceptionInvalidNotch(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        return {}; // Something wrong
    }

    // The anchor is built from previous/passmark/next, which live in the piece's own coordinates: it has to be fixed
    // against that same contour. The rotated copy is only a walk order (it starts at the notch so a rollback can be
    // walked), and on a joint with a true zero margin it degenerates to three points — fixing the anchor against it
    // dragged notches off the allowance entirely (A47, client pattern: a point on a fake triangle instead of the
    // corner of the step).
    if (not FixNotchPoint(seamAllowance, m_data, &seamPassmarkSAPoint))
    {
        const QString errorMsg =
            QCoreApplication::translate("VPassmark",
                                        "Cannot calculate a notch for point '%1' in piece '%2'. Unable to fix a "
                                        "notch position.")
                .arg(m_data.nodeName, m_data.pieceName);
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VExceptionInvalidNotch(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        // **Le cran s'arrête là.** L'ancre n'est pas sur le contour : continuer construisait une
        // ligne de cran à partir d'un point qui n'appartient pas à la pièce. Cette pointe entrait
        // dans la forme que le placement avale — mesurée à 2026 px du contour sur le patron de
        // Sophie — et le moteur de placement cherchait sans fin : le run ne rendait plus la main.
        // Les autres échecs de ce calcul abandonnent déjà le cran ; celui-ci était le seul à
        // poursuivre, et c'est lui qui cassait le run.
        return {};
    }

    if (m_data.passmarkAngleType == PassmarkAngleType::Straightforward)
    {
        // On a straight run the notch stands square to the seam, and the anchor-to-node direction is that square.
        // At a corner it is not: the anchor sits on one of the two edges, so anchor-to-node runs *along* that
        // edge and the notch comes out lying on the seam instead of marking the corner. There the notch has to
        // bisect the joint — which is exactly what "bisector" builds, and why the two rules are one and the same
        // on a corner.
        // ⚠️ **Ne pas transformer un « tout droit » en bissectrice.** La règle a existé le temps d'une
        // journée : sur un coin, la direction ancre→point longe une arête et le cran se couchait sur la
        // couture, donc on la remplaçait par la bissectrice. Mais le test « les deux arêtes forment un
        // angle > 5° » est vrai sur TOUTE une courbe : sur la manche pagode, les six crans « tout droit »
        // sont tous sortis en bissectrices (Sophie, 06-11 : « mauvais rendu sur TOUS les crans »).
        // Valentina ne fait pas cette substitution : un « tout droit » est perpendiculaire à la couture,
        // partout. On s'y tient.
        return PassmarkStraightforwardBaseLine(seamPassmarkSAPoint);
    }

    if (m_data.passmarkAngleType == PassmarkAngleType::Bisector)
    {
        return PassmarkBisectorBaseLine(seamPassmarkType, seamPassmarkSAPoint, seamAllowance);
    }

    if (m_data.passmarkAngleType == PassmarkAngleType::Intersection ||
        m_data.passmarkAngleType == PassmarkAngleType::IntersectionOnlyLeft ||
        m_data.passmarkAngleType == PassmarkAngleType::IntersectionOnlyRight)
    {
        return PassmarkIntersectionBaseLine(seamAllowance, side);
    }

    if (m_data.passmarkAngleType == PassmarkAngleType::Intersection2 ||
        m_data.passmarkAngleType == PassmarkAngleType::Intersection2OnlyLeft ||
        m_data.passmarkAngleType == PassmarkAngleType::Intersection2OnlyRight)
    {
        return PassmarkIntersection2BaseLine(seamAllowance, side);
    }

    return {};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::SAPassmarkPath(const VPiece &piece, const VContainer *data, PassmarkSide side) const -> QPainterPath
{
    return PassmarkToPath(SAPassmark(piece, data, side));
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::BuiltInSAPassmarkPath(const VPiece &piece, const VContainer *data) const -> QPainterPath
{
    return PassmarkToPath(BuiltInSAPassmark(piece, data));
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::CreatePassmarkLines(const QVector<QLineF> &lines, const QVector<QPointF> &seamAllowance,
                                    PassmarkSide side) const -> QVector<QLineF>
{
    if (lines.isEmpty())
    {
        return {};
    }

    auto CreateLinesWithCorrection =
        [this, side, lines, seamAllowance](
            QVector<QLineF> (*create)(const VPiecePassmarkData &passmarkData, const QLineF &, const QVector<QPointF> &))
    {
        if (m_data.passmarkAngleType == PassmarkAngleType::Straightforward)
        {
            return (*create)(m_data, lines.constFirst(), seamAllowance);
        }

        QVector<QLineF> passmarksLines;

        if (side == PassmarkSide::All || side == PassmarkSide::Left)
        {
            passmarksLines += (*create)(m_data, lines.constFirst(), seamAllowance);
        }

        if (side == PassmarkSide::All || side == PassmarkSide::Right)
        {
            passmarksLines += (*create)(m_data, lines.constLast(), seamAllowance);
        }

        return passmarksLines;
    };

    auto CreateLines =
        [this, side, lines](QVector<QLineF> (*create)(const VPiecePassmarkData &passmarkData, const QLineF &))
    {
        if (m_data.passmarkAngleType == PassmarkAngleType::Straightforward)
        {
            return (*create)(m_data, lines.constFirst());
        }

        QVector<QLineF> passmarksLines;

        if (side == PassmarkSide::All || side == PassmarkSide::Left)
        {
            passmarksLines += (*create)(m_data, lines.constFirst());
        }

        if (side == PassmarkSide::All || side == PassmarkSide::Right)
        {
            passmarksLines += (*create)(m_data, lines.constLast());
        }

        return passmarksLines;
    };

    if (m_data.passmarkAngleType != PassmarkAngleType::Bisector)
    {
        switch (m_data.passmarkLineType)
        {
            case PassmarkLineType::TwoLines:
                return CreateLinesWithCorrection(CreateTwoPassmarkLines);
            case PassmarkLineType::ThreeLines:
                return CreateLinesWithCorrection(CreateThreePassmarkLines);
            case PassmarkLineType::TMark:
                return CreateLines(CreateTMarkPassmark);
            case PassmarkLineType::ExternalVMark:
                return CreateLines(CreateExternalVMarkPassmark);
            case PassmarkLineType::InternalVMark:
                return CreateLinesWithCorrection(CreateInternalVMarkPassmark);
            case PassmarkLineType::UMark:
                return CreateLinesWithCorrection(CreateUMarkPassmark);
            case PassmarkLineType::BoxMark:
                return CreateLinesWithCorrection(CreateBoxMarkPassmark);
            case PassmarkLineType::CheckMark:
                return CreateLinesWithCorrection(CreateCheckMarkPassmark);
            case PassmarkLineType::OneLine:
            default:
                return CreateLines(CreateOnePassmarkLines);
        }
    }

    switch (m_data.passmarkLineType)
    {
        case PassmarkLineType::TMark:
            return CreateTMarkPassmark(m_data, lines.constFirst());
        case PassmarkLineType::OneLine:
        case PassmarkLineType::TwoLines:
        case PassmarkLineType::ThreeLines:
        case PassmarkLineType::ExternalVMark:
        case PassmarkLineType::InternalVMark:
        case PassmarkLineType::UMark:
        case PassmarkLineType::BoxMark:
        case PassmarkLineType::CheckMark:
        default:
            return {lines.constFirst()};
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPassmark::ShapeLines(const VPiecePassmarkData &data, const QVector<QLineF> &baseLines,
                           PassmarkSide side) -> QVector<QLineF>
{
    // **The empty contour is the answer, not a stand-in for one.** What trims a notch is the seam
    // allowance it stands in; a symbol stands in nothing, so there is nothing to trim against and
    // the notch keeps its whole length. FindIntersection hands back the line it is given when the
    // contour is empty, which is exactly what is wanted here.
    //
    // The dispatch below is the canvas's own — CreatePassmarkLines — and is reached by handing it a
    // notch built on @p data. The two therefore draw the same shapes from the same code.
    return VPassmark(data).CreatePassmarkLines(baseLines, {}, side);
}
