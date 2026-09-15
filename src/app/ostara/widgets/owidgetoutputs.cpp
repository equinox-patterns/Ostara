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
#include <QTimer>
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

// Custom roles on the Name cell. Qt::UserRole carries the stable row identity; the
// previous-name role sits one slot above — see OnNameEdited() for why "previous" must be
// tracked in a role rather than read off item->text() at itemChanged time.
constexpr int kPreviousNameRole = Qt::UserRole + 1;

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
    connect(m_addOutputButton, &QPushButton::clicked, this, &OWidgetOutputs::OnAddClicked);
    rootLayout->addWidget(m_addOutputButton, 0, Qt::AlignHCenter);

    // ── Outputs list ──
    m_table = new QTableWidget(0, 7, this);
    m_table->setObjectName(QStringLiteral("outputsTable"));
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);

    // The Name column is editable: double-click or F2 opens the inline editor. Other
    // columns stay read-only — Qt::ItemIsSelectable is dropped on the Name item too,
    // so the only entry point is setEditTriggers (same pattern as Size Ranges).
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    connect(m_table, &QTableWidget::itemChanged, this, &OWidgetOutputs::OnNameEdited);

    // A click on the status column switches the canvas to that generation. The view is in
    // NoSelection, so nothing is selected — but cellClicked is emitted all the same, which
    // is exactly what the status column needs: a target, not a selection.
    connect(m_table, &QTableWidget::cellClicked, this, &OWidgetOutputs::OnCellClicked);

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

    // Populate rebuilds every row and rewrites every text — that fires itemChanged per
    // cell, which would route through OnNameEdited. Guard the whole rebuild so the
    // signal handler skips the validation/rename chain during a programmatic refresh.
    m_filling = true;

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

        // The name is editable inline — but ItemIsSelectable is dropped so a single
        // click does not select the cell (the editor would then open on any click,
        // overriding setEditTriggers). Editable + not-selectable is the canonical Qt
        // combo for "the cell can be edited, but only via the triggers I picked".
        auto *nameItem = new QTableWidgetItem(output.name);
        nameItem->setFlags(nameItem->flags() | Qt::ItemIsEditable);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsSelectable);
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        nameItem->setData(Qt::UserRole, output.id);              // the row's identity
        nameItem->setData(kPreviousNameRole, output.name);       // name BEFORE the next edit
        m_table->setItem(row, kName, nameItem);

        // Favorite and edit still belong to the shell: drawn, and inert.
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
        refreshItem->setData(Qt::UserRole, output.id); // the cell knows the row it acts on
        refreshItem->setToolTip(tr("Refresh"));
        m_table->setItem(row, kRefresh, refreshItem);

        auto *exportItem = new QTableWidgetItem(MakeCellIcon(QStringLiteral("download"), dark), QString());
        exportItem->setFlags(exportItem->flags() & ~Qt::ItemIsEditable);
        exportItem->setData(Qt::UserRole, output.id); // the cell knows the row it acts on
        exportItem->setToolTip(tr("Export"));
        m_table->setItem(row, kExport, exportItem);

        auto *deleteItem = new QTableWidgetItem(MakeCellIcon(QStringLiteral("trash"), dark), QString());
        deleteItem->setFlags(deleteItem->flags() & ~Qt::ItemIsEditable);
        deleteItem->setData(Qt::UserRole, output.id); // the cell knows the row it acts on
        deleteItem->setToolTip(tr("Delete"));
        m_table->setItem(row, kDelete, deleteItem);

        ++row;
    }

    m_filling = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::OnAddClicked()
{
    emit addRequested();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief OnCellClicked route a click on the columns that carry a command.
 *
 * Three columns are wired — status, refresh, delete — and the identity is read off the
 * cell that was clicked, not off a neighbour. The name column is edited inline, and a
 * single click there must not take the canvas away from the generation being renamed,
 * which is why it does not share the status branch the way the Size Ranges dock shares
 * its own two. The rest — favorite and edit — are still the shell (§7.2).
 */
void OWidgetOutputs::OnCellClicked(int row, int column)
{
    if (column != kStatus && column != kRefresh && column != kDelete && column != kExport)
    {
        return;
    }

    if (row < 0 || row >= m_table->rowCount())
    {
        return;
    }

    const QTableWidgetItem *cell = m_table->item(row, column);
    if (cell == nullptr)
    {
        return;
    }

    const quint32 id = cell->data(Qt::UserRole).toUInt();
    if (id == 0)
    {
        return; // a row without a generation behind it — defensive
    }

    switch (column)
    {
        case kStatus:
            emit activeRequested(id);
            break;
        case kRefresh:
            emit refreshRequested(id);
            break;
        case kDelete:
            emit deleteRequested(id);
            break;
        case kExport:
            emit exportRequested(id);
            break;
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::OnNameEdited(QTableWidgetItem *item)
{
    if (m_filling || item == nullptr || item->column() != kName)
    {
        return;
    }

    const quint32 id = item->data(Qt::UserRole).toUInt();
    if (id == 0)
    {
        return; // row not associated to a real generation — defensive
    }

    // The itemChanged signal is fired AFTER the editor commits — by the time we land
    // here, item->text() already holds the NEW name. The "previous name" role was set
    // during the last programmatic write (Populate, or the commit at the bottom of this
    // slot) and is untouched by the editor — so it carries what was on screen BEFORE
    // the user typed. Reading oldName off item->text() would silently break conflict
    // detection: any name typed that already belongs to another row would compare equal
    // to itself and slip through.
    const QString oldName = item->data(kPreviousNameRole).toString();
    const QString newName = item->text().trimmed();

    if (newName.isEmpty())
    {
        // Empty — rollback to the stored previous name (which is what the user saw before
        // she started editing) and flash the cell. The role is rewritten too, so the next
        // edit opens on the right baseline.
        m_filling = true;
        item->setText(oldName);
        item->setData(kPreviousNameRole, oldName);
        m_filling = false;
        FlashNameError(item);
        return;
    }

    if (newName == oldName)
    {
        // Unchanged (post-trim). Nothing to do, nothing to flash: rolling back to the same
        // value would just re-emit itemChanged for no reason. Re-sync the role and leave.
        m_filling = true;
        item->setData(kPreviousNameRole, oldName);
        m_filling = false;
        return;
    }

    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        if (row == item->row())
        {
            continue;
        }
        const QTableWidgetItem *other = m_table->item(row, kName);
        if (other != nullptr && other->text() == newName)
        {
            // Conflict with another generation — rollback to the previous name and flash.
            m_filling = true;
            item->setText(oldName);
            item->setData(kPreviousNameRole, oldName);
            m_filling = false;
            FlashNameError(item);
            return;
        }
    }

    // Accepted — commit the new text under the filling guard so itemChanged does not
    // re-enter, then notify MainWindow. SetData(Qt::UserRole) is defensive: nothing else
    // should have rewritten it, but if a future delegate does, the identity survives.
    // kPreviousNameRole is rewritten to newName so the next edit opens on the right
    // baseline (otherwise the user would "go back" to a name two edits old).
    m_filling = true;
    item->setText(newName);
    item->setData(Qt::UserRole, id);
    item->setData(kPreviousNameRole, newName);
    m_filling = false;

    emit renameRequested(id, newName);
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::FlashNameError(QTableWidgetItem *item)
{
    if (item == nullptr)
    {
        return;
    }

    // The red flash is the only visual cue here — the rollback is silent and the cell
    // briefly tints to tell the user "the name was rejected". The exact colour comes
    // from palette tokens; this literal is a placeholder until the token exists.
    const QBrush original = item->background();
    item->setBackground(QBrush(QColor(0xE5, 0x3E, 0x3E)));
    QTimer::singleShot(600, this, [item, original]() {
        if (item != nullptr)
        {
            item->setBackground(original);
        }
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetOutputs::RetranslateUI()
{
    m_addOutputButton->setText(tr("Add an output"));
    UpdateButtonIcons();
    Populate();
}
