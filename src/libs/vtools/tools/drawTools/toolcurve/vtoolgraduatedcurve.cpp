/************************************************************************
 **
 **  @file   vtoolgraduatedcurve.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 12, 2025
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2025 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 09/10/2026.
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
#include "vtoolgraduatedcurve.h"

#include "../../../dialogs/tools/dialoggraduatedcurve.h"
#include "../../../visualization/path/vistoolgraduatedcurve.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vgeometry/vabstractcurve.h"
#include "../vgeometry/vsplinepath.h"
#include "../toolpoint/toolsinglepoint/otoolcurvepoint.h"
#include "../vmisc/compatibility.h"
#include "../vpatterndb/variables/vincrement.h"

using namespace Qt::Literals::StringLiterals;

const QString VToolGraduatedCurve::ToolType = QStringLiteral("graduatedCurve");

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::SetDialog()
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogGraduatedCurve> dialogTool = qobject_cast<DialogGraduatedCurve *>(m_dialog);
    SCASSERT(not dialogTool.isNull())
    const QSharedPointer<VSplinePath> splPath = VAbstractTool::data.GeometricObject<VSplinePath>(m_id);
    dialogTool->CheckDependencyTreeComplete();
    dialogTool->SetOriginCurveId(OriginCurveId());
    dialogTool->SetOffsets(m_offsets);
    dialogTool->SetPenStyle(splPath->GetPenStyle());
    dialogTool->SetColor(splPath->GetColor());
    dialogTool->SetName(GetName());
    dialogTool->SetAliasSuffix(splPath->GetAliasSuffix());
    dialogTool->SetNotes(m_notes);
    // Whether the curve carries a point at either end is a fact of the pattern, written nowhere in the dialog's own
    // options: the tab shows it, rather than a box left unchecked on a curve that has had one for months.
    dialogTool->SetStartPoint(HasStartPoint(), StartPointName(), StartPointShowLabel());
    dialogTool->SetEndPoint(HasEndPoint(), EndPointName(), EndPointShowLabel());
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief ApplyPointOptions gives the curve the two points the dialog's Point tab describes, or takes them away.
 *
 *  The boxes of that tab are the dialog's own copy of a fact the pattern owns — a point exists at an end of the curve,
 *  or it does not — and the property browser has always been able to change it, while the dialog could only ever
 *  speak once, at creation. This is the dialog's voice after that: the same two setters the browser uses, which do
 *  nothing at all when the point is already the one asked for.
 */
void VToolGraduatedCurve::ApplyPointOptions()
{
    auto *dialogTool = qobject_cast<DialogGraduatedCurve *>(m_dialog.data());
    if (dialogTool == nullptr)
    {
        return;
    }

    if (HasStartPoint() && StartPointShowLabel() != dialogTool->GetStartShowLabel())
    {
        SetStartPointShowLabel(dialogTool->GetStartShowLabel());
    }
    if (HasEndPoint() && EndPointShowLabel() != dialogTool->GetEndShowLabel())
    {
        SetEndPointShowLabel(dialogTool->GetEndShowLabel());
    }

    SetCreateStartPoint(dialogTool->IsCreateStartPoint(), dialogTool->GetStartPointName(),
                        dialogTool->GetStartShowLabel());
    SetCreateEndPoint(dialogTool->IsCreateEndPoint(), dialogTool->GetEndPointName(),
                      dialogTool->GetEndShowLabel());

    // The tab shows what the pattern now holds: a point that has just been created is not renamed from a field that
    // would take a new name and drop it.
    dialogTool->SetStartPoint(HasStartPoint(), StartPointName(), StartPointShowLabel());
    dialogTool->SetEndPoint(HasEndPoint(), EndPointName(), EndPointShowLabel());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::FullUpdateFromGuiApply()
{
    ApplyPointOptions();
    VToolAbstractOffsetCurve::FullUpdateFromGuiApply();
}//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::FullUpdateFromGuiOk(int result)
{
    if (result == QDialog::Accepted)
    {
        ApplyPointOptions();
    }

    VToolAbstractOffsetCurve::FullUpdateFromGuiOk(result);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolGraduatedCurve::Create(const QPointer<DialogTool> &dialog,
                                 VMainGraphicsScene *scene,
                                 VAbstractPattern *doc,
                                 VContainer *data) -> VToolGraduatedCurve *
{
    SCASSERT(not dialog.isNull())
    const QPointer<DialogGraduatedCurve> dialogTool = qobject_cast<DialogGraduatedCurve *>(dialog);
    SCASSERT(not dialogTool.isNull())

    VToolGraduatedCurveInitData initData;
    initData.originCurveId = dialogTool->GetOriginCurveId();
    initData.offsets = dialogTool->GetOffsets();
    initData.name = dialogTool->GetName();
    initData.color = dialogTool->GetColor();
    initData.penStyle = dialogTool->GetPenStyle();
    initData.scene = scene;
    initData.doc = doc;
    initData.data = data;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.notes = dialogTool->GetNotes();
    initData.aliasSuffix = dialogTool->GetAliasSuffix();
    initData.createStartPoint = dialogTool->IsCreateStartPoint();
    initData.createEndPoint = dialogTool->IsCreateEndPoint();
    initData.startPointName = dialogTool->GetStartPointName();
    initData.endPointName = dialogTool->GetEndPointName();
    initData.startShowLabel = dialogTool->GetStartShowLabel();
    initData.endShowLabel = dialogTool->GetEndShowLabel();

    VToolGraduatedCurve *curve = Create(initData);
    if (curve != nullptr)
    {
        curve->m_dialog = dialog;
    }
    return curve;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolGraduatedCurve::Create(VToolGraduatedCurveInitData &initData) -> VToolGraduatedCurve *
{
    QVector<qreal> widths;
    widths.reserve(initData.offsets.size());
    {
        // Own table first: the offsets below are scratch variables, and this copy shares the living pattern's
        // table — left attached, each one would be registered in the pattern itself.
        VContainer localData = *initData.data;
        localData.DetachVariables();

        for (auto &offset : initData.offsets)
        {
            qreal const calcWidth = VAbstractValApplication::VApp()->toPixel(
                CheckFormula(initData.id, offset.formula, &localData));

            auto *offsetVal = new VIncrement(&localData, offset.name);
            offsetVal->SetFormula(calcWidth, offset.formula, true);
            offsetVal->SetDescription(offset.description.isEmpty() ? tr("Offset %1").arg(offset.name)
                                                                   : offset.description);

            localData.AddVariable(offsetVal);

            widths.append(calcWidth);
        }
    }

    const QSharedPointer<VAbstractCurve> curve = initData.data->GeometricObject<VAbstractCurve>(initData.originCurveId);

    VSplinePath splPath = curve->Outline(widths, initData.name);
    splPath.SetColor(initData.color);
    splPath.SetPenStyle(initData.penStyle);
    splPath.SetApproximationScale(initData.approximationScale);
    splPath.SetAliasSuffix(initData.aliasSuffix);

    if (initData.typeCreation == Source::FromGui)
    {
        initData.id = initData.data->AddGObject(new VSplinePath(splPath));
        initData.data->AddSpline(initData.data->GeometricObject<VAbstractBezier>(initData.id), initData.id);
    }
    else
    {
        initData.data->UpdateGObject(initData.id, splPath);
    }

    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::TOOL, initData.doc->PatternBlockMapper()->GetActiveId());

    const auto varData = initData.data->DataDependencyVariables();
    for (const auto &offset : std::as_const(initData.offsets))
    {
        initData.doc->FindFormulaDependencies(offset.formula, initData.id, varData);
    }

    initData.data->AddSpline(initData.data->GeometricObject<VAbstractBezier>(initData.id), initData.id);

    patternGraph->AddEdge(initData.originCurveId, initData.id);

    if (initData.typeCreation != Source::FromGui && initData.parse != Document::FullParse)
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::GraduatedCurve, initData.doc);
        auto *path = new VToolGraduatedCurve(initData);
        initData.scene->addItem(path);
        InitSplinePathToolConnections(initData.scene, path);
        VAbstractPattern::AddTool(initData.id, path);

        // Create the optional start/end points (GUI creation only).
        if (initData.typeCreation == Source::FromGui)
        {
            if (initData.createStartPoint)
            {
                OToolCurvePointInitData pointData;
                pointData.splineId = initData.id;
                pointData.position = CurvePointPosition::Start;
                pointData.name = initData.startPointName;
                pointData.showLabel = initData.startShowLabel;
                pointData.scene = initData.scene;
                pointData.doc = initData.doc;
                pointData.data = initData.data;
                pointData.parse = initData.parse;
                pointData.typeCreation = Source::FromGui;
                OToolCurvePoint::Create(pointData);
            }
            if (initData.createEndPoint)
            {
                OToolCurvePointInitData pointData;
                pointData.splineId = initData.id;
                pointData.position = CurvePointPosition::End;
                pointData.name = initData.endPointName;
                pointData.showLabel = initData.endShowLabel;
                pointData.scene = initData.scene;
                pointData.doc = initData.doc;
                pointData.data = initData.data;
                pointData.parse = initData.parse;
                pointData.typeCreation = Source::FromGui;
                OToolCurvePoint::Create(pointData);
            }
        }

        return path;
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolGraduatedCurve::GetGraduatedOffsets() const -> QVector<VGraduatedCurveOffset>
{
    QVector<VGraduatedCurveOffset> widths;
    widths.reserve(m_offsets.size());

    // Each VFormula below only stores a pointer to its container, and callers keep using
    // that pointer after this call returns (e.g. the property browser's "fx" button), so the
    // container must be this tool's own persistent one, not a local variable that goes out of
    // scope here (see VToolCut::GetFormulaLength()).
    auto *container = const_cast<VContainer *>(getData());

    for (const auto &offset : m_offsets)
    {
        VFormula width(offset.formula, container);
        width.setToolId(m_id);
        width.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
        width.Eval();

        auto *offsetVal = new VIncrement(container, offset.name);
        offsetVal->SetFormula(width.getDoubleValue(), offset.formula, width.error());

        container->AddVariable(offsetVal);

        widths.append({.name = offset.name, .offset = width, .description = offset.description});
    }

    return widths;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::SetGraduatedOffsets(const QVector<VGraduatedCurveOffset> &offsets)
{
    m_offsets.clear();
    m_offsets.reserve(offsets.size());

    for (const auto &offset : offsets)
    {
        QString formula = "0"_L1;
        if (!offset.offset.error())
        {
            formula = offset.offset.GetFormula(FormulaType::FromUser);
        }

        m_offsets.append({.name = offset.name, .formula = formula, .description = offset.description});
    }

    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::ShowVisualization(bool show)
{
    ShowToolVisualization<VisToolGraduatedCurve>(show);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::OpenOptionsDialog()
{
    ShowOptionsDialog<DialogGraduatedCurve>();
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolGraduatedCurve::ExtractOffsetData(const QDomElement &domElement) -> QVector<VRawGraduatedCurveOffset>
{
    QVector<VRawGraduatedCurveOffset> offsets;
    const QDomNodeList nodeList = domElement.childNodes();
    offsets.reserve(nodeList.size());

    QDOM_LOOP(nodeList, i)
    {
        if (const QDomElement element = QDOM_ELEMENT(nodeList, i).toElement();
            not element.isNull() && element.tagName() == VAbstractPattern::TagOffset)
        {
            offsets.append({.name = VDomDocument::GetParametrString(element, AttrName),
                            .formula = VDomDocument::GetParametrString(element, AttrWidth, QChar('0')),
                            .description = VDomDocument::GetParametrEmptyString(element, AttrDescription)});
        }
    }

    return offsets;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::ShowContextMenu(QGraphicsSceneContextMenuEvent *event, quint32 id)
{
    Q_UNUSED(id)
    try
    {
        ContextMenu<DialogGraduatedCurve>(event);
    }
    catch (const VExceptionToolWasDeleted &e)
    {
        Q_UNUSED(e)
        return; // Leave this method immediately!!!
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::SaveDialog(QDomElement &domElement)
{
    SCASSERT(not m_dialog.isNull())
    QPointer<DialogGraduatedCurve> const dialogTool = qobject_cast<DialogGraduatedCurve *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    doc->SetAttribute(domElement, AttrCurve, dialogTool->GetOriginCurveId());
    doc->SetAttribute(domElement, AttrName, dialogTool->GetName());
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

    QVector<VRawGraduatedCurveOffset> const offsets = dialogTool->GetOffsets();
    UpdateOffsets(domElement, offsets);

    // We no longer need to handle suffix attribute here. The code can be removed.
    Q_STATIC_ASSERT(VPatternConverter::PatternMinVer < FormatVersion(1, 1, 1));
    if (!dialogTool->GetName().isEmpty() && domElement.hasAttribute(AttrSuffix))
    {
        domElement.removeAttribute(AttrSuffix);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolAbstractOffsetCurve::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrType, ToolType);

    UpdateOffsets(tag, m_offsets);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::SetVisualization()
{
    if (not vis.isNull())
    {
        QSharedPointer<VAbstractCurve> const curve = VAbstractTool::data.GeometricObject<VAbstractCurve>(m_id);
        auto *visual = qobject_cast<VisToolGraduatedCurve *>(vis);
        SCASSERT(visual != nullptr)

        visual->SetCurveId(OriginCurveId());

        const bool osSeparator = VAbstractApplication::VApp()->Settings()->GetOsSeparator();
        const VTranslateVars *trVars = VAbstractApplication::VApp()->TrVars();

        QVector<VRawGraduatedCurveOffset> toUserOffsets;
        toUserOffsets.reserve(m_offsets.size());
        for (const auto &offset : std::as_const(m_offsets))
        {
            toUserOffsets.append({.name = offset.name, .formula = trVars->FormulaToUser(offset.formula, osSeparator)});
        }
        visual->SetOffsets(toUserOffsets);

        visual->SetLineStyle(LineStyleToPenStyle(curve->GetPenStyle()));
        visual->SetApproximationScale(curve->GetApproximationScale());
        visual->SetMode(Mode::Show);
        visual->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::ReadToolAttributes(const QDomElement &domElement)
{
    VToolAbstractOffsetCurve::ReadToolAttributes(domElement);

    m_offsets = ExtractOffsetData(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::ApplyToolOptions(const QDomElement &oldDomElement, const QDomElement &newDomElement)
{
    ProcessOffsetCurveToolOptions(oldDomElement, newDomElement, GatherToolChanges());
}

//---------------------------------------------------------------------------------------------------------------------
VToolGraduatedCurve::VToolGraduatedCurve(const VToolGraduatedCurveInitData &initData, QGraphicsItem *parent)
  : VToolAbstractOffsetCurve(initData.doc,
                             initData.data,
                             initData.id,
                             initData.originCurveId,
                             initData.name,
                             initData.notes,
                             parent),
    m_offsets(initData.offsets)
{
    SetSceneType(SceneObject::SplinePath);

    this->setFlag(QGraphicsItem::ItemIsFocusable, true); // For keyboard input focus

    ToolCreation(initData.typeCreation);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolGraduatedCurve::UpdateOffsets(QDomElement &tag, const QVector<VRawGraduatedCurveOffset> &offsets)
{
    VAbstractPattern::RemoveAllChildren(tag);
    for (const auto &offset : offsets)
    {
        QDomElement offsetTag = doc->createElement(AttrOffset);

        doc->SetAttribute(offsetTag, AttrName, offset.name);
        doc->SetAttribute(offsetTag, AttrWidth, offset.formula);
        doc->SetAttribute(offsetTag, AttrDescription, offset.description);

        tag.appendChild(offsetTag);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolGraduatedCurve::GatherToolChanges() const -> VToolAbstractOffsetCurve::ToolChanges
{
    SCASSERT(not m_dialog.isNull())
    const QPointer<DialogGraduatedCurve> dialogTool = qobject_cast<DialogGraduatedCurve *>(m_dialog);
    SCASSERT(not dialogTool.isNull())

    return {.oldName = GetName(),
            .newName = dialogTool->GetName(),
            .oldAliasSuffix = GetAliasSuffix(),
            .newAliasSuffix = dialogTool->GetAliasSuffix()};
}
