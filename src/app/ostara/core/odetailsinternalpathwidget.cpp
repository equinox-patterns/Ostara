/************************************************************************
 **
 **  @file   odetailsinternalpathwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   08/09/2026
 **
 **  @brief  Implementation of the Internal paths list widget
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

#include "odetailsinternalpathwidget.h"

#include "../vpatterndb/vcontainer.h"
#include "../vtheme/vtheme.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QIcon>
#include <QPainter>
#include <QSize>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVariant>
#include <QVBoxLayout>

namespace
{

constexpr int kIconSize = 20;

// Table columns: active dot + name on the left, action icons on the right.
constexpr int kStatus = 0;
constexpr int kName = 1;
constexpr int kEdit = 2;
constexpr int kDelete = 3;

//---------------------------------------------------------------------------------------------------------------------
auto IconResPath(const QString &baseName, bool dark) -> QString
{
    return QStringLiteral(":/icons/%1/%2_%1.svg")
        .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), baseName);
}

//---------------------------------------------------------------------------------------------------------------------
// One themed icon: Normal uses the current theme variant, Selected always uses
// the dark variant (accent/hover backgrounds are dark in both themes).
auto MakeCellIcon(const QString &baseName, bool dark) -> QIcon
{
    QIcon icon;
    icon.addFile(IconResPath(baseName, dark), QSize(), QIcon::Normal);
    icon.addFile(IconResPath(baseName, true), QSize(), QIcon::Selected);
    return icon;
}

//---------------------------------------------------------------------------------------------------------------------
// Draws a centered cell icon. The base class would paint the icon left-aligned
// (SUBTLITES_CODE §16/§26); clearing the icon here lets us paint it centered.
class IconDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->icon = QIcon(); // Prevent the base paint() from drawing the icon left-aligned
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (icon.isNull())
        {
            return;
        }

        QIcon::Mode mode = QIcon::Normal;
        if (option.state & (QStyle::State_Selected | QStyle::State_MouseOver))
        {
            mode = QIcon::Selected;
        }

        const QSize iconSize(kIconSize, kIconSize);
        const QPoint center = option.rect.center();
        const QRect iconRect(center.x() - iconSize.width() / 2,
                             center.y() - iconSize.height() / 2, iconSize.width(), iconSize.height());
        icon.paint(painter, iconRect, Qt::AlignCenter, mode);
    }
};

// Restores the flag on scope exit (Populate and programmatic cell writes).
class FillGuard final
{
public:
    explicit FillGuard(bool &flag) : m_flag(flag)
    {
        m_flag = true;
    }

    ~FillGuard()
    {
        m_flag = false;
    }

private:
    bool &m_flag;
};

} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsInternalPathWidget::ODetailsInternalPathWidget(QWidget *parent)
  : QWidget(parent)
{
    BuildUi();
    ConnectControls();

    // The dock stays open across theme toggles — the icons must follow.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, [this]() {
        if (m_data != nullptr)
        {
            Populate(m_det, m_data);
        }
    });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsInternalPathWidget::BuildUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    m_table = new QTableWidget(0, 4, this);
    m_table->setObjectName(QStringLiteral("internalPathTable")); // QSS: base = $DockBody (matches the tab page)
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers); // edit goes through the gear-six dialog
    m_table->setAlternatingRowColors(true);

    m_table->horizontalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(kName, QHeaderView::Stretch);
    m_table->setColumnWidth(kStatus, 36);
    m_table->setColumnWidth(kEdit, 44);
    m_table->setColumnWidth(kDelete, 44);

    // Compact fixed-height rows — the list never stretches rows vertically.
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(36);

    m_table->setIconSize(QSize(kIconSize, kIconSize));
    for (int col : {kStatus, kEdit, kDelete})
    {
        m_table->setItemDelegateForColumn(col, new IconDelegate(m_table));
    }

    rootLayout->addWidget(m_table);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsInternalPathWidget::ConnectControls()
{
    // Route clicks: activate (dot/name), edit, delete.
    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int column) {
        const quint32 pathId = PathIdAtRow(row);
        if (pathId == 0)
        {
            return;
        }
        switch (column)
        {
            case kStatus:
            case kName: // click on the row (active dot or name) → radio toggle
                if (m_activePathId == pathId)
                {
                    // Re-click on the active row: toggle off (dot back to circle).
                    m_activePathId = 0;
                    UpdateStatusColumn();
                    emit InternalPathActivated(0); // id 0 → the parent drops the canvas highlight
                }
                else
                {
                    m_activePathId = pathId;
                    UpdateStatusColumn();
                    emit InternalPathActivated(pathId);
                }
                break;
            case kEdit:
                emit EditRequested(pathId);
                break;
            case kDelete:
                emit DeleteRequested(pathId);
                break;
        }
    });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsInternalPathWidget::UpdateTableHeight()
{
    if (m_table == nullptr)
    {
        return;
    }
    // The item view paints its own base over any empty viewport area — on macOS the QSS
    // background-color of the view is not honored there (subtilité §13c/13g). Make the table
    // exactly as tall as its rows so no empty region exists: below the list the tab page
    // ($DockBody) shows through, and beyond a few paths the table scrolls internally.
    constexpr int kMaxVisibleRows = 6;
    const int rows = m_table->rowCount();
    const int visibleRows = qMin(rows, kMaxVisibleRows);
    const int rowHeight = m_table->verticalHeader()->defaultSectionSize();
    m_table->setFixedHeight(visibleRows * rowHeight);
    m_table->setVerticalScrollBarPolicy(rows > kMaxVisibleRows ? Qt::ScrollBarAlwaysOn
                                                               : Qt::ScrollBarAlwaysOff);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsInternalPathWidget::Populate(const VPiece &det, const VContainer *data)
{
    FillGuard guard(m_filling); // ignore programmatic writes while we rebuild the rows

    if (m_table == nullptr)
    {
        return;
    }

    m_table->setRowCount(0);
    m_pathIds.clear();

    if (data == nullptr)
    {
        return;
    }

    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);

    const QVector<quint32> pathIds = det.GetInternalPaths();
    m_pathIds.reserve(pathIds.size());

    int row = 0;
    for (quint32 pathId : pathIds)
    {
        if (!data->ContainsPiecePath(pathId))
        {
            continue; // stale id (removed by an undo/redo not yet flushed) — skip it
        }
        const VPiecePath path = data->GetPiecePath(pathId);

        m_table->insertRow(row);
        m_pathIds.append(pathId);

        const bool active = (m_activePathId == pathId);
        const QString statusBase =
            active ? QStringLiteral("check-circle-fill") : QStringLiteral("circle");
        auto *statusItem = new QTableWidgetItem(MakeCellIcon(statusBase, dark), QString());
        statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kStatus, statusItem);

        auto *nameItem = new QTableWidgetItem(path.GetName());
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kName, nameItem);

        auto *editItem = new QTableWidgetItem(MakeCellIcon("gear-six", dark), QString());
        editItem->setFlags(editItem->flags() & ~Qt::ItemIsEditable);
        editItem->setToolTip(tr("Edit"));
        m_table->setItem(row, kEdit, editItem);

        auto *deleteItem = new QTableWidgetItem(MakeCellIcon("trash", dark), QString());
        deleteItem->setFlags(deleteItem->flags() & ~Qt::ItemIsEditable);
        deleteItem->setToolTip(tr("Delete"));
        m_table->setItem(row, kDelete, deleteItem);

        ++row;
    }

    UpdateTableHeight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsInternalPathWidget::UpdateStatusColumn()
{
    if (m_table == nullptr)
    {
        return;
    }
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        QTableWidgetItem *statusItem = m_table->item(row, kStatus);
        if (statusItem == nullptr)
        {
            continue;
        }
        const quint32 pathId = PathIdAtRow(row);
        const QString statusBase =
            (pathId != 0 && m_activePathId == pathId) ? QStringLiteral("check-circle-fill")
                                                      : QStringLiteral("circle");
        statusItem->setIcon(MakeCellIcon(statusBase, dark));
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsInternalPathWidget::PathIdAtRow(int row) const -> quint32
{
    if (row < 0 || row >= m_pathIds.size())
    {
        return 0;
    }
    return m_pathIds.at(row);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsInternalPathWidget::Show(const VPiece &det, const VContainer *data)
{
    m_det = det;
    m_data = data;
    m_activePathId = 0; // a new piece context starts with no active path
    Populate(det, data);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsInternalPathWidget::Clear()
{
    m_det = VPiece();
    m_data = nullptr;
    m_pathIds.clear();
    m_activePathId = 0;
    if (m_table != nullptr)
    {
        m_table->setRowCount(0);
        UpdateTableHeight();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsInternalPathWidget::RefreshList(const VPiece &det, const VContainer *data) -> bool
{
    m_det = det;
    m_data = data;
    const quint32 previousActive = m_activePathId;
    Populate(det, data);
    if (previousActive != 0 && !m_pathIds.contains(previousActive))
    {
        m_activePathId = 0; // the active path was removed — parent must clear the highlight
        return true;
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsInternalPathWidget::HasActivePath() const -> bool
{
    return m_activePathId != 0;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsInternalPathWidget::Deactivate()
{
    m_activePathId = 0;
    UpdateStatusColumn();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsInternalPathWidget::RetranslateUI()
{
    if (m_data != nullptr)
    {
        Populate(m_det, m_data); // refresh tooltips / icons
    }
}
