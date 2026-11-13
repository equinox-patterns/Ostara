/************************************************************************
 **
 **  @file   vtoolsplinepath.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 17/10/2026.
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

#include "vtoolsplinepath.h"

#include <QDomElement>
#include <QEvent>
#include <QFlags>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QList>
#include <QPen>
#include <QPoint>
#include <QRectF>
#include <QSet>
#include <QSharedPointer>
#include <QUndoStack>
#include <QtMath>

#include "../../../dialogs/tools/dialogsplinepath.h"
#include "../../../dialogs/tools/dialogtool.h"
#include "../../../undocommands/movesplinepath.h"
#include "../../../undocommands/vundocommand.h"
#include "../../../visualization/path/vistoolsplinepath.h"
#include "../../../visualization/visualization.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vdomdocument.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../qmuparser/qmutokenparser.h"
#include "../vgeometry/vabstractcubicbezierpath.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepoint.h"
#include "vexception.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/../vgeometry/vsplinepath.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vcontrolpointspline.h"
#include "../vwidgets/vsimplecurve.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../operation/ocopypieces.h"
#include "osplinepathportions.h"
#include "vtoolabstractcurve.h"

const QString VToolSplinePath::ToolType = QStringLiteral("pathInteractive");
const QString VToolSplinePath::OldToolType = QStringLiteral("path");

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VToolSplinePath constructor.
 * @param initData init data.
 * @param parent parent object.
 */
VToolSplinePath::VToolSplinePath(const VToolSplinePathInitData &initData, QGraphicsItem *parent)
  : VToolAbstractBezier(initData.doc, initData.data, initData.id, initData.notes, parent)
{
    SetSceneType(SceneObject::SplinePath);

    this->setFlag(QGraphicsItem::ItemIsMovable, true);
    this->setFlag(QGraphicsItem::ItemIsFocusable, true); // For keyboard input focus

    const QSharedPointer<VSplinePath> splPath = initData.data->GeometricObject<VSplinePath>(initData.id);
    InitControlPoints(splPath.data());
    ToolCreation(initData.typeCreation);

    m_patternData = initData.data;

    m_cutNodes = initData.cutNodes;
    m_portionIds = initData.portionIds;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setDialog set dialog when user want change tool option.
 */
void VToolSplinePath::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogSplinePath> dialogTool = qobject_cast<DialogSplinePath *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    const QSharedPointer<VSplinePath> splPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetPath(*splPath);
    dialogTool->SetNotes(m_notes);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create help create tool from GUI.
 * @param dialog dialog.
 * @param scene pointer to scene.
 * @param doc dom document container.
 * @param data container with variables.
 */
auto VToolSplinePath::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                             VContainer *data) -> VToolSplinePath *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogSplinePath> dialogTool = qobject_cast<DialogSplinePath *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolSplinePathInitData initData;
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();

    auto path = QSharedPointer<VSplinePath>::create(dialogTool->GetPath());

    initData.points.reserve(path->CountPoints());
    for (qint32 i = 0; i < path->CountPoints(); ++i)
    {
        VPointF const point = (*path)[i].P();
        initData.points.append(point.id());
    }

    VToolSplinePath *spl = Create(initData, path);
    if (spl != nullptr)
    {
        spl->m_dialog = dialog;
    }
    return spl;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create help create tool.
 * @param initData init data.
 * @param path spline path.
 */
auto VToolSplinePath::Create(VToolSplinePathInitData &initData, const QSharedPointer<VSplinePath> &path)
    -> VToolSplinePath *
{
    SCASSERT(not path.isNull())
    // The container updates the object it already holds when the id is known, and does not adopt what it is
    // given. The caller's share — the one handed in here — is what keeps the path alive on that branch, so the
    // signature says it: pass a QSharedPointer, never a bare new.
    const QSharedPointer<VSplinePath> curve = path;
    const bool curveUnique = initData.data->IsUnique(curve->name());
    const QString curveName = curve->name();

    // An axis only fills the angles of its anchor point: what is stored stays a plain number, which is what
    // keeps the file readable and every other reader of an angle working. Done before the object is published,
    // so the very first geometry already follows the axis.
    for (vsizetype i = 0; i < curve->CountPoints(); ++i)
    {
        VToolAbstractCurve::ApplyAxes((*curve)[i], initData.data);
    }

    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->AddGObject(curve);
    }
    else
    {
        initData.data->UpdateGObject(initData.id, curve);
    }

    // Re-parsing recreates the same objects with the same names, so a duplicate here is a false positive.
    if (!curveUnique && (initData.typeCreation != Source::FromFile || initData.parse == Document::FullParse))
    {
        const QString errorMsg
            = QObject::tr("Curve '%1' with id %2 has not unique name.").arg(curveName).arg(initData.id);
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VException(errorMsg)
            : qWarning() << errorMsg; // internal invariant: the log is enough
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();

    for (int i = 0; i < initData.points.size(); ++i)
    {
        initData.doc->FindFormulaDependencies(initData.a1.value(i), initData.id, varData);
        initData.doc->FindFormulaDependencies(initData.a2.value(i), initData.id, varData);
        initData.doc->FindFormulaDependencies(initData.l1.value(i), initData.id, varData);
        initData.doc->FindFormulaDependencies(initData.l2.value(i), initData.id, varData);
    }

    initData.data->AddCurveWithSegments(initData.data->GeometricObject<VAbstractCubicBezierPath>(initData.id),
                                        initData.id);

    for (const auto &pId : std::as_const(initData.points))
    {
        patternGraph->AddEdge(pId, initData.id);
    }

    // The axis points are dependencies of the curve, exactly as the anchor points are: declared here, on the
    // path both the reader and the interface take, so the curve follows a point of another block and cannot
    // outlive one of its axis points.
    for (vsizetype i = 0; i < curve->CountPoints(); ++i)
    {
        if (const OAxis axis = curve->at(i).Axis(); axis.IsValid())
        {
            patternGraph->AddEdge(axis.P1(), initData.id);
            patternGraph->AddEdge(axis.P2(), initData.id);
        }
    }

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::SplinePath, initData.doc);
        auto *spl = new VToolSplinePath(initData);
        initData.scene->addItem(spl);
        InitSplinePathToolConnections(initData.scene, spl);
        VAbstractPattern::AddTool(initData.id, spl);
        // The runs need the tool's scene, so they are built once it is in one: before that, the items would
        // exist without being drawn by anything.
        spl->RebuildPortions();
        return spl;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSplinePath::Create(VToolSplinePathInitData &initData) -> VToolSplinePath *
{
    auto path = QSharedPointer<VSplinePath>::create();

    if (initData.duplicate > 0)
    {
        path->SetDuplicate(initData.duplicate);
    }

    for (int i = 0; i < initData.points.size(); ++i)
    {
        const qreal calcAngle1 = CheckFormula(initData.id, initData.a1[i], initData.data);
        const qreal calcAngle2 = CheckFormula(initData.id, initData.a2[i], initData.data);

        const qreal calcLength1 =
            VAbstractValApplication::VApp()->toPixel(CheckFormula(initData.id, initData.l1[i], initData.data));
        const qreal calcLength2 =
            VAbstractValApplication::VApp()->toPixel(CheckFormula(initData.id, initData.l2[i], initData.data));

        const auto p = *initData.data->GeometricObject<VPointF>(initData.points.at(i));

        VSplinePoint point(p, calcAngle1, initData.a1.at(i), calcAngle2, initData.a2.at(i), calcLength1,
                           initData.l1.at(i), calcLength2, initData.l2.at(i));
        point.SetAxis(initData.axes.value(i));
        path->append(point);
    }

    path->SetColor(initData.color);
    path->SetPenStyle(initData.penStyle);
    path->SetApproximationScale(initData.approximationScale);
    path->SetAliasSuffix(initData.aliasSuffix);

    return VToolSplinePath::Create(initData, path);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ControlPointChangePosition handle change position control point.
 * @param indexSpline position spline in spline list.
 * @param position position point in spline.
 * @param pos new position.
 */
void VToolSplinePath::ControlPointChangePosition(const qint32 &indexSpline, const SplinePointPosition &position,
                                                 const QPointF &pos)
{
    const QSharedPointer<VSplinePath> oldSplPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);

    if (VAbstractApplication::VApp()->Settings()->IsFreeCurveMode() && not moved)
    {
        oldMoveSplinePath = QSharedPointer<VSplinePath>::create(*oldSplPath);
        moved = true;
    }

    auto newSplPath = QSharedPointer<VSplinePath>::create(*oldSplPath);
    const VSpline spl = CorrectedSpline(newSplPath->GetSpline(indexSpline), position, pos);

    UpdateControlPoints(spl, newSplPath, indexSpline);

    if (not VAbstractApplication::VApp()->Settings()->IsFreeCurveMode())
    {
        UndoCommandMove(*oldSplPath, *newSplPath);
    }
    else
    {
        newMoveSplinePath = newSplPath;
        VAbstractTool::data.UpdateGObject(m_id, newMoveSplinePath);
        RefreshGeometry();

        if (QGraphicsScene *sc = scene())
        {
            VMainGraphicsView::NewSceneRect(sc, VAbstractValApplication::VApp()->getSceneView(), this);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::EnableToolMove(bool move)
{
    this->setFlag(QGraphicsItem::ItemIsMovable, move);

    for (auto *point : std::as_const(controlPoints))
    {
        point->setFlag(QGraphicsItem::ItemIsMovable, move);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::AllowHover(bool enabled)
{
    m_acceptHover = enabled;
    VToolAbstractBezier::AllowHover(enabled);

    // Hover belongs to the hover question and to nothing else: mixing it into AllowSelecting turned it off the
    // moment a tool started picking objects, and the pieces stopped answering the mouse.
    for (VSimpleCurve *item : std::as_const(m_portionItems))
    {
        if (item != nullptr && not m_cutNodes.isEmpty())
        {
            item->setAcceptHoverEvents(enabled);
        }
    }

    for (auto *point : std::as_const(controlPoints))
    {
        point->setAcceptHoverEvents(enabled);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::AllowSelecting(bool enabled)
{
    VToolAbstractBezier::AllowSelecting(enabled);

    for (auto *point : std::as_const(controlPoints))
    {
        point->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
    }

    // The pieces are objects of the pattern: the tools that pick objects must be able to make them selectable,
    // exactly as they do for the curve they were taken from.
    for (VSimpleCurve *item : std::as_const(m_portionItems))
    {
        if (item != nullptr)
        {
            item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    Q_UNUSED(id)
    try
    {
        ContextMenu<DialogSplinePath>(event);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::CurveSelected(bool selected)
{
    setSelected(selected);

    for (auto *point : std::as_const(controlPoints))
    {
        const QSignalBlocker blocker(point);
        point->setSelected(selected);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdateControlPoints update position control points in file.
 * @param spl spline that was changed.
 * @param splPath spline path.
 * @param indexSpline index spline in spline path.
 */
void VToolSplinePath::UpdateControlPoints(const VSpline &spl, QSharedPointer<VSplinePath> &splPath,
                                          qint32 indexSpline) const
{
    VSplinePoint p = splPath->GetSplinePoint(indexSpline, SplinePointPosition::FirstPoint);

    if (indexSpline > 1)
    {
        VSpline const prevSpline = splPath->GetSpline(indexSpline - 1);
        if (qmu::QmuTokenParser::IsSingle(prevSpline.GetEndAngleFormula()))
        {
            p.SetAngle2(spl.GetStartAngle(), spl.GetStartAngleFormula());
        }
    }
    else
    {
        p.SetAngle2(spl.GetStartAngle(), spl.GetStartAngleFormula());
    }

    p.SetLength2(spl.GetC1Length(), spl.GetC1LengthFormula());
    splPath->UpdatePoint(indexSpline, SplinePointPosition::FirstPoint, p);

    p = splPath->GetSplinePoint(indexSpline, SplinePointPosition::LastPoint);

    if (indexSpline < splPath->CountSubSpl())
    {
        VSpline const nextSpline = splPath->GetSpline(indexSpline + 1);
        if (qmu::QmuTokenParser::IsSingle(nextSpline.GetStartAngleFormula()))
        {
            p.SetAngle1(spl.GetEndAngle(), spl.GetEndAngleFormula());
        }
    }
    else
    {
        p.SetAngle1(spl.GetEndAngle(), spl.GetEndAngleFormula());
    }

    p.SetLength1(spl.GetC2Length(), spl.GetC2LengthFormula());
    splPath->UpdatePoint(indexSpline, SplinePointPosition::LastPoint, p);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::SetSplinePathAttributes(QDomElement &domElement, const VSplinePath &path)
{
    doc->SetAttribute(domElement, AttrType, ToolType);
    doc->SetAttributeOrRemoveIf<quint32>(domElement, AttrDuplicate, path.GetDuplicate(),
                                         [](quint32 duplicate) noexcept { return duplicate == 0; });

    if (domElement.hasAttribute(AttrKCurve))
    {
        domElement.removeAttribute(AttrKCurve);
    }

    doc->SetAttribute(domElement, AttrColor, path.GetColor());
    doc->SetAttribute(domElement, AttrPenStyle, path.GetPenStyle());
    doc->SetAttribute(domElement, AttrAScale, path.GetApproximationScale());
    doc->SetAttributeOrRemoveIf<QString>(domElement, AttrAlias, path.GetAliasSuffix(),
                                         [](const QString &suffix) noexcept { return suffix.isEmpty(); });

    UpdatePathPoints(doc, domElement, path);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::UndoCommandMove(const VSplinePath &oldPath, const VSplinePath &newPath)
{
    auto *moveSplPath = new MoveSplinePath(doc, oldPath, newPath, m_id);
    connect(moveSplPath, &VUndoCommand::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    VAbstractApplication::VApp()->getUndoStack()->push(moveSplPath);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::InitControlPoints(const VSplinePath *splPath)
{
    auto InitControlPoint = [this](VControlPointSpline *cPoint)
    {
        connect(cPoint, &VControlPointSpline::ControlPointChangePosition, this,
                &VToolSplinePath::ControlPointChangePosition);
        connect(this, &VToolSplinePath::setEnabledPoint, cPoint, &VControlPointSpline::setEnabledPoint);
        connect(cPoint, &VControlPointSpline::ShowContextMenu, this, &VToolSplinePath::contextMenuEvent);
        connect(cPoint, &VControlPointSpline::Released, this, &VToolSplinePath::CurveReleased);
        connect(cPoint, &VControlPointSpline::Selected, this, &VToolSplinePath::CurveSelected);
        controlPoints.append(cPoint);
    };

    for (qint32 i = 1; i <= splPath->CountSubSpl(); ++i)
    {
        const VSpline spl = splPath->GetSpline(i);

        bool freeAngle1 = true;

        if (i > 1)
        {
            const VSpline prevSpl = splPath->GetSpline(i - 1);
            freeAngle1 = spl.IsStartAngleFree() && prevSpl.IsEndAngleFree();
        }
        else
        {
            freeAngle1 = spl.IsStartAngleFree();
        }

        const bool freeLength1 = qmu::QmuTokenParser::IsSingle(spl.GetC1LengthFormula());

        auto *controlPoint = new VControlPointSpline(i, SplinePointPosition::FirstPoint,
                                                     static_cast<QPointF>(spl.GetP2()), freeAngle1, freeLength1, this);
        InitControlPoint(controlPoint);

        bool freeAngle2 = true;

        if (i < splPath->CountSubSpl())
        {
            const VSpline nextSpl = splPath->GetSpline(i + 1);
            freeAngle2 = nextSpl.IsStartAngleFree() && spl.IsEndAngleFree();
        }
        else
        {
            freeAngle2 = spl.IsEndAngleFree();
        }

        const bool freeLength2 = qmu::QmuTokenParser::IsSingle(spl.GetC2LengthFormula());

        controlPoint = new VControlPointSpline(i, SplinePointPosition::LastPoint, static_cast<QPointF>(spl.GetP3()),
                                               freeAngle2, freeLength2, this);
        InitControlPoint(controlPoint);
    }

    VToolSplinePath::RefreshCtrlPoints();
    VToolSplinePath::ShowHandles(IsDetailsMode());
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSplinePath::GatherToolChanges() const -> VToolAbstractBezier::ToolChanges
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogSplinePath> dialogTool = qobject_cast<DialogSplinePath *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    const VSplinePath newCurve = dialogTool->GetPath();
    const auto oldCurve = VAbstractTool::data.GeometricObject<VAbstractCubicBezierPath>(m_id);

    return {.oldP1Label = oldCurve->FirstPoint().name(),
            .newP1Label = newCurve.FirstPoint().name(),
            .oldP4Label = oldCurve->LastPoint().name(),
            .newP4Label = newCurve.LastPoint().name(),
            .oldAliasSuffix = oldCurve->GetAliasSuffix(),
            .newAliasSuffix = newCurve.GetAliasSuffix(),
            .newDuplicate = newCurve.GetDuplicate()};
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdatePathPoints update spline path in pattern file.
 * @param doc dom document container.
 * @param element tag in file.
 * @param path spline path.
 */
void VToolSplinePath::UpdatePathPoints(VAbstractPattern *doc, QDomElement &element, const VSplinePath &path)
{
    VAbstractPattern::RemoveAllChildren(element);
    for (qint32 i = 0; i < path.CountPoints(); ++i)
    {
        AddPathPoint(doc, element, path.at(i));
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSplinePath::getSplinePath() const -> VSplinePath
{
    QSharedPointer<VSplinePath> const splPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);
    return *splPath.data();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::setSplinePath(const VSplinePath &splPath)
{
    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    QSharedPointer<VSplinePath> const splinePath = qSharedPointerDynamicCast<VSplinePath>(obj);
    *splinePath.data() = splPath;
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::PreviewPath(const VSplinePath &splPath)
{
    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    QSharedPointer<VSplinePath> const splinePath = qSharedPointerDynamicCast<VSplinePath>(obj);
    *splinePath.data() = splPath;

    // No SaveOption: the file keeps its own copy of the curve until the preview is validated, which is what
    // lets the user walk away from the axis window leaving nothing behind. RefreshGeometry() recomputes the
    // painter path and the handles from the container, and the scene rect follows the new shape — exactly what
    // every other change of geometry does.
    RefreshGeometry();

    if (QGraphicsScene *sc = scene())
    {
        VMainGraphicsView::NewSceneRect(sc, VAbstractValApplication::VApp()->getSceneView(), this);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSplinePath::AxisSlotCount() const -> int
{
    // One slot per anchor point: its two handles follow the same line, on either side of the point.
    return static_cast<int>(getSplinePath().CountPoints());
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSplinePath::AxisSlot(int slot) const -> OAxis
{
    const VSplinePath path = getSplinePath();
    const auto index = static_cast<vsizetype>(slot);
    return index >= 0 && index < path.CountPoints() ? path.at(index).Axis() : OAxis();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::PreviewAxis(int slot, const OAxis &axis)
{
    const VSplinePath path = getSplinePath();
    const auto index = static_cast<vsizetype>(slot);
    if (index < 0 || index >= path.CountPoints())
    {
        return;
    }

    if (axisPreviewStart.isNull())
    {
        axisPreviewStart = QSharedPointer<VSplinePath>::create(path);
    }

    VSplinePath previewed = path;
    previewed[index].SetAxis(axis);
    VToolAbstractCurve::ApplyAxes(previewed[index], getData());
    PreviewPath(previewed);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::CommitAxis()
{
    if (axisPreviewStart.isNull())
    {
        return;
    }
    axisPreviewStart.clear();

    // The file never heard of the preview, so the command it pushes holds the whole story: its old element is
    // the path as it was before, its new one what is now shown.
    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::RevertAxis()
{
    if (axisPreviewStart.isNull())
    {
        return;
    }
    const QSharedPointer<VSplinePath> start = axisPreviewStart;
    axisPreviewStart.clear();
    PreviewPath(*start);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::ShowVisualization(bool show)
{
    ShowToolVisualization<VisToolSplinePath>(show);
    HideShownPointLabels(show);
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief OpenOptionsDialog opens the same dialog the context menu does — the dock's "Edit curve" button and the
 *  right-click → Options have to lead to one and the same window. */
void VToolSplinePath::OpenOptionsDialog()
{
    ShowOptionsDialog<DialogSplinePath>();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::HideShownPointLabels(bool hide)
{
    const VSplinePath path = getSplinePath();

    if (hide)
    {
        // Remember which of our points currently have their name shown, so we can restore them.
        m_hiddenLabels.clear();
        for (qsizetype i = 0; i < path.CountPoints(); ++i)
        {
            const quint32 pointId = path.at(i).P().id();
            const auto point = VAbstractTool::data.GeometricObject<VPointF>(pointId);
            if (point->IsShowLabel())
            {
                m_hiddenLabels.append(pointId);
            }
        }
    }

    // Toggle the label on each matching point tool (their own name label, not our visu).
    const QList<QGraphicsItem *> items = scene()->items();
    for (auto *item : items)
    {
        auto *tool = qobject_cast<VAbstractTool *>(dynamic_cast<QObject *>(item));
        if (tool != nullptr && m_hiddenLabels.contains(tool->getId()))
        {
            tool->SetLabelVisible(tool->getId(), !hide);
        }
    }

    if (!hide)
    {
        m_hiddenLabels.clear();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::ShowHandles(bool show)
{
    for (auto *point : std::as_const(controlPoints))
    {
        point->setVisible(show);
    }
    VToolAbstractBezier::ShowHandles(show);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddPathPoint write path point to pattern file.
 * @param domElement dom element.
 * @param splPoint spline path point.
 */
void VToolSplinePath::AddPathPoint(VAbstractPattern *doc, QDomElement &domElement, const VSplinePoint &splPoint)
{
    SCASSERT(doc != nullptr)
    QDomElement pathPoint = doc->createElement(TagPathPoint);

    doc->SetAttribute(pathPoint, AttrPSpline, splPoint.P().id());
    doc->SetAttribute(pathPoint, AttrLength1, splPoint.Length1Formula());
    doc->SetAttribute(pathPoint, AttrLength2, splPoint.Length2Formula());
    doc->SetAttribute(pathPoint, AttrAngle1, splPoint.Angle1Formula());
    doc->SetAttribute(pathPoint, AttrAngle2, splPoint.Angle2Formula());

    VDrawTool::SaveAxis(doc, pathPoint, splPoint.Axis(), AttrAxisP1, AttrAxisP2, AttrAxisSense);

    if (domElement.hasAttribute(AttrKAsm1))
    {
        domElement.removeAttribute(AttrKAsm1);
    }

    if (domElement.hasAttribute(AttrKAsm2))
    {
        domElement.removeAttribute(AttrKAsm2);
    }

    if (domElement.hasAttribute(AttrAngle))
    {
        domElement.removeAttribute(AttrAngle);
    }

    domElement.appendChild(pathPoint);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SaveDialog save options into file after change in dialog.
 */
void VToolSplinePath::SaveDialog(QDomElement &domElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogSplinePath> dialogTool = qobject_cast<DialogSplinePath *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    const VSplinePath splPath = dialogTool->GetPath();

    qDeleteAll(controlPoints);
    controlPoints.clear();
    InitControlPoints(&splPath);

    doc->SetAttributeOrRemoveIf<QString>(domElement, AttrNotes, dialogTool->GetNotes(),
                                         [](const QString &notes) noexcept { return notes.isEmpty(); });

    SetSplinePathAttributes(domElement, splPath);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolAbstractBezier::SaveOptions(tag, obj);

    QSharedPointer<VSplinePath> const splPath = qSharedPointerDynamicCast<VSplinePath>(obj);
    SCASSERT(splPath.isNull() == false)

    SetSplinePathAttributes(tag, *splPath);

    doc->SetAttributeOrRemoveIf<QString>(tag,
                                         AttrCutNodes,
                                         OSplinePathPortions::EncodeNodes(m_cutNodes),
                                         [](const QString &value) noexcept -> bool { return value.isEmpty(); });
    doc->SetAttributeOrRemoveIf<QString>(tag,
                                         AttrPortionIds,
                                         OSplinePathPortions::EncodePieces(m_portionIds),
                                         [](const QString &value) noexcept -> bool { return value.isEmpty(); });
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (flags() & QGraphicsItem::ItemIsMovable && event->button() == Qt::LeftButton &&
        event->type() != QEvent::GraphicsSceneMouseDoubleClick)
    {
        oldPosition = event->scenePos();
        const auto splPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);
        splIndex = splPath->Segment(oldPosition);
        if (IsMovable(splIndex))
        {
            SetItemOverrideCursor(this, cursorArrowCloseHand, 1, 1);
            event->accept();
        }
    }
    VToolAbstractBezier::mousePressEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (flags() & QGraphicsItem::ItemIsMovable && event->button() == Qt::LeftButton &&
        event->type() != QEvent::GraphicsSceneMouseDoubleClick)
    {
        oldPosition = event->scenePos();
        SetItemOverrideCursor(this, cursorArrowOpenHand, 1, 1);

        CurveReleased();
    }
    VToolAbstractBezier::mouseReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // Don't need to check if left mouse button was pressed. According to the Qt documentation "If you do receive this
    // event, you can be certain that this item also received a mouse press event, and that this item is the current
    // mouse grabber.".

    if (!IsMovable(splIndex) || !(flags() & QGraphicsItem::ItemIsMovable))
    {
        return;
    }

    const auto oldSplPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);

    if (VAbstractApplication::VApp()->Settings()->IsFreeCurveMode() && not moved)
    {
        oldMoveSplinePath = QSharedPointer<VSplinePath>::create(*oldSplPath);
        moved = true;
    }

    newMoveSplinePath = QSharedPointer<VSplinePath>::create(*oldSplPath);

    VSpline const spline = newMoveSplinePath->GetSpline(splIndex);
    const qreal t = spline.ParamT(oldPosition);

    if (qFloor(t) == -1)
    {
        return;
    }

    // Magic Bezier Drag Equations follow!
    // "weight" describes how the influence of the drag should be distributed
    // among the handles; 0 = front handle only, 1 = back handle only.

    double weight;
    if (t <= 1.0 / 6.0)
    {
        weight = 0;
    }
    else if (t <= 0.5)
    {
        weight = (pow((6 * t - 1) / 2.0, 3)) / 2;
    }
    else if (t <= 5.0 / 6.0)
    {
        weight = (1 - pow((6 * (1 - t) - 1) / 2.0, 3)) / 2 + 0.5;
    }
    else
    {
        weight = 1;
    }

    const QPointF delta = event->scenePos() - oldPosition;
    const QPointF offset0 = ((1 - weight) / (3 * t * (1 - t) * (1 - t))) * delta;
    const QPointF offset1 = (weight / (3 * t * t * (1 - t))) * delta;

    const auto p2 = static_cast<QPointF>(spline.GetP2()) + offset0;
    const auto p3 = static_cast<QPointF>(spline.GetP3()) + offset1;

    oldPosition = event->scenePos(); // Now mouse here

    const auto spl = VSpline(spline.GetP1(), p2, p3, spline.GetP4());

    UpdateControlPoints(spl, newMoveSplinePath, splIndex);

    if (not VAbstractApplication::VApp()->Settings()->IsFreeCurveMode())
    {
        UndoCommandMove(*oldSplPath, *newMoveSplinePath);
    }
    else
    {
        VAbstractTool::data.UpdateGObject(m_id, newMoveSplinePath);
        RefreshGeometry();

        if (QGraphicsScene *sc = scene())
        {
            VMainGraphicsView::NewSceneRect(sc, VAbstractValApplication::VApp()->getSceneView(), this);
        }
    }

    // Each time we move something we call recalculation scene rect. In some cases this can cause moving
    // objects positions. And this cause infinite redrawing. That's why we wait the finish of saving the last move.
    static bool changeFinished = true;
    if (changeFinished)
    {
        changeFinished = false;

        const QList<QGraphicsView *> viewList = scene()->views();
        if (not viewList.isEmpty())
        {
            if (auto *view = qobject_cast<VMainGraphicsView *>(viewList.at(0)))
            {
                auto *currentScene = qobject_cast<VMainGraphicsScene *>(scene());
                SCASSERT(currentScene)
                const QPointF cursorPosition = currentScene->getScenePos();
                const qreal scale = SceneScale(scene());
                view->EnsureVisibleWithDelay(QRectF(cursorPosition.x() - 5 / scale,
                                                    cursorPosition.y() - 5 / scale,
                                                    10 / scale,
                                                    10 / scale),
                                             VMainGraphicsView::scrollDelay);
            }
        }
        changeFinished = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    if (GetAcceptHoverEvents())
    {
        if (flags() & QGraphicsItem::ItemIsMovable)
        {
            oldPosition = event->scenePos();
            const auto splPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);
            splIndex = splPath->Segment(oldPosition);
            if (IsMovable(splIndex))
            {
                SetItemOverrideCursor(this, cursorArrowOpenHand, 1, 1);
            }
            else
            {
                unsetCursor(); // no cursor of its own: the view decides, and a copied bitmap would cross into Cocoa
            }
        }
        else
        {
            unsetCursor(); // no cursor of its own: the view decides, and a copied bitmap would cross into Cocoa
        }

        VToolAbstractBezier::hoverEnterEvent(event);
    }
    else
    {
        unsetCursor(); // no cursor of its own: the view decides, and a copied bitmap would cross into Cocoa
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (GetAcceptHoverEvents())
    {
        VToolAbstractBezier::hoverLeaveEvent(event);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<VisToolSplinePath *>(vis);
        SCASSERT(visual != nullptr)

        QSharedPointer<VSplinePath> const splPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);
        visual->SetPath(*splPath.data());
        visual->SetLineStyle(LineStyleToPenStyle(splPath->GetPenStyle()));
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSplinePath::IsMovable(int index) const -> bool
{
    const auto splPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);

    if (index < 1 || index > splPath->CountSubSpl())
    {
        return false;
    }

    const VSplinePoint p1 = splPath->GetSplinePoint(index, SplinePointPosition::FirstPoint);
    const VSplinePoint p2 = splPath->GetSplinePoint(index, SplinePointPosition::LastPoint);

    return p1.IsMovable() && p2.IsMovable();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::RefreshCtrlPoints()
{
    // Very important to disable control points. Without it the pogram can't move the curve.
    for (auto *point : std::as_const(controlPoints))
    {
        point->setFlag(QGraphicsItem::ItemSendsGeometryChanges, false);
    }

    const auto splPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);

    for (qint32 i = 1; i <= splPath->CountSubSpl(); ++i)
    {
        const qint32 j = i * 2;

        if (j - 2 >= controlPoints.size())
        {
            break;
        }

        const QSignalBlocker blockerJ2(controlPoints[j - 2]);
        const QSignalBlocker blockerJ1(controlPoints[j - 1]);

        const auto spl = splPath->GetSpline(i);

        {
            bool freeAngle1 = true;

            if (i > 1)
            {
                const VSpline prevSpl = splPath->GetSpline(i - 1);
                freeAngle1 = spl.IsStartAngleFree() && prevSpl.IsEndAngleFree();
            }
            else
            {
                freeAngle1 = spl.IsStartAngleFree();
            }

            const bool freeLength1 = qmu::QmuTokenParser::IsSingle(spl.GetC1LengthFormula());

            const auto splinePoint = spl.GetP1();
            controlPoints[j - 2]->RefreshCtrlPoint(i, SplinePointPosition::FirstPoint,
                                                   static_cast<QPointF>(spl.GetP2()), static_cast<QPointF>(splinePoint),
                                                   freeAngle1, freeLength1);
        }

        {
            bool freeAngle2 = true;

            if (i < splPath->CountSubSpl())
            {
                const VSpline nextSpl = splPath->GetSpline(i + 1);
                freeAngle2 = nextSpl.IsStartAngleFree() && spl.IsEndAngleFree();
            }
            else
            {
                freeAngle2 = spl.IsEndAngleFree();
            }

            const bool freeLength2 = qmu::QmuTokenParser::IsSingle(spl.GetC2LengthFormula());

            const auto splinePoint = spl.GetP4();
            controlPoints[j - 1]->RefreshCtrlPoint(i, SplinePointPosition::LastPoint, static_cast<QPointF>(spl.GetP3()),
                                                   static_cast<QPointF>(splinePoint), freeAngle2, freeLength2);
        }
    }

    for (auto *point : std::as_const(controlPoints))
    {
        point->setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ProcessSplinePathToolOptions(oldDomElement, newDomElement, GatherToolChanges());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::CurveReleased()
{
    if (VAbstractApplication::VApp()->Settings()->IsFreeCurveMode() && moved)
    {
        UndoCommandMove(*oldMoveSplinePath, *newMoveSplinePath);

        oldMoveSplinePath.clear();
        newMoveSplinePath.clear();

        moved = false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::FullUpdateFromFile()
{

    VToolAbstractBezier::FullUpdateFromFile();

    // The runs are copies of this curve: when the curve moves or changes shape, they are stale until rebuilt.
    // This is the same refresh the cut tools rely on (VToolCut::FullUpdateFromFile -> RefreshGeometry), and a
    // lite parse — which is what ticking a box does — comes through here.
    RebuildPortions();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::RebuildDerivedObjects()
{
    // The portions are the objects this tool creates beside itself: a container restored from another tool's copy
    // has lost them, and the file still names their ids.
    RebuildPortions();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::GroupVisibility(quint32 object, bool visible)
{
    VToolAbstractBezier::GroupVisibility(object, visible);

    m_layerVisible = visible;

    // The runs are drawn by items of their own, so the curve's own visibility answers for them too: a run shown
    // while the layer of its curve is masked is exactly the trap of the seven copies (see m_objectVisible).
    ApplyPortionVisibility();
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSplinePath::IsCutNode(int node) const -> bool
{
    return m_cutNodes.contains(node);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::SetCutNode(int node, bool cut)
{
    const QSharedPointer<VSplinePath> path = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);

    if (path.isNull() || node <= 0 || node >= path->CountPoints() - 1)
    {
        return; // an end separates nothing: the piece before it or after it would be empty
    }

    if (IsCutNode(node) == cut)
    {
        return;
    }

    QVector<int> cutNodes = m_cutNodes;

    if (cut)
    {
        cutNodes.append(node);
        std::sort(cutNodes.begin(), cutNodes.end());
    }
    else
    {
        cutNodes.removeOne(node);
    }

    // **Asked now, while the graph knows every user.** A piece that stops being a run leaves the pattern only when
    // nothing is built on it — on this curve and on every copy an operation made of it. The question cannot wait for
    // the parse that follows: that parse empties the graph before it rebuilds it.
    PruneUnusedPieces(cutNodes);
    OCopyPieces::PruneUnused(doc, m_patternData != nullptr ? m_patternData : getData(), m_id, cutNodes);

    m_cutNodes = cutNodes;

    // **Built here, now.** Ticking a box is the moment the pieces come into being: their ids are allocated by the
    // rebuild below and written into the element by the SaveOption that follows, in the same gesture. The refresh
    // of the lite parse then finds them, instead of inventing ids the file never carried.
    RebuildPortions();

    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOption(obj);

    // A ticked node brings pieces into being, and they draw in place of the curve: nothing puts them in a layer
    // once the tool exists, the load-time passes only run on a file. Ticking is a creation like any other.
    AttachToActiveLayer();
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolSplinePath::PortionRuns() const -> QVector<QPair<int, int>>
{
    QVector<QPair<int, int>> runs;

    if (m_cutNodes.isEmpty())
    {
        return runs; // an uncut path is not "one run": it is the curve, and it has no pieces to speak of
    }

    const QSharedPointer<VSplinePath> path = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);

    if (path.isNull())
    {
        return runs;
    }

    for (const OSplinePathPortions::Run &run : OSplinePathPortions::Runs(m_cutNodes, path->CountPoints()))
    {
        runs.append({run.from, run.to});
    }

    return runs;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::RebuildPortions()
{
    // A tool built without a pattern container (a unit test, a dialog preview) keeps its own copy as the target.
    if (m_patternData == nullptr)
    {
        m_patternData = getData();
    }

    // **Ask before reading.** `GeometricObject` throws on an id the container does not hold, and this runs from
    // `setCurrentData()`: the pattern rebuilds the container from a tool's copy, and a curve read after that copy
    // was taken is simply not there yet. Throwing from here escaped the parse and closed the whole file
    // (`ExceptionBadId` in GlobalChangePP) — a rebuild that cannot find its curve has nothing to do, not a reason
    // to fail.
    if (not m_patternData->HasObject(m_id))
    {
        return;
    }

    const QSharedPointer<VSplinePath> path = m_patternData->GeometricObject<VSplinePath>(m_id);

    if (path.isNull())
    {
        return;
    }

    // The ids the pieces had before this rebuild: the ones that do not come back leave the graph with them.
    QVector<quint32> previousIds;
    for (const OSplinePathPortions::StoredPiece &stored : std::as_const(m_portionIds))
    {
        previousIds.append(stored.id);
    }

    // The items are rebuilt from the state every time; only the container objects outlive a reparse.
    for (VSimpleCurve *item : std::as_const(m_portionItems))
    {
        if (item != nullptr)
        {
            if (scene() != nullptr)
            {
                scene()->removeItem(item);
            }

            // A QObject child of this tool: it must be taken out of the child list as well, or the tool would
            // delete it a second time when it dies. Deleted later, not now: the rebuild may run from a handler the
            // item itself is still inside.
            item->setParent(nullptr);
            item->deleteLater();
        }
    }

    m_portionItems.clear();

    const vsizetype nodes = path->CountPoints();
    const QVector<OSplinePathPortions::Run> runs =
        m_cutNodes.isEmpty() ? QVector<OSplinePathPortions::Run>{} : OSplinePathPortions::Runs(m_cutNodes, nodes);

    // One piece of the path, as an object of the pattern under @a id (NULL_ID: a new one): the rule every piece of a
    // curve follows, the copies' included (OSplinePathPortions).
    auto BuildPiece = [this, &path](const OSplinePathPortions::Run &run, quint32 id,
                                    const QString &suffix) -> QPair<quint32, QSharedPointer<VAbstractCurve>>
    {
        const QSharedPointer<VAbstractCurve> piece = OSplinePathPortions::RunCurve(*path, run);
        if (piece.isNull())
        {
            return {NULL_ID, {}};
        }

        OSplinePathPortions::MarkPiece(*piece, *path, m_id, suffix);
        const quint32 stored = OSplinePathPortions::StorePiece(m_patternData, piece, id);
        return {stored, OSplinePathPortions::CurveOf(m_patternData, stored)};
    };

    // What this rebuild leaves: the runs of the boxes, and the pieces kept for their users.
    QMap<QString, OSplinePathPortions::StoredPiece> pieces;

    int portionIndex = 0;

    for (const OSplinePathPortions::Run &run : runs)
    {
        ++portionIndex;

        // A run keeps the object it already has, found by its two ends: an id written in the file is the same object
        // after a reparse, so a name a formula cites keeps pointing at it. A file written before the runs had two
        // ends named them by the node they end on, and is read that way once.
        const QString key = OSplinePathPortions::RunKey(run);
        quint32 id = m_portionIds.value(key).id;
        if (id == NULL_ID)
        {
            id = m_portionIds.value(OSplinePathPortions::LegacyRunKey(run.to)).id;
        }

        const auto [builtId, curve] =
            BuildPiece(run, id, path->HeadlessName() + OSplinePathPortions::PortionSuffix(portionIndex));

        if (curve.isNull())
        {
            continue;
        }

        pieces.insert(key, OSplinePathPortions::StoredPiece{builtId, QString()});

        auto *item = new VSimpleCurve(builtId, curve, this);
        item->SetType(curve->getType());
        item->RefreshGeometry(curve);
        item->setZValue(0);
        item->setVisible(false);
        m_portionItems.append(item);

        // A piece is a thing of the pattern, so it answers a click like any other: it announces itself through the
        // tool's own signal, which is what the scene carries to the dialog that is picking objects. Without this line
        // a piece was drawn, hovered — and mute: no operation tool could take it.
        const SceneObject type = curve->SceneObjectType();
        const quint32 pieceId = builtId;
        connect(item,
                &VSimpleCurve::Choosed,
                this,
                [this, pieceId, type](quint32) -> void { emit ChoosedTool(pieceId, type); });
        connect(item,
                &VSimpleCurve::Selected,
                this,
                [this, pieceId](bool selected) -> void { emit ChangedToolSelection(selected, pieceId, m_id); });
        connect(item,
                &VSimpleCurve::ShowContextMenu,
                this,
                [this](QGraphicsSceneContextMenuEvent *event, quint32) -> void { ShowContextMenu(event); });

        if (scene() != nullptr)
        {
            scene()->addItem(item);
        }
    }

    // **The pieces kept for their users.** A run that stopped being one when a box was unticked, and that something
    // is built on (see PruneUnusedPieces), stays in the pattern: same id, same name, the geometry of the curve it
    // still lies on. It is not drawn — the curve is whole again on the canvas — but the copy an operation made of it,
    // the point a tool cut on it, the formula that reads its length all keep reading it.
    for (auto it = m_portionIds.cbegin(); it != m_portionIds.cend(); ++it)
    {
        OSplinePathPortions::Run run;

        if (pieces.contains(it.key()))
        {
            continue; // a run of the boxes
        }

        if (not OSplinePathPortions::ParseRunKey(it.key(), run))
        {
            // A legacy key no run of the boxes matched: it is kept as the file wrote it rather than dropped — what is
            // built on that piece would otherwise lose it with no word.
            if (not it.key().contains(QLatin1Char('-')))
            {
                pieces.insert(it.key(), it.value());
            }
            continue;
        }

        if (run.to >= nodes)
        {
            continue; // the path no longer has the nodes this run lay between
        }

        QString suffix = it.value().name;
        if (const QSharedPointer<VAbstractCurve> held = OSplinePathPortions::CurveOf(m_patternData, it.value().id);
            suffix.isEmpty() && not held.isNull())
        {
            suffix = held->HeadlessName();
        }
        if (suffix.isEmpty())
        {
            suffix = OSplinePathPortions::KeptSuffix(path->HeadlessName(), run);
        }

        const auto [builtId, curve] = BuildPiece(run, it.value().id, suffix);

        if (not curve.isNull())
        {
            pieces.insert(it.key(), OSplinePathPortions::StoredPiece{builtId, suffix});
        }
    }

    m_portionIds = pieces;

    QVector<quint32> ids;
    for (const OSplinePathPortions::StoredPiece &stored : std::as_const(m_portionIds))
    {
        ids.append(stored.id);
    }

    // A piece is an object of the pattern: the graph holds it as one, with an edge from the curve it comes
    // from, so a dependency tree, a deletion and an operation that asks what a source depends on all see it.
    OSplinePathPortions::SyncGraph(doc, m_id, previousIds, ids);

    // **The tool's own copy has to carry what the tool just built.** The pattern rebuilds the live container from
    // that copy (`setCurrentData()`, `GetCompletePPData()`), and the copy was taken before these pieces existed —
    // so every container rebuilt from it had lost them: drawn, clickable, and unfindable by id. The cut tools
    // never meet this because their halves are made inside their own `Create`, before their copy is written.
    doc->UpdateToolData(m_id, m_patternData);

    // The ids are written straight into the tool's element, without touching the undo stack: they are not a
    // decision of the user, and a parse that did not find them would allocate a new object beside each piece —
    // every box tick, every reparse, a new set of orphan curves nobody asked for.
    if (QDomElement element = doc->FindElementById(m_id, getTagName()); not element.isNull())
    {
        doc->SetAttributeOrRemoveIf<QString>(element,
                                             AttrPortionIds,
                                             OSplinePathPortions::EncodePieces(m_portionIds),
                                             [](const QString &value) noexcept -> bool { return value.isEmpty(); });
    }

    ApplyPortionVisibility();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::PruneUnusedPieces(const QVector<int> &cutNodes)
{
    VContainer *const container = m_patternData != nullptr ? m_patternData : getData();
    const QSharedPointer<VSplinePath> path = container->HasObject(m_id)
                                                 ? container->GeometricObject<VSplinePath>(m_id)
                                                 : QSharedPointer<VSplinePath>();

    if (path.isNull())
    {
        return;
    }

    QSet<QString> runKeys;
    if (not cutNodes.isEmpty())
    {
        for (const OSplinePathPortions::Run &run : OSplinePathPortions::Runs(cutNodes, path->CountPoints()))
        {
            runKeys.insert(OSplinePathPortions::RunKey(run));
        }
    }

    for (auto it = m_portionIds.begin(); it != m_portionIds.end();)
    {
        if (runKeys.contains(it.key()))
        {
            ++it;
            continue; // still a run: it stays, under the name of its rank
        }

        if (not OSplinePathPortions::IsInUse(doc, it.value().id))
        {
            // Nothing is built on it: it leaves with its run, and with its place in the graph — the rebuild that
            // follows no longer knows it, and could not take it out.
            OSplinePathPortions::SyncGraph(doc, m_id, {it.value().id}, {});
            it = m_portionIds.erase(it);
            continue;
        }

        // Kept for its users, under the name they know it by: the rank it was named after is gone with its run.
        if (const QSharedPointer<VAbstractCurve> held = OSplinePathPortions::CurveOf(container, it.value().id);
            it.value().name.isEmpty() && not held.isNull())
        {
            it.value().name = held->HeadlessName();
        }

        ++it;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolSplinePath::ApplyPortionVisibility()
{
    const bool cut = not m_cutNodes.isEmpty();

    SetReplacedBySegments(cut);

    // A curve that has been cut is no longer the curve: the pieces answer for it. Leaving it hoverable made the
    // parent win wherever a piece lies on it — the mouse found the whole curve on some parts of the drawing and
    // the piece on others — and highlighted the curve the user had just replaced. It is hidden, and it stops
    // taking the mouse at all; the control points stay live, since the handles are still how the curve is edited.
    setAcceptHoverEvents(not cut && m_acceptHover);
    setAcceptedMouseButtons(cut ? Qt::NoButton : (Qt::LeftButton | Qt::RightButton));
    setFlag(QGraphicsItem::ItemIsSelectable, not cut);

    for (VSimpleCurve *item : std::as_const(m_portionItems))
    {
        if (item != nullptr)
        {
            item->setVisible(cut && m_layerVisible);
            // Above the curve's own item, never behind it: a piece drawn at -1 loses the mouse to the curve it
            // was taken from, exactly where the two coincide.
            item->setZValue(0);
        }
    }
}
