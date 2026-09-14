/************************************************************************
 **
 **  @file   owidgetoutputs.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   14/09/2026
 **
 **  @brief  Implementation of the "Outputs" dock content (Export mode).
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

#include "owidgetoutputs.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QIcon>
#include <QPainter>
#include <QPushButton>
#include <QSize>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "../vtheme/vtheme.h"

namespace
{

constexpr int kIconSize = 20;

// Table columns: active dot + name on the left, action icons on the right.
// Same seven columns as the Size Ranges dock — "Refresh" replaces its
// "Duplicate" (spec DockOutputs §7.2).
constexpr int kStatus = 0;
constexpr int kName = 1;
constexpr int kFavorite = 2;
constexpr int kEdit = 3;
constexpr int kRefresh = 4;
constexpr int kExport = 5;
constexpr int kDelete = 6;

// The three groups the dock opens with come from the catalogue below (Populate) — the
// destinations of the Export mode, which the user is free to rename or delete.


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
// (SubtiliteCode §26); clearing the icon here lets us paint it centered.
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

} // namespace

//---------------------------------------------------------------------------------------------------------------------
OWidgetOutputs::OWidgetOutputs(QWidget *parent)
  : QWidget(parent)
{
    BuildUi();

    // The dock stays open across theme toggles — the icons must follow.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, [this]() {
        UpdateButtonIcons();
        Populate();
    });
}

//---------------------------------------------------------------------------------------------------------------------
QPushButton *OWidgetOutputs::MakeIconButton(const QString &objectName, const QString &iconBase)
{
    auto *button = new QPushButton(this);
    button->setObjectName(objectName);
    button->setIconSize(QSize(kIconSize, kIconSize));
    button->setFocusPolicy(Qt::NoFocus);
    button->setAutoDefault(false);
    button->installEventFilter(this);
    m_iconButtons.append({button, iconBase, QIcon(), QIcon()});
    return button;
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::BuildUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(6);

    // ── "Add an output" ──
    // The only control above the list. There are no groups and no move buttons: a
    // generation is not kept beyond the session, so there is nothing to file it under
    // and nothing to reorder it against (spec DockOutputs §10).
    m_addOutputButton = MakeIconButton(QStringLiteral("addOutputButton"),
                                       QStringLiteral("stack-plus-fill"));
    m_addOutputButton->setText(tr("Add an output"));
    rootLayout->addWidget(m_addOutputButton, 0, Qt::AlignHCenter);

    // ── Outputs list ──
    m_table = new QTableWidget(0, 7, this);
    m_table->setObjectName(QStringLiteral("outputsTable"));
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);

    m_table->horizontalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(kName, QHeaderView::Stretch);
    m_table->setColumnWidth(kStatus, 36);
    m_table->setColumnWidth(kFavorite, 44);
    m_table->setColumnWidth(kEdit, 44);
    m_table->setColumnWidth(kRefresh, 44);
    m_table->setColumnWidth(kExport, 44);
    m_table->setColumnWidth(kDelete, 44);

    // Compact fixed-height rows — the list never stretches rows vertically.
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(36);

    m_table->setIconSize(QSize(kIconSize, kIconSize));
    for (int col : {kStatus, kFavorite, kEdit, kRefresh, kExport, kDelete})
    {
        m_table->setItemDelegateForColumn(col, new IconDelegate(m_table));
    }

    rootLayout->addWidget(m_table, 1);

    UpdateButtonIcons();
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::UpdateButtonIcons()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);

    for (IconButton &entry : m_iconButtons)
    {
        // Normal: current-theme variant. Hover: always the dark variant — the
        // hovered button background is an accent, so a light glyph keeps the
        // contrast in both themes.
        entry.normal = QIcon(IconResPath(entry.baseName, dark));
        entry.hover = QIcon(IconResPath(entry.baseName, true));
        entry.button->setIcon(entry.normal);
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool OWidgetOutputs::eventFilter(QObject *watched, QEvent *event)
{
    for (const IconButton &entry : m_iconButtons)
    {
        if (watched != entry.button)
        {
            continue;
        }

        if (event->type() == QEvent::Enter)
        {
            entry.button->setIcon(entry.hover);
        }
        else if (event->type() == QEvent::Leave || event->type() == QEvent::EnabledChange)
        {
            entry.button->setIcon(entry.normal);
        }
        break;
    }
    return QWidget::eventFilter(watched, event);
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::SetOutputs(const QVector<OExportOutputRow> &outputs)
{
    m_outputs = outputs;
    // A pure setter, like SetTableFilter() on the Size Ranges dock: the caller follows
    // with UpdateList(), and a theme change or RetranslateUI re-pops from what is stored.
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::UpdateList(quint32 activeOutputId)
{
    m_activeOutputId = activeOutputId;
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::Populate()
{
    if (m_table == nullptr)
    {
        return;
    }

    m_table->setRowCount(0);

    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    int row = 0;

    // ── The generations ──
    // A flat list: a generation is not kept beyond the session, so there is nothing to
    // file it under and nothing to reorder it against (spec DockOutputs §10).
    for (const OExportOutputRow &output : m_outputs)
    {
        m_table->insertRow(row);

        const bool active = (output.id == m_activeOutputId);

        auto *statusItem = new QTableWidgetItem(
            MakeCellIcon(active ? QStringLiteral("check-circle-fill") : QStringLiteral("circle"), dark),
            QString());
        statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
        statusItem->setData(Qt::UserRole, output.id); // the row's identity, never its index
        if (active)
        {
            statusItem->setToolTip(tr("Shown in the canvas"));
        }
        m_table->setItem(row, kStatus, statusItem);

        // No Name field exists yet, so the name is not editable.
        auto *nameItem = new QTableWidgetItem(output.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->setItem(row, kName, nameItem);

        // The action icons belong to the shell: drawn, and inert.
        auto *favoriteItem = new QTableWidgetItem(MakeCellIcon(QStringLiteral("star"), dark), QString());
        favoriteItem->setFlags(favoriteItem->flags() & ~Qt::ItemIsEditable);
        favoriteItem->setToolTip(tr("Add to favorites"));
        m_table->setItem(row, kFavorite, favoriteItem);

        auto *editItem = new QTableWidgetItem(MakeCellIcon(QStringLiteral("gear-six"), dark), QString());
        editItem->setFlags(editItem->flags() & ~Qt::ItemIsEditable);
        editItem->setToolTip(tr("Edit"));
        m_table->setItem(row, kEdit, editItem);

        auto *refreshItem =
            new QTableWidgetItem(MakeCellIcon(QStringLiteral("arrows-clockwise"), dark), QString());
        refreshItem->setFlags(refreshItem->flags() & ~Qt::ItemIsEditable);
        refreshItem->setToolTip(tr("Refresh"));
        m_table->setItem(row, kRefresh, refreshItem);

        auto *exportItem = new QTableWidgetItem(MakeCellIcon(QStringLiteral("download"), dark), QString());
        exportItem->setFlags(exportItem->flags() & ~Qt::ItemIsEditable);
        exportItem->setToolTip(tr("Export"));
        m_table->setItem(row, kExport, exportItem);

        auto *deleteItem = new QTableWidgetItem(MakeCellIcon(QStringLiteral("trash"), dark), QString());
        deleteItem->setFlags(deleteItem->flags() & ~Qt::ItemIsEditable);
        deleteItem->setToolTip(tr("Delete"));
        m_table->setItem(row, kDelete, deleteItem);

        ++row;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::RetranslateUI()
{
    m_addOutputButton->setText(tr("Add an output"));
    UpdateButtonIcons();
    Populate();
}
