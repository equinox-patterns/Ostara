/************************************************************************
 **
 **  @file   vpattern.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   24 2, 2014
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
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

#include "vpattern.h"
#include "../core/vapplication.h"
#include "../ifc/exception/vexceptionconversionerror.h"
#include "../ifc/exception/vexceptionemptyparameter.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../ifc/exception/vexceptionundo.h"
#include "../ifc/exception/vexceptionwrongid.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../qmuparser/qmuparsererror.h"
#include "../qmuparser/qmutokenparser.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vsplinepath.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/customevents.h"
#include "../vmisc/def.h"
#include "../vmisc/projectversion.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/ochildanchor.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vpatterndb/vnodedetail.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
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
#include "../vtools/tools/nodeDetails/vnodearc.h"
#include "../vtools/tools/nodeDetails/vnodeellipticalarc.h"
#include "../vtools/tools/nodeDetails/vnodepoint.h"
#include "../vtools/tools/nodeDetails/vnodespline.h"
#include "../vtools/tools/nodeDetails/vnodesplinepath.h"
#include "../vtools/tools/nodeDetails/vtoolpiecepath.h"
#include "../vtools/tools/nodeDetails/vtoolplacelabel.h"
#include "../vtools/tools/vdatatool.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/tools/vtooluniondetails.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "tools/drawTools/toolcurve/vtoolparallelcurve.h"
#include "typedef.h"
#include "vpattern_helpers.h"

#include <chrono>
#include <functional>
#include <qstringliteral.h>
#include <QDebug>
#include <QFileInfo>
#include <QFuture>
#include <QMessageBox>
#include <QScopeGuard>
#include <QTimer>
#include <QUndoStack>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QtNumeric>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif /*Q_OS_WIN*/

using namespace std::chrono_literals;

using namespace Qt::Literals::StringLiterals;

const QString VPattern::AttrReadOnly = QStringLiteral("readOnly");       // NOLINT(cert-err58-cpp)
const QString VPattern::AttrLabelPrefix = QStringLiteral("labelPrefix"); // NOLINT(cert-err58-cpp)

using namespace VPatternHelpers;

//---------------------------------------------------------------------------------------------------------------------
VPattern::VPattern(VContainer *data, VMainGraphicsScene *sceneDraw, VMainGraphicsScene *sceneDetail, QObject *parent)
  : VAbstractPattern(parent),
    data(data),
    sceneDraw(sceneDraw),
    sceneDetail(sceneDetail),
    m_refreshGeometryTimer(new QTimer(this))
{
    SCASSERT(sceneDraw != nullptr)
    SCASSERT(sceneDetail != nullptr)

    m_refreshGeometryTimer->setSingleShot(true);
    m_refreshGeometryTimer->setTimerType(Qt::CoarseTimer);
    m_refreshGeometryTimer->setInterval(250ms);
    connect(m_refreshGeometryTimer, &QTimer::timeout, this, &VPattern::RefreshPieceGeometry);

    connect(this, &VAbstractPattern::PatternDependencyGraphCompleted, this, &VPattern::CollectGarbage);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief CreateEmptyFile create minimal empty file.
 */
void VPattern::CreateEmptyFile()
{
    this->clear();
    QDomElement patternElement = this->createElement(TagPattern);

    patternElement.appendChild(createComment(FileComment()));
    patternElement.appendChild(CreateElementWithText(TagVersion, QStringLiteral("1.0.0")));
    patternElement.appendChild(
        CreateElementWithText(TagUnit, UnitsToStr(VAbstractValApplication::VApp()->patternUnits())));

    patternElement.appendChild(createElement(TagDescription));
    patternElement.appendChild(createElement(TagNotes));

    patternElement.appendChild(createElement(TagMeasurements));
    patternElement.appendChild(createElement(TagIncrements));
    patternElement.appendChild(createElement(TagPreviewCalculations));

    this->appendChild(patternElement);
    insertBefore(
        createProcessingInstruction(QStringLiteral("xml"), QStringLiteral("version=\"1.0\" encoding=\"UTF-8\"")),
        this->firstChild());

    // Cache values
    m_units = VAbstractValApplication::VApp()->patternUnits();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Parse parse file.
 * @param parse parser file mode.
 */
void VPattern::Parse(const Document &parse)
{
    emit PreParseState();
    SCASSERT(sceneDraw != nullptr)
    SCASSERT(sceneDetail != nullptr)
    PrepareForParse(parse);

    // PrepareForParse() disables dependency-graph rebuilding only for a FullLiteParse (values-only
    // reparse). Re-enable it once the parse finishes - even on exception - so later single-tool/GUI
    // edits and LiteParse/FullParse runs keep updating the graph. FullParse/LiteParse leave
    // rebuilding enabled via Clear().
    auto graphRebuildGuard = qScopeGuard(
        [this, parse]() -> void
        {
            if (parse == Document::FullLiteParse)
            {
                PatternGraph()->SetGraphRebuildEnabled(true);
            }
        });

    m_fileParsingCompleted = false;
    QDomNode domNode = documentElement().firstChild();
    while (not domNode.isNull())
    {
        ParseRootElement(parse, domNode);
        domNode = domNode.nextSibling();
    }

    m_fileParsingCompleted = true;

    // Adopt an anchor for every piece child that carries a deliberate placement but no anchor
    // yet — a file written before the anchor existed. Done here rather than left to the next
    // geometry refresh because a LiteParseTree rebuilds the container from the DOM with no
    // geometry pass in between, and the Graded layers read that container right after.
    MaterializeChildAnchors();

    if (IsPatternGraphComplete())
    {
        emit PatternDependencyGraphCompleted();
    }

    if (VApplication::IsGUIMode())
    {
        // Restart the debounce timer so a burst of parses collapses into a single refresh that runs
        // shortly after the last parse. Discrete actions (e.g. switching size) flush it immediately
        // by calling RefreshPieceGeometry() directly, which cancels the pending timer.
        m_refreshGeometryTimer->start();
    }
    else if (VApplication::CommandLine()->IsTestModeEnabled())
    {
        RefreshPieceGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::MaterializeChildAnchors()
{
    if (data == nullptr)
    {
        return;
    }

    // Pieces only. A group's children are stored in the holder's local frame, whose origin is
    // the union of its members' *rendered* bounds — a scene notion that does not exist yet at
    // this point, and one that would not match an origin guessed from the node points alone.
    // OPieceGroupHolderItem::Refresh does the group's own pass, which a change of size reaches
    // through CheckLayout.
    const QList<quint32> ids = data->DataPieces()->keys();
    for (quint32 id : ids)
    {
        const VPiece piece = data->GetPiece(id);
        const QRectF ref = OChildAnchor::RefRect(piece, data);
        if (ref.isNull())
        {
            continue; // no usable node — nothing to anchor against
        }

        VPiece followed = piece;
        if (OChildAnchor::Materialize(followed, ref))
        {
            data->UpdatePiece(id, followed);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
namespace
{
/// Writes a child's absolute position and its anchor onto an element the document already holds.
///
/// Never recreates the element: the schema checks the order of xs:sequence, so moving a child
/// element would make the file fail validation. The anchor pair is written exactly when an anchor
/// is recorded — the same predicate the reader uses, or the attributes would come and go on every
/// save/reload round.
void WriteChildGeometry(VDomDocument *doc, QDomElement &element, const QPointF &pos,
                        const ONormalizedAnchor &anchor)
{
    doc->SetAttribute(element, AttrMx, pos.x());
    doc->SetAttribute(element, AttrMy, pos.y());

    if (anchor.valid)
    {
        doc->SetAttribute(element, AttrNx, anchor.uv.x());
        doc->SetAttribute(element, AttrNy, anchor.uv.y());
    }
    else
    {
        element.removeAttribute(AttrNx);
        element.removeAttribute(AttrNy);
    }
}

/// The children of one piece or one group, mirrored into its element.
void SyncOwnerChildren(VDomDocument *doc, QDomElement &ownerElement, const IPieceInfo &owner,
                       const VGrainlineData &grainline)
{
    if (QDomElement grainlineEl = ownerElement.firstChildElement(VAbstractPattern::TagGrainline);
        !grainlineEl.isNull())
    {
        WriteChildGeometry(doc, grainlineEl, grainline.GetPos(), grainline.GetAnchor());
    }

    if (QDomElement blocksEl = ownerElement.firstChildElement(VToolSeamAllowance::TagCartoucheBlocks);
        !blocksEl.isNull())
    {
        const OInfoBlockData &blockData = owner.GetInfoBlockData();
        QDomElement blockEl = blocksEl.firstChildElement(VToolSeamAllowance::TagBlock);
        while (!blockEl.isNull())
        {
            const int index = blockEl.attribute(VToolSeamAllowance::AttrIndex).toInt();
            // A block the model no longer stores is left alone rather than erased: the writers own
            // the shape of the element, this pass only refreshes what is already in it.
            if (index >= 1 && index <= 11 && blockData.HasStoredGeometry(index))
            {
                const OInfoBlockGeom geom = blockData.BlockGeom(index);
                WriteChildGeometry(doc, blockEl, geom.pos, geom.anchor);
            }
            blockEl = blockEl.nextSiblingElement(VToolSeamAllowance::TagBlock);
        }
    }

    if (QDomElement annotationsEl = ownerElement.firstChildElement(VToolSeamAllowance::TagAnnotations);
        !annotationsEl.isNull())
    {
        // The nth <annotation> is the nth of the model list: the writer emits them in list order,
        // and the element is an Ostara creation, so no other producer can have ordered it.
        const QList<OAnnotation> annotations = owner.GetAnnotationData().Annotations();
        QDomElement annotationEl = annotationsEl.firstChildElement(VToolSeamAllowance::TagAnnotation);
        int index = 0;
        while (!annotationEl.isNull() && index < annotations.size())
        {
            if (QDomElement geomEl = annotationEl.firstChildElement(VToolSeamAllowance::TagGeom);
                !geomEl.isNull())
            {
                const OAnnotationGeom &geom = annotations.at(index).geom;
                WriteChildGeometry(doc, geomEl, geom.pos, geom.anchor);
            }
            annotationEl = annotationEl.nextSiblingElement(VToolSeamAllowance::TagAnnotation);
            ++index;
        }
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void VPattern::SyncChildGeometryToDom()
{
    QDomElement root = documentElement();
    if (root.isNull() || data == nullptr)
    {
        return;
    }

    if (QDomElement details = root.firstChildElement(TagDetails); !details.isNull())
    {
        const auto *pieces = data->DataPieces();
        QDomElement detail = details.firstChildElement(TagDetail);
        while (!detail.isNull() && pieces != nullptr)
        {
            const quint32 id = detail.attribute(VDomDocument::AttrId).toUInt();
            if (pieces->contains(id))
            {
                const VPiece piece = pieces->value(id);
                SyncOwnerChildren(this, detail, piece, piece.GetGrainlineGeometry());
            }
            detail = detail.nextSiblingElement(TagDetail);
        }
    }

    if (QDomElement groups = root.firstChildElement(TagPieceGroups); !groups.isNull())
    {
        const auto *all = data->DataPieceGroups();
        QDomElement group = groups.firstChildElement(TagPieceGroup);
        while (!group.isNull() && all != nullptr)
        {
            const quint32 id = group.attribute(VDomDocument::AttrId).toUInt();
            if (all->contains(id))
            {
                const OPieceGroup pieceGroup = all->value(id);
                SyncOwnerChildren(this, group, pieceGroup, pieceGroup.m_glGrainline);
            }
            group = group.nextSiblingElement(TagPieceGroup);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::Clear()
{
    VAbstractPattern::Clear();
    updatePieces.clear();

    m_refreshGeometryTimer->stop(); // Drop any pending refresh; it would run against a cleared document.

    m_pieceGeometryDirty = true;

    m_garbageCollected = false;
    m_garbageCollectBackupFilePath.clear();
    history.clear();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setCurrentData set current data set.
 *
 * Each time after parsing need set correct data set for current pattern piece. After parsing it is always last.
 * Current data set for pattern pice it is data set for last object in pattern pice (point, arc, spline, spline path so
 * on).
 */
void VPattern::setCurrentData()
{
    qCDebug(vXML, "Setting current data");
    qCDebug(vXML, "Current PP name %s", qUtf8Printable(PatternBlockMapper()->GetActive()));
    qCDebug(vXML, "PP count %d", CountPatternBlockTags());

    const QVector<VToolRecord> localHistory = GetLocalHistory();
    if (localHistory.isEmpty())
    {
        qCDebug(vXML, "History is empty!");
        return;
    }

    const quint32 id = localHistory.constLast().GetId();
    qCDebug(vXML, "Resoring data from tool with id %u", id);

    if (tools.isEmpty())
    {
        qCDebug(vXML, "List of tools is empty!");
        return;
    }

    try
    {
        ToolExists(id);
    }
    catch (VExceptionBadId &e)
    {
        Q_UNUSED(e)
        qCDebug(vXML, "List of tools doesn't containe id= %u", id);
        return;
    }

    const VDataTool *vTool = tools.value(id);
    *data = vTool->GetDataCopy();
    // Delete special variables if exist
    data->RemoveVariable(currentLength);
    data->RemoveVariable(currentSeamAllowance);
    qCDebug(vXML, "Data successfully updated.");
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdateToolData update tool in list tools.
 * @param id tool id.
 * @param data container with variables.
 */
void VPattern::UpdateToolData(const quint32 &id, VContainer *data)
{
    Q_ASSERT_X(id != 0, Q_FUNC_INFO, "id == 0"); //-V712 //-V654
    SCASSERT(data != nullptr)
    if (!tools.contains(id))
    {
        // Tool was deleted (e.g. an undo command freed its owner piece); skip gracefully.
        return;
    }
    VDataTool *tool = tools.value(id);
    SCASSERT(tool != nullptr)
    tool->VDataTool::setData(data);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::GetCompleteData() const -> VContainer
{
    const int countPP = CountPatternBlockTags();
    if (countPP <= 0 || history.isEmpty() || tools.isEmpty())
    {
        return (data != nullptr ? *data : VContainer(nullptr, nullptr, VContainer::UniqueNamespace()));
    }

    const quint32 id = (countPP == 1 ? history.constLast().GetId() : LastToolId());

    if (id == NULL_ID)
    {
        return (data != nullptr ? *data : VContainer(nullptr, nullptr, VContainer::UniqueNamespace()));
    }

    try
    {
        ToolExists(id);
    }
    catch (VExceptionBadId &e)
    {
        Q_UNUSED(e)
        return (data != nullptr ? *data : VContainer(nullptr, nullptr, VContainer::UniqueNamespace()));
    }

    const VDataTool *vTool = tools.value(id);
    SCASSERT(vTool != nullptr)
    VContainer lastData = vTool->GetDataCopy();
    // Delete special variables if exist
    lastData.RemoveVariable(currentLength);
    lastData.RemoveVariable(currentSeamAllowance);
    return lastData;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::GetCompletePPData(const QString &name) const -> VContainer
{
    const int countPP = CountPatternBlockTags();
    if (countPP <= 0 || history.isEmpty() || tools.isEmpty())
    {
        return (data != nullptr ? *data : VContainer(nullptr, nullptr, VContainer::UniqueNamespace()));
    }

    const quint32 id = (countPP == 1 ? history.constLast().GetId() : PPLastToolId(PatternBlockMapper()->FindId(name)));

    if (id == NULL_ID)
    {
        return (data != nullptr ? *data : VContainer(nullptr, nullptr, VContainer::UniqueNamespace()));
    }

    try
    {
        ToolExists(id);
    }
    catch (VExceptionBadId &e)
    {
        Q_UNUSED(e)
        return (data != nullptr ? *data : VContainer(nullptr, nullptr, VContainer::UniqueNamespace()));
    }

    const VDataTool *vTool = tools.value(id);
    SCASSERT(vTool != nullptr)
    VContainer lastData = vTool->GetDataCopy();
    // Delete special variables if exist
    lastData.RemoveVariable(currentLength);
    lastData.RemoveVariable(currentSeamAllowance);
    return lastData;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SPointActiveDraw return id base point current pattern peace.
 * @return id base point.
 */
// cppcheck-suppress unusedFunction
auto VPattern::SPointActiveDraw() -> quint32
{
    if (QDomElement calcElement; GetActivNodeElement(TagCalculation, calcElement))
    {
        if (const QDomNode domNode = calcElement.firstChild(); not domNode.isNull() && domNode.isElement())
        {
            if (const QDomElement domElement = domNode.toElement();
                not domElement.isNull() && domElement.tagName() == TagPoint &&
                domElement.attribute(AttrType, QString()) == VToolBasePoint::ToolType)
            {
                return GetParametrId(domElement);
            }
        }
    }
    return 0;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::GetActivePPPieces() const -> QVector<quint32>
{
    QDomElement const drawElement = PatternBlockMapper()->GetActiveElement();
    if (drawElement.isNull())
    {
        return {};
    }

    const QDomElement details = drawElement.firstChildElement(TagDetails);
    if (details.isNull())
    {
        return {};
    }

    QVector<quint32> pieces;
    QDomElement detail = details.firstChildElement(TagDetail);
    while (!detail.isNull())
    {
        if (bool const united = GetParametrBool(detail, VToolSeamAllowance::AttrUnited, falseStr); not united)
        {
            pieces.append(GetParametrId(detail));
        }
        detail = detail.nextSiblingElement(TagDetail);
    }

    return pieces;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::SaveDocument(const QString &fileName, QString &error) -> bool
{
    try
    {
        TestUniqueId();
    }
    catch (const VExceptionWrongId &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error not unique id.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        return false;
    }

    // Update comment with Valentina version
    if (QDomNode const commentNode = documentElement().firstChild(); commentNode.isComment())
    {
        QDomComment comment = commentNode.toComment();
        comment.setData(FileComment());
    }

    SyncLineNamesToDom();
    // A position that moved with the active size exists in the container only: the refresh that
    // moved it pushes no undo command, so no writer has seen it. This is where it reaches the file.
    SyncChildGeometryToDom();
    const bool saved = VAbstractPattern::SaveDocument(fileName, error);
    if (saved && QFileInfo(fileName).suffix() != "autosave"_L1)
    {
        modified = false;
    }

    return saved;
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::ElementsToParse() const -> int
{
    QVector<QString> const tags{TagCalculation, TagDetails, TagModeling, TagIncrements};

    std::function<int(const QString &tagName)> const TagsCount = [this](const QString &tagName)
    { return elementsByTagName(tagName).length(); };

    return QtConcurrent::blockingMappedReduced(tags, TagsCount, GatherCount);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LiteParseTree lite parse file.
 */
void VPattern::LiteParseTree(const Document &parse)
{
    // Save current pattern block name
    QString const namePP = PatternBlockMapper()->GetActive();

    try
    {
        emit SetEnabledGUI(true);
        switch (parse)
        {
            case Document::LitePPParse:
                PatternGraph()->SetGraphRebuildEnabled(false);
                ParseCurrentPP();
                PatternGraph()->SetGraphRebuildEnabled(true);
                break;
            case Document::FullLiteParse:
            case Document::LiteParse:
                Parse(parse);
                break;
            case Document::FullParse:
                qCWarning(vXML, "Lite parsing doesn't support full parsing");
                break;
            default:
                break;
        }
    }
    catch (const VExceptionUndo &e)
    {
        Q_UNUSED(e)
        /* If user want undo last operation before undo we need finish broken redo operation. For those we post event
         * myself. Later in method customEvent call undo.*/
        QApplication::postEvent(this, new UndoEvent());
        return;
    }
    catch (const VExceptionObjectError &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error parsing file.")), //-V807
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
    catch (const VExceptionConversionError &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error can't convert value.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
    catch (const VExceptionEmptyParameter &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error empty parameter.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
    catch (const VExceptionWrongId &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error wrong id.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
    catch (VException &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error parsing file.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
    catch (const std::bad_alloc &)
    {
        qCCritical(vXML, "%s", qUtf8Printable(tr("Error parsing file (std::bad_alloc).")));
        emit SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }

    // Restore current pattern block name
    PatternBlockMapper()->SetActive(namePP);
    qCDebug(vXML, "Current pattern piece %s", qUtf8Printable(PatternBlockMapper()->GetActive()));
    setCurrentData();
    try
    {
        emit FullUpdateFromFile();
    }
    catch (VException &e)
    {
        qCCritical(vXML, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error updating scene after parsing.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        emit SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
    // Recalculate scene rect
    VMainGraphicsView::NewSceneRect(sceneDraw, VAbstractValApplication::VApp()->getSceneView());
    VMainGraphicsView::NewSceneRect(sceneDetail, VAbstractValApplication::VApp()->getSceneView());
    qCDebug(vXML, "Scene size updated.");
}

//---------------------------------------------------------------------------------------------------------------------
// cppcheck-suppress unusedFunction
void VPattern::customEvent(QEvent *event)
{
    if (event->type() == UNDO_EVENT)
    {
        VAbstractApplication::VApp()->getUndoStack()->undo();
    }
    else if (event->type() == LITE_PARSE_EVENT)
    {
        LiteParseTree(Document::LiteParse);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseRootElement(const Document &parse, const QDomNode &node)
{
    static const QStringList tags(
        {TagLineNames, TagDraw, TagIncrements, TagPreviewCalculations, TagPieceGroups, QStringLiteral("sizeRanges")});

    if (not node.isElement())
    {
        return;
    }

    const QDomElement domElement = node.toElement();
    if (domElement.isNull())
    {
        return;
    }

    switch (tags.indexOf(domElement.tagName()))
    {
        case 0: // TagLineNames
            ParseLineNamesElement(domElement);
            break;
        case 1: // TagDraw
        {
            VPatternBlockMapper *blocks = PatternBlockMapper();
            if (parse == Document::FullParse)
            {
                int const id = blocks->AddBlock(GetParametrString(domElement, AttrName), domElement);
                blocks->SetActiveById(id);
            }
            else
            {
                const QSignalBlocker blocker(blocks);
                blocks->SetActive(GetParametrString(domElement, AttrName));
            }
            ParseDrawElement(domElement, parse);
            break;
        }
        case 2: // TagIncrements
            if (parse != Document::LiteParse)
            {
                ParseIncrementsElement(domElement, parse);
            }
            break;
        case 3: // TagPreviewCalculations
            if (parse != Document::LiteParse)
            {
                ParseIncrementsElement(domElement, parse);
            }
            break;
        case 4: // TagPieceGroups
            if (parse != Document::LiteParse)
            {
                ParsePieceGroups(domElement);
            }
            break;
        case 5: // sizeRanges
            if (parse != Document::LiteParse)
            {
                ParseSizeRanges(domElement);
            }
            break;
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParsePieceGroups(const QDomElement &domElement)
{
    Q_ASSERT_X(not domElement.isNull(), Q_FUNC_INFO, "domElement is null");

    QDomNode domNode = domElement.firstChild();
    while (not domNode.isNull())
    {
        if (domNode.isElement())
        {
            const QDomElement groupElement = domNode.toElement();
            if (groupElement.tagName() == TagPieceGroup)
            {
                OPieceGroup group;
                group.id = GetParametrId(groupElement);
                data->UpdateId(group.id);
                group.name = GetParametrEmptyString(groupElement, VAbstractPattern::AttrName);
                group.alignH = GetParametrInt(groupElement, AttrGradedAlignH, QStringLiteral("2"));
                group.alignV = GetParametrInt(groupElement, AttrGradedAlignV, QStringLiteral("2"));
                group.m_inLayout = GetParametrBool(groupElement, AttrInLayout, QStringLiteral("true"));

                const uint groupPieceNumber = GetParametrUInt(groupElement, AttrPieceNumber, QChar('0'));
                group.m_cutFabricQty = GetParametrInt(groupElement, AttrCutFabricQty, QChar('0'));
                group.m_cutFabricOnFold = GetParametrBool(groupElement, AttrCutFabricOnFold, QStringLiteral("false"));
                group.m_cutInterfacingQty = GetParametrInt(groupElement, AttrCutInterfacingQty, QChar('0'));
                group.m_cutInterfacingOnFold =
                    GetParametrBool(groupElement, AttrCutInterfacingOnFold, QStringLiteral("false"));
                group.m_cutLiningQty = GetParametrInt(groupElement, AttrCutLiningQty, QChar('0'));
                group.m_cutLiningOnFold = GetParametrBool(groupElement, AttrCutLiningOnFold, QStringLiteral("false"));
                group.m_freeText = GetParametrEmptyString(groupElement, AttrFreeText);

                QDomNode memberNode = groupElement.firstChild();
                while (not memberNode.isNull())
                {
                    if (memberNode.isElement())
                    {
                        const QDomElement childElement = memberNode.toElement();
                        const QString tagName = childElement.tagName();
                        if (tagName == TagMember)
                        {
                            const quint32 pieceId =
                                GetParametrUInt(childElement, QStringLiteral("piece"), NULL_ID_STR);
                            if (pieceId != NULL_ID)
                            {
                                group.pieceIds.append(pieceId);

                                const QString offsetStr =
                                    GetParametrEmptyString(childElement, QStringLiteral("offset"));
                                if (not offsetStr.isEmpty())
                                {
                                    const QStringList parts = offsetStr.split(QLatin1Char(','));
                                    if (parts.size() == 2)
                                    {
                                        bool okX = false;
                                        bool okY = false;
                                        const qreal x = parts.at(0).toDouble(&okX);
                                        const qreal y = parts.at(1).toDouble(&okY);
                                        if (okX && okY)
                                        {
                                            group.memberOffsets[pieceId] = QPointF(x, y);
                                        }
                                    }
                                }
                            }
                        }
                        else if (tagName == TagGrainline)
                        {
                            group.m_glGrainline = ParsePieceGrainline(childElement, group.m_glGrainline);
                        }
                        else if (tagName == VToolSeamAllowance::TagCartoucheBlocks)
                        {
                            group.m_infoBlocks = ParsePieceCartoucheBlocks(childElement);
                        }
                        else if (tagName == VToolSeamAllowance::TagAnnotations)
                        {
                            group.m_annotations = ParsePieceAnnotations(childElement);
                        }
                        else if (tagName == VAbstractPattern::TagMirrorLine)
                        {
                            // Carrier pointer only: the whole fold geometry, the dimensions included,
                            // lives on the member piece designated here — the one that draws it.
                            group.m_foldLineMemberId =
                                GetParametrUInt(childElement, QStringLiteral("foldLineMember"), NULL_ID_STR);
                        }
                    }
                    memberNode = memberNode.nextSibling();
                }

                if (groupPieceNumber > 0)
                {
                    group.m_pieceNumber = groupPieceNumber;
                }
                data->AddPieceGroup(group);
            }
        }
        domNode = domNode.nextSibling();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseDrawElement parse draw tag.
 * @param node node.
 * @param parse parser file mode.
 */
void VPattern::ParseDrawElement(const QDomNode &node, const Document &parse)
{
    QStringList const tags{TagCalculation, TagModeling, TagDetails, TagGroups, TagAttachments};
    QDomNode domNode = node.firstChild();
    while (not domNode.isNull())
    {
        if (not domNode.isElement())
        {
            domNode = domNode.nextSibling();
            continue;
        }

        const QDomElement domElement = domNode.toElement();
        if (domElement.isNull())
        {
            domNode = domNode.nextSibling();
            continue;
        }

        switch (tags.indexOf(domElement.tagName()))
        {
            case 0: // TagCalculation
                data->ClearCalculationGObjects();
                ParseDrawMode(domElement, parse, Draw::Calculation);
                break;
            case 1: // TagModeling
                ParseDrawMode(domElement, parse, Draw::Modeling);
                break;
            case 2: // TagDetails
                ParseDetails(domElement, parse);
                break;
            case 3: // TagGroups
                ParseGroups(domElement);
                break;
            case 4: // TagAttachments
                ParseAttachments(domElement);
                break;
            default:
                throw VException(tr("Wrong tag name '%1'.").arg(domElement.tagName()));
        }
        domNode = domNode.nextSibling();
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ParseDrawMode parse draw tag with draw mode.
 * @param node node.
 * @param parse parser file mode.
 * @param mode draw mode.
 */
void VPattern::ParseDrawMode(const QDomNode &node, const Document &parse, const Draw &mode)
{
    SCASSERT(sceneDraw != nullptr)
    SCASSERT(sceneDetail != nullptr)

    const QDomNodeList nodeList = node.childNodes();
    QDOM_LOOP(nodeList, i)
    {
        QDomElement domElement = QDOM_ELEMENT(nodeList, i).toElement();
        ParseDrawModeElement(domElement, parse, mode);
    }

    if (parse == Document::FullParse)
    {
        emit MadeProgress();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ParseDrawModeElement(QDomElement &domElement, const Document &parse, const Draw &mode)
{
    VMainGraphicsScene *scene = mode == Draw::Calculation ? sceneDraw : sceneDetail;
    static const QStringList tags({TagPoint, TagLine, TagSpline, TagArc, TagTools, TagOperation, TagElArc, TagPath});

    if (domElement.isNull())
    {
        return;
    }

    switch (tags.indexOf(domElement.tagName()))
    {
        case 0: // TagPoint
            ParsePointElement(scene, domElement, parse, domElement.attribute(AttrType, QString()));
            break;
        case 1: // TagLine
            ParseLineElement(scene, domElement, parse);
            break;
        case 2: // TagSpline
            ParseSplineElement(scene, domElement, parse, domElement.attribute(AttrType, QString()));
            break;
        case 3: // TagArc
            ParseArcElement(scene, domElement, parse, domElement.attribute(AttrType, QString()));
            break;
        case 4: // TagTools
            ParseToolsElement(scene, domElement, parse, domElement.attribute(AttrType, QString()));
            break;
        case 5: // TagOperation
            ParseOperationElement(scene, domElement, parse, domElement.attribute(AttrType, QString()));
            break;
        case 6: // TagElArc
            ParseEllipticalArcElement(scene, domElement, parse, domElement.attribute(AttrType, QString()));
            break;
        case 7: // TagPath
            ParsePathElement(scene, domElement, parse);
            break;
        default:
            throw VException(tr("Wrong tag name '%1'.").arg(domElement.tagName()));
    }
}



//---------------------------------------------------------------------------------------------------------------------
void VPattern::PrepareForParse(const Document &parse)
{
    SCASSERT(sceneDraw != nullptr)
    SCASSERT(sceneDetail != nullptr)

    emit CancelLabelRendering();

    // Clear line role-name cache; will be repopulated from <lineNames> during parse
    data->ClearLineRoleNames();

    // We can run garbage collection only in a narrow window between first successfull parse and user change
    if (!m_garbageCollected && !IsPatternGraphComplete())
    {
        disconnect(this, &VAbstractPattern::PatternDependencyGraphCompleted, this, &VPattern::CollectGarbage);
        m_garbageCollected = true;
    }

    CancelFormulaDependencyChecks();

    if (parse == Document::FullLiteParse)
    {
        // FullLiteParse is the only value-only reparse (size switch, measurement sync, increment
        // edit): formulas and structure are unchanged, only variable values differ, objects keep
        // their ids, and increments are not graph vertices. So keep the dependency graph built by the
        // last full parse and skip rebuilding it - AddVertex/AddEdge/FindFormulaDependencies become
        // no-ops while disabled; Parse() re-enables rebuilding once the parse finishes.
        //
        // LiteParse is intentionally NOT skipped: it follows a tool edit (SaveToolOptions), whose
        // changed formula may add or remove dependency edges, so its graph must be rebuilt from
        // scratch - just like a full parse. The parse type alone does not tell us that; the
        // FullLiteParse-vs-LiteParse split is what carries the "values only" guarantee.
        PatternGraph()->SetGraphRebuildEnabled(false);
    }
    else
    {
        PatternGraph()->Clear();
    }

    if (parse == Document::FullParse)
    {
        RefreshElementIdCache();
        sceneDraw->clear();
        sceneDetail->clear();
        data->ClearForFullParse();
        data->UpdateId(MaxRecordedId()); // Resync against orphaned nodes that keep their id (upstream 51adb0101).
        PatternBlockMapper()->Clear();

        qDeleteAll(toolsOnRemove); // Remove all invisible on a scene objects.
        toolsOnRemove.clear();

        tools.clear();
        history.clear();
    }
    else if (parse == Document::LiteParse || parse == Document::FullLiteParse)
    {
        Q_STATIC_ASSERT_X(static_cast<int>(VarType::Unknown) == 12, "Check that you used all types");
        QVector<VarType> types{VarType::LineAngle,         VarType::LineLength,       VarType::CurveLength,
                               VarType::CurveCLength,      VarType::ArcRadius,        VarType::CurveAngle,
                               VarType::PieceExternalArea, VarType::PieceSeamLineArea};
        if (parse == Document::FullLiteParse)
        {
            types.append(VarType::Increment);
            types.append(VarType::IncrementSeparator);
        }

        data->ClearVariables(types);
        parse == Document::FullLiteParse ? data->ClearUniqueNames() : data->ClearExceptUniqueIncrementNames();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::ToolsCommonAttributes(const QDomElement &domElement, quint32 &id)
{
    id = GetParametrId(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void VPattern::DrawToolsCommonAttributes(const QDomElement &domElement, quint32 &id, QString &notes)
{
    ToolsCommonAttributes(domElement, id);
    notes = GetParametrEmptyString(domElement, AttrNotes);
}

//---------------------------------------------------------------------------------------------------------------------
auto VPattern::ActiveDrawBoundingRect() const -> QRectF
{
    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 69, "Not all tools were used.");

    QRectF rec;

    const int index = PatternBlockMapper()->GetActiveId();

    for (const auto &tool : history)
    {
        if (tool.GetPatternBlockIndex() != index)
        {
            continue;
        }

        switch (tool.GetToolType())
        {
            case Tool::Arrow:
            case Tool::SinglePoint:
            case Tool::DoublePoint:
            case Tool::LinePoint:
            case Tool::AbstractSpline:
            case Tool::Cut:
            case Tool::Midpoint:                // Same as Tool::AlongLine, but tool will never has such a type
            case Tool::ArcIntersectAxis:        // Same as Tool::CurveIntersectAxis, but tool will never has such a type
            case Tool::BackgroundImage:         // Not part of active draw
            case Tool::BackgroundImageControls: // Not part of active draw
            case Tool::BackgroundPixmapImage:   // Not part of active draw
            case Tool::BackgroundSVGImage:      // Not part of active draw
            case Tool::ArcStart:                // Same as Tool::CutArc, but tool will never has such a type
            case Tool::ArcEnd:                  // Same as Tool::CutArc, but tool will never has such a type
            case Tool::LAST_ONE_DO_NOT_USE:
                Q_UNREACHABLE();
                break;
            case Tool::BasePoint:
            case Tool::LineIntersect:
            case Tool::PointOfContact:
            case Tool::Triangle:
            case Tool::PointOfIntersection:
            case Tool::CutArc:
            case Tool::CutSpline:
            case Tool::CutSplinePath:
            case Tool::PointOfIntersectionArcs:
            case Tool::PointOfIntersectionCircles:
            case Tool::PointOfIntersectionCurves:
            case Tool::PointFromCircleAndTangent:
            case Tool::PointFromArcAndTangent:
            case Tool::ArcPoint:
            case Tool::CurvePoint:
                rec = ToolBoundingRect<VToolSinglePoint>(rec, tool.GetId());
                break;
            case Tool::EndLine:
            case Tool::AlongLine:
            case Tool::ShoulderPoint:
            case Tool::Normal:
            case Tool::Bisector:
            case Tool::Height:
            case Tool::LineIntersectAxis:
            case Tool::CurveIntersectAxis:
                rec = ToolBoundingRect<VToolLinePoint>(rec, tool.GetId());
                break;
            case Tool::Line:
                rec = ToolBoundingRect<VToolLine>(rec, tool.GetId());
                break;
            case Tool::Spline:
            case Tool::CubicBezier:
            case Tool::Arc:
            case Tool::SplinePath:
            case Tool::CubicBezierPath:
            case Tool::ArcWithLength:
            case Tool::EllipticalArc:
            case Tool::EllipticalArcWithLength:
            case Tool::ParallelCurve:
            case Tool::GraduatedCurve:
            case Tool::ArmholeCurve:
                rec = ToolBoundingRect<VToolAbstractCurve>(rec, tool.GetId());
                break;
            case Tool::TrueDarts:
                rec = ToolBoundingRect<VToolDoublePoint>(rec, tool.GetId());
                break;
            case Tool::Rotation:
            case Tool::FlippingByLine:
            case Tool::FlippingByAxis:
            case Tool::Move:
                rec = ToolBoundingRect<VAbstractOperation>(rec, tool.GetId());
                break;
            // These tools are not accesseble in Draw mode, but still 'history' contains them.
            case Tool::Piece:
            case Tool::UnionDetails:
            case Tool::NodeArc:
            case Tool::NodeElArc:
            case Tool::NodePoint:
            case Tool::NodeSpline:
            case Tool::NodeSplinePath:
            case Tool::Group:
            case Tool::Attach:
            case Tool::PiecePath:
            case Tool::InsertNode:
            case Tool::PlaceLabel:
            case Tool::DuplicateDetail:
            default:
                break;
        }
    }
    return rec;
}

//---------------------------------------------------------------------------------------------------------------------
template <typename T> auto VPattern::ToolBoundingRect(const QRectF &rec, quint32 id) const -> QRectF
{
    VDataTool *dataTool = tools.value(id, nullptr);
    if (dataTool == nullptr)
    {
        qDebug() << "Can't find tool or null pointer for id=" << id;
        return rec;
    }

    const T *vTool = qobject_cast<T *>(dataTool);
    if (vTool == nullptr)
    {
        qDebug() << "qobject_cast<" << T::staticMetaObject.className() << "> failed for id=" << id
                 << "actual type:" << dataTool->metaObject()->className();
        return rec;
    }

    // sceneBoundingRect() and scenePos() are undefined if the item has no scene.
    if (vTool->scene() == nullptr)
    {
        qDebug() << "Tool id=" << id << "(" << dataTool->metaObject()->className()
                 << ") has no scene, skipping bounding rect";
        return rec;
    }

    QRectF recTool = rec;

    if (const QRectF sceneBounds = vTool->sceneBoundingRect(); sceneBounds.isValid())
    {
        recTool = recTool.united(sceneBounds);
    }

    if (const QRectF childrenRect = vTool->childrenBoundingRect(); childrenRect.isValid())
    {
        // childrenBoundingRect() is in item-local coords — translate to scene space.
        QRectF childrenSceneRect = childrenRect;
        childrenSceneRect.translate(vTool->scenePos());
        recTool = recTool.united(childrenSceneRect);
    }

    return recTool;
}
