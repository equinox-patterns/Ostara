/************************************************************************
 **
 **  @file   vtoolendline.cpp
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
 **  Modified for Ostara (fork of Valentina) on 18/10/2026.
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

#include "vtoolendline.h"

#include <QLineF>
#include <QPointF>
#include <QSharedPointer>

#include "../../../../../dialogs/tools/dialogendline.h"
#include "../../../../../dialogs/tools/dialogtool.h"
#include "../../../../../undocommands/vchangelinerolename.h"
#include "../../../../../visualization/line/vistoolendline.h"
#include "../../../../../visualization/visualization.h"
#include "../../../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vpointf.h"
#include "vexception.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "vtoollinepoint.h"

template <class T> class QSharedPointer;

const QString VToolEndLine::ToolType = QStringLiteral("endLine");

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VToolEndLine constructor.
 * @param initData init data.
 * @param parent parent object.
 */
VToolEndLine::VToolEndLine(const VToolEndLineInitData &initData, QGraphicsItem *parent)
  : VToolLinePoint(initData.doc, initData.data, initData.id, initData.typeLine, initData.lineColor,
                   initData.formulaLength, initData.basePointId, 0, initData.notes, parent),
    formulaAngle(initData.formulaAngle)
{
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setDialog set dialog when user want change tool option.
 */
void VToolEndLine::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    m_dialog->setModal(true);
    QPointer<DialogEndLine> const dialogTool = qobject_cast<DialogEndLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    const QSharedPointer<VPointF> p = VAbstractTool::data.GeometricObject<VPointF>(m_id);
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetTypeLine(m_lineType);
    dialogTool->SetLineColor(lineColor);
    dialogTool->SetFormula(formulaLength);
    dialogTool->SetAngle(EffectiveAngleFormula());
    dialogTool->SetAxis(m_axis);
    dialogTool->SetBasePointId(basePointId);
    dialogTool->SetPointName(p->name());
    dialogTool->SetNotes(m_notes);
    dialogTool->SetLineRoleName(GetLineRoleName());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create help create tool from GUI.
 * @param dialog dialog.
 * @param scene pointer to scene.
 * @param doc dom document container.
 * @param data container with variables.
 * @return the created tool
 */
auto VToolEndLine::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                          VContainer *data) -> VToolEndLine *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogEndLine> dialogTool = qobject_cast<DialogEndLine *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolEndLineInitData initData;
    initData.formulaLength = dialogTool->GetFormula();
    initData.formulaAngle = dialogTool->GetAngle();
    initData.axis = dialogTool->GetAxis();
    initData.basePointId = dialogTool->GetBasePointId();
    initData.typeLine = dialogTool->GetTypeLine();
    initData.lineColor = dialogTool->GetLineColor();
    initData.name = dialogTool->GetPointName();
    initData.showLabel = dialogTool->GetShowLabel();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();
    initData.lineRoleName = dialogTool->GetLineRoleName();

    VToolEndLine *point = Create(initData);
    if (point != nullptr)
    {
        point->m_dialog = dialog;
    }
    return point;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Where the point lands for a given direction: the base point, moved by the length formula along the angle.
 *
 *  One construction for the two callers — the creation of the tool and the living preview of an axis — so the
 *  preview can never show a place the real tool would not take. */
auto EndLinePoint(const VContainer *data, quint32 id, quint32 basePointId, const QString &formulaLength,
                  qreal angle) -> QPointF
{
    const QSharedPointer<VPointF> basePoint = data->GeometricObject<VPointF>(basePointId);
    auto line = QLineF(static_cast<QPointF>(*basePoint), QPointF(basePoint->x() + 100, basePoint->y()));

    line.setAngle(angle);
    QString length = formulaLength;
    line.setLength(VAbstractValApplication::VApp()->toPixel(VAbstractTool::CheckFormula(id, length, data)));

    return line.p2();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create help create tool.
 * @param initData init data.
 */
auto VToolEndLine::Create(VToolEndLineInitData &initData) -> VToolEndLine *
{
    const qreal calcAngle = CheckFormula(initData.id, initData.formulaAngle, initData.data);
    // An axis only fills the angle: the tool is placed as if the same number had been typed by hand.
    const qreal angle = initData.axis.IsValid() ? AxisAngle(initData.axis, initData.data) : calcAngle;

    QSharedPointer<VPointF> p = QSharedPointer<VPointF>::create(
        EndLinePoint(initData.data, initData.id, initData.basePointId, initData.formulaLength, angle), initData.name,
        initData.mx, initData.my);
    p->SetShowLabel(initData.showLabel);

    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->AddGObject(p);
    }
    else
    {
        initData.data->UpdateGObject(initData.id, p);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();
    initData.doc->FindFormulaDependencies(initData.formulaAngle, initData.id, varData);
    initData.doc->FindFormulaDependencies(initData.formulaLength, initData.id, varData);

    if (!initData.lineRoleName.isEmpty())
    {
        const QString basePointName = initData.data->GeometricObject<VPointF>(initData.basePointId)->name();
        const QString newPointName = initData.data->GeometricObject<VPointF>(initData.id)->name();
        initData.data->SetLineRoleName(basePointName + '_' + newPointName, initData.lineRoleName);
    }

    initData.data->AddLine(initData.basePointId, initData.id);

    patternGraph->AddEdge(initData.basePointId, initData.id);

    // The axis points are dependencies of the point, exactly as its base point is: declared here, on the path
    // both the reader and the interface take, so the point follows a point of another block and cannot outlive
    // one of its axis points.
    if (initData.axis.IsValid())
    {
        patternGraph->AddEdge(initData.axis.P1(), initData.id);
        patternGraph->AddEdge(initData.axis.P2(), initData.id);
    }

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::EndLine, initData.doc);
        auto *point = new VToolEndLine(initData);
        point->SetAxis(initData.axis);
        initData.scene->addItem(point);
        InitPointToolConnections(initData.scene, point);
        VAbstractPattern::AddTool(initData.id, point);
        return point;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SaveDialog save options into file after change in dialog.
 */
void VToolEndLine::SaveDialog(QDomElement &domElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogEndLine> dialogTool = qobject_cast<DialogEndLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    doc->SetAttribute(domElement, AttrName, dialogTool->GetPointName());
    doc->SetAttribute(domElement, AttrTypeLine, dialogTool->GetTypeLine());
    doc->SetAttribute(domElement, AttrLineColor, dialogTool->GetLineColor());
    doc->SetAttribute(domElement, AttrLength, dialogTool->GetFormula());
    doc->SetAttribute(domElement, AttrAngle, dialogTool->GetAngle());
    // The dialog only CARRIES the axis — it cannot set one (its window is modal). What it must not do is erase
    // it: an angle written by hand gives the axis up, entirely, and an untouched field keeps it (see
    // AxisAfterHandWrittenAngle).
    SaveAxis(doc,
             domElement,
             AxisAfterHandWrittenAngle(dialogTool->GetAxis(), EffectiveAngleFormula(), dialogTool->GetAngle()),
             AttrAxisP1,
             AttrAxisP2,
             AttrAxisSense);
    doc->SetAttribute(domElement, AttrBasePoint, QString().setNum(dialogTool->GetBasePointId()));
    doc->SetAttributeOrRemoveIf<QString>(domElement,
                                         AttrNotes,
                                         dialogTool->GetNotes(),
                                         [](const QString &notes) noexcept -> bool { return notes.isEmpty(); });

    // Persist the role name via the dedicated undo command (lineNames DOM section)
    SetLineRoleName(dialogTool->GetLineRoleName());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolEndLine::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolLinePoint::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrLength, formulaLength);
    doc->SetAttribute(tag, AttrAngle, EffectiveAngleFormula());
    doc->SetAttribute(tag, AttrBasePoint, basePointId);
    SaveAxisInto(tag);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolEndLine::ReadToolAttributes(const QDomElement &domElement)
{
    VToolLinePoint::ReadToolAttributes(domElement);

    m_lineType = VDomDocument::GetParametrString(domElement, AttrTypeLine, TypeLineLine);
    lineColor = VDomDocument::GetParametrString(domElement, AttrLineColor, ColorBlack);
    formulaLength = VDomDocument::GetParametrString(domElement, AttrLength, QString());
    basePointId = VAbstractPattern::GetParametrUInt(domElement, AttrBasePoint, NULL_ID_STR);
    formulaAngle = VDomDocument::GetParametrString(domElement, AttrAngle, QString());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolEndLine::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<VisToolEndLine *>(vis);
        SCASSERT(visual != nullptr)

        const bool osSeparator = VAbstractApplication::VApp()->Settings()->GetOsSeparator();
        const VTranslateVars *trVars = VAbstractApplication::VApp()->TrVars();

        visual->SetBasePointId(basePointId);
        visual->SetLength(trVars->FormulaToUser(formulaLength, osSeparator));
        visual->SetAngle(trVars->FormulaToUser(formulaAngle, osSeparator));
        visual->SetLineStyle(LineStyleToPenStyle(m_lineType));
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolEndLine::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogEndLine> dialogTool = qobject_cast<DialogEndLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    ProcessLinePointToolOptions(oldDomElement,
                                newDomElement,
                                dialogTool->GetPointName(),
                                VAbstractTool::data.GetGObject(dialogTool->GetBasePointId())->name());
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolEndLine::GetFormulaAngle() const -> VFormula
{
    VFormula fAngle(EffectiveAngleFormula(), getData());
    fAngle.setToolId(m_id);
    fAngle.setPostfix(degreeSymbol);
    fAngle.Eval();
    return fAngle;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolEndLine::SetFormulaAngle(const VFormula &value)
{
    if (value.error() == false)
    {
        formulaAngle = value.GetFormula(FormulaType::FromUser);
        // A formula written by hand gives the axis up, entirely: the angle is either taken from the axis or
        // written here, never composed of the two.
        m_axis.Clear();

        QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
        SaveOption(obj);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolEndLine::ShowVisualization(bool show)
{
    ShowToolVisualization<VisToolEndLine>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolEndLine::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    try
    {
        ContextMenu<DialogEndLine>(event, id);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolEndLine::GetLineRoleName() const -> QString
{
    return VAbstractTool::data.LineRoleName(BasePointName() + '_' + VAbstractTool::data.GetGObject(m_id)->name());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolEndLine::SetLineRoleName(const QString &suffix)
{
    TrySetLineRoleName(BasePointName() + '_' + VAbstractTool::data.GetGObject(m_id)->name(), suffix);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolEndLine::AngleFormulaString() const -> QString
{
    return formulaAngle;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolEndLine::SetAngleFormulaString(const QString &formula)
{
    formulaAngle = formula;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolEndLine::PointForAxisAngle(qreal angle) const -> std::optional<QPointF>
{
    return EndLinePoint(getData(), m_id, basePointId, formulaLength, angle);
}
