/************************************************************************
 **
 **  @file   vwidgetdetails.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   25 6, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and model patterns of clothing.
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

#include "vwidgetdetails.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vtools/tools/nodeDetails/vnodepoint.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/linkpiecegroup.h"
#include "../vtools/undocommands/renamepiece.h"
#include "../vtools/undocommands/togglepiecestate.h"
#include "../vtools/undocommands/togglegroupinlayout.h"
#include "../vtools/undocommands/unlinkpiecegroup.h"
#include "../core/opiececommonpoint.h"
#include "../core/opiecegrouplayoutflags.h"
#include "opiecegroupdialog.h"
#include "ui_vwidgetdetails.h"

#include <QIcon>
#include <QMenu>
#include <QVariant>
#include <QMessageBox>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QUndoStack>

using namespace std::chrono_literals;

namespace
{
enum PieceColumn : quint8
{
    InLayout = 0,
    PieceName = 1
};

// Role keys
constexpr int kIdRole = Qt::UserRole;
constexpr int kTypeRole = Qt::UserRole + 1;
constexpr char kGroupType[] = "group";
constexpr char kPieceType[] = "piece";

auto FindNodeScenePos(quint32 nodeId) -> QPointF
{
    auto *nodeTool = qobject_cast<VNodePoint *>(VAbstractPattern::getTool(nodeId));
    if (nodeTool != nullptr)
    {
        return nodeTool->scenePos();
    }
    return QPointF();
}

// In-layout indicator icons of the Details tree: the library `check` / `x` icons, theme-aware.
auto LayoutIcon(bool inLayout) -> QIcon
{
    const bool dark = VAbstractApplication::VApp()->Settings()->GetThemeMode() == VThemeMode::Dark;
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");
    const QString name = inLayout ? QStringLiteral("check") : QStringLiteral("x");
    return QIcon(QStringLiteral(":/icons/%1/%2_%1.svg").arg(theme, name));
}

} // namespace

//---------------------------------------------------------------------------------------------------------------------
VWidgetDetails::VWidgetDetails(VContainer *data, VAbstractPattern *doc, QWidget *parent)
  : QWidget(parent),
    ui(new Ui::VWidgetDetails),
    m_doc(doc),
    m_data(data),
    m_updateListTimer(new QTimer(this))
{
    ui->setupUi(this);

    // Two columns: icon (InLayout) in 0, piece name in 1. Without an explicit column count
    // the tree keeps a single column and the name column never renders.
    ui->treeWidget->setColumnCount(2);
    // The in-layout icon column fits its icon tightly so it reads as part of the name row,
    // not as an extra editable column.
    ui->treeWidget->header()->setSectionResizeMode(PieceColumn::InLayout, QHeaderView::ResizeToContents);

    ui->checkBoxHideNotInLayout->setChecked(false);

    // The Link button stays visually pressed (checked) for the whole group workflow; the
    // checked state is driven by SetGroupToolState.
    ui->linkPiecesButton->setCheckable(true);
    ui->linkPiecesButton->setChecked(false);

    FillTree();

    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->treeWidget, &QTreeWidget::itemClicked, this, &VWidgetDetails::ItemClicked);
    connect(ui->treeWidget, &QTreeWidget::itemChanged, this, &VWidgetDetails::ItemChanged);
    connect(ui->treeWidget, &QTreeWidget::customContextMenuRequested, this, &VWidgetDetails::ShowContextMenu);

    m_updateListTimer->setSingleShot(true);
    connect(m_updateListTimer, &QTimer::timeout, this, [this]() -> void { FillTree(); });
}

//---------------------------------------------------------------------------------------------------------------------
VWidgetDetails::~VWidgetDetails()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::UpdateList()
{
    // Filling the tree is expensive; coalesce bursts of updates (800 ms).
    m_updateListTimer->start(800ms);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::SelectDetail(quint32 id)
{
    if (id == NULL_ID)
    {
        ui->treeWidget->clearSelection();
        ui->treeWidget->setCurrentItem(nullptr);
        return;
    }

    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it)
    {
        QTreeWidgetItem *item = *it;
        if (item->data(0, kIdRole).toUInt() == id)
        {
            ui->treeWidget->setCurrentItem(item);
            return;
        }
        ++it;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ToggledPiece(quint32 id)
{
    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it)
    {
        QTreeWidgetItem *item = *it;
        if (item->data(0, kIdRole).toUInt() == id)
        {
            ToggledPieceItem(item);
            return;
        }
        ++it;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);

        // ⚠️ **Le libellé du bouton de fusion ne vient pas du `.ui`.** Il dépend de l'état du
        // groupe : « Merge pieces into one » au repos, « Cancel » pendant le travail — écrit par
        // `SetGroupToolState()`, qui sort tôt quand l'état n'a pas changé et ne peut donc pas être
        // rappelé ici. `retranslateUi()` réécrit le texte du `.ui` : il remettait « Fusionner les
        // pièces en une seule » alors que le travail était en cours, et le bouton annonçait le
        // contraire de ce qu'il fait. On repose la chaîne qui correspond à l'état, dans la langue
        // qui vient d'être installée.
        const bool active = m_groupToolState != GroupToolState::Idle;
        ui->linkPiecesButton->setChecked(active);
        ui->linkPiecesButton->setText(active ? tr("Cancel") : tr("Merge pieces into one"));
    }

    // remember to call base class implementation
    QWidget::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ItemClicked(QTreeWidgetItem *item, int column)
{
    if (item == nullptr)
    {
        return;
    }

    const QString type = item->data(0, kTypeRole).toString();
    const quint32 id = item->data(0, kIdRole).toUInt();
    if (id == NULL_ID)
    {
        return;  // informational child rows are not interactive
    }

    if (type == QLatin1String(kGroupType))
    {
        // Clicking the in-layout icon toggles the group's state (like a piece); clicking the
        // name opens the group's property browser.
        if (column == PieceColumn::InLayout)
        {
            ToggleGroupInLayout(id);
        }
        else
        {
            SelectGroup(id);
        }
        return;
    }

    emit Highlight(id);
    // Opening the piece's property browser (Val Fork OnItemClicked): a member piece opens
    // its OWN browser, the group's browser only via the group node. Not during the Link tool.
    if (m_groupToolState == GroupToolState::Idle)
    {
        emit PieceSelected(id);
    }
    if (column == PieceColumn::InLayout)
    {
        ToggleInLayout(id);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ItemChanged(QTreeWidgetItem *item, int column)
{
    if (m_filling || item == nullptr)
    {
        return;
    }

    const QString type = item->data(0, kTypeRole).toString();
    if (type != QLatin1String(kPieceType) || column != PieceColumn::PieceName)
    {
        return;
    }

    const quint32 id = item->data(0, kIdRole).toUInt();
    emit Highlight(id);

    const QString newName = item->text(column);
    auto *renameDetail = new ::RenamePiece(m_doc, newName, id);
    connect(renameDetail, &RenamePiece::UpdateList, this, &VWidgetDetails::UpdateList);
    VAbstractApplication::VApp()->getUndoStack()->push(renameDetail);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::FillTree()
{
    const QSignalBlocker blocker(ui->treeWidget);
    m_filling = true;

    ui->treeWidget->clear();

    const auto *details = m_data->DataPieces();
    const auto *groups = m_data->DataPieceGroups();

    QSet<quint32> groupedPieceIds;

    // Group items first — group top-level, members as children (without their own children).
    if (groups != nullptr && not groups->isEmpty())
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            const OPieceGroup &group = git.value();

            auto *groupItem = new QTreeWidgetItem();
            groupItem->setText(PieceColumn::PieceName, group.name);
            groupItem->setIcon(PieceColumn::InLayout, LayoutIcon(group.m_inLayout));
            groupItem->setData(0, kIdRole, git.key());
            groupItem->setData(0, kTypeRole, QLatin1String(kGroupType));
            groupItem->setData(0, kTreeInLayoutRole, group.m_inLayout);

            for (quint32 pieceId : group.pieceIds)
            {
                groupedPieceIds.insert(pieceId);

                if (details != nullptr && details->contains(pieceId))
                {
                    const VPiece det = details->value(pieceId);
                    auto *pieceItem = new QTreeWidgetItem();
                    pieceItem->setText(PieceColumn::PieceName,
                                       det.GetName().isEmpty() ? tr("Unnamed") : det.GetName());
                    // A member's displayed state follows its group, never its own flag.
                    const bool inLayout = m_data->IsPieceInLayout(pieceId);
                    pieceItem->setIcon(PieceColumn::InLayout, LayoutIcon(inLayout));
                    pieceItem->setData(0, kIdRole, pieceId);
                    pieceItem->setData(0, kTypeRole, QLatin1String(kPieceType));
                    pieceItem->setData(0, kTreeInLayoutRole, inLayout);

                    // Member pieces hide their own children (preserved for unlink).
                    groupItem->addChild(pieceItem);
                }
            }

            ui->treeWidget->addTopLevelItem(groupItem);
        }
    }

    // Standalone pieces — top-level with their child rows.
    if (details != nullptr)
    {
        for (auto it = details->cbegin(); it != details->cend(); ++it)
        {
            if (groupedPieceIds.contains(it.key()))
            {
                continue;
            }
            const VPiece det = it.value();
            auto *pieceItem = new QTreeWidgetItem();
            pieceItem->setText(PieceColumn::PieceName, det.GetName().isEmpty() ? tr("Unnamed") : det.GetName());
            pieceItem->setIcon(PieceColumn::InLayout, LayoutIcon(m_data->IsPieceInLayout(it.key())));
            pieceItem->setData(0, kIdRole, it.key());
            pieceItem->setData(0, kTypeRole, QLatin1String(kPieceType));
            pieceItem->setData(0, kTreeInLayoutRole, m_data->IsPieceInLayout(it.key()));
            pieceItem->setFlags(pieceItem->flags() | Qt::ItemIsEditable);

            ui->treeWidget->addTopLevelItem(pieceItem);
        }
    }

    ui->treeWidget->expandAll();
    m_filling = false;

    on_checkBoxHideNotInLayout_stateChanged(); // Trigger hide for action from context menu
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ToggleSectionDetails(bool select)
{
    const QHash<quint32, VPiece> *allDetails = m_data->DataPieces();
    if (allDetails == nullptr || allDetails->isEmpty())
    {
        return;
    }
    const auto *groups = m_data->DataPieceGroups();

    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it)
    {
        QTreeWidgetItem *item = *it;
        const quint32 id = item->data(0, kIdRole).toUInt();
        const QString type = item->data(0, kTypeRole).toString();
        if (type == QLatin1String(kGroupType) && groups != nullptr && groups->contains(id))
        {
            // A group behaves like one piece: its own in-layout flag drives the whole group.
            if (not(select == groups->value(id).IsInLayout()))
            {
                auto *toggleGroup = new ::ToggleGroupInLayout(id, select, m_data, m_doc);
                connect(toggleGroup, &::ToggleGroupInLayout::Toggled, this, &VWidgetDetails::ToggledGroup);
                VAbstractApplication::VApp()->getUndoStack()->push(toggleGroup);
            }
        }
        else if (type == QLatin1String(kPieceType) && allDetails->contains(id) &&
                 m_data->GetPieceGroupId(id) == NULL_ID &&  // a member's state follows its group
                 not(select == allDetails->value(id).IsInLayout()))
        {
            auto *togglePrint = new TogglePieceInLayout(id, select, m_data, m_doc);
            connect(togglePrint, &TogglePieceInLayout::Toggled, this, &VWidgetDetails::ToggledPiece);
            VAbstractApplication::VApp()->getUndoStack()->push(togglePrint);
        }
        ++it;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ToggledPieceItem(QTreeWidgetItem *item)
{
    SCASSERT(item != nullptr)

    const quint32 id = item->data(0, kIdRole).toUInt();
    const QHash<quint32, VPiece> *details = m_data->DataPieces();
    if (details == nullptr || not details->contains(id))
    {
        return;
    }

    // A member piece's visibility follows its GROUP (a group behaves like one piece).
    const bool inLayout = m_data->IsPieceInLayout(id);
    item->setIcon(PieceColumn::InLayout, LayoutIcon(inLayout));
    item->setData(0, kTreeInLayoutRole, inLayout);

    VToolSeamAllowance *tool = nullptr;
    try
    {
        tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(id));
        if (tool != nullptr)
        {
            tool->setVisible(ui->checkBoxHideNotInLayout->isChecked() ? inLayout : true);
        }
    }
    catch (VExceptionBadId &)
    {
        // do nothing
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ToggleInLayout(quint32 id)
{
    const QHash<quint32, VPiece> *allDetails = m_data->DataPieces();
    if (allDetails == nullptr || not allDetails->contains(id))
    {
        return;
    }

    // A member piece follows its GROUP (a group behaves like one piece): it has no in-layout state
    // of its own, so nothing is pushed and nothing is written to the pattern file. The group's own
    // row carries the toggle.
    if (m_data->GetPieceGroupId(id) != NULL_ID)
    {
        return;
    }

    const bool inLayout = not allDetails->value(id).IsInLayout();
    auto *togglePrint = new TogglePieceInLayout(id, inLayout, m_data, m_doc);
    connect(togglePrint, &TogglePieceInLayout::Toggled, this, &VWidgetDetails::ToggledPiece);
    VAbstractApplication::VApp()->getUndoStack()->push(togglePrint);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ToggleGroupInLayout(quint32 groupId)
{
    auto *groups = m_data->DataPieceGroups();
    if (groups == nullptr || not groups->contains(groupId))
    {
        return;
    }

    const bool inLayout = not groups->value(groupId).IsInLayout();
    auto *toggleCommand = new ::ToggleGroupInLayout(groupId, inLayout, m_data, m_doc);
    connect(toggleCommand, &::ToggleGroupInLayout::Toggled, this, &VWidgetDetails::ToggledGroup);
    VAbstractApplication::VApp()->getUndoStack()->push(toggleCommand);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ToggledGroup(quint32 groupId)
{
    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it)
    {
        QTreeWidgetItem *item = *it;
        if (item->data(0, kIdRole).toUInt() == groupId)
        {
            ToggledGroupItem(item);
            return;
        }
        ++it;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ToggledGroupItem(QTreeWidgetItem *item)
{
    SCASSERT(item != nullptr)

    const quint32 groupId = item->data(0, kIdRole).toUInt();
    auto *groups = m_data->DataPieceGroups();
    if (groups == nullptr || not groups->contains(groupId))
    {
        return;
    }

    const bool inLayout = groups->value(groupId).IsInLayout();
    item->setIcon(PieceColumn::InLayout, LayoutIcon(inLayout));
    item->setData(0, kTreeInLayoutRole, inLayout);

    // Refresh the member rows too: their displayed in-layout state follows the group, so
    // toggling the group (Select all/none, Invert, or the group's own check icon) must update
    // their check/x immediately, not only on the next full tree refresh.
    for (int i = 0; i < item->childCount(); ++i)
    {
        if (QTreeWidgetItem *child = item->child(i))
        {
            ToggledPieceItem(child);
        }
    }

    // The group behaves like one piece: its state + the "Hide not in layout" checkbox drive
    // the visibility of the whole group on the canvas (holder + members).
    const bool visible = ui->checkBoxHideNotInLayout->isChecked() ? inLayout : true;
    emit GroupVisibilityChanged(groupId, visible);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::SelectGroup(quint32 groupId)
{
    if (groupId == NULL_ID)
    {
        return;
    }
    emit GroupSelected(groupId);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ShowContextMenu(const QPoint &pos)
{
    QScopedPointer<QMenu> const menu(new QMenu());

    // Tree context-menu glyphs (see Icons.md): `link-break` for a group's "Unlink all",
    // `trash` for a piece's delete. A highlighted row has a dark accent background in both
    // themes, so under the cursor the glyph is swapped to its dark (light-colored) variant.
    const bool dark = VAbstractApplication::VApp()->Settings()->GetThemeMode() == VThemeMode::Dark;
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");
    const auto themeIcon = [&theme](const QString &name) {
        return QIcon(QStringLiteral(":/icons/%1/%2_%1.svg").arg(theme, name));
    };
    const auto hoverIcon = [](const QString &name) {
        return QIcon(QStringLiteral(":/icons/dark/%1_dark.svg").arg(name));
    };
    const auto addGlyphIcon = [&themeIcon, &hoverIcon](QAction *action, const QString &name) {
        const QIcon rest = themeIcon(name);
        action->setIcon(rest);
        action->setProperty("restIcon", QVariant::fromValue(rest));
        action->setProperty("hoverIcon", QVariant::fromValue(hoverIcon(name)));
    };
    // "In layout" mirrors the canvas context menu (VToolSeamAllowance::contextMenuEvent): a themed
    // `check` when the unit goes in the layout, no glyph when it does not, and the dark variant
    // under the cursor.
    const auto addInLayoutToggle = [&menu, &themeIcon, &hoverIcon](bool inLayout) {
        QAction *action = menu->addAction(VWidgetDetails::tr("In layout"));
        if (inLayout)
        {
            const QIcon check = themeIcon(QStringLiteral("check"));
            action->setIcon(check);
            action->setProperty("restIcon", QVariant::fromValue(check));
            action->setProperty("hoverIcon", QVariant::fromValue(hoverIcon(QStringLiteral("check"))));
        }
        return action;
    };

    QAction *actionSelectAll = menu->addAction(tr("Select all"));
    QAction *actionSelectNone = menu->addAction(tr("Select none"));

    menu->addSeparator();

    bool pieceMode = false;
    QAction *actionDeletePiece = nullptr;
    QAction *actionUnlinkGroup = nullptr;
    QAction *actionInLayout = nullptr;
    VToolSeamAllowance *toolPiece = nullptr;
    quint32 groupId = NULL_ID;
    quint32 pieceId = NULL_ID;

    if (QTreeWidgetItem *selectedItem = ui->treeWidget->itemAt(pos); selectedItem)
    {
        const QString type = selectedItem->data(0, kTypeRole).toString();
        const quint32 id = selectedItem->data(0, kIdRole).toUInt();

        if (type == QLatin1String(kGroupType) && id != NULL_ID)
        {
            groupId = id;
            menu->addSeparator();

            // The group's own state, never a member's: a group behaves like one piece.
            bool groupInLayout = false;
            if (const auto *groups = m_data->DataPieceGroups();
                groups != nullptr && groups->contains(id))
            {
                groupInLayout = groups->value(id).IsInLayout();
            }
            actionInLayout = addInLayoutToggle(groupInLayout);

            actionUnlinkGroup = menu->addAction(tr("Unlink all"));
            addGlyphIcon(actionUnlinkGroup, QStringLiteral("link-break"));
        }
        else if (id != NULL_ID)
        {
            try
            {
                toolPiece = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(id));
                if (toolPiece != nullptr)
                {
                    pieceMode = true;
                    pieceId = id;
                    menu->addSeparator();

                    // A member piece follows its group: it has no in-layout state of its own, so the
                    // entry shows the group's state and stays disabled (nothing is written).
                    actionInLayout = addInLayoutToggle(m_data->IsPieceInLayout(id));
                    actionInLayout->setEnabled(m_data->GetPieceGroupId(id) == NULL_ID);

                    actionDeletePiece = menu->addAction(tr("Delete piece"));
                    addGlyphIcon(actionDeletePiece, QStringLiteral("trash"));
                    actionDeletePiece->setEnabled(toolPiece->IsRemovable() == RemoveStatus::Removable);
                }
            }
            catch (const VExceptionBadId &)
            {
                const QString errorMsg = tr("Cannot find piece by id '%1'").arg(id);
                qWarning() << VAbstractValApplication::warningMessageSignature + errorMsg;
            }
        }
    }

    const QHash<quint32, VPiece> *allDetails = m_data->DataPieces();
    if (allDetails == nullptr || allDetails->isEmpty())
    {
        return;
    }

    // Count the effective togglable units: standalone pieces + groups (a group = one piece,
    // its members' individual flags are masked by the group).
    int selectedUnits = 0;
    int totalUnits = 0;
    const auto *groups = m_data->DataPieceGroups();
    for (auto it = allDetails->cbegin(); it != allDetails->cend(); ++it)
    {
        if (m_data->GetPieceGroupId(it.key()) == NULL_ID)
        {
            ++totalUnits;
            if (it.value().IsInLayout())
            {
                ++selectedUnits;
            }
        }
    }
    if (groups != nullptr)
    {
        for (auto git = groups->cbegin(); git != groups->cend(); ++git)
        {
            ++totalUnits;
            if (git.value().IsInLayout())
            {
                ++selectedUnits;
            }
        }
    }

    if (selectedUnits == 0)
    {
        actionSelectNone->setDisabled(true);
    }
    else if (selectedUnits == totalUnits)
    {
        actionSelectAll->setDisabled(true);
    }

    // Swap the glyph to its dark variant while its row is highlighted (a highlighted row has a
    // dark accent background in both themes); restore it when the highlight moves away.
    QAction *hoveredAction = nullptr;
    const auto applyHover = [&hoveredAction](QAction *action) {
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
    connect(menu.data(), &QMenu::hovered, this, [&applyHover](QAction *action) { applyHover(action); });
    connect(menu.data(), &QMenu::aboutToHide, this, [&applyHover]() { applyHover(nullptr); });

    QAction *selectedAction = menu->exec(ui->treeWidget->viewport()->mapToGlobal(pos));

    bool select = false;
    if (selectedAction == actionSelectAll)
    {
        select = true;
        VAbstractApplication::VApp()->getUndoStack()->beginMacro(tr("select all details"));
        ToggleSectionDetails(select);
        VAbstractApplication::VApp()->getUndoStack()->endMacro();
    }
    else if (selectedAction == actionSelectNone)
    {
        VAbstractApplication::VApp()->getUndoStack()->beginMacro(tr("select none details"));
        ToggleSectionDetails(select);
        VAbstractApplication::VApp()->getUndoStack()->endMacro();
    }
    else if (actionInLayout != nullptr && selectedAction == actionInLayout && actionInLayout->isEnabled())
    {
        if (groupId != NULL_ID)
        {
            ToggleGroupInLayout(groupId);
        }
        else if (pieceMode)
        {
            ToggleInLayout(pieceId);
        }
    }
    else if (pieceMode && selectedAction == actionDeletePiece)
    {
        try
        {
            toolPiece->DeleteFromMenu();
        }
        catch (const VExceptionToolWasDeleted &e)
        {
            Q_UNUSED(e);
            return; // Leave this method immediately!!!
        }
        // Leave this method immediately after call!!!
    }
    else if (groupId != NULL_ID && selectedAction == actionUnlinkGroup)
    {
        UnlinkGroup(groupId);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::on_checkBoxHideNotInLayout_stateChanged()
{
    QTreeWidgetItemIterator it(ui->treeWidget);
    while (*it)
    {
        QTreeWidgetItem *item = *it;
        const QString type = item->data(0, kTypeRole).toString();
        if (type == QLatin1String(kGroupType))
        {
            // Groups are masked like one piece: their state + the checkbox drive the holder.
            ToggledGroupItem(item);
        }
        else
        {
            ToggledPieceItem(item);
        }
        ++it;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::on_linkPiecesButton_clicked()
{
    if (m_groupToolState != GroupToolState::Idle)
    {
        CancelGroupSelection();
        return;
    }

    m_selectedPiecesForGroup.clear();
    m_tempOffsets.clear();
    SetGroupToolState(GroupToolState::Selecting);
    emit ShowStatusTip(tr("Select pieces to link (min 2, Enter to confirm, Esc to cancel)"));
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::OnPieceClickedForGroup(quint32 pieceId)
{
    if (m_groupToolState != GroupToolState::Selecting)
    {
        return;
    }

    // A piece already in a group cannot be re-selected (no group-in-group).
    if (m_data->GetPieceGroupId(pieceId) != NULL_ID)
    {
        emit ShowStatusTip(tr("This piece is already part of a group"));
        return;
    }

    auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
    if (tool == nullptr)
    {
        return;
    }

    if (m_selectedPiecesForGroup.contains(pieceId))
    {
        m_selectedPiecesForGroup.remove(pieceId);
        tool->SetGroupSelectionHighlight(false);
        emit ShowStatusTip(tr("Select pieces to link (min 2, Enter to confirm, Esc to cancel)"));
    }
    else
    {
        m_selectedPiecesForGroup.insert(pieceId);
        tool->SetGroupSelectionHighlight(true);
        emit ShowStatusTip(tr("%1 piece(s) selected — Enter to confirm, Esc to cancel")
                               .arg(m_selectedPiecesForGroup.size()));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::ValidateGroupSelection()
{
    if (m_groupToolState != GroupToolState::Selecting)
    {
        return;
    }

    if (m_selectedPiecesForGroup.size() < 2)
    {
        emit ShowStatusTip(tr("Less than 2 pieces selected — cancelled"));
        CancelGroupSelection();
        return;
    }

    EnterNodeSelectMode();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::EnterNodeSelectMode()
{
    m_tempOffsets.clear();
    SetGroupToolState(GroupToolState::NodeSelecting);
    emit ShowStatusTip(tr("Click the common point (Esc to cancel)"));
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::OnNodeSelected(quint32 nodeId)
{
    if (m_groupToolState != GroupToolState::NodeSelecting)
    {
        return;
    }

    // Strict validation (Ostara divergence): the point must exist in EVERY selected piece's path.
    // The match is made on the construction source of the node (idObject), not on the node id
    // itself: two pieces sharing a construction point each own a different modeling copy of it.
    // A piece may have been deleted between the selection and this click — abort cleanly.
    const quint32 sourceId = OPieceCommonPoint::SourceNodeId(nodeId, m_data);
    for (quint32 pid : m_selectedPiecesForGroup)
    {
        if (m_data->DataPieces() == nullptr || not m_data->DataPieces()->contains(pid))
        {
            CancelGroupSelection();
            return;
        }
        const VPiece piece = m_data->GetPiece(pid);
        if (not OPieceCommonPoint::PieceHasNodeWithSource(piece, sourceId, m_data))
        {
            QMessageBox::information(
                this, tr("Link pieces"),
                tr("The selected point is not common to the %1 pieces of the group")
                    .arg(m_selectedPiecesForGroup.size()));
            return;  // stay in NodeSelecting — let the user click another point
        }
    }

    // Common point: compute each member's offset relative to the shared source node position.
    // Each member owns its own modeling copy of the source — resolve its own node id.
    const QPointF refScenePos = FindNodeScenePos(nodeId);
    m_tempOffsets.clear();
    m_pendingAlignSourceId = sourceId;
    m_pendingAlignMemberId = NULL_ID;
    for (quint32 pid : m_selectedPiecesForGroup)
    {
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
        if (tool == nullptr)
        {
            continue;
        }
        const quint32 memberNodeId =
            OPieceCommonPoint::PieceNodeWithSource(m_data->GetPiece(pid), sourceId, m_data);
        if (memberNodeId == NULL_ID)
        {
            continue;  // already validated above — defensive only
        }
        if (memberNodeId == nodeId)
        {
            m_pendingAlignMemberId = pid;  // the member the user actually clicked
        }
        const QPointF memberScenePos = FindNodeScenePos(memberNodeId);
        const QPointF offset = refScenePos - memberScenePos;
        if (!qFuzzyIsNull(offset.x()) || !qFuzzyIsNull(offset.y()))
        {
            m_tempOffsets[pid] = offset;
        }
    }

    CompleteGroupCreation();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::CompleteGroupCreation()
{
    OPieceGroupDialog dialog(m_data, static_cast<int>(m_selectedPiecesForGroup.size()), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        CancelGroupSelection();
        return;
    }

    OPieceGroup group = dialog.GetGroup();
    group.pieceIds = QVector<quint32>(m_selectedPiecesForGroup.cbegin(), m_selectedPiecesForGroup.cend());
    group.memberOffsets = m_tempOffsets;
    // The common point itself is kept, so the graded alignment can anchor on it later. Using it
    // stays an explicit choice: alignOnPoint is left at its default (off).
    group.alignPointSourceId = m_pendingAlignSourceId;
    group.alignPointMemberId = m_pendingAlignMemberId;

    // The layout flags are the members' own: a group carries none. Read here, before the dialog goes away.
    const bool groupForceFlipping = dialog.IsForceFlipping();
    const bool groupSymmetricalCopy = dialog.IsSymmetricalCopy();

    auto *linkCommand = new LinkPieceGroup(group, m_doc, m_data);
    connect(linkCommand, &LinkPieceGroup::UpdateGroups, this, &VWidgetDetails::UpdateList);

    // Linking and the flags the group was given are one action, so they are one undo step.
    QUndoStack *stack = VAbstractApplication::VApp()->getUndoStack();
    stack->beginMacro(tr("Link pieces"));
    stack->push(linkCommand);
    OPieceGroupLayoutFlags::Apply(group.pieceIds, groupForceFlipping, groupSymmetricalCopy, m_data, m_doc);
    stack->endMacro();

    // Mask the members' own children (derived from GetPieceGroupId at render).
    RefreshGroupedChildren(group.pieceIds);

    DeselectSelectedPieces();
    m_tempOffsets.clear();
    m_pendingAlignSourceId = NULL_ID;
    m_pendingAlignMemberId = NULL_ID;
    SetGroupToolState(GroupToolState::Idle);
    emit ShowStatusTip(QString());
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::UnlinkGroup(quint32 groupId)
{
    const auto *groups = m_data->DataPieceGroups();
    QVector<quint32> memberIds;
    if (groups != nullptr && groups->contains(groupId))
    {
        memberIds = groups->value(groupId).pieceIds;
    }

    auto *unlinkCommand = new UnlinkPieceGroup(groupId, m_doc, m_data);
    connect(unlinkCommand, &UnlinkPieceGroup::UpdateGroups, this, &VWidgetDetails::UpdateList);
    VAbstractApplication::VApp()->getUndoStack()->push(unlinkCommand);

    // Restore the members' own children after unlink.
    RefreshGroupedChildren(memberIds);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::FinishGroupSelection()
{
    if (m_groupToolState == GroupToolState::Selecting)
    {
        ValidateGroupSelection();
    }
    // In NodeSelecting the common point is click-driven; Enter has nothing to do.
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::CancelGroupSelection()
{
    DeselectSelectedPieces();
    m_tempOffsets.clear();
    m_pendingAlignSourceId = NULL_ID;
    m_pendingAlignMemberId = NULL_ID;
    SetGroupToolState(GroupToolState::Idle);
    emit ShowStatusTip(QString());
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::DeselectSelectedPieces()
{
    for (quint32 pid : m_selectedPiecesForGroup)
    {
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
        if (tool != nullptr)
        {
            tool->SetGroupSelectionHighlight(false);
        }
    }
    m_selectedPiecesForGroup.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::SetGroupToolState(GroupToolState state)
{
    if (m_groupToolState == state)
    {
        return;
    }
    m_groupToolState = state;
    // The button is a toggle: it starts the merge workflow and stays visually selected with a
    // "Cancel" label while the workflow is active (Idle → unchecked, "Merge pieces into one").
    const bool active = state != GroupToolState::Idle;
    ui->linkPiecesButton->setChecked(active);
    ui->linkPiecesButton->setText(active ? tr("Cancel") : tr("Merge pieces into one"));
    emit GroupToolStateChanged(state);
}

//---------------------------------------------------------------------------------------------------------------------
void VWidgetDetails::RefreshGroupedChildren(const QVector<quint32> &pieceIds)
{
    for (quint32 pid : pieceIds)
    {
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid));
        if (tool != nullptr)
        {
            tool->RefreshGroupedChildren();
        }
    }
}
