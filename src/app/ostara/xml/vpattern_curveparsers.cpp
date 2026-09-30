/************************************************************************
 **
 **  @file   vpattern_curveparsers.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Curve, arc, operation, and path parsing methods extracted from vpattern.cpp
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
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../qmuparser/qmuparsererror.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vsplinepath.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/oclosedartrule.h"
#include "../vtools/tools/drawTools/operation/otoolclosedart.h"
#include "../vtools/tools/drawTools/operation/otooltransfert.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarcwithlength.h"
#include "../vtools/tools/drawTools/vtoolline.h"
#include "../vtools/tools/nodeDetails/vnodearc.h"
#include "../vtools/tools/nodeDetails/vnodeellipticalarc.h"
#include "../vtools/tools/nodeDetails/vtoolpiecepath.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/tools/vtooluniondetails.h"
#include "../vwidgets/vabstractmainwindow.h"

using namespace Qt::Literals::StringLiterals;


//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolArc(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolArcInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.center = GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
        initData.radius = GetParametrString(domElement, AttrRadius, QStringLiteral("10"));
        const QString r = initData.radius; // need for saving fixed formula;
        initData.f1 = GetParametrString(domElement, AttrAngle1, QStringLiteral("180"));
        const QString f1Fix = initData.f1; // need for saving fixed formula;
        initData.f2 = GetParametrString(domElement, AttrAngle2, QStringLiteral("270"));
        const QString f2Fix = initData.f2; // need for saving fixed formula;
        initData.color = GetParametrString(domElement, AttrColor, ColorBlack);
        initData.penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        initData.approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        initData.aliasSuffix = GetParametrEmptyString(domElement, AttrAlias);

        VToolArc::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (r != initData.radius || f1Fix != initData.f1 || f2Fix != initData.f2)
        {
            SetAttribute(domElement, AttrRadius, initData.radius);
            SetAttribute(domElement, AttrAngle1, initData.f1);
            SetAttribute(domElement, AttrAngle2, initData.f2);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating simple arc"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating simple arc"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolEllipticalArc(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolEllipticalArcInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.center = GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
        initData.radius1 = GetParametrString(domElement, AttrRadius1, QStringLiteral("10"));
        initData.radius2 = GetParametrString(domElement, AttrRadius2, QStringLiteral("10"));
        const QString r1 = initData.radius1; // need for saving fixed formula;
        const QString r2 = initData.radius2; // need for saving fixed formula;
        initData.f1 = GetParametrString(domElement, AttrAngle1, QStringLiteral("180"));
        const QString f1Fix = initData.f1; // need for saving fixed formula;
        initData.f2 = GetParametrString(domElement, AttrAngle2, QStringLiteral("270"));
        const QString f2Fix = initData.f2; // need for saving fixed formula;
        initData.rotationAngle = GetParametrString(domElement, AttrRotationAngle, QChar('0'));
        const QString frotationFix = initData.rotationAngle; // need for saving fixed formula;
        initData.color = GetParametrString(domElement, AttrColor, ColorBlack);
        initData.penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        initData.approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        initData.aliasSuffix = GetParametrEmptyString(domElement, AttrAlias);

        VToolEllipticalArc::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (r1 != initData.radius1 || r2 != initData.radius2 || f1Fix != initData.f1 || f2Fix != initData.f2 ||
            frotationFix != initData.rotationAngle)
        {
            SetAttribute(domElement, AttrRadius1, initData.radius1);
            SetAttribute(domElement, AttrRadius2, initData.radius2);
            SetAttribute(domElement, AttrAngle1, initData.f1);
            SetAttribute(domElement, AttrAngle2, initData.f2);
            SetAttribute(domElement, AttrRotationAngle, initData.rotationAngle);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating simple elliptical arc"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating simple elliptical arc"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseNodeEllipticalArc(const QDomElement &domElement, const Document &parse)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VAbstractNodeInitData initData;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        ToolsCommonAttributes(domElement, initData.id);
        initData.idObject = GetParametrUInt(domElement, AttrIdObject, NULL_ID_STR);
        initData.idTool = GetParametrUInt(domElement, VAbstractNode::AttrIdTool, NULL_ID_STR);

        VNodeEllipticalArc::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating modeling elliptical arc"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolEllipticalArcWithLength(VMainGraphicsScene *scene,
                                                QDomElement &domElement,
                                                const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolEllipticalArcWithLengthInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.center = GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
        initData.radius1 = GetParametrString(domElement, AttrRadius1, QStringLiteral("10"));
        initData.radius2 = GetParametrString(domElement, AttrRadius2, QStringLiteral("10"));
        const QString r1 = initData.radius1; // need for saving fixed formula;
        const QString r2 = initData.radius2; // need for saving fixed formula;
        initData.f1 = GetParametrString(domElement, AttrAngle1, QStringLiteral("180"));
        const QString f1Fix = initData.f1; // need for saving fixed formula;
        initData.length = GetParametrString(domElement, AttrLength, QStringLiteral("10"));
        const QString lengthFix = initData.length; // need for saving fixed length;
        initData.rotationAngle = GetParametrString(domElement, AttrRotationAngle, QChar('0'));
        const QString frotationFix = initData.rotationAngle; // need for saving fixed formula;
        initData.color = GetParametrString(domElement, AttrColor, ColorBlack);
        initData.penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        initData.approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        initData.aliasSuffix = GetParametrEmptyString(domElement, AttrAlias);

        VToolEllipticalArcWithLength::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (r1 != initData.radius1 || r2 != initData.radius2 || f1Fix != initData.f1 || lengthFix != initData.length
            || frotationFix != initData.rotationAngle)
        {
            SetAttribute(domElement, AttrRadius1, initData.radius1);
            SetAttribute(domElement, AttrRadius2, initData.radius2);
            SetAttribute(domElement, AttrAngle1, initData.f1);
            SetAttribute(domElement, AttrLength, initData.length);
            SetAttribute(domElement, AttrRotationAngle, initData.rotationAngle);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating elliptical arc with length"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating elliptical arc with length"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseNodeArc(const QDomElement &domElement, const Document &parse)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    try
    {
        VAbstractNodeInitData initData;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        ToolsCommonAttributes(domElement, initData.id);
        initData.idObject = GetParametrUInt(domElement, AttrIdObject, NULL_ID_STR);
        initData.idTool = GetParametrUInt(domElement, VAbstractNode::AttrIdTool, NULL_ID_STR);

        VNodeArc::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating modeling arc"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolArcWithLength(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolArcWithLengthInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.center = GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
        initData.radius = GetParametrString(domElement, AttrRadius, QStringLiteral("10"));
        const QString r = initData.radius; // need for saving fixed formula;
        initData.f1 = GetParametrString(domElement, AttrAngle1, QStringLiteral("180"));
        const QString f1Fix = initData.f1; // need for saving fixed formula;
        initData.length = GetParametrString(domElement, AttrLength, QStringLiteral("10"));
        const QString lengthFix = initData.length; // need for saving fixed length;
        initData.color = GetParametrString(domElement, AttrColor, ColorBlack);
        initData.penStyle = GetParametrString(domElement, AttrPenStyle, TypeLineLine);
        initData.approximationScale = GetParametrDouble(domElement, AttrAScale, QChar('0'));
        initData.aliasSuffix = GetParametrEmptyString(domElement, AttrAlias);

        VToolArcWithLength::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (r != initData.radius || f1Fix != initData.f1 || lengthFix != initData.length)
        {
            SetAttribute(domElement, AttrRadius, initData.radius);
            SetAttribute(domElement, AttrAngle1, initData.f1);
            SetAttribute(domElement, AttrLength, initData.length);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating simple arc"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating simple arc"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolRotation(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolRotationInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.origin = GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
        // The geometry below is built around this angle, and VToolRotation::ReadToolAttributes reads the same
        // one back: one default for both, or a file without the attribute would open with points at one angle
        // and the dialog showing another.
        initData.angle = GetParametrString(domElement, AttrAngle, VToolRotation::DefaultAngle);
        const QString a = initData.angle; // need for saving fixed formula;

        // We no longer need to handle suffix attribute here. The code can be removed.
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        initData.suffix = GetParametrEmptyString(domElement, AttrSuffix);

        VAbstractOperation::ExtractData(domElement, initData);

        VToolRotation::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (a != initData.angle)
        {
            SetAttribute(domElement, AttrAngle, initData.angle);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating operation of rotation"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating operation of rotation"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolCloseDart(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        OToolCloseDartInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        // The three points of the dart and how much of its opening is closed. No angle is read here: it is
        // derived from the points, so a pattern never holds one.
        initData.origin = GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
        initData.moving = GetParametrUInt(domElement, AttrMovingPoint, NULL_ID_STR);
        initData.target = GetParametrUInt(domElement, AttrTargetPoint, NULL_ID_STR);
        initData.divisor = GetParametrString(domElement, AttrClosingDivisor,
                                             OCloseDartRule::DefaultDivisorFormula());
        const QString divisor = initData.divisor; // need for saving fixed formula;

        // We no longer need to handle suffix attribute here. The code can be removed.
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        initData.suffix = GetParametrEmptyString(domElement, AttrSuffix);

        VAbstractOperation::ExtractData(domElement, initData);

        OToolCloseDart::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (divisor != initData.divisor)
        {
            SetAttribute(domElement, AttrClosingDivisor, initData.divisor);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating the operation that closes a dart"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating the operation that closes a dart"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolTransfert(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        OToolTransfertInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        // The two merge points, and nothing else: the portion is the source list, and the copies are recomputed
        // from it at every parse.
        initData.origin = GetParametrUInt(domElement, AttrOrigin, NULL_ID_STR);
        initData.target = GetParametrUInt(domElement, AttrTarget, NULL_ID_STR);

        // Both merge points are what this operation is made of. Without one of them it cannot be built, and
        // reading it throws — which would take the whole pattern down with it. Skipped and said out loud, the
        // rest of the file still opens.
        if (initData.origin == NULL_ID || initData.target == NULL_ID)
        {
            qWarning() << VAbstractValApplication::warningMessageSignature
                       << tr("A transfer without both of its merge points was ignored.");
            return;
        }

        // We no longer need to handle suffix attribute here. The code can be removed.
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        initData.suffix = GetParametrEmptyString(domElement, AttrSuffix);

        VAbstractOperation::ExtractData(domElement, initData);

        // The portion is read from another piece, and a piece can be built BEFORE the one that receives it:
        // nothing forbids it. The container of this moment is a prefix of the file and does not hold those
        // objects yet. The transfer is kept aside and built at the end of the first pass, when every block has
        // been read — throwing here would take the whole pattern down for a file that is merely in an order the
        // code did not expect.
        // Two reasons to wait: the objects are not in the container yet, or the graph has no vertex for them
        // yet. The second one is quieter and just as wrong — AddEdge wants both ends and drops the edge in
        // silence when one is missing, so the transfer would exist without its dependencies: no protection from
        // a delete, and nothing in the Dependencies dock.
        if (!TransfertInputsAreThere(data, initData) || !TransfertSourcesAreGraphVertices(initData))
        {
            m_deferredTransfers.append({.element = domElement,
                                        .id = initData.id,
                                        .parse = parse,
                                        .blockIndex = PatternBlockMapper()->GetActiveId()});
            return;
        }

        OToolTransfert::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating the operation of transfer"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::TransfertSourcesAreGraphVertices(const OToolTransfertInitData &initData) -> bool
{
    VPatternGraph *graph = PatternGraph();
    if (graph == nullptr || !graph->IsGraphRebuildEnabled())
    {
        // A values-only reparse keeps the graph of the last full one: every vertex is already there.
        return true;
    }

    if (!graph->HasVertex(initData.origin))
    {
        return false;
    }

    for (const SourceItem &item : initData.source)
    {
        if (!graph->HasVertex(item.id))
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ReplayDeferredTransfers()
{
    // The list is emptied first: a transfer that still cannot be built here has a genuinely dangling reference,
    // and deferring it again would only postpone the answer to the next pass.
    QVector<DeferredTransfert> pending;
    pending.swap(m_deferredTransfers);
    const QString active = PatternBlockMapper()->GetActive();

    for (const DeferredTransfert &entry : pending)
    {
        // The record and the graph node both take the block of the tool from the mapper: the block the transfer
        // belongs to has to be the active one while it is built — at the end of the pass, that is the last block
        // of the file.
        PatternBlockMapper()->SetActiveById(entry.blockIndex);

        // The first time through, the tool does not exist yet, and only a FullParse instantiates it and its
        // scene item. Afterwards, the parse of the run is what re-reads it. FindTool, not getTool: the question
        // here is precisely whether it exists, and getTool answers it with a throw.
        const Document replayParse = VAbstractPattern::FindTool(entry.id) == nullptr ? Document::FullParse
                                                                                    : entry.parse;

        QDomElement element = entry.element;
        ParseToolTransfert(sceneDraw, element, replayParse);
    }

    PatternBlockMapper()->SetActive(active);

    if (!m_deferredTransfers.isEmpty())
    {
        // Still not resolvable with every block read: the reference is genuinely dangling. Said out loud, and
        // the operations are dropped rather than taking the file down.
        qWarning() << VAbstractValApplication::warningMessageSignature
                   << tr("A transfer refers to objects the pattern does not define, and was ignored.");
        m_deferredTransfers.clear();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolFlippingByLine(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolFlippingByLineInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.firstLinePointId = GetParametrUInt(domElement, AttrP1Line, NULL_ID_STR);
        initData.secondLinePointId = GetParametrUInt(domElement, AttrP2Line, NULL_ID_STR);

        // We no longer need to handle suffix attribute here. The code can be removed.
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        initData.suffix = GetParametrEmptyString(domElement, AttrSuffix);

        VAbstractOperation::ExtractData(domElement, initData);

        VToolFlippingByLine::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating operation of flipping by line"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolFlippingByAxis(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolFlippingByAxisInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.originPointId = GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
        initData.axisType = static_cast<AxisType>(GetParametrUInt(domElement, AttrAxisType, QChar('1')));

        // We no longer need to handle suffix attribute here. The code can be removed.
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        initData.suffix = GetParametrEmptyString(domElement, AttrSuffix);

        VAbstractOperation::ExtractData(domElement, initData);

        VToolFlippingByAxis::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating operation of flipping by axis"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseToolMove(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");

    try
    {
        VToolMoveInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        DrawToolsCommonAttributes(domElement, initData.id, initData.notes);
        initData.formulaAngle = GetParametrString(domElement, AttrAngle, QChar('0'));
        const QString a = initData.formulaAngle; // need for saving fixed formula;
        initData.formulaRotationAngle = GetParametrString(domElement, AttrRotationAngle, QChar('0'));
        const QString r = initData.formulaRotationAngle; // need for saving fixed formula;
        initData.formulaLength = GetParametrString(domElement, AttrLength, QChar('0'));
        const QString len = initData.formulaLength; // need for saving fixed formula;
        initData.rotationOrigin = GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);

        // We no longer need to handle suffix attribute here. The code can be removed.
        Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
        initData.suffix = GetParametrEmptyString(domElement, AttrSuffix);

        VAbstractOperation::ExtractData(domElement, initData);

        VToolMove::Create(initData);
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (a != initData.formulaAngle || r != initData.formulaRotationAngle || len != initData.formulaLength)
        {
            SetAttribute(domElement, AttrAngle, initData.formulaAngle);
            SetAttribute(domElement, AttrRotationAngle, initData.formulaRotationAngle);
            SetAttribute(domElement, AttrLength, initData.formulaLength);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating operation of moving"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
    catch (qmu::QmuParserError &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating operation of moving"), domElement);
        excep.AddMoreInformation("Message:     "_L1 + e.GetMsg() + '\n'_L1 + "Expression:  "_L1 + e.GetExpr());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseArcElement parse arc tag.
 * @param scene scene.
 * @param domElement tag in xml tree.
 * @param parse parser file mode.
 * @param type type of spline.
 */
void VPattern::ParseArcElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                               const QString &type)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");
    Q_ASSERT_X(not type.isEmpty(), Q_FUNC_INFO, "type of arc is empty");

    auto const arcs = QStringList() << VToolArc::ToolType            /*0*/
                                    << VNodeArc::ToolType            /*1*/
                                    << VToolArcWithLength::ToolType; /*2*/

    switch (arcs.indexOf(type))
    {
        case 0: // VToolArc::ToolType
            ParseToolArc(scene, domElement, parse);
            break;
        case 1: // VNodeArc::ToolType
            ParseNodeArc(domElement, parse);
            break;
        case 2: // VToolArcWithLength::ToolType
            ParseToolArcWithLength(scene, domElement, parse);
            break;
        default:
            throw VException(tr("Unknown arc type '%1'.").arg(type));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseEllipticalArcElement parse elliptical arc tag.
 * @param scene scene.
 * @param domElement tag in xml tree.
 * @param parse parser file mode.
 * @param type type of spline.
 */
void VPattern::ParseEllipticalArcElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                                         const QString &type)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");
    Q_ASSERT_X(not type.isEmpty(), Q_FUNC_INFO, "type of elliptical arc is empty");

    const auto arcs = QStringList{
        VToolEllipticalArc::ToolType,          /*0*/
        VNodeEllipticalArc::ToolType,          /*1*/
        VToolEllipticalArcWithLength::ToolType /*2*/
    };

    switch (arcs.indexOf(type))
    {
        case 0: // VToolArc::ToolType
            ParseToolEllipticalArc(scene, domElement, parse);
            break;
        case 1: // VNodeEllipticalArc::ToolType
            ParseNodeEllipticalArc(domElement, parse);
            break;
        case 2: // VToolEllipticalArcWithLength::ToolType
            ParseToolEllipticalArcWithLength(scene, domElement, parse);
            break;
        default:
            throw VException(tr("Unknown elliptical arc type '%1'.").arg(type));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseToolsElement parse tools tag.
 * @param scene scene.
 * @param domElement tag in xml tree.
 * @param parse parser file mode.
 * @param type type of spline.
 */
void VPattern::ParseToolsElement(VMainGraphicsScene *scene, const QDomElement &domElement, const Document &parse,
                                 const QString &type)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(domElement.isNull() == false, Q_FUNC_INFO, "domElement is null");
    Q_ASSERT_X(type.isEmpty() == false, Q_FUNC_INFO, "type of spline is empty");

    const auto tools = QStringList() << VToolUnionDetails::ToolType;
    switch (tools.indexOf(type))
    {
        case 0: // VToolUnionDetails::ToolType
            try
            {
                VToolUnionDetailsInitData initData;
                initData.indexD1 = GetParametrUInt(domElement, VToolUnionDetails::AttrIndexD1, NULL_ID_STR);
                initData.indexD2 = GetParametrUInt(domElement, VToolUnionDetails::AttrIndexD2, NULL_ID_STR);
                initData.version = GetParametrUInt(domElement, AttrVersion, QChar('1'));
                initData.scene = scene;
                initData.doc = this;
                initData.data = data;
                initData.parse = parse;
                initData.typeCreation = Source::FromFile;

                ToolsCommonAttributes(domElement, initData.id);

                VToolUnionDetails::Create(initData);
            }
            catch (const VExceptionBadId &e)
            {
                VExceptionObjectError excep(tr("Error creating or updating union details"), domElement);
                excep.AddMoreInformation(e.ErrorMessage());
                throw excep;
            }
            break;
        default:
            throw VException(tr("Unknown tools type '%1'.").arg(type));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseOperationElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse,
                                     const QString &type)
{
    SCASSERT(scene != nullptr)
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");
    Q_ASSERT_X(not type.isEmpty(), Q_FUNC_INFO, "type of operation is empty");

    const auto opers = QStringList() << VToolRotation::ToolType       /*0*/
                                     << VToolFlippingByLine::ToolType /*1*/
                                     << VToolFlippingByAxis::ToolType /*2*/
                                     << VToolMove::ToolType           /*3*/
                                     << OToolCloseDart::ToolType      /*4*/
                                     << OToolTransfert::ToolType;     /*5*/

    switch (opers.indexOf(type))
    {
        case 0: // VToolRotation::ToolType
            ParseToolRotation(scene, domElement, parse);
            break;
        case 1: // VToolFlippingByLine::ToolType
            ParseToolFlippingByLine(scene, domElement, parse);
            break;
        case 2: // VToolFlippingByAxis::ToolType
            ParseToolFlippingByAxis(scene, domElement, parse);
            break;
        case 3: // VToolMove::ToolType
            ParseToolMove(scene, domElement, parse);
            break;
        case 4: // OToolCloseDart::ToolType
            ParseToolCloseDart(scene, domElement, parse);
            break;
        case 5: // OToolTransfert::ToolType
            ParseToolTransfert(scene, domElement, parse);
            break;
        default:
            throw VException(tr("Unknown operation type '%1'.").arg(type));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParsePathElement(VMainGraphicsScene *scene, QDomElement &domElement, const Document &parse)
{
    SCASSERT(scene != nullptr);
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");
    try
    {
        VToolPiecePathInitData initData;
        initData.scene = scene;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        ToolsCommonAttributes(domElement, initData.id);
        initData.idTool = GetParametrUInt(domElement, VAbstractNode::AttrIdTool, NULL_ID_STR);

        if (const QDomElement element = domElement.firstChildElement(VAbstractPattern::TagNodes); not element.isNull())
        {
            initData.path = ParsePathNodes(element);
        }
        else
        {
            VExceptionObjectError excep(tr("Error creating or updating a piece path"), domElement);
            excep.AddMoreInformation(tr("Piece path doesn't contain nodes"));
            throw excep;
        }

        const QString defType = QString().setNum(static_cast<int>(PiecePathType::CustomSeamAllowance));
        initData.path.SetType(static_cast<PiecePathType>(GetParametrUInt(domElement, AttrType, defType)));
        initData.path.SetName(GetParametrString(domElement, AttrName, tr("Unnamed path")));
        initData.path.SetPenType(LineStyleToPenStyle(GetParametrString(domElement, AttrTypeLine, TypeLineLine)));

        if (initData.path.GetType() == PiecePathType::InternalPath)
        {
            initData.path.SetCutPath(GetParametrBool(domElement, AttrCut, falseStr));
            initData.path.SetFirstToCuttingContour(GetParametrBool(domElement, AttrFirstToContour, falseStr));
            initData.path.SetLastToCuttingContour(GetParametrBool(domElement, AttrLastToContour, falseStr));
            initData.path.SetVisibilityTrigger(GetParametrString(domElement, AttrVisible, QChar('1')));
            initData.path.SetNotMirrored(GetParametrBool(domElement, AttrNotMirrored, falseStr));
        }

        VToolPiecePath::Create(initData);
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating a piece path"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}
