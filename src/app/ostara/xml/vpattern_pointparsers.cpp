/************************************************************************
 **
 **  @file   vpattern_pointparsers.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Point parsing methods extracted from vpattern.cpp
 **
 **  @note   Extracted from vpattern.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
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

#include "vpattern.h"

#include "../core/vapplication.h"
#include "../ifc/exception/vexceptionbadid.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../vmisc/def.h"
#include "../vmisc/vvalentinasettings.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../qmuparser/qmuparsererror.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezier.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezierpath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolspline.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/tooldoublepoint/vtooltruedarts.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutarc.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolalongline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolbisector.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolcurveintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolendline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolheight.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoollineintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolnormal.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolshoulderpoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolbasepoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoollineintersect.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/otoolarcpoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/otoolcurvepoint.h"
#include "../vtools/tools/drawTools/operation/otooldartcap.h"
#include "../vtools/tools/drawTools/operation/opleatdisplay.h"
#include "../vtools/tools/drawTools/toolpoint/tooldoublepoint/otoolrectangle.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromarcandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromcircleandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofcontact.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersection.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectionarcs.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncircles.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncurves.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtooltriangle.h"
#include "../vtools/tools/drawTools/vtoolline.h"
#include "../vtools/tools/nodeDetails/vabstractnode.h"
#include "../vtools/tools/nodeDetails/vnodepoint.h"
#include "../vtools/tools/nodeDetails/vtoolplacelabel.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/tools/vtooluniondetails.h"

using namespace Qt::Literals::StringLiterals;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/** @brief The axis an angle takes its direction from, unknown when the two attributes are absent.
 *
 *  The same three attributes on every <point>: the tools that offer the feature read them, the others simply
 *  find none. The sense is stored as a name (line, lineReverse, …) so the file says what it means. */
auto ReadPointAxis(const QDomElement &element) -> OAxis
{
    const quint32 p1 = VDomDocument::GetParametrUInt(element, AttrAxisP1, NULL_ID_STR);
    const quint32 p2 = VDomDocument::GetParametrUInt(element, AttrAxisP2, NULL_ID_STR);
    const QString sense = VDomDocument::GetParametrEmptyString(element, AttrAxisSense);

    return {p1, p2, OAxis::SenseFromName(sense)};
}
} // namespace


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParsePointElement parse point tag.
 * @param scene scene.
 * @param domElement tag in xml tree.
 * @param parse parser file mode.
 * @param type type of point.
 */
void VPattern::ParsePointElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                                 const QString &type)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");
    Q_ASSERT_X(not type.isEmpty(), Q_FUNC_INFO, "type of point is empty");

    static const QStringList points({VToolBasePoint::ToolType,                  /*0*/
                                     VToolEndLine::ToolType,                    /*1*/
                                     VToolAlongLine::ToolType,                  /*2*/
                                     VToolShoulderPoint::ToolType,              /*3*/
                                     VToolNormal::ToolType,                     /*4*/
                                     VToolBisector::ToolType,                   /*5*/
                                     VToolLineIntersect::ToolType,              /*6*/
                                     VToolPointOfContact::ToolType,             /*7*/
                                     VNodePoint::ToolType,                      /*8*/
                                     VToolHeight::ToolType,                     /*9*/
                                     VToolTriangle::ToolType,                   /*10*/
                                     VToolPointOfIntersection::ToolType,        /*11*/
                                     VToolCutSpline::ToolType,                  /*12*/
                                     VToolCutSplinePath::ToolType,              /*13*/
                                     VToolCutArc::ToolType,                     /*14*/
                                     VToolLineIntersectAxis::ToolType,          /*15*/
                                     VToolCurveIntersectAxis::ToolType,         /*16*/
                                     VToolPointOfIntersectionArcs::ToolType,    /*17*/
                                     VToolPointOfIntersectionCircles::ToolType, /*18*/
                                     VToolPointFromCircleAndTangent::ToolType,  /*19*/
                                     VToolPointFromArcAndTangent::ToolType,     /*20*/
                                     VToolTrueDarts::ToolType,                  /*21*/
                                     VToolPointOfIntersectionCurves::ToolType,  /*22*/
                                     // Slot 23 ("pin"): the pin node type was removed in Ostara. The entry is kept so
                                     // the hard-coded case numbers below stay aligned with this list — shifting them
                                     // would silently break placeLabel / arcPoint / curvePoint parsing.
                                     QStringLiteral("pin"),                      /*23*/
                                     VToolPlaceLabel::ToolType,                  /*24*/
                                     OToolArcPoint::ToolTypeStart,               /*25*/
                                     OToolArcPoint::ToolTypeEnd,                 /*26*/
                                     OToolCurvePoint::ToolTypeStart,             /*27*/
                                     OToolCurvePoint::ToolTypeEnd,
                                     OToolDartCap::ToolType,               /*28*/
                                     OToolRectangle::ToolType});           /*29*/
    switch (points.indexOf(type))
    {
        case 0: // VToolBasePoint::ToolType
            ParseToolBasePoint(scene, domElement, parse);
            break;
        case 1: // VToolEndLine::ToolType
            ParseToolEndLine(scene, domElement, parse);
            break;
        case 2: // VToolAlongLine::ToolType
            ParseToolAlongLine(scene, domElement, parse);
            break;
        case 3: // VToolShoulderPoint::ToolType
            ParseToolShoulderPoint(scene, domElement, parse);
            break;
        case 4: // VToolNormal::ToolType
            ParseToolNormal(scene, domElement, parse);
            break;
        case 5: // VToolBisector::ToolType
            ParseToolBisector(scene, domElement, parse);
            break;
        case 6: // VToolLineIntersect::ToolType
            ParseToolLineIntersect(scene, domElement, parse);
            break;
        case 7: // VToolPointOfContact::ToolType
            ParseToolPointOfContact(scene, domElement, parse);
            break;
        case 8: // VNodePoint::ToolType
            ParseNodePoint(domElement, parse);
            break;
        case 9: // VToolHeight::ToolType
            ParseToolHeight(scene, domElement, parse);
            break;
        case 10: // VToolTriangle::ToolType
            ParseToolTriangle(scene, domElement, parse);
            break;
        case 11: // VToolPointOfIntersection::ToolType
            ParseToolPointOfIntersection(scene, domElement, parse);
            break;
        case 12: // VToolCutSpline::ToolType
            ParseToolCutSpline(scene, domElement, parse);
            break;
        case 13: // VToolCutSplinePath::ToolType
            ParseToolCutSplinePath(scene, domElement, parse);
            break;
        case 14: // VToolCutArc::ToolType
            ParseToolCutArc(scene, domElement, parse);
            break;
        case 15: // VToolLineIntersectAxis::ToolType
            ParseToolLineIntersectAxis(scene, domElement, parse);
            break;
        case 16: // VToolCurveIntersectAxis::ToolType
            ParseToolCurveIntersectAxis(scene, domElement, parse);
            break;
        case 17: // VToolPointOfIntersectionArcs::ToolType
            ParseToolPointOfIntersectionArcs(scene, domElement, parse);
            break;
        case 18: // VToolPointOfIntersectionCircles::ToolType
            ParseToolPointOfIntersectionCircles(scene, domElement, parse);
            break;
        case 19: // VToolPointFromCircleAndTangent::ToolType
            ParseToolPointFromCircleAndTangent(scene, domElement, parse);
            break;
        case 20: // VToolPointFromArcAndTangent::ToolType
            ParseToolPointFromArcAndTangent(scene, domElement, parse);
            break;
        case 21: // VToolTrueDarts::ToolType
            ParseToolTrueDarts(scene, domElement, parse);
            break;
        case 22: // VToolPointOfIntersectionCurves::ToolType
            ParseToolPointOfIntersectionCurves(scene, domElement, parse);
            break;
        case 23: // "pin" — legacy node type, removed in Ostara. Skipped silently: raising here would make any
                 // legacy .val containing pins impossible to open.
            break;
        case 24: // VToolPlaceLabel::ToolType
            ParsePlaceLabel(domElement, parse);
            break;
        case 25: // OToolArcPoint::ToolTypeStart
            ParseToolArcPoint(scene, domElement, parse, true);
            break;
        case 26: // OToolArcPoint::ToolTypeEnd
            ParseToolArcPoint(scene, domElement, parse, false);
            break;
        case 27: // OToolCurvePoint::ToolTypeStart
            ParseToolCurvePoint(scene, domElement, parse, true);
            break;
        case 28: // OToolCurvePoint::ToolTypeEnd
            ParseToolCurvePoint(scene, domElement, parse, false);
            break;
        case 29: // OToolDartCap::ToolType
            ParseToolDartCap(scene, domElement, parse);
            break;
        case 30: // OToolRectangle::ToolType
            ParseToolRectangle(scene, domElement, parse);
            break;
        default:
            throw VException(tr("Unknown point type '%1'.").arg(type));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseLineElement parse line tag.
 * @param scene scene.
 * @param domElement tag in xml tree.
 * @param parse parser file mode.
 */
void VPattern::ParseLineElement(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");
    try
    {
        VToolLineInitData initData;
        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.firstPoint = GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
        initData.secondPoint = GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);
        initData.typeLine = GetParametrString(domElement, AttrTypeLine, TypeLineLine);
        initData.lineColor = GetParametrString(domElement, AttrLineColor, ColorBlack);
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        VToolLine::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating line"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseLineNamesElement(const QDomElement &domElement)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    const QDomNodeList lineNodes = domElement.elementsByTagName(VDomDocument::TagLine);
    QSet<QString> seenSuffixes;
    for (int i = 0; i < lineNodes.count(); ++i)
    {
        const QDomElement lineEl = lineNodes.at(i).toElement();
        if (lineEl.isNull())
        {
            continue;
        }

        const QString points = lineEl.attribute(AttrPoints);
        const QString name = lineEl.attribute(AttrName);
        if (points.isEmpty() || name.isEmpty())
        {
            continue;
        }

        // Reject duplicate line RoleNames at parse time — a suffix reused by a
        // different pairKey would cause data corruption in d->variables.
        if (seenSuffixes.contains(name))
        {
            qCCritical(vXML, "%s", qUtf8Printable(tr("Duplicate line role name '%1' in <lineNames>; "
                                                     "second occurrence ignored.")
                                                      .arg(name)));
            continue;
        }
        seenSuffixes.insert(name);

        const QStringList pointNames = points.split(' ', Qt::SkipEmptyParts);
        if (pointNames.size() != 2)
        {
            continue;
        }

        // The two names are handed over as they were read, and not only joined: this runs **before** the blocks,
        // so no object exists yet to recover the pair from — and a name may hold an underscore.
        data->SetLineRoleName(pointNames.at(0), pointNames.at(1), name);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SyncLineNamesToDom()
{
    QDomElement root = documentElement();
    if (root.isNull())
    {
        return;
    }

    // Find or create the <lineNames> element
    QDomElement lineNamesElement = root.firstChildElement(TagLineNames);
    const bool hasNames = !data->LineRoleNames().isEmpty();

    if (!hasNames)
    {
        // Remove existing <lineNames> if the map is empty
        if (!lineNamesElement.isNull())
        {
            root.removeChild(lineNamesElement);
        }
        return;
    }

    // Create <lineNames> if it doesn't exist
    if (lineNamesElement.isNull())
    {
        // **Placed, not inserted before the first <draw>.** The `<lineNames>` element comes before
        // the draws in the schema, and the pattern is the one that knows that — the same answer the
        // other writers get, so a fifth one cannot get it wrong.
        lineNamesElement = createElement(TagLineNames);
        InsertRootElement(lineNamesElement);
    }

    // Remove all existing <line> children
    while (!lineNamesElement.firstChildElement(VDomDocument::TagLine).isNull())
    {
        lineNamesElement.removeChild(lineNamesElement.firstChildElement(VDomDocument::TagLine));
    }

    // Rebuild from the map
    const auto &roleNames = data->LineRoleNames();
    for (auto it = roleNames.cbegin(); it != roleNames.cend(); ++it)
    {
        // The pair as the container holds it. Splitting the key is only the fallback for a role declared by a
        // caller that had nothing but the key: a name may contain '_' — the import of a piece marks names with
        // `__P<n>` — and the split would then throw the entry away, silently, at this very save.
        QPair<QString, QString> pair = data->LineRolePair(it.key());
        if (pair.first.isEmpty())
        {
            const QStringList pointNames = it.key().split('_');
            if (pointNames.size() != 2)
            {
                continue;
            }
            pair = {pointNames.at(0), pointNames.at(1)};
        }

        QDomElement lineEl = createElement(VDomDocument::TagLine);
        lineEl.setAttribute(AttrPoints, pair.first + ' ' + pair.second);
        lineEl.setAttribute(AttrName, it.value());
        lineNamesElement.appendChild(lineEl);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseCurrentPP re-reads the active pattern block alone, against the living container. Cheap enough to
 * run on every mouse move of a base-point drag, which is the whole point of it — and the reason the rest of the
 * file is caught up later, see VPattern::RefreshCrossBlockDependents.
 */
void VPattern::ParseCurrentPP()
{
    QDomElement const domElement = PatternBlockMapper()->GetActiveElement();
    if (domElement.isNull())
    {
        return;
    }

    ParseDrawElement(domElement, Document::LiteParse);
    emit CheckLayout();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RefreshCrossBlockDependents re-reads the whole file when a tool of another piece is built on what the
 * active block holds.
 *
 * The cheap parse above covers the active block only, and a base-point drag runs it on every mouse move. A
 * transfer copies a portion of another piece, a formula in a later piece can name a variable of this one: for
 * those, the block that was just re-read is only half the story, and nothing said so — the copies simply stayed
 * where they were. The whole file is re-read here, once, after the gesture has settled. Not the blocks the graph
 * names: reading one of them needs every block before it in the file back in the container, and the walk is what
 * puts it there.
 */
void VPattern::RefreshCrossBlockDependents()
{
    m_crossBlockParseTimer->stop();

    const std::optional<QSet<int>> blocks = PatternGraph()->TryGetDependentBlocks(PatternBlockMapper()->GetActiveId());
    const bool dependentsOutside = blocks.has_value() && !blocks->isEmpty();

    // A transfer the cheap parse could not build is a reason of its own: its sources are in a piece read LATER,
    // so nothing outside the active block depends on it and the graph alone would not ask for the whole file —
    // while the block's own copies stay where they were. The deferred list says it happened.
    if (!dependentsOutside && m_deferredTransfers.isEmpty())
    {
        // Nothing outside this block reads it: the cheap parse was the whole story.
        return;
    }

    LiteParseTree(Document::LiteParse);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolBasePoint(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    VToolBasePoint *spoint = nullptr;
    try
    {
        VToolBasePointInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.x =
            VAbstractValApplication::VApp()->toPixel(GetParametrDouble(domElement, AttrX, QStringLiteral("10.0")));
        initData.y =
            VAbstractValApplication::VApp()->toPixel(GetParametrDouble(domElement, AttrY, QStringLiteral("10.0")));

        spoint = VToolBasePoint::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating single point"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        scene->removeItem(spoint);
        delete spoint;
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolEndLine(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolEndLineInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsWithLineCommonAttributes(domElement, initData);

        initData.formulaLength = GetParametrString(domElement, AttrLength, QStringLiteral("100.0"));
        const QString f = initData.formulaLength; // need for saving fixed formula;

        initData.basePointId = GetParametrUInt(domElement, AttrBasePoint, NULL_ID_STR);

        initData.formulaAngle = GetParametrString(domElement, AttrAngle, QStringLiteral("0.0"));
        const QString angleFix = initData.formulaAngle;
        initData.axis = ReadPointAxis(domElement);

        VToolEndLine::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (f != initData.formulaLength || angleFix != initData.formulaAngle)
        {
            SetAttribute(domElement, AttrLength, initData.formulaLength);
            SetAttribute(domElement, AttrAngle, initData.formulaAngle);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of end line"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of end line"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolAlongLine(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wnoexcept")

        VToolAlongLineInitData initData;

        QT_WARNING_POP

        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsWithLineCommonAttributes(domElement, initData);
        initData.formula = GetParametrString(domElement, AttrLength, QStringLiteral("100.0"));
        const QString f = initData.formula; // need for saving fixed formula;
        initData.firstPointId = GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
        initData.secondPointId = GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);

        VToolAlongLine::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (f != initData.formula)
        {
            SetAttribute(domElement, AttrLength, initData.formula);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point along line"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point along line"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolShoulderPoint(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolShoulderPointInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsWithLineCommonAttributes(domElement, initData);
        initData.formula = GetParametrString(domElement, AttrLength, QStringLiteral("100.0"));
        const QString f = initData.formula; // need for saving fixed formula;
        initData.p1Line = GetParametrUInt(domElement, AttrP1Line, NULL_ID_STR);
        initData.p2Line = GetParametrUInt(domElement, AttrP2Line, NULL_ID_STR);
        initData.pShoulder = GetParametrUInt(domElement, AttrPShoulder, NULL_ID_STR);

        VToolShoulderPoint::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (f != initData.formula)
        {
            SetAttribute(domElement, AttrLength, initData.formula);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of shoulder"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of shoulder"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolNormal(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolNormalInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsWithLineCommonAttributes(domElement, initData);
        initData.formula = GetParametrString(domElement, AttrLength, QStringLiteral("100.0"));
        const QString f = initData.formula; // need for saving fixed formula;
        initData.firstPointId = GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
        initData.secondPointId = GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);
        initData.angle = GetParametrDouble(domElement, AttrAngle, QStringLiteral("0.0"));

        VToolNormal::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (f != initData.formula)
        {
            SetAttribute(domElement, AttrLength, initData.formula);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of normal"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of normal"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolBisector(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolBisectorInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsWithLineCommonAttributes(domElement, initData);
        initData.formula = GetParametrString(domElement, AttrLength, QStringLiteral("100.0"));
        const QString f = initData.formula; // need for saving fixed formula;
        initData.firstPointId = GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
        initData.secondPointId = GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);
        initData.thirdPointId = GetParametrUInt(domElement, AttrThirdPoint, NULL_ID_STR);

        VToolBisector::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (f != initData.formula)
        {
            SetAttribute(domElement, AttrLength, initData.formula);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of bisector"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of bisector"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolLineIntersect(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolLineIntersectInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.p1Line1Id = GetParametrUInt(domElement, AttrP1Line1, NULL_ID_STR);
        initData.p2Line1Id = GetParametrUInt(domElement, AttrP2Line1, NULL_ID_STR);
        initData.p1Line2Id = GetParametrUInt(domElement, AttrP1Line2, NULL_ID_STR);
        initData.p2Line2Id = GetParametrUInt(domElement, AttrP2Line2, NULL_ID_STR);

        VToolLineIntersect::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of line intersection"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolPointOfContact(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolPointOfContactInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.radius = GetParametrString(domElement, AttrRadius, QChar('0'));
        const QString f = initData.radius; // need for saving fixed formula;
        initData.center = GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
        initData.firstPointId = GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
        initData.secondPointId = GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);

        VToolPointOfContact::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (f != initData.radius)
        {
            SetAttribute(domElement, AttrRadius, initData.radius);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of contact"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of contact"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseNodePoint(const QDomElement &domElement, const Document &parse)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VAbstractNodeInitData initData;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;
        initData.scene = sceneDetail;

        PointsCommonAttributes(domElement, initData.id, initData.mx, initData.my);
        initData.idObject = GetParametrUInt(domElement, AttrIdObject, NULL_ID_STR);
        initData.idTool = GetParametrUInt(domElement, VAbstractNode::AttrIdTool, NULL_ID_STR);
        initData.showLabel = GetParametrBool(domElement, AttrShowLabel, falseStr);
        initData.showLabelExplicit = domElement.hasAttribute(AttrShowLabel);

        VNodePoint::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating modeling point"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}



//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParsePlaceLabel(QDomElement &domElement, const Document &parse)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolPlaceLabelInitData initData;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        ToolsCommonAttributes(domElement, initData.id);
        initData.centerPoint = GetParametrUInt(domElement, AttrIdObject, NULL_ID_STR);

        try
        {
            initData.data->GeometricObject<VPointF>(initData.centerPoint);
        }
        catch (const VExceptionBadId &)
        { // Possible case. Parent was deleted, but the node object is still here.
            qDebug() << "Broken relation. Parent was deleted, but the place label object is still here. Place label "
                        "id ="
                     << initData.id << ".";
            return; // Just ignore
        }

        initData.idTool = GetParametrUInt(domElement, VAbstractNode::AttrIdTool, NULL_ID_STR);

        initData.width = GetParametrString(domElement, AttrWidth, QStringLiteral("1.0"));
        const QString w = initData.width; // need for saving fixed formula;

        initData.height = GetParametrString(domElement, AttrHeight, QStringLiteral("1.0"));
        const QString h = initData.height; // need for saving fixed formula;

        initData.angle = GetParametrString(domElement, AttrAngle, QStringLiteral("0.0"));
        const QString angle = initData.angle; // need for saving fixed formula;

        initData.visibilityTrigger =
            GetParametrString(domElement, VAbstractPattern::AttrVisible, QStringLiteral("1.0"));
        const QString visibility = initData.visibilityTrigger; // need for saving fixed formula;

        initData.type = static_cast<PlaceLabelType>(GetParametrUInt(domElement, AttrPlaceLabelType, QChar('0')));

        initData.notMirrored = GetParametrBool(domElement, AttrNotMirrored, falseStr);

        VToolPlaceLabel::Create(initData);

        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (w != initData.width || h != initData.height || angle != initData.angle ||
            visibility != initData.visibilityTrigger)
        {
            SetAttribute(domElement, AttrWidth, initData.width);
            SetAttribute(domElement, AttrHeight, initData.height);
            SetAttribute(domElement, AttrAngle, initData.angle);
            SetAttribute(domElement, VAbstractPattern::AttrVisible, initData.visibilityTrigger);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating place label"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating place label"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}


//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolHeight(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolHeightInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsWithLineCommonAttributes(domElement, initData);
        initData.basePointId = GetParametrUInt(domElement, AttrBasePoint, NULL_ID_STR);
        initData.p1LineId = GetParametrUInt(domElement, AttrP1Line, NULL_ID_STR);
        initData.p2LineId = GetParametrUInt(domElement, AttrP2Line, NULL_ID_STR);

        VToolHeight::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating height"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolTriangle(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolTriangleInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.axisP1Id = GetParametrUInt(domElement, AttrAxisP1, NULL_ID_STR);
        initData.axisP2Id = GetParametrUInt(domElement, AttrAxisP2, NULL_ID_STR);
        initData.firstPointId = GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
        initData.secondPointId = GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);

        VToolTriangle::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating triangle"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolPointOfIntersection(VMainGraphicsScene *scene, const QDomElement &domElement,
                                            const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolPointOfIntersectionInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.firstPointId = GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
        initData.secondPointId = GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);

        VToolPointOfIntersection::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of intersection"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolCutSpline(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolCutInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.formula = GetParametrString(domElement, AttrLength, QChar('0'));
        const QString f = initData.formula; // need for saving fixed formula;
        initData.baseCurveId = GetParametrUInt(domElement, VToolCutSpline::AttrSpline, NULL_ID_STR);
        initData.aliasSuffix1 = GetParametrEmptyString(domElement, AttrAlias1);
        initData.aliasSuffix2 = GetParametrEmptyString(domElement, AttrAlias2);
        initData.segment1Id = GetParametrId(domElement, AttrSegment1Id);
        initData.segment2Id = GetParametrId(domElement, AttrSegment2Id);
        initData.showTwoSegments = GetParametrBool(domElement, AttrShowTwoSegments, falseStr);
        initData.lineType = GetParametrString(domElement, AttrTypeLine, TypeLineLine);
        initData.lineColor = GetParametrString(domElement, AttrLineColor, ColorBlack);

        const VAbstractValApplication *app = VAbstractValApplication::VApp();

        const QString labelMXStr = QString::number(app->fromPixel(labelMX));
        const QString labelMYStr = QString::number(app->fromPixel(labelMY));

        initData.segment1Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment1Mx, labelMXStr));
        initData.segment1My = app->toPixel(GetParametrDouble(domElement, AttrSegment1My, labelMYStr));
        initData.segment2Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment2Mx, labelMXStr));
        initData.segment2My = app->toPixel(GetParametrDouble(domElement, AttrSegment2My, labelMYStr));

        // Make name1 and name2 mandatory
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        initData.name1 = GetParametrEmptyString(domElement, AttrCurveName1); // Optionally empty for compatibility
        initData.name2 = GetParametrEmptyString(domElement, AttrCurveName2); // Optionally empty for compatibility

        VToolCutSpline::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (f != initData.formula)
        {
            SetAttribute(domElement, AttrLength, initData.formula);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating cut spline point"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating cut spline point"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolCutSplinePath(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolCutInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.formula = GetParametrString(domElement, AttrLength, QChar('0'));
        const QString f = initData.formula; // need for saving fixed formula;
        initData.baseCurveId = GetParametrUInt(domElement, VToolCutSplinePath::AttrSplinePath, NULL_ID_STR);
        initData.aliasSuffix1 = GetParametrEmptyString(domElement, AttrAlias1);
        initData.aliasSuffix2 = GetParametrEmptyString(domElement, AttrAlias2);
        initData.segment1Id = GetParametrId(domElement, AttrSegment1Id);
        initData.segment2Id = GetParametrId(domElement, AttrSegment2Id);
        initData.showTwoSegments = GetParametrBool(domElement, AttrShowTwoSegments, falseStr);
        initData.lineType = GetParametrString(domElement, AttrTypeLine, TypeLineLine);
        initData.lineColor = GetParametrString(domElement, AttrLineColor, ColorBlack);

        const VAbstractValApplication *app = VAbstractValApplication::VApp();

        const QString labelMXStr = QString::number(app->fromPixel(labelMX));
        const QString labelMYStr = QString::number(app->fromPixel(labelMY));

        initData.segment1Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment1Mx, labelMXStr));
        initData.segment1My = app->toPixel(GetParametrDouble(domElement, AttrSegment1My, labelMYStr));
        initData.segment2Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment2Mx, labelMXStr));
        initData.segment2My = app->toPixel(GetParametrDouble(domElement, AttrSegment2My, labelMYStr));

        // Make name1 and name2 mandatory
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        initData.name1 = GetParametrEmptyString(domElement, AttrCurveName1); // Optionally empty for compatibility
        initData.name2 = GetParametrEmptyString(domElement, AttrCurveName2); // Optionally empty for compatibility

        VToolCutSplinePath::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (f != initData.formula)
        {
            SetAttribute(domElement, AttrLength, initData.formula);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating cut spline path point"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating cut spline path point"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolCutArc(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolCutInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.formula = GetParametrString(domElement, AttrLength, QChar('0'));
        const QString f = initData.formula; // need for saving fixed formula;
        initData.baseCurveId = GetParametrUInt(domElement, AttrArc, NULL_ID_STR);
        initData.aliasSuffix1 = GetParametrEmptyString(domElement, AttrAlias1);
        initData.aliasSuffix2 = GetParametrEmptyString(domElement, AttrAlias2);
        initData.segment1Id = GetParametrId(domElement, AttrSegment1Id);
        initData.segment2Id = GetParametrId(domElement, AttrSegment2Id);
        initData.showTwoSegments = GetParametrBool(domElement, AttrShowTwoSegments, falseStr);
        initData.lineType = GetParametrString(domElement, AttrTypeLine, TypeLineLine);
        initData.lineColor = GetParametrString(domElement, AttrLineColor, ColorBlack);

        const VAbstractValApplication *app = VAbstractValApplication::VApp();

        const QString labelMXStr = QString::number(app->fromPixel(labelMX));
        const QString labelMYStr = QString::number(app->fromPixel(labelMY));

        initData.segment1Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment1Mx, labelMXStr));
        initData.segment1My = app->toPixel(GetParametrDouble(domElement, AttrSegment1My, labelMYStr));
        initData.segment2Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment2Mx, labelMXStr));
        initData.segment2My = app->toPixel(GetParametrDouble(domElement, AttrSegment2My, labelMYStr));

        // Make name1 and name2 mandatory
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        initData.name1 = GetParametrEmptyString(domElement, AttrCurveName1); // Optionally empty for compatibility
        initData.name2 = GetParametrEmptyString(domElement, AttrCurveName2); // Optionally empty for compatibility

        VToolCutArc::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (f != initData.formula)
        {
            SetAttribute(domElement, AttrLength, initData.formula);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating cut arc point"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating cut arc point"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolLineIntersectAxis(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolLineIntersectAxisInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsWithLineCommonAttributes(domElement, initData);

        initData.basePointId = GetParametrUInt(domElement, AttrBasePoint, NULL_ID_STR);
        initData.firstPointId = GetParametrUInt(domElement, AttrP1Line, NULL_ID_STR);
        initData.secondPointId = GetParametrUInt(domElement, AttrP2Line, NULL_ID_STR);

        initData.formulaAngle = GetParametrString(domElement, AttrAngle, QStringLiteral("0.0"));
        const QString angleFix = initData.formulaAngle;
        initData.axis = ReadPointAxis(domElement);

        VToolLineIntersectAxis::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (angleFix != initData.formulaAngle)
        {
            SetAttribute(domElement, AttrAngle, initData.formulaAngle);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of intersection line and axis"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of intersection line and axis"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolCurveIntersectAxis(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wnoexcept")

        VToolCurveIntersectAxisInitData initData;

        QT_WARNING_POP

        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsWithLineCommonAttributes(domElement, initData);

        initData.basePointId = GetParametrUInt(domElement, AttrBasePoint, NULL_ID_STR);
        initData.curveId = GetParametrUInt(domElement, AttrCurve, NULL_ID_STR);
        initData.formulaAngle = GetParametrString(domElement, AttrAngle, QStringLiteral("0.0"));
        const QString angleFix = initData.formulaAngle;
        initData.axis = ReadPointAxis(domElement);
        initData.name1 = GetParametrEmptyString(domElement, AttrName1);
        initData.name2 = GetParametrEmptyString(domElement, AttrName2);
        initData.aliasSuffix1 = GetParametrEmptyString(domElement, AttrAlias1);
        initData.aliasSuffix2 = GetParametrEmptyString(domElement, AttrAlias2);
        initData.segment1Id = GetParametrId(domElement, AttrSegment1Id);
        initData.segment2Id = GetParametrId(domElement, AttrSegment2Id);

        const VAbstractValApplication *app = VAbstractValApplication::VApp();

        const QString labelMXStr = QString::number(app->fromPixel(labelMX));
        const QString labelMYStr = QString::number(app->fromPixel(labelMY));

        initData.segment1Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment1Mx, labelMXStr));
        initData.segment1My = app->toPixel(GetParametrDouble(domElement, AttrSegment1My, labelMYStr));
        initData.segment2Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment2Mx, labelMXStr));
        initData.segment2My = app->toPixel(GetParametrDouble(domElement, AttrSegment2My, labelMYStr));

        VToolCurveIntersectAxis::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (angleFix != initData.formulaAngle)
        {
            SetAttribute(domElement, AttrAngle, initData.formulaAngle);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of intersection curve and axis"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of intersection curve and axis"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolPointOfIntersectionArcs(VMainGraphicsScene *scene, const QDomElement &domElement,
                                                const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolPointOfIntersectionArcsInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.firstArcId = GetParametrUInt(domElement, AttrFirstArc, NULL_ID_STR);
        initData.secondArcId = GetParametrUInt(domElement, AttrSecondArc, NULL_ID_STR);
        initData.pType = static_cast<CrossCirclesPoint>(GetParametrUInt(domElement, AttrCrossPoint, QChar('1')));
        initData.arc1Name1 = GetParametrEmptyString(domElement, AttrCurve1Name1);
        initData.arc1Name2 = GetParametrEmptyString(domElement, AttrCurve1Name2);
        initData.arc2Name1 = GetParametrEmptyString(domElement, AttrCurve2Name1);
        initData.arc2Name2 = GetParametrEmptyString(domElement, AttrCurve2Name2);
        initData.arc1AliasSuffix1 = GetParametrEmptyString(domElement, AttrCurve1Alias1);
        initData.arc1AliasSuffix2 = GetParametrEmptyString(domElement, AttrCurve1Alias2);
        initData.arc2AliasSuffix1 = GetParametrEmptyString(domElement, AttrCurve2Alias1);
        initData.arc2AliasSuffix2 = GetParametrEmptyString(domElement, AttrCurve2Alias2);
        initData.arc1Segment1Id = GetParametrId(domElement, AttrSegment1Id);
        initData.arc1Segment2Id = GetParametrId(domElement, AttrSegment2Id);
        initData.arc2Segment1Id = GetParametrId(domElement, AttrSegment3Id);
        initData.arc2Segment2Id = GetParametrId(domElement, AttrSegment4Id);

        const VAbstractValApplication *app = VAbstractValApplication::VApp();

        const QString labelMXStr = QString::number(app->fromPixel(labelMX));
        const QString labelMYStr = QString::number(app->fromPixel(labelMY));

        initData.arc1Segment1Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment1Mx, labelMXStr));
        initData.arc1Segment1My = app->toPixel(GetParametrDouble(domElement, AttrSegment1My, labelMYStr));
        initData.arc1Segment2Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment2Mx, labelMXStr));
        initData.arc1Segment2My = app->toPixel(GetParametrDouble(domElement, AttrSegment2My, labelMYStr));
        initData.arc2Segment1Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment3Mx, labelMXStr));
        initData.arc2Segment1My = app->toPixel(GetParametrDouble(domElement, AttrSegment3My, labelMYStr));
        initData.arc2Segment2Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment4Mx, labelMXStr));
        initData.arc2Segment2My = app->toPixel(GetParametrDouble(domElement, AttrSegment4My, labelMYStr));

        VToolPointOfIntersectionArcs::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of intersection arcs"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolPointOfIntersectionCircles(VMainGraphicsScene *scene, QDomElement &domElement,
                                                   const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolPointOfIntersectionCirclesInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.firstCircleCenterId = GetParametrUInt(domElement, AttrC1Center, NULL_ID_STR);
        initData.secondCircleCenterId = GetParametrUInt(domElement, AttrC2Center, NULL_ID_STR);
        initData.firstCircleRadius = GetParametrString(domElement, AttrC1Radius);
        const QString c1R = initData.firstCircleRadius;
        initData.secondCircleRadius = GetParametrString(domElement, AttrC2Radius);
        const QString c2R = initData.secondCircleRadius;
        initData.crossPoint = static_cast<CrossCirclesPoint>(GetParametrUInt(domElement, AttrCrossPoint, QChar('1')));

        VToolPointOfIntersectionCircles::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (c1R != initData.firstCircleRadius || c2R != initData.secondCircleRadius)
        {
            SetAttribute(domElement, AttrC1Radius, initData.firstCircleRadius);
            SetAttribute(domElement, AttrC2Radius, initData.secondCircleRadius);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of intersection circles"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolPointOfIntersectionCurves(VMainGraphicsScene *scene, QDomElement &domElement,
                                                  const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        QT_WARNING_PUSH
        QT_WARNING_DISABLE_GCC("-Wnoexcept")

        VToolPointOfIntersectionCurvesInitData initData;

        QT_WARNING_POP

        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.firstCurveId = GetParametrUInt(domElement, AttrCurve1, NULL_ID_STR);
        initData.secondCurveId = GetParametrUInt(domElement, AttrCurve2, NULL_ID_STR);
        initData.vCrossPoint = static_cast<VCrossCurvesPoint>(GetParametrUInt(domElement, AttrVCrossPoint, QChar('1')));
        initData.hCrossPoint = static_cast<HCrossCurvesPoint>(GetParametrUInt(domElement, AttrHCrossPoint, QChar('1')));
        initData.curve1Name1 = GetParametrEmptyString(domElement, AttrCurve1Name1);
        initData.curve1Name2 = GetParametrEmptyString(domElement, AttrCurve1Name2);
        initData.curve2Name1 = GetParametrEmptyString(domElement, AttrCurve2Name1);
        initData.curve2Name2 = GetParametrEmptyString(domElement, AttrCurve2Name2);
        initData.curve1AliasSuffix1 = GetParametrEmptyString(domElement, AttrCurve1Alias1);
        initData.curve1AliasSuffix2 = GetParametrEmptyString(domElement, AttrCurve1Alias2);
        initData.curve2AliasSuffix1 = GetParametrEmptyString(domElement, AttrCurve2Alias1);
        initData.curve2AliasSuffix2 = GetParametrEmptyString(domElement, AttrCurve2Alias2);
        initData.curve1Segment1Id = GetParametrId(domElement, AttrSegment1Id);
        initData.curve1Segment2Id = GetParametrId(domElement, AttrSegment2Id);
        initData.curve2Segment1Id = GetParametrId(domElement, AttrSegment3Id);
        initData.curve2Segment2Id = GetParametrId(domElement, AttrSegment4Id);

        const VAbstractValApplication *app = VAbstractValApplication::VApp();

        const QString labelMXStr = QString::number(app->fromPixel(labelMX));
        const QString labelMYStr = QString::number(app->fromPixel(labelMY));

        initData.curve1Segment1Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment1Mx, labelMXStr));
        initData.curve1Segment1My = app->toPixel(GetParametrDouble(domElement, AttrSegment1My, labelMYStr));
        initData.curve1Segment2Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment2Mx, labelMXStr));
        initData.curve1Segment2My = app->toPixel(GetParametrDouble(domElement, AttrSegment2My, labelMYStr));
        initData.curve2Segment1Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment3Mx, labelMXStr));
        initData.curve2Segment1My = app->toPixel(GetParametrDouble(domElement, AttrSegment3My, labelMYStr));
        initData.curve2Segment2Mx = app->toPixel(GetParametrDouble(domElement, AttrSegment4Mx, labelMXStr));
        initData.curve2Segment2My = app->toPixel(GetParametrDouble(domElement, AttrSegment4My, labelMYStr));

        VToolPointOfIntersectionCurves::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point of intersection curves"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolPointFromCircleAndTangent(VMainGraphicsScene *scene, QDomElement &domElement,
                                                  const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolPointFromCircleAndTangentInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.circleCenterId = GetParametrUInt(domElement, AttrCCenter, NULL_ID_STR);
        initData.tangentPointId = GetParametrUInt(domElement, AttrTangent, NULL_ID_STR);
        initData.circleRadius = GetParametrString(domElement, AttrCRadius);
        const QString cR = initData.circleRadius;
        initData.crossPoint = static_cast<CrossCirclesPoint>(GetParametrUInt(domElement, AttrCrossPoint, QChar('1')));

        VToolPointFromCircleAndTangent::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (cR != initData.circleRadius)
        {
            SetAttribute(domElement, AttrCRadius, initData.circleRadius);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point from circle and tangent"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolPointFromArcAndTangent(VMainGraphicsScene *scene, const QDomElement &domElement,
                                               const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolPointFromArcAndTangentInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.arcId = GetParametrUInt(domElement, AttrArc, NULL_ID_STR);
        initData.tangentPointId = GetParametrUInt(domElement, AttrTangent, NULL_ID_STR);
        initData.crossPoint = static_cast<CrossCirclesPoint>(GetParametrUInt(domElement, AttrCrossPoint, QChar('1')));

        VToolPointFromArcAndTangent::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating point from arc and tangent"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolArcPoint(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse,
                                 bool isStart)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        OToolArcPointInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.arcId = GetParametrUInt(domElement, AttrArc, NULL_ID_STR);
        initData.position = isStart ? ArcPointPosition::Start : ArcPointPosition::End;

        OToolArcPoint::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating arc point"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolCurvePoint(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse,
                                   bool isStart)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        OToolCurvePointInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        PointsCommonAttributes(domElement, initData);
        initData.splineId = GetParametrUInt(domElement, AttrSpline, NULL_ID_STR);
        initData.position = isStart ? CurvePointPosition::Start : CurvePointPosition::End;

        OToolCurvePoint::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating curve point"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolTrueDarts(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolTrueDartsInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);

        initData.p1id = GetParametrUInt(domElement, AttrPoint1, NULL_ID_STR);
        initData.p2id = GetParametrUInt(domElement, AttrPoint2, NULL_ID_STR);

        initData.baseLineP1Id = GetParametrUInt(domElement, AttrBaseLineP1, NULL_ID_STR);
        initData.baseLineP2Id = GetParametrUInt(domElement, AttrBaseLineP2, NULL_ID_STR);
        initData.dartP1Id = GetParametrUInt(domElement, AttrDartP1, NULL_ID_STR);
        initData.dartP2Id = GetParametrUInt(domElement, AttrDartP2, NULL_ID_STR);
        initData.dartP3Id = GetParametrUInt(domElement, AttrDartP3, NULL_ID_STR);

        const VAbstractValApplication *app = VAbstractValApplication::VApp();

        const QString labelMXStr = QString::number(app->fromPixel(labelMX));
        const QString labelMYStr = QString::number(app->fromPixel(labelMY));

        initData.name1 = GetParametrString(domElement, AttrName1, QChar('A'));
        initData.mx1 = app->toPixel(GetParametrDouble(domElement, AttrMx1, labelMXStr));
        initData.my1 = app->toPixel(GetParametrDouble(domElement, AttrMy1, labelMYStr));
        initData.showLabel1 = GetParametrBool(domElement, AttrShowLabel1, falseStr);

        initData.name2 = GetParametrString(domElement, AttrName2, QChar('A'));
        initData.mx2 = app->toPixel(GetParametrDouble(domElement, AttrMx2, labelMXStr));
        initData.my2 = app->toPixel(GetParametrDouble(domElement, AttrMy2, labelMYStr));
        initData.showLabel2 = GetParametrBool(domElement, AttrShowLabel2, falseStr);

        VToolTrueDarts::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating true darts"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolDartCap(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        // An old shape — a `<point>` — read into the OPERATION it became. The type is the same ("dartCap"), and it
        // is the ELEMENT that decides which reader takes it: this one keeps opening patterns already saved, and the
        // cap it builds is the very same operation the `<operation type="dartCap">` shape builds. Nothing has to be
        // converted on disk.
        OToolDartCapInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        // `PointsWithLineCommonAttributes` is no longer usable: it writes into a point tool's data, and this is an
        // operation. id and notes come from the common draw-tool attributes, and the name is the POINT's — the name
        // the file gave to the point C, which the figure keeps.
        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.pointName = domElement.attribute(AttrName);
        initData.showLabel = domElement.attribute(AttrShowLabel) == trueStr;

        initData.apexId = GetParametrUInt(domElement, AttrApex, NULL_ID_STR);
        initData.firstPointId = GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
        initData.secondPointId = GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);
        initData.extendPointId = GetParametrUInt(domElement, AttrExtendPoint, NULL_ID_STR);
        initData.drawCap = GetParametrBool(domElement, AttrDrawCap, trueStr);
        initData.secondSense
            = VAbstractPattern::GetParametrUInt(domElement, OToolDartCap::AttrSecondSense, u"1"_s) == 2;
        initData.showNotch = GetParametrBool(domElement, OToolDartCap::AttrNotch, falseStr);

        // The figure was kept as FLAT ATTRIBUTES of the `<point>` — there is no `<pleatDisplay>` to extract — so the
        // mark and the crank are filled from them, and the display carries what the operation draws.
        // The very same rule as ReadToolAttributes: a dart cap can have its drawing turned off, and the element
        // says so — the attribute wins over any default a shared reader would impose.
        initData.display.show = initData.drawCap;

        // The pen and the colour the POINT was drawn with belong to the FIGURE it became. Read them, or a cap taken
        // from a pattern already saved would come back in the default pen and the default colour — a silent change
        // of what its author saw, and one nothing on screen would explain.
        initData.display.lineTypeLine = domElement.attribute(AttrTypeLine, TypeLineDefault);
        initData.display.lineColor = domElement.attribute(AttrLineColor, ColorDefault);
        {
            OPleatMarkDisplay mark;
            mark.pleat = 0;
            mark.end = 0;
            mark.curve1 = GetParametrUInt(domElement, OToolDartCap::AttrCurve1, NULL_ID_STR);
            mark.curve2 = GetParametrUInt(domElement, OToolDartCap::AttrCurve2, NULL_ID_STR);
            // The label place of the point, with the same conversion every other reader of this file uses.
            mark.mx = VAbstractValApplication::VApp()->toPixel(GetParametrDouble(domElement, AttrMx, QString::number(labelMX)));
            mark.my = VAbstractValApplication::VApp()->toPixel(GetParametrDouble(domElement, AttrMy, QString::number(labelMY)));
            initData.display.marks = QVector<OPleatMarkDisplay>{mark};
        }
        {
            OPleatNotchDisplay notch;
            notch.pleat = 0;
            notch.end = 0;
            notch.point = GetParametrUInt(domElement, OToolDartCap::AttrNotchPoint, NULL_ID_STR);
            notch.curve1 = GetParametrUInt(domElement, OToolDartCap::AttrNotchCurve1, NULL_ID_STR);
            notch.curve2 = GetParametrUInt(domElement, OToolDartCap::AttrNotchCurve2, NULL_ID_STR);
            if (notch.point != NULL_ID)
            {
                initData.display.notches = QVector<OPleatNotchDisplay>{notch};
            }
        }

        // The points the figure is built on are the operation's SOURCE — the same list the dialog feeds when the cap
        // is made from the interface. An operation without one cannot be written (the schema asks for at least one
        // item) nor replayed by a recipe, and an old `<point>` names them in its flat attributes. The names are
        // given by the operation itself, from the objects: a source is a record of what the tool is built on, and
        // the file must carry a name for it.
        for (const quint32 id : {initData.apexId, initData.firstPointId, initData.secondPointId, initData.extendPointId})
        {
            if (id != NULL_ID)
            {
                initData.source.append(SourceItem{.id = id});
            }
        }

        // The operation takes the id the `<point>` had — it is the tool the layer and the graph name — and the point
        // C is given a fresh one by the figure itself, which is what BuildFigure does when `mark.point` is unset.
        OToolDartCap::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating the cap of a dart"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating the cap of a dart"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolRectangle(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        OToolRectangleInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);

        initData.basePointId = GetParametrUInt(domElement, AttrBasePoint, NULL_ID_STR);
        initData.formulaWidth = GetParametrString(domElement, AttrWidth);
        initData.formulaHeight = GetParametrString(domElement, AttrHeight);
        initData.typeLine = GetParametrString(domElement, AttrTypeLine, TypeLineLine);
        initData.lineColor = GetParametrString(domElement, AttrLineColor, ColorBlack);

        initData.p2id = GetParametrUInt(domElement, AttrPoint1, NULL_ID_STR);
        initData.p3id = GetParametrUInt(domElement, AttrPoint2, NULL_ID_STR);
        initData.p4id = GetParametrUInt(domElement, AttrPoint4, NULL_ID_STR);

        const VAbstractValApplication *app = VAbstractValApplication::VApp();

        const QString labelMXStr = QString::number(app->fromPixel(labelMX));
        const QString labelMYStr = QString::number(app->fromPixel(labelMY));

        initData.name2 = GetParametrString(domElement, AttrName1, QChar('B'));
        initData.mx2 = app->toPixel(GetParametrDouble(domElement, AttrMx1, labelMXStr));
        initData.my2 = app->toPixel(GetParametrDouble(domElement, AttrMy1, labelMYStr));
        initData.showLabel2 = GetParametrBool(domElement, AttrShowLabel1, falseStr);

        initData.name3 = GetParametrString(domElement, AttrName2, QChar('C'));
        initData.mx3 = app->toPixel(GetParametrDouble(domElement, AttrMx2, labelMXStr));
        initData.my3 = app->toPixel(GetParametrDouble(domElement, AttrMy2, labelMYStr));
        initData.showLabel3 = GetParametrBool(domElement, AttrShowLabel2, falseStr);

        initData.name4 = GetParametrString(domElement, AttrName3, QChar('D'));
        initData.mx4 = app->toPixel(GetParametrDouble(domElement, AttrMx3, labelMXStr));
        initData.my4 = app->toPixel(GetParametrDouble(domElement, AttrMy3, labelMYStr));
        initData.showLabel4 = GetParametrBool(domElement, AttrShowLabel3, falseStr);

        OToolRectangle::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating a rectangle"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating a rectangle"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}
