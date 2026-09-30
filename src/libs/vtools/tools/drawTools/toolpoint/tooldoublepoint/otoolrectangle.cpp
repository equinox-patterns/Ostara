/************************************************************************
 **
 **  @file   otoolrectangle.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   27/09/2026
 **
 **  @brief  a rectangle: three points and four sides, from a base point
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

#include "otoolrectangle.h"

#include <QGraphicsLineItem>
#include <QPainter>
#include <QPen>
#include <QSharedPointer>
#include <QStyleOptionGraphicsItem>
#include <QUndoStack>

#include "../../../../dialogs/tools/dialogtool.h"
#include "../../../../dialogs/tools/odialogrectangle.h"
#include "../../../../undocommands/renameobject.h"
#include "../../../../undocommands/savetooloptions.h"
#include "../../../../undocommands/vchangelinerolename.h"
#include "../../../../visualization/line/ovistoolrectangle.h"
#include "../../../../visualization/visualization.h"
#include "../../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vexception/vexception.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vwidgets/global.h"
#include "../vwidgets/scalesceneitems.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vsimplepoint.h"
#include "orectanglerule.h"
#include "vtooldoublepoint.h"

using namespace Qt::Literals::StringLiterals;

const QString OToolRectangle::ToolType = QStringLiteral("rectangle"); // NOLINT

//---------------------------------------------------------------------------------------------------------------------
OToolRectangle::OToolRectangle(const OToolRectangleInitData &initData, QGraphicsItem *parent)
  : VToolDoublePoint(initData.doc, initData.data, initData.id, initData.p2id, initData.p3id, initData.notes, parent),
    m_basePointId(initData.basePointId),
    m_formulaWidth(initData.formulaWidth),
    m_formulaHeight(initData.formulaHeight),
    m_p4id(initData.p4id)
{
    // B and C ride VToolDoublePoint, which knows them as firstPoint and secondPoint. D is ours.
    m_lineType = initData.typeLine;
    m_lineColor = initData.lineColor;

    m_fourthPoint = new VSimplePoint(m_p4id, VColorRole::PatternColor);
    m_fourthPoint->setParentItem(this);
    m_fourthPoint->setToolTip(PointToolTip(m_p4id));
    connect(m_fourthPoint, &VSimplePoint::Choosed, this,
            [this](quint32) { emit ChoosedTool(m_p4id, SceneObject::Point); });
    connect(m_fourthPoint, &VSimplePoint::Selected, this,
            [this](bool selected, quint32) { emit ChangedToolSelection(selected, m_p4id, m_id); });
    connect(m_fourthPoint, &VSimplePoint::ShowContextMenu, this, &OToolRectangle::ShowContextMenu);
    connect(m_fourthPoint, &VSimplePoint::Delete, this, &OToolRectangle::DeleteFromLabel);
    connect(m_fourthPoint, &VSimplePoint::NameChangedPosition, this,
            [this](const QPointF &pos, quint32) { UpdateNamePosition(m_p4id, pos); });
    m_fourthPoint->RefreshPointGeometry(*VAbstractTool::data.GeometricObject<VPointF>(m_p4id));

    // The base class gives B and C a generic tooltip: the rectangle names its own sides.
    firstPoint->setToolTip(PointToolTip(p1id));
    secondPoint->setToolTip(PointToolTip(p2id));

    // The four sides are children of the tool, so setLine() calls prepareGeometryChange() on their behalf and
    // a moved base point leaves no trail behind. They stack behind the points.
    const QSharedPointer<VPointF> pA = VAbstractTool::data.GeometricObject<VPointF>(m_basePointId);
    const QSharedPointer<VPointF> pB = VAbstractTool::data.GeometricObject<VPointF>(p1id);
    const QSharedPointer<VPointF> pC = VAbstractTool::data.GeometricObject<VPointF>(p2id);
    const QSharedPointer<VPointF> pD = VAbstractTool::data.GeometricObject<VPointF>(m_p4id);

    const QPointF a = static_cast<QPointF>(*pA);
    const QPointF b = static_cast<QPointF>(*pB);
    const QPointF c = static_cast<QPointF>(*pC);
    const QPointF d = static_cast<QPointF>(*pD);

    // The four sides are children of the tool and live in SCENE coordinates, like the points they join and
    // like the child items of every other tool. They only take the two settings EndLine gives its own line.
    auto MakeSide = [this](const QLineF &line) -> VScaledLine *
    {
        auto *side = new VScaledLine(line, VColorRole::CustomColor, this);
        side->SetBoldLine(false);
        side->SetShowArrows(true);
        side->setAcceptHoverEvents(true);
        side->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
        side->setZValue(-0.5); // Child line below parent point (Z=0.5 effective vs Z=1)
        side->setVisible(not line.isNull());
        return side;
    };

    m_lineAB = MakeSide(QLineF(a, b));
    m_lineBC = MakeSide(QLineF(b, c));
    m_lineCD = MakeSide(QLineF(c, d));
    m_lineDA = MakeSide(QLineF(d, a));

    ApplyPenToSides();

    // The four sides are the segments an operation may take from the rectangle: each of them is clicked on its
    // own, and the pair of points is what tells the one clicked from the other three.
    InitDrawnSegmentConnections();

    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogRectangle> dialogTool = qobject_cast<ODialogRectangle *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    dialogTool->SetBasePointId(m_basePointId);
    dialogTool->SetFormulaWidth(m_formulaWidth);
    dialogTool->SetFormulaHeight(m_formulaHeight);
    dialogTool->SetPointBName(VAbstractTool::data.GetGObject(p1id)->name());
    dialogTool->SetPointCName(VAbstractTool::data.GetGObject(p2id)->name());
    dialogTool->SetPointDName(VAbstractTool::data.GetGObject(m_p4id)->name());
    dialogTool->SetShowPointLabels(VAbstractTool::data.GeometricObject<VPointF>(p1id)->IsShowLabel(),
                                   VAbstractTool::data.GeometricObject<VPointF>(p2id)->IsShowLabel(),
                                   VAbstractTool::data.GeometricObject<VPointF>(m_p4id)->IsShowLabel());
    dialogTool->SetTypeLine(m_lineType);
    dialogTool->SetLineColor(m_lineColor);
    dialogTool->SetWidthLineRoleName(GetWidthLineRoleName());
    dialogTool->SetHeightLineRoleName(GetHeightLineRoleName());
    dialogTool->SetNotes(m_notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                            VContainer *data) -> OToolRectangle *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogRectangle> dialogTool = qobject_cast<ODialogRectangle *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolRectangleInitData initData;
    initData.basePointId = dialogTool->GetBasePointId();
    initData.formulaWidth = dialogTool->GetFormulaWidth();
    initData.formulaHeight = dialogTool->GetFormulaHeight();
    initData.name2 = dialogTool->GetPointBName();
    initData.name3 = dialogTool->GetPointCName();
    initData.name4 = dialogTool->GetPointDName();
    initData.showLabel2 = dialogTool->IsPointBLabelVisible();
    initData.showLabel3 = dialogTool->IsPointCLabelVisible();
    initData.showLabel4 = dialogTool->IsPointDLabelVisible();
    initData.widthLineRoleName = dialogTool->GetWidthLineRoleName();
    initData.heightLineRoleName = dialogTool->GetHeightLineRoleName();
    initData.typeLine = dialogTool->GetTypeLine();
    initData.lineColor = dialogTool->GetLineColor();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();

    OToolRectangle *point = Create(initData);
    if (point != nullptr)
    {
        point->m_dialog = dialog;
    }
    return point;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::Create(OToolRectangleInitData initData) -> OToolRectangle *
{
    const QSharedPointer<VPointF> basePoint = initData.data->GeometricObject<VPointF>(initData.basePointId);
    const QPointF a = static_cast<QPointF>(*basePoint);

    // The two lengths are read once, here. A formula that cannot be evaluated gives a null side rather than
    // refusing the whole rectangle: the window keeps the user on the dialog when the expression is wrong.
    qreal width = 0;
    qreal height = 0;
    try
    {
        width = CheckFormula(initData.id, initData.formulaWidth, initData.data);
        height = CheckFormula(initData.id, initData.formulaHeight, initData.data);
    }
    catch (const VExceptionBadId &e)
    {
        qWarning() << VAbstractApplication::warningMessageSignature + e.ErrorMessage();
        width = 0;
        height = 0;
    }


    const Unit appUnit = VAbstractValApplication::VApp()->patternUnits();
    const ORectangleRule::Corners corners =
        ORectangleRule::Build(a, ToPixel(width, appUnit), ToPixel(height, appUnit));

    // See VToolCurveIntersectAxis::Create: a container that already knows the id updates the object in place
    // and does not adopt the pointer it is handed, so the corners must not be read through raw pointers —
    // their names are used below, once the container has been given them.
    QSharedPointer<VPointF> pB
        = QSharedPointer<VPointF>::create(corners.b, initData.name2, initData.mx2, initData.my2, initData.id);
    pB->SetShowLabel(initData.showLabel2);

    QSharedPointer<VPointF> pC
        = QSharedPointer<VPointF>::create(corners.c, initData.name3, initData.mx3, initData.my3, initData.id);
    pC->SetShowLabel(initData.showLabel3);

    QSharedPointer<VPointF> pD
        = QSharedPointer<VPointF>::create(corners.d, initData.name4, initData.mx4, initData.my4, initData.id);
    pD->SetShowLabel(initData.showLabel4);

    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->getNextId();
        initData.p2id = initData.data->AddGObject(pB);
        initData.p3id = initData.data->AddGObject(pC);
        initData.p4id = initData.data->AddGObject(pD);
    }
    else
    {
        initData.data->UpdateGObject(initData.p2id, pB);
        initData.data->UpdateGObject(initData.p3id, pC);
        initData.data->UpdateGObject(initData.p4id, pD);
    }

    // The two lengths may carry a name of their own, which the rest of the pattern then reads as a variable.
    // They are named in the direction they are read: the width from the base point to B, the height from B to
    // C — the same two sides the window offers, with the same two pastilles.
    //
    // Set *before* the sides are registered: AddLine is what turns a pair of point names into the Line_ and
    // AngleLine_ variables, and it reads this map to alias them. Naming a line after it is registered would
    // leave the variable anonymous, which is exactly what the user named it to avoid.
    if (not initData.widthLineRoleName.isEmpty())
    {
        initData.data->SetLineRoleName(basePoint->name() + '_' + pB->name(), initData.widthLineRoleName);
    }
    if (not initData.heightLineRoleName.isEmpty())
    {
        initData.data->SetLineRoleName(pB->name() + '_' + pC->name(), initData.heightLineRoleName);
    }

    // The four sides become the line variables a pattern can name: A→B, B→C, C→D, D→A.
    initData.data->AddLine(initData.basePointId, initData.p2id, initData.id);
    initData.data->AddLine(initData.p2id, initData.p3id, initData.id);
    initData.data->AddLine(initData.p3id, initData.p4id, initData.id);
    initData.data->AddLine(initData.p4id, initData.basePointId, initData.id);

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());
    patternGraph->AddEdge(initData.basePointId, initData.id);

    patternGraph->AddVertex(initData.p2id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
    patternGraph->AddVertex(initData.p3id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
    patternGraph->AddVertex(initData.p4id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());

    patternGraph->AddEdge(initData.id, initData.p2id);
    patternGraph->AddEdge(initData.id, initData.p3id);
    patternGraph->AddEdge(initData.id, initData.p4id);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::Rectangle, initData.doc);
        auto *tool = new OToolRectangle(initData);
        initData.scene->addItem(tool);
        InitDrawToolConnections(initData.scene, tool);
        VAbstractPattern::AddTool(initData.id, tool);
        return tool;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolRectangle>(show);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::GetFormulaWidth() const -> QString
{
    return m_formulaWidth;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::GetFormulaHeight() const -> QString
{
    return m_formulaHeight;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::GetFormulaWidthF() const -> VFormula
{
    VFormula fWidth(m_formulaWidth, getData());
    fWidth.setToolId(m_id);
    fWidth.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    fWidth.Eval();
    return fWidth;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::GetFormulaHeightF() const -> VFormula
{
    VFormula fHeight(m_formulaHeight, getData());
    fHeight.setToolId(m_id);
    fHeight.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    fHeight.Eval();
    return fHeight;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetFormulaWidth(const VFormula &value)
{
    if (!value.error())
    {
        m_formulaWidth = value.GetFormula(FormulaType::FromUser);
        SaveOptionAndRebuild();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetFormulaHeight(const VFormula &value)
{
    if (!value.error())
    {
        m_formulaHeight = value.GetFormula(FormulaType::FromUser);
        SaveOptionAndRebuild();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SaveOptionAndRebuild()
{
    // Exactly what every other tool of Ostara does from the property browser — VToolParallelCurve, EndLine, Arc:
    // write the formula into the tool element through SaveOption(), which also pushes the undo command that makes
    // the change reversible with Ctrl+Z. The rectangle is not a special case and must not behave like one.
    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);

    // Then the geometry. The command SaveOption() has just pushed runs its redo() on the spot, and that redo()
    // asks the pattern to reparse — so by the time we get here the rectangle has normally already been rebuilt
    // from the new lengths. This call covers the case where it has not, and it must never take the application
    // down with it: a failure here is reported, not thrown out of commitData().
    try
    {
        UpdateLengths();
    }
    catch (const std::exception &e)
    {
        qWarning() << "OToolRectangle: cannot rebuild the geometry: " << e.what();
    }
    catch (...)
    {
        qWarning() << "OToolRectangle: cannot rebuild the geometry (unknown exception)";
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::UpdateLengths()
{
    // What a changed length has to do, and nothing else.
    //
    // NOT `Create()`: that function is the *creation* of the rectangle. It registers the tool in the pattern's
    // dependency graph, which is already built while the tool exists — calling it here added the same vertices a
    // second time and raised an error. A tool that already exists must never re-create itself.
    //
    // So the container is put directly: the three corners come from the one rule the creation and the preview
    // already share, then the four sides are registered again on the new positions — AddLine recomputes
    // Line_A_B / Line_C_D / Line_B_C / Line_D_A from those positions, which is why a changed width carries both
    // A_B and C_D: they are the same length, so they are recomputed together, never one without the other.
    if (m_basePointId == NULL_ID || p1id == NULL_ID || p2id == NULL_ID || m_p4id == NULL_ID)
    {
        return;
    }

    const QPointF a = static_cast<QPointF>(*VAbstractTool::data.GeometricObject<VPointF>(m_basePointId));

    // CheckFormula takes the expression by non-const reference, so it works on the members themselves.
    const Unit appUnit = VAbstractValApplication::VApp()->patternUnits();
    const ORectangleRule::Corners corners =
        ORectangleRule::Build(a, ToPixel(CheckFormula(m_id, m_formulaWidth, getData()), appUnit),
                              ToPixel(CheckFormula(m_id, m_formulaHeight, getData()), appUnit));

    const QSharedPointer<VPointF> oldB = VAbstractTool::data.GeometricObject<VPointF>(p1id);
    const QSharedPointer<VPointF> oldC = VAbstractTool::data.GeometricObject<VPointF>(p2id);
    const QSharedPointer<VPointF> oldD = VAbstractTool::data.GeometricObject<VPointF>(m_p4id);

    // Only the position moves: the name, the label offset and the "show the name" box of a corner are the
    // user's, and a changed length has no business touching them.
    auto ReplacePoint = [](const QSharedPointer<VPointF> &old, const QPointF &pos) -> QSharedPointer<VPointF>
    {
        auto *point = new VPointF(pos, old->name(), old->mx(), old->my());
        point->SetShowLabel(old->IsShowLabel());
        return QSharedPointer<VPointF>(point);
    };

    VAbstractTool::data.UpdateGObject(p1id, ReplacePoint(oldB, corners.b));
    VAbstractTool::data.UpdateGObject(p2id, ReplacePoint(oldC, corners.c));
    VAbstractTool::data.UpdateGObject(m_p4id, ReplacePoint(oldD, corners.d));

    // The four sides, registered again on the positions that have just been written. This is what carries the
    // change to the rest of the pattern: everything that reads Line_<name> reads it from here.
    VAbstractTool::data.AddLine(m_basePointId, p1id, m_id); // A→B
    VAbstractTool::data.AddLine(p1id, p2id, m_id);          // B→C
    VAbstractTool::data.AddLine(p2id, m_p4id, m_id);        // C→D
    VAbstractTool::data.AddLine(m_p4id, m_basePointId, m_id); // D→A

    // The container is true; put it on screen — the sides, the three handles, and the item itself.
    FullUpdateFromFile();

    // A rectangle can grow past the edge of the scene, which is sized for what it holds.
    if (QGraphicsScene *sc = scene())
    {
        VMainGraphicsView::NewSceneRect(sc, VAbstractValApplication::VApp()->getSceneView(), this);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetLineColor(const QString &value)
{
    m_lineColor = value;
    // Apply before the file round-trip: SaveOption ends in a light reparse, which does redraw the sides, but
    // the click must not wait for a parse to look right.
    ApplyPenToSides();
    update();
    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetLineType(const QString &value)
{
    m_lineType = value;
    ApplyPenToSides();
    update();
    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::BasePointName() const -> QString
{
    return VAbstractTool::data.GetGObject(m_basePointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::nameP4() const -> QString
{
    return VAbstractTool::data.GetGObject(m_p4id)->name();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::setNameP4(const QString &name)
{
    // The base class names its two points' ToolChanges after its own shape; UpdatePointName below speaks the
    // shape VAbstractPoint defines, so the fully qualified name is not decoration here.
    UpdatePointName(m_p4id, name,
                    [this](const QDomElement &oldElem, const QDomElement &newElem,
                           const VAbstractPoint::ToolChanges &changes) -> void
                    { ProcessPointToolOptions(oldElem, newElem, changes); });
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::IsLabelVisible(quint32 id) const -> bool
{
    // D first: the base class only ever looks at p1id and p2id, and answers false for anything else.
    if (m_p4id == id)
    {
        return VAbstractTool::data.GeometricObject<VPointF>(m_p4id)->IsShowLabel();
    }
    return VToolDoublePoint::IsLabelVisible(id);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetLabelVisible(quint32 id, bool visible)
{
    // D is the fourth point, which the base class does not carry — without this branch the "show the name" box
    // of D would ask to be checked, get nothing, and uncheck itself on the next refresh.
    if (m_p4id == id)
    {
        const QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(m_p4id);
        point->SetShowLabel(visible);
        m_fourthPoint->RefreshPointGeometry(*point);

        QDomElement element = doc->FindElementById(m_id, getTagName());
        if (element.isElement())
        {
            doc->SetAttribute<bool>(element, AttrShowLabel3, visible);
        }
        if (QGraphicsScene *sc = m_fourthPoint->scene())
        {
            VMainGraphicsView::NewSceneRect(sc, VAbstractValApplication::VApp()->getSceneView(), m_fourthPoint);
        }
        return;
    }

    VToolDoublePoint::SetLabelVisible(id, visible);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::GroupVisibility(quint32 object, bool visible)
{
    // A rectangle is one figure: the three points it creates — B, C, D — and the four sides that join them. The
    // sides are drawn by this tool and are not objects of their own, so no layer could ever reach them: hiding
    // the points alone left the four sides on the canvas, a rectangle that refuses to disappear. Whichever of
    // the three points a layer changes, the whole figure follows it.
    if (object != p1id && object != p2id && object != m_p4id)
    {
        // Not one of ours: the base knows about the two points a double point already had.
        VToolDoublePoint::GroupVisibility(object, visible);
        return;
    }

    firstPoint->setVisible(visible);
    secondPoint->setVisible(visible);
    m_fourthPoint->setVisible(visible);

    for (VScaledLine *side : {m_lineAB, m_lineBC, m_lineCD, m_lineDA})
    {
        // A side whose two points coincide is not drawn at all — MakeSide decides that once, and showing the
        // figure again must not bring that empty line back.
        side->setVisible(visible && not side->line().isNull());
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::WidthPairKey() const -> QString
{
    return BasePointName() + '_' + VAbstractTool::data.GetGObject(p1id)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::HeightPairKey() const -> QString
{
    return VAbstractTool::data.GetGObject(p1id)->name() + '_' + VAbstractTool::data.GetGObject(p2id)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::GetWidthLineRoleName() const -> QString
{
    return VAbstractTool::data.LineRoleName(WidthPairKey());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetWidthLineRoleName(const QString &suffix)
{
    SetLineRoleNameForPair(WidthPairKey(), suffix);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::GetHeightLineRoleName() const -> QString
{
    return VAbstractTool::data.LineRoleName(HeightPairKey());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetHeightLineRoleName(const QString &suffix)
{
    SetLineRoleNameForPair(HeightPairKey(), suffix);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetLineRoleNameForPair(const QString &pairKey, const QString &suffix)
{
    const QString oldSuffix = VAbstractTool::data.LineRoleName(pairKey);
    if (not suffix.isEmpty() && not VAbstractTool::data.IsUnique(line_ + suffix))
    {
        return;
    }

    if (oldSuffix == suffix)
    {
        return;
    }

    VAbstractTool::data.SetLineRoleName(pairKey, suffix);

    auto *command = new VChangeLineRoleName(doc, pairKey, oldSuffix, suffix);
    connect(command, &VChangeLineRoleName::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    VAbstractApplication::VApp()->getUndoStack()->push(command);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    try
    {
        ContextMenu<ODialogRectangle>(event, id);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::ApplyPenToSides()
{
    // Every side takes the type and the colour the user chose, and the pen is built the way EndLine builds its
    // own line — through the same two helpers, so a side of a rectangle is an ordinary line of the canvas. The
    // two lengths carry their own colour only while the rectangle is selected, which itemChange handles.
    const QColor chosenColor = VSceneStylesheet::CorrectToolColor(
        this, VSceneStylesheet::CorrectToolColorForDarkTheme(QColor(m_lineColor)));
    const Qt::PenStyle chosenStyle = LineStyleToPenStyle(m_lineType);

    for (VScaledLine *line : {m_lineAB, m_lineBC, m_lineCD, m_lineDA})
    {
        QPen linePen = line->pen();
        linePen.setColor(chosenColor);
        linePen.setStyle(chosenStyle);
        line->setPen(linePen);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::DimensionLines() const -> QVector<VScaledLine *>
{
    // The top side is the width, the right one is the height: the very two sides the pastilles of the window
    // name, and the two the preview draws in the support colours.
    return {m_lineAB, m_lineBC};
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::UpdateDimensionHighlight()
{
    UpdateDimensionHighlight(isSelected());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::UpdateDimensionHighlight(bool selected)
{
    // The two colours mean something only while the rectangle is selected: they say which side the green
    // pastille and the red pastille are about. Outside of a selection, the four sides are ordinary sides, in
    // the colour the user chose.
    //
    // The line is made bold at the same time, which is what VScaledLine calls being highlighted — the pen
    // thickens and the direction arrows come out. Bold alone would paint both lengths the same, so the colour
    // is set from the same two roles the preview and the pastilles use.
    const Qt::PenStyle chosenStyle = LineStyleToPenStyle(m_lineType);

    auto Apply = [&](VScaledLine *line, bool lit, VColorRole role) -> void
    {
        QPen linePen = line->pen();
        linePen.setColor(lit ? VSceneStylesheet::Color(role)
                             : VSceneStylesheet::CorrectToolColor(
                                   this, VSceneStylesheet::CorrectToolColorForDarkTheme(QColor(m_lineColor))));
        linePen.setStyle(chosenStyle);
        line->SetBoldLine(lit);
        line->setPen(linePen);
        line->RefreshPen(); // takes the bold flag into account for the width, and the colour above is kept
        line->update();
    };

    Apply(m_lineAB, selected, VColorRole::VisSupportColor2); // the width, green
    Apply(m_lineBC, selected, VColorRole::VisSupportColor4); // the height, red
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant
{
    if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        // Called before the flag is applied, so the new state is the one passed in — asking isSelected() here
        // would still give the old one and the highlight would always lag one click behind.
        UpdateDimensionHighlight(value.toBool());
    }

    return VToolDoublePoint::itemChange(change, value);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // The four sides draw themselves; the tool draws nothing of its own. This exists for one reason: the
    // selection highlight. It is the helper EndLine reaches through its line, and without it a selected
    // rectangle shows no sign of being selected.
    PaintWithFixItemHighlightSelected<VToolDoublePoint>(this, painter, option, widget);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SaveDialog(QDomElement &domElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogRectangle> dialogTool = qobject_cast<ODialogRectangle *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    doc->SetAttribute(domElement, AttrBasePoint, QString().setNum(dialogTool->GetBasePointId()));
    doc->SetAttribute(domElement, AttrWidth, dialogTool->GetFormulaWidth());
    doc->SetAttribute(domElement, AttrHeight, dialogTool->GetFormulaHeight());
    doc->SetAttribute(domElement, AttrName1, dialogTool->GetPointBName());
    doc->SetAttribute(domElement, AttrName2, dialogTool->GetPointCName());
    doc->SetAttribute(domElement, AttrName3, dialogTool->GetPointDName());

    // The three "show the name" boxes travel with the names: the parser reads them back on every load, so
    // leaving them out silently reset D's box (and B's and C's) at each reopening.
    doc->SetAttribute<bool>(domElement, AttrShowLabel1, dialogTool->IsPointBLabelVisible());
    doc->SetAttribute<bool>(domElement, AttrShowLabel2, dialogTool->IsPointCLabelVisible());
    doc->SetAttribute<bool>(domElement, AttrShowLabel3, dialogTool->IsPointDLabelVisible());

    doc->SetAttribute(domElement, AttrTypeLine, dialogTool->GetTypeLine());
    doc->SetAttribute(domElement, AttrLineColor, dialogTool->GetLineColor());
    doc->SetAttributeOrRemoveIf<QString>(domElement, AttrNotes, dialogTool->GetNotes(),
                                         [](const QString &notes) noexcept -> bool { return notes.isEmpty(); });
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolDoublePoint::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrBasePoint, m_basePointId);
    doc->SetAttribute(tag, AttrWidth, m_formulaWidth);
    doc->SetAttribute(tag, AttrHeight, m_formulaHeight);
    doc->SetAttribute(tag, AttrTypeLine, m_lineType);
    doc->SetAttribute(tag, AttrLineColor, m_lineColor);

    // D is not one of the two points VToolDoublePoint writes, so it is written here, under the same names
    // the other two use.
    const QSharedPointer<VPointF> pD = VAbstractTool::data.GeometricObject<VPointF>(m_p4id);
    doc->SetAttribute(tag, AttrPoint4, m_p4id);
    doc->SetAttribute(tag, AttrName3, pD->name());
    doc->SetAttribute(tag, AttrMx3, VAbstractValApplication::VApp()->fromPixel(pD->mx()));
    doc->SetAttribute(tag, AttrMy3, VAbstractValApplication::VApp()->fromPixel(pD->my()));
    doc->SetAttribute<bool>(tag, AttrShowLabel3, pD->IsShowLabel());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::ReadToolAttributes(const QDomElement &domElement)
{
    VToolDoublePoint::ReadToolAttributes(domElement);

    m_basePointId = VAbstractPattern::GetParametrUInt(domElement, AttrBasePoint, NULL_ID_STR);
    m_formulaWidth = VAbstractPattern::GetParametrString(domElement, AttrWidth);
    m_formulaHeight = VAbstractPattern::GetParametrString(domElement, AttrHeight);
    m_lineType = VAbstractPattern::GetParametrString(domElement, AttrTypeLine, TypeLineLine);
    m_lineColor = VAbstractPattern::GetParametrString(domElement, AttrLineColor, ColorBlack);
    m_p4id = VAbstractPattern::GetParametrUInt(domElement, AttrPoint4, NULL_ID_STR);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<OVisToolRectangle *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetBasePointId(m_basePointId);
        visual->SetFormulaWidth(m_formulaWidth);
        visual->SetFormulaHeight(m_formulaHeight);
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogRectangle> dialogTool = qobject_cast<ODialogRectangle *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    const QString newNameB = dialogTool->GetPointBName();
    const QString newNameC = dialogTool->GetPointCName();
    const QString newNameD = dialogTool->GetPointDName();

    const QString oldNameB = VAbstractTool::data.GetGObject(p1id)->name();
    const QString oldNameC = VAbstractTool::data.GetGObject(p2id)->name();
    const QString oldNameD = VAbstractTool::data.GetGObject(m_p4id)->name();

    // The two lengths can be renamed as well, and a renamed length is a variable the whole pattern reads: its
    // own undo command rewrites the formulas that used the old name, so it is pushed on its own.
    const QString newWidthRoleName = dialogTool->GetWidthLineRoleName();
    const QString newHeightRoleName = dialogTool->GetHeightLineRoleName();

    const bool hasNameChanges = (oldNameB != newNameB) || (oldNameC != newNameC) || (oldNameD != newNameD);

    if (not hasNameChanges)
    {
        VAbstractPoint::ApplyToolOptions(oldDomElement, newDomElement);
    }
    else
    {
        QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
        auto *newGroup = new QUndoCommand();
        newGroup->setText(tr("save tool options"));

        auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id, newGroup);
        saveOptions->SetInGroup(true);
        // The names live in the file as attributes of this very element, so one lite parse at the end covers
        // them all — the rename commands below only have to move the labels on screen.
        connect(saveOptions, &SaveToolOptions::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);

        if (oldNameB != newNameB)
        {
            new RenameLabel(oldNameB, newNameB, doc, p1id, newGroup);
        }
        if (oldNameC != newNameC)
        {
            new RenameLabel(oldNameC, newNameC, doc, p2id, newGroup);
        }
        if (oldNameD != newNameD)
        {
            new RenameLabel(oldNameD, newNameD, doc, m_p4id, newGroup);
        }

        undoStack->push(newGroup);
    }

    // The two lengths are renamed last, and never inside the group above: the key a named length is filed
    // under is made of the point names, so a length has to be named once the points have their new ones —
    // and its own command is what rewrites the formulas that read the old name.
    SetWidthLineRoleName(newWidthRoleName);
    SetHeightLineRoleName(newHeightRoleName);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolRectangle::PointToolTip(quint32 id) const -> QString
{
    const QSharedPointer<VPointF> pA = VAbstractTool::data.GeometricObject<VPointF>(m_basePointId);
    const QSharedPointer<VPointF> pB = VAbstractTool::data.GeometricObject<VPointF>(p1id);
    const QSharedPointer<VPointF> pC = VAbstractTool::data.GeometricObject<VPointF>(p2id);
    const QSharedPointer<VPointF> pD = VAbstractTool::data.GeometricObject<VPointF>(m_p4id);
    const QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(id);

    QString name1;
    QString name2;
    QLineF side1;
    QLineF side2;

    if (id == p1id) // B: the sides A→B and B→C
    {
        name1 = u"%1→%2"_s.arg(pA->name(), pB->name());
        name2 = u"%1→%2"_s.arg(pB->name(), pC->name());
        side1 = QLineF(static_cast<QPointF>(*pA), static_cast<QPointF>(*pB));
        side2 = QLineF(static_cast<QPointF>(*pB), static_cast<QPointF>(*pC));
    }
    else if (id == p2id) // C: the sides B→C and C→D
    {
        name1 = u"%1→%2"_s.arg(pB->name(), pC->name());
        name2 = u"%1→%2"_s.arg(pC->name(), pD->name());
        side1 = QLineF(static_cast<QPointF>(*pB), static_cast<QPointF>(*pC));
        side2 = QLineF(static_cast<QPointF>(*pC), static_cast<QPointF>(*pD));
    }
    else // D: the sides C→D and D→A
    {
        name1 = u"%1→%2"_s.arg(pC->name(), pD->name());
        name2 = u"%1→%2"_s.arg(pD->name(), pA->name());
        side1 = QLineF(static_cast<QPointF>(*pC), static_cast<QPointF>(*pD));
        side2 = QLineF(static_cast<QPointF>(*pD), static_cast<QPointF>(*pA));
    }

    const QString unit = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);
    const qreal l1 = VAbstractValApplication::VApp()->fromPixel(side1.length());
    const qreal l2 = VAbstractValApplication::VApp()->fromPixel(side2.length());

    const QString rowLabel = u"<tr> <td><b>%1:</b> %2</td> </tr>"_s.arg(tr("Label"), point->name());
    const QString rowSide1 =
        u"<tr> <td><b>%1:</b> %2 %3, %4°</td> </tr>"_s.arg(name1).arg(l1).arg(unit).arg(side1.angle());
    const QString rowSide2 =
        u"<tr> <td><b>%1:</b> %2 %3, %4°</td> </tr>"_s.arg(name2).arg(l2).arg(unit).arg(side2.angle());

    return u"<table>%1%2%3</table>"_s.arg(rowLabel, rowSide1, rowSide2);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::RefreshGeometry()
{
    const QSharedPointer<VPointF> pA = VAbstractTool::data.GeometricObject<VPointF>(m_basePointId);
    const QSharedPointer<VPointF> pB = VAbstractTool::data.GeometricObject<VPointF>(p1id);
    const QSharedPointer<VPointF> pC = VAbstractTool::data.GeometricObject<VPointF>(p2id);
    const QSharedPointer<VPointF> pD = VAbstractTool::data.GeometricObject<VPointF>(m_p4id);

    m_lineAB->setLine(QLineF(static_cast<QPointF>(*pA), static_cast<QPointF>(*pB)));
    m_lineBC->setLine(QLineF(static_cast<QPointF>(*pB), static_cast<QPointF>(*pC)));
    m_lineCD->setLine(QLineF(static_cast<QPointF>(*pC), static_cast<QPointF>(*pD)));
    m_lineDA->setLine(QLineF(static_cast<QPointF>(*pD), static_cast<QPointF>(*pA)));

    // Every path that redraws the rectangle comes through here — creation, a moved point, a changed option, a
    // light reparse — so this is where the pen is put back, and where the two lengths are lit again if the
    // rectangle happens to be selected while it moves.
    ApplyPenToSides();
    UpdateDimensionHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::FullUpdateFromFile()
{
    VToolDoublePoint::FullUpdateFromFile();
    firstPoint->setToolTip(PointToolTip(p1id));
    secondPoint->setToolTip(PointToolTip(p2id));
    m_fourthPoint->setToolTip(PointToolTip(m_p4id));
    m_fourthPoint->RefreshPointGeometry(*VAbstractTool::data.GeometricObject<VPointF>(m_p4id));
    RefreshGeometry();
    update();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::AllowHover(bool enabled)
{
    VToolDoublePoint::AllowHover(enabled);
    m_fourthPoint->setAcceptHoverEvents(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::AllowSelecting(bool enabled)
{
    VToolDoublePoint::AllowSelecting(enabled);
    m_fourthPoint->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolRectangle::Enable()
{
    VToolDoublePoint::Enable();
    const bool enabled = m_indexActivePatternBlock == doc->PatternBlockMapper()->GetActiveId();
    m_fourthPoint->SetEnabled(enabled);
}
