/************************************************************************
 **
 **  @file   onodeplacelabel.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/09/2026
 **
 **  @brief  Clickable place-label point of a Details piece — implementation
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

#include "onodeplacelabel.h"

#include <QCoreApplication>
#include <QGraphicsSceneContextMenuEvent>
#include <QIcon>
#include <QMenu>
#include <QVariant>
#include <QVector>

#include "../vgeometry/vplacelabelitem.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtheme/vtheme.h"
#include "../vtoolseamallowance.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vexception/vexception.h"

//---------------------------------------------------------------------------------------------------------------------
ONodePlaceLabel::ONodePlaceLabel(QGraphicsItem *parent)
  : VScenePoint(VColorRole::PiecePointColor, parent)
{
    // Details-mode piece points are outline-only at rest; the fill appears on hover/active.
    SetPointFillEnabled(false);

    // VScenePoint defaults to the Draw-mode DraftLabel* roles — switch to the piece roles, the
    // same recolouring VNodePoint applies for the piece's own node points.
    SetLabelTextColorRole(VColorRole::PieceNodeLabelColor);
    SetLabelTextHoverColorRole(VColorRole::PieceNodeLabelHoverColor);
    SetLabelLineColorRole(VColorRole::PieceNodeLabelLineColor);
    m_namePoint->SetShowParentTooltip(false);

    // Standard selectable-point semantics: a click selects the handle (Qt single-selection);
    // picking any other item or the empty canvas clears it.
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setCursor(Qt::ArrowCursor);
}

//---------------------------------------------------------------------------------------------------------------------
auto ONodePlaceLabel::itemChange(GraphicsItemChange change, const QVariant &value) -> QVariant
{
    // Mirror the Qt selection state onto VScenePoint's active pen, so a selected label handle is
    // drawn exactly like an active point (orange) and reverts as soon as Qt deselects it.
    if (change == QGraphicsItem::ItemSelectedHasChanged)
    {
        SetActivePoint(isSelected());
    }
    return VScenePoint::itemChange(change, value);
}

//---------------------------------------------------------------------------------------------------------------------
void ONodePlaceLabel::UpdateFromLabel(quint32 labelId, const VPlaceLabelItem &label)
{
    m_id = labelId;
    setToolTip(label.name());

    // VScenePoint::RefreshPointGeometry sets the scene position from the label coordinates, the
    // point-name text at mx/my and re-evaluates label visibility (zoom/settings) + leader line.
    RefreshPointGeometry(label);
}

//---------------------------------------------------------------------------------------------------------------------
void ONodePlaceLabel::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    auto *piece = qgraphicsitem_cast<VToolSeamAllowance *>(parentItem());
    const QSharedPointer<VPlaceLabelItem> label = piece != nullptr
                                                      ? piece->getData()->GeometricObject<VPlaceLabelItem>(m_id)
                                                      : QSharedPointer<VPlaceLabelItem>();
    if (label.isNull())
    {
        event->accept();
        return;
    }

    // The context menu of an internal-mark handle offers a single choice: the mark Type.
    QMenu menu;

    // Glyph: at rest the current theme variant; a highlighted row has a dark accent background in
    // both themes, so under the cursor the check is swapped to the dark (light-colored) variant.
    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");
    const auto themeIcon = [&theme](const QString &name) {
        return QIcon(QStringLiteral(":/icons/%1/%2_%1.svg").arg(theme, name));
    };
    const auto darkIcon = [](const QString &name) {
        return QIcon(QStringLiteral(":/icons/dark/%1_dark.svg").arg(name));
    };
    const auto addType = [&menu, &themeIcon, &darkIcon](const QString &text, bool active) -> QAction *
    {
        QAction *action = menu.addAction(text);
        if (active)
        {
            const QIcon check = themeIcon(QStringLiteral("check"));
            action->setIcon(check);
            action->setProperty("restIcon", QVariant::fromValue(check));
            action->setProperty("hoverIcon", QVariant::fromValue(darkIcon(QStringLiteral("check"))));
        }
        return action;
    };

    // Same order and names as the dock page and the creation dialog (see ODetailsPlaceLabelWidget).
    struct TypeEntry
    {
        PlaceLabelType type;
        const char *name;
    };
    constexpr TypeEntry kTypes[] = {
        {PlaceLabelType::Cross, "Cross"},
        {PlaceLabelType::Segment, "Segment"},
        {PlaceLabelType::Rectangle, "Rectangle"},
        {PlaceLabelType::Tshaped, "T-shaped"},
        {PlaceLabelType::Doubletree, "Doubletree"},
        {PlaceLabelType::Corner, "Corner"},
        {PlaceLabelType::Triangle, "Triangle"},
        {PlaceLabelType::Hshaped, "H-shaped"},
        {PlaceLabelType::Button, "Button"},
        {PlaceLabelType::Circle, "Circle"},
    };

    const PlaceLabelType current = label->GetLabelType();
    QAction *selectedAction = nullptr;
    for (const TypeEntry &entry : kTypes)
    {
        QAction *action =
            addType(QCoreApplication::translate("ODetailsPlaceLabelWidget", entry.name), current == entry.type);
        action->setData(static_cast<int>(entry.type));
    }

    // Same layout as a piece node's menu (see VNodePoint::InitContextMenu): the action list, then a
    // separator, then Delete with the trash glyph.
    auto *separatorAct = new QAction(&menu);
    separatorAct->setSeparator(true);
    menu.addAction(separatorAct);

    QAction *deleteAction = menu.addAction(QCoreApplication::translate("ODetailsPlaceLabelWidget", "Delete"));
    const QIcon trash = themeIcon(QStringLiteral("trash"));
    deleteAction->setIcon(trash);
    deleteAction->setProperty("restIcon", QVariant::fromValue(trash));
    deleteAction->setProperty("hoverIcon", QVariant::fromValue(darkIcon(QStringLiteral("trash"))));

    // Swap the glyph to its dark variant while its row is highlighted; restore on move/hide.
    QAction *hoveredAction = nullptr;
    const auto applyHover = [&hoveredAction](QAction *action)
    {
        if (action == hoveredAction)
        {
            return;
        }
        if (hoveredAction != nullptr)
        {
            const QVariant rest = hoveredAction->property("restIcon");
            if (rest.isValid())
            {
                hoveredAction->setIcon(rest.value<QIcon>());
            }
        }
        hoveredAction = action;
        if (action != nullptr)
        {
            const QVariant hover = action->property("hoverIcon");
            if (hover.isValid())
            {
                action->setIcon(hover.value<QIcon>());
            }
        }
    };
    QObject::connect(&menu, &QMenu::hovered, [&applyHover](QAction *action) { applyHover(action); });
    QObject::connect(&menu, &QMenu::aboutToHide, [&applyHover]() { applyHover(nullptr); });

    selectedAction = menu.exec(event->screenPos());
    event->accept();

    if (selectedAction == nullptr)
    {
        return;
    }

    if (selectedAction == deleteAction)
    {
        // A real deletion: the mark leaves the piece's list and its <point> record is removed, which
        // triggers a full parse — this scene, this handle included, dies inside the call. So leave at
        // once; VExceptionToolWasDeleted is the same contract VNodePoint relies on.
        try
        {
            piece->DeletePlaceLabels(QVector<quint32>{m_id});
        }
        catch (const VExceptionToolWasDeleted &e)
        {
            Q_UNUSED(e);
        }
        return;
    }

    const auto type = static_cast<PlaceLabelType>(selectedAction->data().toInt());
    if (type != current)
    {
        piece->ChangePlaceLabelType(m_id, static_cast<int>(type));
    }
}
