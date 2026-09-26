/************************************************************************
 **
 **  @file   vtoolrotation.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   12 4, 2016
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

#include "vtoolrotation.h"

#include <QColor>
#include <QDomNode>
#include <QDomNodeList>
#include <QMapIterator>
#include <QPoint>
#include <QSharedPointer>
#include <QUndoStack>
#include <climits>

#include "../../../dialogs/tools/dialogrotation.h"
#include "../../../dialogs/tools/dialogtool.h"
#include "../../../visualization/line/operation/vistoolrotation.h"
#include "../../../visualization/visualization.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgeometrydef.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "ooperationdestination.h"
#include "orotationlength.h"
#include "vexception.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vmaingraphicsscene.h"

class QDomElement;
class QGraphicsSceneContextMenuEvent;
class QPainter;
class QStyleOptionGraphicsItem;
class QWidget;
template <class T> class QSharedPointer;

const QString VToolRotation::ToolType = QStringLiteral("rotation");
const QString VToolRotation::DefaultAngle = QStringLiteral("10");

namespace
{
//---------------------------------------------------------------------------------------------------------------------
/** @brief The transformation of a rotation: every source object is turned around the pivot by the same angle.
 *  Points, arcs, splines and the three curve-with-segments types all answer to Rotate(origin, angle, name),
 *  so one template serves every shape. */
class ORotationTransform
{
public:
    ORotationTransform(const QPointF &origin, qreal angle) : m_origin(origin), m_angle(angle) {}

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        return item.Rotate(m_origin, m_angle, name);
    }

private:
    QPointF m_origin;
    qreal m_angle;
};

//---------------------------------------------------------------------------------------------------------------------
auto MakeDestination(quint32 originId, qreal calcAngle, VContainer *data)
    -> OOperationDestination<ORotationTransform>
{
    const auto originPoint = *data->GeometricObject<VPointF>(originId);
    return OOperationDestination<ORotationTransform>(
        ORotationTransform(static_cast<QPointF>(originPoint), calcAngle));
}

//---------------------------------------------------------------------------------------------------------------------
void CreateDestination(VToolRotationInitData &initData)
{
    qreal const calcAngle = VAbstractTool::CheckFormula(initData.id, initData.angle, initData.data);
    const OOperationDestination<ORotationTransform> destination = MakeDestination(initData.origin, calcAngle,
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
VToolRotation::VToolRotation(const VToolRotationInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_origPointId(initData.origin),
    m_formulaAngle(initData.angle)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::SyncDestination(const QVector<SourceItem> &source, QString &formulaAngle)
    -> QVector<DestinationItem>
{
    bool unchanged = false;
    const QMap<QUuid, DestinationItem> existingByRecord = SyncDestinationPreamble(source, unchanged);
    if (unchanged)
    {
        return m_destination;
    }

    qreal const calcAngle = CheckFormula(m_id, formulaAngle, getData());
    const OOperationDestination<ORotationTransform> destination = MakeDestination(m_origPointId, calcAngle, getData());

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
void VToolRotation::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogRotation> dialogTool = qobject_cast<DialogRotation *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetOrigPointId(m_origPointId);
    dialogTool->SetAngle(m_formulaAngle);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);
    dialogTool->SetDestinationObjects(m_destination);

    // Only the lines explicitly selected by the user come back into the dialog: the implicit ones follow
    // their points.
    QVector<quint32> lineSources;
    for (const auto &line : Lines())
    {
        if (OOperationLines::IsExplicit(line))
        {
            lineSources.append(line.source);
        }
    }
    dialogTool->SetLineSources(lineSources);

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                           VContainer *data) -> VToolRotation *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogRotation> dialogTool = qobject_cast<DialogRotation *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolRotationInitData initData;
    initData.origin = dialogTool->GetOrigPointId();
    initData.angle = dialogTool->GetAngle();
    initData.source = dialogTool->GetSourceObjects();
    initData.lines = OOperationLines::Collect(data, initData.source, dialogTool->GetLineSources());
    initData.hasLinkedVisibilityGroup = dialogTool->HasLinkedVisibilityGroup();
    initData.visibilityGroupName = dialogTool->GetVisibilityGroupName();
    initData.visibilityGroupTags = dialogTool->GetVisibilityGroupTags();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();

    VToolRotation *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::Create(VToolRotationInitData &initData) -> VToolRotation *
{
    if (initData.typeCreation == Source::FromGui)
    {
        initData.destination.clear(); // Try to avoid mistake, value must be empty

        initData.id = initData.data->getNextId(); // Just reserve id for tool

        // The transformed lines are persisted with the operation, so their ids are reserved now, before
        // the operation element is written.
        OOperationLines::AllocateResultIds(initData.data, initData.lines);
    }
    else
    {
        PrepareNames(initData);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();
    initData.doc->FindFormulaDependencies(initData.angle, initData.id, varData);

    CreateDestination(initData);

    // The centre of rotation is a fixed point of the transformation: it is its own image, so a line
    // anchored on it still has a transformed copy although the centre is not an operated object.
    QVector<quint32> fixedPoints;
    if (initData.origin != NULL_ID)
    {
        fixedPoints.append(initData.origin);
    }

    patternGraph->AddEdge(initData.origin, initData.id);

    for (const auto &object : std::as_const(initData.source))
    {
        patternGraph->AddEdge(object.id, initData.id);
    }

    for (const auto &object : std::as_const(initData.destination))
    {
        patternGraph->AddVertex(object.id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, object.id);
    }

    // Rebuilt once the destination objects and their graph nodes exist. A line whose endpoint has no image
    // is not created.
    CreateDestinationLines(initData, fixedPoints);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        if (initData.typeCreation == Source::FromGui && initData.hasLinkedVisibilityGroup)
        {
            VAbstractApplication::VApp()->getUndoStack()->beginMacro(tr("rotate"));
        }

        VAbstractTool::AddRecord(initData.id, Tool::Rotation, initData.doc);
        auto *tool = new VToolRotation(initData);
        initData.scene->addItem(tool);
        InitOperationToolConnections(initData.scene, tool);
        VAbstractPattern::AddTool(initData.id, tool);

        if (initData.typeCreation == Source::FromGui && initData.hasLinkedVisibilityGroup)
        {
            VAbstractOperation::CreateVisibilityGroup(initData);
            VAbstractApplication::VApp()->getUndoStack()->endMacro();
        }

        return tool;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::OriginPointName() const -> QString
{
    // The name is read by the tooltip and by the dock, both of which can be asked about a tool whose pivot is
    // no longer in the pattern: letting the exception out would end the application (DialogTool::SetObject and
    // the property browser are not the place to catch it). Same guard as VToolMove::OriginPointName.
    try
    {
        return VAbstractTool::data.GetGObject(m_origPointId)->name();
    }
    catch (const VExceptionBadId &)
    {
        return tr("Pivot point");
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::GetFormulaAngle() const -> VFormula
{
    VFormula fAngle(m_formulaAngle, getData());
    fAngle.setToolId(m_id);
    fAngle.setPostfix(degreeSymbol);
    fAngle.Eval();
    return fAngle;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolRotation::SetFormulaAngle(const VFormula &value)
{
    if (!value.error())
    {
        m_formulaAngle = value.GetFormula(FormulaType::FromUser);

        QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
        SaveOption(obj);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::ReferenceRadius() const -> qreal
{
    const VContainer *container = getData();
    const quint32 reference = ORotationLength::ReferencePoint(container, SourceToObjects(m_source));
    return ORotationLength::Radius(container, m_origPointId, reference);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::HasReferencePoint() const -> bool
{
    return ORotationLength::HasRadius(ReferenceRadius());
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::GetFormulaLength() const -> VFormula
{
    const qreal radius = ReferenceRadius();
    QString length = QStringLiteral("0"); // no reference point: no length to speak of, the dock shows a dash

    if (ORotationLength::HasRadius(radius))
    {
        // The same rule as the dialog field: what an angle means as a length, in the unit the user reads.
        length = FormatDisplayNumber(ORotationLength::LengthOfAngle(radius,
                                                                   GetFormulaAngle().getDoubleValue(),
                                                                   *getData()->GetPatternUnit()));
    }

    VFormula fLength(length, getData());
    fLength.setToolId(m_id);
    fLength.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    fLength.Eval();
    return fLength;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::SetFormulaLength(const VFormula &value) -> bool
{
    if (value.error())
    {
        return false;
    }

    // Same rule as the dialog field, so the two cannot refuse or accept the same length differently.
    const qreal angle = ORotationLength::AngleOfLength(ReferenceRadius(), value.getDoubleValue(),
                                                       *getData()->GetPatternUnit());
    if (qIsNaN(angle))
    {
        return false; // further from the pivot than the diameter of the circle it draws around it
    }

    // A rotation is stored as an angle, so a length entered here takes its place: the two say the same thing,
    // and the length is not something a pattern keeps (see Rotation.md).
    m_formulaAngle = FormatDisplayNumber(angle);

    QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
    SaveOption(obj);
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolRotation::ShowVisualization(bool show)
{
    ShowToolVisualization<VisToolRotation>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolRotation::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    ShowContextMenuOf<DialogRotation>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolRotation::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<VisToolRotation *>(vis);
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
        visual->SetAngle(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            m_formulaAngle, VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolRotation::SaveDialog(QDomElement &domElement)
{
    const QPointer<DialogRotation> dialogTool = DialogOf<DialogRotation>();

    QString formulaAngle = dialogTool->GetAngle();

    doc->SetAttribute(domElement, AttrCenter, QString().setNum(dialogTool->GetOrigPointId()));
    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();
    const QVector<DestinationItem> destination = SyncDestination(source, formulaAngle);
    // Sync destination may require a formula fix. Saving after synchronization
    doc->SetAttribute(domElement, AttrAngle, formulaAngle);

    // Persist the dialog line selection: a line added here must be created, a line dropped must go, and
    // the transformed copies that already exist keep their id.
    m_lines = OOperationLines::SyncLines(getData(), m_lines, source, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);

    // Save visibility data for later use
    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolRotation::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractOperation::ReadToolAttributes(domElement);

    m_origPointId = VAbstractPattern::GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
    m_formulaAngle = VDomDocument::GetParametrString(domElement, AttrAngle, VToolRotation::DefaultAngle);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolRotation::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractOperation::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrCenter, QString().setNum(m_origPointId));
    doc->SetAttribute(tag, AttrAngle, m_formulaAngle);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolRotation::MakeToolTip() const -> QString
{
    return QStringLiteral("<tr> <td><b>%1:</b> %2</td> </tr>"
                          "<tr> <td><b>%3:</b> %4°</td> </tr>"
                          "%5")
        .arg(tr("Pivot point"), OriginPointName(), tr("Rotation angle"))  // 1, 2, 3
        .arg(FormatDisplayNumber(RoundForDisplay(GetFormulaAngle().getDoubleValue()))) // 4: read, not raw
        .arg(VisibilityGroupToolTip());                                   // 5
}

//---------------------------------------------------------------------------------------------------------------------
void VToolRotation::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<DialogRotation>(oldDomElement, newDomElement);
}
