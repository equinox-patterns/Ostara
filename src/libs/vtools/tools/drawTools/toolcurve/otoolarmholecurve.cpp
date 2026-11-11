/************************************************************************
 **
 **  @file   otoolarmholecurve.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/08/2026
 **
 **  @brief  Armhole curve tool — 2-segment cubic Bezier path through 4
 **          user-selected points (armhole, chest/back width, perpendicular,
 **          shoulder) with 3 auto-computed control handles.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
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

#include "otoolarmholecurve.h"

#include "../vwidgets/vsimplecurve.h"

#include <QDomElement>
#include <QPen>
#include <QSharedPointer>
#include <cmath>
#include <new>

#include "../../../visualization/path/ovistoolarmholecurve.h"
#include "../../../visualization/visualization.h"
#include "../../vabstracttool.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/../ifc/ifcdef.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "vexception.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../operation/ocopypieces.h"
#include "osplinepathportions.h"
#include "vtoolabstractcurve.h"

const QString OToolArmholeCurve::ToolType = QStringLiteral("armholeCurve");

//---------------------------------------------------------------------------------------------------------------------
OToolArmholeCurve::OToolArmholeCurve(const OToolArmholeCurveInitData &initData, QGraphicsItem *parent)
  : VToolAbstractBezier(initData.doc, initData.data, initData.id, initData.notes, parent)
{
    SetSceneType(SceneObject::SplinePath);

    this->setFlag(QGraphicsItem::ItemIsFocusable, true); // For keyboard input focus

    ToolCreation(initData.typeCreation);

    m_patternData = initData.data;

    m_segmentAtCarrure = initData.segmentAtCarrure;
    m_firstSegmentId = initData.firstSegmentId;
    m_secondSegmentId = initData.secondSegmentId;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::Create(OToolArmholeCurveInitData initData) -> OToolArmholeCurve *
{
    // The container updates the object it already holds when the id is known, and does not adopt what it is
    // given: the share the init data carries keeps the curve alive on that branch too, so nothing read below
    // can hit freed memory.
    const QSharedPointer<VCubicBezierPath> curve = initData.path;

    const bool curveUnique = initData.data->IsUnique(curve->name());

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
            = QObject::tr("Curve '%1' with id %2 has not unique name.").arg(curve->name()).arg(initData.id);
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VException(errorMsg)
            : qWarning() << errorMsg; // internal invariant: the log is enough
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    initData.data->AddCurveWithSegments(initData.data->GeometricObject<VAbstractCubicBezierPath>(initData.id),
                                        initData.id);

    // Graph edges from the 4 user-selected points only. The 3 control handles are
    // derived from these points, so they carry no vertices or edges of their own.
    const auto &path = *initData.data->GeometricObject<VCubicBezierPath>(initData.id);
    patternGraph->AddEdge(path.at(0).id(), initData.id); // armhole
    patternGraph->AddEdge(path.at(3).id(), initData.id); // chest/back width
    if (path.at(5).id() != path.at(6).id()) // skipped perpendicular — already the shoulder's edge
    {
        patternGraph->AddEdge(path.at(5).id(), initData.id);
    }
    patternGraph->AddEdge(path.at(6).id(), initData.id); // shoulder

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::ArmholeCurve, initData.doc);
        auto *_spl = new OToolArmholeCurve(initData);
        initData.scene->addItem(_spl);
        InitSplineToolConnections(initData.scene, _spl);
        VAbstractPattern::AddTool(initData.id, _spl);
        _spl->RebuildSegments();
        return _spl;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::SolveHandles(const VPointF &em, const VPointF &carrure, double t) -> ArmholeHandles
{
    // Segment 1 (armhole → chest/back width):
    //   C1 = V = (Carrure.x, Em.y) — same horizontal handle as clicking V in standard CubicBezier
    //   C2 = V1 = Carrure − 0.1cm along V→Carrure — fixed distance before Carrure
    //
    // Segment 2 (chest/back width → shoulder):
    //   C1 = Carrure + t·(Carrure−V) — length = t·|V−Carrure|, direction auto-mirrored by GetSpline

    // V = corner of the Em-Carrure rectangle
    const VPointF V(carrure.x(), em.y(), QString());

    // --- Segment 1 handles ---

    // h1a = V: horizontal from armhole, gives the natural "creux" depth
    const VPointF h1a = V;

    const double dxVtoC = V.x() - carrure.x();
    const double dyVtoC = V.y() - carrure.y();
    const double seg1HandleLen = std::sqrt(dxVtoC * dxVtoC + dyVtoC * dyVtoC);

    // h1b (V1): incoming to Carrure, at a fixed 0.1cm before Carrure on the V→Carrure line.
    // Matches the standard CubicBezierPath construction where V1 = Carrure − 0.1cm toward V.
    constexpr double v1Offset = 0.1; // cm — distance before Carrure (0.01cm also sufficient)
    VPointF h1b(carrure.x(), carrure.y(), QString());
    if (seg1HandleLen > 1e-9)
    {
        h1b = VPointF(carrure.x() + v1Offset * dxVtoC / seg1HandleLen,
                      carrure.y() + v1Offset * dyVtoC / seg1HandleLen, QString());
    }

    // --- Segment 2 handle ---

    // h2a = Carrure + t·(Carrure−V): stored in the mirror direction, length t·|Carrure−V|.
    // VCubicBezierPath::GetSpline() will re-angle it to mirror V1 for G1 continuity, keeping this length.
    const VPointF h2a(carrure.x() + t * (carrure.x() - V.x()),
                      carrure.y() + t * (carrure.y() - V.y()), QString());

    return {h1a, h1b, h2a};
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::IsPerpendicularSkipped(quint32 perpendicularId, quint32 shoulderId) -> bool
{
    return perpendicularId == NULL_ID || perpendicularId == shoulderId;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::SolveSkippedPerpendicular(const VPointF &carrure, const VPointF &shoulder) -> VPointF
{
    // shoulder + β·(carrure − shoulder): a point on the Carrure→Shoulder chord, so the curve arrives
    // at the shoulder along the shoulder line — the behaviour of the original 3-point solver. The
    // shoulder's identity is kept on purpose: the file marker point6 == point7 depends on it.
    const QPointF point = static_cast<QPointF>(shoulder)
                          + kPerpendicularRatio * (static_cast<QPointF>(carrure) - static_cast<QPointF>(shoulder));

    VPointF perpendicular = shoulder;
    perpendicular.setX(point.x());
    perpendicular.setY(point.y());
    return perpendicular;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::BuildPath(const VPointF &armhole, const VPointF &carrure, const VPointF &perpendicular,
                                  const VPointF &shoulder) -> VCubicBezierPath
{
    const ArmholeHandles handles = SolveHandles(armhole, carrure, kDefaultHandleRatio);

    const VPointF segment2C2 = IsPerpendicularSkipped(perpendicular.id(), shoulder.id())
                                   ? SolveSkippedPerpendicular(carrure, shoulder)
                                   : perpendicular;

    // armhole, V, V1, chest/back width, h2a, perpendicular, shoulder
    return VCubicBezierPath({armhole, handles.h1a, handles.h1b, carrure, handles.h2a, segment2C2, shoulder});
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::FirstPointName() const -> QString
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return path->at(0).name(); // armhole
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::SecondPointName() const -> QString
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return path->at(3).name(); // chest/back width
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::ThirdPointName() const -> QString
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return path->at(5).name(); // perpendicular
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::FourthPointName() const -> QString
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return path->at(6).name(); // shoulder
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::GetPath() const -> VCubicBezierPath
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return *path.data();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::SetPath(const VCubicBezierPath &path)
{
    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    QSharedPointer<VCubicBezierPath> const splinePath = qSharedPointerDynamicCast<VCubicBezierPath>(obj);
    *splinePath.data() = path;
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolArmholeCurve>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    Q_UNUSED(id)
    try
    {
        // No "Options" entry here: the curve is edited through the property browser only.
        ContextMenuWithoutOptions(event);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::SaveDialog(QDomElement &domElement)
{
    doc->SetAttributeOrRemoveIf<QString>(domElement, AttrNotes, GetNotes(),
                                         [](const QString &notes) noexcept { return notes.isEmpty(); });

    SetPathAttributes(domElement, GetPath());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolAbstractBezier::SaveOptions(tag, obj);

    auto path = qSharedPointerDynamicCast<VCubicBezierPath>(obj);
    SCASSERT(path.isNull() == false)
    SetPathAttributes(tag, *path);

    doc->SetAttributeOrRemoveIf<bool>(tag,
                                      AttrSegmentAtCarrure,
                                      m_segmentAtCarrure,
                                      [](bool value) noexcept -> bool { return !value; });
    doc->SetAttributeOrRemoveIf<quint32>(tag,
                                         AttrSegment1Id,
                                         m_firstSegmentId,
                                         [](quint32 id) noexcept -> bool { return id == NULL_ID; });
    doc->SetAttributeOrRemoveIf<quint32>(tag,
                                         AttrSegment2Id,
                                         m_secondSegmentId,
                                         [](quint32 id) noexcept -> bool { return id == NULL_ID; });
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<OVisToolArmholeCurve *>(vis);
        SCASSERT(visual != nullptr)

        const QSharedPointer<VCubicBezierPath> path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
        // Hand the stored path over: recomputing it from the 4 points would be wrong when the
        // perpendicular was skipped.
        visual->SetPath(*path);
        visual->SetLineStyle(LineStyleToPenStyle(path->GetPenStyle()));
        visual->SetApproximationScale(path->GetApproximationScale());
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::RefreshGeometry()
{
    const QSharedPointer<VCubicBezierPath> path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    this->setPath(path->GetPath());

    SetVisualization();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::SetPathAttributes(QDomElement &domElement, const VCubicBezierPath &path)
{
    SCASSERT(doc != nullptr)

    doc->SetAttribute(domElement, AttrType, ToolType);
    // Store the 4 user-selected point IDs. The 3 handles are recomputed on parse,
    // so their point attributes are informational only (kept for graph-edge context).
    doc->SetAttribute(domElement, AttrPoint1, path.at(0).id()); // armhole
    doc->SetAttribute(domElement, AttrPoint4, path.at(3).id()); // chest/back width
    doc->SetAttribute(domElement, AttrPoint6, path.at(5).id()); // perpendicular
    doc->SetAttribute(domElement, AttrPoint7, path.at(6).id()); // shoulder
    doc->SetAttribute(domElement, AttrColor, path.GetColor());
    doc->SetAttribute(domElement, AttrPenStyle, path.GetPenStyle());
    doc->SetAttribute(domElement, AttrAScale, path.GetApproximationScale());
    doc->SetAttributeOrRemoveIf<quint32>(domElement, AttrDuplicate, path.GetDuplicate(),
                                         [](quint32 duplicate) noexcept { return duplicate == 0; });
    doc->SetAttributeOrRemoveIf<QString>(domElement, AttrAlias, path.GetAliasSuffix(),
                                         [](const QString &suffix) noexcept { return suffix.isEmpty(); });
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolArmholeCurve::IsSegmentAtCarrure() const -> bool
{
    return m_segmentAtCarrure;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::SetSegmentAtCarrure(bool segment)
{
    if (m_segmentAtCarrure == segment)
    {
        return;
    }

    // **Asked now, while the graph knows every user.** Unticking the box takes the two pieces away from the curve,
    // but a piece something is built on — an operation that copies it, a point cut on it, a formula — stays in the
    // pattern for it, under its id and its name. The same question is asked of every copy an operation made of this
    // curve: a copy of the whole curve loses its pieces with it, unless one of them is in use. It cannot wait for the
    // parse that follows, which empties the graph before rebuilding it.
    if (not segment)
    {
        for (quint32 *id : {&m_firstSegmentId, &m_secondSegmentId})
        {
            if (*id != NULL_ID && not OSplinePathPortions::IsInUse(doc, *id))
            {
                // It leaves the graph here: the rebuild that follows no longer knows it, and could not take it out.
                OSplinePathPortions::SyncGraph(doc, m_id, {*id}, {});
                *id = NULL_ID;
            }
        }
    }

    OCopyPieces::PruneUnused(doc, m_patternData != nullptr ? m_patternData : getData(), m_id,
                             segment ? QVector<int>{CarrureNode} : QVector<int>{});

    m_segmentAtCarrure = segment;

    // **Built here, now.** Ticking the box is the moment the two pieces come into being: their ids are allocated
    // by the rebuild below and written into the element by the SaveOption that follows, in the same gesture. A
    // rebuild left to the refresh of the lite parse happened after the element was written — the ids then
    // arrived by another road, and an operation could cite a piece the file never named.
    RebuildSegments();

    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::RebuildSegments()
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

    const QSharedPointer<VCubicBezierPath> path = m_patternData->GeometricObject<VCubicBezierPath>(m_id);

    if (path.isNull())
    {
        return;
    }

    // The ids the pieces had before this rebuild: the ones that do not come back leave the graph with them.
    const QVector<quint32> previousIds{m_firstSegmentId, m_secondSegmentId};

    for (VSimpleCurve **slot : {&m_firstCurve, &m_secondCurve})
    {
        VSimpleCurve *item = *slot;

        if (item != nullptr)
        {
            if (scene() != nullptr)
            {
                scene()->removeItem(item);
            }

            item->setParent(nullptr);
            item->deleteLater(); // the rebuild may run from a handler the item is still inside
            *slot = nullptr;
        }
    }

    // Uncut: the curve is the curve, and only the pieces kept for their users are left — the ones unticking found
    // something built on (see SetSegmentAtCarrure). They are not drawn: the curve is whole again on the canvas.
    const bool drawn = m_segmentAtCarrure;

    // The pieces are the two sub-bezier segments the curve already carries — nothing is recomputed, and the
    // joint is the chest/back width anchor, which is the end of the first and the start of the second.
    quint32 *const ids[2] = {&m_firstSegmentId, &m_secondSegmentId};
    VSimpleCurve **const items[2] = {&m_firstCurve, &m_secondCurve};

    for (int index = 1; index <= 2; ++index)
    {
        if (index > path->CountSubSpl())
        {
            break;
        }

        quint32 id = *ids[index - 1];

        if (not drawn && id == NULL_ID)
        {
            continue; // uncut, and nothing kept this piece
        }

        // The piece is the sub-bezier segment the curve already carries, under the rule every piece of a curve
        // follows (OSplinePathPortions): its pen, the derivative flag, the curve's name plus its own mark
        // (`Spl_A18_A28__c1__p1`), and the id the file gave it when that id is still this piece.
        const QSharedPointer<VAbstractCurve> piece =
            OSplinePathPortions::RunCurve(*path, OSplinePathPortions::Run{index - 1, index});
        if (piece.isNull())
        {
            continue;
        }

        OSplinePathPortions::MarkPiece(*piece, *path, m_id,
                                       path->HeadlessName() + OSplinePathPortions::PortionSuffix(index));
        id = OSplinePathPortions::StorePiece(m_patternData, piece, id);
        *ids[index - 1] = id;

        const QSharedPointer<VAbstractCurve> stored = OSplinePathPortions::CurveOf(m_patternData, id);

        if (stored.isNull())
        {
            continue;
        }

        if (not drawn)
        {
            continue; // kept, not drawn
        }

        QSharedPointer<VAbstractCurve> curve = stored;

        auto *item = new VSimpleCurve(id, curve, this);
        item->SetType(curve->getType());
        item->RefreshGeometry(curve);
        item->setZValue(0);
        item->setVisible(false);
        *items[index - 1] = item;

        // A piece answers a click like any object of the pattern: it announces itself through the tool's own
        // signal, which is the channel the scene carries to the dialog picking objects. Without it a piece is
        // drawn, hovered — and mute.
        const SceneObject type = curve->SceneObjectType();
        connect(item,
                &VSimpleCurve::Choosed,
                this,
                [this, id, type](quint32) -> void { emit ChoosedTool(id, type); });
        connect(item,
                &VSimpleCurve::Selected,
                this,
                [this, id](bool selected) -> void { emit ChangedToolSelection(selected, id, m_id); });
        connect(item,
                &VSimpleCurve::ShowContextMenu,
                this,
                [this](QGraphicsSceneContextMenuEvent *event, quint32) -> void { ShowContextMenu(event); });

        if (scene() != nullptr)
        {
            scene()->addItem(item);
        }
    }

    // The ids are written straight into the element, or every parse would allocate a new pair beside the old
    // ones — which is exactly what left orphans behind on the curve cut.
    if (QDomElement element = doc->FindElementById(m_id, getTagName()); not element.isNull())
    {
        doc->SetAttributeOrRemoveIf<quint32>(element,
                                             AttrSegment1Id,
                                             m_firstSegmentId,
                                             [](quint32 id) noexcept -> bool { return id == NULL_ID; });
        doc->SetAttributeOrRemoveIf<quint32>(element,
                                             AttrSegment2Id,
                                             m_secondSegmentId,
                                             [](quint32 id) noexcept -> bool { return id == NULL_ID; });
    }

    // **The tool's own copy has to carry what the tool just built.** `VDataTool` keeps a copy of the container,
    // and the pattern rebuilds the live one from it — `setCurrentData()` and `GetCompletePPData()` both do
    // `*data = vTool->GetDataCopy()`. That copy is taken when the tool is read, before this method runs, so the
    // two pieces were absent from every container rebuilt from it: they stayed drawn on the canvas, and every
    // lookup by id failed — `ExceptionBadId` in the preview of an operation, and no copy when it was created.
    // The cut tools never meet this because their halves are made inside their own `Create`, before their copy is
    // written. The snapshot is refreshed here, exactly as `UpdateToolData` does at the end of a tool's creation.
    doc->UpdateToolData(m_id, m_patternData);

    // A piece is an object of the pattern: the graph holds it as one, with an edge from the curve it comes
    // from, so a dependency tree, a deletion and an operation citing it all see it.
    OSplinePathPortions::SyncGraph(doc, m_id, previousIds, {m_firstSegmentId, m_secondSegmentId});

    ApplySegmentVisibility();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::ApplySegmentVisibility()
{
    SetReplacedBySegments(m_segmentAtCarrure);

    // A curve shown as two pieces is no longer the curve: the pieces answer for it, and the parent must not
    // take the mouse nor the hover — a parent left live wins wherever a piece lies on it and highlights the
    // curve the user has just replaced.
    setAcceptHoverEvents(not m_segmentAtCarrure && m_acceptHover);
    setAcceptedMouseButtons(m_segmentAtCarrure ? Qt::NoButton : (Qt::LeftButton | Qt::RightButton));
    setFlag(QGraphicsItem::ItemIsSelectable, not m_segmentAtCarrure);

    for (VSimpleCurve *item : {m_firstCurve, m_secondCurve})
    {
        if (item != nullptr)
        {
            item->setVisible(m_segmentAtCarrure && m_layerVisible);

        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::GroupVisibility(quint32 object, bool visible)
{
    VToolAbstractBezier::GroupVisibility(object, visible);

    m_layerVisible = visible;
    ApplySegmentVisibility();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::FullUpdateFromFile()
{
    VToolAbstractBezier::FullUpdateFromFile();

    // The pieces are copies of this curve: when the curve changes shape, they are stale until rebuilt.
    RebuildSegments();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::RebuildDerivedObjects()
{
    // The two pieces are the objects this tool creates beside itself: a container restored from another tool's
    // copy has lost them, and the file still names their ids.
    RebuildSegments();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::AllowHover(bool enabled)
{
    m_acceptHover = enabled;
    VToolAbstractBezier::AllowHover(enabled);

    // The pieces answer the hover the application asks for, exactly as the curve does: a piece that stops
    // answering as soon as a tool picks objects is a piece the user cannot aim at.
    for (VSimpleCurve *item : {m_firstCurve, m_secondCurve})
    {
        if (item != nullptr && m_segmentAtCarrure)
        {
            item->setAcceptHoverEvents(enabled);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolArmholeCurve::AllowSelecting(bool enabled)
{
    VToolAbstractBezier::AllowSelecting(enabled);

    // The two pieces are objects of the pattern: the tools that pick objects make them selectable as they do
    // for the curve they were taken from.
    for (VSimpleCurve *item : {m_firstCurve, m_secondCurve})
    {
        if (item != nullptr)
        {
            item->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
        }
    }
}
