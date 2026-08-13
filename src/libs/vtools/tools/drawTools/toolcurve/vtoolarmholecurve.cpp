/************************************************************************
 **
 **  @file   vtoolarmholecurve.cpp
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

#include "vtoolarmholecurve.h"

#include <QDomElement>
#include <QPen>
#include <QSharedPointer>
#include <cmath>
#include <new>

#include "../../../dialogs/tools/dialogarmholecurve.h"
#include "../../../dialogs/tools/dialogtool.h"
#include "../../../visualization/path/vistoolarmholecurve.h"
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
#include "vtoolabstractcurve.h"

const QString VToolArmholeCurve::ToolType = QStringLiteral("armholeCurve");

//---------------------------------------------------------------------------------------------------------------------
VToolArmholeCurve::VToolArmholeCurve(const VToolArmholeCurveInitData &initData, QGraphicsItem *parent)
  : VToolAbstractBezier(initData.doc, initData.data, initData.id, initData.notes, parent)
{
    SetSceneType(SceneObject::SplinePath);

    this->setFlag(QGraphicsItem::ItemIsFocusable, true); // For keyboard input focus

    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    auto *dialogTool = qobject_cast<DialogArmholeCurve *>(m_dialog);
    SCASSERT(dialogTool != nullptr)
    const auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetPath(*path);
    dialogTool->SetNotes(m_notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolArmholeCurve::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                               VContainer *data) -> VToolArmholeCurve *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogArmholeCurve> dialogTool = qobject_cast<DialogArmholeCurve *>(dialog);
    SCASSERT(not dialogTool.isNull())

    // The dialog's path already holds the 7 points (4 user points + 3 local handles).
    // The 3 handles are derived points (computed by SolveHandles) — they are not
    // container GObjects and carry no graph vertices, mirroring VToolCubicBezierPath.
    VToolArmholeCurveInitData initData;
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.path = new VCubicBezierPath(dialogTool->GetPath());
    initData.notes = dialogTool->GetNotes();

    auto *spl = Create(initData);

    if (spl != nullptr)
    {
        spl->m_dialog = dialog;
    }
    return spl;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolArmholeCurve::Create(VToolArmholeCurveInitData initData) -> VToolArmholeCurve *
{
    const bool curveUnique = initData.data->IsUnique(initData.path->name());

    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->AddGObject(initData.path);
    }
    else
    {
        initData.data->UpdateGObject(initData.id, initData.path);
    }

    if (!curveUnique)
    {
        const QString errorMsg
            = QObject::tr("Curve '%1' with id %2 has not unique name.").arg(initData.path->name()).arg(initData.id);
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VException(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    initData.data->AddSpline(initData.data->GeometricObject<VAbstractBezier>(initData.id), initData.id);

    // Graph edges from the 4 user-selected points only. The 3 control handles are
    // derived from these points, so they carry no vertices or edges of their own.
    const auto &path = *initData.data->GeometricObject<VCubicBezierPath>(initData.id);
    patternGraph->AddEdge(path.at(0).id(), initData.id); // armhole
    patternGraph->AddEdge(path.at(3).id(), initData.id); // chest/back width
    patternGraph->AddEdge(path.at(5).id(), initData.id); // perpendicular
    patternGraph->AddEdge(path.at(6).id(), initData.id); // shoulder

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::ArmholeCurve, initData.doc);
        auto *_spl = new VToolArmholeCurve(initData);
        initData.scene->addItem(_spl);
        InitSplineToolConnections(initData.scene, _spl);
        VAbstractPattern::AddTool(initData.id, _spl);
        return _spl;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolArmholeCurve::SolveHandles(const VPointF &em, const VPointF &carrure, double t) -> ArmholeHandles
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
auto VToolArmholeCurve::FirstPointName() const -> QString
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return path->at(0).name(); // armhole
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolArmholeCurve::SecondPointName() const -> QString
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return path->at(3).name(); // chest/back width
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolArmholeCurve::ThirdPointName() const -> QString
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return path->at(5).name(); // perpendicular
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolArmholeCurve::FourthPointName() const -> QString
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return path->at(6).name(); // shoulder
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolArmholeCurve::GetPath() const -> VCubicBezierPath
{
    auto path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    return *path.data();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::SetPath(const VCubicBezierPath &path)
{
    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    QSharedPointer<VCubicBezierPath> const splinePath = qSharedPointerDynamicCast<VCubicBezierPath>(obj);
    *splinePath.data() = path;
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::ShowVisualization(bool show)
{
    ShowToolVisualization<VisToolArmholeCurve>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    Q_UNUSED(id)
    try
    {
        ContextMenu<DialogArmholeCurve>(event);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::SaveDialog(QDomElement &domElement)
{
    SCASSERT(not m_dialog.isNull())
    auto *dialogTool = qobject_cast<DialogArmholeCurve *>(m_dialog);
    SCASSERT(dialogTool != nullptr)

    doc->SetAttributeOrRemoveIf<QString>(domElement, AttrNotes, dialogTool->GetNotes(),
                                         [](const QString &notes) noexcept { return notes.isEmpty(); });

    SetPathAttributes(domElement, dialogTool->GetPath());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolAbstractBezier::SaveOptions(tag, obj);

    auto path = qSharedPointerDynamicCast<VCubicBezierPath>(obj);
    SCASSERT(path.isNull() == false)
    SetPathAttributes(tag, *path);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<VisToolArmholeCurve *>(vis);
        SCASSERT(visual != nullptr)

        const QSharedPointer<VCubicBezierPath> path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
        visual->SetPoint1Id(path->at(0).id());  // armhole
        visual->SetPoint2Id(path->at(3).id());  // chest/back width
        visual->SetPoint3Id(path->at(5).id());  // perpendicular
        visual->SetPoint4Id(path->at(6).id());  // shoulder
        visual->SetLineStyle(LineStyleToPenStyle(path->GetPenStyle()));
        visual->SetApproximationScale(path->GetApproximationScale());
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::RefreshGeometry()
{
    const QSharedPointer<VCubicBezierPath> path = VAbstractTool::data.GeometricObject<VCubicBezierPath>(m_id);
    this->setPath(path->GetPath());

    SetVisualization();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ProcessSplineToolOptions(oldDomElement, newDomElement, GatherToolChanges());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolArmholeCurve::SetPathAttributes(QDomElement &domElement, const VCubicBezierPath &path)
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
auto VToolArmholeCurve::GatherToolChanges() const -> VToolAbstractBezier::ToolChanges
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogArmholeCurve> dialogTool = qobject_cast<DialogArmholeCurve *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    const VCubicBezierPath newPath = dialogTool->GetPath();
    const auto oldPath = VAbstractTool::data.GeometricObject<VAbstractCubicBezierPath>(m_id);

    return {.oldP1Label = oldPath->FirstPoint().name(),
            .newP1Label = newPath.FirstPoint().name(),
            .oldP4Label = oldPath->LastPoint().name(),
            .newP4Label = newPath.LastPoint().name(),
            .oldAliasSuffix = oldPath->GetAliasSuffix(),
            .newAliasSuffix = newPath.GetAliasSuffix(),
            .newDuplicate = newPath.GetDuplicate()};
}
