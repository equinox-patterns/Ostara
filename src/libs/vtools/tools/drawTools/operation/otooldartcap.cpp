/************************************************************************
 **
 **  @file   otooldartcapop.cpp
 **  @author Sophie Denys <contact@sophiedensys.com>
 **  @date   08/10/2026
 **
 **  @brief  The cap of a dart, written as an operation. See the header.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedensys.com>
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

#include "otooldartcap.h"

#include <QPointer>
#include <QUndoStack>
#include <QtGlobal>

#include "../../../dialogs/tools/dialogtool.h"
#include "../../../dialogs/tools/odialogdartcap.h"
#include "../../../undocommands/savetooloptions.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/xml/vabstractpattern.h"
#include "vexception.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "vwidgets/vsimplecurve.h"
#include "vwidgets/vsimplepoint.h"
#include "../vgeometry/vabstractbezier.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/literals.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../toolpoint/toolsinglepoint/odartcaprule.h"
#include "../vtoolline.h"
#include "odartcapedge.h"
#include "oedgereader.h"
#include "ooperationlines.h"
#include "../../../visualization/line/ovistooldartcap.h"
#include "opleatcaprule.h"
#include "opleatnotch.h"
#include "opleatsupport.h"

using namespace Qt::Literals::StringLiterals;

namespace
{
//! Read the cap and the crank from the geometry and write their objects into the container.
//!
//! Filled on every parse, and **before the tool is built** — InitOperatedObjects() reads ExtraPointIds() and
//! ExtraCurveIds(), which read the display, so the display must carry its ids and the container must hold the
//! objects before there is any item to make. The pleat builds its own the same way and at the same moment
//! (OToolPleat::CreateDestination, otoolpleat.cpp:576). This is the one place the geometry is read — the file is a
//! convenience, never the thing the figure exists by.
//! Give a curve to the pattern as a curve — a spline or an arc — so that it has a length to be read by and a
//! variable for the tools that build on it. The same two calls the pleat makes for every curve of its figure.
void RegisterCurve(VContainer *data, const QSharedPointer<VAbstractCurve> &curve, quint32 id)
{
    if (auto bezier = qSharedPointerDynamicCast<VAbstractBezier>(curve))
    {
        data->AddSpline(bezier, id);
    }
    else
    {
        data->AddArc(curve, id);
    }
}

//! The name of the point C: the apex it belongs to, made unique as any object of a figure is.
auto DartCapName(const OToolDartCapInitData &initData, const VContainer *data) -> QString
{
    if (initData.apexId != NULL_ID && data->HasObject(initData.apexId))
    {
        return CopyName(data, data->GetGObject(initData.apexId)->name(), CopySuffix::DartCap);
    }

    return CopyName(data, QStringLiteral("cap"), QString());
}

void BuildFigure(OToolDartCapInitData &initData)
{
    VContainer *data = initData.data;
    if (data == nullptr)
    {
        return;
    }

    // Before a single new id is handed out, the counter is raised above every id the figure already holds — the
    // pleat's own prelude (OPleatDisplay::EnsureIds). Without it an id taken during a parse falls on an object the
    // file already names, and other tools lose theirs.
    for (const OPleatMarkDisplay &mark : std::as_const(initData.display.marks))
    {
        for (const quint32 id : {mark.point, mark.side1, mark.side2, mark.curve1, mark.curve2})
        {
            if (id != NULL_ID)
            {
                data->UpdateId(id);
            }
        }
    }
    for (const OPleatNotchDisplay &notch : std::as_const(initData.display.notches))
    {
        for (const quint32 id : {notch.point, notch.curve1, notch.curve2})
        {
            if (id != NULL_ID)
            {
                data->UpdateId(id);
            }
        }
    }

    // ⚠️ The cap of a dart is built by the TWO FLIPS a dart's fold is made of — the general construction, of which
    // the cap of a pleat is the parallel case (see OPleatCapRule::Dart):
    //
    //  1. the edge the pattern draws at the opening point the sense reads is reflected about the ARM that joins that
    //     point to the apex of the dart: that is the reflection which brings the piece onto the fold;
    //  2. the curve that comes out of it meets the FOLD LINE — the line through the apex and the middle of the
    //     opening, which is the perpendicular bisector of the two opening points — at the point C, and it STOPS
    //     there: what runs on past the fold is the rest of the piece's edge, and no cap follows it;
    //  3. the piece that runs from the point of the opening to C is the first side, and its reflection about the
    //     fold line is the second, which lays it exactly on the copy.
    //
    // The pleat needs no first flip because its two axes are parallel and its edge already ends on the fold line
    // (OPleatCapRule::Sides cuts it there); a dart's two axes meet at its apex, so the first reflection is a real
    // one. The edge is read at the opening point the SENSE designates — the second one is the copy of the first and
    // carries the same edge, the operation that made it having copied the curves of that point too — and on the side
    // that leaves that point AWAY from the other one: the piece of the outline the dart's mouth does not take.
    QPointF capPoint;
    QString apexName;
    QLineF foldLine; // the fold line of the dart: through its apex and the middle of the opening
    QPointF edgePosition;
    QPointF edgeDirectionUsed; // which way the edge runs at the opening point — read by OPleatSupport::Resolve
    OPleatCapSides sides;      // the two sides, when a curve carries the edge at that point
    {
        const QPointF apexPosition = static_cast<QPointF>(*data->GeometricObject<VPointF>(initData.apexId));
        const QPointF firstPosition = static_cast<QPointF>(*data->GeometricObject<VPointF>(initData.firstPointId));
        const QPointF secondPosition = static_cast<QPointF>(*data->GeometricObject<VPointF>(initData.secondPointId));

        const QPointF middle = ODartCapRule::Midpoint(firstPosition, secondPosition);
        foldLine = QLineF(apexPosition, middle);

        const QPointF opening = secondPosition - firstPosition;
        const qreal openingLength = QLineF(QPointF(), opening).length();
        const QPointF normal = (openingLength < 1e-9) ? QPointF() : opening / openingLength;

        // Where the pattern draws the edge, and which way it runs there — asked of the PLEAT'S OWN RULE: a point the
        // user DESIGNATED wins over anything that could be read, and with no designated point OEdgeReader answers.
        OPleatEnd prise;
        prise.pointId = initData.secondSense ? initData.secondPointId : initData.firstPointId;
        prise.position = static_cast<QPointF>(*data->GeometricObject<VPointF>(prise.pointId));
        prise.edgePointId = initData.extendPointId;
        OPleatSupport::Resolve(prise, data);

        // The other point of the opening: the one a dart's copy stands on, and the one that wears the edge when
        // nothing is drawn at the place the copy came to.
        const quint32 otherPointId = initData.secondSense ? initData.firstPointId : initData.secondPointId;

        // WHICH LINE carries the edge, when no point was designated: a question of its own, asked in one place
        // (ODartCapEdge) — and the window asks it with the very same call. A designated point still wins: it is the
        // way out when the pattern cannot say. What is read here is a line and not the origin of a copy: the cap of a
        // dart prolongs the edge of its mouth, and that edge is often a `<line>` no other reader ever sees.
        if (initData.extendPointId == NULL_ID)
        {
            const QPointF edge = ODartCapEdge::Direction(
                prise.pointId, initData.apexId, otherPointId, initData.id, data,
                initData.doc == nullptr ? nullptr : initData.doc->PatternGraph());
            if (not edge.isNull())
            {
                prise.edgeDirection = edge;
            }
        }

        // The side the edge is read on: away from the OTHER point of the opening. The two senses name the two pieces
        // of the piece's outline — before the first point, after the copy — and only their own side leads to them.
        const QPointF side = initData.secondSense ? normal : -normal;

        // The name of the mark: the one the file gave, the one the dialog holds, or the name the tool makes of the
        // apex. It names the junction of the cut, exactly as a pleat names the apex of its own cap.
        const OPleatMarkDisplay saved = initData.display.marks.value(0);
        apexName = saved.name.isEmpty()
                       ? (initData.pointName.isEmpty() ? DartCapName(initData, data) : initData.pointName)
                       : saved.name;

        // WHERE THE EDGE IS DRAWN, and what carries the figure to the place the cap belongs. The sense reads the
        // opening point it designates; when that point is a copy whose operation copied the point and not the curve
        // that ends there — a point of a pleat's figure, where the figure draws points and no curve — the edge is read
        // at the OTHER point of the opening, which a pleat made the image of the very same point: a copy wears the
        // same edge, moved, and the offset between the two places IS that move (see OPleatCapRule::Dart).
        const auto edgeAt = [data, &side](quint32 pointId) -> QSharedPointer<VAbstractCurve>
        {
            const OEdgeCarrier found = OEdgeReader::Carrier(pointId, side, data);
            return (found.objectId != NULL_ID && data->HasObject(found.objectId))
                       ? qSharedPointerDynamicCast<VAbstractCurve>(data->GetGObject(found.objectId))
                       : QSharedPointer<VAbstractCurve>{};
        };

        QPointF edgePoint = prise.position; // where the pattern draws the edge the cap is read on
        QPointF offset;                     // and what carries the figure from there to the point the sense reads
        QSharedPointer<VAbstractCurve> edge = edgeAt(prise.pointId);

        if (edge.isNull() && otherPointId != NULL_ID && data->HasObject(otherPointId))
        {
            const QPointF otherPosition = static_cast<QPointF>(*data->GeometricObject<VPointF>(otherPointId));
            edge = edgeAt(otherPointId);
            if (not edge.isNull())
            {
                edgePoint = otherPosition;
                offset = prise.position - otherPosition;
            }
        }

        sides = OPleatCapRule::Dart(edge, edgePoint, QLineF(prise.position, apexPosition), foldLine, apexName, offset,
                                    &capPoint, nullptr);

        if (sides.edge.isNull() || sides.image.isNull())
        {
            // A line carries the edge — or the two folds meet nowhere: the straight tracés stand, and the cap point
            // is the one the construction alone gives, the edge carried onto the fold line.
            const int baseIndex = initData.secondSense ? 1 : 0;
            if (ODartCapRule::IsFailure(ODartCapRule::CapPointFromEdge(apexPosition, firstPosition, secondPosition,
                                                                      baseIndex, prise.edgeDirection, &capPoint)))
            {
                // Nothing could be read at that point and none was designated: the middle of the opening is the one
                // point of the fold line the user can name.
                capPoint = middle;
            }
        }

        edgePosition = prise.position;
        edgeDirectionUsed = prise.edgeDirection;
    }

    initData.display.show = initData.drawCap;

    // The mark: the point C. It is a point of the pattern like any other — the framework makes its item
    // (InitOperatedObject), places it and draws it, which is what the point tool had to do by hand.
    OPleatMarkDisplay mark = initData.display.marks.value(0);
    mark.pleat = 0;
    mark.end = 0;
    if (mark.point == NULL_ID)
    {
        mark.point = data->getNextId();
        data->UpdateId(mark.point);
    }
    if (mark.name.isEmpty())
    {
        mark.name = apexName; // the name the geometry was read with, so the cut and the mark name the same point
    }
    {
        // The label place the figure carries — zero means it never said, and the usual default stands.
        const QSharedPointer<VPointF> cap = QSharedPointer<VPointF>::create(
            capPoint, mark.name, qFuzzyIsNull(mark.mx) ? labelMX : mark.mx,
            qFuzzyIsNull(mark.my) ? labelMY : mark.my);
        cap->setIdObject(initData.id);
        cap->SetShowLabel(initData.showLabel);
        data->UpdateGObject(mark.point, cap);
    }

    // The two sides of the cap — the piece of the edge the fold turns over, and its image by the fold — or the two
    // straight tracés when nothing but a line carries the edge. A cap holds one pair or the other, never both, which
    // is the pleat's own rule (`OPleatDisplay` keeps curve1/curve2 and side1/side2 that way). Both tracés are LINES
    // of the figure: their ids are given here, and the tools themselves are built where a document and a scene are at
    // hand (CreateFigureLines) — a line of the figure is a tool, never an object the framework invents.
    const auto straightSides = [&mark, data]() -> void
    {
        mark.curve1 = NULL_ID;
        mark.curve2 = NULL_ID;

        if (mark.side1 == NULL_ID)
        {
            mark.side1 = data->getNextId();
            data->UpdateId(mark.side1);
        }
        if (mark.side2 == NULL_ID)
        {
            mark.side2 = data->getNextId();
            data->UpdateId(mark.side2);
        }
    };

    if (not sides.edge.isNull() && not sides.image.isNull())
    {
        // An id the file gave is KEPT — the pleat's own rule (OPleatDisplay::EnsureIds allocates only when the id is
        // NULL_ID), and the counter was raised above every id the figure holds before this point. Testing
        // `HasObject` here would be wrong: the object is created right below, so the test could only ever fail, and
        // every parse would hand the sides a NEW id — leaving the layer's `<item object="…">` entries pointing at
        // objects that no longer exist.
        if (mark.curve1 == NULL_ID)
        {
            mark.curve1 = data->getNextId();
            data->UpdateId(mark.curve1);
        }
        if (mark.curve2 == NULL_ID)
        {
            mark.curve2 = data->getNextId();
            data->UpdateId(mark.curve2);
        }

        QSharedPointer<VAbstractCurve> arm1 = sides.edge;
        QSharedPointer<VAbstractCurve> arm2 = sides.image;
        arm1->setIdObject(initData.id);
        arm2->setIdObject(initData.id);

        // The pen the two sides are DRAWN with is stamped on them at every parse, exactly as the pleat does for its
        // own (OPleatDisplay::Create, opleatdisplay.cpp:802-807): the drawing of a curve is read off the object, so a
        // colour or a style written only in the file would never reach the canvas. A cap holds one pair or the other
        // — two curves here, two lines below — never both.
        for (const QSharedPointer<VAbstractCurve> &side : {arm1, arm2})
        {
            side->SetColor(initData.display.lineColor);
            side->SetPenStyle(initData.display.lineTypeLine);
        }

        data->UpdateGObject(mark.curve1, arm1);
        data->UpdateGObject(mark.curve2, arm2);
        RegisterCurve(data, arm1, mark.curve1);
        RegisterCurve(data, arm2, mark.curve2);

        mark.side1 = NULL_ID;
        mark.side2 = NULL_ID;
    }
    else
    {
        // No curve carries the edge at that point — a line, or a point the user designated — or the two folds meet
        // nowhere: the cap is the straight one the construction gives, and its two sides are the tracés that join C
        // to each point of the opening.
        straightSides();
    }

    // The crank: the mark the fitter reads on the piece, walked **along the very edge the cap follows** — the curve
    // before the first point of the opening at one sense, the curve the copy wears after the second one at the other —
    // by **half the opening** (`ODartCapRule::CrankValue`), the value a pleat counts its own notch with.
    //
    // Two things set it apart from the pleat's notch, and both come from the dart's own geometry:
    //
    //  - the value is counted **from the opening point the sense reads** (`ODartCapRule::CrankBaseIndex`), never from
    //    the other one: the two senses name the two curves that meet the mouth, and each crank belongs to its own.
    //    Counting it from the other point is what made the crank of one sense the crank of the other;
    //  - the crank **stands on that curve**: it is the point of it a value along, and the curve is cut there, so the
    //    mark is a point OF the edge and not a point beside it. The pleat can carry its notch to the copy's place
    //    because its edge is a line that travels; a dart's copy wears its own curve, and the crank is read on it.
    QVector<OPleatNotchDisplay> notches;
    if (initData.showNotch)
    {
        OPleatNotchDisplay notch = initData.display.notches.value(0);
        notch.pleat = 0;
        notch.end = 0;

        const QPointF firstPosition = static_cast<QPointF>(*data->GeometricObject<VPointF>(initData.firstPointId));
        const QPointF secondPosition = static_cast<QPointF>(*data->GeometricObject<VPointF>(initData.secondPointId));
        const qreal value = ODartCapRule::CrankValue(firstPosition, secondPosition);
        const quint32 notchBaseId = (ODartCapRule::CrankBaseIndex(initData.secondSense) == 0)
                                        ? initData.firstPointId
                                        : initData.secondPointId;

        QPointF notchPoint = capPoint;
        qreal notchLength = -1.;
        quint32 notchEdgeId = NULL_ID;
        QPointF notchOffset; // where the edge is drawn, from the place the crank belongs

        if (value > 0. && data->HasObject(notchBaseId))
        {
            const QPointF opening = secondPosition - firstPosition;
            const qreal openingLength = QLineF(QPointF(), opening).length();
            const QPointF normal = (openingLength < 1e-9) ? QPointF() : opening / openingLength;

            // The side the CAP reads, and no other: the same curve carries the cap and the crank, so the mark is cut
            // where the cap's own side runs.
            const QPointF side = initData.secondSense ? normal : -normal;

            // WHERE THE EDGE IS DRAWN: at the point the sense reads, and, when that point is a copy the pattern drew
            // no curve at, at the other point of the opening — the very same two questions the cap asks above, so
            // that a sense reads its cap and its crank on one and the same edge.
            const quint32 otherNotchId = initData.secondSense ? initData.firstPointId : initData.secondPointId;

            QPointF from = edgePosition;
            OEdgeCarrier carrier = (initData.extendPointId == NULL_ID) ? OEdgeReader::Carrier(notchBaseId, side, data)
                                                                      : OEdgeCarrier{};
            if (carrier.objectId == NULL_ID && initData.extendPointId == NULL_ID && otherNotchId != NULL_ID
                && data->HasObject(otherNotchId))
            {
                const OEdgeCarrier other = OEdgeReader::Carrier(otherNotchId, side, data);
                if (other.objectId != NULL_ID)
                {
                    carrier = other;
                    from = static_cast<QPointF>(*data->GeometricObject<VPointF>(otherNotchId));
                    notchOffset = edgePosition - from;
                }
            }

            const QSharedPointer<VAbstractCurve> curve
                = (carrier.objectId != NULL_ID && data->HasObject(carrier.objectId))
                      ? qSharedPointerDynamicCast<VAbstractCurve>(data->GetGObject(carrier.objectId))
                      : QSharedPointer<VAbstractCurve>{};

            if (not curve.isNull())
            {
                const OPleatNotchPlacement placement = OPleatNotchRule::Place(curve, from, value);
                if (placement.length >= 0.)
                {
                    // The point of the curve itself, a value along it — the mark stands ON the edge — carried from
                    // where that edge is drawn to the place the sense reads.
                    notchPoint = placement.point + notchOffset;
                    notchLength = placement.length;
                    notchEdgeId = carrier.objectId;
                }
            }

            if (notchEdgeId == NULL_ID)
            {
                // A LINE carries the edge — or the walk did not stand on it: the mark is then a point on that line,
                // the pleat's own branch (`notch.point = from + inward * values.at(departure)`, opleatrule.cpp:632).
                QPointF inward = carrier.inward;
                if (inward.isNull())
                {
                    const qreal length = QLineF(QPointF(), edgeDirectionUsed).length();
                    if (length > 1e-9)
                    {
                        const QPointF unit = edgeDirectionUsed / length;
                        inward = (QPointF::dotProduct(unit, side) > 0.) ? unit : -unit;
                    }
                }

                if (not inward.isNull())
                {
                    notchPoint = from + inward * value + notchOffset;
                }
            }
        }

        if (notch.point == NULL_ID)
        {
            notch.point = data->getNextId();
            data->UpdateId(notch.point);
        }
        if (notch.name.isEmpty())
        {
            notch.name = mark.name + CopySuffix::Notch;
        }
        {
            const QSharedPointer<VPointF> crank
                = QSharedPointer<VPointF>::create(notchPoint, notch.name, labelMX, labelMY);
            crank->setIdObject(initData.id);
            crank->SetShowLabel(false);
            data->UpdateGObject(notch.point, crank);
        }

        // What the mark cuts: the two halves of the edge at the place it stands. A line carries the edge, there is
        // nothing to cut, and no curve may be named — the file must not name what the parse will never make.
        const bool cut = (notchEdgeId != NULL_ID && notchLength >= 0. && value > 0.);
        if (cut)
        {
            const QSharedPointer<VAbstractCurve> edge
                = qSharedPointerDynamicCast<VAbstractCurve>(data->GetGObject(notchEdgeId));
            const OPleatNotchHalves halves
                = edge.isNull() ? OPleatNotchHalves{}
                                : OPleatNotchRule::Cut(edge, notchLength, notch.name, {}, {});
            // The halves are carried with the mark: the crank stands on the edge where the sense reads it, and the cut
            // that makes it a point OF that edge is made where the pattern drew it (see OPleatCapRule::Moved).
            QSharedPointer<VAbstractCurve> first = OPleatCapRule::Moved(halves.first, notchOffset);
            QSharedPointer<VAbstractCurve> second = OPleatCapRule::Moved(halves.second, notchOffset);
            if (not first.isNull() && not second.isNull())
            {
                // See the sides: an id the file gave is kept, and only a missing one is taken.
                if (notch.curve1 == NULL_ID)
                {
                    notch.curve1 = data->getNextId();
                    data->UpdateId(notch.curve1);
                }
                if (notch.curve2 == NULL_ID)
                {
                    notch.curve2 = data->getNextId();
                    data->UpdateId(notch.curve2);
                }

                first->setIdObject(initData.id);
                second->setIdObject(initData.id);

                data->UpdateGObject(notch.curve1, first);
                data->UpdateGObject(notch.curve2, second);
                RegisterCurve(data, first, notch.curve1);
                RegisterCurve(data, second, notch.curve2);

                notches.append(notch);
            }
        }

        if (notches.isEmpty())
        {
            // The mark stands alone: no curve carries the edge, so it is a point and nothing else — and the ids of
            // halves a previous state had must go, or the file would name curves that will never exist.
            notch.curve1 = NULL_ID;
            notch.curve2 = NULL_ID;
            notches.append(notch);
        }
    }

    initData.display.marks = QVector<OPleatMarkDisplay>{mark};
    initData.display.notches = notches;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The two straight sides of the cap, as LINES of the figure.
 *
 *  A line of the figure is a TOOL: the framework only gives an item to what is already one (InitOperatedLine asks
 *  VAbstractPattern::FindTool), so a cap a line carries is built here, where a document and a scene are at hand —
 *  exactly as the pleat builds its own (OToolPleat::CreateDisplayObjects → OOperationLines::CreateLine). The pen of
 *  the figure is stamped on them by that very call, at every parse, which is what makes a change of style or of
 *  colour reach the canvas. A cap holds one pair or the other: nothing is built when a curve carries the edge. */
void CreateFigureLines(const OToolDartCapInitData &initData)
{
    if (initData.doc == nullptr || initData.data == nullptr || initData.scene == nullptr)
    {
        return;
    }

    for (const OPleatMarkDisplay &mark : std::as_const(initData.display.marks))
    {
        if (mark.point == NULL_ID || mark.side1 == NULL_ID || mark.side2 == NULL_ID)
        {
            continue;
        }

        OOperationLine line;
        line.typeLine = initData.display.lineTypeLine;
        line.lineColor = initData.display.lineColor;

        // The two tracés the point tool drew by hand: the cap point C and each of the two points of the opening.
        const QVector<QPair<quint32, quint32>> sides = {{mark.side1, initData.firstPointId},
                                                       {mark.side2, initData.secondPointId}};
        for (const auto &[side, end] : sides)
        {
            if (end == NULL_ID || not initData.data->HasObject(end))
            {
                continue;
            }

            line.result = side;
            OOperationLines::CreateLine(initData.doc, initData.data, initData.scene, line, mark.point, end,
                                        initData.parse);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Turn the element of an old cap — a `<point>` — into the operation that cap became.
 *
 *  The element is what decides which reader takes a cap, and the file of a cap written before it was an operation
 *  still holds a `<point>`. Everything of the FIGURE such an element carries — the name and the label place of the
 *  point C, the pen it was drawn with — the operation keeps in its `<pleatDisplay>`, and `<operation>` declares none
 *  of them: they go, and the write that follows puts them where they belong. The schema is closed, so an attribute of
 *  the old shape left behind is a pattern that no longer opens — measured, not supposed: `name` alone was enough to
 *  make the save fail with a facet error pointing at a line the user cannot read. */
void MigrateFromPointShape(QDomElement &element)
{
    if (element.isNull() || element.tagName() == VAbstractPattern::TagOperation)
    {
        return;
    }

    element.setTagName(VAbstractPattern::TagOperation);

    // The very five the old shape declared and the operation does not: `VDrawTool` writes the id and the notes,
    // `VToolSinglePoint` the name and the two label places, `VToolLinePoint` the pen — and the operation writes its
    // own list, which holds neither.
    for (const QString &attribute : {AttrName, AttrMx, AttrMy, AttrTypeLine, AttrLineColor})
    {
        element.removeAttribute(attribute);
    }
}

} // namespace

const QString OToolDartCap::ToolType = QStringLiteral("dartCap");
const QString OToolDartCap::AttrApex = QStringLiteral("apex");
const QString OToolDartCap::AttrSecondSense = QStringLiteral("sense");
const QString OToolDartCap::AttrDrawCap = QStringLiteral("drawCap");
const QString OToolDartCap::AttrNotch = QStringLiteral("notch");
const QString OToolDartCap::AttrCurve1 = QStringLiteral("curve1");
const QString OToolDartCap::AttrCurve2 = QStringLiteral("curve2");
const QString OToolDartCap::AttrNotchCurve1 = QStringLiteral("notchCurve1");
const QString OToolDartCap::AttrNotchCurve2 = QStringLiteral("notchCurve2");
const QString OToolDartCap::AttrNotchPoint = QStringLiteral("notchPoint");

//---------------------------------------------------------------------------------------------------------------------
OToolDartCap::OToolDartCap(const OToolDartCapInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_apexId(initData.apexId),
    m_firstPointId(initData.firstPointId),
    m_secondPointId(initData.secondPointId),
    m_extendPointId(initData.extendPointId),
    m_secondSense(initData.secondSense),
    m_drawCap(initData.drawCap),
    m_showNotch(initData.showNotch),
    m_showLabel(initData.showLabel),
    m_display(initData.display)
{
    // The framework makes the items of the figure from ExtraPointIds()/ExtraCurveIds() — the point C, its two sides
    // and the crank — and parents them to this item, which stays at the origin.
    InitOperatedObjects();
    ApplyFigureVisibility(); // what the figure shows, and what it never shows

    // An old cap is still a `<point>` in its file, and this is the only moment the element can be turned into the
    // operation it became: once per open, on the tool the parse has just built, before anything reads it again.
    MigrateLegacyElement();

    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::RefreshFigure()
{
    OToolDartCapInitData initData;
    // EVERY field BuildFigure reads must be here. It was not: `id` was missing, so every figure object was handed a
    // NULL owner at each refresh — `setIdObject(0)` — and `showLabel` too, which hid the name of C again. A struct
    // built by hand is a struct that loses a field the day one is added.
    initData.id = m_id;
    initData.data = getData();
    initData.pointName = m_display.marks.isEmpty() ? QString() : m_display.marks.constFirst().name;
    initData.showLabel = m_showLabel;
    initData.apexId = m_apexId;
    initData.firstPointId = m_firstPointId;
    initData.secondPointId = m_secondPointId;
    initData.extendPointId = m_extendPointId;
    initData.secondSense = m_secondSense;
    initData.drawCap = m_drawCap;
    initData.showNotch = m_showNotch;
    initData.display = m_display;

    BuildFigure(initData);

    m_display = initData.display;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ApplyFigureVisibility()
{
    // What the figure never shows: the two halves of the edge a crank cuts. They are objects of the pattern — a piece
    // is built through the crank, so they have to exist — and the edge draws itself, whole. Nothing else takes them
    // off the canvas, a curve answering the layer as a whole. This is the pleat's own rule, word for word.
    for (const OPleatNotchDisplay &notch : std::as_const(m_display.notches))
    {
        for (const quint32 id : {notch.curve1, notch.curve2})
        {
            if (auto *item = qobject_cast<VSimpleCurve *>(operatedObjects.value(id, nullptr)); item != nullptr)
            {
                item->setVisible(false);
            }
        }
    }

    // What the box adds, and only that: the two tracés of the cap and the crank that marks it. "The point exists
    // either way, the two tracés are what the box adds" — the very sentence the point tool wrote over the box (see
    // the dock's own comment), so the point C is left alone and the rest follows the flag. Read here and not only
    // where the flag is set: every parse rebuilds the items of the figure — a fresh item is drawn — so the flags have
    // to be given back to them at each pass. A hidden layer does not enter here: it hides the item of the operation,
    // and the whole figure goes with it (VAbstractOperation::GroupVisibility).
    for (const OPleatMarkDisplay &mark : std::as_const(m_display.marks))
    {
        for (const quint32 id : {mark.curve1, mark.curve2, mark.side1, mark.side2})
        {
            if (id == NULL_ID)
            {
                continue;
            }

            if (auto *item = qobject_cast<VSimpleCurve *>(operatedObjects.value(id, nullptr)); item != nullptr)
            {
                item->setVisible(m_drawCap);
                continue;
            }

            // The straight tracés are LINES of the pattern, built by the parse like any other line: they are not in
            // operatedObjects, and the registry is what names them.
            if (auto *line = qobject_cast<VToolLine *>(VAbstractPattern::FindTool(id)); line != nullptr)
            {
                line->setVisible(m_drawCap);
            }
        }
    }

    for (const OPleatNotchDisplay &notch : std::as_const(m_display.notches))
    {
        // The crank is an object the operation parents itself, like the point C: its item is in operatedObjects. No
        // cap, no crank — the box that asks for one is greyed with the cap turned off, and a mark of nothing would
        // sit on the piece with no line to explain it.
        if (auto *item = qobject_cast<VSimplePoint *>(operatedObjects.value(notch.point, nullptr)); item != nullptr)
        {
            item->setVisible(m_drawCap);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::AttachFigureToLayer()
{
    if (doc == nullptr)
    {
        return;
    }

    const quint32 active = doc->GetActiveGroup();
    if (active == NULL_ID)
    {
        return; // the piece has no layer at all: nothing to draw into yet
    }

    QVector<QPair<quint32, quint32>> objects;

    // The ids are read off the FIGURE itself, and never off the container: a pair is an id, exactly as a line's is,
    // and a layer must already hold what a parse creates a moment later. Asking the container here is what used to
    // leave a cap point or a crank outside every layer — the objects do not exist yet when the change is written.
    // The pleat's own rule, word for word (OToolPleat::AttachFigureToLayer).
    for (const OPleatMarkDisplay &mark : std::as_const(m_display.marks))
    {
        if (mark.point != NULL_ID)
        {
            objects.append(qMakePair(mark.point, m_id));
        }

        // The two sides of a cap a curve carries: a curve answers to its own id, exactly as a line does, and both
        // sides are the figure — they are what the pattern shows of the cap.
        for (const quint32 id : {mark.curve1, mark.curve2})
        {
            if (id != NULL_ID)
            {
                objects.append(qMakePair(id, id));
            }
        }
    }

    for (const OPleatNotchDisplay &notch : std::as_const(m_display.notches))
    {
        if (notch.point != NULL_ID)
        {
            objects.append(qMakePair(notch.point, m_id));
        }

        for (const quint32 id : {notch.curve1, notch.curve2})
        {
            if (id != NULL_ID)
            {
                objects.append(qMakePair(id, id));
            }
        }
    }

    for (const quint32 id : ExtraLineIds())
    {
        objects.append(qMakePair(id, id));
    }

    doc->AddItemsToGroup(objects, active);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::SyncFigureItems() -> bool
{
    QSet<quint32> wanted; // the points and the curves the operation draws now
    for (const DestinationItem &object : std::as_const(m_destination))
    {
        if (object.id != NULL_ID)
        {
            wanted.insert(object.id);
        }
    }
    for (const quint32 id : ExtraPointIds())
    {
        wanted.insert(id);
    }
    for (const quint32 id : ExtraCurveIds())
    {
        wanted.insert(id);
    }

    for (auto it = operatedObjects.begin(); it != operatedObjects.end();)
    {
        // Only the objects of the figure are in this table: a line of the figure is a *tool*, which a full parse
        // builds and takes away, and which never enters here (see InitOperatedLine, in the base).
        if (wanted.contains(it.key()))
        {
            ++it;
            continue;
        }

        // The figure no longer holds it. The base has just refreshed it from the container, which still carries the
        // object — a lite parse never empties it — so nothing but this call takes it off the canvas.
        delete it.value();
        it = operatedObjects.erase(it);
    }

    bool built = false;
    const auto build = [this, &built](quint32 id)
    {
        if (id != NULL_ID && not operatedObjects.contains(id) && getData() != nullptr && getData()->HasObject(id))
        {
            InitOperatedObject(id);
            built = true;
        }
    };

    for (const DestinationItem &object : std::as_const(m_destination))
    {
        build(object.id);
    }
    for (const quint32 id : ExtraPointIds())
    {
        build(id);
    }
    for (const quint32 id : ExtraCurveIds())
    {
        build(id);
    }

    return built;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SaveOptionsAndReparse(bool fullParse, const QString &commandText)
{
    if (doc == nullptr)
    {
        return;
    }

    QDomElement const oldDomElement = doc->FindElementById(m_id, getTagName());
    if (!oldDomElement.isElement())
    {
        return;
    }

    // The figure follows the options, so its objects have to be given their ids before the file is written: the parse
    // that rebuilds them reads the ids from here. And what the change has just created belongs to the active layer,
    // like every object.
    AttachFigureToLayer();

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    QDomElement newDomElement = oldDomElement.cloneNode().toElement();
    SaveOptions(newDomElement, obj);

    // The command goes in a group of its own so that it can neither merge with the option edit before it nor lose the
    // parse it asks for: SaveToolOptions merges with its predecessor and the merged command keeps the parse mode of
    // the first one (see SaveToolOptions::mergeWith).
    auto *group = new QUndoCommand();
    group->setText(commandText);

    auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id, group);
    if (fullParse)
    {
        saveOptions->SetFullParse(true);
        connect(saveOptions, &SaveToolOptions::NeedFullParsing, doc, &VAbstractPattern::NeedFullParsing,
                Qt::QueuedConnection);
    }
    else
    {
        connect(saveOptions, &SaveToolOptions::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    }

    VAbstractApplication::VApp()->getUndoStack()->push(group);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::FigureFollowsLiteParse(const QDomElement &oldElement, const QDomElement &newElement) const -> bool
{
    // The sides of the cap, when they follow a curve, are objects of the pattern: a full parse alone registers one,
    // and the registry is rebuilt by every parse, so a curve born on a lite one would be gone at the next. A change
    // of them is therefore not the tool's to carry — the pleat's own rule, word for word.
    if (OPleatDisplay::LineIds(oldElement) != OPleatDisplay::LineIds(newElement))
    {
        return false;
    }

    // The copies the operation makes of the objects it names: a change of them goes with a change of those objects,
    // and the rest of the tool asks a full parse for that.
    const auto destinationIds = [](const QDomElement &element) -> QSet<quint32>
    {
        QSet<quint32> ids;
        for (const DestinationItem &item : VAbstractOperation::ExtractDestinationData(element))
        {
            if (item.id != NULL_ID)
            {
                ids.insert(item.id);
            }
        }
        return ids;
    };

    return destinationIds(oldElement) == destinationIds(newElement);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::RefreshDataInFile()
{
    VAbstractOperation::RefreshDataInFile();

    QDomElement domElement = doc->FindElementById(m_id, getTagName());
    // Only an element that IS an operation takes a figure: writing a `<pleatDisplay>` into the `<point>` of an old
    // cap would make that element invalid long before any save. Such an element is migrated whole by SaveOptions,
    // which is the door every change of these options goes through (SaveOptionsAndReparse, SaveOption).
    if (domElement.isElement() && domElement.tagName() == VAbstractPattern::TagOperation)
    {
        OPleatDisplay::Save(doc, domElement, m_display);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::FullUpdateFromFile()
{
    VAbstractOperation::FullUpdateFromFile();

    // The figure the parse has made: an object the operation parents itself has no element of its own, so nothing but
    // this call gives it an item — the constructor does it once, on the full parse, and a lite one goes here. The
    // layer is only asked when an object was born: attaching what is already there would mark the pattern changed on
    // every parse, an open of it included. The pleat does exactly this, in this order.
    if (SyncFigureItems())
    {
        AttachFigureToLayer();
    }

    ApplyFigureVisibility(); // the halves a crank cuts, the tracés of a cap that is not drawn, and its crank
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene,
                                   VAbstractPattern *doc, VContainer *data) -> OToolDartCap *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogDartCap> dialogTool = qobject_cast<ODialogDartCap *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolDartCapInitData initData;
    initData.apexId = dialogTool->GetApexId();
    initData.firstPointId = dialogTool->GetFirstPointId();
    initData.secondPointId = dialogTool->GetSecondPointId();
    initData.extendPointId = dialogTool->GetExtendPointId();
    initData.secondSense = dialogTool->GetSecondSense();
    initData.drawCap = dialogTool->GetDrawCap();
    initData.showNotch = dialogTool->GetNotchDrawn();
    initData.display = dialogTool->GetDisplay();

    // The name of the point C and whether its label is shown: they belong to the FIGURE — the old tool read them here
    // and wrote them on its element, and the operation keeps them in its mark (`name()` is the mark's name, written
    // in the `<pleatDisplay>`). Read from the dialog like every other part of the figure, or the name the user typed
    // would be thrown away for a name made from the apex, and the label would never show at all.
    initData.pointName = dialogTool->GetPointName();
    initData.showLabel = dialogTool->GetShowLabel();
    initData.source = dialogTool->GetSourceObjects();
    initData.lines = dialogTool->GetLineSources();
    initData.notes = dialogTool->GetNotes();
    // The layer the dialog chose for the new tool. Without it the operation is created with no layer at all
    // (`AddToolToScene` only makes one for a source that asked for it), and its whole figure is then outside every
    // layer: the eye of a layer would neither show nor hide it. The pleat carries the same three fields.
    initData.hasLinkedVisibilityGroup = dialogTool->HasLinkedVisibilityGroup();
    initData.visibilityGroupName = dialogTool->GetVisibilityGroupName();
    initData.layerGroup = dialogTool->GetLayerGroup();
    initData.newLayerGroupName = dialogTool->GetNewLayerGroupName();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;

    OToolDartCap *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::Create(OToolDartCapInitData &initData) -> OToolDartCap *
{
    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->getNextId(); // Just reserve id for tool
    }

    // The sources: a name they do not carry is made here, from the object they name. A source is written with a name
    // in the file — the schema asks for one that is not empty — and the dialog hands bare ids. This is the very place
    // the pleat names its own (PrepareNames, with the suffix its dialog chose); a name the file gave is left alone.
    NameSources(initData.source, initData.data);

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    // What the cap is built from. The edge goes from the point used to the tool that uses it, as everywhere else.
    for (const quint32 id : {initData.apexId, initData.firstPointId, initData.secondPointId, initData.extendPointId})
    {
        if (id != NULL_ID)
        {
            patternGraph->AddEdge(id, initData.id);
        }
    }

    // The figure is read from the geometry HERE — before the tool is built, and on every parse. This is what the
    // pleat does with its own (OToolPleat::CreateDestination, otoolpleat.cpp:576), and it is what keeps the sides
    // and the crank from depending on the ids a file gave: the file is a convenience, the geometry is the source.
    BuildFigure(initData);

    // The two straight sides are LINES of the figure, and a line is a tool: built here, at every parse, with the pen
    // the figure carries — the pleat's own call, and the only thing that re-reads the pen of such a line.
    CreateFigureLines(initData);

    for (const quint32 id : OPleatDisplay::PointImages(initData.display, initData.data))
    {
        patternGraph->AddVertex(id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, id);
    }

    for (const quint32 id : OPleatDisplay::CurveImages(initData.display, initData.data))
    {
        patternGraph->AddVertex(id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, id);
    }

    CreateDestinationLines(initData, {});

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        return AddToolToScene<OToolDartCap>(
            initData, Tool::DartCap, tr("dart cap"),
            [&initData]() { return new OToolDartCap(initData); });
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::MigrateLegacyElement()
{
    if (doc == nullptr)
    {
        return;
    }

    QDomElement element = doc->FindElementById(m_id);
    if (not element.isElement() || element.tagName() == VAbstractPattern::TagOperation)
    {
        return; // an operation already, or a tool no element holds yet — a creation from the interface
    }

    // The element becomes the operation the cap is, whole, once, here at the open: its attributes, its source and its
    // figure. Every other path follows from this one — a read gives the figure back instead of rebuilding it from
    // nothing, and an edit is written where the user expects it. Written through SaveOptions, the writer of every
    // other save, so there is one place that knows what a cap's element holds.
    MigrateFromPointShape(element);

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOptions(element, obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ChangeLabelPosition(quint32 id, const QPointF &pos)
{
    VAbstractOperation::ChangeLabelPosition(id, pos); // the container object, which is what the scene draws

    // The label of C belongs to the FIGURE. The base writes the destination record of the point that moved — and a
    // dart cap copies nothing: C is no destination, so nothing was written, `RefreshFigure()` put the label back where
    // the file had it at the next parse, and the file kept that place forever. The old tool wrote the point's own
    // `mx`/`my` at every drag; the operation writes the mark's, in the element that holds it — the same direct write,
    // out of the undo stack, which is how a label has always been moved.
    if (m_display.marks.isEmpty() || id != m_display.marks.constFirst().point)
    {
        return;
    }

    m_display.marks[0].mx = pos.x();
    m_display.marks[0].my = pos.y();
    RefreshDataInFile();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::ExtraPointIds() const -> QVector<quint32>
{
    return OPleatDisplay::PointImages(m_display, getData());
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::ExtraCurveIds() const -> QVector<quint32>
{
    return OPleatDisplay::CurveImages(m_display, getData());
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::ExtraLineIds() const -> QVector<quint32>
{
    // The pleat's own filter, word for word: an id of NULL would name no object, and handing it over would have the
    // framework make an item for something that is not there. The lists come from a file, so they can carry one.
    QVector<quint32> ids;
    ids.reserve(m_display.lines.size() + m_display.foldLines.size() + m_display.marks.size() * 2);

    for (const quint32 id : std::as_const(m_display.lines))
    {
        if (id != NULL_ID)
        {
            ids.append(id);
        }
    }

    for (const quint32 id : std::as_const(m_display.foldLines))
    {
        if (id != NULL_ID)
        {
            ids.append(id);
        }
    }

    for (const OPleatMarkDisplay &mark : std::as_const(m_display.marks))
    {
        for (const quint32 id : {mark.side1, mark.side2})
        {
            if (id != NULL_ID)
            {
                ids.append(id);
            }
        }
    }
    return ids;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::DisplayObjectIds(const QDomElement &domElement) const -> QSet<quint32>
{
    return OPleatDisplay::ObjectIds(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetSecondSense(bool secondSense)
{
    if (m_secondSense == secondSense)
    {
        return;
    }

    m_secondSense = secondSense;

    // The cap and the crank are read again on the other edge, and the objects of the figure with them: a side may
    // turn from a curve to a line, which is a change of the ids a full parse alone carries (see
    // FigureFollowsLiteParse). The command and the parse are the pleat's own (SaveOptionsAndReparse).
    RefreshFigure();
    SaveOptionsAndReparse(true, secondSense ? tr("read the cap on the other edge")
                                            : tr("read the cap on this edge"));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetCapDrawn(bool drawn)
{
    if (m_drawCap == drawn)
    {
        return;
    }

    m_drawCap = drawn;

    // The figure's own flag, and the one the parse shows the items by: `OPLeatDisplay::Save` writes `show`, and the
    // framework reads it — leaving it behind meant unticking "Draw the cap" changed the attribute and nothing else.
    m_display.show = drawn;

    // The command and the parse FIRST, and the visibility after them: the parse rebuilds what the figure changed and
    // hands a fresh item to anything it adds — a fresh item is drawn — so the tracés would come back for the time of
    // it. Hidden after, the last word is ours. The layer pass of `SaveOptionsAndReparse` does not show them any more:
    // it hides the item of the operation, not the objects of its figure (VAbstractOperation::GroupVisibility).
    SaveOptionsAndReparse(false, drawn ? tr("draw the cap") : tr("stop drawing the cap"));

    // Taken off the canvas at once, and not a parse later: the box is what asks for the tracés, and the eye must see
    // the answer to the click.
    ApplyFigureVisibility();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetNotchDrawn(bool drawn)
{
    if (m_showNotch == drawn)
    {
        return;
    }

    m_showNotch = drawn;

    // The crank is a NEW object when the box is ticked: its id and the ids of the two halves it cuts are given here,
    // before the file holds them, and a full parse is what builds the tools and the items of a figure that grew.
    RefreshFigure();
    SaveOptionsAndReparse(true, drawn ? tr("mark the crank") : tr("take the crank away"));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::getLineType() const -> QString
{
    // The pen the figure is drawn with — never VDrawTool's own, which nothing reads for this operation.
    return m_display.lineTypeLine;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetLineType(const QString &value)
{
    if (m_display.lineTypeLine == value)
    {
        return;
    }

    m_display.lineTypeLine = value;

    // A pen redraws lines: the options and the parse that re-stamps it, exactly as the pleat's own setter
    // (OToolPleat::SetDisplayLineType) — writing the file alone left the canvas untouched.
    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetLineColor(const QString &value)
{
    if (m_display.lineColor == value)
    {
        return;
    }

    m_display.lineColor = value;

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::NameSources(QVector<SourceItem> &source, const VContainer *data)
{
    if (data == nullptr)
    {
        return;
    }

    for (SourceItem &item : source)
    {
        if (item.id == NULL_ID || not item.name.isEmpty() || not data->HasObject(item.id))
        {
            continue; // a name the file gave is the one its author gave: never renamed from under him
        }

        try
        {
            item.name = CopyNameOfSource(data, item.id, CopySuffix::DartCap);
        }
        catch (const VExceptionBadId &)
        {
            // An object the container does not know is one no name can be made from: the source keeps the empty name
            // it came with, and the writer simply leaves it out of the file rather than write nothing.
            continue;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::PointNameOf(quint32 pointId) const -> QString
{
    const VContainer *data = getData();
    if (pointId == NULL_ID || data == nullptr || not data->HasObject(pointId))
    {
        return {};
    }
    return data->GetGObject(pointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::name() const -> QString
{
    return m_display.marks.isEmpty() ? QString() : m_display.marks.constFirst().name;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::ApexName() const -> QString
{
    return PointNameOf(m_apexId);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::FirstPointName() const -> QString
{
    return PointNameOf(m_firstPointId);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::SecondPointName() const -> QString
{
    return PointNameOf(m_secondPointId);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::ExtendPointName() const -> QString
{
    return PointNameOf(m_extendPointId);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::OpenOptionsDialog()
{
    // What the dock cannot edit: the figure itself, which the dialog settles. Same door as the pleat's.
    ShowOptionsDialog<ODialogDartCap>();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetDialog()
{
    // The dialog opens on what the tool already holds, so an edit starts from the truth and not from empty fields.
    const QPointer<ODialogDartCap> dialogTool = qobject_cast<ODialogDartCap *>(m_dialog);
    if (dialogTool.isNull())
    {
        return; // in Release SCASSERT says nothing, and a dialog that is not ours is not a dialog at all
    }

    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetApexId(m_apexId);
    dialogTool->SetFirstPointId(m_firstPointId);
    dialogTool->SetSecondPointId(m_secondPointId);
    dialogTool->SetExtendPointId(m_extendPointId);
    dialogTool->SetSecondSense(m_secondSense);
    dialogTool->SetDrawCap(m_drawCap);
    dialogTool->SetNotchDrawn(m_showNotch);
    dialogTool->SetDisplay(m_display);
    // The name of the point C and whether it is shown: the dialog opens on what the figure holds, exactly as it opens
    // on the four points and the flags. Left out, the field came up empty and the checkbox unchecked — and pressing OK
    // then wrote that emptiness back, which is how a name and a label were lost by the simple act of opening the
    // dialog. The old tool filled both here.
    dialogTool->SetPointName(name());
    dialogTool->SetPointLabelVisible(m_showLabel);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);
    dialogTool->SetLineSources(Lines());

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolDartCap>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetVisualization()
{
    // The tool's own visualization — the one the dependency tree and the options browser show for the tool being
    // looked at — fed here, once when it is created and again after every parse (VAbstractOperation::FullUpdateFromFile
    // calls this). Left empty, selecting a cap in the tree showed NOTHING: the old tool filled its own the same way,
    // and the dialog fills its own the same way too (ODialogDartCap::RefreshPreview).
    if (vis.isNull())
    {
        return; // no visualization to feed: the tool is not being shown, and there is nothing to draw for one
    }

    auto *visual = qobject_cast<OVisToolDartCap *>(vis);
    if (visual == nullptr)
    {
        return; // in Release SCASSERT says nothing: the visualization is either ours, or it is not there
    }

    // The aids of the placement — the four points, the axis of the dart, the line the cap is read on — and NEVER the
    // cap itself: a preview reads no curve, so a cap drawn here would be the straight one, and one tool would show two
    // different caps (see OVisToolDartCap::RefreshGeometry, and the pleat's own preview).
    visual->SetApexId(m_apexId);
    visual->SetFirstPointId(m_firstPointId);
    visual->SetSecondPointId(m_secondPointId);
    visual->SetExtendPointId(m_extendPointId);
    visual->SetSecondSense(m_secondSense);

    // The point C the cap made, when the figure carries one: the axis of the dart runs from the apex of the dart to
    // it, instead of stopping at the middle of the opening (see OVisToolDartCap::RefreshGeometry).
    visual->SetCapPointId(m_display.marks.value(0).point);

    // Which way the edge runs where the sense reads it: the axis and the line to extend are taken from it, so it is
    // asked of the reader rather than read from the file — exactly as the dialog asks it (ODialogDartCap::EdgeDirection).
    const quint32 baseId = m_secondSense ? m_secondPointId : m_firstPointId;
    const bool readable = baseId != NULL_ID && getData() != nullptr && getData()->HasObject(baseId);
    visual->SetEdgeDirection(readable ? OEdgeReader::Direction(baseId, getData()) : QPointF{});

    visual->SetMode(Mode::Show);
    visual->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    // Without this there is no way to reach the options, nor to delete the tool from the canvas.
    ShowContextMenuOf<ODialogDartCap>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SaveDialog(QDomElement &domElement)
{
    // What the dialog settled, written where the tool's element is updated IN PLACE — the path an edit takes, and
    // the one the pleat takes the same way. `SaveOptions` is for a whole file being written; this is for one tool
    // being changed, and it has to do the whole job on its own.
    const QPointer<ODialogDartCap> dialogTool = DialogOf<ODialogDartCap>();
    if (dialogTool.isNull())
    {
        return;
    }

    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();

    m_apexId = dialogTool->GetApexId();
    m_firstPointId = dialogTool->GetFirstPointId();
    m_secondPointId = dialogTool->GetSecondPointId();
    m_extendPointId = dialogTool->GetExtendPointId();
    m_secondSense = dialogTool->GetSecondSense();
    m_drawCap = dialogTool->GetDrawCap();
    m_showNotch = dialogTool->GetNotchDrawn();

    // The name of C and its label: the dialog settles both, and both live in the FIGURE — the same two lines the old
    // tool read here, kept where the operation keeps its point. Set BEFORE `RefreshFigure()` below, which is what
    // puts the name on the mark and the flag on the label.
    m_showLabel = dialogTool->GetShowLabel();
    if (not m_display.marks.isEmpty())
    {
        m_display.marks[0].name = dialogTool->GetPointName();
    }

    const OPleatDisplay dialogDisplay = dialogTool->GetDisplay();
    // The figure is not optional: the dialog decides how it looks, never whether it exists.
    m_display.show = dialogDisplay.show;
    m_display.lineTypeLine = dialogDisplay.lineTypeLine;
    m_display.lineColor = dialogDisplay.lineColor;

    // The source, and the copies the operation makes of it, written where they are read from. The figure's own
    // objects join the layer through DisplayObjectIds/ExtraLayerableObjects, which the base reads on the next save.
    m_lines = dialogTool->GetLineSources();

    // The names the file needs: a source is written with one, and the dialog hands bare ids (see NameSources). A
    // copy, because the dialog's own list is the user's selection, and the tool does not rename what he picked.
    QVector<SourceItem> namedSource = source;
    NameSources(namedSource, getData());

    SaveSourceDestination(domElement, namedSource, m_destination);

    doc->SetAttribute(domElement, AttrType, ToolType);
    doc->SetAttribute(domElement, AttrApex, m_apexId);
    doc->SetAttribute(domElement, AttrFirstPoint, m_firstPointId);
    doc->SetAttribute(domElement, AttrSecondPoint, m_secondPointId);
    doc->SetAttribute(domElement, AttrExtendPoint, m_extendPointId);
    doc->SetAttribute(domElement, AttrSecondSense, m_secondSense ? 2 : 1);
    doc->SetAttribute(domElement, AttrDrawCap, m_drawCap ? trueStr : falseStr);
    doc->SetAttribute(domElement, AttrNotch, m_showNotch ? trueStr : falseStr);
    doc->SetAttribute(domElement, AttrShowLabel, m_showLabel ? trueStr : falseStr);

    // The figure is read again from the geometry the dialog just settled, then written with it. What the read has
    // just created belongs to the active layer, like every object — the pleat's own call at the same place.
    RefreshFigure();
    AttachFigureToLayer();
    OPleatDisplay::Save(doc, domElement, m_display);

    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    // The element of an old cap is a `<point>`: the write below is what makes it the operation it became, and the
    // only place the whole element can be rebuilt at once — its attributes, its source and its figure.
    MigrateFromPointShape(tag);

    VAbstractOperation::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrApex, m_apexId);
    doc->SetAttribute(tag, AttrFirstPoint, m_firstPointId);
    doc->SetAttribute(tag, AttrSecondPoint, m_secondPointId);
    doc->SetAttribute(tag, AttrExtendPoint, m_extendPointId);
    doc->SetAttribute(tag, AttrSecondSense, m_secondSense ? 2 : 1);
    doc->SetAttribute(tag, AttrDrawCap, m_drawCap ? trueStr : falseStr);
    doc->SetAttribute(tag, AttrNotch, m_showNotch ? trueStr : falseStr);
    doc->SetAttribute(tag, AttrShowLabel, m_showLabel ? trueStr : falseStr);

    OPleatDisplay::Save(doc, tag, m_display);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractOperation::ReadToolAttributes(domElement);

    m_apexId = VAbstractPattern::GetParametrUInt(domElement, AttrApex, NULL_ID_STR);
    m_firstPointId = VAbstractPattern::GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
    m_secondPointId = VAbstractPattern::GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);
    m_extendPointId = VAbstractPattern::GetParametrUInt(domElement, AttrExtendPoint, NULL_ID_STR);
    m_secondSense = VAbstractPattern::GetParametrUInt(domElement, AttrSecondSense, u"1"_s) == 2;
    m_drawCap = VDomDocument::GetParametrBool(domElement, AttrDrawCap, trueStr);
    m_showNotch = VDomDocument::GetParametrBool(domElement, AttrNotch, falseStr);
    m_showLabel = domElement.attribute(AttrShowLabel) == trueStr;

    // The sources the element carries, named here when it gave none: every operation writes a name with a source, and
    // a file written before that was asked for — or by a tool that never filled it — must not stay unusable.
    NameSources(m_source, getData());

    m_display = OPleatDisplay::Extract(domElement);
    m_display.show = m_drawCap;

    // `Extract` forces `show` to true — right for a pleat, whose figure is always drawn, WRONG here: a dart cap can
    // have its drawing turned off, and the element says so. The attribute wins, or unticking the cap would be lost
    // on the next open.

    // Where the points are, and which way the edge runs there, are never read from the file: they are asked of the
    // container again, which is what makes the figure follow a point the user moves.
    RefreshFigure();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::MakeToolTip() const -> QString
{
    // The cap is built on three points, and a point name is never written (see OToolTip): what is left is the
    // layer the result belongs to.
    return VisibilityGroupToolTip();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    // The undo/redo path, and the pleat's own one-liner: it is the DIALOG that knows how to put the tool back into
    // the state an element describes. Written by hand — as it was — it ignored `oldDomElement` and could therefore
    // never restore anything.
    ApplyDialogOptions<ODialogDartCap>(oldDomElement, newDomElement);
}
