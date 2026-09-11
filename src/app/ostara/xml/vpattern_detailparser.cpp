/************************************************************************
 **
 **  @file   vpattern_detailparser.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Detail/piece parsing methods extracted from vpattern.cpp
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
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/oannotationdata.h"
#include "../vpatterndb/oinfoblockdata.h"
#include "../vpatterndb/vnodedetail.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "vpattern_helpers.h"

#include <QtConcurrentRun>

using namespace VPatternHelpers;

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::ParseDetailNode(const QDomElement &domElement) -> VNodeDetail
{
    const quint32 id = GetParametrUInt(domElement, AttrIdObject, NULL_ID_STR);
    const qreal mx = GetParametrDouble(domElement, AttrMx, QStringLiteral("0.0"));
    const qreal my = GetParametrDouble(domElement, AttrMy, QStringLiteral("0.0"));
    const bool reverse = GetParametrUInt(domElement, VAbstractPattern::AttrNodeReverse, QChar('0'));
    const NodeDetail nodeType = NodeDetail::Contour;

    const QString t = GetParametrString(domElement, AttrType, QStringLiteral("NodePoint"));
    Tool tool;

    switch (QStringList const types{VAbstractPattern::NodePoint, VAbstractPattern::NodeArc,
                                    VAbstractPattern::NodeSpline, VAbstractPattern::NodeSplinePath,
                                    VAbstractPattern::NodeElArc};
            types.indexOf(t))
    {
        case 0: // NodePoint
            tool = Tool::NodePoint;
            break;
        case 1: // NodeArc
            tool = Tool::NodeArc;
            break;
        case 2: // NodeSpline
            tool = Tool::NodeSpline;
            break;
        case 3: // NodeSplinePath
            tool = Tool::NodeSplinePath;
            break;
        case 4: // NodeElArc
            tool = Tool::NodeElArc;
            break;
        default:
            throw VException(tr("Wrong tag name '%1'.").arg(t));
    }
    return {id, tool, nodeType, mx, my, reverse};
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::ParsePieceCartoucheBlocks(const QDomElement &domElement) const -> OInfoBlockData
{
    OInfoBlockData info;
    info.SetSet(true);
    info.SetActive(GetParametrBool(domElement, VToolSeamAllowance::AttrActive, falseStr));
    info.SetTemplate(GetParametrEmptyString(domElement, VAbstractPattern::AttrTemplate));

    QHash<QString, int> dispatch;
    QHash<int, OInfoBlockGeom> geoms;
    const QDomNodeList children = domElement.childNodes();
    QDOM_LOOP(children, i)
    {
        const QDomElement element = QDOM_ELEMENT(children, i).toElement();
        if (element.isNull())
        {
            continue;
        }
        if (element.tagName() == VToolSeamAllowance::TagDispatch)
        {
            const QString type = element.attribute(AttrType);
            const int block = element.attribute(VToolSeamAllowance::AttrBlock).toInt();
            if (not type.isEmpty() && block >= 1 && block <= 11)
            {
                dispatch.insert(type, block);
            }
        }
        else if (element.tagName() == VToolSeamAllowance::TagBlock)
        {
            const int index = element.attribute(VToolSeamAllowance::AttrIndex).toInt();
            if (index < 1 || index > 11)
            {
                continue;
            }
            OInfoBlockGeom geom;
            geom.pos = QPointF(GetParametrDouble(element, AttrMx, QStringLiteral("0.0")),
                               GetParametrDouble(element, AttrMy, QStringLiteral("0.0")));
            geom.width = GetParametrDouble(element, AttrWidth, QStringLiteral("0.0"));
            geom.height = GetParametrDouble(element, AttrHeight, QStringLiteral("0.0"));
            geom.rotation = GetParametrDouble(element, VAbstractPattern::AttrRotation, QStringLiteral("0.0"));
            geom.fontScale = GetParametrDouble(element, VToolSeamAllowance::AttrFontScale, QStringLiteral("1.0"));
            geoms.insert(index, geom);
        }
    }
    info.SetDispatch(dispatch);
    for (auto it = geoms.constBegin(); it != geoms.constEnd(); ++it)
    {
        info.SetBlockGeom(it.key(), it.value());
    }
    return info;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::ParsePieceAnnotations(const QDomElement &domElement) const -> OAnnotationData
{
    OAnnotationData annotations;
    annotations.SetSet(true);

    QList<OAnnotation> list;
    const QDomNodeList children = domElement.childNodes();
    QDOM_LOOP(children, i)
    {
        const QDomElement element = QDOM_ELEMENT(children, i).toElement();
        if (element.isNull() || element.tagName() != VToolSeamAllowance::TagAnnotation)
        {
            continue;
        }

        OAnnotation annotation;
        annotation.templateName = GetParametrEmptyString(element, VAbstractPattern::AttrTemplate);
        annotation.rotationFormula = GetParametrEmptyString(element, VAbstractPattern::AttrRotation);

        const QDomNodeList annotationChildren = element.childNodes();
        QDOM_LOOP(annotationChildren, k)
        {
            const QDomElement child = QDOM_ELEMENT(annotationChildren, k).toElement();
            if (child.isNull())
            {
                continue;
            }
            if (child.tagName() == VToolSeamAllowance::TagText)
            {
                annotation.text = child.text();
            }
            else if (child.tagName() == VToolSeamAllowance::TagGeom)
            {
                annotation.geom.pos =
                    QPointF(GetParametrDouble(child, AttrMx, QStringLiteral("0.0")),
                            GetParametrDouble(child, AttrMy, QStringLiteral("0.0")));
                annotation.geom.boxWidth = GetParametrDouble(child, AttrWidth, QStringLiteral("0.0"));
                annotation.geom.boxHeight = GetParametrDouble(child, AttrHeight, QStringLiteral("0.0"));
                annotation.geom.fontScale =
                    GetParametrDouble(child, VToolSeamAllowance::AttrFontScale, QStringLiteral("1.0"));
            }
        }
        list.append(annotation);
    }
    annotations.SetAnnotations(list);
    return annotations;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseDetailElement parse detail tag.
 * @param domElement tag in xml tree.
 * @param parse parser file mode.
 */
void VPattern::ParseDetailElement(QDomElement &domElement, const Document &parse)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");
    try
    {
        VToolSeamAllowanceInitData initData;
        initData.id = GetParametrId(domElement);
        initData.detail.SetName(GetParametrString(domElement, AttrName, tr("Detail")));
        initData.detail.SetUUID(GetParametrEmptyString(domElement, AttrUUID));
        initData.detail.SetMx(
            VAbstractValApplication::VApp()->toPixel(GetParametrDouble(domElement, AttrMx, QStringLiteral("0.0"))));
        initData.detail.SetMy(
            VAbstractValApplication::VApp()->toPixel(GetParametrDouble(domElement, AttrMy, QStringLiteral("0.0"))));
        initData.detail.SetSeamAllowance(GetParametrBool(domElement, VToolSeamAllowance::AttrSeamAllowance, falseStr));
        initData.detail.SetHideMainPath(
            GetParametrBool(domElement, VToolSeamAllowance::AttrHideMainPath,
                            QString().setNum(VAbstractValApplication::VApp()->ValentinaSettings()->IsHideMainPath())));
        initData.detail.SetSecondPassmarkOnSeamLine(
            GetParametrBool(domElement, VToolSeamAllowance::AttrSecondPassmarkOnSeamLine,
                            QString().setNum(VAbstractValApplication::VApp()->Settings()->IsDoublePassmark())));
        initData.detail.SetShowFullPiece(GetParametrBool(domElement, VToolSeamAllowance::AttrShowFullPiece, trueStr));
        initData.detail.SetForbidFlipping(GetParametrBool(
            domElement, AttrForbidFlipping,
            QString().setNum(VAbstractValApplication::VApp()->ValentinaSettings()->GetForbidWorkpieceFlipping())));
        initData.detail.SetForceFlipping(GetParametrBool(
            domElement, AttrForceFlipping,
            QString().setNum(VAbstractValApplication::VApp()->ValentinaSettings()->GetForceWorkpieceFlipping())));
        initData.detail.SetSymmetricalCopy(GetParametrBool(domElement, AttrSymmetricalCopy, falseStr));
        initData.detail.SetFollowGrainline(GetParametrBool(domElement, AttrFollowGrainline, falseStr));
        initData.detail.SetSewLineOnDrawing(GetParametrBool(
            domElement, AttrSewLineOnDrawing,
            QString().setNum(VAbstractValApplication::VApp()->ValentinaSettings()->GetSewLineOnDrawing())));
        initData.detail.SetInLayout(GetParametrBool(domElement, AttrInLayout, trueStr));
        initData.detail.SetUnited(GetParametrBool(domElement, VToolSeamAllowance::AttrUnited, falseStr));
        initData.detail.SetPriority(GetParametrUInt(domElement, VToolSeamAllowance::AttrPiecePriority, QChar('0')));
        initData.detail.SetGradedAlignH(GetParametrInt(domElement, AttrGradedAlignH, QStringLiteral("2")));
        initData.detail.SetGradedAlignV(GetParametrInt(domElement, AttrGradedAlignV, QStringLiteral("2")));
        initData.detail.SetPieceNumber(GetParametrUInt(domElement, AttrPieceNumber, QChar('0')));
        initData.detail.SetCutFabricQty(GetParametrInt(domElement, AttrCutFabricQty, QStringLiteral("0")));
        initData.detail.SetCutFabricOnFold(GetParametrBool(domElement, AttrCutFabricOnFold, falseStr));
        initData.detail.SetCutInterfacingQty(GetParametrInt(domElement, AttrCutInterfacingQty, QStringLiteral("0")));
        initData.detail.SetCutInterfacingOnFold(GetParametrBool(domElement, AttrCutInterfacingOnFold, falseStr));
        initData.detail.SetCutLiningQty(GetParametrInt(domElement, AttrCutLiningQty, QStringLiteral("0")));
        initData.detail.SetCutLiningOnFold(GetParametrBool(domElement, AttrCutLiningOnFold, falseStr));
        initData.detail.SetFreeText(GetParametrEmptyString(domElement, AttrFreeText));

        initData.width = GetParametrString(domElement, AttrWidth, QStringLiteral("0.0"));
        const QString w = initData.width; // need for saving fixed formula;

        ParseDetailInternals(domElement, initData.detail);

        initData.scene = sceneDetail;
        initData.doc = this;
        initData.data = data;
        initData.parse = parse;
        initData.typeCreation = Source::FromFile;

        if (const VToolSeamAllowance *piece = VToolSeamAllowance::Create(initData); parse == Document::FullParse)
        {
            updatePieces.append(piece->getId());
        }
        // Rewrite attribute formula. Need for situation when we have wrong formula.
        if (w != initData.width)
        {
            SetAttribute(domElement, AttrWidth, initData.width);
            modified = true;
            haveLiteChange();
        }
    }
    catch (const VExceptionBadId &e)
    {
        VExceptionObjectError excep(tr("Error creating or updating detail"), domElement);
        excep.AddMoreInformation(e.ErrorMessage());
        throw excep;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseDetailInternals(const QDomElement &domElement, VPiece &detail) const
{
    const uint version = GetParametrUInt(domElement, AttrVersion, QChar('1'));

    const QStringList tags{TagNodes,
                           TagGrainline,
                           VToolSeamAllowance::TagCSA,
                           VToolSeamAllowance::TagIPaths,
                           // Slot 4 ("pins"): kept so the case numbers below stay aligned with this list. The pin
                           // node type was removed in Ostara, and <pins> lists are stripped from the DOM while
                           // loading a .val — this slot is only ever a safety net.
                           QStringLiteral("pins"),
                           VToolSeamAllowance::TagPlaceLabels,
                           VAbstractPattern::TagMirrorLine,
                           VToolSeamAllowance::TagSaMarginProperties,
                           VToolSeamAllowance::TagCartoucheBlocks,
                           VToolSeamAllowance::TagAnnotations};

    QFuture<QVector<VPieceNode>> futurePathV1;
    QFuture<VPiecePath> futurePathV2;
    QFuture<VGrainlineData> futureGGeometry;
    QFuture<QVector<CustomSARecord>> futureRecords;
    QFuture<QVector<quint32>> futureIPaths;
    QFuture<QVector<quint32>> futurePlaceLabels;
    QFuture<VPieceFoldLineData> futureMirrorLine;

    const QDomNodeList nodeList = domElement.childNodes();
    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement(); not element.isNull())
        {
            switch (tags.indexOf(element.tagName()))
            {
                case 0: // TagNodes
                    if (version == 1)
                    {
                        // TODO. Delete if minimal supported version is 0.4.0
                        Q_STATIC_ASSERT_X(VPatternConverter::PatternMinVer < FormatVersion(0, 4, 0),
                                          "Time to refactor the code.");
                        futurePathV1 = QtConcurrent::run(
                            [this, domElement, element]() -> QVector<VPieceNode>
                            {
                                const bool closed = GetParametrUInt(domElement, AttrClosed, QChar('1'));
                                const qreal width = GetParametrDouble(domElement, AttrWidth, QStringLiteral("0.0"));
                                return ParseDetailNodes(element, width, closed);
                            });
                    }
                    else
                    {
                        futurePathV2 = QtConcurrent::run(&VPattern::ParsePieceNodes, element);
                    }
                    break;
                case 1: // TagGrainline
                    futureGGeometry = QtConcurrent::run(
                        [this, element, detail]() -> VGrainlineData
                        { return ParsePieceGrainline(element, detail.GetGrainlineGeometry()); });
                    break;
                case 2: // VToolSeamAllowance::TagCSA
                    futureRecords = QtConcurrent::run(&VPattern::ParsePieceCSARecords, element);
                    break;
                case 3: // VToolSeamAllowance::TagIPaths
                    futureIPaths = QtConcurrent::run(&VPattern::ParsePieceInternalPaths, element);
                    break;
                case 4: // "pins" — legacy element, stripped from the DOM at load. Nothing left to parse.
                    break;
                case 5: // VToolSeamAllowance::TagPlaceLabels
                    futurePlaceLabels = QtConcurrent::run(&VPattern::ParsePiecePointRecords, element);
                    break;
                case 6: // VToolSeamAllowance::TagMirrorLine
                    futureMirrorLine = QtConcurrent::run(&ParsePieceMirrorLine, element);
                    break;
                case 7: // VToolSeamAllowance::TagSaMarginProperties
                {
                    QVector<QVector<quint32>> saNodeIdGroups;
                    QVector<QString> saNames;
                    QVector<int> saColors;
                    QVector<bool> saShows;
                    const QDomNodeList saGroupNodes = element.childNodes();
                    QDOM_LOOP(saGroupNodes, k)
                    {
                        const QDomElement gEl = QDOM_ELEMENT(saGroupNodes, k).toElement();
                        if (gEl.isNull() || gEl.tagName() != QLatin1String("group"))
                        {
                            continue;
                        }
                        const QStringList idStr = gEl.attribute(QStringLiteral("nodeIds")).split(QLatin1Char(','));
                        QVector<quint32> grp;
                        for (const QString &s : idStr)
                        {
                            bool ok = false;
                            const quint32 id = s.toUInt(&ok);
                            if (ok && id > 0)
                            {
                                grp.append(id);
                            }
                        }
                        if (!grp.isEmpty())
                        {
                            saNodeIdGroups.append(grp);
                            saNames.append(gEl.attribute(QStringLiteral("name")));
                            saColors.append(gEl.attribute(QStringLiteral("colorIndex")).toInt());
                            saShows.append(gEl.attribute(QStringLiteral("show")) == QLatin1String("true"));
                        }
                    }
                    detail.SetSAMarginGroupNodeIds(saNodeIdGroups);
                    detail.SetSAMarginGroupNames(saNames);
                    detail.SetSAMarginGroupColorIndices(saColors);
                    detail.SetSAMarginGroupShowStates(saShows);
                    break;
                }
                case 8: // VToolSeamAllowance::TagCartoucheBlocks
                    detail.SetInfoBlockData(ParsePieceCartoucheBlocks(element));
                    break;
                case 9: // VToolSeamAllowance::TagAnnotations
                    detail.SetAnnotationData(ParsePieceAnnotations(element));
                    break;
                default:
                    break;
            }
        }
    }

    if (version == 1 && not futurePathV1.isCanceled())
    {
        detail.GetPath().SetNodes(futurePathV1.result());
    }
    else if (not futurePathV2.isCanceled())
    {
        detail.SetPath(futurePathV2.result());
    }

    if (not futureGGeometry.isCanceled())
    {
        detail.SetGrainlineGeometry(futureGGeometry.result());
    }

    if (not futureRecords.isCanceled())
    {
        detail.SetCustomSARecords(futureRecords.result());
    }

    if (not futureIPaths.isCanceled())
    {
        detail.SetInternalPaths(futureIPaths.result());
    }

    if (not futurePlaceLabels.isCanceled())
    {
        detail.SetPlaceLabels(futurePlaceLabels.result());
    }

    if (not futureMirrorLine.isCanceled())
    {
        VPieceFoldLineData const data = futureMirrorLine.result();
        detail.SetMirrorLineStartPoint(data.p1);
        detail.SetMirrorLineEndPoint(data.p2);
        detail.SetShowMirrorLine(data.mirrorLineVisible);
        detail.SetManualFoldHeight(data.manualHeight);
        detail.SetManualFoldWidth(data.manualWidth);
        detail.SetManualFoldCenter(data.manualCenter);
        detail.SetFormulaFoldHeight(data.heightFormula);
        detail.SetFormulaFoldWidth(data.widthFormula);
        detail.SetFormulaFoldCenter(data.centerFormula);
        detail.SetFoldLineType(data.type);
        detail.SetFoldLineLabel(data.label);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::ParseDetailNodes(const QDomElement &domElement, qreal width, bool closed) const -> QVector<VPieceNode>
{
    QVector<VNodeDetail> oldNodes;
    const QDomNodeList nodeList = domElement.childNodes();
    QDOM_LOOP(nodeList, i)
    {
        const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
        if (not element.isNull() &&
            element.tagName() == VAbstractPattern::TagNode) // Old detail version need this check!
        {
            oldNodes.append(ParseDetailNode(element));
        }
    }

    return VNodeDetail::Convert(data, oldNodes, width, closed);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::ParsePieceGrainline(const QDomElement &domElement, VGrainlineData gGeometry) const -> VGrainlineData
{
    gGeometry.SetEnabled(GetParametrBool(domElement, AttrEnabled, falseStr));
    gGeometry.SetVisible(GetParametrBool(domElement, AttrVisible, trueStr));
    gGeometry.SetPos(
        QPointF(GetParametrDouble(domElement, AttrMx, QChar('0')), GetParametrDouble(domElement, AttrMy, QChar('0'))));
    gGeometry.SetArrowType(static_cast<GrainlineArrowDirection>(GetParametrUInt(domElement, AttrArrows, QChar('0'))));

    // Pin anchors (topPin/bottomPin/centerPin) are gone along with the pin node type. A pin-anchored grainline is
    // disabled and stripped from the DOM by StripPinElements while loading a .val, so what reaches this point is
    // always a free grainline driven by its own formulas.
    gGeometry.SetLength(GetParametrString(domElement, AttrLength, QChar('1')));
    gGeometry.SetRotation(GetParametrString(domElement, AttrRotation, QString::number(90)));

    return gGeometry;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseDetails parse details tag.
 * @param domElement tag in xml tree.
 * @param parse parser file mode.
 */
void VPattern::ParseDetails(const QDomElement &domElement, const Document &parse)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");
    QDomNode domNode = domElement.firstChild();
    while (!domNode.isNull())
    {
        if (domNode.isElement())
        {
            if (QDomElement domElement = domNode.toElement(); !domElement.isNull() && domElement.tagName() == TagDetail)
            {
                ParseDetailElement(domElement, parse);
            }
        }
        domNode = domNode.nextSibling();
    }

    if (parse == Document::FullParse)
    {
        emit MadeProgress();
    }
}
