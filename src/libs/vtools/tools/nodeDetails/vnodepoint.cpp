/************************************************************************
 **
 **  @file   vnodepoint.cpp
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

#include "vnodepoint.h"

#include <QBrush>
#include <QDomElement>
#include <QFlags>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QIcon>
#include <QMenu>
#include <QPen>
#include <QPoint>
#include <QRectF>
#include <QSharedPointer>
#include <QVariant>

#include <memory>

#include "../ifc/ifcdef.h"
#include "../ifc/xml/vdomdocument.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../ifc/xml/vpatterngraph.h"
#include "../vabstracttool.h"
#include "../vdatatool.h"
#include "../vgeometry/vpointf.h"
#include "themeDef.h"
#include "../vtheme/vtheme.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vtoolseamallowance.h"
#include "../vwidgets/global.h"
#include "../vwidgets/scalesceneitems.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "vabstractnode.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

const QString VNodePoint::ToolType = QStringLiteral("modeling");

namespace
{
enum class ContextMenuOption : std::uint8_t
{
    NoSelection,
    ShowLabel,
    NonePassmark,
    Exclude,
    ByLength,
    ByPointsIntersection,
    ByFirstEdgeSymmetry,
    BySecondEdgeSymmetry,
    ByFirstEdgeRightAngle,
    BySecondEdgeRightAngle,
    Straightforward,
    Bisector,
    Intersection,
    IntersectionOnlyLeft,
    IntersectionOnlyRight,
    Intersection2,
    Intersection2OnlyLeft,
    Intersection2OnlyRight,
    OneLine,
    TwoLines,
    ThreeLines,
    TMark,
    ExternalVMark,
    InternalVMark,
    UMark,
    BoxMark,
    CheckMark,
    CheckMarkClockwise,
    Option,
    InLayout,
    ForceFlipping,
    ShowFullPiece,
    Remove,
    TurnPoint,
    LAST_ONE_DO_NOT_USE
};

// Theme-aware glyphs for the node context menu (see Icons.md): `check` marks an active toggle,
// `trash` the delete action. A highlighted menu row has a dark accent background in both themes,
// so under the cursor the glyph is swapped to the dark (light-colored) variant.
QString NodeIconTheme()
{
    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;
    return dark ? QStringLiteral("dark") : QStringLiteral("light");
}

QIcon NodeThemeIcon(const QString &name)
{
    return QIcon(QStringLiteral(":/icons/%1/%2_%1.svg").arg(NodeIconTheme(), name));
}

QIcon NodeHoverIcon(const QString &name)
{
    return QIcon(QStringLiteral(":/icons/dark/%1_dark.svg").arg(name));
}

QAction *AddNodeToggle(QMenu *menu, const QString &text, bool active)
{
    QAction *action = menu->addAction(text);
    action->setProperty("active", active);
    if (active)
    {
        const QIcon check = NodeThemeIcon(QStringLiteral("check"));
        action->setIcon(check);
        action->setProperty("restIcon", QVariant::fromValue(check));
        action->setProperty("hoverIcon", QVariant::fromValue(NodeHoverIcon(QStringLiteral("check"))));
    }
    return action;
}

void AddNodeHoverIcon(QAction *action, const QString &name)
{
    const QIcon icon = NodeThemeIcon(name);
    action->setIcon(icon);
    action->setProperty("restIcon", QVariant::fromValue(icon));
    action->setProperty("hoverIcon", QVariant::fromValue(NodeHoverIcon(name)));
}

struct GlyphHoverState
{
    QAction *lastHovered{nullptr};
};

void ApplyGlyphHover(const std::shared_ptr<GlyphHoverState> &state, QAction *action)
{
    if (action == state->lastHovered)
    {
        return;
    }
    if (state->lastHovered != nullptr)
    {
        const QVariant rest = state->lastHovered->property("restIcon");
        if (rest.isValid())
        {
            state->lastHovered->setIcon(rest.value<QIcon>());
        }
    }
    state->lastHovered = action;
    if (action != nullptr)
    {
        const QVariant hover = action->property("hoverIcon");
        if (hover.isValid())
        {
            action->setIcon(hover.value<QIcon>());
        }
    }
}

void InstallGlyphHover(QMenu *menu, QObject *context)
{
    auto state = std::make_shared<GlyphHoverState>();
    QObject::connect(menu, &QMenu::hovered, context, [state](QAction *action) { ApplyGlyphHover(state, action); });
    QObject::connect(menu, &QMenu::aboutToHide, context, [state]() { ApplyGlyphHover(state, nullptr); });
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief VNodePoint constructor.
 * @param initData init data.
 * @param parent parent object.
 */
VNodePoint::VNodePoint(const VAbstractNodeInitData &initData, QObject *qoParent, QGraphicsItem *parent)
  : VAbstractNode(initData.doc, initData.data, initData.id, initData.idObject, initData.drawName, initData.idTool,
                  qoParent),
    VScenePoint(VColorRole::PiecePointColor, parent)
{
    // Details-mode piece points are outline-only at rest; hover/active keep their fill.
    // Draw-mode points (VSimplePoint) are untouched (m_fillPoints stays true).
    SetPointFillEnabled(false);

    m_namePoint->SetTextColor(VColorRole::PieceNodeLabelColor);
    m_namePoint->SetTextHoverColor(VColorRole::PieceNodeLabelHoverColor);
    m_namePoint->SetShowParentTooltip(false);
    connect(m_namePoint, &VGraphicsSimpleTextItem::PointChoosed, this, &VNodePoint::PointChoosed);
    connect(m_namePoint, &VGraphicsSimpleTextItem::NameChangePosition, this, &VNodePoint::NameChangePosition);
    connect(m_namePoint,
            &VGraphicsSimpleTextItem::ShowContextMenu,
            this,
            [this](QGraphicsSceneContextMenuEvent *event) -> void { contextMenuEvent(event); });
    // Ugly, but there is no other way
    connect(
        m_namePoint,
        &VGraphicsSimpleTextItem::UpdateLine,
        this,
        [this]() -> void { RefreshLine(); },
        Qt::QueuedConnection);
    connect(m_namePoint, &VGraphicsSimpleTextItem::LabelHovered, this,
            [this](bool hovered)
            {
                m_isHovered = hovered;
                RefreshHoverPen();
            });
    m_lineName->SetColorRole(VColorRole::PieceNodeLabelLineColor);
    RefreshPointGeometry(*VAbstractTool::data.GeometricObject<VPointF>(initData.id));
    ToolCreation(initData.typeCreation);
    setCursor(Qt::ArrowCursor);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Create help create tool.
 * @param initData init data.
 */
void VNodePoint::Create(const VAbstractNodeInitData &initData)
{
    VPatternGraph *patternGraph = initData.doc->PatternGraph();
    SCASSERT(patternGraph != nullptr)

    patternGraph->AddVertex(initData.id, VNodeType::MODELING_OBJECT, initData.doc->PatternBlockMapper()->GetActiveId());

    QSharedPointer<VPointF> point;
    try
    {
        point = initData.data->GeometricObject<VPointF>(initData.idObject);
    }
    catch (const VExceptionBadId &e)
    { // Possible case. Parent was deleted, but the node object is still here.
        qDebug() << "Broken relation. Parent was deleted, but the node object is still here. Node point id ="
                 << initData.id << "." << e.ErrorMessage();
        return; // Just ignore
    }

    patternGraph->AddEdge(initData.idObject, initData.id);

    QSharedPointer<VPointF> const p(new VPointF(*point));
    p->setIdObject(initData.idObject);
    p->setMode(Draw::Modeling);
    p->SetShowLabel(initData.showLabel);
    p->SetShowLabelExplicit(initData.showLabelExplicit);
    p->setMx(initData.mx);
    p->setMy(initData.my);

    initData.data->UpdateGObject(initData.id, p);

    if (initData.parse == Document::FullParse)
    {
        VAbstractTool::AddRecord(initData.id, Tool::NodePoint, initData.doc);
        // TODO Need create garbage collector and remove all nodes, that we don't use.
        // Better check garbage before each saving file. Check only modeling tags.
        auto *point = new VNodePoint(initData);

        connect(initData.scene, &VMainGraphicsScene::EnableToolMove, point, &VNodePoint::EnableToolMove);
        connect(initData.scene, &VMainGraphicsScene::EnablePointItemHover, point, &VNodePoint::AllowHover);
        connect(initData.scene, &VMainGraphicsScene::EnablePointItemSelection, point, &VNodePoint::AllowSelecting);
        connect(initData.scene, &VMainGraphicsScene::EnableLabelItemHover, point, &VNodePoint::AllowLabelHover);
        connect(initData.scene, &VMainGraphicsScene::EnableLabelItemSelection, point, &VNodePoint::AllowLabelSelecting);
        VAbstractPattern::AddTool(initData.id, point);
        if (initData.idTool != NULL_ID)
        {
            // Some nodes we don't show on scene. Tool that create this nodes must free memory.
            VDataTool *tool = VAbstractPattern::getTool(initData.idTool);
            SCASSERT(tool != nullptr)
            point->setParent(tool); // Adopted by a tool
        }
        else
        {
            // Try to prevent memory leak
            initData.scene->addItem(point); // First adopted by scene
            point->hide();                  // If no one will use node, it will stay hidden
            point->SetParentType(ParentType::Scene);
        }
    }
    else
    {
        initData.doc->UpdateToolData(initData.id, initData.data);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VNodePoint::getTagName() const -> QString
{
    return VAbstractPattern::TagPoint;
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::ChangeLabelPosition(quint32 id, const QPointF &pos)
{
    if (id == m_id)
    {
        QSharedPointer<VPointF> const point = VAbstractTool::data.GeometricObject<VPointF>(id);
        point->setMx(pos.x());
        point->setMy(pos.y());
        m_namePoint->SetRealPos(pos);
        RefreshLine();
        QDomElement element = doc->FindElementById(m_id, getTagName());
        if (element.isElement())
        {
            doc->SetAttribute(element, AttrMx, VAbstractValApplication::VApp()->fromPixel(pos.x()));
            doc->SetAttribute(element, AttrMy, VAbstractValApplication::VApp()->fromPixel(pos.y()));
        }
        if (QGraphicsScene *sc = scene())
        {
            VMainGraphicsView::NewSceneRect(sc, VAbstractValApplication::VApp()->getSceneView(), this);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::SetLabelVisible(quint32 id, bool visible)
{
    if (m_id == id)
    {
        const QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(id);
        point->SetShowLabel(visible);
        point->SetShowLabelExplicit(true);

        if (m_internalPathNode && IsOnlyPoint())
        {
            SetOnlyPoint(false);
        }

        RefreshPointGeometry(*point);
        QDomElement element = doc->FindElementById(m_id, getTagName());
        if (element.isElement())
        {
            doc->SetAttributeOrRemoveIf<bool>(element,
                                              AttrShowLabel,
                                              visible,
                                              [](bool show) noexcept -> bool { return show; });
        }
        if (QGraphicsScene *sc = scene())
        {
            VMainGraphicsView::NewSceneRect(sc, VAbstractValApplication::VApp()->getSceneView(), this);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::PointChoosed()
{
    emit ChoosedTool(m_id, SceneObject::Point);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief FullUpdateFromFile update tool data form file.
 */
void VNodePoint::FullUpdateFromFile()
{
    try
    {
        RefreshPointGeometry(*VAbstractTool::data.GeometricObject<VPointF>(m_id));
    }
    catch (const VExceptionBadId &)
    {
        // ignore
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AddToFile add tag with informations about tool into file.
 */
void VNodePoint::AddToFile()
{
    const QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(m_id);
    QDomElement domElement = doc->createElement(getTagName());

    doc->SetAttribute(domElement, VDomDocument::AttrId, m_id);
    doc->SetAttribute(domElement, AttrType, ToolType);
    doc->SetAttribute(domElement, AttrIdObject, idNode);
    doc->SetAttribute(domElement, AttrMx, VAbstractValApplication::VApp()->fromPixel(point->mx()));
    doc->SetAttribute(domElement, AttrMy, VAbstractValApplication::VApp()->fromPixel(point->my()));
    doc->SetAttributeOrRemoveIf<bool>(domElement,
                                      AttrShowLabel,
                                      point->IsShowLabel(),
                                      [](bool show) noexcept -> bool { return show; });
    if (idTool != NULL_ID)
    {
        doc->SetAttribute(domElement, AttrIdTool, idTool);
    }

    AddToModeling(domElement);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // Special for not selectable item first need to call standard mousePressEvent then accept event
    VScenePoint::mousePressEvent(event);

    // Somehow clicking on notselectable object do not clean previous selections.
    if (not(flags() & ItemIsSelectable) && scene())
    {
        scene()->clearSelection();
    }

    event->accept(); // Special for not selectable item first need to call standard mousePressEvent then accept event
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mouseReleaseEvent handle mouse release events.
 * @param event mouse release event.
 */
void VNodePoint::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (IsSelectedByReleaseEvent(this, event))
    {
        emit ChoosedTool(m_id, SceneObject::Point);
    }
    VScenePoint::mouseReleaseEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief NameChangePosition label change position.
 * @param pos new position.
 */
void VNodePoint::NameChangePosition(const QPointF &pos)
{
    ChangeLabelPosition(m_id, pos - this->pos());
}

//---------------------------------------------------------------------------------------------------------------------
auto VNodePoint::InitContextMenu(QMenu *menu, vidtype pieceId, RemoveStatus status) -> QHash<int, QAction *>
{
    SCASSERT(menu != nullptr)

    QHash<int, QAction *> contextMenu;

    bool showLabel = false;
    if (const QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(m_id); !m_internalPathNode)
    {
        showLabel = point->IsShowLabel();
    }
    else
    {
        showLabel = point->IsShowLabelExplicit() ? point->IsShowLabel() : false;
    }
    QAction *actionShowLabel =
        AddNodeToggle(menu, QCoreApplication::translate("VNodePoint", "Show Point name"), showLabel);
    contextMenu.insert(static_cast<int>(ContextMenuOption::ShowLabel), actionShowLabel);

    InitPassmarkMenu(menu, pieceId, contextMenu);

    contextMenu.insert(static_cast<int>(ContextMenuOption::Exclude),
                       menu->addAction(QCoreApplication::translate("VNodePoint", "Exclude")));

    InitAngleTypeMenu(menu, pieceId, contextMenu);
    InitPassmarkAngleTypeMenu(menu, pieceId, contextMenu);

    auto *separatorAct = new QAction(this);
    separatorAct->setSeparator(true);
    menu->addAction(separatorAct);

    // "Options" (which opened DialogSeamAllowance) is no longer offered in the node menu.

    const VPiece detail = VAbstractTool::data.GetPiece(pieceId);

    QAction *inLayoutOption =
        AddNodeToggle(menu, QCoreApplication::translate("VNodePoint", "In layout"), detail.IsInLayout());
    contextMenu.insert(static_cast<int>(ContextMenuOption::InLayout), inLayoutOption);

    QAction *forceFlippingOption = AddNodeToggle(menu, QCoreApplication::translate("VNodePoint", "Force flipping"),
                                                 detail.IsForceFlipping());
    contextMenu.insert(static_cast<int>(ContextMenuOption::ForceFlipping), forceFlippingOption);

    QAction *showFullPieceOption = nullptr;
    {
        // We cannot use current VContainer because it doesn't have current seam allowance value
        const VContainer pData = VAbstractPattern::getTool(pieceId)->GetDataCopy();
        const QLineF mirrorLine = detail.SeamAllowanceMirrorLine(&pData);
        const bool enabled = not mirrorLine.isNull();
        showFullPieceOption = AddNodeToggle(menu, QCoreApplication::translate("VNodePoint", "Show full piece"),
                                            enabled && detail.IsShowFullPiece());
        showFullPieceOption->setEnabled(enabled);
    }
    contextMenu.insert(static_cast<int>(ContextMenuOption::ShowFullPiece), showFullPieceOption);

    QAction *actionRemove = menu->addAction(QCoreApplication::translate("VNodePoint", "Delete"));
    AddNodeHoverIcon(actionRemove, QStringLiteral("trash"));
    actionRemove->setEnabled(status == RemoveStatus::Removable);
    if (status == RemoveStatus::Pending)
    {
        actionRemove->setText(actionRemove->text() + " ("_L1 + QCoreApplication::translate("VNodePoint", "Pending")
                              + ')');
    }
    contextMenu.insert(static_cast<int>(ContextMenuOption::Remove), actionRemove);

    return contextMenu;
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::InitPassmarkMenu(QMenu *menu, vidtype pieceId, QHash<int, QAction *> &contextMenu)
{
    SCASSERT(menu != nullptr)

    const VPiece detail = VAbstractTool::data.GetPiece(pieceId);
    const VPiecePath &path = detail.GetPath();
    const int nodeIndex = path.indexOfNode(m_id);
    if (nodeIndex == -1)
    {
        return;
    }

    const VPieceNode &node = path.at(nodeIndex);

    QMenu *passmarkSubmenu = menu->addMenu(tr("Passmark"));

    QAction *actionNonePassmark = AddNodeToggle(passmarkSubmenu, tr("None"), !node.IsPassmark());
    contextMenu.insert(static_cast<int>(ContextMenuOption::NonePassmark), actionNonePassmark);

    Q_STATIC_ASSERT_X(static_cast<int>(PassmarkLineType::LAST_ONE_DO_NOT_USE) == 9, "Not all types were handled.");

    auto InitPassmarkLineTypeAction = [passmarkSubmenu, node](const QString &name, PassmarkLineType lineType,
                                                              bool clockwise = false) -> QAction *
    {
        // The two "Check mark" entries share their line type and are told apart by the opening, so the
        // mark is active only when both match. The flag means nothing for the other types, which are
        // therefore marked on the type alone.
        const bool active = node.IsPassmark() && lineType == node.GetPassmarkLineType() &&
                            (lineType != PassmarkLineType::CheckMark ||
                             clockwise == node.IsPassmarkClockwiseOpening());
        return AddNodeToggle(passmarkSubmenu, name, active);
    };

    contextMenu.insert(static_cast<int>(ContextMenuOption::OneLine),
                       InitPassmarkLineTypeAction(tr("One line"), PassmarkLineType::OneLine));
    contextMenu.insert(static_cast<int>(ContextMenuOption::TwoLines),
                       InitPassmarkLineTypeAction(tr("Two lines"), PassmarkLineType::TwoLines));
    contextMenu.insert(static_cast<int>(ContextMenuOption::ThreeLines),
                       InitPassmarkLineTypeAction(tr("Three lines"), PassmarkLineType::ThreeLines));
    contextMenu.insert(static_cast<int>(ContextMenuOption::TMark),
                       InitPassmarkLineTypeAction(tr("T mark"), PassmarkLineType::TMark));
    contextMenu.insert(static_cast<int>(ContextMenuOption::ExternalVMark),
                       InitPassmarkLineTypeAction(tr("External V mark"), PassmarkLineType::ExternalVMark));
    contextMenu.insert(static_cast<int>(ContextMenuOption::InternalVMark),
                       InitPassmarkLineTypeAction(tr("Internal V mark"), PassmarkLineType::InternalVMark));
    contextMenu.insert(static_cast<int>(ContextMenuOption::UMark),
                       InitPassmarkLineTypeAction(tr("U mark"), PassmarkLineType::UMark));
    contextMenu.insert(static_cast<int>(ContextMenuOption::BoxMark),
                       InitPassmarkLineTypeAction(tr("Box mark"), PassmarkLineType::BoxMark));
    contextMenu.insert(static_cast<int>(ContextMenuOption::CheckMark),
                       InitPassmarkLineTypeAction(tr("Check mark"), PassmarkLineType::CheckMark));
    contextMenu.insert(static_cast<int>(ContextMenuOption::CheckMarkClockwise),
                       InitPassmarkLineTypeAction(tr("Check mark clockwise"), PassmarkLineType::CheckMark, true));

    QAction *actionTurnPoint = AddNodeToggle(menu, tr("Turn point (for DXF)"), node.IsTurnPoint());
    contextMenu.insert(static_cast<int>(ContextMenuOption::TurnPoint), actionTurnPoint);

    InstallGlyphHover(passmarkSubmenu, this);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::InitAngleTypeMenu(QMenu *menu, vidtype pieceId, QHash<int, QAction *> &contextMenu)
{
    const VPiece detail = VAbstractTool::data.GetPiece(pieceId);
    const VPiecePath &path = detail.GetPath();
    const int nodeIndex = path.indexOfNode(m_id);
    if (nodeIndex == -1)
    {
        return;
    }

    const PieceNodeAngle curType = path.at(nodeIndex).GetAngleType();
    QMenu *angleTypeMenu = menu->addMenu(tr("Seam allowance angle"));
    angleTypeMenu->setEnabled(detail.IsSeamAllowance());

    auto InitAngleAction = [angleTypeMenu, curType](const QString &name, PieceNodeAngle checkType) -> QAction *
    {
        return AddNodeToggle(angleTypeMenu, name, curType == checkType);
    };

    Q_STATIC_ASSERT_X(static_cast<int>(PieceNodeAngle::LAST_ONE_DO_NOT_USE) == 7, "Not all types were handled.");
    contextMenu.insert(static_cast<int>(ContextMenuOption::ByLength),
                       InitAngleAction(tr("by length"), PieceNodeAngle::ByLength));
    contextMenu.insert(static_cast<int>(ContextMenuOption::ByPointsIntersection),
                       InitAngleAction(tr("by points intersetions"), PieceNodeAngle::ByPointsIntersection));
    contextMenu.insert(static_cast<int>(ContextMenuOption::ByFirstEdgeSymmetry),
                       InitAngleAction(tr("by first edge symmetry"), PieceNodeAngle::ByFirstEdgeSymmetry));
    contextMenu.insert(static_cast<int>(ContextMenuOption::BySecondEdgeSymmetry),
                       InitAngleAction(tr("by second edge symmetry"), PieceNodeAngle::BySecondEdgeSymmetry));
    contextMenu.insert(static_cast<int>(ContextMenuOption::ByFirstEdgeRightAngle),
                       InitAngleAction(tr("by first edge right angle"), PieceNodeAngle::ByFirstEdgeRightAngle));
    contextMenu.insert(static_cast<int>(ContextMenuOption::BySecondEdgeRightAngle),
                       InitAngleAction(tr("by second edge right angle"), PieceNodeAngle::BySecondEdgeRightAngle));

    InstallGlyphHover(angleTypeMenu, this);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::InitPassmarkAngleTypeMenu(QMenu *menu, vidtype pieceId, QHash<int, QAction *> &contextMenu)
{
    const VPiece detail = VAbstractTool::data.GetPiece(pieceId);
    const VPiecePath &path = detail.GetPath();
    const int nodeIndex = path.indexOfNode(m_id);
    if (nodeIndex == -1)
    {
        return;
    }

    const VPieceNode &node = path.at(nodeIndex);

    QMenu *passmarkAngleMenu = menu->addMenu(tr("Passmark angle"));

    auto InitPassmarkAngleAction = [passmarkAngleMenu, node](const QString &name, PassmarkAngleType checkType)
    {
        QAction *action =
            AddNodeToggle(passmarkAngleMenu, name, node.IsPassmark() && node.GetPassmarkAngleType() == checkType);
        action->setEnabled(node.IsPassmark());
        return action;
    };

    Q_STATIC_ASSERT_X(static_cast<int>(PassmarkAngleType::LAST_ONE_DO_NOT_USE) == 8, "Not all types were handled.");
    contextMenu.insert(static_cast<int>(ContextMenuOption::Straightforward),
                       InitPassmarkAngleAction(tr("Straightforward"), PassmarkAngleType::Straightforward));
    contextMenu.insert(static_cast<int>(ContextMenuOption::Bisector),
                       InitPassmarkAngleAction(tr("Bisector"), PassmarkAngleType::Bisector));
    contextMenu.insert(static_cast<int>(ContextMenuOption::Intersection),
                       InitPassmarkAngleAction(tr("Intersection"), PassmarkAngleType::Intersection));
    contextMenu.insert(
        static_cast<int>(ContextMenuOption::IntersectionOnlyLeft),
        InitPassmarkAngleAction(tr("Intersection (only left)"), PassmarkAngleType::IntersectionOnlyLeft));
    contextMenu.insert(
        static_cast<int>(ContextMenuOption::IntersectionOnlyRight),
        InitPassmarkAngleAction(tr("Intersection (only right)"), PassmarkAngleType::IntersectionOnlyRight));
    contextMenu.insert(static_cast<int>(ContextMenuOption::Intersection2),
                       InitPassmarkAngleAction(tr("Intersection 2"), PassmarkAngleType::Intersection2));
    contextMenu.insert(
        static_cast<int>(ContextMenuOption::Intersection2OnlyLeft),
        InitPassmarkAngleAction(tr("Intersection 2 (only left)"), PassmarkAngleType::Intersection2OnlyLeft));
    contextMenu.insert(
        static_cast<int>(ContextMenuOption::Intersection2OnlyRight),
        InitPassmarkAngleAction(tr("Intersection 2 (only right)"), PassmarkAngleType::Intersection2OnlyRight));

    InstallGlyphHover(passmarkAngleMenu, this);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    if (m_suppressContextMenu)
    {
        return;
    }

    auto *piece = qgraphicsitem_cast<VToolSeamAllowance *>(parentItem());
    if (piece == nullptr)
    {
        return;
    }

    QMenu menu;
    QHash<int, QAction *> const contextMenu = InitContextMenu(&menu, piece->getId(), piece->IsRemovable());

    PieceNodeAngle angleCurType = PieceNodeAngle::ByLength;
    PassmarkAngleType passmarkAngleCurType = PassmarkAngleType::Straightforward;
    PassmarkLineType passmarkLineCurType = PassmarkLineType::OneLine;
    bool passmarkLineCurClockwise = false;

    const VPiece detail = VAbstractTool::data.GetPiece(piece->getId());
    const VPiecePath &path = detail.GetPath();
    const int nodeIndex = path.indexOfNode(m_id);
    if (nodeIndex != -1)
    {
        const VPieceNode &node = path.at(nodeIndex);
        angleCurType = node.GetAngleType();
        passmarkAngleCurType = node.GetPassmarkAngleType();
        passmarkLineCurType = node.GetPassmarkLineType();
        passmarkLineCurClockwise = node.IsPassmarkClockwiseOpening();
    }

    auto SelectSeamAllowanceAngle = [angleCurType, this](PieceNodeAngle type) -> void
    {
        if (angleCurType != type)
        {
            emit ToggleSeamAllowanceAngleType(m_id, type);
        }
    };

    auto SelectPassmarkAngle = [passmarkAngleCurType, this](PassmarkAngleType type) -> void
    {
        if (passmarkAngleCurType != type)
        {
            emit TogglePassmarkAngleType(m_id, type);
        }
    };

    auto SelectPassmarkLine = [passmarkLineCurType, passmarkLineCurClockwise, this](PassmarkLineType type,
                                                                                    bool clockwise) -> void
    {
        emit TogglePassmark(m_id, true);

        // The opening is part of the choice: the two "Check mark" entries share their line type, so
        // picking the other one is a real change even though the type does not move.
        if (passmarkLineCurType != type || passmarkLineCurClockwise != clockwise)
        {
            emit TogglePassmarkLineType(m_id, type, clockwise);
        }
    };

    // Swap the glyph to its dark variant while a row is highlighted (a highlighted row has a
    // dark accent background in both themes); restore when the highlight moves away.
    InstallGlyphHover(&menu, this);

    QAction *selectedAction = menu.exec(event->screenPos());
    auto const selectedOption = static_cast<ContextMenuOption>(
        contextMenu.key(selectedAction, static_cast<int>(ContextMenuOption::NoSelection)));

    Q_STATIC_ASSERT_X(static_cast<int>(ContextMenuOption::LAST_ONE_DO_NOT_USE) == 34, "Not all options were handled.");

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (selectedOption)
    {
        case ContextMenuOption::LAST_ONE_DO_NOT_USE:
            Q_UNREACHABLE();
            break;
        case ContextMenuOption::NoSelection:
            return;
        case ContextMenuOption::Option:
            break; // "Options" is no longer offered in the node context menu
        case ContextMenuOption::InLayout:
            emit ToggleInLayout(not selectedAction->property("active").toBool());
            break;
        case ContextMenuOption::ForceFlipping:
            emit ToggleForceFlipping(not selectedAction->property("active").toBool());
            break;
        case ContextMenuOption::ShowFullPiece:
            emit ToggleShowFullPiece(not selectedAction->property("active").toBool());
            break;
        case ContextMenuOption::Remove:
            try
            {
                emit Delete();
            }
            catch (const VExceptionToolWasDeleted &e)
            {
                Q_UNUSED(e);
                return; // Leave this method immediately!!!
            }
            return; // Leave this method immediately after call!!!
        case ContextMenuOption::ShowLabel:
            SetLabelVisible(m_id, not selectedAction->property("active").toBool());
            break;
        case ContextMenuOption::Exclude:
            emit ToggleExcludeState(m_id);
            break;
        case ContextMenuOption::TurnPoint:
            emit ToggleTurnPointState(m_id);
            break;
        case ContextMenuOption::ByLength:
            SelectSeamAllowanceAngle(PieceNodeAngle::ByLength);
            break;
        case ContextMenuOption::ByPointsIntersection:
            SelectSeamAllowanceAngle(PieceNodeAngle::ByPointsIntersection);
            break;
        case ContextMenuOption::ByFirstEdgeSymmetry:
            SelectSeamAllowanceAngle(PieceNodeAngle::ByFirstEdgeSymmetry);
            break;
        case ContextMenuOption::BySecondEdgeSymmetry:
            SelectSeamAllowanceAngle(PieceNodeAngle::BySecondEdgeSymmetry);
            break;
        case ContextMenuOption::ByFirstEdgeRightAngle:
            SelectSeamAllowanceAngle(PieceNodeAngle::ByFirstEdgeRightAngle);
            break;
        case ContextMenuOption::BySecondEdgeRightAngle:
            SelectSeamAllowanceAngle(PieceNodeAngle::BySecondEdgeRightAngle);
            break;
        case ContextMenuOption::NonePassmark:
            emit TogglePassmark(m_id, false);
            break;
        case ContextMenuOption::Straightforward:
            SelectPassmarkAngle(PassmarkAngleType::Straightforward);
            break;
        case ContextMenuOption::Bisector:
            SelectPassmarkAngle(PassmarkAngleType::Bisector);
            break;
        case ContextMenuOption::Intersection:
            SelectPassmarkAngle(PassmarkAngleType::Intersection);
            break;
        case ContextMenuOption::IntersectionOnlyLeft:
            SelectPassmarkAngle(PassmarkAngleType::IntersectionOnlyLeft);
            break;
        case ContextMenuOption::IntersectionOnlyRight:
            SelectPassmarkAngle(PassmarkAngleType::IntersectionOnlyRight);
            break;
        case ContextMenuOption::Intersection2:
            SelectPassmarkAngle(PassmarkAngleType::Intersection2);
            break;
        case ContextMenuOption::Intersection2OnlyLeft:
            SelectPassmarkAngle(PassmarkAngleType::Intersection2OnlyLeft);
            break;
        case ContextMenuOption::Intersection2OnlyRight:
            SelectPassmarkAngle(PassmarkAngleType::Intersection2OnlyRight);
            break;
        case ContextMenuOption::OneLine:
            SelectPassmarkLine(PassmarkLineType::OneLine, false);
            break;
        case ContextMenuOption::TwoLines:
            SelectPassmarkLine(PassmarkLineType::TwoLines, false);
            break;
        case ContextMenuOption::ThreeLines:
            SelectPassmarkLine(PassmarkLineType::ThreeLines, false);
            break;
        case ContextMenuOption::TMark:
            SelectPassmarkLine(PassmarkLineType::TMark, false);
            break;
        case ContextMenuOption::ExternalVMark:
            SelectPassmarkLine(PassmarkLineType::ExternalVMark, false);
            break;
        case ContextMenuOption::InternalVMark:
            SelectPassmarkLine(PassmarkLineType::InternalVMark, false);
            break;
        case ContextMenuOption::UMark:
            SelectPassmarkLine(PassmarkLineType::UMark, false);
            break;
        case ContextMenuOption::BoxMark:
            SelectPassmarkLine(PassmarkLineType::BoxMark, false);
            break;
        case ContextMenuOption::CheckMark:
            SelectPassmarkLine(PassmarkLineType::CheckMark, false);
            break;
        case ContextMenuOption::CheckMarkClockwise:
            SelectPassmarkLine(PassmarkLineType::CheckMark, true);
            break;
    };

    QT_WARNING_POP
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    setToolTip(VAbstractTool::data.GetGObject(m_id)->name());
    VScenePoint::hoverEnterEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::EnableToolMove(bool move)
{
    m_namePoint->setFlag(QGraphicsItem::ItemIsMovable, move);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::AllowHover(bool enabled)
{
    setAcceptHoverEvents(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::AllowSelecting(bool enabled)
{
    setFlag(QGraphicsItem::ItemIsSelectable, enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::AllowLabelHover(bool enabled)
{
    m_namePoint->setAcceptHoverEvents(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::AllowLabelSelecting(bool enabled)
{
    m_namePoint->setFlag(QGraphicsItem::ItemIsSelectable, enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void VNodePoint::SetInternalPathNode(bool internalPathNode)
{
    m_internalPathNode = internalPathNode;
    bool onlyPoint = false;
    if (const QSharedPointer<VPointF> point = VAbstractTool::data.GeometricObject<VPointF>(m_id);
        m_internalPathNode && !point->IsShowLabelExplicit())
    {
        onlyPoint = true;
    }

    SetOnlyPoint(onlyPoint);
    // SetOnlyPoint() force-shows the label when onlyPoint is false, without consulting m_showLabel / the
    // global "show all labels" flag. Recompute through the standard predicate so an internal-path node name
    // is hidden unless explicitly shown.
    RefreshScale();
}
