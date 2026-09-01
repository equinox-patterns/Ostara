/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   6 11, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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
// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#include "vtoolseamallowance.h"
#include "oannotationitem.h"
#include "oinfoblockitem.h"
#include "vtoolseamallowance_autoplace.h"
#include "../dialogs/tools/piece/dialogduplicatedetail.h"
#include "../dialogs/tools/piece/dialogseamallowance.h"
#include "../ifc/exception/vexceptionwrongid.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../qmuparser/qmutokenparser.h"
#include "../undocommands/addpiece.h"
#include "../undocommands/deletepiece.h"
#include "../undocommands/movepiece.h"
#include "../undocommands/savepieceoptions.h"
#include "../undocommands/savepiecepathoptions.h"
#include "../undocommands/togglepiecestate.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vplacelabelitem.h"
#include "../vgeometry/vpointf.h"
#include "../vlayout/vboundary.h"
#include "../vlayout/vfoldline.h"
#include "../vlayout/vlayoutpiece.h"
#include "../vlayout/vlayoutpiecepath.h"
#include "../vmisc/def.h"
#include "themeDef.h"
#include "vscenestylesheet.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vtheme/vtheme.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/oinfoblockdata.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vpatterndb/vformula.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "../vwidgets/vnobrushscalepathitem.h"
#include "../vwidgets/vpiecegrainline.h"
#include "nodeDetails/vnodearc.h"
#include "nodeDetails/vnodeellipticalarc.h"
#include "nodeDetails/vnodepoint.h"
#include "nodeDetails/vnodespline.h"
#include "nodeDetails/vnodesplinepath.h"
#include "nodeDetails/vtoolpiecepath.h"
#include "nodeDetails/vtoolpin.h"
#include "nodeDetails/vtoolplacelabel.h"
#include "qpainterpath.h"
#include "toolsdef.h"
#include "vinteractivetool.h"

#include <QFuture>
#include <QFutureWatcher>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPainterPath>
#include <QTimer>
#include <QUuid>
#include <QtConcurrent/QtConcurrentRun>
#include <QtMath>
#include <chrono>
#include <memory>

using namespace std::chrono_literals;

// Current version of seam allowance tag need for backward compatibility
const quint8 VToolSeamAllowance::pieceVersion = 2;

OInfoBlockContentBuilder VToolSeamAllowance::s_infoBlockContentBuilder; // NOLINT(cert-err58-cpp)
OAnnotationStyleResolver VToolSeamAllowance::s_annotationStyleResolver; // NOLINT(cert-err58-cpp)

const QString VToolSeamAllowance::TagCSA = QStringLiteral("csa");                 // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagRecord = QStringLiteral("record");           // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagIPaths = QStringLiteral("iPaths");           // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagPins = QStringLiteral("pins");               // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagPlaceLabels = QStringLiteral("placeLabels"); // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagSaMarginProperties = QStringLiteral("saMarginProperties"); // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagCartoucheBlocks = QStringLiteral("cartoucheBlocks");       // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagDispatch = QStringLiteral("dispatch");                     // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagBlock = QStringLiteral("block");                           // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagAnnotations = QStringLiteral("annotations");               // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagAnnotation = QStringLiteral("annotation");                 // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagText = QStringLiteral("text");                             // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::TagGeom = QStringLiteral("geom");                             // NOLINT(cert-err58-cpp)

const QString VToolSeamAllowance::AttrSeamAllowance = QStringLiteral("seamAllowance"); // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrHideMainPath = QStringLiteral("hideMainPath");   // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrShowFullPiece = QStringLiteral("showFullPiece"); // NOLINT(cert-err58-cpp)
// NOLINTNEXTLINE(cert-err58-cpp)
const QString VToolSeamAllowance::AttrSeamAllowanceBuiltIn = QStringLiteral("seamAllowanceBuiltIn");
const QString VToolSeamAllowance::AttrUnited = QStringLiteral("united");                 // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrFont = QStringLiteral("fontSize");                 // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrActive = QStringLiteral("active");                 // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrBlock = QStringLiteral("block");                   // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrIndex = QStringLiteral("index");                   // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrFontScale = QStringLiteral("fontScale");           // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrTopLeftPin = QStringLiteral("topLeftPin");         // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrBottomRightPin = QStringLiteral("bottomRightPin"); // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrCenterPin = QStringLiteral("centerPin");           // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrTopPin = QStringLiteral("topPin");                 // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrBottomPin = QStringLiteral("bottomPin");           // NOLINT(cert-err58-cpp)
const QString VToolSeamAllowance::AttrPiecePriority = QStringLiteral("priority");        // NOLINT(cert-err58-cpp)

namespace
{
//---------------------------------------------------------------------------------------------------------------------
template <typename T> auto FixGrainlinePins(T itemData, const QMap<quint32, quint32> &mappedPins) -> T
{
    itemData.SetCenterPin(mappedPins.value(itemData.CenterPin(), NULL_ID));
    itemData.SetTopPin(mappedPins.value(itemData.TopPin(), NULL_ID));
    itemData.SetBottomPin(mappedPins.value(itemData.BottomPin(), NULL_ID));
    return itemData;
}

//---------------------------------------------------------------------------------------------------------------------
auto DuplicatePins(const QVector<quint32> &pins, const VToolSeamAllowanceInitData &initData) -> QMap<quint32, quint32>
{
    QMap<quint32, quint32> newPins;
    for (auto p : pins)
    {
        QSharedPointer<VPointF> const pin = initData.data->GeometricObject<VPointF>(p);

        auto *tool = qobject_cast<VAbstractNode *>(VAbstractPattern::getTool(p));
        SCASSERT(tool != nullptr)

        VToolPinInitData initNodeData;
        initNodeData.id = initData.data->AddGObject(new VPointF(*pin));
        initNodeData.pointId = pin->getIdObject();
        initNodeData.idObject = NULL_ID; // piece id
        initNodeData.doc = initData.doc;
        initNodeData.data = initData.data;
        initNodeData.parse = Document::FullParse;
        initNodeData.typeCreation = Source::FromTool;
        initNodeData.drawName = initData.drawName;
        initNodeData.idTool = tool->GetIdTool();

        VToolPin::Create(initNodeData);
        newPins.insert(p, initNodeData.id);
    }
    return newPins;
}

//---------------------------------------------------------------------------------------------------------------------
auto SeamPoints(const VPiece &detail, bool combineTogether, const VContainer *data) -> QVector<VLayoutPoint>
{
    if (combineTogether)
    {
        const QVector<VLayoutPassmark> passmarks = VLayoutPiece::ConvertPassmarks(detail, data);
        const QVector<VLayoutPoint> points = detail.FullMainPathPoints(data);

        bool const seamAllowance = detail.IsSeamAllowance() && detail.IsSeamAllowanceBuiltIn();
        bool const builtInSeamAllowance = detail.IsSeamAllowance() && detail.IsSeamAllowanceBuiltIn();

        // DumpVector(points, QStringLiteral("points.json.XXXXXX")); // Uncomment for dumping test data
        // DumpVector(passmarks, QStringLiteral("passmarks.json.XXXXXX")); // Uncomment for dumping test data

        VBoundary boundary(points, seamAllowance, builtInSeamAllowance);
        boundary.SetPieceName(detail.GetName());

        if (QLineF const mirrorLine = detail.SeamMirrorLine(data); !mirrorLine.isNull() && detail.IsShowFullPiece())
        {
            boundary.SetMirrorLine(mirrorLine);
        }
        const QList<VBoundarySequenceItemData> sequence = boundary.Combine(passmarks, false, false);

        QVector<VLayoutPoint> combinedBoundary;
        for (const auto &item : sequence)
        {
            combinedBoundary += item.item.value<VLayoutPiecePath>().Points();
        }

        // DumpVector(combinedBoundary, QStringLiteral("boundary.json.XXXXXX")); // Uncomment for dumping test data
        return combinedBoundary;
    }

    return detail.FullMainPathPoints(data);
}

//---------------------------------------------------------------------------------------------------------------------
auto RenderSeamPath(const VPiece &detail, bool combineTogether, const VContainer *data) -> QPainterPath
{
    QVector<VLayoutPoint> const points = SeamPoints(detail, combineTogether, data);
    QVector<QPointF> boundary;
    CastTo(points, boundary);

    if (combineTogether)
    {
        QPainterPath combinedPath;
        combinedPath.addPolygon(QPolygonF(boundary));
        combinedPath.closeSubpath();
        combinedPath.setFillRule(Qt::OddEvenFill);

        return combinedPath;
    }

    return VPiece::MainPathPath(boundary);
}

//---------------------------------------------------------------------------------------------------------------------
auto SeamAllowancePoints(const VPiece &detail, bool combineTogether, const VContainer *data) -> QVector<VLayoutPoint>
{
    if (combineTogether)
    {
        const QVector<VLayoutPassmark> passmarks = VLayoutPiece::ConvertPassmarks(detail, data);
        const QVector<VLayoutPoint> points = detail.FullSeamAllowancePoints(data);

        // DumpVector(points, QStringLiteral("points.json.XXXXXX"));       // Uncomment for dumping test data
        // DumpVector(passmarks, QStringLiteral("passmarks.json.XXXXXX")); // Uncomment for dumping test data

        VBoundary boundary(points, true);
        boundary.SetPieceName(detail.GetName());

        if (QLineF const mirrorLine = detail.SeamAllowanceMirrorLine(data);
            !mirrorLine.isNull() && detail.IsShowFullPiece())
        {
            boundary.SetMirrorLine(mirrorLine);
        }

        const QList<VBoundarySequenceItemData> sequence = boundary.Combine(passmarks, false, false);

        QVector<VLayoutPoint> combinedBoundary;
        for (const auto &item : sequence)
        {
            combinedBoundary += item.item.value<VLayoutPiecePath>().Points();
        }

        // DumpVector(combinedBoundary, QStringLiteral("boundary.json.XXXXXX")); // Uncomment for dumping test data
        return combinedBoundary;
    }

    return detail.FullSeamAllowancePoints(data);
}

//---------------------------------------------------------------------------------------------------------------------
auto RenderSeamAllowancePath(const VPiece &detail, bool combineTogether, const VContainer *data) -> QPainterPath
{
    QVector<VLayoutPoint> const points = SeamAllowancePoints(detail, combineTogether, data);
    if (combineTogether)
    {
        QVector<QPointF> combinedBoundary;
        CastTo(points, combinedBoundary);

        QPainterPath combinedPath;
        combinedPath.addPolygon(QPolygonF(combinedBoundary));
        combinedPath.closeSubpath();
        combinedPath.setFillRule(Qt::OddEvenFill);

        return combinedPath;
    }

    return detail.SeamAllowancePath(points);
}

//---------------------------------------------------------------------------------------------------------------------
auto RenderPassmarks(const VPiece &detail, const VContainer *data) -> QPainterPath
{
    if (const QLineF mirrorLine = detail.SeamMirrorLine(data); !mirrorLine.isNull() && detail.IsShowFullPiece())
    {
        if (!detail.IsSeamAllowance())
        {
            return {};
        }

        QPainterPath path;
        const QTransform matrix = VGObject::FlippingMatrix(mirrorLine);
        const QVector<VLayoutPassmark> passmarks = VLayoutPiece::ConvertPassmarks(detail, data);
        for (const auto &passmark : passmarks)
        {
            QPainterPath passmaksPath;
            for (const auto &line : passmark.lines)
            {
                passmaksPath.moveTo(line.p1());
                passmaksPath.lineTo(line.p2());
            }
            path.addPath(passmaksPath);

            if (!IsPointOnLineviaPDP(passmark.baseLine.p1(), mirrorLine.p1(), mirrorLine.p2(), accuracyPointOnLine * 2)
                && !passmark.notMirrored)
            {
                QPainterPath mirroredPassmaksPath;
                for (const auto &line : passmark.lines)
                {
                    mirroredPassmaksPath.moveTo(line.p1());
                    mirroredPassmaksPath.lineTo(line.p2());
                }
                path.addPath(matrix.map(mirroredPassmaksPath));
            }
        }

        return path;
    }
    return detail.PassmarksPath(data);
}

//---------------------------------------------------------------------------------------------------------------------
auto RenderFoldLine(const VPiece &detail, const VContainer *data) -> VFoldLine
{
    QLineF const foldLine = detail.IsHideMainPath() ? detail.SeamAllowanceMirrorLine(data)
                                                    : detail.SeamMirrorLine(data);
    VFoldLine fLine(foldLine, detail.GetFoldLineType());
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    fLine.SetLabelSvgFontSize(static_cast<unsigned int>(settings->GetFoldLineLabelFontPixelSize()));
    fLine.SetLabel(detail.GetFoldLineLabel());

    {
        QFont font(settings->GetFoldLineLabelFontFamily());
        font.setWeight(settings->GetFoldLineLabelFontWeight());
        font.setPixelSize(settings->GetFoldLineLabelFontPixelSize());
        fLine.SetOutlineFont(font);
        fLine.SetSvgFont(settings->GetLabelSVGFont());
    }

    if (detail.IsManualFoldHeight())
    {
        VFormula formula(detail.GetFormulaFoldHeight(), data);
        formula.setCheckZero(false);
        formula.setCheckLessThanZero(true);
        formula.Eval();

        if (formula.error())
        {
            const QString errorMsg = QObject::tr("Cannot calculate fold line height for piece '%1'. Reason: %2.")
                                         .arg(detail.GetName(), formula.Reason());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
        else
        {
            fLine.SetHeight(ToPixel(formula.getDoubleValue(), *data->GetPatternUnit()));
        }
    }

    if (detail.IsManualFoldWidth())
    {
        VFormula formula(detail.GetFormulaFoldWidth(), data);
        formula.setCheckZero(false);
        formula.setCheckLessThanZero(true);
        formula.Eval();

        if (formula.error())
        {
            const QString errorMsg = QObject::tr("Cannot calculate fold line width for piece '%1'. Reason: %2.")
                                         .arg(detail.GetName(), formula.Reason());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
        else
        {
            fLine.SetWidth(ToPixel(formula.getDoubleValue(), *data->GetPatternUnit()));
        }
    }

    if (detail.IsManualFoldCenter())
    {
        VFormula formula(detail.GetFormulaFoldCenter(), data);
        formula.setCheckZero(false);
        formula.setCheckLessThanZero(true);
        formula.Eval();

        if (formula.error())
        {
            const QString errorMsg =
                QObject::tr("Cannot calculate fold line center position for piece '%1'. Reason: %2.")
                    .arg(detail.GetName(), formula.Reason());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
        else
        {
            fLine.SetCenterPosition(formula.getDoubleValue());
        }
    }

    return fLine;
}

//---------------------------------------------------------------------------------------------------------------------
auto RenderMirrorLine(const VPiece &detail, const VContainer *data, bool combineTogether) -> QPainterPath
{
    if (!detail.IsShowFullPiece() || !detail.IsShowMirrorLine())
    {
        return {};
    }

    // Determine the appropriate seam line and points based on the seam allowance state
    QLineF mirrorLine;
    QVector<VLayoutPoint> points;

    if (!detail.IsSeamAllowance() || detail.IsSeamAllowanceBuiltIn())
    {
        mirrorLine = detail.SeamMirrorLine(data);
        // Return empty path if no valid mirror line is found
        if (mirrorLine.isNull())
        {
            return {};
        }

        if (combineTogether)
        {
            points = SeamPoints(detail, combineTogether, data);
        }
    }
    else
    {
        mirrorLine = detail.SeamAllowanceMirrorLine(data);
        // Return empty path if no valid mirror line is found
        if (mirrorLine.isNull())
        {
            return {};
        }

        if (combineTogether)
        {
            points = SeamAllowancePoints(detail, combineTogether, data);
        }
    }

    QPointF p1 = mirrorLine.p1();
    QPointF p2 = mirrorLine.p2();

    // Adjust points based on intersections, if required
    if (combineTogether && !points.isEmpty())
    {
        QVector<QPointF> seam;
        CastTo(points, seam);

        if (QVector<QPointF> const intersectionPoints = VAbstractCurve::CurveIntersectLine(seam,
                                                                                           QLineF(mirrorLine.center(),
                                                                                                  p1));
            !intersectionPoints.isEmpty())
        {
            p1 = intersectionPoints.constFirst();
        }

        if (QVector<QPointF> const intersectionPoints = VAbstractCurve::CurveIntersectLine(seam,
                                                                                           QLineF(mirrorLine.center(),
                                                                                                  p2));
            !intersectionPoints.isEmpty())
        {
            p2 = intersectionPoints.constFirst();
        }
    }

    // Construct and return the path
    QPainterPath path;
    path.moveTo(p1);
    path.lineTo(p2);
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
void UpdatePathExcludeState(const VPiecePath &path)
{
    for (int i = 0; i < path.CountNodes(); ++i)
    {
        if (const VPieceNode &node = path.at(i); node.GetTypeTool() == Tool::NodePoint)
        {
            auto *tool = qobject_cast<VNodePoint *>(VAbstractPattern::getTool(node.GetId()));
            SCASSERT(tool != nullptr);

            tool->SetExluded(node.IsExcluded());
            tool->setVisible(not node.IsExcluded()); // Hide excluded point
        }
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
VToolSeamAllowance::~VToolSeamAllowance() = default;

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                                VContainer *data) -> VToolSeamAllowance *
{
    SCASSERT(not dialog.isNull());
    const QPointer<DialogSeamAllowance> dialogTool = qobject_cast<DialogSeamAllowance *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolSeamAllowanceInitData initData;
    initData.detail = dialogTool->GetPiece();
    initData.width = initData.detail.GetFormulaSAWidth();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;

    initData.detail.GetPath().SetNodes(PrepareNodes(initData.detail.GetPath(), scene, doc, data));

    VToolSeamAllowance *piece = Create(initData);

    if (piece != nullptr)
    {
        piece->m_dialog = dialog;
        piece->RefreshGeometry(true); // Refresh internal paths
    }
    return piece;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::Create(VToolSeamAllowanceInitData &initData) -> VToolSeamAllowance *
{
    if (initData.typeCreation == Source::FromGui || initData.typeCreation == Source::FromTool)
    {
        auto *currentSA = new VIncrement(initData.data, currentSeamAllowance);
        currentSA->SetFormula(initData.detail.GetSAWidth(), initData.width, true);
        currentSA->SetDescription(tr("Current seam allowance"));

        initData.data->AddVariable(currentSA);
        initData.id = initData.data->AddPiece(initData.detail);
    }
    else
    {
        const qreal calcWidth = CheckFormula(initData.id, initData.width, initData.data);
        initData.detail.SetFormulaSAWidth(initData.width, calcWidth);

        auto *currentSA = new VIncrement(initData.data, currentSeamAllowance);
        currentSA->SetFormula(calcWidth, initData.width, true);
        currentSA->SetDescription(tr("Current seam allowance"));

        initData.data->AddVariable(currentSA);

        initData.data->UpdatePiece(initData.id, initData.detail);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::PIECE, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();
    AddPieceDependencies(initData.id, initData.detail, initData.doc, varData);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    VToolSeamAllowance *piece = nullptr;
    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::Piece, initData.doc);
        piece = new VToolSeamAllowance(initData);
        initData.scene->addItem(piece);
        VMainGraphicsView::NewSceneRect(initData.scene, VAbstractValApplication::VApp()->getSceneView(), piece);
        VAbstractPattern::AddTool(initData.id, piece);
    }
    // Very important to delete it. Only this tool need this special variable.
    initData.data->RemoveVariable(currentSeamAllowance);
    return piece;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::Duplicate(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc)
    -> VToolSeamAllowance *
{
    SCASSERT(not dialog.isNull());
    const QPointer<DialogDuplicateDetail> dialogTool = qobject_cast<DialogDuplicateDetail *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolSeamAllowanceInitData initData;
    initData.scene = scene;
    initData.doc = doc;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.drawName = doc->PieceDrawName(dialogTool->Duplicate());

    VContainer toolData = VAbstractPattern::getTool(dialogTool->Duplicate())->GetDataCopy();
    initData.data = &toolData;

    VPiece detail = initData.data->GetPiece(dialogTool->Duplicate());
    detail.SetMx(dialogTool->MoveDuplicateX());
    detail.SetMy(dialogTool->MoveDuplicateY());
    initData.detail = detail;
    initData.width = initData.detail.GetFormulaSAWidth();

    VToolSeamAllowance *piece = Duplicate(initData);
    if (piece != nullptr)
    {
        piece->RefreshGeometry(true); // Refresh internal paths
    }
    return piece;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::Duplicate(VToolSeamAllowanceInitData &initData) -> VToolSeamAllowance *
{
    VPiece dupDetail = initData.detail;

    QMap<quint32, quint32> replacements;
    dupDetail.GetPath().SetNodes(DuplicateNodes(initData.detail.GetPath(), initData, replacements));
    dupDetail.SetCustomSARecords(
        DuplicateCustomSARecords(initData.detail.GetCustomSARecords(), initData, replacements));
    dupDetail.SetInternalPaths(DuplicateInternalPaths(initData.detail.GetInternalPaths(), initData));
    dupDetail.SetPlaceLabels(DuplicatePlaceLabels(initData.detail.GetPlaceLabels(), initData));
    dupDetail.SetUUID(QUuid::createUuid());

    const QMap<quint32, quint32> mappedPins = DuplicatePins(initData.detail.GetPins(), initData);
    dupDetail.SetPins(ConvertToVector(mappedPins.values()));
    dupDetail.SetGrainlineGeometry(FixGrainlinePins(initData.detail.GetGrainlineGeometry(), mappedPins));

    if (replacements.contains(dupDetail.GetMirrorLineStartPoint()))
    {
        dupDetail.SetMirrorLineStartPoint(replacements.value(dupDetail.GetMirrorLineStartPoint()));
    }

    if (replacements.contains(dupDetail.GetMirrorLineEndPoint()))
    {
        dupDetail.SetMirrorLineEndPoint(replacements.value(dupDetail.GetMirrorLineEndPoint()));
    }

    initData.detail = dupDetail;
    return VToolSeamAllowance::Create(initData);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddPieceDependencies(quint32 id,
                                              const VPiece &piece,
                                              VAbstractPattern *doc,
                                              const QHash<QString, QList<quint32>> &variables)
{
    SCASSERT(doc != nullptr)

    VPatternGraph *patternGraph = doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    doc->FindFormulaDependencies(piece.GetFormulaSAWidth(), id, variables);

    if (piece.IsManualFoldHeight())
    {
        doc->FindFormulaDependencies(piece.GetFormulaFoldHeight(), id, variables);
    }

    if (piece.IsManualFoldWidth())
    {
        doc->FindFormulaDependencies(piece.GetFormulaFoldWidth(), id, variables);
    }

    if (piece.IsManualFoldCenter())
    {
        doc->FindFormulaDependencies(piece.GetFormulaFoldCenter(), id, variables);
    }

    const VGrainlineData &grainlineData = piece.GetGrainlineGeometry();
    doc->FindFormulaDependencies(grainlineData.GetLength(), id, variables);
    doc->FindFormulaDependencies(grainlineData.GetRotation(), id, variables);

    const VPiecePath &path = piece.GetPath();
    for (int i = 0; i < path.CountNodes(); ++i)
    {
        const VPieceNode &node = path.at(i);

        if (node.GetFormulaSABefore() != currentSeamAllowance)
        {
            doc->FindFormulaDependencies(node.GetFormulaSABefore(), id, variables);
        }

        if (node.GetFormulaSAAfter() != currentSeamAllowance)
        {
            doc->FindFormulaDependencies(node.GetFormulaSAAfter(), id, variables);
        }

        doc->FindFormulaDependencies(node.GetFormulaPassmarkLength(), id, variables);
        doc->FindFormulaDependencies(node.GetFormulaPassmarkWidth(), id, variables);
        doc->FindFormulaDependencies(node.GetFormulaPassmarkAngle(), id, variables);

        if (node.GetFormulaPassmarkVisibility() != '1')
        {
            doc->FindFormulaDependencies(node.GetFormulaPassmarkVisibility(), id, variables);
        }

        patternGraph->AddEdge(node.GetId(), id);
    }

    const QVector<CustomSARecord> records = piece.GetCustomSARecords();
    for (const auto &record : records)
    {
        patternGraph->AddEdge(record.path, id);
    }

    const QVector<quint32> paths = piece.GetInternalPaths();
    for (auto iPath : paths)
    {
        patternGraph->AddEdge(iPath, id);
    }

    for (auto point : piece.GetPins())
    {
        patternGraph->AddEdge(point, id);
    }

    for (auto point : piece.GetPlaceLabels())
    {
        patternGraph->AddEdge(point, id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::RemoveWithConfirm(bool ask)
{
    try
    {
        DeleteToolWithConfirm(ask);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e);
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::InsertNodes(const QVector<VPieceNode> &nodes, quint32 pieceId, VMainGraphicsScene *scene,
                                     VContainer *data, VAbstractPattern *doc) -> bool
{
    SCASSERT(scene != nullptr)
    SCASSERT(data != nullptr)
    SCASSERT(doc != nullptr)

    if (pieceId == NULL_ID || nodes.isEmpty())
    {
        return false;
    }

    VPiece oldDet;
    try
    {
        oldDet = data->GetPiece(pieceId);
    }
    catch (const VExceptionBadId &)
    {
        return false;
    }

    VPiece newDet = oldDet;
    QVector<VPieceNode> path = newDet.GetPath().GetNodes();

    // Seam allowance tool already initializated and can't init the node
    auto *saTool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
    SCASSERT(saTool != nullptr);

    bool placedAny = false;
    for (auto node : nodes)
    {
        // Auto-placement: find where this node goes in the current path.
        VNodePlacement placement;
        if (not ComputeNodePlacement(path, node, data, doc, placement))
        {
            continue; // not placeable: skip the node, never crash
        }
        placedAny = true;

        if (node.GetTypeTool() != Tool::NodePoint)
        {
            node.SetReverse(placement.reverse); // whole curve: contour-imposed sense
        }

        const quint32 id = PrepareNode(node, scene, doc, data);
        if (id == NULL_ID)
        {
            continue;
        }
        node.SetId(id);
        node.SetExcluded(false); // auto-included: the node is correctly placed

        qsizetype idx = placement.index;
        if (idx < 0)
        {
            idx = 0;
        }
        if (idx > path.size())
        {
            idx = path.size();
        }
        path.insert(idx, node);
        InitNode(node, scene, saTool);

        // A point that cuts a curve: also insert the curve portion after it.
        if (placement.hasCurvePortion)
        {
            VPieceNode curve = placement.curvePortion;
            const quint32 curveToolId = PrepareNode(curve, scene, doc, data);
            if (curveToolId != NULL_ID)
            {
                curve.SetId(curveToolId);
                curve.SetExcluded(false);
                path.insert(idx + 1, curve);
                InitNode(curve, scene, saTool);
            }
        }
    }

    newDet.GetPath().SetNodes(path);
    VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, doc, pieceId));
    return placedAny;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddAttributes(VAbstractPattern *doc, QDomElement &domElement, quint32 id, const VPiece &piece)
{
    SCASSERT(doc != nullptr);

    doc->SetAttribute(domElement, VDomDocument::AttrId, id);
    doc->SetAttribute(domElement, AttrName, piece.GetName());
    doc->SetAttributeOrRemoveIf<QString>(domElement, AttrShortName, piece.GetShortName(),
                                         [](const QString &name) noexcept { return name.isEmpty(); });
    doc->SetAttribute(domElement, AttrUUID, piece.GetUUID().toString());
    doc->SetAttribute(domElement, AttrVersion, QString().setNum(pieceVersion));
    doc->SetAttribute(domElement, AttrMx, VAbstractValApplication::VApp()->fromPixel(piece.GetMx()));
    doc->SetAttribute(domElement, AttrMy, VAbstractValApplication::VApp()->fromPixel(piece.GetMy()));
    doc->SetAttributeOrRemoveIf<bool>(domElement, AttrInLayout, piece.IsInLayout(),
                                      [](bool inLayout) noexcept { return inLayout; });
    doc->SetAttribute(domElement, AttrForbidFlipping, piece.IsForbidFlipping());
    doc->SetAttribute(domElement, AttrForceFlipping, piece.IsForceFlipping());
    doc->SetAttributeOrRemoveIf<bool>(domElement,
                                      AttrSymmetricalCopy,
                                      piece.IsSymmetricalCopy(),
                                      [](bool sCopy) noexcept { return not sCopy; });
    doc->SetAttribute(domElement, AttrFollowGrainline, piece.IsFollowGrainline());
    doc->SetAttribute(domElement, AttrSewLineOnDrawing, piece.IsSewLineOnDrawing());
    doc->SetAttributeOrRemoveIf<bool>(domElement, AttrSeamAllowance, piece.IsSeamAllowance(),
                                      [](bool seamAllowance) noexcept { return not seamAllowance; });
    doc->SetAttribute(domElement, AttrHideMainPath, piece.IsHideMainPath());
    doc->SetAttributeOrRemoveIf<bool>(domElement, AttrSeamAllowanceBuiltIn, piece.IsSeamAllowanceBuiltIn(),
                                      [](bool builtin) noexcept { return not builtin; });
    doc->SetAttributeOrRemoveIf<bool>(domElement, AttrShowFullPiece, piece.IsShowFullPiece(),
                                      [](bool show) noexcept { return show; });
    doc->SetAttributeOrRemoveIf<bool>(domElement,
                                      AttrTrueZeroWidth,
                                      piece.IsTrueZeroWidth(),
                                      [](bool value) noexcept { return !value; });
    doc->SetAttribute(domElement, AttrWidth, piece.GetFormulaSAWidth());
    doc->SetAttributeOrRemoveIf<bool>(domElement, AttrUnited, piece.IsUnited(),
                                      [](bool united) noexcept { return not united; });
    doc->SetAttributeOrRemoveIf<uint>(domElement, AttrPiecePriority, piece.GetPriority(),
                                      [](uint priority) noexcept { return priority == 0; });
    doc->SetAttributeOrRemoveIf<int>(domElement, AttrGradedAlignH, piece.GetGradedAlignH(),
                                     [](int val) noexcept { return val == 2; });
    doc->SetAttributeOrRemoveIf<int>(domElement, AttrGradedAlignV, piece.GetGradedAlignV(),
                                     [](int val) noexcept { return val == 2; });
    doc->SetAttributeOrRemoveIf<uint>(domElement, AttrPieceNumber, piece.GetPieceNumber(),
                                      [](uint number) noexcept { return number == 0; });
    doc->SetAttributeOrRemoveIf<int>(domElement, AttrCutFabricQty, piece.GetCutFabricQty(),
                                     [](int qty) noexcept { return qty == 0; });
    doc->SetAttributeOrRemoveIf<bool>(domElement, AttrCutFabricOnFold, piece.IsCutFabricOnFold(),
                                      [](bool on) noexcept { return not on; });
    doc->SetAttributeOrRemoveIf<int>(domElement, AttrCutInterfacingQty, piece.GetCutInterfacingQty(),
                                     [](int qty) noexcept { return qty == 0; });
    doc->SetAttributeOrRemoveIf<bool>(domElement, AttrCutInterfacingOnFold, piece.IsCutInterfacingOnFold(),
                                      [](bool on) noexcept { return not on; });
    doc->SetAttributeOrRemoveIf<int>(domElement, AttrCutLiningQty, piece.GetCutLiningQty(),
                                     [](int qty) noexcept { return qty == 0; });
    doc->SetAttributeOrRemoveIf<bool>(domElement, AttrCutLiningOnFold, piece.IsCutLiningOnFold(),
                                      [](bool on) noexcept { return not on; });
    doc->SetAttributeOrRemoveIf<QString>(domElement, AttrFreeText, piece.GetFreeText(),
                                         [](const QString &text) noexcept { return text.isEmpty(); });
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddCSARecord(VAbstractPattern *doc, QDomElement &domElement, CustomSARecord record)
{
    QDomElement recordNode = doc->createElement(VToolSeamAllowance::TagRecord);

    doc->SetAttribute(recordNode, VAbstractPattern::AttrStart, record.startPoint);
    doc->SetAttribute(recordNode, VAbstractPattern::AttrPath, record.path);
    doc->SetAttribute(recordNode, VAbstractPattern::AttrEnd, record.endPoint);
    doc->SetAttribute(recordNode, VAbstractPattern::AttrNodeReverse, record.reverse);
    doc->SetAttribute(recordNode, VAbstractPattern::AttrIncludeAs, static_cast<unsigned int>(record.includeType));

    domElement.appendChild(recordNode);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddCSARecords(VAbstractPattern *doc, QDomElement &domElement,
                                       const QVector<CustomSARecord> &records)
{
    if (not records.empty())
    {
        QDomElement csaRecordsElement = doc->createElement(VToolSeamAllowance::TagCSA);
        for (const auto &record : records)
        {
            AddCSARecord(doc, csaRecordsElement, record);
        }
        domElement.appendChild(csaRecordsElement);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddInternalPaths(VAbstractPattern *doc, QDomElement &domElement, const QVector<quint32> &paths)
{
    if (not paths.empty())
    {
        QDomElement iPathsElement = doc->createElement(VToolSeamAllowance::TagIPaths);
        for (auto path : paths)
        {
            QDomElement recordNode = doc->createElement(VToolSeamAllowance::TagRecord);
            doc->SetAttribute(recordNode, VAbstractPattern::AttrPath, path);
            iPathsElement.appendChild(recordNode);
        }
        domElement.appendChild(iPathsElement);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddPins(VAbstractPattern *doc, QDomElement &domElement, const QVector<quint32> &pins)
{
    AddPointRecords(doc, domElement, pins, VToolSeamAllowance::TagPins);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddPlaceLabels(VAbstractPattern *doc, QDomElement &domElement,
                                        const QVector<quint32> &placeLabels)
{
    AddPointRecords(doc, domElement, placeLabels, VToolSeamAllowance::TagPlaceLabels);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddGrainline(VAbstractPattern *doc, QDomElement &domElement, const VPiece &piece)
{
    // grainline
    QDomElement domData = doc->createElement(VAbstractPattern::TagGrainline);
    const VGrainlineData &glGeom = piece.GetGrainlineGeometry();
    doc->SetAttribute(domData, VAbstractPattern::AttrEnabled, glGeom.IsEnabled());
    doc->SetAttribute(domData, VAbstractPattern::AttrVisible, glGeom.IsVisible());
    doc->SetAttribute(domData, AttrMx, glGeom.GetPos().x());
    doc->SetAttribute(domData, AttrMy, glGeom.GetPos().y());
    doc->SetAttribute(domData, AttrLength, glGeom.GetLength());
    doc->SetAttribute(domData, VAbstractPattern::AttrRotation, glGeom.GetRotation());
    doc->SetAttribute(domData, VAbstractPattern::AttrArrows, static_cast<int>(glGeom.GetArrowType()));
    doc->SetAttributeOrRemoveIf<quint32>(domData,
                                         AttrCenterPin,
                                         glGeom.CenterPin(),
                                         [](quint32 pin) noexcept { return pin == NULL_ID; });
    doc->SetAttributeOrRemoveIf<quint32>(domData,
                                         AttrTopPin,
                                         glGeom.TopPin(),
                                         [](quint32 pin) noexcept { return pin == NULL_ID; });
    doc->SetAttributeOrRemoveIf<quint32>(domData,
                                         AttrBottomPin,
                                         glGeom.BottomPin(),
                                         [](quint32 pin) noexcept { return pin == NULL_ID; });

    domElement.appendChild(domData);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddMirrorLine(VAbstractPattern *doc, QDomElement &domElement, const VPiece &piece)
{
    QDomElement domData = doc->createElement(VAbstractPattern::TagMirrorLine);
    doc->SetAttributeOrRemoveIf<quint32>(domData, VAbstractPattern::AttrMirrorLineP1, piece.GetMirrorLineStartPoint(),
                                         [](quint32 id) noexcept { return id <= NULL_ID; });
    doc->SetAttributeOrRemoveIf<quint32>(domData, VAbstractPattern::AttrMirrorLineP2, piece.GetMirrorLineEndPoint(),
                                         [](quint32 id) noexcept { return id <= NULL_ID; });
    doc->SetAttributeOrRemoveIf<bool>(domData, VAbstractPattern::AttrFoldLineManualHeight, piece.IsManualFoldHeight(),
                                      [](bool manual) noexcept { return not manual; });
    doc->SetAttributeOrRemoveIf<bool>(domData, VAbstractPattern::AttrFoldLineManualWidth, piece.IsManualFoldWidth(),
                                      [](bool manual) noexcept { return not manual; });
    doc->SetAttributeOrRemoveIf<bool>(domData, VAbstractPattern::AttrFoldLineManualCenter, piece.IsManualFoldCenter(),
                                      [](bool manual) noexcept { return not manual; });
    doc->SetAttributeOrRemoveIf<QString>(
        domData, VAbstractPattern::AttrFoldLineHeightFormula, piece.GetFormulaFoldHeight(),
        [piece](const QString &height) noexcept { return not piece.IsManualFoldHeight() || height.isEmpty(); });
    doc->SetAttributeOrRemoveIf<QString>(
        domData, VAbstractPattern::AttrFoldLineWidthFormula, piece.GetFormulaFoldWidth(),
        [piece](const QString &width) noexcept { return not piece.IsManualFoldWidth() || width.isEmpty(); });
    doc->SetAttributeOrRemoveIf<QString>(
        domData, VAbstractPattern::AttrFoldLineCenterFormula, piece.GetFormulaFoldCenter(),
        [piece](const QString &center) noexcept { return not piece.IsManualFoldCenter() || center.isEmpty(); });
    doc->SetAttribute(domData, VAbstractPattern::AttrFoldLineType, FoldLineTypeToString(piece.GetFoldLineType()));
    doc->SetAttributeOrRemoveIf<QString>(domData, VAbstractPattern::AttrFoldLineLabel, piece.GetFoldLineLabel(),
                                         [](const QString &label) noexcept { return label.isEmpty(); });
    doc->SetAttributeOrRemoveIf<bool>(domData, VAbstractPattern::AttrMirrorLineVisible, piece.IsShowMirrorLine(),
                                      [](bool visible) noexcept { return visible; });

    domElement.appendChild(domData);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddCartoucheBlocks(VAbstractPattern *doc, QDomElement &domElement, const VPiece &piece)
{
    const OInfoBlockData &info = piece.GetInfoBlockData();
    if (not info.IsSet())
    {
        return;
    }

    QDomElement cartoucheEl = doc->createElement(VToolSeamAllowance::TagCartoucheBlocks);
    doc->SetAttributeOrRemoveIf<bool>(cartoucheEl, VToolSeamAllowance::AttrActive, info.IsActive(),
                                      [](bool active) noexcept { return not active; });
    doc->SetAttributeOrRemoveIf<QString>(cartoucheEl, VAbstractPattern::AttrTemplate, info.Template(),
                                         [](const QString &name) noexcept { return name.isEmpty(); });

    const QHash<QString, int> &dispatch = info.Dispatch();
    for (auto it = dispatch.constBegin(); it != dispatch.constEnd(); ++it)
    {
        QDomElement dispatchEl = doc->createElement(VToolSeamAllowance::TagDispatch);
        doc->SetAttribute(dispatchEl, AttrType, it.key());
        doc->SetAttribute(dispatchEl, VToolSeamAllowance::AttrBlock, it.value());
        cartoucheEl.appendChild(dispatchEl);
    }

    const QList<int> blockIndexes = info.StoredBlockIndexes();
    for (int index : blockIndexes)
    {
        const OInfoBlockGeom geom = info.BlockGeom(index);
        QDomElement blockEl = doc->createElement(VToolSeamAllowance::TagBlock);
        doc->SetAttribute(blockEl, VToolSeamAllowance::AttrIndex, index);
        doc->SetAttribute(blockEl, AttrMx, geom.pos.x());
        doc->SetAttribute(blockEl, AttrMy, geom.pos.y());
        doc->SetAttributeOrRemoveIf<qreal>(blockEl, AttrWidth, geom.width,
                                           [](qreal width) noexcept { return qFuzzyIsNull(width); });
        doc->SetAttributeOrRemoveIf<qreal>(blockEl, AttrHeight, geom.height,
                                           [](qreal height) noexcept { return qFuzzyIsNull(height); });
        doc->SetAttributeOrRemoveIf<qreal>(blockEl, VAbstractPattern::AttrRotation, geom.rotation,
                                           [](qreal rotation) noexcept { return qFuzzyIsNull(rotation); });
        doc->SetAttributeOrRemoveIf<qreal>(blockEl, VToolSeamAllowance::AttrFontScale, geom.fontScale,
                                           [](qreal scale) noexcept { return qFuzzyCompare(scale, 1.0); });
        cartoucheEl.appendChild(blockEl);
    }

    domElement.appendChild(cartoucheEl);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddAnnotations(VAbstractPattern *doc, QDomElement &domElement, const VPiece &piece)
{
    const OAnnotationData &data = piece.GetAnnotationData();
    if (not data.IsSet() || data.Annotations().isEmpty())
    {
        return;  // the <annotations> element is present only when the list is non-empty
    }

    QDomElement annotationsEl = doc->createElement(VToolSeamAllowance::TagAnnotations);
    for (const OAnnotation &annotation : data.Annotations())
    {
        QDomElement annotationEl = doc->createElement(VToolSeamAllowance::TagAnnotation);
        doc->SetAttributeOrRemoveIf<QString>(annotationEl, VAbstractPattern::AttrTemplate, annotation.templateName,
                                             [](const QString &name) noexcept { return name.isEmpty(); });
        doc->SetAttributeOrRemoveIf<QString>(annotationEl, VAbstractPattern::AttrRotation, annotation.rotationFormula,
                                             [](const QString &formula) noexcept { return formula.isEmpty(); });

        QDomElement textEl = doc->createElement(VToolSeamAllowance::TagText);
        textEl.appendChild(doc->createTextNode(annotation.text));
        annotationEl.appendChild(textEl);

        QDomElement geomEl = doc->createElement(VToolSeamAllowance::TagGeom);
        doc->SetAttribute(geomEl, AttrMx, annotation.geom.pos.x());
        doc->SetAttribute(geomEl, AttrMy, annotation.geom.pos.y());
        doc->SetAttributeOrRemoveIf<qreal>(geomEl, AttrWidth, annotation.geom.boxWidth,
                                           [](qreal width) noexcept { return qFuzzyIsNull(width); });
        doc->SetAttributeOrRemoveIf<qreal>(geomEl, AttrHeight, annotation.geom.boxHeight,
                                           [](qreal height) noexcept { return qFuzzyIsNull(height); });
        doc->SetAttributeOrRemoveIf<qreal>(geomEl, VToolSeamAllowance::AttrFontScale, annotation.geom.fontScale,
                                           [](qreal scale) noexcept { return qFuzzyCompare(scale, 1.0); });
        annotationEl.appendChild(geomEl);

        annotationsEl.appendChild(annotationEl);
    }
    domElement.appendChild(annotationsEl);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SetInfoBlockContentBuilder(OInfoBlockContentBuilder builder)
{
    s_infoBlockContentBuilder = std::move(builder);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SetAnnotationStyleResolver(OAnnotationStyleResolver resolver)
{
    s_annotationStyleResolver = std::move(resolver);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddSAMarginProperties(VAbstractPattern *doc, QDomElement &domElement, const VPiece &piece)
{
    const QVector<QVector<quint32>> saNodeIdGroups = piece.GetSAMarginGroupNodeIds();
    if (saNodeIdGroups.isEmpty())
    {
        return;
    }

    const QVector<QString> saNames = piece.GetSAMarginGroupNames();
    const QVector<int> saColors = piece.GetSAMarginGroupColorIndices();
    const QVector<bool> saShows = piece.GetSAMarginGroupShowStates();

    QDomElement saEl = doc->createElement(VToolSeamAllowance::TagSaMarginProperties);
    for (int i = 0; i < saNodeIdGroups.size(); ++i)
    {
        QDomElement gEl = doc->createElement(QStringLiteral("group"));
        QStringList ids;
        for (quint32 id : saNodeIdGroups[i])
        {
            ids << QString::number(id);
        }
        doc->SetAttribute(gEl, QStringLiteral("nodeIds"), ids.join(QLatin1Char(',')));
        if (i < saNames.size() && !saNames[i].isEmpty())
        {
            doc->SetAttribute(gEl, QStringLiteral("name"), saNames[i]);
        }
        if (i < saColors.size())
        {
            doc->SetAttribute(gEl, QStringLiteral("colorIndex"), saColors[i]);
        }
        if (i < saShows.size())
        {
            doc->SetAttribute(gEl, QStringLiteral("show"), saShows[i]);
        }
        saEl.appendChild(gEl);
    }
    domElement.appendChild(saEl);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SetName(const QString &name)
{
    VPiece detail = VAbstractTool::data.GetPiece(m_id);
    detail.SetName(name);
    VAbstractTool::data.UpdatePiece(m_id, detail);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::Move(qreal x, qreal y)
{
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
    VPiece detail = VAbstractTool::data.GetPiece(m_id);
    detail.SetMx(x);
    detail.SetMy(y);
    VAbstractTool::data.UpdatePiece(m_id, detail);

    setPos(x, y);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::Update(const VPiece &piece)
{
    qCDebug(vTool, "VToolSeamAllowance::Update: id=%u start.", m_id);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
    VAbstractTool::data.UpdatePiece(m_id, piece);
    RefreshGeometry();
    qCDebug(vTool, "VToolSeamAllowance::Update: id=%u geometry refreshed, updating scene rect.", m_id);
    VMainGraphicsView::NewSceneRect(m_sceneDetails, VAbstractValApplication::VApp()->getSceneView(), this);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    qCDebug(vTool, "VToolSeamAllowance::Update: id=%u done.", m_id);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::DisconnectOutsideSignals()
{
    // If UnionDetails tool delete the detail this object will be deleted only after full parse.
    // Deleting inside UnionDetails cause crash.
    // Because this object should be inactive from no one we disconnect all signals that may cause a crash
    // KEEP THIS LIST ACTUALL!!!
    disconnect(doc, nullptr, this, nullptr);
    if (QGraphicsScene  const*toolScene = scene())
    {
        disconnect(toolScene, nullptr, this, nullptr);
    }
    disconnect(m_grainLine, nullptr, this, nullptr);
    disconnect(m_sceneDetails, nullptr, this, nullptr);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ConnectOutsideSignals()
{
    connect(m_grainLine, &VGrainlineItem::SignalMoved, this, &VToolSeamAllowance::SaveMoveGrainline);
    connect(m_grainLine, &VGrainlineItem::SignalResized, this, &VToolSeamAllowance::SaveResizeGrainline);
    connect(m_grainLine, &VGrainlineItem::SignalRotated, this, &VToolSeamAllowance::SaveRotateGrainline);

    connect(doc, &VAbstractPattern::CheckLayout, this, &VToolSeamAllowance::UpdateGrainline);
    connect(doc, &VAbstractPattern::CheckLayout, this, &VToolSeamAllowance::RefreshFoldLine);
    connect(doc, &VAbstractPattern::CheckLayout, this, &VToolSeamAllowance::UpdateInfoBlocks);
    connect(doc, &VAbstractPattern::CheckLayout, this, &VToolSeamAllowance::UpdateAnnotations);

    connect(m_sceneDetails, &VMainGraphicsScene::EnableToolMove, this, &VToolSeamAllowance::EnableToolMove);
    connect(m_sceneDetails, &VMainGraphicsScene::ItemByMousePress, this, &VToolSeamAllowance::ResetChildren);
    connect(m_sceneDetails, &VMainGraphicsScene::LanguageChanged, this, &VToolSeamAllowance::retranslateUi);
    connect(m_sceneDetails, &VMainGraphicsScene::EnableDetailItemHover, this, &VToolSeamAllowance::AllowHover);
    connect(m_sceneDetails, &VMainGraphicsScene::EnableDetailItemSelection, this, &VToolSeamAllowance::AllowSelecting);
    connect(m_sceneDetails, &VMainGraphicsScene::HighlightDetail, this, &VToolSeamAllowance::Highlight);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ReinitInternals(const VPiece &detail, VMainGraphicsScene *scene)
{
    InitNodes(detail, scene);
    InitInternalPaths(detail);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::getTagName() const -> QString
{
    return VAbstractPattern::TagDetail;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ShowVisualization(bool show)
{
    Q_UNUSED(show)
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::GroupVisibility(quint32 object, bool visible)
{
    Q_UNUSED(object);
    Q_UNUSED(visible);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::FullUpdateFromFile()
{
    // Do nothing. Refresh triggered outside
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::EnableToolMove(bool move)
{
    setFlag(QGraphicsItem::ItemIsMovable, move);

    m_grainLine->setFlag(QGraphicsItem::ItemIsMovable, move);

    for (OInfoBlockItem *blockItem : m_infoBlockItems)
    {
        blockItem->SetReadOnly(!move);
    }
    for (OAnnotationItem *annotationItem : m_annotationItems)
    {
        annotationItem->SetReadOnly(!move);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AllowHover(bool enabled)
{
    // Manually handle hover events. Need for setting cursor for not selectable paths.
    m_acceptHoverEvents = enabled;
    m_grainLine->setAcceptHoverEvents(enabled);

    for (OInfoBlockItem *blockItem : m_infoBlockItems)
    {
        blockItem->setAcceptHoverEvents(enabled);
    }
    for (OAnnotationItem *annotationItem : m_annotationItems)
    {
        annotationItem->setAcceptHoverEvents(enabled);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AllowSelecting(bool enabled)
{
    setFlag(QGraphicsItem::ItemIsSelectable, enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ResetChildren(QGraphicsItem *pItem)
{
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    const auto *pGLI = qgraphicsitem_cast<VGrainlineItem *>(pItem);

    // If the pressed item is one of this piece's children (grainline or an info
    // block), keep the piece selected so the child stays interactive. This used
    // to be polled via IsIdle() inside paint(), which mutates selection state
    // during paint and crashes on Windows under Qt 6.
    OInfoBlockItem *pBlock = nullptr;
    for (OInfoBlockItem *blockItem : m_infoBlockItems)
    {
        if (blockItem == pItem)
        {
            pBlock = blockItem;
            break;
        }
    }
    OAnnotationItem *pAnnotation = nullptr;
    for (OAnnotationItem *annotationItem : m_annotationItems)
    {
        if (annotationItem == pItem)
        {
            pAnnotation = annotationItem;
            break;
        }
    }
    const bool myChildPressed = (pGLI != nullptr && pGLI == m_grainLine) || pBlock != nullptr
                                || pAnnotation != nullptr;
    const bool selected = isSelected() || myChildPressed;

    if (pGLI != m_grainLine && detail.GetGrainlineGeometry().IsEnabled())
    {
        m_grainLine->Reset();
    }

    // Only the clicked block shows its editing outline + handles.
    for (OInfoBlockItem *blockItem : m_infoBlockItems)
    {
        blockItem->SetActive(blockItem == pBlock);
    }
    // Only the clicked annotation shows its editing outline + handles.
    for (OAnnotationItem *annotationItem : m_annotationItems)
    {
        annotationItem->SetActive(annotationItem == pAnnotation);
    }

    setSelected(selected);
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::UpdateAll()
{
    m_sceneDetails->update();
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::retranslateUi()
{
    // No label text to retranslate anymore.
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::Highlight(quint32 id)
{
    setSelected(m_id == id);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::UpdatePassmarks()
{
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    m_passmarks->setPath(RenderPassmarks(detail, getData()));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VToolDetail::UpdateGrainline updates the grain line item
 */
void VToolSeamAllowance::UpdateGrainline()
{
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    const VGrainlineData &geom = detail.GetGrainlineGeometry();
    const QVector<quint32> &pins = detail.GetPins();

    if (geom.IsEnabled() && geom.IsVisible())
    {
        QPointF pos;
        qreal dRotation = 0;
        qreal dLength = 0;

        const VGrainlineItem::MoveTypes type = FindGrainlineGeometry(geom, pins, dLength, dRotation, pos);
        if ((type & VGrainlineItem::Error) != 0U)
        {
            m_grainLine->hide();
            return;
        }

        m_grainLine->SetMoveType(type);
        m_grainLine->UpdateGeometry(pos, dRotation, ToPixel(dLength, *VDataTool::data.GetPatternUnit()),
                                    geom.GetArrowType());
        m_grainLine->show();

        if (m_geometryIsReady && not IsGrainlinePositionValid())
        {
            const QString errorMsg = QObject::tr("Piece '%1'. Grainline is not valid.").arg(detail.GetName());
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
    }
    else
    {
        m_grainLine->hide();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SaveMoveGrainline(const QPointF &ptPos)
{
    VPiece const oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;
    newDet.GetGrainlineGeometry().SetPos(ptPos);
    qDebug() << "******* new grainline pos" << ptPos;

    auto *moveCommand = new SavePieceOptions(oldDet, newDet, doc, m_id);
    moveCommand->setText(tr("move grainline"));
    VAbstractApplication::VApp()->getUndoStack()->push(moveCommand);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SaveResizeGrainline(qreal dLength)
{
    VPiece const oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;

    dLength = FromPixel(dLength, *VDataTool::data.GetPatternUnit());
    newDet.GetGrainlineGeometry().SetPos(m_grainLine->pos());
    newDet.GetGrainlineGeometry().SetLength(QString().setNum(dLength));

    auto *resizeCommand = new SavePieceOptions(oldDet, newDet, doc, m_id);
    resizeCommand->setText(tr("resize grainline"));
    VAbstractApplication::VApp()->getUndoStack()->push(resizeCommand);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SaveRotateGrainline(qreal dRot, const QPointF &ptPos)
{
    VPiece const oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;

    newDet.GetGrainlineGeometry().SetRotation(QString().setNum(qRadiansToDegrees(dRot)));
    newDet.GetGrainlineGeometry().SetPos(ptPos);

    auto *rotateCommand = new SavePieceOptions(oldDet, newDet, doc, m_id);
    rotateCommand->setText(tr("rotate grainline"));
    VAbstractApplication::VApp()->getUndoStack()->push(rotateCommand);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::UpdateInfoBlocks()
{
    if (!s_infoBlockContentBuilder)
    {
        return;  // no app-side resolver registered
    }
    constexpr qreal kBlockStackingGap = 12.0;
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    const OInfoBlockEffectiveState state = s_infoBlockContentBuilder(detail, doc, getData());

    for (int block = 1; block <= 11; ++block)
    {
        OInfoBlockItem *blockItem = m_infoBlockItems.at(block - 1);
        const bool exists = state.active && state.blockContent.contains(block);
        if (!exists)
        {
            blockItem->SetActive(false);
            blockItem->hide();
            continue;
        }

        OInfoBlockGeom geom = detail.GetInfoBlockData().BlockGeom(block);
        const QVector<OCartoucheRenderLine> lines = state.blockContent.value(block).toVector();

        // The block always fits its content at the current zoom (fontScale): the
        // size is recomputed on every refresh, so a template edit resizes the block
        // to its new content while keeping the user's resize (the zoom) intact.
        // The painter scales the margins with the zoom, so the fitted size is
        // simply the natural size times the scale.
        const QSizeF natural = OCartouchePainter::NaturalSize(lines);
        const qreal scale = geom.fontScale > 0.0 ? geom.fontScale : 1.0;
        geom.width = scale * natural.width();
        geom.height = scale * natural.height();

        if (!detail.GetInfoBlockData().HasStoredGeometry(block))
        {
            // Auto-place: centered on the piece; further blocks stacked below.
            geom.pos = m_pieceBoundingRect.center() + QPointF(0.0, (block - 1) * (geom.height + kBlockStackingGap));
        }

        blockItem->setVisible(true);
        blockItem->SetBlockGeom(geom);
        blockItem->SetContent(lines, geom.fontScale);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SaveMoveInfoBlock(int blockIndex, const QPointF &pos)
{
    VPiece const oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;
    OInfoBlockData data = std::as_const(newDet).GetInfoBlockData();
    if (!data.IsSet() && s_infoBlockContentBuilder)
    {
        // First edit: snapshot the effective (seed-derived) state so the seed is never lost.
        const OInfoBlockEffectiveState st = s_infoBlockContentBuilder(oldDet, doc, getData());
        data.SetActive(st.active);
        data.SetTemplate(st.templateName);
        data.SetDispatch(st.dispatch);
        data.SetSet(true);
    }
    OInfoBlockGeom geom = data.BlockGeom(blockIndex);
    geom.pos = pos;
    data.SetBlockGeom(blockIndex, geom);
    newDet.SetInfoBlockData(data);

    auto *moveCommand = new SavePieceOptions(oldDet, newDet, doc, m_id);
    moveCommand->setText(tr("move information block"));
    VAbstractApplication::VApp()->getUndoStack()->push(moveCommand);
    UpdateInfoBlocks();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SaveResizeInfoBlock(int blockIndex, qreal newW, qreal newH, qreal origW, qreal origH)
{
    Q_UNUSED(origH)

    VPiece const oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;
    OInfoBlockData data = std::as_const(newDet).GetInfoBlockData();
    if (!data.IsSet() && s_infoBlockContentBuilder)
    {
        // First edit: snapshot the effective (seed-derived) state so the seed is never lost.
        const OInfoBlockEffectiveState st = s_infoBlockContentBuilder(oldDet, doc, getData());
        data.SetActive(st.active);
        data.SetTemplate(st.templateName);
        data.SetDispatch(st.dispatch);
        data.SetSet(true);
    }
    OInfoBlockGeom geom = data.BlockGeom(blockIndex);
    geom.pos = m_infoBlockItems.at(blockIndex - 1)->pos();
    geom.width = newW;
    geom.height = newH;
    // Resize is always proportional: the content scales with the block.
    geom.fontScale *= (newW / origW);
    data.SetBlockGeom(blockIndex, geom);
    newDet.SetInfoBlockData(data);

    auto *resizeCommand = new SavePieceOptions(oldDet, newDet, doc, m_id);
    resizeCommand->setText(tr("resize information block"));
    VAbstractApplication::VApp()->getUndoStack()->push(resizeCommand);
    UpdateInfoBlocks();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::UpdateAnnotations()
{
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    const QList<OAnnotation> annotations = detail.GetAnnotationData().Annotations();

    // Grow the item pool on demand — never delete/recreate (leaks + duplicate
    // connections); hidden items stay pooled, pattern of the 11 blocks.
    while (m_annotationItems.size() < annotations.size())
    {
        const int index = static_cast<int>(m_annotationItems.size());
        auto *item = new OAnnotationItem(index, this);
        connect(item, &OAnnotationItem::Moved, this, &VToolSeamAllowance::SaveMoveAnnotation);
        connect(item, &OAnnotationItem::Resized, this, &VToolSeamAllowance::SaveResizeAnnotation);
        item->hide();
        m_annotationItems.append(item);
    }

    for (int i = 0; i < m_annotationItems.size(); ++i)
    {
        OAnnotationItem *item = m_annotationItems.at(i);
        const bool exists = i < annotations.size() && !annotations.at(i).text.isEmpty();
        if (!exists)
        {
            item->SetActive(false);
            item->hide();
            continue;
        }

        const OAnnotation &annotation = annotations.at(i);
        item->SetIndex(i);
        item->SetContent(annotation.text, ResolveAnnotationStyle(annotation.templateName));
        OAnnotationGeom geom = annotation.geom;
        if (qFuzzyIsNull(geom.pos.x()) && qFuzzyIsNull(geom.pos.y()))
        {
            // Never moved (pos still the default): auto-place centered on the piece.
            geom.pos = m_pieceBoundingRect.center();
        }
        item->SetAnnotationGeom(geom, EvaluateAnnotationRotation(annotation.rotationFormula));
        item->setVisible(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::ResolveAnnotationStyle(const QString &name) const -> OAnnotationRenderStyle
{
    if (s_annotationStyleResolver)
    {
        return s_annotationStyleResolver(name);
    }
    return OAnnotationRenderStyle();  // default font, transparent background, black text
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::EvaluateAnnotationRotation(const QString &formula) const -> qreal
{
    if (formula.isEmpty())
    {
        return 0.0;
    }
    Calculator cal;
    const qreal value = cal.EvalFormula(VAbstractTool::data.DataVariables(), formula);
    if (qIsNaN(value) || qIsInf(value))
    {
        return 0.0;  // invalid at render time → fallback 0° (spec §10)
    }
    return value;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SaveMoveAnnotation(int index, const QPointF &pos)
{
    VPiece const oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;
    OAnnotationData data = std::as_const(newDet).GetAnnotationData();
    QList<OAnnotation> annotations = data.Annotations();
    if (index < 0 || index >= annotations.size())
    {
        return;
    }
    annotations[index].geom.pos = pos;
    data.SetAnnotations(annotations);
    data.SetSet(true);
    newDet.SetAnnotationData(data);

    auto *moveCommand = new SavePieceOptions(oldDet, newDet, doc, m_id);
    moveCommand->setText(tr("move annotation"));
    VAbstractApplication::VApp()->getUndoStack()->push(moveCommand);
    UpdateAnnotations();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SaveResizeAnnotation(int index, qreal newW, qreal newH, qreal origW, qreal origH)
{
    VPiece const oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;
    OAnnotationData data = std::as_const(newDet).GetAnnotationData();
    QList<OAnnotation> annotations = data.Annotations();
    if (index < 0 || index >= annotations.size())
    {
        return;
    }
    OAnnotation &annotation = annotations[index];
    // The drag kept the opposite corner/edge fixed by moving the item — persist
    // that final position too, or the refresh would snap the box back to the old
    // stored position (spec §7.2: pos/width/height are stored as-is).
    annotation.geom.pos = m_annotationItems.at(index)->pos();
    annotation.geom.boxWidth = newW;
    annotation.geom.boxHeight = newH;
    // A corner resize scales the content too; an edge resize only grows the box
    // (the two ratios match only for a proportional corner scale).
    const qreal rx = newW / origW;
    const qreal ry = newH / origH;
    if (qFuzzyCompare(rx, ry))
    {
        annotation.geom.fontScale *= rx;
    }
    data.SetAnnotations(annotations);
    data.SetSet(true);
    newDet.SetAnnotationData(data);

    auto *resizeCommand = new SavePieceOptions(oldDet, newDet, doc, m_id);
    resizeCommand->setText(tr("resize annotation"));
    VAbstractApplication::VApp()->getUndoStack()->push(resizeCommand);
    UpdateAnnotations();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VToolDetail::paint draws a bounding box around detail, if one of its text or grainline items is not idle.
 */
void VToolSeamAllowance::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Do not mutate item state here (pens, brushes, selection). Since Qt 6 changing such state
    // inside paint() crashes the paint pipeline on Windows. Scale/settings-dependent pens and
    // brushes live in RefreshScale(); keeping the piece selected while one of its child labels
    // is manipulated is handled in ResetChildren().
    PaintWithFixItemHighlightSelected<QGraphicsPathItem>(this, painter, option, widget);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::RefreshScale()
{
    QPen toolPen = pen();
    toolPen.setWidthF(ScaleWidth(VAbstractApplication::VApp()->Settings()->WidthHairLine(), SceneScale(scene())));
    toolPen.setColor(VSceneStylesheet::PatternPieceStyle().PieceColor());

    setPen(toolPen);
    m_seamAllowance->setPen(toolPen);
    m_passmarks->setPen(toolPen);
    m_placeLabels->setPen(toolPen);
    m_foldLineMark->setPen(toolPen);
    m_foldLineLabel->setPen(toolPen);

    QPen mirrorLinePen = toolPen;
    mirrorLinePen.setStyle(Qt::DashDotLine);
    m_mirrorLine->setPen(mirrorLinePen);

    // The fold-line mark and the "Cut On Fold" label are filled with the piece color
    // (theme-aware: re-applied on every Light/Dark toggle via RefreshSceneItemPens).
    const QBrush foldLineBrush(VSceneStylesheet::PatternPieceStyle().PieceColor());
    m_foldLineMark->setBrush(foldLineBrush);
    m_foldLineLabelText->setBrush(foldLineBrush);

    VCommonSettings  const*settings = VAbstractApplication::VApp()->Settings();
    if (settings->GetSingleStrokeOutlineFont() || settings->GetSingleLineFonts())
    {
        m_foldLineLabel->setBrush(Qt::NoBrush);
    }
    else
    {
        m_foldLineLabel->setBrush(foldLineBrush);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::boundingRect() const -> QRectF
{
    if (m_pieceBoundingRect.isNull())
    {
        return QGraphicsPathItem::boundingRect();
    }

    return m_pieceBoundingRect;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::shape() const -> QPainterPath
{
    if (m_mainPath == QPainterPath())
    {
        return QGraphicsPathItem::shape();
    }

    return ItemShapeFromPath(m_mainPath, pen());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::FullUpdateFromGuiApply()
{
    SaveDialogChange(tr("apply save detail options"));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddToFile()
{
    const QDomElement duplicate = doc->FindElementById(m_id);
    if (not duplicate.isNull())
    {
        throw VExceptionWrongId(tr("This id (%1) is not unique.").arg(m_id), duplicate);
    }

    const VPiece piece = VAbstractTool::data.GetPiece(m_id);

    QDomElement domElement = doc->createElement(VToolSeamAllowance::getTagName());

    AddAttributes(doc, domElement, m_id, piece);
    AddGrainline(doc, domElement, piece);

    // nodes
    AddNodes(doc, domElement, piece);
    // custom seam allowance
    AddCSARecords(doc, domElement, piece.GetCustomSARecords());
    AddInternalPaths(doc, domElement, piece.GetInternalPaths());
    AddPins(doc, domElement, piece.GetPins());
    AddPlaceLabels(doc, domElement, piece.GetPlaceLabels());
    AddSAMarginProperties(doc, domElement, piece);
    AddMirrorLine(doc, domElement, piece);
    AddCartoucheBlocks(doc, domElement, piece);
    AddAnnotations(doc, domElement, piece);

    VAbstractApplication::VApp()->getUndoStack()->push(
        new AddPiece(domElement, doc, VAbstractTool::data, m_sceneDetails, m_drawName));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::RefreshDataInFile()
{
    QDomElement domElement = doc->FindElementById(m_id, VToolSeamAllowance::getTagName());
    if (!domElement.isElement())
    {
        return;
    }

    // Refresh only parts that we possibly need to update

    // TODO. Delete if minimal supported version is 0.4.0
    Q_STATIC_ASSERT_X(VPatternConverter::PatternMinVer < FormatVersion(0, 4, 0), "Time to refactor the code.");

    const uint version = VAbstractPattern::GetParametrUInt(domElement, AttrVersion, QChar('1'));
    if (version == 1)
    {
        const VPiece piece = VAbstractTool::data.GetPiece(m_id);

        doc->SetAttribute(domElement, AttrVersion, QString().setNum(pieceVersion));

        VAbstractPattern::RemoveAllChildren(domElement); // Very important to clear before rewrite
        AddGrainline(doc, domElement, piece);
        AddNodes(doc, domElement, piece);
        AddCSARecords(doc, domElement, piece.GetCustomSARecords());
        AddInternalPaths(doc, domElement, piece.GetInternalPaths());
        AddPins(doc, domElement, piece.GetPins());
        AddPlaceLabels(doc, domElement, piece.GetPlaceLabels());
        AddSAMarginProperties(doc, domElement, piece);
        AddMirrorLine(doc, domElement, piece);
        AddCartoucheBlocks(doc, domElement, piece);
        AddAnnotations(doc, domElement, piece);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value) -> QVariant
{
    if (change == ItemPositionChange && scene())
    {
        // Each time we move something we call recalculation scene rect. In some cases this can cause moving
        // objects positions. And this cause infinite redrawing. That's why we wait the finish of saving the last move.
        static bool changeFinished = true;
        if (changeFinished)
        {
            changeFinished = false;

            // value - this is new position.
            const QPointF newPos = value.toPointF();

            VAbstractApplication::VApp()->getUndoStack()->push(
                new MovePiece(doc, newPos.x(), newPos.y(), m_id, scene()));

            const QList<QGraphicsView *> viewList = scene()->views();
            if (not viewList.isEmpty())
            {
                if (auto *view = qobject_cast<VMainGraphicsView *>(viewList.at(0)))
                {
                    view->EnsureItemVisibleWithDelay(this, VMainGraphicsView::scrollDelay);
                }
            }

            changeFinished = true;
        }
    }

    if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        if (value == true)
        {
            // do stuff if selected
            this->setFocus();
        }
        else
        {
            // do stuff if not selected
        }
    }

    return QGraphicsPathItem::itemChange(change, value);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Special for not selectable item first need to call standard mousePressEvent then accept event
    QGraphicsPathItem::mousePressEvent(event);

    // Somehow clicking on notselectable object do not clean previous selections.
    if (not(flags() & ItemIsSelectable) && scene())
    {
        scene()->clearSelection();
    }

    if (flags() & QGraphicsItem::ItemIsMovable && event->button() == Qt::LeftButton &&
        event->type() != QEvent::GraphicsSceneMouseDoubleClick)
    {
        SetItemOverrideCursor(this, cursorArrowCloseHand, 1, 1);
    }

    if (m_selectionType == SelectionType::ByMouseRelease)
    {
        event
            ->accept(); // Special for not selectable item first need to call standard mousePressEvent then accept event
    }
    else
    {
        if (event->button() == Qt::LeftButton && event->type() != QEvent::GraphicsSceneMouseDoubleClick)
        {
            doc->SelectedDetail(m_id);
            emit ChoosedTool(m_id, SceneObject::Detail);
            event->accept();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && (flags() & QGraphicsItem::ItemIsMovable))
    {
        SetItemOverrideCursor(this, cursorArrowOpenHand, 1, 1);
    }

    if (m_selectionType == SelectionType::ByMouseRelease && IsSelectedByReleaseEvent(this, event))
    {
        doc->SelectedDetail(m_id);
        emit ChoosedTool(m_id, SceneObject::Detail);
    }
    QGraphicsPathItem::mouseReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (m_acceptHoverEvents)
    {
        if (flags() & QGraphicsItem::ItemIsMovable)
        {
            SetItemOverrideCursor(this, cursorArrowOpenHand, 1, 1);
        }
        else
        {
            setCursor(VAbstractValApplication::VApp()->getSceneView()->viewport()->cursor());
        }
        QGraphicsPathItem::hoverEnterEvent(event);
    }
    else
    {
        setCursor(VAbstractValApplication::VApp()->getSceneView()->viewport()->cursor());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (m_acceptHoverEvents)
    {
        QGraphicsPathItem::hoverLeaveEvent(event);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (m_suppressContextMenu)
    {
        return;
    }

    QMenu menu;
    QAction  const*actionOption = menu.addAction(FromTheme(VThemeIcon::PreferencesOther), tr("Options"));

    const VPiece detail = VAbstractTool::data.GetPiece(m_id);

    QAction *inLayoutOption = menu.addAction(tr("In layout"));
    inLayoutOption->setCheckable(true);
    inLayoutOption->setChecked(detail.IsInLayout());

    QAction *hideMainPathOption = menu.addAction(tr("Hide main path"));
    hideMainPathOption->setCheckable(true);
    hideMainPathOption->setChecked(detail.IsHideMainPath());

    QAction *forbidFlippingOption = menu.addAction(tr("Forbid flipping"));
    forbidFlippingOption->setCheckable(true);
    forbidFlippingOption->setChecked(detail.IsForbidFlipping());

    QAction *forceFlippingOption = menu.addAction(tr("Force flipping"));
    forceFlippingOption->setCheckable(true);
    forceFlippingOption->setChecked(detail.IsForceFlipping());

    QAction *showFullPiece = menu.addAction(tr("Show full piece"));
    showFullPiece->setCheckable(true);
    {
        const QLineF mirrorLine = detail.SeamAllowanceMirrorLine(&(VAbstractTool::data));
        showFullPiece->setEnabled(not mirrorLine.isNull());
        showFullPiece->setChecked(true);
        if (not mirrorLine.isNull())
        {
            showFullPiece->setChecked(detail.IsShowFullPiece());
        }
    }

    QAction *actionRemove = menu.addAction(FromTheme(VThemeIcon::EditDelete), tr("Delete"));
    actionRemove->setEnabled(IsRemovable() == RemoveStatus::Removable);

    QAction  const*selectedAction = menu.exec(event->screenPos());
    if (selectedAction == actionOption)
    {
        ShowOptions();
    }
    else if (selectedAction == inLayoutOption)
    {
        ToggleInLayout(selectedAction->isChecked());
    }
    else if (selectedAction == hideMainPathOption)
    {
        ToggleHideMainPath(selectedAction->isChecked());
    }
    else if (selectedAction == forbidFlippingOption)
    {
        ToggleForbidFlipping(selectedAction->isChecked());
    }
    else if (selectedAction == forceFlippingOption)
    {
        ToggleForceFlipping(selectedAction->isChecked());
    }
    else if (selectedAction == showFullPiece)
    {
        ToggleShowFullPiece(selectedAction->isChecked());
    }
    else if (selectedAction == actionRemove)
    {
        try
        {
            DeleteFromMenu();
        }
        catch (const VExceptionToolWasDeleted &e)
        {
            Q_UNUSED(e);
            return; // Leave this method immediately!!!
        }
        // Leave this method immediately after call!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::keyReleaseEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Delete:
            if (ConfirmDeletion() == QMessageBox::Yes)
            {
                const QList<VToolSeamAllowance *> toolList = SelectedTools();

                try
                {
                    if (not toolList.isEmpty())
                    {
                        VAbstractApplication::VApp()->getUndoStack()->beginMacro(tr("multi deletion"));

                        for (auto *tool : toolList)
                        {
                            tool->RemoveWithConfirm(false);
                        }
                    }
                    DeleteToolWithConfirm(false);
                }
                catch (const VExceptionToolWasDeleted &e)
                {
                    Q_UNUSED(e);
                    if (not toolList.isEmpty())
                    {
                        VAbstractApplication::VApp()->getUndoStack()->endMacro();
                    }
                    return; // Leave this method immediately!!!
                }
            }
            break;
        default:
            break;
    }

    QGraphicsPathItem::keyReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SetDialog()
{
    SCASSERT(not m_dialog.isNull());
    const QPointer<DialogSeamAllowance> dialogTool = qobject_cast<DialogSeamAllowance *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    dialogTool->SetPiece(VAbstractTool::data.GetPiece(m_id));
    dialogTool->EnableApply(true);
}

//---------------------------------------------------------------------------------------------------------------------
VToolSeamAllowance::VToolSeamAllowance(const VToolSeamAllowanceInitData &initData, QGraphicsItem *parent)
  : VInteractiveTool(initData.doc, initData.data, initData.id),
    QGraphicsPathItem(parent),
    m_sceneDetails(initData.scene),
    m_drawName(initData.drawName),
    m_seamAllowance(new VNoBrushScalePathItem(this)),
    m_grainLine(new VGrainlineItem(VColorRole::PieceColor, this)),
    m_passmarks(new QGraphicsPathItem(this)),
    m_placeLabels(new QGraphicsPathItem(this)),
    m_mirrorLine(new QGraphicsPathItem(this)),
    m_foldLineMark(new QGraphicsPathItem(this)),
    m_foldLineLabel(new QGraphicsPathItem(this)),
    m_foldLineLabelText(new QGraphicsSimpleTextItem(this)),
    m_saHighlights(this)
{
    VPiece const detail = initData.data->GetPiece(initData.id);
    ReinitInternals(detail, m_sceneDetails);
    VToolSeamAllowance::AllowSelecting(true);
    EnableToolMove(true);
    VToolSeamAllowance::AllowHover(true);

    this->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    this->setFlag(QGraphicsItem::ItemIsFocusable, true); // For keyboard input focus

    VToolSeamAllowance::ToolCreation(initData.typeCreation);
    setAcceptHoverEvents(m_acceptHoverEvents);

    connect(this, &VToolSeamAllowance::ChoosedTool, m_sceneDetails, &VMainGraphicsScene::ChoosedItem);
    connect(m_sceneDetails, &VMainGraphicsScene::EnableToolMove, this, &VToolSeamAllowance::EnableToolMove);
    connect(m_sceneDetails, &VMainGraphicsScene::ItemSelection, this, &VToolSeamAllowance::ToolSelectionType);
    connect(m_sceneDetails, &VMainGraphicsScene::UpdatePassmarks, this, &VToolSeamAllowance::UpdatePassmarks);

    ConnectOutsideSignals();

    // Re-paint the seam-allowance highlight colors when the theme toggles. `this` is
    // a QObject and the tools are re-created on every reparse, so the connection is
    // cleaned up automatically and never duplicated.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this,
            [this]() { m_saHighlights.UpdateColors(); });

    // The fold-line mark and the "Cut On Fold" label follow the piece color (theme-aware),
    // not a hard-coded black: the black fill was invisible on the dark canvas.
    const QBrush foldLineBrush(VSceneStylesheet::PatternPieceStyle().PieceColor());
    m_foldLineMark->setBrush(foldLineBrush);
    m_foldLineLabelText->setBrush(foldLineBrush);

    // On-piece information blocks (one per block 1..11), hidden until a piece
    // configures them (UpdateInfoBlocks decides visibility).
    m_infoBlockItems.reserve(11);
    for (int block = 1; block <= 11; ++block)
    {
        auto *blockItem = new OInfoBlockItem(block, this);
        connect(blockItem, &OInfoBlockItem::Moved, this, &VToolSeamAllowance::SaveMoveInfoBlock);
        connect(blockItem, &OInfoBlockItem::Resized, this, &VToolSeamAllowance::SaveResizeInfoBlock);
        m_infoBlockItems.append(blockItem);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::UpdateExcludeState()
{
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    UpdatePathExcludeState(detail.GetPath());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::UpdateInternalPaths()
{
    VPiece piece = VAbstractTool::data.GetPiece(m_id);
    piece.TestInternalPaths(&(VAbstractTool::data));
    const QVector<quint32> paths = piece.GetInternalPaths();
    for (auto pathId : paths)
    {
        try
        {
            if (auto *tool = qobject_cast<VToolPiecePath *>(VAbstractPattern::getTool(pathId)))
            {
                tool->RefreshGeometry();

                const VPiecePath pathItem = VAbstractTool::data.GetPiecePath(pathId);
                for (int i = 0; i < pathItem.CountNodes(); ++i)
                {
                    InitInternalNode(pathItem.at(i), this);
                }
            }
        }
        catch (const VExceptionBadId &)
        {
            // ignore
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::RefreshGeometry(bool updateChildren)
{
    qCDebug(vTool, "VToolSeamAllowance::RefreshGeometry: id=%u start.", m_id);
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    const bool combineTogether = settings->IsBoundaryTogetherWithNotches();
    const bool pieceShowMainPath = VAbstractApplication::VApp()->Settings()->IsPieceShowMainPath();

    ApplyPieceGeometry(ComputePieceGeometry(combineTogether, pieceShowMainPath), updateChildren);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::PrepareRefreshGeometry(bool combineTogether, bool pieceShowMainPath)
{
    // Runs off the GUI thread (e.g. via QtConcurrent for several pieces at once). Must not touch any
    // QGraphicsItem; only ComputePieceGeometry() work happens here. The result is applied later by
    // ApplyBatchGeometry() on the GUI thread.
    try
    {
        m_batchGeometry = ComputePieceGeometry(combineTogether, pieceShowMainPath);
    }
    catch (const VExceptionBadId &)
    {
        m_batchGeometry = VToolSeamAllowanceGeometry(); // valid stays false -> piece skipped on apply
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ApplyBatchGeometry(bool updateChildren)
{
    if (m_batchGeometry.valid)
    {
        ApplyPieceGeometry(m_batchGeometry, updateChildren);
    }
    m_batchGeometry = VToolSeamAllowanceGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::ComputePieceGeometry(bool combineTogether, bool pieceShowMainPath) const
    -> VToolSeamAllowanceGeometry
{
    // Pure geometry computation: safe to run on a worker thread and concurrently for different
    // pieces. It must not access any QGraphicsItem - everything GUI-related lives in
    // ApplyPieceGeometry().
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    const VContainer *containerData = getData();

    VToolSeamAllowanceGeometry geom;
    geom.combineTogether = combineTogether;
    geom.pos = QPointF(detail.GetMx(), detail.GetMy());
    geom.pieceName = detail.GetName();

    geom.seamPath = RenderSeamPath(detail, combineTogether, containerData);

    if (!combineTogether)
    {
        geom.passmarks = RenderPassmarks(detail, containerData);
    }

    geom.foldLine = RenderFoldLine(detail, containerData);
    geom.mirrorLine = RenderMirrorLine(detail, containerData, combineTogether);

    geom.hasSeamAllowance = detail.IsSeamAllowance() && not detail.IsSeamAllowanceBuiltIn();
    if (geom.hasSeamAllowance)
    {
        geom.seamAllowance = RenderSeamAllowancePath(detail, combineTogether, containerData);
        geom.seamAllowanceValid = detail.IsSeamAllowanceValid(containerData);
    }

    geom.showMainPath = pieceShowMainPath || not detail.IsHideMainPath() || not detail.IsSeamAllowance() ||
                        detail.IsSeamAllowanceBuiltIn();

    geom.placeLabels = detail.PlaceLabelPath(containerData);

    geom.valid = true;
    return geom;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ApplyPieceGeometry(const VToolSeamAllowanceGeometry &geom, bool updateChildren)
{
    // Applies a previously computed geometry snapshot to the scene items. Must run on the GUI thread.
    this->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);

    this->setPos(geom.pos);

    QPainterPath path;

    if (geom.showMainPath)
    {
        m_mainPath = QPainterPath();
        m_seamAllowance->setBrush(QBrush(VSceneStylesheet::PatternPieceStyle().PieceColor(), Qt::Dense7Pattern));
        path = geom.seamPath;
    }
    else
    {
        m_seamAllowance->setBrush(QBrush(Qt::NoBrush)); // Disable if the main path was hidden
        // need for returning a bounding rect when main path is not visible
        m_mainPath = geom.seamPath;
        path = QPainterPath();
    }

    this->setPath(path);

    m_placeLabels->setPath(geom.placeLabels);
    m_saHighlights.Update(VAbstractTool::data.GetPiece(m_id), getData());

    if (geom.hasSeamAllowance)
    {
        if (not geom.seamAllowanceValid)
        {
            qCDebug(vTool, "VToolSeamAllowance::RefreshGeometry: id=%u seam allowance is not valid.", m_id);
            const QString errorMsg = QObject::tr("Piece '%1'. Seam allowance is not valid.").arg(geom.pieceName);
            VAbstractApplication::VApp()->IsPedantic()
                ? throw VException(errorMsg)
                : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        }
        path.addPath(geom.seamAllowance);
        path.setFillRule(Qt::OddEvenFill);
        m_seamAllowance->setPath(path);

        m_pieceBoundingRect = m_seamAllowance->path().controlPointRect();
    }
    else
    {
        m_seamAllowance->setPath(QPainterPath());

        m_pieceBoundingRect = m_mainPath.controlPointRect();
    }

    m_mirrorLine->setPath(geom.mirrorLine);

    UpdateFoldLine(geom.foldLine);

    if (VAbstractApplication::VApp()->IsAppInGUIMode())
    {
        QTimer::singleShot(100ms,
                           Qt::CoarseTimer,
                           this,
                           [this, updateChildren]() -> void
                           {
                               this->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
                               UpdateGrainline();
                               UpdateInfoBlocks();
                               UpdateAnnotations();
                               UpdateExcludeState();
                               if (updateChildren)
                               {
                                   UpdateInternalPaths();
                               }
                               this->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
                           });
    }
    else
    {
        UpdateGrainline();
        UpdateInfoBlocks();
        UpdateAnnotations();
        UpdateExcludeState();
        if (updateChildren)
        {
            UpdateInternalPaths();
        }
    }

    m_passmarks->setPath(!geom.combineTogether ? geom.passmarks : QPainterPath());

    RefreshScale();

    this->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    // Now we can start checking validity of the grainline
    m_geometryIsReady = true;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::SaveDialogChange(const QString &undoText)
{
    SCASSERT(not m_dialog.isNull());
    auto *dialogTool = qobject_cast<DialogSeamAllowance *>(m_dialog.data());
    SCASSERT(dialogTool != nullptr);
    const VPiece newDet = dialogTool->GetPiece();
    const VPiece oldDet = VAbstractTool::data.GetPiece(m_id);

    QVector<QPointer<VUndoCommand>> &undocommands = dialogTool->UndoStack();
    const bool groupChange = not undocommands.isEmpty();

    auto *saveCommand = new SavePieceOptions(oldDet, newDet, doc, m_id);
    if (auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow()))
    { // Better not to crash here, just silently do not update list.
        connect(saveCommand, &SavePieceOptions::UpdateGroups, window, &VAbstractMainWindow::UpdateDetailsList);
    }

    if (groupChange)
    {
        VAbstractApplication::VApp()->getUndoStack()->beginMacro(undoText.isEmpty() ? saveCommand->text() : undoText);

        for (auto command : undocommands)
        {
            VAbstractApplication::VApp()->getUndoStack()->push(command);
            command.clear(); // To prevent double free memory
        }
        undocommands.clear();
    }

    VAbstractApplication::VApp()->getUndoStack()->push(saveCommand);

    if (groupChange)
    {
        VAbstractApplication::VApp()->getUndoStack()->endMacro();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ShowOptions()
{
    qCDebug(vTool, "Show options for tool id=%u (%s).", m_id, qUtf8Printable(getTagName()));
    QPointer<DialogSeamAllowance> const dialog =
        new DialogSeamAllowance(getData(), doc, m_id, VAbstractValApplication::VApp()->getMainWindow());
    dialog->EnableApply(true);
    m_dialog = dialog;
    m_dialog->setModal(true);
    connect(m_dialog.data(), &DialogTool::DialogClosed, this, &VToolSeamAllowance::FullUpdateFromGuiOk);
    connect(m_dialog.data(), &DialogTool::DialogApplied, this, &VToolSeamAllowance::FullUpdateFromGuiApply);
    BlockUndoRedoWhileDialogOpen();
    SetDialog();
    m_dialog->show();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToggleInLayout(bool checked)
{
    auto *toggleInLayout = new TogglePieceInLayout(m_id, checked, &(VAbstractTool::data), doc);
    connect(toggleInLayout, &TogglePieceInLayout::Toggled, doc, &VAbstractPattern::CheckInLayoutList);
    VAbstractApplication::VApp()->getUndoStack()->push(toggleInLayout);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToggleHideMainPath(bool checked)
{
    auto *toggleHideMainPath = new class ToggleHideMainPath(m_id, checked, &(VAbstractTool::data), doc);
    connect(toggleHideMainPath, &ToggleHideMainPath::Toggled, this, [this]() { RefreshGeometry(false); });
    VAbstractApplication::VApp()->getUndoStack()->push(toggleHideMainPath);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToggleForbidFlipping(bool checked)
{
    VAbstractApplication::VApp()->getUndoStack()->push(new TogglePieceForceForbidFlipping(
        m_id, checked, ForceForbidFlippingType::ForbidFlipping, &(VAbstractTool::data), doc));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToggleForceFlipping(bool checked)
{
    VAbstractApplication::VApp()->getUndoStack()->push(new TogglePieceForceForbidFlipping(
        m_id, checked, ForceForbidFlippingType::ForceFlipping, &(VAbstractTool::data), doc));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToggleShowFullPiece(bool checked)
{
    auto *toggleShowFullPiece = new class ToggleShowFullPiece(m_id, checked, &(VAbstractTool::data), doc);
    connect(toggleShowFullPiece, &ToggleShowFullPiece::Toggled, this, [this]() -> void { RefreshGeometry(true); });
    VAbstractApplication::VApp()->getUndoStack()->push(toggleShowFullPiece);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::DeleteFromMenu()
{
    DeleteToolWithConfirm();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToggleExcludeState(quint32 id)
{
    const VPiece oldDet = VAbstractTool::data.GetPiece(m_id);

    {
        VPiece newDet = oldDet;
        for (int i = 0; i < oldDet.GetPath().CountNodes(); ++i)
        {
            if (VPieceNode node = oldDet.GetPath().at(i); node.GetId() == id && node.GetTypeTool() == Tool::NodePoint)
            {
                node.SetExcluded(!node.IsExcluded());
                newDet.GetPath()[i] = node;

                VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, doc, m_id));
                return;
            }
        }
    }

    const QVector<quint32> paths = oldDet.GetInternalPaths();
    for (auto pathId : paths)
    {
        const VPiecePath oldPath = VAbstractTool::data.GetPiecePath(pathId);
        VPiecePath newPath = oldPath;
        for (int i = 0; i < newPath.CountNodes(); ++i)
        {
            if (VPieceNode node = newPath.at(i); node.GetId() == id && node.GetTypeTool() == Tool::NodePoint)
            {
                node.SetExcluded(!node.IsExcluded());
                newPath[i] = node;

                VAbstractApplication::VApp()->getUndoStack()->push(
                    new SavePiecePathOptions(m_id, oldPath, newPath, doc, pathId));
                return;
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToggleTurnPointState(quint32 id)
{
    const VPiece oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;

    for (int i = 0; i < oldDet.GetPath().CountNodes(); ++i)
    {
        VPieceNode node = oldDet.GetPath().at(i);
        if (node.GetId() == id && node.GetTypeTool() == Tool::NodePoint)
        {
            node.SetTurnPoint(not node.IsTurnPoint());
            newDet.GetPath()[i] = node;

            VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, doc, m_id));
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToggleNodePointAngleType(quint32 id, PieceNodeAngle type)
{
    const VPiece oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;

    for (int i = 0; i < oldDet.GetPath().CountNodes(); ++i)
    {
        VPieceNode node = oldDet.GetPath().at(i);
        if (node.GetId() == id && node.GetTypeTool() == Tool::NodePoint)
        {
            node.SetAngleType(type);
            newDet.GetPath()[i] = node;

            VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, doc, m_id));
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToggleNodePointPassmark(quint32 id, bool toggle)
{
    const VPiece oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;

    for (int i = 0; i < oldDet.GetPath().CountNodes(); ++i)
    {
        VPieceNode node = oldDet.GetPath().at(i);
        if (node.GetId() == id && node.GetTypeTool() == Tool::NodePoint)
        {
            node.SetPassmark(toggle);
            newDet.GetPath()[i] = node;

            VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, doc, m_id));
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::TogglePassmarkAngleType(quint32 id, PassmarkAngleType type)
{
    const VPiece oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;

    for (int i = 0; i < oldDet.GetPath().CountNodes(); ++i)
    {
        VPieceNode node = oldDet.GetPath().at(i);
        if (node.GetId() == id && node.GetTypeTool() == Tool::NodePoint)
        {
            node.SetPassmarkAngleType(type);
            newDet.GetPath()[i] = node;

            VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, doc, m_id));
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::TogglePassmarkLineType(quint32 id, PassmarkLineType type)
{
    const VPiece oldDet = VAbstractTool::data.GetPiece(m_id);
    VPiece newDet = oldDet;

    for (int i = 0; i < oldDet.GetPath().CountNodes(); ++i)
    {
        VPieceNode node = oldDet.GetPath().at(i);
        if (node.GetId() == id && node.GetTypeTool() == Tool::NodePoint)
        {
            node.SetPassmarkLineType(type);
            newDet.GetPath()[i] = node;

            VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, doc, m_id));
            return;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::FindGrainlineGeometry(const VGrainlineData &geom, const QVector<quint32> &pins, qreal &length,
                                               qreal &rotationAngle, QPointF &pos) -> VPieceItem::MoveTypes
{
    const quint32 topPin = geom.TopPin();

    if (const quint32 bottomPin = geom.BottomPin();
        topPin != NULL_ID && pins.contains(topPin) && bottomPin != NULL_ID && pins.contains(bottomPin))
    {
        try
        {
            const auto topPinPoint = VAbstractTool::data.GeometricObject<VPointF>(topPin);
            const auto bottomPinPoint = VAbstractTool::data.GeometricObject<VPointF>(bottomPin);

            QLineF grainline(static_cast<QPointF>(*bottomPinPoint), static_cast<QPointF>(*topPinPoint));
            length = FromPixel(grainline.length(), *VDataTool::data.GetPatternUnit());
            rotationAngle = grainline.angle();

            if (not VFuzzyComparePossibleNulls(rotationAngle, 0))
            {
                grainline.setAngle(0);
            }

            pos = grainline.p1();

            return VPieceItem::NotMovable;
        }
        catch (const VExceptionBadId &)
        {
            // do nothing.
        }
    }

    VPieceItem::MoveTypes restrictions = VPieceItem::AllModifications;
    try
    {
        if (not qmu::QmuTokenParser::IsSingle(geom.GetRotation()))
        {
            restrictions &= ~VPieceItem::IsRotatable;
        }

        Calculator cal1;
        rotationAngle = cal1.EvalFormula(VAbstractTool::data.DataVariables(), geom.GetRotation());

        if (not qmu::QmuTokenParser::IsSingle(geom.GetLength()))
        {
            restrictions &= ~VPieceItem::IsResizable;
        }

        Calculator cal2;
        length = cal2.EvalFormula(VAbstractTool::data.DataVariables(), geom.GetLength());
    }
    catch (qmu::QmuParserError &e)
    {
        Q_UNUSED(e);
        return VPieceItem::Error;
    }

    if (const quint32 centerPin = geom.CenterPin(); centerPin != NULL_ID && pins.contains(centerPin))
    {
        try
        {
            const auto centerPinPoint = VAbstractTool::data.GeometricObject<VPointF>(centerPin);

            const qreal cLength = ToPixel(length, *VDataTool::data.GetPatternUnit());
            QLineF grainline(centerPinPoint->x(), centerPinPoint->y(), centerPinPoint->x() - cLength / 2.0,
                             centerPinPoint->y());

            grainline.setAngle(rotationAngle);
            Swap(grainline);
            grainline.setLength(cLength);

            pos = grainline.p2();
            restrictions &= ~VPieceItem::IsMovable;
        }
        catch (const VExceptionBadId &)
        {
            pos = geom.GetPos();
        }
    }
    else
    {
        pos = geom.GetPos();
    }

    return restrictions;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::InitNodes(const VPiece &detail, VMainGraphicsScene *scene)
{
    const VPiecePath &path = detail.GetPath();
    for (int i = 0; i < path.CountNodes(); ++i)
    {
        InitNode(path.at(i), scene, this);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::InitNode(const VPieceNode &node, VMainGraphicsScene *scene, VToolSeamAllowance *parent)
{
    SCASSERT(scene != nullptr)
    SCASSERT(parent != nullptr)

    switch (node.GetTypeTool())
    {
        case Tool::NodePoint:
        {
            auto *tool = qobject_cast<VNodePoint *>(VAbstractPattern::getTool(node.GetId()));
            SCASSERT(tool != nullptr);

            if (tool->parent() != parent)
            {
                connect(tool, &VNodePoint::ShowOptions, parent, &VToolSeamAllowance::ShowOptions, Qt::UniqueConnection);
                connect(tool, &VNodePoint::ToggleInLayout, parent, &VToolSeamAllowance::ToggleInLayout,
                        Qt::UniqueConnection);
                connect(tool, &VNodePoint::ToggleForbidFlipping, parent, &VToolSeamAllowance::ToggleForbidFlipping,
                        Qt::UniqueConnection);
                connect(tool, &VNodePoint::ToggleForceFlipping, parent, &VToolSeamAllowance::ToggleForceFlipping,
                        Qt::UniqueConnection);
                connect(tool, &VNodePoint::ToggleShowFullPiece, parent, &VToolSeamAllowance::ToggleShowFullPiece,
                        Qt::UniqueConnection);
                connect(tool, &VNodePoint::Delete, parent, &VToolSeamAllowance::DeleteFromMenu, Qt::UniqueConnection);
                connect(tool, &VNodePoint::ToggleExcludeState, parent, &VToolSeamAllowance::ToggleExcludeState,
                        Qt::UniqueConnection);
                connect(tool, &VNodePoint::ToggleTurnPointState, parent, &VToolSeamAllowance::ToggleTurnPointState,
                        Qt::UniqueConnection);
                connect(tool, &VNodePoint::ToggleSeamAllowanceAngleType, parent,
                        &VToolSeamAllowance::ToggleNodePointAngleType, Qt::UniqueConnection);
                connect(tool, &VNodePoint::TogglePassmark, parent, &VToolSeamAllowance::ToggleNodePointPassmark,
                        Qt::UniqueConnection);
                connect(tool, &VNodePoint::ChoosedTool, scene, &VMainGraphicsScene::ChoosedItem, Qt::UniqueConnection);
                connect(tool, &VNodePoint::TogglePassmarkAngleType, parent,
                        &VToolSeamAllowance::TogglePassmarkAngleType, Qt::UniqueConnection);
                connect(tool, &VNodePoint::TogglePassmarkLineType, parent, &VToolSeamAllowance::TogglePassmarkLineType,
                        Qt::UniqueConnection);
                tool->setParentItem(parent);
                tool->SetParentType(ParentType::Item);
                tool->SetExluded(node.IsExcluded());
            }
            tool->setVisible(not node.IsExcluded()); // Hide excluded point
            break;
        }
        case Tool::NodeArc:
        case Tool::NodeElArc:
        case Tool::NodeSpline:
        case Tool::NodeSplinePath:
            // Do nothing
            break;
        default:
            qDebug() << "Get wrong tool type. Ignore.";
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::InitInternalNode(const VPieceNode &node, VToolSeamAllowance *parent)
{
    SCASSERT(parent != nullptr)

    switch (node.GetTypeTool())
    {
        case Tool::NodePoint:
        {
            auto *tool = qobject_cast<VNodePoint *>(VAbstractPattern::getTool(node.GetId()));
            SCASSERT(tool != nullptr);

            if (tool->parent() != parent)
            {
                connect(tool, &VNodePoint::ShowOptions, parent, &VToolSeamAllowance::ShowOptions, Qt::UniqueConnection);
                connect(tool,
                        &VNodePoint::ToggleInLayout,
                        parent,
                        &VToolSeamAllowance::ToggleInLayout,
                        Qt::UniqueConnection);
                connect(tool,
                        &VNodePoint::ToggleForbidFlipping,
                        parent,
                        &VToolSeamAllowance::ToggleForbidFlipping,
                        Qt::UniqueConnection);
                connect(tool,
                        &VNodePoint::ToggleForceFlipping,
                        parent,
                        &VToolSeamAllowance::ToggleForceFlipping,
                        Qt::UniqueConnection);
                connect(tool,
                        &VNodePoint::ToggleShowFullPiece,
                        parent,
                        &VToolSeamAllowance::ToggleShowFullPiece,
                        Qt::UniqueConnection);
                connect(tool, &VNodePoint::Delete, parent, &VToolSeamAllowance::DeleteFromMenu, Qt::UniqueConnection);
                connect(tool,
                        &VNodePoint::ToggleExcludeState,
                        parent,
                        &VToolSeamAllowance::ToggleExcludeState,
                        Qt::UniqueConnection);

                tool->setParentItem(parent);
                tool->SetParentType(ParentType::Item);
                tool->SetExluded(node.IsExcluded());
            }
            tool->SetInternalPathNode(true);
            tool->setVisible(not node.IsExcluded()); // Hide excluded point
            break;
        }
        case Tool::NodeArc:
        case Tool::NodeElArc:
        case Tool::NodeSpline:
        case Tool::NodeSplinePath:
            // Do nothing
            break;
        default:
            qDebug() << "Get wrong tool type. Ignore.";
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::InitInternalPaths(const VPiece &detail)
{
    const QVector<quint32> paths = detail.GetInternalPaths();
    for (auto pathId : paths)
    {
        auto *tool = qobject_cast<VToolPiecePath *>(VAbstractPattern::getTool(pathId));
        SCASSERT(tool != nullptr);

        if (tool->parent() != this)
        {
            tool->setParentItem(this);
            tool->SetParentType(ParentType::Item);
        }
        tool->show();

        const VPiecePath pathItem = VAbstractTool::data.GetPiecePath(pathId);
        for (int i = 0; i < pathItem.CountNodes(); ++i)
        {
            InitInternalNode(pathItem.at(i), this);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::DeleteToolWithConfirm(bool ask)
{
    if (ask && ConfirmDeletion() == QMessageBox::No)
    {
        return;
    }

    auto delDet = std::make_unique<DeletePiece>(doc, m_id, VAbstractTool::data, m_sceneDetails);
    VAbstractApplication::VApp()->getUndoStack()->push(delDet.release());

    // Throw exception, this will help prevent case when we forget to immediately quit function.
    throw VExceptionToolWasDeleted(tr("Tool was used after deleting."));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::ToolCreation(const Source &typeCreation)
{
    if (typeCreation == Source::FromGui || typeCreation == Source::FromTool)
    {
        VToolSeamAllowance::AddToFile();
    }
    else
    {
        VToolSeamAllowance::RefreshDataInFile();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::SelectedTools() const -> QList<VToolSeamAllowance *>
{
    QList<VToolSeamAllowance *> tools;
    if (m_sceneDetails)
    {
        if (const QList<QGraphicsItem *> list = m_sceneDetails->selectedItems(); not list.isEmpty())
        {
            tools.reserve(list.size());
            for (auto *item : list)
            {
                auto *tool = qgraphicsitem_cast<VToolSeamAllowance *>(item);
                if (tool != nullptr && tool->getId() != m_id)
                {
                    tools.append(tool);
                }
            }
        }
    }

    return tools;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::IsGrainlinePositionValid() const -> bool
{
    VPieceGrainline const grainLine = m_grainLine->Grainline();
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    QVector<QPointF> contourPoints;
    detail.IsSeamAllowance() && not detail.IsSeamAllowanceBuiltIn()
        ? CastTo(detail.FullSeamAllowancePoints(getData()), contourPoints)
        : CastTo(detail.FullMainPathPoints(getData()), contourPoints);

    return grainLine.IsPositionValid(contourPoints);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::IsFoldLinePositionValid(const QVector<QPainterPath> &shape, FoldLineType type) const -> bool
{
    if (type == FoldLineType::None || shape.isEmpty())
    {
        return true;
    }

    QPainterPath foldLinePath;
    foldLinePath.addPath(shape.constFirst());

    if (shape.size() > 1)
    {
        foldLinePath.addPath(shape.constLast());
    }

    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    QVector<QPointF> contourPoints;
    detail.IsSeamAllowance() && not detail.IsSeamAllowanceBuiltIn()
        ? CastTo(detail.FullSeamAllowancePoints(getData()), contourPoints)
        : CastTo(detail.FullMainPathPoints(getData()), contourPoints);

    const QPainterPath contourPath = VGObject::PainterPath(contourPoints);
    return contourPath.contains(foldLinePath);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::AddPointRecords(VAbstractPattern *doc, QDomElement &domElement,
                                         const QVector<quint32> &records, const QString &tag)
{
    if (not records.empty())
    {
        QDomElement pinsElement = doc->createElement(tag);
        for (auto record : records)
        {
            pinsElement.appendChild(
                doc->CreateElementWithText(VToolSeamAllowance::TagRecord, QString().setNum(record)));
        }
        domElement.appendChild(pinsElement);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::DuplicateNode(const VPieceNode &node, const VToolSeamAllowanceInitData &initData) -> quint32
{
    SCASSERT(initData.scene != nullptr)
    SCASSERT(initData.doc != nullptr)
    SCASSERT(initData.data != nullptr)

    const QSharedPointer<VGObject> gobj = initData.data->GetGObject(node.GetId());
    auto *tool = qobject_cast<VAbstractNode *>(VAbstractPattern::getTool(node.GetId()));
    SCASSERT(tool != nullptr)

    VAbstractNodeInitData initNodeData;
    initNodeData.idObject = gobj->getIdObject();
    initNodeData.doc = initData.doc;
    initNodeData.data = initData.data;
    initNodeData.parse = Document::FullParse;
    initNodeData.typeCreation = Source::FromGui;
    initNodeData.scene = initData.scene;
    initNodeData.drawName = initData.drawName;
    initNodeData.idTool = tool->GetIdTool();

    switch (node.GetTypeTool())
    {
        case Tool::NodePoint:
        {
            auto point = QSharedPointer<VPointF>(new VPointF(*qSharedPointerDynamicCast<VPointF>(gobj).data()));
            initNodeData.id = VAbstractTool::CreateNodePoint(initData.data, gobj->getIdObject(), point);
            VNodePoint::Create(initNodeData);
            break;
        }
        case Tool::NodeArc:
            initNodeData.id = VAbstractTool::CreateNode<VArc>(initData.data, gobj->getIdObject());
            VNodeArc::Create(initNodeData);
            break;
        case Tool::NodeElArc:
            initNodeData.id = VAbstractTool::CreateNode<VEllipticalArc>(initData.data, gobj->getIdObject());
            VNodeEllipticalArc::Create(initNodeData);
            break;
        case Tool::NodeSpline:
            initNodeData.id = VAbstractTool::CreateNodeSpline(initData.data, gobj->getIdObject());
            VNodeSpline::Create(initNodeData);
            break;
        case Tool::NodeSplinePath:
            initNodeData.id = VAbstractTool::CreateNodeSplinePath(initData.data, gobj->getIdObject());
            VNodeSplinePath::Create(initNodeData);
            break;
        default:
            qDebug() << "May be wrong tool type!!! Ignoring." << Q_FUNC_INFO;
            break;
    }
    return initNodeData.id;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::DuplicatePiecePath(quint32 id, const VToolSeamAllowanceInitData &initData) -> quint32
{
    const VPiecePath path = initData.data->GetPiecePath(id);
    VPiecePath newPath = path;
    QMap<quint32, quint32> recordReplacements; // Not used
    newPath.SetNodes(DuplicateNodes(path, initData, recordReplacements));

    const quint32 idPath = initData.data->AddPiecePath(newPath);

    auto *tool = qobject_cast<VAbstractNode *>(VAbstractPattern::getTool(id));
    SCASSERT(tool != nullptr)

    VToolPiecePathInitData initNodeData;
    initNodeData.id = idPath;
    initNodeData.idObject = NULL_ID; // piece id
    initNodeData.scene = initData.scene;
    initNodeData.doc = initData.doc;
    initNodeData.data = initData.data;
    initNodeData.parse = Document::FullParse;
    initNodeData.typeCreation = Source::FromTool;
    initNodeData.drawName = initData.drawName;
    initNodeData.idTool = tool->GetIdTool();
    initNodeData.path = newPath;

    VToolPiecePath::Create(initNodeData);
    return idPath;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::DuplicateCustomSARecords(const QVector<CustomSARecord> &records,
                                                  const VToolSeamAllowanceInitData &initData,
                                                  const QMap<quint32, quint32> &replacements) -> QVector<CustomSARecord>
{
    QVector<CustomSARecord> newRecords;
    newRecords.reserve(records.size());
    for (auto record : records)
    {
        record.path = DuplicatePiecePath(record.path, initData);
        record.startPoint = replacements.value(record.startPoint, NULL_ID);
        record.endPoint = replacements.value(record.endPoint, NULL_ID);
        newRecords.append(record);
    }
    return newRecords;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::DuplicateInternalPaths(const QVector<quint32> &iPaths,
                                                const VToolSeamAllowanceInitData &initData) -> QVector<quint32>
{
    QVector<quint32> newPaths;
    newPaths.reserve(iPaths.size());
    for (auto iPath : iPaths)
    {
        newPaths.append(DuplicatePiecePath(iPath, initData));
    }
    return newPaths;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::DuplicatePlaceLabels(const QVector<quint32> &placeLabels,
                                              const VToolSeamAllowanceInitData &initData) -> QVector<quint32>
{
    QVector<quint32> newPlaceLabels;
    newPlaceLabels.reserve(placeLabels.size());
    for (auto placeLabel : placeLabels)
    {
        QSharedPointer<VPlaceLabelItem> const label = initData.data->GeometricObject<VPlaceLabelItem>(placeLabel);
        auto *tool = qobject_cast<VAbstractNode *>(VAbstractPattern::getTool(placeLabel));
        SCASSERT(tool != nullptr)

        VToolPlaceLabelInitData initNodeData;
        initNodeData.idObject = NULL_ID; // piece id
        initNodeData.doc = initData.doc;
        initNodeData.data = initData.data;
        initNodeData.parse = Document::FullParse;
        initNodeData.typeCreation = Source::FromTool;
        initNodeData.drawName = initData.drawName;
        initNodeData.width = label->GetWidthFormula();
        initNodeData.height = label->GetHeightFormula();
        initNodeData.angle = label->GetAngleFormula();
        initNodeData.visibilityTrigger = label->GetVisibilityTrigger();
        initNodeData.type = label->GetLabelType();
        initNodeData.centerPoint = label->GetCenterPoint();
        initNodeData.notMirrored = label->IsNotMirrored();
        initNodeData.id = initNodeData.data->AddGObject(new VPlaceLabelItem(*label));
        initNodeData.idTool = tool->GetIdTool();

        VToolPlaceLabel::Create(initNodeData);
        newPlaceLabels.append(initNodeData.id);
    }
    return newPlaceLabels;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::RefreshFoldLine()
{
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);
    UpdateFoldLine(RenderFoldLine(detail, getData()));

    // m_foldLineLabel's brush (filled glyph vs. stroke-only path) depends on the same
    // single-stroke/single-line font settings and is otherwise only refreshed by the full
    // geometry pipeline.
    RefreshScale();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSeamAllowance::UpdateFoldLine(const VFoldLine &foldLine)
{
    const VPiece detail = VAbstractTool::data.GetPiece(m_id);

    if (detail.GetFoldLineType() == FoldLineType::None || (detail.IsShowFullPiece() && !detail.IsShowMirrorLine()))
    {
        m_foldLineLabelText->setVisible(false);
        m_foldLineMark->setPath(QPainterPath());
        m_foldLineLabel->setPath(QPainterPath());
        return;
    }

    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();

    if (settings->GetSingleStrokeOutlineFont() || settings->GetSingleLineFonts())
    {
        m_foldLineLabelText->setVisible(false);
    }
    else
    {
        foldLine.UpdateFoldLineLabel(m_foldLineLabelText);
    }

    QVector<QPainterPath> const shape = foldLine.FoldLinePath();

    if (detail.GetFoldLineType() == FoldLineType::ThreeDots || detail.GetFoldLineType() == FoldLineType::ThreeX ||
        detail.GetFoldLineType() == FoldLineType::TwoArrows)
    {
        m_foldLineMark->setPath(!shape.isEmpty() ? shape.constFirst() : QPainterPath());
        m_foldLineLabel->setPath(QPainterPath());
    }
    else if (detail.GetFoldLineType() == FoldLineType::Text)
    {
        m_foldLineMark->setPath(QPainterPath());
        if (!shape.isEmpty() && (settings->GetSingleStrokeOutlineFont() || settings->GetSingleLineFonts()))
        {
            m_foldLineLabel->setPath(shape.constFirst());
        }
        else
        {
            m_foldLineLabel->setPath(QPainterPath());
        }
    }
    else
    {
        m_foldLineMark->setPath(!shape.isEmpty() ? shape.constFirst() : QPainterPath());

        if (shape.size() > 1 && (settings->GetSingleStrokeOutlineFont() || settings->GetSingleLineFonts()))
        {
            m_foldLineLabel->setPath(shape.constLast());
        }
        else
        {
            m_foldLineLabel->setPath(QPainterPath());
        }
    }

    if (m_geometryIsReady && not IsFoldLinePositionValid(shape, detail.GetFoldLineType()))
    {
        const QString errorMsg = QObject::tr("Piece '%1'. Fold line is not valid.").arg(detail.GetName());
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VException(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSeamAllowance::DuplicateNodes(const VPiecePath &path, const VToolSeamAllowanceInitData &initData,
                                        QMap<quint32, quint32> &replacements) -> QVector<VPieceNode>
{
    QVector<VPieceNode> nodes;
    nodes.reserve(path.CountNodes());
    for (int i = 0; i < path.CountNodes(); ++i)
    {
        VPieceNode nodeD = path.at(i);
        const quint32 oldId = nodeD.GetId();
        const quint32 id = DuplicateNode(nodeD, initData);
        if (id > NULL_ID)
        {
            nodeD.SetId(id);
            nodes.append(nodeD);
            replacements.insert(oldId, id);
        }
    }
    return nodes;
}
