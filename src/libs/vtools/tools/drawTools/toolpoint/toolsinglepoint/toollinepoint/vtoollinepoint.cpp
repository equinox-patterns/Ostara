/************************************************************************
 **
 **  @file   vtoollinepoint.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
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

#include "vtoollinepoint.h"

#include <QColor>
#include <QGraphicsLineItem>
#include <QLineF>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QSharedPointer>
#include <QUndoStack>

#include "../../../../../undocommands/renameobject.h"
#include "../../../../../undocommands/savetooloptions.h"
#include "../../../../vabstracttool.h"
#include "../../../vdrawtool.h"
#include "../ifc/ifcdef.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../ifc/xml/vdomdocument.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "vscenestylesheet.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtoolsinglepoint.h"
#include "../vwidgets/scalesceneitems.h"
#include "tools/otooltip.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

template <class T> class QSharedPointer;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VToolLinePoint constructor.
 * @param doc dom document container.
 * @param data container with variables.
 * @param id object id in container.
 * @param typeLine line type.
 * @param lineColor line color.
 * @param formula string with length formula.
 * @param basePointId id base line point.
 * @param angle line angle.
 * @param parent parent object.
 */
VToolLinePoint::VToolLinePoint(VAbstractPattern *doc, VContainer *data, const quint32 &id, const QString &typeLine,
                               const QString &lineColor, const QString &formula, const quint32 &basePointId,
                               const qreal &angle, const QString &notes, QGraphicsItem *parent)
  : VToolSinglePoint(doc, data, id, notes, parent),
    formulaLength(formula),
    angle(angle),
    basePointId(basePointId),
    lineColor(lineColor)
{
    this->m_lineType = typeLine;
    Q_ASSERT_X(basePointId != 0, Q_FUNC_INFO, "basePointId == 0"); //-V654 //-V712
    auto const point1 = static_cast<QPointF>(*data->GeometricObject<VPointF>(basePointId));
    auto const point2 = static_cast<QPointF>(*data->GeometricObject<VPointF>(id));
    QLineF const line(point1 - point2, QPointF());
    mainLine = new VScaledLine(line, VColorRole::CustomColor, this);
    mainLine->SetBoldLine(false);
    mainLine->SetShowArrows(true);
    mainLine->setAcceptHoverEvents(true);
    mainLine->setFlag(QGraphicsItem::ItemStacksBehindParent, true);
    mainLine->setZValue(-0.5); // Child line below parent point (Z=0.5 effective vs Z=1)
    mainLine->setVisible(not line.isNull());

    // The line's colour comes from the tool, and only from it: its role is CustomColor, so RefreshPen has no
    // role to recompute a colour from. The tool answers for it — see RefreshAuthoredPen.
    AttachAuthoredColor(mainLine);
    RefreshAuthoredPen(mainLine);

    // The main line of a line point is one of the segments an operation may take, so its click is heard here.
    // A tool that draws more — the dart cap — calls this again once its other tracé exists, and its own list wins.
    InitDrawnSegmentConnections();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    VToolSinglePoint::paint(painter, option, widget);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RefreshGeometry  refresh item on scene.
 */
void VToolLinePoint::RefreshGeometry()
{
    VToolSinglePoint::RefreshGeometry();
    auto const point = static_cast<QPointF>(*VDrawTool::data.GeometricObject<VPointF>(m_id));
    auto const basePoint = static_cast<QPointF>(*VDrawTool::data.GeometricObject<VPointF>(basePointId));
    QLineF const line(basePoint - point, QPointF());
    mainLine->setLine(line);
    RefreshAuthoredPen(mainLine);
    ApplySegmentsVisibility(); // the geometry is set: the layers can now judge the tracé
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::ApplySegmentsVisibility()
{
    const QVector<VDrawnSegment> segments = DrawnSegments();
    const QVector<VScaledLine *> items = DrawnSegmentItems();
    const qsizetype count = qMin(segments.size(), items.size());

    for (qsizetype i = 0; i < count; ++i)
    {
        VScaledLine *item = items.at(i);
        if (item == nullptr)
        {
            continue;
        }

        // A tracé joins two points, and it is shown only when the layers show both of them. An empty tracé is
        // never drawn, whatever the layers say — the same rule the rectangle applies to its four sides.
        item->setVisible(doc->IsObjectVisible(segments.at(i).point1) && doc->IsObjectVisible(segments.at(i).point2)
                         && not item->line().isNull());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::GroupVisibility(quint32 object, bool visible)
{
    VToolSinglePoint::GroupVisibility(object, visible);

    // The tool has just followed its layer; its tracés follow the layers of the points they join, which this tool
    // never hears about otherwise.
    ApplySegmentsVisibility();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::RefreshAuthoredPen(VScaledLine *line)
{
    if (line == nullptr)
    {
        return;
    }

    QPen mPen = line->pen();
    mPen.setColor(VSceneStylesheet::CorrectToolColor(this, VSceneStylesheet::CorrectToolColorForDarkTheme(lineColor)));
    mPen.setStyle(LineStyleToPenStyle(m_lineType));
    line->setPen(mPen);
    // The line keeps the style the tool asked for, so that a "none" line can be shown while it is emphasized
    // (its point hovered or picked) and become invisible again by itself afterwards.
    line->SetAuthoredLineStyle(LineStyleToPenStyle(m_lineType));
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::AttachAuthoredColor(VScaledLine *line)
{
    if (line == nullptr)
    {
        return;
    }

    // The tool answers for the colour of every line it draws. Their role is CustomColor, so there is no role
    // for RefreshPen to recompute a colour from: without this answer, a colour written for a moment — the
    // orange a line wears while its point holds the click, the magenta of a removal pick — had nothing to
    // replace it once that state was gone, and stayed in the pen. The tool is the item asked for the
    // disabled colour too, so that every line of a disabled tool greys out together.
    line->SetAuthoredColorProvider([this]() -> QColor
                                   {
                                       return VSceneStylesheet::CorrectToolColor(
                                           this, VSceneStylesheet::CorrectToolColorForDarkTheme(lineColor));
                                   });
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::SaveOptions(QDomElement &tag, QSharedPointer<VGObject> &obj)
{
    VToolSinglePoint::SaveOptions(tag, obj);

    doc->SetAttribute(tag, AttrTypeLine, m_lineType);
    doc->SetAttribute(tag, AttrLineColor, lineColor);

    // Persist line role name to <lineNames> DOM section (equivalent to AttrAlias for curves)
    SyncLineRoleNameToDom();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::SetActivePoint(bool active)
{
    // The point this line belongs to is the one the user clicked: it is the only moment where the line it comes
    // from can be wanted on screen, and it is what the point's own colour already answers to.
    VToolSinglePoint::SetActivePoint(active);
    UpdateLinesHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::UpdateLinesHighlight()
{
    // The click is the only state that touches the lines: a hover must leave them exactly as they are, or the line
    // a point sits on would go bold every time the cursor passes over that point — right when the point is about to
    // be picked. The tool deliberately overrides no hover event at all: the point's own hover (magenta, tooltip) is
    // VScenePoint's business, and it does not reach the tracés.
    //
    // The lines are shown while the point holds the click: that is the only way to see which base point they come
    // from, since a "none" line is invisible the rest of the time. The click wears the colour of the active point,
    // the one the point already wears then, so that line and point read as one object. Never done from paint():
    // Qt 6 does not support mutating item state while painting.
    //
    // Every line the tool draws is written in the same pass, and its colour is rewritten from the tool before
    // the emphasis: a cap is one object, and one of its two tracés may not light up alone — nor keep a colour
    // that only the emphasis was asking for.
    const bool clicked = not m_suppressContextMenu && m_selected;

    for (VScaledLine *line : DrawnSegmentItems())
    {
        if (line == nullptr)
        {
            continue;
        }

        line->SetColorRole(clicked ? VColorRole::ActivePointColor : VColorRole::CustomColor);
        RefreshAuthoredPen(line); // gives the line back its own colour and style
        line->SetHighlighted(clicked);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLinePoint::MakeToolTip() const -> QString
{
    const QSharedPointer<VPointF> first = VAbstractTool::data.GeometricObject<VPointF>(basePointId);
    const QSharedPointer<VPointF> second = VAbstractTool::data.GeometricObject<VPointF>(m_id);

    const QLineF line(static_cast<QPointF>(*first), static_cast<QPointF>(*second));

    // The point is named, never: the tooltip reads its distance and its direction from the base point, and
    // nothing else (see OToolTip).
    return OToolTip::Table({
        OToolTip::Row(tr("Length"), OToolTip::Measure(line.length())),
        OToolTip::Row(tr("Angle"), OToolTip::Degrees(line.angle())),
    });
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::Enable()
{
    VToolSinglePoint::Enable();

    // Every line the tool draws follows its enabled state, not only the inherited one: a tool that declares more
    // than one segment (the cut line of VToolAlongLine) would otherwise leave the others enabled, and with the
    // pen of the state before.
    for (VScaledLine *line : DrawnSegmentItems())
    {
        if (line == nullptr)
        {
            continue;
        }

        line->setEnabled(isEnabled());
        RefreshAuthoredPen(line);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLinePoint::GetAngle() const -> qreal
{
    return angle;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::SetAngle(const qreal &value)
{
    angle = value;
    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLinePoint::GetLineColor() const -> QString
{
    return lineColor;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::SetLineColor(const QString &value)
{
    lineColor = value;

    QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLinePoint::GetFormulaLength() const -> VFormula
{
    VFormula fLength(formulaLength, this->getData());
    fLength.setToolId(m_id);
    fLength.setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits()));
    fLength.Eval();

    return fLength;
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::SetFormulaLength(const VFormula &value)
{
    if (!value.error())
    {
        formulaLength = value.GetFormula(FormulaType::FromUser);

        QSharedPointer<VGObject> obj = VAbstractTool::data.GetGObject(m_id);
        SaveOption(obj);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLinePoint::BasePointName() const -> QString
{
    return VAbstractTool::data.GetGObject(basePointId)->name();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::ProcessLinePointToolOptions(const QDomElement &oldDomElement,
                                                 const QDomElement &newDomElement,
                                                 const QString &newLabel,
                                                 const QString &newBasePointLabel)
{
    const QString oldLabel = VAbstractTool::data.GetGObject(m_id)->name();
    const QString oldBasePointLabel = BasePointName();

    if (oldBasePointLabel == newBasePointLabel && oldLabel == newLabel)
    {
        VToolSinglePoint::ApplyToolOptions(oldDomElement, newDomElement);
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
        if (oldLabel == newLabel)
        {
            connect(renamePair, &RenamePair::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
        }
    }

    if (oldLabel != newLabel)
    {
        const auto *renameLabel = new RenameLabel(oldLabel, newLabel, doc, m_id, newGroup);
        connect(renameLabel, &RenameLabel::NeedLiteParsing, doc, &VAbstractPattern::LiteParseTree);
    }

    undoStack->push(newGroup);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::SyncLineRoleNameToDom()
{
    SyncLineRoleNameToDom(BasePointName() + '_' + VAbstractTool::data.GetGObject(m_id)->name());
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::SyncLineRoleNameToDom(const QString &pairKey)
{
    const QString suffix = VAbstractTool::data.LineRoleName(pairKey);
    if (suffix.isEmpty())
    {
        return;
    }

    QDomElement root = doc->documentElement();
    if (root.isNull())
    {
        return;
    }

    QDomElement lineNamesElement = root.firstChildElement(VAbstractPattern::TagLineNames);
    if (lineNamesElement.isNull())
    {
        lineNamesElement = doc->createElement(VAbstractPattern::TagLineNames);
        const QDomElement drawElement = root.firstChildElement(VAbstractPattern::TagDraw);
        if (!drawElement.isNull())
        {
            root.insertBefore(lineNamesElement, drawElement);
        }
        else
        {
            root.appendChild(lineNamesElement);
        }
    }

    const QStringList pointNames = pairKey.split('_', Qt::SkipEmptyParts);
    if (pointNames.size() != 2)
    {
        return;
    }

    const QString points = pointNames.at(0) + ' ' + pointNames.at(1);
    QDomElement existingLine = lineNamesElement.firstChildElement(VDomDocument::TagLine);
    bool found = false;
    while (!existingLine.isNull())
    {
        if (existingLine.attribute(AttrPoints) == points)
        {
            existingLine.setAttribute(AttrName, suffix);
            found = true;
            break;
        }
        existingLine = existingLine.nextSiblingElement(VDomDocument::TagLine);
    }

    if (!found)
    {
        QDomElement lineEl = doc->createElement(VDomDocument::TagLine);
        lineEl.setAttribute(AttrPoints, points);
        lineEl.setAttribute(AttrName, suffix);
        lineNamesElement.appendChild(lineEl);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLinePoint::EffectiveAngleFormula() const -> QString
{
    if (m_axis.IsValid())
    {
        return QString::number(AxisAngle(m_axis, getData()));
    }

    return AngleFormulaString();
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLinePoint::AngleFormulaString() const -> QString
{
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::SetAngleFormulaString(const QString &formula)
{
    Q_UNUSED(formula)
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLinePoint::AxisSlot(int slot) const -> OAxis
{
    Q_UNUSED(slot)
    return m_axis;
}

//---------------------------------------------------------------------------------------------------------------------
auto VToolLinePoint::PointForAxisAngle(qreal angle) const -> std::optional<QPointF>
{
    Q_UNUSED(angle)
    return std::nullopt;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Shows the point the direction would give it, without writing anything.
 *
 *  The point is placed where the tool's own geometry puts it for that angle — along its length for a line end,
 *  on the line or the curve it crosses for an intersection — and the container is updated in place, exactly as
 *  the free curve preview does. Only what the preview started from is kept, so RevertAxis() can put back the
 *  very state the gesture found. */
void VToolLinePoint::PreviewAxis(int slot, const OAxis &axis)
{
    Q_UNUSED(slot)

    if (not m_axisPreviewing)
    {
        const QSharedPointer<VPointF> point = VDrawTool::data.GeometricObject<VPointF>(m_id);
        if (point.isNull())
        {
            return;
        }

        m_axisPreviewStart = QSharedPointer<VPointF>::create(*point);
        m_axisFormulaStart = AngleFormulaString();
        m_axisAngleStart = angle;
        m_axisStart = m_axis;
        m_axisPreviewing = true;
    }

    const qreal newAngle = AxisAngle(axis, getData());
    const std::optional<QPointF> position = PointForAxisAngle(newAngle);
    if (not position.has_value())
    {
        // No intersection in that direction: nothing to show, and the point stays where it stands rather than
        // jumping to a place the geometry does not hold.
        return;
    }

    m_axis = axis;
    angle = newAngle;
    SetAngleFormulaString(QString::number(newAngle));

    const QSharedPointer<VPointF> point = VDrawTool::data.GeometricObject<VPointF>(m_id);
    point->setX(position->x());
    point->setY(position->y());

    RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::CommitAxis()
{
    if (not m_axisPreviewing)
    {
        return;
    }

    m_axisPreviewing = false;
    m_axisPreviewStart.clear();

    // The file never heard of the preview, so this one command tells the whole story: its old element is the
    // point as it stood before, its new one what is now shown.
    QSharedPointer<VGObject> obj = VDrawTool::data.GetGObject(m_id);
    SaveOption(obj);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::RevertAxis()
{
    if (not m_axisPreviewing)
    {
        return;
    }

    m_axisPreviewing = false;

    if (not m_axisPreviewStart.isNull())
    {
        const QSharedPointer<VPointF> point = VDrawTool::data.GeometricObject<VPointF>(m_id);
        *point.data() = *m_axisPreviewStart;
    }
    m_axisPreviewStart.clear();

    m_axis = m_axisStart;
    angle = m_axisAngleStart;
    SetAngleFormulaString(m_axisFormulaStart);

    RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::ReadToolAttributes(const QDomElement &domElement)
{
    VToolSinglePoint::ReadToolAttributes(domElement);

    m_axis = ReadAxis(domElement, AttrAxisP1, AttrAxisP2, AttrAxisSense);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::SaveAxisInto(QDomElement &element)
{
    SaveAxis(doc, element, m_axis, AttrAxisP1, AttrAxisP2, AttrAxisSense);
}

//---------------------------------------------------------------------------------------------------------------------
void VToolLinePoint::RefreshAxisFromFile()
{
    ReadAttributes();
}
