/************************************************************************
 **
 **  @file   dialogmainpath.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Modal dialog for editing the main path nodes of a piece
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

#include "dialogmainpath.h"
#include "../../dialogtoolbox.h"
#include "../../tools/dialogtool.h"
#include "../vmisc/def.h"
#include "vtheme.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vlayout/vlayoutpoint.h"

#include <QIcon>

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{

enum class MainPathContextMenuOption : int
{
    NoSelection,
    Reverse,
    NonePassmark,
    OneLine,
    TwoLines,
    ThreeLines,
    TMark,
    ExternalVMark,
    InternalVMark,
    UMark,
    BoxMark,
    CheckMark,
    Uniqueness,
    TurnPoint,
    Excluded,
    DuplicateNode,
    Delete,
    LAST_ONE_DO_NOT_USE
};

} // namespace

//---------------------------------------------------------------------------------------------------------------------
DialogMainPath::DialogMainPath(const VContainer *data, QWidget *parent)
  : QDialog(parent),
    m_data(data)
{
    setWindowTitle(tr("Manage Main Path"));
    setMinimumSize(550, 400);
    BuildUi();
    ConnectSignals();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::BuildUi()
{
    auto *mainLayout = new QVBoxLayout(this);

    // ── Toolbar ────────────────────────────────────────────────────────
    auto *toolbarLayout = new QHBoxLayout();

    m_btnReverse = new QToolButton();
    m_btnReverse->setToolTip(tr("Reverse node direction"));
    m_btnReverse->setIcon(FromTheme(VThemeIcon::ListRemove));
    m_btnReverse->setCheckable(true);
    m_btnReverse->setEnabled(false);
    toolbarLayout->addWidget(m_btnReverse);

    m_btnExcluded = new QToolButton();
    m_btnExcluded->setToolTip(tr("Excluded node"));
    m_btnExcluded->setIcon(FromTheme(VThemeIcon::FormatTextStrikethrough));
    m_btnExcluded->setCheckable(true);
    m_btnExcluded->setEnabled(false);
    toolbarLayout->addWidget(m_btnExcluded);

    m_btnTurnPoint = new QToolButton();
    m_btnTurnPoint->setToolTip(tr("Turn point"));
    m_btnTurnPoint->setIcon(FromTheme(VThemeIcon::SnapNodesRotationCenter));
    m_btnTurnPoint->setCheckable(true);
    m_btnTurnPoint->setEnabled(false);
    toolbarLayout->addWidget(m_btnTurnPoint);

    m_btnCheckUniqueness = new QToolButton();
    m_btnCheckUniqueness->setToolTip(tr("Check node uniqueness"));
    m_btnCheckUniqueness->setIcon(FromTheme(VThemeIcon::DrawStar));
    m_btnCheckUniqueness->setCheckable(true);
    m_btnCheckUniqueness->setEnabled(false);
    toolbarLayout->addWidget(m_btnCheckUniqueness);

    m_btnPassmark = new QToolButton();
    m_btnPassmark->setToolTip(tr("Notch"));
    m_btnPassmark->setIcon(FromTheme(VThemeIcon::AddPlacemark));
    m_btnPassmark->setCheckable(true);
    m_btnPassmark->setEnabled(false);
    toolbarLayout->addWidget(m_btnPassmark);

    m_btnDuplicateNode = new QToolButton();
    m_btnDuplicateNode->setToolTip(tr("Duplicate"));
    m_btnDuplicateNode->setIcon(FromTheme(VThemeIcon::DuplicateNode));
    m_btnDuplicateNode->setEnabled(false);
    toolbarLayout->addWidget(m_btnDuplicateNode);

    m_btnDelete = new QToolButton();
    m_btnDelete->setToolTip(tr("Delete node"));
    m_btnDelete->setIcon(FromTheme(VThemeIcon::EditDelete));
    m_btnDelete->setEnabled(false);
    toolbarLayout->addWidget(m_btnDelete);

    mainLayout->addLayout(toolbarLayout);

    // ── Node list ──────────────────────────────────────────────────────
    m_listWidget = new QListWidget();
    m_listWidget->setDragDropMode(QAbstractItemView::InternalMove);
    m_listWidget->setAlternatingRowColors(true);
    mainLayout->addWidget(m_listWidget);

    // ── Navigation ─────────────────────────────────────────────────────
    auto *navLayout = new QHBoxLayout();

    m_btnTop = new QToolButton();
    m_btnTop->setToolTip(tr("Move on top"));
    m_btnTop->setIcon(QIcon::fromTheme(QStringLiteral("go-top")));
    m_btnTop->setEnabled(false);
    navLayout->addWidget(m_btnTop);

    m_btnUp = new QToolButton();
    m_btnUp->setToolTip(tr("Move up"));
    m_btnUp->setIcon(FromTheme(VThemeIcon::GoUp));
    m_btnUp->setEnabled(false);
    navLayout->addWidget(m_btnUp);

    m_btnDown = new QToolButton();
    m_btnDown->setToolTip(tr("Move down"));
    m_btnDown->setIcon(FromTheme(VThemeIcon::GoDown));
    m_btnDown->setEnabled(false);
    navLayout->addWidget(m_btnDown);

    m_btnBottom = new QToolButton();
    m_btnBottom->setToolTip(tr("Move on bottom"));
    m_btnBottom->setIcon(QIcon::fromTheme(QStringLiteral("go-bottom")));
    m_btnBottom->setEnabled(false);
    navLayout->addWidget(m_btnBottom);

    navLayout->addStretch();
    mainLayout->addLayout(navLayout);

    // ── Help / validation label ────────────────────────────────────────
    m_helpLabel = new QLabel();
    m_helpLabel->setTextFormat(Qt::RichText);
    m_helpLabel->setWordWrap(true);
    mainLayout->addWidget(m_helpLabel);

    // ── Button box ─────────────────────────────────────────────────────
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(m_buttonBox);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::ConnectSignals()
{
    // List signals
    m_listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_listWidget, &QListWidget::customContextMenuRequested, this, &DialogMainPath::ShowContextMenu);
    connect(m_listWidget, &QListWidget::itemSelectionChanged, this, &DialogMainPath::OnSelectionChanged);
    connect(m_listWidget->model(), &QAbstractItemModel::rowsMoved, this, [this]() { UpdateValidation(); });

    // Toolbar buttons
    connect(m_btnReverse, &QToolButton::toggled, this, &DialogMainPath::OnReverseToggled);
    connect(m_btnExcluded, &QToolButton::toggled, this, &DialogMainPath::OnExcludedToggled);
    connect(m_btnTurnPoint, &QToolButton::toggled, this, &DialogMainPath::OnTurnPointToggled);
    connect(m_btnCheckUniqueness, &QToolButton::toggled, this, &DialogMainPath::OnCheckUniquenessToggled);
    connect(m_btnPassmark, &QToolButton::toggled, this, &DialogMainPath::OnPassmarkToggled);
    connect(m_btnDuplicateNode, &QToolButton::clicked, this, &DialogMainPath::OnDuplicateNode);
    connect(m_btnDelete, &QToolButton::clicked, this, &DialogMainPath::OnDeleteNode);

    // Navigation buttons
    connect(m_btnTop, &QToolButton::clicked, this, &DialogMainPath::OnMoveTop);
    connect(m_btnUp, &QToolButton::clicked, this, &DialogMainPath::OnMoveUp);
    connect(m_btnDown, &QToolButton::clicked, this, &DialogMainPath::OnMoveDown);
    connect(m_btnBottom, &QToolButton::clicked, this, &DialogMainPath::OnMoveBottom);

    // Dialog buttons
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, [this]()
    {
        if (MainPathIsValid())
        {
            accept();
        }
        else
        {
            QMessageBox::warning(this, tr("Invalid Path"),
                                 tr("The main path is not valid. Please fix the errors before saving."));
        }
    });
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::SetNodes(const QVector<VPieceNode> &nodes)
{
    const QSignalBlocker blocker(m_listWidget);
    m_listWidget->clear();
    for (const auto &node : nodes)
    {
        const QString name = GetNodeName(m_data, node, true);
        auto *item = new QListWidgetItem(name);
        item->setFont(NodeFont(item->font(), node.IsExcluded()));
        item->setData(Qt::UserRole, QVariant::fromValue(node));
        m_listWidget->addItem(item);
    }
    EnableControls();
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogMainPath::GetNodes() const -> QVector<VPieceNode>
{
    QVector<VPieceNode> nodes;
    nodes.reserve(m_listWidget->count());
    for (int i = 0; i < m_listWidget->count(); ++i)
    {
        nodes.append(qvariant_cast<VPieceNode>(m_listWidget->item(i)->data(Qt::UserRole)));
    }
    return nodes;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnSelectionChanged()
{
    EnableControls();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnReverseToggled(bool checked)
{
    const int row = m_listWidget->currentRow();
    if (row < 0)
    {
        return;
    }

    auto *item = m_listWidget->item(row);
    auto node = qvariant_cast<VPieceNode>(item->data(Qt::UserRole));

    if (node.GetTypeTool() == Tool::NodePoint)
    {
        return;
    }

    node.SetReverse(checked);
    item->setData(Qt::UserRole, QVariant::fromValue(node));
    item->setText(GetNodeName(m_data, node, true));
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnExcludedToggled(bool checked)
{
    const int row = m_listWidget->currentRow();
    if (row < 0)
    {
        return;
    }

    auto *item = m_listWidget->item(row);
    auto node = qvariant_cast<VPieceNode>(item->data(Qt::UserRole));

    node.SetExcluded(checked);
    item->setData(Qt::UserRole, QVariant::fromValue(node));
    item->setText(GetNodeName(m_data, node, true));
    item->setFont(NodeFont(item->font(), checked));
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnTurnPointToggled(bool checked)
{
    const int row = m_listWidget->currentRow();
    if (row < 0)
    {
        return;
    }

    auto *item = m_listWidget->item(row);
    auto node = qvariant_cast<VPieceNode>(item->data(Qt::UserRole));

    if (node.GetTypeTool() != Tool::NodePoint)
    {
        return;
    }

    node.SetTurnPoint(checked);
    item->setData(Qt::UserRole, QVariant::fromValue(node));
    item->setText(GetNodeName(m_data, node, true));
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnCheckUniquenessToggled(bool checked)
{
    const int row = m_listWidget->currentRow();
    if (row < 0)
    {
        return;
    }

    auto *item = m_listWidget->item(row);
    auto node = qvariant_cast<VPieceNode>(item->data(Qt::UserRole));

    if (node.GetTypeTool() != Tool::NodePoint)
    {
        return;
    }

    node.SetCheckUniqueness(checked);
    item->setData(Qt::UserRole, QVariant::fromValue(node));
    item->setText(GetNodeName(m_data, node, true));
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnPassmarkToggled(bool checked)
{
    const int row = m_listWidget->currentRow();
    if (row < 0)
    {
        return;
    }

    auto *item = m_listWidget->item(row);
    auto node = qvariant_cast<VPieceNode>(item->data(Qt::UserRole));

    if (node.GetTypeTool() != Tool::NodePoint)
    {
        return;
    }

    node.SetPassmark(checked);
    item->setData(Qt::UserRole, QVariant::fromValue(node));
    item->setText(GetNodeName(m_data, node, true));
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnDuplicateNode()
{
    const int row = m_listWidget->currentRow();
    if (row < 0)
    {
        return;
    }

    auto node = qvariant_cast<VPieceNode>(m_listWidget->item(row)->data(Qt::UserRole));
    node.SetExcluded(true);

    auto *item = new QListWidgetItem(GetNodeName(m_data, node, true));
    item->setFont(NodeFont(item->font(), true));
    item->setData(Qt::UserRole, QVariant::fromValue(node));
    m_listWidget->addItem(item);
    m_listWidget->setCurrentRow(m_listWidget->count() - 1);
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnDeleteNode()
{
    const int row = m_listWidget->currentRow();
    if (row < 0)
    {
        return;
    }

    delete m_listWidget->item(row);
    EnableControls();
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnMoveTop()
{
    DialogTool::MoveListRowTop(m_listWidget);
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnMoveUp()
{
    DialogTool::MoveListRowUp(m_listWidget);
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnMoveDown()
{
    DialogTool::MoveListRowDown(m_listWidget);
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::OnMoveBottom()
{
    DialogTool::MoveListRowBottom(m_listWidget);
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::ShowContextMenu(const QPoint &pos)
{
    const int row = m_listWidget->currentRow();
    if (m_listWidget->count() == 0 || row == -1 || row >= m_listWidget->count())
    {
        return;
    }

    QListWidgetItem *rowItem = m_listWidget->item(row);
    auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

    QMenu menu;
    QHash<int, QAction *> contextMenu;

    auto SelectPassmarkLineType = [&rowNode, rowItem, this](PassmarkLineType type)
    {
        rowNode.SetPassmark(true);
        rowNode.SetPassmarkLineType(type);
        rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
        rowItem->setText(GetNodeName(m_data, rowNode, true));
    };

    if (rowNode.GetTypeTool() != Tool::NodePoint)
    {
        QAction *actionReverse = menu.addAction(FromTheme(VThemeIcon::ListRemove), tr("Reverse"));
        actionReverse->setCheckable(true);
        actionReverse->setChecked(rowNode.GetReverse());
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::Reverse), actionReverse);

        QAction *actionDuplicate = menu.addAction(FromTheme(VThemeIcon::DuplicateNode), tr("Duplicate"));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::DuplicateNode), actionDuplicate);
    }
    else
    {
        QMenu *passmarkSubmenu = menu.addMenu(FromTheme(VThemeIcon::AddPlacemark), tr("Passmark"));

        QAction *actionNonePassmark = passmarkSubmenu->addAction(tr("None"));
        actionNonePassmark->setCheckable(true);
        actionNonePassmark->setChecked(!rowNode.IsPassmark());
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::NonePassmark), actionNonePassmark);

        auto AddPassmarkTypeAction = [passmarkSubmenu, &rowNode](const QString &name, PassmarkLineType lineType)
        {
            QAction *action = passmarkSubmenu->addAction(name);
            action->setCheckable(true);
            action->setChecked(rowNode.IsPassmark() && lineType == rowNode.GetPassmarkLineType());
            return action;
        };

        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::OneLine),
                           AddPassmarkTypeAction(tr("One line"), PassmarkLineType::OneLine));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::TwoLines),
                           AddPassmarkTypeAction(tr("Two lines"), PassmarkLineType::TwoLines));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::ThreeLines),
                           AddPassmarkTypeAction(tr("Three lines"), PassmarkLineType::ThreeLines));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::TMark),
                           AddPassmarkTypeAction(tr("T mark"), PassmarkLineType::TMark));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::ExternalVMark),
                           AddPassmarkTypeAction(tr("External V mark"), PassmarkLineType::ExternalVMark));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::InternalVMark),
                           AddPassmarkTypeAction(tr("Internal V mark"), PassmarkLineType::InternalVMark));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::UMark),
                           AddPassmarkTypeAction(tr("U mark"), PassmarkLineType::UMark));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::BoxMark),
                           AddPassmarkTypeAction(tr("Box mark"), PassmarkLineType::BoxMark));
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::CheckMark),
                           AddPassmarkTypeAction(tr("Check mark"), PassmarkLineType::CheckMark));

        QAction *actionUniqueness = menu.addAction(FromTheme(VThemeIcon::DrawStar), tr("Check uniqueness"));
        actionUniqueness->setCheckable(true);
        actionUniqueness->setChecked(rowNode.IsCheckUniqueness());
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::Uniqueness), actionUniqueness);

        QAction *actionTurnPoint = menu.addAction(FromTheme(VThemeIcon::SnapNodesRotationCenter), tr("Turn point"));
        actionTurnPoint->setCheckable(true);
        actionTurnPoint->setChecked(rowNode.IsTurnPoint());
        contextMenu.insert(static_cast<int>(MainPathContextMenuOption::TurnPoint), actionTurnPoint);
    }

    QAction *actionExcluded = menu.addAction(FromTheme(VThemeIcon::FormatTextStrikethrough), tr("Excluded"));
    actionExcluded->setCheckable(true);
    actionExcluded->setChecked(rowNode.IsExcluded());
    contextMenu.insert(static_cast<int>(MainPathContextMenuOption::Excluded), actionExcluded);

    QAction *actionDelete = menu.addAction(FromTheme(VThemeIcon::EditDelete), tr("Delete"));
    actionDelete->setShortcut(QKeySequence::Delete);
    contextMenu.insert(static_cast<int>(MainPathContextMenuOption::Delete), actionDelete);

    QAction *selectedAction = menu.exec(m_listWidget->viewport()->mapToGlobal(pos));
    auto selectedOption = static_cast<MainPathContextMenuOption>(
        contextMenu.key(selectedAction, static_cast<int>(MainPathContextMenuOption::NoSelection)));

    Q_STATIC_ASSERT_X(static_cast<int>(MainPathContextMenuOption::LAST_ONE_DO_NOT_USE) == 17,
                      "Not all options were handled.");

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (selectedOption)
    {
        case MainPathContextMenuOption::NoSelection:
            return;
        case MainPathContextMenuOption::Reverse:
            rowNode.SetReverse(not rowNode.GetReverse());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(m_data, rowNode, true));
            break;
        case MainPathContextMenuOption::NonePassmark:
            rowNode.SetPassmark(false);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(m_data, rowNode, true));
            break;
        case MainPathContextMenuOption::OneLine:
            SelectPassmarkLineType(PassmarkLineType::OneLine);
            break;
        case MainPathContextMenuOption::TwoLines:
            SelectPassmarkLineType(PassmarkLineType::TwoLines);
            break;
        case MainPathContextMenuOption::ThreeLines:
            SelectPassmarkLineType(PassmarkLineType::ThreeLines);
            break;
        case MainPathContextMenuOption::TMark:
            SelectPassmarkLineType(PassmarkLineType::TMark);
            break;
        case MainPathContextMenuOption::ExternalVMark:
            SelectPassmarkLineType(PassmarkLineType::ExternalVMark);
            break;
        case MainPathContextMenuOption::InternalVMark:
            SelectPassmarkLineType(PassmarkLineType::InternalVMark);
            break;
        case MainPathContextMenuOption::UMark:
            SelectPassmarkLineType(PassmarkLineType::UMark);
            break;
        case MainPathContextMenuOption::BoxMark:
            SelectPassmarkLineType(PassmarkLineType::BoxMark);
            break;
        case MainPathContextMenuOption::CheckMark:
            SelectPassmarkLineType(PassmarkLineType::CheckMark);
            break;
        case MainPathContextMenuOption::Uniqueness:
            rowNode.SetCheckUniqueness(not rowNode.IsCheckUniqueness());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(m_data, rowNode, true));
            break;
        case MainPathContextMenuOption::TurnPoint:
            rowNode.SetTurnPoint(not rowNode.IsTurnPoint());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(m_data, rowNode, true));
            break;
        case MainPathContextMenuOption::Excluded:
            rowNode.SetExcluded(not rowNode.IsExcluded());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(m_data, rowNode, true));
            rowItem->setFont(NodeFont(rowItem->font(), rowNode.IsExcluded()));
            break;
        case MainPathContextMenuOption::DuplicateNode:
        {
            VPieceNode duplicate = rowNode;
            duplicate.SetExcluded(true);

            auto *item = new QListWidgetItem(GetNodeName(m_data, duplicate, true));
            item->setFont(NodeFont(item->font(), true));
            item->setData(Qt::UserRole, QVariant::fromValue(duplicate));
            m_listWidget->addItem(item);
            break;
        }
        case MainPathContextMenuOption::Delete:
            delete m_listWidget->item(row);
            break;
        case MainPathContextMenuOption::LAST_ONE_DO_NOT_USE:
            Q_UNREACHABLE();
            break;
    };

    QT_WARNING_POP

    EnableControls();
    UpdateValidation();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::EnableControls()
{
    // ── Move buttons ───────────────────────────────────────────────────
    m_btnTop->setEnabled(false);
    m_btnUp->setEnabled(false);
    m_btnDown->setEnabled(false);
    m_btnBottom->setEnabled(false);

    const int row = m_listWidget->currentRow();
    const int count = m_listWidget->count();

    if (count >= 2 && row >= 0)
    {
        if (row == 0)
        {
            m_btnDown->setEnabled(true);
            m_btnBottom->setEnabled(true);
        }
        else if (row == count - 1)
        {
            m_btnTop->setEnabled(true);
            m_btnUp->setEnabled(true);
        }
        else
        {
            m_btnTop->setEnabled(true);
            m_btnUp->setEnabled(true);
            m_btnDown->setEnabled(true);
            m_btnBottom->setEnabled(true);
        }
    }

    // ── Option buttons ─────────────────────────────────────────────────
    auto SetChecked = [](QToolButton *btn, bool checked)
    {
        const QSignalBlocker blocker(btn);
        btn->setChecked(checked);
    };

    m_btnReverse->setEnabled(false);
    m_btnExcluded->setEnabled(false);
    m_btnTurnPoint->setEnabled(false);
    m_btnCheckUniqueness->setEnabled(false);
    m_btnPassmark->setEnabled(false);
    m_btnDuplicateNode->setEnabled(false);
    m_btnDelete->setEnabled(false);

    SetChecked(m_btnReverse, false);
    SetChecked(m_btnExcluded, false);
    SetChecked(m_btnTurnPoint, false);
    SetChecked(m_btnCheckUniqueness, false);
    SetChecked(m_btnPassmark, false);

    if (row < 0)
    {
        return;
    }

    const auto *item = m_listWidget->item(row);
    auto rowNode = qvariant_cast<VPieceNode>(item->data(Qt::UserRole));

    if (rowNode.GetTypeTool() != Tool::NodePoint)
    {
        m_btnReverse->setEnabled(true);
        SetChecked(m_btnReverse, rowNode.GetReverse());
        m_btnDuplicateNode->setEnabled(true);
    }
    else
    {
        m_btnPassmark->setEnabled(true);
        SetChecked(m_btnPassmark, rowNode.IsPassmark());

        m_btnCheckUniqueness->setEnabled(true);
        SetChecked(m_btnCheckUniqueness, rowNode.IsCheckUniqueness());

        m_btnTurnPoint->setEnabled(true);
        SetChecked(m_btnTurnPoint, rowNode.IsTurnPoint());
    }

    m_btnExcluded->setEnabled(true);
    SetChecked(m_btnExcluded, rowNode.IsExcluded());

    m_btnDelete->setEnabled(true);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMainPath::UpdateValidation()
{
    MainPathIsValid();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogMainPath::MainPathIsValid() const -> bool
{
    const QVector<VPieceNode> nodes = GetNodes();

    // Count non-excluded point nodes
    int pointCount = 0;
    for (const auto &node : nodes)
    {
        if (node.GetTypeTool() == Tool::NodePoint && not node.IsExcluded())
        {
            pointCount++;
        }
    }

    if (pointCount < 3)
    {
        m_helpLabel->setText(DialogWarningIcon(m_helpLabel) + tr("You need more points!"));
        return false;
    }

    if (not MainPathIsClockwise())
    {
        m_helpLabel->setText(DialogWarningIcon(m_helpLabel) +
                             tr("You have to choose points in a clockwise direction!"));
        return false;
    }

    QString error;
    if (FirstPointEqualLast(m_listWidget, m_data, error))
    {
        m_helpLabel->setText(QStringLiteral("%1%2 %3").arg(DialogWarningIcon(m_helpLabel),
                                                            tr("First point cannot be equal to the last point!"), error));
        return false;
    }

    error.clear();
    if (DoublePoints(m_listWidget, m_data, error))
    {
        m_helpLabel->setText(QStringLiteral("%1%2 %3").arg(DialogWarningIcon(m_helpLabel),
                                                            tr("You have double points!"), error));
        return false;
    }

    error.clear();
    if (DoubleCurves(m_listWidget, m_data, error))
    {
        m_helpLabel->setText(QStringLiteral("%1%2 %3").arg(DialogWarningIcon(m_helpLabel),
                                                            tr("The same curve repeats twice!"), error));
        return false;
    }

    if (not EachPointLabelIsUnique(m_listWidget))
    {
        m_helpLabel->setText(DialogWarningIcon(m_helpLabel) +
                             tr("Each point in the path must be unique!"));
        return false;
    }

    error.clear();
    if (InvalidSegment(m_listWidget, m_data, error))
    {
        m_helpLabel->setText(QStringLiteral("%1%2 %3").arg(DialogWarningIcon(m_helpLabel),
                                                            tr("Invalid segment!"), error));
        return false;
    }

    m_helpLabel->setText(tr("Ready!"));
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogMainPath::MainPathIsClockwise() const -> bool
{
    QVector<VLayoutPoint> layoutPoints;
    const QVector<VPieceNode> nodes = GetNodes();

    // Build a VPiecePath to compute points
    VPiecePath path{PiecePathType::PiecePath};
    path.SetNodes(nodes);

    try
    {
        layoutPoints = path.PathPoints(m_data);
    }
    catch (const VExceptionBadId &)
    {
        return false;
    }

    QVector<QPointF> points;
    CastTo(layoutPoints, points);

    if (points.count() < 3)
    {
        return false;
    }

    const qreal res = VPiece::SumTrapezoids(points);
    return res < 0;
}
