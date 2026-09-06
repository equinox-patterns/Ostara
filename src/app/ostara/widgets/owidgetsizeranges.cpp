/************************************************************************
 **
 **  @file   owidgetsizeranges.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   05/09/2026
 **
 **  @brief  Implementation of the "Size Ranges" dock content.
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

#include "owidgetsizeranges.h"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QIcon>
#include <QMap>
#include <QPainter>
#include <QPushButton>
#include <QSize>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVariant>
#include <QVBoxLayout>

#include "MeasurementWorkspace/MeasurementLibraryScanner.h"
#include "core/ogradedfavorites.h"
#include "../vpatterndb/vcontainer.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vtheme/vtheme.h"

namespace
{

constexpr int kIconSize = 20;

// Table columns: active dot + name on the left, action icons on the right.
constexpr int kStatus = 0;
constexpr int kName = 1;
constexpr int kFavorite = 2;
constexpr int kEdit = 3;
constexpr int kDuplicate = 4;
constexpr int kExport = 5;
constexpr int kDelete = 6;

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
// (SUBTILITES_CODE.md §16/§26); clearing the icon here lets us paint it centered.
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
OWidgetSizeRanges::OWidgetSizeRanges(VContainer *pattern, QWidget *parent)
  : QWidget(parent),
    m_pattern(pattern)
{
    BuildUi();

    // The dock stays open across theme toggles — the icons must follow.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, [this]() {
        UpdateAddButtonIcon();
        Populate();
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeRanges::BuildUi()
{
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(6);

    // ── "Add a size range" trigger (opens the creation dialog) ──
    m_addButton = new QPushButton(this);
    m_addButton->setObjectName(QStringLiteral("addSizeRangeButton"));
    m_addButton->setText(tr("Add a size range"));
    m_addButton->setIconSize(QSize(kIconSize, kIconSize));
    m_addButton->setFocusPolicy(Qt::NoFocus);
    m_addButton->setAutoDefault(false);
    connect(m_addButton, &QPushButton::clicked, this, &OWidgetSizeRanges::addRequested);
    m_addButton->installEventFilter(this);
    rootLayout->addWidget(m_addButton, 0, Qt::AlignHCenter);

    // ── Ranges list ──
    m_table = new QTableWidget(0, 7, this);
    m_table->setObjectName(QStringLiteral("sizeRangesTable"));
    m_table->setShowGrid(false);
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    // Inline rename: double-click (or F2) on the name column only — every other
    // cell is non-editable (flags without Qt::ItemIsEditable).
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked
                             | QAbstractItemView::EditKeyPressed);
    m_table->setAlternatingRowColors(true);

    m_table->horizontalHeader()->setVisible(false);
    m_table->horizontalHeader()->setSectionResizeMode(kName, QHeaderView::Stretch);
    m_table->setColumnWidth(kStatus, 36);
    m_table->setColumnWidth(kFavorite, 44);
    m_table->setColumnWidth(kEdit, 44);
    m_table->setColumnWidth(kDuplicate, 44);
    m_table->setColumnWidth(kExport, 44);
    m_table->setColumnWidth(kDelete, 44);

    // Compact fixed-height rows — the list never stretches rows vertically.
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(36);

    m_table->setIconSize(QSize(kIconSize, kIconSize));
    for (int col : {kStatus, kFavorite, kEdit, kDuplicate, kExport, kDelete})
    {
        m_table->setItemDelegateForColumn(col, new IconDelegate(m_table));
    }

    // Route clicks: activate (dot/name), favorite toggle, edit, duplicate, delete.
    // Only the export column (kExport) is still visual-only.
    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int column) {
        if (row < 0 || row >= m_table->rowCount())
        {
            return;
        }
        QTableWidgetItem *nameItem = m_table->item(row, kName);
        if (nameItem == nullptr)
        {
            return;
        }
        const QString rangeName = nameItem->text();
        switch (column)
        {
            case kStatus:
            case kName: // single click on the row (active dot or name) → activate
                emit rangeActivated(rangeName);
                break;
            case kFavorite:
                ToggleFavorite(row, rangeName);
                break;
            case kEdit:
                emit editRequested(rangeName);
                break;
            case kDuplicate:
                emit duplicateRequested(rangeName);
                break;
            case kDelete:
                emit deleteRequested(rangeName);
                break;
            case kExport: // Export the range as a layered graded SVG (direct).
                emit exportRequested(rangeName);
                break;
        }
    });

    // Inline rename of the range name (double-click on the name cell).
    connect(m_table, &QTableWidget::itemChanged, this, &OWidgetSizeRanges::OnNameEdited);

    rootLayout->addWidget(m_table, 1);

    UpdateAddButtonIcon();
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeRanges::UpdateAddButtonIcon()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    // Normal: current-theme variant. Hover: always the dark variant — the hovered
    // button background is an accent, so a light glyph keeps the contrast (light
    // theme hovers over the same rule as the Measurement workspace).
    m_addNormalIcon = QIcon(IconResPath(QStringLiteral("stack-plus-fill"), dark));
    m_addHoverIcon = QIcon(IconResPath(QStringLiteral("stack-plus-fill"), true));
    m_addButton->setIcon(m_addNormalIcon);
}

//---------------------------------------------------------------------------------------------------------------------
bool OWidgetSizeRanges::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_addButton)
    {
        if (event->type() == QEvent::Enter)
        {
            m_addButton->setIcon(m_addHoverIcon);
        }
        else if (event->type() == QEvent::Leave || event->type() == QEvent::EnabledChange)
        {
            m_addButton->setIcon(m_addNormalIcon);
        }
    }
    return QWidget::eventFilter(watched, event);
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeRanges::UpdateList(const QString &activeRangeName)
{
    m_activeRangeName = activeRangeName;
    Populate();
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeRanges::Populate()
{
    FillGuard guard(m_filling); // ignore itemChanged while we rebuild the rows

    if (m_table == nullptr)
    {
        return;
    }

    m_table->setRowCount(0);

    if (m_pattern == nullptr)
    {
        return;
    }

    const auto *ranges = m_pattern->SizeRanges();
    if (ranges == nullptr || ranges->isEmpty())
    {
        return;
    }

    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    auto *settings = VAbstractApplication::VApp()->Settings();

    // Read each (table, axis) favorite set once instead of re-opening the
    // settings file per row (all rows of a table usually share the same pair).
    QMap<QString, QVariantMap> favoriteCache;

    QStringList names = ranges->keys();
    names.sort();

    m_table->setRowCount(static_cast<int>(names.size()));
    for (int i = 0; i < names.size(); ++i)
    {
        const QString &name = names.at(i);
        const bool active = (name == m_activeRangeName);

        // Active dot (left) + name
        const QString statusBase =
            active ? QStringLiteral("check-circle-fill") : QStringLiteral("circle");
        auto *statusItem = new QTableWidgetItem(MakeCellIcon(statusBase, dark), QString());
        statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(i, kStatus, statusItem);

        auto *nameItem = new QTableWidgetItem(name);
        nameItem->setData(Qt::UserRole, name); // original name, for the rename commit
        nameItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_table->setItem(i, kName, nameItem); // editable (double-click) — see OnNameEdited

        // Favorite (star) — reflects the software favorite store; disabled when the
        // range's table is not in the measurements library.
        auto *favoriteItem = new QTableWidgetItem();
        favoriteItem->setFlags(favoriteItem->flags() & ~Qt::ItemIsEditable);
        const OSizerange range = ranges->value(name);
        const QString favoriteKey = range.measurement + QLatin1Char('\n') + range.sizeAxis;
        if (not favoriteCache.contains(favoriteKey))
        {
            favoriteCache.insert(favoriteKey,
                                 settings->GetGradedFavorites(range.measurement, range.sizeAxis));
        }
        const bool favorite = favoriteCache.value(favoriteKey).contains(name);
        const bool inLibrary = MeasurementLibraryScanner::IsInLibrary(
            settings->GetPathMeasurementLibrary(), range.measurement);
        favoriteItem->setIcon(MakeCellIcon(favorite ? "star-fill" : "star", dark));
        if (inLibrary)
        {
            favoriteItem->setToolTip(favorite ? tr("Remove from favorites") : tr("Add to favorites"));
        }
        else
        {
            favoriteItem->setFlags(favoriteItem->flags() & ~Qt::ItemIsEnabled);
            favoriteItem->setToolTip(tr("Favorites need a measurements table from the library"));
        }
        m_table->setItem(i, kFavorite, favoriteItem);

        // Remaining action icons (right) — tooltips use literal tr() calls so lupdate
        // can extract them (a tr() on a variable would stay untranslated).
        auto *editItem = new QTableWidgetItem(MakeCellIcon("gear-six", dark), QString());
        editItem->setFlags(editItem->flags() & ~Qt::ItemIsEditable);
        editItem->setToolTip(tr("Edit"));
        m_table->setItem(i, kEdit, editItem);

        auto *duplicateItem = new QTableWidgetItem(MakeCellIcon("copy-fill", dark), QString());
        duplicateItem->setFlags(duplicateItem->flags() & ~Qt::ItemIsEditable);
        duplicateItem->setToolTip(tr("Duplicate"));
        m_table->setItem(i, kDuplicate, duplicateItem);

        auto *exportItem = new QTableWidgetItem(MakeCellIcon("file-svg", dark), QString());
        exportItem->setFlags(exportItem->flags() & ~Qt::ItemIsEditable);
        exportItem->setToolTip(tr("Export SVG"));
        m_table->setItem(i, kExport, exportItem);

        auto *deleteItem = new QTableWidgetItem(MakeCellIcon("trash", dark), QString());
        deleteItem->setFlags(deleteItem->flags() & ~Qt::ItemIsEditable);
        deleteItem->setToolTip(tr("Delete"));
        m_table->setItem(i, kDelete, deleteItem);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeRanges::OnNameEdited(QTableWidgetItem *item)
{
    if (m_filling || item == nullptr || item->column() != kName)
    {
        return;
    }

    const QString oldName = item->data(Qt::UserRole).toString();
    const QString newName = item->text().trimmed();

    // Empty or unchanged → revert (revert is a no-op when nothing changed).
    if (newName.isEmpty() || newName == oldName)
    {
        item->setText(oldName);
        return;
    }

    // A range name must stay unique within the pattern.
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        if (row == item->row())
        {
            continue;
        }
        QTableWidgetItem *other = m_table->item(row, kName);
        if (other != nullptr && other->text() == newName)
        {
            item->setText(oldName);
            return;
        }
    }

    // Normalize the committed text and remember the new name — guard the two
    // programmatic writes so they don't re-enter this handler.
    m_filling = true;
    item->setText(newName);
    item->setData(Qt::UserRole, newName);
    m_filling = false;

    emit renameRequested(oldName, newName);
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeRanges::ToggleFavorite(int row, const QString &rangeName)
{
    if (m_pattern == nullptr)
    {
        return;
    }

    QTableWidgetItem *favoriteItem = m_table != nullptr ? m_table->item(row, kFavorite) : nullptr;
    if (favoriteItem == nullptr || not (favoriteItem->flags() & Qt::ItemIsEnabled))
    {
        return; // table outside the library → no favorite persistence
    }

    const auto *ranges = m_pattern->SizeRanges();
    if (ranges == nullptr || !ranges->contains(rangeName))
    {
        return;
    }

    const OSizerange range = ranges->value(rangeName);
    auto *settings = VAbstractApplication::VApp()->Settings();
    if (OGradedFavorites::IsFavorite(settings, range.measurement, range.sizeAxis, rangeName))
    {
        OGradedFavorites::RemoveFavorite(settings, range);
    }
    else
    {
        OGradedFavorites::StoreFavorite(settings, range);
    }
    Populate(); // refresh the star state
}

//---------------------------------------------------------------------------------------------------------------------
void OWidgetSizeRanges::RetranslateUI()
{
    m_addButton->setText(tr("Add a size range"));
    UpdateAddButtonIcon();
    Populate();
}
