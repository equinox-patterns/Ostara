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
#include "../vtools/tools/nodeDetails/vtoolpin.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/tools/vtooluniondetails.h"

using namespace Qt::Literals::StringLiterals;


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
                                     VToolPin::ToolType});                      /*23*/
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
        case 23: // VToolPin::ToolType
            ParsePinPoint(domElement, parse);
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

        const QStringList pointNames = points.split(' ', Qt::SkipEmptyParts);
        if (pointNames.size() != 2)
        {
            continue;
        }

        const QString pairKey = pointNames.at(0) + '_' + pointNames.at(1);
        data->SetLineRoleName(pairKey, name);
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
        lineNamesElement = createElement(TagLineNames);
        // Insert before the first <draw> element so it's parsed first
        const QDomElement drawElement = root.firstChildElement(TagDraw);
        if (!drawElement.isNull())
        {
            root.insertBefore(lineNamesElement, drawElement);
        }
        else
        {
            root.appendChild(lineNamesElement);
        }
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
        QDomElement lineEl = createElement(VDomDocument::TagLine);
        const QStringList pointNames = it.key().split('_');
        if (pointNames.size() != 2)
        {
            continue;
        }
        lineEl.setAttribute(AttrPoints, pointNames.at(0) + ' ' + pointNames.at(1));
        lineEl.setAttribute(AttrName, it.value());
        lineNamesElement.appendChild(lineEl);
    }
}

//---------------------------------------------------------------------------------------------------------------------
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
        initData.showLabel = GetParametrBool(domElement, AttrShowLabel, trueStr);
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
void VPattern::ParsePinPoint(const QDomElement &domElement, const Document &parse)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolPinInitData initData;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        ToolsCommonAttributes(domElement, initData.id);
        initData.pointId = GetParametrUInt(domElement, AttrIdObject, NULL_ID_STR);
        initData.idTool = GetParametrUInt(domElement, VAbstractNode::AttrIdTool, NULL_ID_STR);

        VToolPin::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating pin point"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
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
        initData.showLabel1 = GetParametrBool(domElement, AttrShowLabel1, trueStr);

        initData.name2 = GetParametrString(domElement, AttrName2, QChar('A'));
        initData.mx2 = app->toPixel(GetParametrDouble(domElement, AttrMx2, labelMXStr));
        initData.my2 = app->toPixel(GetParametrDouble(domElement, AttrMy2, labelMYStr));
        initData.showLabel2 = GetParametrBool(domElement, AttrShowLabel2, trueStr);

        VToolTrueDarts::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating true darts"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}
