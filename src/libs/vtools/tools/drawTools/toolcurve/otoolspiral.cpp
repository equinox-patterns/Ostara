/************************************************************************
 **
 **  @file   otoolspiral.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   01/10/2026
 **
 **  @brief  Spiral tool: builds the golden spiral from its start point,
 **          its first quarter radius, its length and its rotation, and
 **          materializes the arrival as a real point.
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

#include "otoolspiral.h"

#include <QDomElement>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QUndoStack>
#include <new>

#include "../../../dialogs/tools/odialogspiral.h"
#include "../../../visualization/path/ovistoolspiral.h"
#include "../../../visualization/visualization.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/ospiral.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/literals.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../toolpoint/toolsinglepoint/otoolcurvepoint.h"
#include "vexception.h"
#include "vtoolabstractcurve.h"

const QString OToolSpiral::ToolType = QStringLiteral("spiral");

//---------------------------------------------------------------------------------------------------------------------
OToolSpiral::OToolSpiral(const OToolSpiralInitData &initData, QGraphicsItem *parent)
  : VToolAbstractBezier(initData.doc, initData.data, initData.id, initData.notes, parent),
    m_startPointId(initData.startPointId),
    m_formulaRadius(initData.formulaRadius),
    m_formulaLength(initData.formulaLength),
    m_formulaAngle(initData.formulaAngle)
{
    SetSceneType(SceneObject::SplinePath);

    this->setFlag(QGraphicsItem::ItemIsFocusable, true); // For keyboard input focus

    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogSpiral> dialogTool = qobject_cast<ODialogSpiral *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    const QSharedPointer<VSplinePath> splPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);

    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetStartPointId(m_startPointId);
    dialogTool->SetFormulaRadius(m_formulaRadius);
    dialogTool->SetFormulaLength(m_formulaLength);
    dialogTool->SetFormulaAngle(m_formulaAngle);
    dialogTool->SetPenStyle(splPath->GetPenStyle());
    dialogTool->SetColor(splPath->GetColor());
    dialogTool->SetAliasSuffix(splPath->GetAliasSuffix());
    dialogTool->SetEndPointName(EndPointName());
    dialogTool->SetEndPointId(FindPointId(false));
    dialogTool->SetEndShowLabel(EndPointLabelVisible());
    dialogTool->SetNotes(m_notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpiral::Create(const QPointer<DialogTool> &dialog,
                         VMainGraphicsScene *scene,
                         VAbstractPattern *doc,
                         VContainer *data) -> OToolSpiral *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogSpiral> dialogTool = qobject_cast<ODialogSpiral *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolSpiralInitData initData;
    initData.startPointId = dialogTool->GetStartPointId();
    initData.formulaRadius = dialogTool->GetFormulaRadius();
    initData.formulaLength = dialogTool->GetFormulaLength();
    initData.formulaAngle = dialogTool->GetFormulaAngle();
    initData.endPointName = dialogTool->GetEndPointName();
    initData.endShowLabel = dialogTool->GetEndShowLabel();
    initData.color = dialogTool->GetColor();
    initData.penStyle = dialogTool->GetPenStyle();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();
    initData.aliasSuffix = dialogTool->GetAliasSuffix();

    OToolSpiral *spiral = Create(initData);
    if (spiral != nullptr)
    {
        spiral->m_dialog = dialog;
    }
    return spiral;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpiral::Create(OToolSpiralInitData &initData) -> OToolSpiral *
{
    VAbstractValApplication *app = VAbstractValApplication::VApp();
    const QSharedPointer<VPointF> start = initData.data->GeometricObject<VPointF>(initData.startPointId);

    // The dialog works in pattern units, the geometry in pixels.
    const qreal radius = app->toPixel(CheckFormula(initData.id, initData.formulaRadius, initData.data));
    const qreal length = app->toPixel(CheckFormula(initData.id, initData.formulaLength, initData.data));
    const qreal angle = CheckFormula(initData.id, initData.formulaAngle, initData.data);

    // The arrival point names the last knot of the spiral, and the two ends are what name the curve. Should a
    // file have lost that point, the start point's name keeps the curve referenceable rather than nameless.
    const QString endName = initData.endPointName.isEmpty() ? start->name() : initData.endPointName;

    VSplinePath splPath = OSpiral::Build(*start, radius, length, angle, endName);
    splPath.SetColor(initData.color);
    splPath.SetPenStyle(initData.penStyle);
    splPath.SetApproximationScale(initData.approximationScale);
    splPath.SetAliasSuffix(initData.aliasSuffix);

    // The native name is made of the two ends, so a second spiral leaving the same point would carry the name of
    // the first. The counter is what separates them — the answer every dialog that names a path after its ends
    // gets, applied here because no dialog names this one. A file hands back the counter it was created with:
    // recomputing it would rename the curve and break the formulas that point at it.
    if (initData.duplicate > 0)
    {
        splPath.SetDuplicate(initData.duplicate);
    }
    else if (initData.typeCreation == Source::FromGui && not initData.data->IsUnique(splPath.name()))
    {
        splPath.SetDuplicate(initData.data->UniqueDuplicate(splPath.name()));
    }

    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->AddGObject(new VSplinePath(splPath));
    }
    else
    {
        initData.data->UpdateGObject(initData.id, splPath);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();
    initData.doc->FindFormulaDependencies(initData.formulaRadius, initData.id, varData);
    initData.doc->FindFormulaDependencies(initData.formulaLength, initData.id, varData);
    initData.doc->FindFormulaDependencies(initData.formulaAngle, initData.id, varData);

    // AddSpline and not AddCurveWithSegments: the knots of a spiral are samples, not points of the pattern, so a
    // measure per segment would only fill the variable list with names nobody can read. The spiral is one curve,
    // and Curve_<name> is its length.
    initData.data->AddSpline(initData.data->GeometricObject<VAbstractBezier>(initData.id), initData.id);

    patternGraph->AddEdge(initData.startPointId, initData.id);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::Spiral, initData.doc);

        // The spiral and the point it arrives at are one gesture: each creation pushes the command that writes
        // it into the file, so without a macro around them one Ctrl+Z takes the point away and leaves half a
        // tool on the canvas, and the next one takes the curve. The two go in together, and come out together.
        QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
        const bool grouped = initData.typeCreation == Source::FromGui && undoStack != nullptr;
        if (grouped)
        {
            undoStack->beginMacro(tr("add a spiral"));
        }

        auto *spiral = new OToolSpiral(initData);
        initData.scene->addItem(spiral);
        InitSplinePathToolConnections(initData.scene, spiral);
        VAbstractPattern::AddTool(initData.id, spiral);

        // The arrival point is always created, and only a gesture creates it: a reparse finds the one the file
        // already carries. It is born on the last knot of the spiral, which is why it comes after the curve.
        if (initData.typeCreation == Source::FromGui)
        {
            OToolCurvePointInitData pointData;
            pointData.splineId = initData.id;
            pointData.position = CurvePointPosition::End;
            pointData.name = endName;
            pointData.showLabel = initData.endShowLabel;
            pointData.scene = initData.scene;
            pointData.doc = initData.doc;
            pointData.data = initData.data;
            pointData.parse = initData.parse;
            pointData.typeCreation = Source::FromGui;
            OToolCurvePoint::Create(pointData);
        }

        if (grouped)
        {
            undoStack->endMacro();
        }

        return spiral;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpiral::GetFormulaRadius() const -> VFormula
{
    VFormula radius(m_formulaRadius, getData());
    radius.setToolId(m_id);
    radius.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    radius.Eval();
    return radius;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::SetFormulaRadius(const VFormula &value)
{
    if (!value.error())
    {
        m_formulaRadius = value.GetFormula(FormulaType::FromUser);

        QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
        SaveOption(obj);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpiral::GetFormulaLength() const -> VFormula
{
    VFormula length(m_formulaLength, getData());
    length.setToolId(m_id);
    length.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    length.Eval();
    return length;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::SetFormulaLength(const VFormula &value)
{
    if (!value.error())
    {
        m_formulaLength = value.GetFormula(FormulaType::FromUser);

        QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
        SaveOption(obj);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpiral::GetFormulaAngle() const -> VFormula
{
    VFormula angle(m_formulaAngle, getData());
    angle.setToolId(m_id);
    angle.setPostfix(degreeSymbol);
    angle.Eval();
    return angle;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::SetFormulaAngle(const VFormula &value)
{
    if (!value.error())
    {
        m_formulaAngle = value.GetFormula(FormulaType::FromUser);

        QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
        SaveOption(obj);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpiral::StartPointName() const -> QString
{
    const QSharedPointer<VSplinePath> path = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);
    return path->FirstPoint().name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpiral::EndPointName() const -> QString
{
    const QSharedPointer<VSplinePath> path = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);
    return path->LastPoint().name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpiral::EndPointLabelVisible() const -> bool
{
    const quint32 pointId = FindPointId(false);
    if (pointId == NULL_ID)
    {
        return false;
    }

    // The name belongs to the point, and the point is what knows it: asking it reads the same truth the setter
    // writes. Reading it here, on this curve's own copy of the container, would throw — that copy is a snapshot
    // taken when the curve was built, before the arrival point existed, and Qt detaches it the moment the point
    // is added to the pattern.
    if (auto *point = dynamic_cast<VDrawTool *>(VAbstractPattern::FindTool(pointId)))
    {
        return point->IsLabelVisible(pointId);
    }

    return false;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::SetEndPointLabelVisible(bool visible)
{
    const quint32 pointId = FindPointId(false);
    if (pointId == NULL_ID)
    {
        return;
    }

    // The name belongs to the point, and the point knows how to write it and redraw itself: asking it is what
    // keeps this switch and the point's own entry in the property browser in step.
    if (auto *point = dynamic_cast<VDrawTool *>(VAbstractPattern::FindTool(pointId)))
    {
        point->SetLabelVisible(pointId, visible);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolSpiral>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    Q_UNUSED(id)
    try
    {
        ContextMenu<ODialogSpiral>(event);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::SaveDialog(QDomElement &domElement)
{
    SCASSERT(not m_dialog.isNull())
    QPointer<ODialogSpiral> const dialogTool = qobject_cast<ODialogSpiral *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    doc->SetAttribute(domElement, AttrType, ToolType);
    doc->SetAttribute(domElement, AttrPoint1, dialogTool->GetStartPointId());
    doc->SetAttribute(domElement, AttrRadius, dialogTool->GetFormulaRadius());
    doc->SetAttribute(domElement, AttrLength, dialogTool->GetFormulaLength());
    doc->SetAttribute(domElement, AttrAngle, dialogTool->GetFormulaAngle());
    doc->SetAttribute(domElement, AttrColor, dialogTool->GetColor());
    doc->SetAttribute(domElement, AttrPenStyle, dialogTool->GetPenStyle());
    doc->SetAttributeOrRemoveIf<QString>(domElement,
                                         AttrAlias,
                                         dialogTool->GetAliasSuffix(),
                                         [](const QString &suffix) noexcept -> bool { return suffix.isEmpty(); });
    doc->SetAttributeOrRemoveIf<QString>(domElement,
                                         AttrNotes,
                                         dialogTool->GetNotes(),
                                         [](const QString &notes) noexcept -> bool { return notes.isEmpty(); });
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolAbstractCurve::SaveOptions(tag, obj);

    const QSharedPointer<VSplinePath> path = qSharedPointerDynamicCast<VSplinePath>(obj);
    SCASSERT(not path.isNull())

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrPoint1, m_startPointId);
    doc->SetAttribute(tag, AttrRadius, m_formulaRadius);
    doc->SetAttribute(tag, AttrLength, m_formulaLength);
    doc->SetAttribute(tag, AttrAngle, m_formulaAngle);
    doc->SetAttributeOrRemoveIf<quint32>(tag,
                                         AttrDuplicate,
                                         path->GetDuplicate(),
                                         [](quint32 duplicate) noexcept { return duplicate == 0; });
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::ReadToolAttributes(const QDomElement &domElement)
{
    VDrawTool::ReadToolAttributes(domElement);

    m_startPointId = VDomDocument::GetParametrUInt(domElement, AttrPoint1, NULL_ID_STR);
    m_formulaRadius = VDomDocument::GetParametrString(domElement, AttrRadius, QChar('0'));
    m_formulaLength = VDomDocument::GetParametrString(domElement, AttrLength, QChar('0'));
    m_formulaAngle = VDomDocument::GetParametrString(domElement, AttrAngle, QChar('0'));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ProcessSplinePathToolOptions(oldDomElement, newDomElement, GatherToolChanges());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolSpiral::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<OVisToolSpiral *>(vis);
        SCASSERT(visual != nullptr)

        const QSharedPointer<VSplinePath> path = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);
        visual->SetStartPointId(m_startPointId);
        visual->SetPath(*path);
        visual->SetLineStyle(LineStyleToPenStyle(path->GetPenStyle()));
        visual->SetApproximationScale(path->GetApproximationScale());
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolSpiral::GatherToolChanges() const -> ToolChanges
{
    const QSharedPointer<VSplinePath> path = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);

    // The two ends of a spiral do not move: only its alias is edited here, so both labels are handed over
    // unchanged and the rename group is left with the alias alone to record.
    ToolChanges changes;
    changes.oldP1Label = path->FirstPoint().name();
    changes.newP1Label = changes.oldP1Label;
    changes.oldP4Label = path->LastPoint().name();
    changes.newP4Label = changes.oldP4Label;
    changes.oldAliasSuffix = path->GetAliasSuffix();

    if (const QPointer<ODialogSpiral> dialogTool = qobject_cast<ODialogSpiral *>(m_dialog); not dialogTool.isNull())
    {
        changes.newAliasSuffix = dialogTool->GetAliasSuffix();
    }
    else
    {
        changes.newAliasSuffix = changes.oldAliasSuffix;
    }

    return changes;
}
