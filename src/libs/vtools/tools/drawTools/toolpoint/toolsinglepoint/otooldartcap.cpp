/************************************************************************
 **
 **  @file   otooldartcap.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  the cap of a dart: the point that closes it on the other side
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

#include "otooldartcap.h"

#include <QLineF>
#include <QPointF>
#include <QSharedPointer>
#include <QUndoStack>

#include "../../../../dialogs/tools/dialogtool.h"
#include "../../../../dialogs/tools/odialogdartcap.h"
#include "../../../../undocommands/vchangelinerolename.h"
#include "../../../../visualization/line/ovistooldartcap.h"
#include "../../../../visualization/visualization.h"
#include "../../../vabstracttool.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vdomdocument.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/compatibility.h"
#include "../vmisc/literals.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "odartcaprule.h"
#include "vexception.h"

using namespace Qt::Literals::StringLiterals;

const QString OToolDartCap::ToolType = QStringLiteral("dartCap");

//---------------------------------------------------------------------------------------------------------------------
OToolDartCap::OToolDartCap(const OToolDartCapInitData &initData, QGraphicsItem *parent)
  : VToolLinePoint(initData.doc, initData.data, initData.id, initData.typeLine, initData.lineColor, QString(),
                   initData.firstPointId, 0., initData.notes, parent),
    m_apexId(initData.apexId),
    m_firstPointId(initData.firstPointId),
    m_secondPointId(initData.secondPointId),
    m_extendPointId(initData.extendPointId),
    m_drawCap(initData.drawCap)
{
    // The second tracé of the cap, C→P2. The first one, C→P1, is the mainLine every line point already has,
    // built on the base point (P1) by VToolLinePoint. One pen for both: RefreshCapGeometry copies the pen of
    // the main line onto this one, so the two can never drift apart.
    m_secondLine = new VScaledLine(QLineF(), VColorRole::CustomColor, this);
    m_secondLine->SetBoldLine(false);
    m_secondLine->SetShowArrows(true);
    m_secondLine->setAcceptHoverEvents(true);
    m_secondLine->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    m_secondLine->setZValue(-0.5);

    ToolCreation(initData.typeCreation);

    // The two tracés are hidden right away when the cap is not drawn: nothing waits for the first refresh.
    RefreshCapGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogDartCap> dialogTool = qobject_cast<ODialogDartCap *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    const QSharedPointer<VPointF> p = VAbstractTool::data.GeometricObject<VPointF>(m_id);
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetApexId(m_apexId);
    dialogTool->SetFirstPointId(m_firstPointId);
    dialogTool->SetSecondPointId(m_secondPointId);
    dialogTool->SetExtendPointId(m_extendPointId);
    dialogTool->SetPointName(p->name());
    dialogTool->SetNotes(m_notes);
    dialogTool->SetLine1RoleName(GetLine1RoleName());
    dialogTool->SetLine2RoleName(GetLine2RoleName());
    dialogTool->SetTypeLine(getLineType());
    dialogTool->SetLineColor(GetLineColor());
    dialogTool->SetDrawCap(m_drawCap);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                          VContainer *data) -> OToolDartCap *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogDartCap> dialogTool = qobject_cast<ODialogDartCap *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolDartCapInitData initData;
    initData.apexId = dialogTool->GetApexId();
    initData.firstPointId = dialogTool->GetFirstPointId();
    initData.secondPointId = dialogTool->GetSecondPointId();
    initData.extendPointId = dialogTool->GetExtendPointId();
    initData.lineRoleName = dialogTool->GetLine1RoleName();
    initData.secondLineRoleName = dialogTool->GetLine2RoleName();
    initData.drawCap = dialogTool->GetDrawCap();
    initData.typeLine = dialogTool->GetTypeLine();
    initData.lineColor = dialogTool->GetLineColor();
    initData.name = dialogTool->GetPointName();
    initData.showLabel = dialogTool->GetShowLabel();
    initData.notes = dialogTool->GetNotes();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;

    OToolDartCap *point = Create(initData);
    if (point != nullptr)
    {
        point->m_dialog = dialog;
    }
    return point;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::Create(OToolDartCapInitData initData) -> OToolDartCap *
{
    const QSharedPointer<VPointF> apex = initData.data->GeometricObject<VPointF>(initData.apexId);
    const QSharedPointer<VPointF> firstPoint = initData.data->GeometricObject<VPointF>(initData.firstPointId);
    const QSharedPointer<VPointF> secondPoint = initData.data->GeometricObject<VPointF>(initData.secondPointId);
    const QSharedPointer<VPointF> extendPoint = initData.data->GeometricObject<VPointF>(initData.extendPointId);

    QPointF capPoint;
    const ODartCapRule::Failure failure =
        ODartCapRule::CapPoint(static_cast<QPointF>(*apex), static_cast<QPointF>(*firstPoint),
                               static_cast<QPointF>(*secondPoint), static_cast<QPointF>(*extendPoint), &capPoint);
    if (ODartCapRule::IsFailure(failure))
    {
        // Same behaviour as the tool this one is built on: the pattern still opens, the user is told. The
        // dialog refuses this very construction before anything is created, so this is only reached when a
        // later move of a parent point made the two lines parallel.
        const QString errorMsg = tr("Error calculating point '%1'. The line of the pattern through %2 does not "
                                    "meet the axis of the dart %3-%4-%5")
                                     .arg(initData.name, extendPoint->name(), apex->name(), firstPoint->name(),
                                          secondPoint->name());
        VAbstractApplication::VApp()->IsPedantic()
            ? throw VExceptionObjectError(errorMsg)
            : qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
        capPoint = QPointF();
    }

    auto *p = new VPointF(capPoint, initData.name, initData.mx, initData.my);
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

    // The two tracés are named in the direction they are read, cap point first: Line_C_P1 and Line_C_P2. The
    // names live in <lineNames>, keyed by the same pair of point names AddLine builds below.
    const QString capPointName = initData.data->GeometricObject<VPointF>(initData.id)->name();
    if (!initData.lineRoleName.isEmpty())
    {
        initData.data->SetLineRoleName(capPointName + '_' + firstPoint->name(), initData.lineRoleName);
    }
    if (!initData.secondLineRoleName.isEmpty())
    {
        initData.data->SetLineRoleName(capPointName + '_' + secondPoint->name(), initData.secondLineRoleName);
    }

    initData.data->AddLine(initData.id, initData.firstPointId);
    initData.data->AddLine(initData.id, initData.secondPointId);

    // The cap depends on the two points of the opening: the edge goes from the point used to the tool
    // that uses it, as everywhere else. Reversed, the cap looks like something others depend on and can
    // never be deleted (VInteractiveTool::IsRemovable counts the dependent nodes).
    patternGraph->AddEdge(initData.firstPointId, initData.id);
    patternGraph->AddEdge(initData.secondPointId, initData.id);

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::DartCap, initData.doc);
        auto *point = new OToolDartCap(initData);
        initData.scene->addItem(point);
        InitPointToolConnections(initData.scene, point);
        VAbstractPattern::AddTool(initData.id, point);
        return point;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::ApexName() const -> QString
{
    return VAbstractTool::data.GetGObject(m_apexId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::FirstPointName() const -> QString
{
    return VAbstractTool::data.GetGObject(m_firstPointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::SecondPointName() const -> QString
{
    return VAbstractTool::data.GetGObject(m_secondPointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::ExtendPointName() const -> QString
{
    return VAbstractTool::data.GetGObject(m_extendPointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::Line1PairKey() const -> QString
{
    return VAbstractTool::data.GetGObject(m_id)->name() + '_' + FirstPointName();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::Line2PairKey() const -> QString
{
    return VAbstractTool::data.GetGObject(m_id)->name() + '_' + SecondPointName();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::GetLine1RoleName() const -> QString
{
    return VAbstractTool::data.LineRoleName(Line1PairKey());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetLine1RoleName(const QString &suffix)
{
    SetLineRoleNameForPair(Line1PairKey(), suffix);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::GetLine2RoleName() const -> QString
{
    return VAbstractTool::data.LineRoleName(Line2PairKey());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetLine2RoleName(const QString &suffix)
{
    SetLineRoleNameForPair(Line2PairKey(), suffix);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetLineRoleNameForPair(const QString &pairKey, const QString &suffix)
{
    const QString oldSuffix = VAbstractTool::data.LineRoleName(pairKey);
    if (!suffix.isEmpty() && !VAbstractTool::data.IsUnique(line_ + suffix))
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
auto OToolDartCap::IsCapDrawn() const -> bool
{
    return m_drawCap;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetCapDrawn(bool drawn)
{
    if (m_drawCap == drawn)
    {
        return;
    }

    m_drawCap = drawn;

    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOption(obj);
    RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolDartCap>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SaveDialog(QDomElement &domElement)
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogDartCap> dialogTool = qobject_cast<ODialogDartCap *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    doc->SetAttribute(domElement, AttrName, dialogTool->GetPointName());
    doc->SetAttribute(domElement, AttrApex, QString().setNum(dialogTool->GetApexId()));
    doc->SetAttribute(domElement, AttrFirstPoint, QString().setNum(dialogTool->GetFirstPointId()));
    doc->SetAttribute(domElement, AttrSecondPoint, QString().setNum(dialogTool->GetSecondPointId()));
    doc->SetAttribute(domElement, AttrExtendPoint, QString().setNum(dialogTool->GetExtendPointId()));
    doc->SetAttribute(domElement, AttrTypeLine, dialogTool->GetTypeLine());
    doc->SetAttribute(domElement, AttrLineColor, dialogTool->GetLineColor());
    doc->SetAttribute(domElement, AttrDrawCap, dialogTool->GetDrawCap() ? trueStr : falseStr);

    doc->SetAttributeOrRemoveIf<QString>(domElement, AttrNotes, dialogTool->GetNotes(),
                                         [](const QString &notes) { return notes.isEmpty(); });

    SetLine1RoleName(dialogTool->GetLine1RoleName());
    SetLine2RoleName(dialogTool->GetLine2RoleName());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    try
    {
        ContextMenu<ODialogDartCap>(event, id);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolLinePoint::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrApex, m_apexId);
    doc->SetAttribute(tag, AttrFirstPoint, m_firstPointId);
    doc->SetAttribute(tag, AttrSecondPoint, m_secondPointId);
    doc->SetAttribute(tag, AttrExtendPoint, m_extendPointId);
    doc->SetAttribute(tag, AttrDrawCap, m_drawCap ? trueStr : falseStr);

    // The two names of the cap live in <lineNames>, keyed by the pair of points — and the key the base class
    // syncs on its own is the pair (P1, C), which is never the one used here.
    SyncLineRoleNameToDom(Line1PairKey());
    SyncLineRoleNameToDom(Line2PairKey());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ReadToolAttributes(const QDomElement &domElement)
{
    VToolLinePoint::ReadToolAttributes(domElement);

    m_apexId = VAbstractPattern::GetParametrUInt(domElement, AttrApex, NULL_ID_STR);
    m_firstPointId = VAbstractPattern::GetParametrUInt(domElement, AttrFirstPoint, NULL_ID_STR);
    m_secondPointId = VAbstractPattern::GetParametrUInt(domElement, AttrSecondPoint, NULL_ID_STR);
    m_extendPointId = VAbstractPattern::GetParametrUInt(domElement, AttrExtendPoint, NULL_ID_STR);
    m_drawCap = VDomDocument::GetParametrBool(domElement, AttrDrawCap, trueStr);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<OVisToolDartCap *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetApexId(m_apexId);
        visual->SetFirstPointId(m_firstPointId);
        visual->SetSecondPointId(m_secondPointId);
        visual->SetExtendPointId(m_extendPointId);
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::RefreshGeometry()
{
    VToolLinePoint::RefreshGeometry(); // the mainLine, C→P1, with the pen of the tool

    RefreshCapGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::RefreshCapGeometry()
{
    if (m_secondLine == nullptr)
    {
        return;
    }

    const QPointF capPoint = static_cast<QPointF>(*VDrawTool::data.GeometricObject<VPointF>(m_id));
    const QPointF secondPoint = static_cast<QPointF>(*VDrawTool::data.GeometricObject<VPointF>(m_secondPointId));

    // The tool stands on the cap point, so a line of its own is written the way the inherited main line is:
    // relative to that point, not in scene coordinates — otherwise the segment is drawn shifted by the whole
    // position of the cap, somewhere else on the canvas.
    const QLineF line(secondPoint - capPoint, QPointF());

    m_secondLine->setLine(line);
    m_secondLine->setPen(mainLine->pen()); // one style, one colour: decided once, on the main line

    mainLine->setVisible(m_drawCap && not mainLine->line().isNull());
    m_secondLine->setVisible(m_drawCap && not line.isNull());
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolDartCap::MakeToolTip() const -> QString
{
    return tr("Cap of the dart %1-%2-%3").arg(ApexName(), FirstPointName(), SecondPointName());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolDartCap::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    VToolLinePoint::ApplyToolOptions(oldDomElement, newDomElement);

    // The cap can be drawn or not: that is the only option the dock and the dialog can change once the tool
    // exists, and the undo stack re-applies it through here.
    m_drawCap = VDomDocument::GetParametrBool(newDomElement, AttrDrawCap, trueStr);
    RefreshGeometry();
}
