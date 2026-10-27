/************************************************************************
 **
 **  @file   vtoolalongline.cpp
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
 **  Modified for Ostara (fork of Valentina) on 27/10/2026.
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

#include "vtoolalongline.h"

#include <QLineF>
#include <QPointF>
#include <QSharedPointer>
#include <QUndoStack>

#include "../../../../../dialogs/tools/dialogalongline.h"
#include "../../../../../dialogs/tools/dialogtool.h"
#include "../../../../../undocommands/renameobject.h"
#include "../../../../../undocommands/savetooloptions.h"
#include "../../../../../undocommands/vchangelinerolename.h"
#include "../../../../../visualization/line/vistoolalongline.h"
#include "../../../../../visualization/visualization.h"
#include "../../../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "vexception.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/scalesceneitems.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "tools/otooltip.h"
#include "vtoollinepoint.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

template <class T> class QSharedPointer;

const QString VToolAlongLine::ToolType = QStringLiteral("alongLine"); // NOLINT(cert-err58-cpp)

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VToolAlongLine constuctor.
 * @param initData init data.
 * @param parent parent object.
 */
VToolAlongLine::VToolAlongLine(const VToolAlongLineInitData &initData, QGraphicsItem *parent)
  : VToolLinePoint(initData.doc, initData.data, initData.id, initData.typeLine, initData.lineColor, initData.formula,
                   initData.firstPointId, 0, initData.notes, parent),
    m_secondPointId(initData.secondPointId)
{
    // A cut line is two segments, and this tool draws both: first point → point is the inherited main line,
    // point → second point is this one. Until now the second segment existed only as the Line_ and AngleLine_
    // variables VToolAlongLine::Create registers — the pen chosen in the dialog never reached it, and no
    // operation could take it. Built here, like the main line, so the scene item exists as soon as the tool does.
    auto const point = static_cast<QPointF>(*VAbstractTool::data.GeometricObject<VPointF>(m_id));
    auto const secondPoint = static_cast<QPointF>(*VAbstractTool::data.GeometricObject<VPointF>(m_secondPointId));
    QLineF const line(secondPoint - point, QPointF());
    m_secondLine = new VScaledLine(line, VColorRole::CustomColor, this);
    m_secondLine->SetBoldLine(false);
    m_secondLine->SetShowArrows(true);
    m_secondLine->setAcceptHoverEvents(true);
    m_secondLine->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    m_secondLine->setZValue(-0.5); // Child line below parent point (Z=0.5 effective vs Z=1)
    m_secondLine->setVisible(not line.isNull());

    AttachAuthoredColor(m_secondLine);
    RefreshAuthoredPen(m_secondLine);

    // Both items exist now: one handler per declared segment, in the order DrawnSegments() names them. Called
    // again on purpose — the inherited constructor wired the one element it knew, and this tool's list wins.
    InitDrawnSegmentConnections();

    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SaveDialog save options into file after change in dialog.
 */
void VToolAlongLine::SaveDialog(QDomElement &domElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogAlongLine> dialogTool = qobject_cast<DialogAlongLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    doc->SetAttribute(domElement, AttrName, dialogTool->GetPointName());
    doc->SetAttribute(domElement, AttrTypeLine, dialogTool->GetTypeLine());
    doc->SetAttribute(domElement, AttrLineColor, dialogTool->GetLineColor());
    doc->SetAttribute(domElement, AttrLength, dialogTool->GetFormula());
    doc->SetAttribute(domElement, AttrFirstPoint, dialogTool->GetFirstPointId());
    doc->SetAttribute(domElement, AttrSecondPoint, dialogTool->GetSecondPointId());

    const QString notes = dialogTool->GetNotes();
    doc->SetAttributeOrRemoveIf<QString>(domElement,
                                         AttrNotes,
                                         notes,
                                         [](const QString &notes) noexcept -> bool { return notes.isEmpty(); });

    // Persist the role name via the dedicated undo command (lineNames DOM section)
    SetLineRoleName(dialogTool->GetLineRoleName());
    SetSecondLineRoleName(dialogTool->GetSecondLineRoleName());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolLinePoint::SaveOptions(tag, obj);

    // Also persist the C→B role name (parent only syncs A→C)
    SyncLineRoleNameToDom(VAbstractTool::data.GetGObject(m_id)->name() + '_' + SecondPointName());

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrLength, formulaLength);
    doc->SetAttribute(tag, AttrFirstPoint, basePointId);
    doc->SetAttribute(tag, AttrSecondPoint, m_secondPointId);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::ReadToolAttributes(const QDomElement &domElement)
{
    VToolLinePoint::ReadToolAttributes(domElement);

    m_lineType = VDomDocument::GetParametrString(domElement, AttrTypeLine, TypeLineLine);
    lineColor = VDomDocument::GetParametrString(domElement, AttrLineColor, ColorBlack);
    formulaLength = VDomDocument::GetParametrString(domElement, AttrLength, QString());
    basePointId = VDomDocument::GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
    m_secondPointId = VDomDocument::GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<VisToolAlongLine *>(vis);
        SCASSERT(visual != nullptr)
        visual->SetPoint1Id(basePointId);
        visual->SetPoint2Id(m_secondPointId);
        visual->SetLength(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            formulaLength, VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
        visual->SetLineStyle(LineStyleToPenStyle(m_lineType));
        visual->SetMode(Mode::Show);
        visual->SetToolPointId(m_id);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::RefreshGeometry()
{
    // The inherited main line is the first segment, first point → point; the second one is this tool's own.
    VToolLinePoint::RefreshGeometry();

    if (m_secondLine == nullptr)
    {
        return;
    }

    // Same convention as the main line: the item is carried by the point, so the point is its origin and the
    // other end is written relative to it.
    auto const point = static_cast<QPointF>(*VAbstractTool::data.GeometricObject<VPointF>(m_id));
    auto const secondPoint = static_cast<QPointF>(*VAbstractTool::data.GeometricObject<VPointF>(m_secondPointId));
    QLineF const line(secondPoint - point, QPointF());
    m_secondLine->setLine(line);
    RefreshAuthoredPen(m_secondLine);

    // Called a second time, and on purpose: the base ran the rule on the geometry this line still had from the
    // refresh before. It knows its own length now — an empty one is not drawn, and neither is a half whose other
    // end the layers hide.
    ApplySegmentsVisibility();
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolAlongLine::MakeToolTip() const -> QString
{
    const QSharedPointer<VPointF> basePoint = VAbstractTool::data.GeometricObject<VPointF>(basePointId);
    const QSharedPointer<VPointF> secondPoint = VAbstractTool::data.GeometricObject<VPointF>(m_secondPointId);
    const QSharedPointer<VPointF> current = VAbstractTool::data.GeometricObject<VPointF>(m_id);

    const QLineF curLine(static_cast<QPointF>(*basePoint), static_cast<QPointF>(*current));
    const QLineF curToSecond(static_cast<QPointF>(*current), static_cast<QPointF>(*secondPoint));

    // The point cuts the line in two, and the preview paints the halves green (A→C, the length asked for)
    // and red (C→B, what remains). Each half is a real line with its own name: when the user has named one,
    // the row carries that name, otherwise it says what it is. The two rows keep the preview's colours, so
    // each number meets the line it measures; the point's own name is not written (see OToolTip).
    const QString firstLine = OToolTip::LineName(GetLineRoleName());
    const QString secondLine = OToolTip::LineName(GetSecondLineRoleName());

    return OToolTip::Table({
        OToolTip::StrokeRow(firstLine.isEmpty() ? tr("Length") : firstLine,
                            OToolTip::Measure(curLine.length()),
                            VColorRole::VisSupportColor2),
        OToolTip::StrokeRow(secondLine.isEmpty() ? tr("Length") : secondLine,
                            OToolTip::Measure(curToSecond.length()),
                            VColorRole::VisSupportColor4),
        OToolTip::Row(tr("Angle"), OToolTip::Degrees(curLine.angle())),
    });
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogAlongLine> dialogTool = qobject_cast<DialogAlongLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    const QString oldLabel = VAbstractTool::data.GetGObject(m_id)->name();
    const QString newLabel = dialogTool->GetPointName();

    const QString newBasePointLabel = VAbstractTool::data.GetGObject(dialogTool->GetFirstPointId())->name();
    const QString oldBasePointLabel = BasePointName();

    const QString newSecondPointLabel = VAbstractTool::data.GetGObject(dialogTool->GetSecondPointId())->name();
    const QString oldSecondPointLabel = SecondPointName();

    if (oldBasePointLabel == newBasePointLabel && oldSecondPointLabel == newSecondPointLabel && oldLabel == newLabel)
    {
        VToolLinePoint::ApplyToolOptions(oldDomElement, newDomElement);
        return;
    }

    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    auto *newGroup = new QUndoCommand(); // an empty command
    newGroup->setText(tr("save tool options"));

    auto *saveOptions = new SaveToolOptions(oldDomElement, newDomElement, doc, m_id, newGroup);
    saveOptions->SetInGroup(true);
    connect(saveOptions, &SaveToolOptions::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);

    if (oldBasePointLabel != newBasePointLabel)
    {
        auto *renamePair = RenamePair::CreateForLine(std::make_pair(oldBasePointLabel, oldLabel),
                                                     std::make_pair(newBasePointLabel, oldLabel),
                                                     doc,
                                                     m_id,
                                                     newGroup);
        if (oldLabel == newLabel && oldSecondPointLabel == newSecondPointLabel)
        {
            connect(renamePair, &RenamePair::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
        }
    }

    if (oldSecondPointLabel != newSecondPointLabel)
    {
        auto *renamePair = RenamePair::CreateForLine(std::make_pair(oldLabel, oldSecondPointLabel),
                                                     std::make_pair(oldLabel, newSecondPointLabel),
                                                     doc,
                                                     m_id,
                                                     newGroup);
        if (oldLabel == newLabel)
        {
            connect(renamePair, &RenamePair::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
        }
    }

    if (oldLabel != newLabel)
    {
        const auto *renameLabel = new RenameLabel(oldLabel, newLabel, doc, m_id, newGroup);
        connect(renameLabel, &RenameLabel::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);

        // Also re-key the C→B role name pair when point C is renamed
        if (oldSecondPointLabel == newSecondPointLabel)
        {
            auto *renamePairCB = RenamePair::CreateForLine(std::make_pair(oldLabel, oldSecondPointLabel),
                                                            std::make_pair(newLabel, oldSecondPointLabel),
                                                            doc, m_id, newGroup);
            connect(renamePairCB, &RenamePair::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
        }
    }

    undoStack->push(newGroup);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::ShowVisualization(bool show)
{
    if (show)
    {
        // A persistent show (click, options panel) takes over from a transient hover show.
        m_hoverOwnsVisualization = false;
    }
    ShowToolVisualization<VisToolAlongLine>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    // Construction visualization on hover is a Select-mode behavior only.
    if (not m_suppressContextMenu)
    {
        const bool wasShown = not vis.isNull();
        ShowVisualization(true);
        m_hoverOwnsVisualization = not wasShown; // We own the visualization only if the hover created it.
    }
    VToolLinePoint::hoverEnterEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    if (m_hoverOwnsVisualization)
    {
        ShowVisualization(false);
        m_hoverOwnsVisualization = false;
    }
    VToolLinePoint::hoverLeaveEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    try
    {
        ContextMenu<DialogAlongLine>(event, id);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setDialog set dialog when user want change tool option.
 */
void VToolAlongLine::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogAlongLine> dialogTool = qobject_cast<DialogAlongLine *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    const QSharedPointer<VPointF> p = VAbstractTool::data.GeometricObject<VPointF>(m_id);
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetTypeLine(m_lineType);
    dialogTool->SetLineColor(lineColor);
    dialogTool->SetFormula(formulaLength);
    dialogTool->SetFirstPointId(basePointId);
    dialogTool->SetSecondPointId(m_secondPointId);
    dialogTool->SetPointName(p->name());
    dialogTool->SetNotes(m_notes);
    dialogTool->SetLineRoleName(GetLineRoleName());
    dialogTool->SetSecondLineRoleName(GetSecondLineRoleName());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create help create tool form GUI.
 * @param dialog dialog options.
 * @param scene pointer to scene.
 * @param doc dom document container.
 * @param data container with variables.
 */
auto VToolAlongLine::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                            VContainer *data) -> VToolAlongLine *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogAlongLine> dialogTool = qobject_cast<DialogAlongLine *>(dialog);
    SCASSERT(not dialogTool.isNull())

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wnoexcept")

    VToolAlongLineInitData initData;

    QT_WARNING_POP

    initData.formula = dialogTool->GetFormula();
    initData.firstPointId = dialogTool->GetFirstPointId();
    initData.secondPointId = dialogTool->GetSecondPointId();
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
    initData.secondLineRoleName = dialogTool->GetSecondLineRoleName();

    VToolAlongLine *point = Create(initData);
    if (point != nullptr)
    {
        point->m_dialog = dialog;
    }
    return point;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create help create tool.
 * @param initData init data.
 */
auto VToolAlongLine::Create(VToolAlongLineInitData &initData) -> VToolAlongLine *
{
    const QSharedPointer<VPointF> firstPoint = initData.data->GeometricObject<VPointF>(initData.firstPointId);
    const QSharedPointer<VPointF> secondPoint = initData.data->GeometricObject<VPointF>(initData.secondPointId);
    auto line = QLineF(static_cast<QPointF>(*firstPoint), static_cast<QPointF>(*secondPoint));

    // Declare special variable "CurrentLength"
    auto *length = new VLengthLine(firstPoint.data(), initData.firstPointId, secondPoint.data(), initData.secondPointId,
                                   *initData.data->GetPatternUnit());
    length->SetName(currentLength);
    initData.data->SetTransientVariable(length);

    line.setLength(
        VAbstractValApplication::VApp()->toPixel(CheckFormula(initData.id, initData.formula, initData.data)));

    QSharedPointer<VPointF> p = QSharedPointer<VPointF>::create(line.p2(), initData.name, initData.mx, initData.my);
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
    initData.doc->FindFormulaDependencies(initData.formula, initData.id, varData);

    if (!initData.lineRoleName.isEmpty())
    {
        const QString basePointNameForRole = initData.data->GeometricObject<VPointF>(initData.firstPointId)->name();
        const QString newPointNameForRole = initData.data->GeometricObject<VPointF>(initData.id)->name();
        initData.data->SetLineRoleName(basePointNameForRole + '_' + newPointNameForRole, initData.lineRoleName);
    }

    if (!initData.secondLineRoleName.isEmpty())
    {
        const QString newPointNameForRole = initData.data->GeometricObject<VPointF>(initData.id)->name();
        const QString secondPointNameForRole = initData.data->GeometricObject<VPointF>(initData.secondPointId)->name();
        initData.data->SetLineRoleName(newPointNameForRole + '_' + secondPointNameForRole, initData.secondLineRoleName);
    }

    initData.data->AddLine(initData.firstPointId, initData.id);
    initData.data->AddLine(initData.id, initData.secondPointId);

    patternGraph->AddEdge(initData.firstPointId, initData.id);
    patternGraph->AddEdge(initData.secondPointId, initData.id);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    VToolAlongLine *point = nullptr;
    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::AlongLine, initData.doc);
        point = new VToolAlongLine(initData);
        initData.scene->addItem(point);
        InitPointToolConnections(initData.scene, point);
        VAbstractPattern::AddTool(initData.id, point);
    }
    // Very important to delete it. Only this tool need this special variable.
    initData.data->RemoveVariable(currentLength);
    return point;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolAlongLine::SecondPointName() const -> QString
{
    return VAbstractTool::data.GetGObject(m_secondPointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolAlongLine::GetLineRoleName() const -> QString
{
    return VAbstractTool::data.LineRoleName(BasePointName() + '_' + VAbstractTool::data.GetGObject(m_id)->name());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::SetLineRoleName(const QString &suffix)
{
    TrySetLineRoleName(BasePointName() + '_' + VAbstractTool::data.GetGObject(m_id)->name(), suffix);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolAlongLine::GetSecondLineRoleName() const -> QString
{
    return VAbstractTool::data.LineRoleName(VAbstractTool::data.GetGObject(m_id)->name() + '_' + SecondPointName());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolAlongLine::SetSecondLineRoleName(const QString &suffix)
{
    TrySetLineRoleName(VAbstractTool::data.GetGObject(m_id)->name() + '_' + SecondPointName(), suffix);
}

//---------------------------------------------------------------------------------------------------------------------
// Upstream 98ad5f895: evaluate the transient 'current length' variable through the persistent container so the
// property browser (fx) reading VFormula's pointer after the call does not hit a dangling/stale value.
auto VToolAlongLine::GetFormulaLength() const -> VFormula
{
    auto *container = const_cast<VContainer *>(getData());
    const QSharedPointer<VPointF> firstPoint = container->GeometricObject<VPointF>(basePointId);
    const QSharedPointer<VPointF> secondPoint = container->GeometricObject<VPointF>(m_secondPointId);

    auto *length = new VLengthLine(firstPoint.data(), basePointId, secondPoint.data(), m_secondPointId,
                                   *container->GetPatternUnit());
    length->SetName(currentLength);
    container->SetTransientVariable(length);

    VFormula val(formulaLength, container);
    val.setToolId(m_id);
    val.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    val.Eval();
    return val;
}
