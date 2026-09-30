/************************************************************************
 **
 **  @file
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   1 10, 2016
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

#include "vtoolmove.h"

#include <QColor>
#include <QDomNode>
#include <QDomNodeList>
#include <QMapIterator>
#include <QPoint>
#include <QSharedPointer>
#include <QUndoStack>
#include <climits>
#include <qiterator.h>

#include "../../../dialogs/tools/dialogmove.h"
#include "../../../dialogs/tools/dialogtool.h"
#include "../../../visualization/line/operation/vistoolmove.h"
#include "../../../visualization/visualization.h"
#include "../../vabstracttool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/varc.h"
#include "../vgeometry/vcubicbezier.h"
#include "../vgeometry/vcubicbezierpath.h"
#include "../vgeometry/vellipticalarc.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vgeometry/vsplinepath.h"
#include "ooperationdestination.h"
#include "vexception.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vformula.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vmaingraphicsscene.h"

template <class T> class QSharedPointer;

const QString VToolMove::ToolType = QStringLiteral("moving");

namespace
{
auto GetOriginPoint(const QVector<SourceItem> &objects, const VContainer *data, qreal calcLength, qreal calcAngle)
    -> QPointF
{
    QPolygonF originObjects;

    for (const auto &object : objects)
    {
        const QSharedPointer<VGObject> obj = data->GetGObject(object.id);

        // This check helps to find missed objects in the switch
        Q_STATIC_ASSERT_X(static_cast<int>(GOType::Unknown) == 8, "Not all objects were handled.");

        switch (obj->getType())
        {
            case GOType::Point:
                originObjects.append(data->GeometricObject<VPointF>(object.id)->toQPointF());
                break;
            case GOType::Arc:
            case GOType::EllipticalArc:
            case GOType::Spline:
            case GOType::SplinePath:
            case GOType::CubicBezier:
            case GOType::CubicBezierPath:
                originObjects.append(data->GeometricObject<VAbstractCurve>(object.id)->GetPoints());
                break;
            case GOType::PlaceLabel:
            case GOType::Unknown:
                Q_UNREACHABLE();
                break;
            default:
                break;
        }
    }

    QPointF const rotationOrigin = originObjects.boundingRect().center();
    QLineF move(rotationOrigin, QPointF(rotationOrigin.x() + calcLength, rotationOrigin.y()));
    move.setAngle(calcAngle);
    return move.p2();
}
//---------------------------------------------------------------------------------------------------------------------
/** @brief The transformation of a move: a translation, then a rotation of the whole pattern around the rotation
 *  origin. Points and curves all answer to Move(length, angle, name) and Rotate(origin, angle, name), so one
 *  template serves every shape. */
class OMoveTransform
{
public:
    OMoveTransform(QPointF rotationOrigin, qreal angle, qreal length, qreal rotationAngle)
      : m_rotationOrigin(std::move(rotationOrigin)),
        m_angle(angle),
        m_length(length),
        m_rotationAngle(rotationAngle)
    {
    }

    template <class Item> auto Image(const Item &item, const QString &name) const -> Item
    {
        return item.Move(m_length, m_angle, name).Rotate(m_rotationOrigin, m_rotationAngle, name);
    }

private:
    QPointF m_rotationOrigin;
    qreal m_angle;
    qreal m_length;
    qreal m_rotationAngle;
};

//---------------------------------------------------------------------------------------------------------------------
auto MakeDestination(QPointF rotationOrigin, qreal calcAngle, qreal calcLength, qreal calcRotationAngle)
    -> OOperationDestination<OMoveTransform>
{
    return OOperationDestination<OMoveTransform>(
        OMoveTransform(std::move(rotationOrigin), calcAngle, calcLength, calcRotationAngle));
}

//---------------------------------------------------------------------------------------------------------------------
void CreateDestination(VToolMoveInitData &initData, QPointF rotationOrigin, qreal calcAngle, qreal calcLength)
{
    qreal const calcRotationAngle = VAbstractTool::CheckFormula(initData.id,
                                                                initData.formulaRotationAngle,
                                                                initData.data);
    const OOperationDestination<OMoveTransform> destination
        = MakeDestination(std::move(rotationOrigin), calcAngle, calcLength, calcRotationAngle);

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
void VToolMove::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogMove> dialogTool = qobject_cast<DialogMove *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetAngle(m_formulaAngle);
    dialogTool->SetRotationAngle(m_formulaRotationAngle);
    dialogTool->SetLength(m_formulaLength);
    dialogTool->SetRotationOrigPointId(m_origPointId);
    dialogTool->SetNotes(m_notes);
    dialogTool->SetSourceObjects(m_source);

    // Only the lines explicitly selected by the user come back into the dialog: the implicit ones
    // follow their points.
    QVector<quint32> lineSources;
    for (const auto &line : Lines())
    {
        if (OOperationLines::IsExplicit(line))
        {
            lineSources.append(line.source);
        }
    }
    dialogTool->SetLineSources(lineSources);
    dialogTool->SetDestinationObjects(m_destination);

    SetDialogVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolMove::Create(const QPointer<DialogTool> &dialog, VMainGraphicsScene *scene, VAbstractPattern *doc,
                       VContainer *data) -> VToolMove *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogMove> dialogTool = qobject_cast<DialogMove *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolMoveInitData initData;
    initData.formulaAngle = dialogTool->GetAngle();
    initData.formulaRotationAngle = dialogTool->GetRotationAngle();
    initData.formulaLength = dialogTool->GetLength();
    initData.rotationOrigin = dialogTool->GetRotationOrigPointId();
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

    VToolMove *operation = Create(initData);
    if (operation != nullptr)
    {
        operation->m_dialog = dialog;
    }
    return operation;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolMove::Create(VToolMoveInitData &initData) -> VToolMove *
{
    if (initData.typeCreation == Source::FromGui)
    {
        initData.destination.clear(); // Try to avoid mistake, value must be empty

        initData.id = initData.data->getNextId(); // Just reserve id for tool
    }
    else
    {
        PrepareNames(initData);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();
    initData.doc->FindFormulaDependencies(initData.formulaAngle, initData.id, varData);
    initData.doc->FindFormulaDependencies(initData.formulaLength, initData.id, varData);
    initData.doc->FindFormulaDependencies(initData.formulaRotationAngle, initData.id, varData);

    qreal const calcAngle = CheckFormula(initData.id, initData.formulaAngle, initData.data);

    qreal const calcLength = VAbstractValApplication::VApp()->toPixel(
        CheckFormula(initData.id, initData.formulaLength, initData.data));

    QPointF rotationOrigin;
    if (initData.rotationOrigin == NULL_ID)
    {
        rotationOrigin = GetOriginPoint(initData.source, initData.data, calcLength, calcAngle);
    }
    else
    {
        QSharedPointer<VPointF> const originPoint = initData.data->GeometricObject<VPointF>(initData.rotationOrigin);
        rotationOrigin = static_cast<QPointF>(*originPoint);
        patternGraph->AddEdge(initData.rotationOrigin, initData.id);
    }

    CreateDestination(initData, rotationOrigin, calcAngle, calcLength);

    for (const auto &object : std::as_const(initData.source))
    {
        patternGraph->AddEdge(object.id, initData.id);
    }

    for (const auto &object : std::as_const(initData.destination))
    {
        patternGraph->AddVertex(object.id, VNodeType::OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());
        patternGraph->AddEdge(initData.id, object.id);
    }

    // Rebuilt once the destination objects and their graph nodes exist. A move has no fixed point: a line
    // is transformed only when both of its endpoints are operated.
    CreateDestinationLines(initData, {});

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        return AddToolToScene<VToolMove>(initData, Tool::Move, tr("move"),
                                         [&initData]() { return new VToolMove(initData); });
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolMove::GetFormulaAngle() const -> VFormula
{
    VFormula fAngle(m_formulaAngle, getData());
    fAngle.setToolId(m_id);
    fAngle.setPostfix(degreeSymbol);
    fAngle.Eval();
    return fAngle;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::SetFormulaAngle(const VFormula &value)
{
    if (!value.error())
    {
        m_formulaAngle = value.GetFormula(FormulaType::FromUser);

        QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
        SaveOption(obj);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolMove::GetFormulaRotationAngle() const -> VFormula
{
    VFormula fAngle(m_formulaRotationAngle, getData());
    fAngle.setToolId(m_id);
    fAngle.setPostfix(degreeSymbol);
    fAngle.Eval();
    return fAngle;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::SetFormulaRotationAngle(const VFormula &value)
{
    if (!value.error())
    {
        m_formulaRotationAngle = value.GetFormula(FormulaType::FromUser);

        QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
        SaveOption(obj);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolMove::GetFormulaLength() const -> VFormula
{
    VFormula fLength(m_formulaLength, getData());
    fLength.setToolId(m_id);
    fLength.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    fLength.Eval();
    return fLength;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::SetFormulaLength(const VFormula &value)
{
    if (!value.error())
    {
        m_formulaLength = value.GetFormula(FormulaType::FromUser);

        QSharedPointer<VGObject> obj = VContainer::GetFakeGObject(m_id);
        SaveOption(obj);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolMove::OriginPointName() const -> QString
{
    try
    {
        return VAbstractTool::data.GetGObject(m_origPointId)->name();
    }
    catch (const VExceptionBadId &)
    {
        return tr("Center point");
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::ShowVisualization(bool show)
{
    ShowToolVisualization<VisToolMove>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    ShowContextMenuOf<DialogMove>(event, id);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::SetVisualization()
{
    if (not vis.isNull())
    {
        auto *visual = qobject_cast<VisToolMove *>(vis);
        SCASSERT(visual != nullptr)

        const bool osSeparator = VAbstractApplication::VApp()->Settings()->GetOsSeparator();
        const VTranslateVars *trVars = VAbstractApplication::VApp()->TrVars();

        visual->SetObjects(SourceToObjects(m_source));

        QVector<QPair<quint32, quint32>> previewLines;
        previewLines.reserve(m_lines.size());
        for (const auto &line : std::as_const(m_lines))
        {
            previewLines.append(qMakePair(line.point1, line.point2));
        }
        visual->SetPreviewLines(previewLines);
        visual->SetAngle(trVars->FormulaToUser(m_formulaAngle, osSeparator));
        visual->SetRotationAngle(trVars->FormulaToUser(m_formulaRotationAngle, osSeparator));
        visual->SetLength(trVars->FormulaToUser(m_formulaLength, osSeparator));
        visual->SetRotationOriginPointId(m_origPointId);
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::SaveDialog(QDomElement &domElement)
{
    const QPointer<DialogMove> dialogTool = DialogOf<DialogMove>();

    QString formulaAngle = dialogTool->GetAngle();
    QString formulaRotationAngle = dialogTool->GetRotationAngle();
    QString formulaLength = dialogTool->GetLength();

    doc->SetAttribute(domElement, AttrCenter, QString().setNum(dialogTool->GetRotationOrigPointId()));

    SaveCommonDialogData(domElement, dialogTool->GetNotes());

    const QVector<SourceItem> source = dialogTool->GetSourceObjects();
    const QVector<DestinationItem> destination = SyncDestination(source,
                                                                 formulaLength,
                                                                 formulaAngle,
                                                                 formulaRotationAngle);
    // Sync destination may require a formula fix. Saving after synchronization
    doc->SetAttribute(domElement, AttrAngle, formulaAngle);
    doc->SetAttribute(domElement, AttrLength, formulaLength);
    doc->SetAttribute(domElement, AttrRotationAngle, formulaRotationAngle);

    // Persist the dialog line selection: a line added here must be created, a line dropped must go, and
    // the transformed copies that already exist keep their id.
    m_lines = OOperationLines::SyncLines(getData(), m_lines, source, dialogTool->GetLineSources());

    SaveSourceDestination(domElement, source, destination);

    // Save visibility data for later use
    SaveVisibilityGroupData(dialogTool);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::ReadToolAttributes(const QDomElement &domElement)
{
    VAbstractOperation::ReadToolAttributes(domElement);

    m_origPointId = VDomDocument::GetParametrUInt(domElement, AttrCenter, NULL_ID_STR);
    m_formulaAngle = VDomDocument::GetParametrString(domElement, AttrAngle, QChar('0'));
    m_formulaRotationAngle = VDomDocument::GetParametrString(domElement, AttrRotationAngle, QChar('0'));
    m_formulaLength = VDomDocument::GetParametrString(domElement, AttrLength, QChar('0'));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VAbstractOperation::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);
    doc->SetAttribute(tag, AttrAngle, m_formulaAngle);
    doc->SetAttribute(tag, AttrRotationAngle, m_formulaRotationAngle);
    doc->SetAttribute(tag, AttrLength, m_formulaLength);
    doc->SetAttribute(tag, AttrCenter, QString().setNum(m_origPointId));
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolMove::MakeToolTip() const -> QString
{
    return QStringLiteral("<tr> <td><b>%1:</b> %2°</td> </tr>"
                          "<tr> <td><b>%3:</b> %4 %5</td> </tr>"
                          "<tr> <td><b>%6:</b> %7°</td> </tr>"
                          "<tr> <td><b>%8:</b> %9</td> </tr>"
                          "%10")
        .arg(tr("Angle"))                                                       // 1
        .arg(GetFormulaAngle().getDoubleValue())                                // 2
        .arg(tr("Length"))                                                      // 3
        .arg(GetFormulaLength().getDoubleValue())                               // 4
        .arg(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true), // 5
             tr("Rotation angle"))                                              // 6
        .arg(GetFormulaRotationAngle().getDoubleValue())                        // 7
        .arg(tr("Pivot point"),                                                 // 8
             OriginPointName(),                                                 // 9
             VisibilityGroupToolTip());                                         // 10
}

//---------------------------------------------------------------------------------------------------------------------
void VToolMove::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ApplyDialogOptions<DialogMove>(oldDomElement, newDomElement);
}

//---------------------------------------------------------------------------------------------------------------------
VToolMove::VToolMove(const VToolMoveInitData &initData, QGraphicsItem *parent)
  : VAbstractOperation(initData, parent),
    m_formulaAngle(initData.formulaAngle),
    m_formulaRotationAngle(initData.formulaRotationAngle),
    m_formulaLength(initData.formulaLength),
    m_origPointId(initData.rotationOrigin)
{
    InitOperatedObjects();
    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolMove::SyncDestination(const QVector<SourceItem> &source,
                                QString &formulaLength,
                                QString &formulaAngle,
                                QString &formulaRotationAngle) -> QVector<DestinationItem>
{
    bool unchanged = false;
    const QMap<QUuid, DestinationItem> existingByRecord = SyncDestinationPreamble(source, unchanged);
    if (unchanged)
    {
        return m_destination;
    }

    qreal const calcAngle = CheckFormula(m_id, formulaAngle, getData());
    qreal const calcLength = VAbstractValApplication::VApp()->toPixel(CheckFormula(m_id, formulaLength, getData()));
    qreal const calcRotationAngle = VAbstractTool::CheckFormula(m_id, formulaRotationAngle, getData());

    QPointF rotationOrigin;
    if (m_origPointId == NULL_ID)
    {
        rotationOrigin = GetOriginPoint(source, getData(), calcLength, calcAngle);
    }
    else
    {
        QSharedPointer<VPointF> const originPoint = getData()->GeometricObject<VPointF>(m_origPointId);
        rotationOrigin = static_cast<QPointF>(*originPoint);
    }

    const OOperationDestination<OMoveTransform> destination
        = MakeDestination(rotationOrigin, calcAngle, calcLength, calcRotationAngle);

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
