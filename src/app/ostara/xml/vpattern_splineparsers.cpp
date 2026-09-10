/************************************************************************
 **
 **  @file   vpattern_splineparsers.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Spline parsing methods extracted from vpattern.cpp
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
#include "../ifc/xml/vpatternconverter.h"
#include "../vmisc/def.h"
#include "../vmisc/vvalentinasettings.h"
#include "../qmuparser/qmuparsererror.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarc.h"
#include "../vtools/tools/drawTools/toolcurve/otoolarmholecurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezier.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezierpath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolspline.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolsplinepath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolparallelcurve.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "../vtools/tools/drawTools/vtoolline.h"
#include "../vtools/tools/nodeDetails/vabstractnode.h"
#include "../vtools/tools/nodeDetails/vnodearc.h"
#include "../vtools/tools/nodeDetails/vnodeellipticalarc.h"
#include "../vtools/tools/nodeDetails/vnodespline.h"
#include "../vtools/tools/nodeDetails/vnodesplinepath.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/vabstractmainwindow.h"

using namespace Qt::Literals::StringLiterals;


//---------------------------------------------------------------------------------------------------------------------
// TODO. Delete if minimal supported version is 0.2.7
void VPattern::ParseOldToolSpline(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolSplineInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.point1 = GetParametrUInt(domElement, AttrPoint1, NULL_ID_STR);
        initData.point4 = GetParametrUInt(domElement, AttrPoint4, NULL_ID_STR);
        const qreal angle1 = GetParametrDouble(domElement, AttrAngle1, QStringLiteral("270.0"));
        const qreal angle2 = GetParametrDouble(domElement, AttrAngle2, QStringLiteral("90.0"));
        const qreal kAsm1 = GetParametrDouble(domElement, AttrKAsm1, QStringLiteral("1.0"));
        const qreal kAsm2 = GetParametrDouble(domElement, AttrKAsm2, QStringLiteral("1.0"));
        const qreal kCurve = GetParametrDouble(domElement, AttrKCurve, QStringLiteral("1.0"));
        const QString color = GetParametrString(domElement, AttrColor, ColorBlack);
        const quint32 duplicate = GetParametrUInt(domElement, AttrDuplicate, QChar('0'));

        const auto p1 = data->GeometricObject<VPointF>(initData.point1);
        const auto p4 = data->GeometricObject<VPointF>(initData.point4);

        auto *spline = new VSpline(*p1, *p4, angle1, angle2, kAsm1, kAsm2, kCurve);
        if (duplicate > 0)
        {
            spline->SetDuplicate(duplicate);
        }
        spline->SetColor(color);

        VToolSpline::Create(initData, spline);

        // Convert to newer format
        SetAttribute(domElement, AttrType, VToolSpline::ToolType);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating simple curve"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolSpline(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolSplineInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.point1 = GetParametrUInt(domElement, AttrPoint1, NULL_ID_STR);
        initData.point4 = GetParametrUInt(domElement, AttrPoint4, NULL_ID_STR);

        initData.a1 = GetParametrString(domElement, AttrAngle1, QChar('0'));
        const QString angle1 = initData.a1; // need for saving fixed formula;

        initData.a2 = GetParametrString(domElement, AttrAngle2, QChar('0'));
        const QString angle2 = initData.a2; // need for saving fixed formula;

        initData.l1 = GetParametrString(domElement, AttrLength1, QChar('0'));
        const QString length1 = initData.l1; // need for saving fixed formula;

        initData.l2 = GetParametrString(domElement, AttrLength2, QChar('0'));
        const QString length2 = initData.l2; // need for saving fixed formula;

        initData.color = GetParametrString(domElement, AttrColor, ColorBlack);
        initData.penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        initData.duplicate = GetParametrUInt(domElement, AttrDuplicate, QChar('0'));
        initData.approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        initData.aliasSuffix = GetParametrEmptyString(domElement, AttrAlias);

        if (const VToolSpline *spl = VToolSpline::Create(initData); spl != nullptr)
        {
            auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
            SCASSERT(window != nullptr)
            connect(spl, &VToolSpline::ToolTip, window, &VAbstractMainWindow::ShowToolTip);
        }

        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (angle1 != initData.a1 || angle2 != initData.a2 || length1 != initData.l1 || length2 != initData.l2)
        {
            SetAttribute(domElement, AttrAngle1, initData.a1);
            SetAttribute(domElement, AttrAngle2, initData.a2);
            SetAttribute(domElement, AttrLength1, initData.l1);
            SetAttribute(domElement, AttrLength2, initData.l2);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating simple curve"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating simple interactive spline"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolCubicBezier(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolCubicBezierInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);

        const quint32 point1 = GetParametrUInt(domElement, AttrPoint1, NULL_ID_STR);
        const quint32 point2 = GetParametrUInt(domElement, AttrPoint2, NULL_ID_STR);
        const quint32 point3 = GetParametrUInt(domElement, AttrPoint3, NULL_ID_STR);
        const quint32 point4 = GetParametrUInt(domElement, AttrPoint4, NULL_ID_STR);

        const QString color = GetParametrString(domElement, AttrColor, ColorBlack);
        const QString penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        const quint32 duplicate = GetParametrUInt(domElement, AttrDuplicate, QChar('0'));
        const qreal approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        const QString alias = GetParametrEmptyString(domElement, AttrAlias);

        auto p1 = data->GeometricObject<VPointF>(point1);
        auto p2 = data->GeometricObject<VPointF>(point2);
        auto p3 = data->GeometricObject<VPointF>(point3);
        auto p4 = data->GeometricObject<VPointF>(point4);

        initData.spline = new VCubicBezier(*p1, *p2, *p3, *p4);
        if (duplicate > 0)
        {
            initData.spline->SetDuplicate(duplicate);
        }
        initData.spline->SetColor(color);
        initData.spline->SetPenStyle(penStyle);
        initData.spline->SetPenStyle(penStyle);
        initData.spline->SetApproximationScale(approximationScale);
        initData.spline->SetAliasSuffix(alias);

        VToolCubicBezier::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating cubic bezier curve"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolArmholeCurve(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        OToolArmholeCurveInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);

        const quint32 point1 = GetParametrUInt(domElement, AttrPoint1, NULL_ID_STR); // armhole (path[0])
        const quint32 point4 = GetParametrUInt(domElement, AttrPoint4, NULL_ID_STR); // chest/back width (path[3])
        const quint32 point6 = GetParametrUInt(domElement, AttrPoint6, NULL_ID_STR); // perpendicular (path[5])
        const quint32 point7 = GetParametrUInt(domElement, AttrPoint7, NULL_ID_STR); // shoulder (path[6])

        const QString color = GetParametrString(domElement, AttrColor, ColorBlack);
        const QString penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        const quint32 duplicate = GetParametrUInt(domElement, AttrDuplicate, QChar('0'));
        const qreal approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        const QString alias = GetParametrEmptyString(domElement, AttrAlias);

        auto p1 = data->GeometricObject<VPointF>(point1); // armhole
        auto pCarrure = data->GeometricObject<VPointF>(point4); // chest/back width
        auto pEpaule = data->GeometricObject<VPointF>(point7); // shoulder
        // point6 == point7 means the perpendicular was skipped: it is then computed, not read.
        auto pPerp = OToolArmholeCurve::IsPerpendicularSkipped(point6, point7)
                         ? pEpaule
                         : data->GeometricObject<VPointF>(point6);

        // Single factory shared with the creation collector: it recomputes the control handles
        // (derived from the 4 user points, not container GObjects — no graph vertices, so their ids
        // stay stable across delete/undo cycles) and resolves the skipped perpendicular.
        initData.path = new VCubicBezierPath(OToolArmholeCurve::BuildPath(*p1, *pCarrure, *pPerp, *pEpaule));
        if (duplicate > 0)
        {
            initData.path->SetDuplicate(duplicate);
        }
        initData.path->SetColor(color);
        initData.path->SetPenStyle(penStyle);
        initData.path->SetApproximationScale(approximationScale);
        initData.path->SetAliasSuffix(alias);

        OToolArmholeCurve::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating armhole curve"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseOldToolSplinePath(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    // TODO. Delete if minimal supported version is 0.2.7
    Q_STATIC_ASSERT_X(VPatternConverter::PatternMinVer < FormatVersion(0, 2, 7), "Time to refactor the code.");

    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolSplinePathInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        const qreal kCurve = GetParametrDouble(domElement, AttrKCurve, QStringLiteral("1.0"));
        const QString color = GetParametrString(domElement, AttrColor, ColorBlack);
        const quint32 duplicate = GetParametrUInt(domElement, AttrDuplicate, QChar('0'));
        const qreal approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));

        QVector<VFSplinePoint> points;

        const QDomNodeList nodeList = domElement.childNodes();
        QDOM_LOOP(nodeList, i)
        {
            if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
                !element.isNull() && element.tagName() == TagPathPoint)
            {
                const qreal kAsm1 = GetParametrDouble(element, AttrKAsm1, QStringLiteral("1.0"));
                const qreal angle = GetParametrDouble(element, AttrAngle, QChar('0'));
                const qreal kAsm2 = GetParametrDouble(element, AttrKAsm2, QStringLiteral("1.0"));
                const quint32 pSpline = GetParametrUInt(element, AttrPSpline, NULL_ID_STR);
                initData.points.append(pSpline);
                const VPointF p = *data->GeometricObject<VPointF>(pSpline);

                QLineF line(0, 0, 100, 0);
                line.setAngle(angle + 180);

                VFSplinePoint const splPoint(p, kAsm1, line.angle(), kAsm2, angle);
                points.append(splPoint);
            }
        }

        auto *path = new VSplinePath(points, kCurve);
        if (duplicate > 0)
        {
            path->SetDuplicate(duplicate);
        }
        path->SetColor(color);
        path->SetApproximationScale(approximationScale);

        VToolSplinePath::Create(initData, path);

        // Convert to newer format
        SetAttribute(domElement, AttrType, VToolSplinePath::ToolType);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating curve path"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolSplinePath(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolSplinePathInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.color = GetParametrString(domElement, AttrColor, ColorBlack);
        initData.penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        initData.duplicate = GetParametrUInt(domElement, AttrDuplicate, QChar('0'));
        initData.approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        initData.aliasSuffix = GetParametrEmptyString(domElement, AttrAlias);

        const QDomNodeList nodeList = domElement.childNodes();
        QDOM_LOOP(nodeList, i)
        {
            if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
                not element.isNull() && element.tagName() == TagPathPoint)
            {
                initData.a1.append(GetParametrString(element, AttrAngle1, QChar('0')));
                initData.a2.append(GetParametrString(element, AttrAngle2, QChar('0')));
                initData.l1.append(GetParametrString(element, AttrLength1, QChar('0')));
                initData.l2.append(GetParametrString(element, AttrLength2, QChar('0')));
                const quint32 pSpline = GetParametrUInt(element, AttrPSpline, NULL_ID_STR);
                initData.points.append(pSpline);
            }
        }

        // need for saving fixed formula;
        const QVector<QString> angle1 = initData.a1;
        const QVector<QString> angle2 = initData.a2;
        const QVector<QString> length1 = initData.l1;
        const QVector<QString> length2 = initData.l2;

        if (const VToolSplinePath *spl = VToolSplinePath::Create(initData); spl != nullptr)
        {
            const auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
            SCASSERT(window != nullptr)
            connect(spl, &VToolSplinePath::ToolTip, window, &VAbstractMainWindow::ShowToolTip);
        }

        // Rewrite attribute formula. Need for situation when we have wrong formula.
        int count = 0;
        QDOM_LOOP(nodeList, i)
        {
            if (QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
                not element.isNull() && element.tagName() == TagPathPoint)
            {
                if (angle1.at(count) != initData.a1.at(count) || angle2.at(count) != initData.a2.at(count) ||
                    length1.at(count) != initData.l1.at(count) || length2.at(count) != initData.l2.at(count))
                {
                    SetAttribute(element, AttrAngle1, initData.a1.at(count));
                    SetAttribute(element, AttrAngle2, initData.a2.at(count));
                    SetAttribute(element, AttrLength1, initData.l1.at(count));
                    SetAttribute(element, AttrLength2, initData.l2.at(count));
                    modified = true;
                    haveLiteChange();
                }
                ++count;
            }
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating curve path"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating interactive spline path"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolParallelCurve(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolParallelCurveInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.formulaWidth = GetParametrString(domElement, AttrWidth, QChar('0'));
        const QString fWidth = initData.formulaWidth; // need for saving fixed formula;
        initData.originCurveId = GetParametrUInt(domElement, AttrCurve, NULL_ID_STR);
        initData.color = GetParametrString(domElement, AttrColor, ColorBlack);
        initData.penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        initData.approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        initData.aliasSuffix = GetParametrEmptyString(domElement, AttrAlias);
        initData.name = GetParametrEmptyString(domElement, AttrName);

        // We no longer need to handle suffix attribute here. The code can be removed.
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        if (initData.name.isEmpty())
        {
            const QString suffix = GetParametrEmptyString(domElement, AttrSuffix);
            const QSharedPointer<VAbstractCurve> curve = initData.data->GeometricObject<VAbstractCurve>(
                initData.originCurveId);
            initData.name = curve->HeadlessName() + suffix;
            SetAttribute(domElement, AttrName, initData.name);
        }

        VToolParallelCurve::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (fWidth != initData.formulaWidth)
        {
            SetAttribute(domElement, AttrWidth, initData.formulaWidth);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating a parallel curve"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating a parallel curve"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolGraduatedCurve(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolGraduatedCurveInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.originCurveId = GetParametrUInt(domElement, AttrCurve, NULL_ID_STR);
        initData.color = GetParametrString(domElement, AttrColor, ColorBlack);
        initData.penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        initData.approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        initData.aliasSuffix = GetParametrEmptyString(domElement, AttrAlias);
        initData.name = GetParametrEmptyString(domElement, AttrName);
        initData.offsets = VToolGraduatedCurve::ExtractOffsetData(domElement);

        const QDomNodeList nodeList = domElement.childNodes();
        const qint32 num = nodeList.size();
        initData.offsets.reserve(num);

        QVector<QString> originalFormulas;
        originalFormulas.reserve(initData.offsets.size());
        for (const auto &offsetData : std::as_const(initData.offsets))
        {
            originalFormulas.append(offsetData.formula); // need for saving fixed formula;
        }

        // We no longer need to handle suffix attribute here. The code can be removed.
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        if (initData.name.isEmpty())
        {
            const QString suffix = GetParametrEmptyString(domElement, AttrSuffix);
            const QSharedPointer<VAbstractCurve> curve = initData.data->GeometricObject<VAbstractCurve>(
                initData.originCurveId);
            initData.name = curve->HeadlessName() + suffix;
            SetAttribute(domElement, AttrName, initData.name);
        }

        VToolGraduatedCurve::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        for (qint32 i = 0; i < num; ++i)
        {
            if (QDomElement element = nodeList.at(i).toElement(); not element.isNull() && element.tagName() == TagOffset)
            {
                if (const QString &newFormula = initData.offsets.at(i).formula; originalFormulas.at(i) != newFormula)
                {
                    SetAttribute(element, AttrWidth, newFormula);
                    modified = true;
                    haveLiteChange();
                }
            }
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating a graduated curve"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating a graduated curve"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolCubicBezierPath(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolCubicBezierPathInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        const QString color = GetParametrString(domElement, AttrColor, ColorBlack);
        const QString penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        const quint32 duplicate = GetParametrUInt(domElement, AttrDuplicate, QChar('0'));
        const qreal approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        const QString alias = GetParametrEmptyString(domElement, AttrAlias);

        QVector<VPointF> points;

        const QDomNodeList nodeList = domElement.childNodes();
        QDOM_LOOP(nodeList, i)
        {
            if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
                !element.isNull() && element.tagName() == TagPathPoint)
            {
                const quint32 pSpline = GetParametrUInt(element, AttrPSpline, NULL_ID_STR);
                const VPointF p = *data->GeometricObject<VPointF>(pSpline);
                points.append(p);
            }
        }

        initData.path = new VCubicBezierPath(points);
        if (duplicate > 0)
        {
            initData.path->SetDuplicate(duplicate);
        }
        initData.path->SetColor(color);
        initData.path->SetPenStyle(penStyle);
        initData.path->SetApproximationScale(approximationScale);
        initData.path->SetAliasSuffix(alias);

        VToolCubicBezierPath::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating cubic bezier path curve"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseNodeSpline(const QDomElement &domElement, const Document &parse)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VAbstractNodeInitData initData;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        SplinesCommonAttributes(domElement, initData.id, initData.idObject, initData.idTool);

        VNodeSpline::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating modeling simple curve"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseNodeSplinePath(const QDomElement &domElement, const Document &parse)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VAbstractNodeInitData initData;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        SplinesCommonAttributes(domElement, initData.id, initData.idObject, initData.idTool);

        VNodeSplinePath::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating modeling curve path"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}


/**
 * @brief ParseSplineElement parse spline tag.
 * @param scene scene.
 * @param domElement tag in xml tree.
 * @param parse parser file mode.
 * @param type type of spline.
 */
void VPattern::ParseSplineElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                                  const QString &type)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");
    Q_ASSERT_X(type.isEmpty() == false, Q_FUNC_INFO, "type of spline is empty");

    static const QStringList splines({VToolSpline::OldToolType,        /*0*/
                                      VToolSpline::ToolType,           /*1*/
                                      VToolSplinePath::OldToolType,    /*2*/
                                      VToolSplinePath::ToolType,       /*3*/
                                      VNodeSpline::ToolType,           /*4*/
                                      VNodeSplinePath::ToolType,       /*5*/
                                      VToolCubicBezier::ToolType,      /*6*/
                                      VToolCubicBezierPath::ToolType,  /*7*/
                                      VToolParallelCurve::ToolType,    /*8*/
                                      VToolGraduatedCurve::ToolType,   /*9*/
                                      OToolArmholeCurve::ToolType});   /*10*/
    switch (splines.indexOf(type))
    {
        case 0: // VToolSpline::OldToolType
            qCDebug(vXML, "VOldToolSpline.");
            // TODO. Delete if minimal supported version is 0.2.7
            Q_STATIC_ASSERT_X(VPatternConverter::PatternMinVer < FormatVersion(0, 2, 7), "Time to refactor the code.");
            ParseOldToolSpline(scene, domElement, parse);
            break;
        case 1: // VToolSpline::ToolType
            ParseToolSpline(scene, domElement, parse);
            break;
        case 2: // VToolSplinePath::OldToolType
            // TODO. Delete if minimal supported version is 0.2.7
            Q_STATIC_ASSERT_X(VPatternConverter::PatternMinVer < FormatVersion(0, 2, 7), "Time to refactor the code.");
            ParseOldToolSplinePath(scene, domElement, parse);
            break;
        case 3: // VToolSplinePath::ToolType
            ParseToolSplinePath(scene, domElement, parse);
            break;
        case 4: // VNodeSpline::ToolType
            ParseNodeSpline(domElement, parse);
            break;
        case 5: // VNodeSplinePath::ToolType
            ParseNodeSplinePath(domElement, parse);
            break;
        case 6: // VToolCubicBezier::ToolType
            ParseToolCubicBezier(scene, domElement, parse);
            break;
        case 7: // VToolCubicBezierPath::ToolType
            ParseToolCubicBezierPath(scene, domElement, parse);
            break;
        case 8: // VToolParallelCurve::ToolType
            ParseToolParallelCurve(scene, domElement, parse);
            break;
        case 9: // VToolGraduatedCurve::ToolType
            ParseToolGraduatedCurve(scene, domElement, parse);
            break;
        case 10: // OToolArmholeCurve::ToolType
            ParseToolArmholeCurve(scene, domElement, parse);
            break;
        default:
            throw VException(tr("Unknown spline type '%1'.").arg(type));
    }
}

