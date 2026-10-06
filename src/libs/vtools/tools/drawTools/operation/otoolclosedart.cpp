/************************************************************************
 **
 **  @file   otoolclosedart.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/09/2026
 **
 **  @brief  The tool that closes a dart, see otoolclosedart.h.
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

#include "otoolclosedart.h"

#include <QDomNode>
#include <QPoint>
#include <QSharedPointer>
#include <QUndoStack>
#include <QtNumeric>

#include "../../../dialogs/tools/dialogtool.h"
#include "../../../dialogs/tools/odialogclosedart.h"
#include "../../../visualization/line/operation/ovistoolclosedart.h"
#include "../../../visualization/visualization.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "oclosedartrule.h"
#include "ooperationdestination.h"
#include "orotationtransform.h"
#include "vexception.h"

class QDomElement;
class QGraphicsSceneContextMenuEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
template <class T> class QSharedPointer;

const QString OToolCloseDart::ToolType = QStringLiteral("closeDart");

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/** @brief The name of one of the three points of the dart, or what to call it when it is no longer there.
 *
 * The name is read by the recipe of a pattern, which can be asked about a tool whose points are gone: letting
 * the exception out would end the application. Same guard as VToolRotation::OriginPointName. */
auto PointName(const VContainer *data, quint32 id, const QString &fallback) -> QString
{
    try
    {
        return data->GetGObject(id)->name();
    }
    catch (const VExceptionBadId &)
    {
        return fallback;
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The angle that closes the dart, NaN as soon as it does not exist: a missing point, a pivot that is
 *  one of the legs, or a divisor that cannot divide. Read from the pattern, this can happen on a file edited
 *  outside the application, and nothing must be built out of it. */
auto ResolveClosingAngle(const VContainer *data, quint32 pivotId, quint32 movingId, quint32 targetId, qreal divisor)
    -> qreal
{
    try
    {
        const QPointF pivot = static_cast<QPointF>(*data->GeometricObject<VPointF>(pivotId));
        const QPointF moving = static_cast<QPointF>(*data->GeometricObject<VPointF>(movingId));
        const QPointF target = static_cast<QPointF>(*data->GeometricObject<VPointF>(targetId));

        return OCloseDartRule::ClosingAngle(pivot, moving, target, divisor);
    }
    catch (const VException &e)
    {
        qCDebug(vTool, "Skipping the dart: %s", e.ErrorMessage().toUtf8().constData());
        return qQNaN();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MakeDestination(quint32 originId, qreal angle, VContainer *data) -> OOperationDestination<ORotationTransform>
{
    const auto originPoint = *data->GeometricObject<VPointF>(originId);
    return OOperationDestination<ORotationTransform>(ORotationTransform(static_cast<QPointF>(originPoint), angle));
}

//---------------------------------------------------------------------------------------------------------------------
void CreateDestination(OToolCloseDartInitData &initData, qreal angle)
{
    const OOperationDestination<ORotationTransform> destination = MakeDestination(initData.origin, angle,
                                                                                  initData.data);

    if (initData.typeCreation == Source::FromGui)
    {
        destination.Create(initData.id, initData.source, initData.data, initData.destination);
    }
    else
    {
        destination.Update(initData.id, initData.source, initData.data, initData.destination);
    }
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OToolCloseDart::OToolCloseDart(const OToolCloseDartInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_origPointId(initData.origin),
    m_movingPointId(initData.moving),
    m_targetPointId(initData.target),
    m_formulaDivisor(initData.divisor)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCloseDart::SyncDestination(const QVector<SourceItem> &source, QString &formulaDivisor)
    -> QVector<DestinationItem>
{
    bool unchanged = false;
    const QMap<QUuid, DestinationItem> existingByRecord = SyncDestinationPreamble(source, unchanged);
    if (unchanged)
    {
        return m_destination;
    }

    const qreal divisor = CheckFormula(m_id, formulaDivisor, getData());
    const qreal angle = ResolveClosingAngle(getData(), m_origPointId, m_movingPointId, m_targetPointId, divisor);
    if (qIsNaN(angle))
    {
        // The dart cannot close: the sources keep the destinations they had rather than gain copies turned by
        // an angle that does not exist. The dialog refuses that case before getting here.
        return m_destination;
    }

    const OOperationDestination<ORotationTransform> destination = MakeDestination(m_origPointId, angle, getData());

    QVector<DestinationItem> newDestination;
    newDestination.reserve(source.size());

    for (const auto &srcItem : std::as_const(source))
    {
        if (existingByRecord.contains(srcItem.recordId))
        {
            // Preserve existing destination item
            newDestination.append(existingByRecord.value(srcItem.recordId));
        }
        else
        {
            // Create destination item for newly added source
            newDestination.append(destination.CreateObject(m_id, srcItem, getData()));
        }
    }

    return newDestination;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCloseDart::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<ODialogCloseDart> dialogTool = qobject_cast<ODialogCloseDart *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetOrigPointId(m_origPointId);
    dialogTool->SetMovingPointId(m_movingPointId);
    dialogTool->SetTargetPointId(m_targetPointId);
    dialogTool->SetDivisor(m_formulaDivisor);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);

    // Every line the operation carries comes back into the dialog: each one was picked by the user, one click
    // per line, and there is no longer a line that follows its points on its own.
    dialogTool->SetLineSources(Lines());

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCloseDart::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                            VContainer *data) -> OToolCloseDart *
{
    SCASSERT(not dialog.isNull())
    const QPointer<ODialogCloseDart> dialogTool = qobject_cast<ODialogCloseDart *>(dialog);
    SCASSERT(not dialogTool.isNull())

    OToolCloseDartInitData initData;
    initData.origin = dialogTool->GetOrigPointId();
    initData.moving = dialogTool->GetMovingPointId();
    initData.target = dialogTool->GetTargetPointId();
    initData.divisor = dialogTool->GetDivisor();
    initData.source = dialogTool->GetSourceObjects();
    initData.lines = OOperationLines::Collect(dialogTool->GetLineSources());
    initData.hasLinkedVisibilityGroup = dialogTool->HasLinkedVisibilityGroup();
    initData.visibilityGroupName = dialogTool->GetVisibilityGroupName();
    initData.visibilityGroupTags = dialogTool->GetVisibilityGroupTags();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();

    OToolCloseDart *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCloseDart::Create(OToolCloseDartInitData &initData) -> OToolCloseDart *
{
    if (initData.typeCreation == Source::FromGui)
    {
        initData.destination.clear(); // Try to avoid mistake, value must be empty

        initData.id = initData.data->getNextId(); // Just reserve id for tool

        // The transformed lines are persisted with the operation, so their ids are reserved now, before the
        // operation element is written.
        OOperationLines::AllocateResultIds(initData.data, initData.lines);
    }
    else
    {
        PrepareNames(initData);
    }

    // The angle is read from the pattern and the divisor comes from a formula: either of them can fail on a
    // file the user edited outside the application, and a copy turned by an angle that does not exist would
    // corrupt the pattern. Nothing is written then — the operation simply is not there.
    const qreal divisor = CheckFormula(initData.id, initData.divisor, initData.data);
    const qreal angle = ResolveClosingAngle(initData.data, initData.origin, initData.moving, initData.target,
                                            divisor);
    if (qIsNaN(angle))
    {
        return nullptr;
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();
    initData.doc->FindFormulaDependencies(initData.divisor, initData.id, varData);

    CreateDestination(initData, angle);

    // The pivot of the dart is a fixed point of the rotation: it is its own image, so a line anchored on it
    // still has a transformed copy although the pivot is not an operated object.
    QVector<quint32> fixedPoints;
    if (initData.origin != NULL_ID)
    {
        fixedPoints.append(initData.origin);
    }

    patternGraph->AddEdge(initData.origin, initData.id);

    // The two other points of the dart are read again at every parse, exactly like the pivot. The moving point
    // is one of the objects being turned and is already declared by the loop below; the target point is not —
    // it does not travel — so nothing else declares it. Without this edge, deleting the point the dart lands on
    // makes the whole operation disappear at the next parse: the angle cannot be built, and the tool is simply
    // not there any more.
    patternGraph->AddEdge(initData.moving, initData.id);
    patternGraph->AddEdge(initData.target, initData.id);

    for (const auto &object : std::as_const(initData.source))
    {
        patternGraph->AddEdge(object.id, initData.id);
    }

    for (const auto &object : std::as_const(initData.destination))
    {
        patternGraph->AddVertex(object.id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, object.id);
    }

    // Rebuilt once the destination objects and their graph nodes exist. A line whose endpoint has no image is
    // not created.
    CreateDestinationLines(initData, fixedPoints);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        return AddToolToScene<OToolCloseDart>(initData, Tool::CloseDart, tr("close a dart"),
                                             [&initData]() { return new OToolCloseDart(initData); });
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCloseDart::Divisor() const -> qreal
{
    VFormula fDivisor(m_formulaDivisor, getData());
    fDivisor.setToolId(m_id);
    fDivisor.Eval();

    if (fDivisor.error())
    {
        return qQNaN(); // no divisor, hence no angle: the preview draws nothing rather than open a dialog
    }
    return fDivisor.getDoubleValue();
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCloseDart::GetFormulaDivisor() const -> VFormula
{
    VFormula fDivisor(m_formulaDivisor, getData());
    fDivisor.setToolId(m_id);
    fDivisor.Eval();
    return fDivisor;
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCloseDart::SetFormulaDivisor(const VFormula &value)
{
    // Zero and infinity cannot divide. Refusing them here is what keeps a pattern from holding a divisor that
    // the tool itself would refuse to read back — in which case the operation would simply disappear. The rule
    // is written once, in OCloseDartRule, and asked here rather than told again.
    if (value.error() || !OCloseDartRule::IsDivisorUsable(value.getDoubleValue()))
    {
        return;
    }

    m_formulaDivisor = value.GetFormula(FormulaType::FromUser);

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCloseDart::OriginPointName() const -> QString
{
    return PointName(getData(), m_origPointId, tr("Pivot point"));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCloseDart::MovingPointName() const -> QString
{
    return PointName(getData(), m_movingPointId, tr("Moving point"));
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCloseDart::TargetPointName() const -> QString
{
    return PointName(getData(), m_targetPointId, tr("Target point"));
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCloseDart::ShowVisualization(bool show)
{
    ShowToolVisualization<OVisToolCloseDart>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCloseDart::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    ShowContextMenuOf<ODialogCloseDart>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCloseDart::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<OVisToolCloseDart *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetObjects(SourceToObjects(m_source));

        QVector<QPair<quint32, quint32>> lines;
        lines.reserve(m_lines.size());
        for (const auto &line : std::as_const(m_lines))
        {
            lines.append(qMakePair(line.point1, line.point2));
        }
        visual->SetPreviewLines(lines);

        visual->SetOriginPointId(m_origPointId);
        visual->SetMovingPointId(m_movingPointId);
        visual->SetTargetPointId(m_targetPointId);
        visual->SetDivisor(Divisor());
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCloseDart::SaveDialog(QDomElement &domElement)
{
    const QPointer<ODialogCloseDart> dialogTool = DialogOf<ODialogCloseDart>();

    QString formulaDivisor = dialogTool->GetDivisor();

    doc->SetAttribute(domElement, AttrCenter, QString().setNum(dialogTool->GetOrigPointId()));
    doc->SetAttribute(domElement, AttrMovingPoint, QString().setNum(dialogTool->GetMovingPointId()));
    doc->SetAttribute(domElement, AttrTargetPoint, QString().setNum(dialogTool->GetTargetPointId()));
    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();
    const QVector<DestinationItem> destination = SyncDestination(source, formulaDivisor);
    // Sync destination may require a formula fix. Saving after synchronization
    doc->SetAttribute(domElement, AttrClosingDivisor, formulaDivisor);

    // Persist the dialog line selection: a line added here must be created, a line dropped must go, and the
    // transformed copies that already exist keep their id.
    m_lines = OOperationLines::SyncLines(getData(), m_lines, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);

    // Save visibility data for later use
    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCloseDart::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractOperation::ReadToolAttributes(domElement);

    m_origPointId = VAbstractPattern::GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
    m_movingPointId = VAbstractPattern::GetParametrUInt(domElement, AttrMovingPoint, NULL_ID_STR);
    m_targetPointId = VAbstractPattern::GetParametrUInt(domElement, AttrTargetPoint, NULL_ID_STR);
    m_formulaDivisor = VDomDocument::GetParametrString(domElement, AttrClosingDivisor,
                                                       OCloseDartRule::DefaultDivisorFormula());
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCloseDart::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractOperation::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrCenter, QString().setNum(m_origPointId));
    doc->SetAttribute(tag, AttrMovingPoint, QString().setNum(m_movingPointId));
    doc->SetAttribute(tag, AttrTargetPoint, QString().setNum(m_targetPointId));
    doc->SetAttribute(tag, AttrClosingDivisor, m_formulaDivisor);
}

//---------------------------------------------------------------------------------------------------------------------
auto OToolCloseDart::MakeToolTip() const -> QString
{
    // No point of the dart is named here: the three points are the ones the user clicked in the pattern, and
    // the tool says what it does with them rather than repeating their names.
    return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>"
                          "%3")
        .arg(tr("Closing divisor"),
             FormatDisplayNumber(RoundForDisplay(GetFormulaDivisor().getDoubleValue()))) // 1, 2
        .arg(VisibilityGroupToolTip());                                                 // 3
}

//---------------------------------------------------------------------------------------------------------------------
void OToolCloseDart::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<ODialogCloseDart>(oldDomElement, newDomElement);
}
