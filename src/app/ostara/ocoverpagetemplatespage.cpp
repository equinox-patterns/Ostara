/************************************************************************
 **
 **  @file   ocoverpagetemplatespage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   18/09/2026
 **
 **  @brief  The list of saved cover page templates.
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

#include "ocoverpagetemplatespage.h"

#include "core/ocoverpagetemplatestorage.h"

#include "vabstractapplication.h"
#include "vcommonsettings.h"
#include "vtheme.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
/** The icons the rows are drawn with, in the theme's own light or dark variant. */
auto IconResPath(const QString &baseName, bool dark) -> QString
{
    return QStringLiteral(":/icons/%1/%2_%1.svg")
        .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), baseName);
}

auto AccentIconPath(const QString &baseName, bool dark) -> QString
{
    return QStringLiteral(":/icons/%1/%2_accent.svg")
        .arg(dark ? QStringLiteral("dark") : QStringLiteral("light"), baseName);
}

/**
 * @brief Centres the icon of a cell that carries nothing else.
 *
 * A QTableWidgetItem draws its icon against the left edge of its cell, which in a column forty
 * pixels wide reads as misaligned. The base paint is left to draw the selection and the
 * background, with the icon taken away, and the icon is then drawn in the middle.
 */
class IconCellDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->icon = QIcon(); // the base paint must not draw it left-aligned
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

        const QSize iconSize(20, 20);
        const QRect target(option.rect.center().x() - iconSize.width() / 2,
                           option.rect.center().y() - iconSize.height() / 2, iconSize.width(),
                           iconSize.height());
        icon.paint(painter, target, Qt::AlignCenter, mode);
    }
};

/** One icon in its two states: the theme's, and the dark one the row takes when pointed at. */
auto RowIcon(const QString &baseName, bool dark) -> QIcon
{
    QIcon icon;
    icon.addFile(IconResPath(baseName, dark), QSize(), QIcon::Normal);
    icon.addFile(IconResPath(baseName, true), QSize(), QIcon::Selected);
    return icon;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------

OCoverPageTemplatesPage::OCoverPageTemplatesPage(QWidget *parent) : QWidget(parent)
{
    SetupUI();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatesPage::SetupUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    m_title = new QLabel();
    QFont titleFont = m_title->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    m_title->setFont(titleFont);
    layout->addWidget(m_title);

    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;

    m_searchEdit = new QLineEdit();
    m_searchEdit->addAction(QIcon(IconResPath(QStringLiteral("magnifying-glass"), dark)),
                            QLineEdit::LeadingPosition);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &OCoverPageTemplatesPage::FilterTable);
    layout->addWidget(m_searchEdit);

    m_table = new QTableWidget();
    m_table->setObjectName(QStringLiteral("coverPageTemplatesTable"));
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({QString(), tr("Name"), QString(), QString()});
    // 0 — the favourite, 2 — edit, 3 — delete. Only the name has anything to say, so it is the
    // one that takes the room.
    m_table->setColumnWidth(0, 44);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(2, 44);
    m_table->setColumnWidth(3, 44);
    m_table->setIconSize(QSize(20, 20));

    QStyledItemDelegate *iconDelegate = new IconCellDelegate(m_table);
    m_table->setItemDelegateForColumn(0, iconDelegate);
    m_table->setItemDelegateForColumn(2, iconDelegate);
    m_table->setItemDelegateForColumn(3, iconDelegate);

    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(36);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // A double-click anywhere opens the row; the columns that carry a button keep their meaning
    // on a single click, which the double-click would otherwise swallow.
    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int) {
                if (row >= 0 && row < m_entries.size())
                {
                    emit OpenTemplate(m_entries.at(row).path);
                }
            });
    connect(m_table, &QTableWidget::cellClicked, this, &OCoverPageTemplatesPage::OnCellClicked);
    layout->addWidget(m_table, 1);

    m_openFolderBtn = new QPushButton();
    connect(m_openFolderBtn, &QPushButton::clicked, this, &OCoverPageTemplatesPage::OnOpenFolder);
    layout->addWidget(m_openFolderBtn, 0, Qt::AlignLeft);

    RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatesPage::RetranslateUI()
{
    m_title->setText(tr("Existing templates"));
    m_searchEdit->setPlaceholderText(tr("Search templates…"));
    m_openFolderBtn->setText(tr("Open folder"));
    PopulateTable();
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatesPage::Refresh()
{
    m_defaultName = VAbstractApplication::VApp()->Settings()->GetDefaultCoverPageTemplate();

    m_entries.clear();
    const QStringList paths =
        OCoverPageTemplateStorage::ScanDirectory(OCoverPageTemplateStorage::DefaultDirectory());
    m_entries.reserve(paths.size());
    for (const QString &path : paths)
    {
        Entry entry;
        // The identity is the base name, read off the file name and never out of the file:
        // listing a library must not mean opening every template in it.
        entry.name = QFileInfo(path).completeBaseName();
        entry.path = path;
        m_entries.append(entry);
    }

    PopulateTable();

    // The search is a property of the view, not of the library: refilling must not bring back
    // the rows it had filtered out.
    FilterTable(m_searchEdit->text());
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatesPage::PopulateTable()
{
    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;

    m_table->setRowCount(0);

    if (m_entries.isEmpty())
    {
        m_table->setRowCount(1);
        auto *item = new QTableWidgetItem(tr("No templates yet. Create one."));
        item->setFlags(Qt::NoItemFlags);
        m_table->setItem(0, 1, item);
        m_table->setSpan(0, 1, 1, 3);
        return;
    }

    m_table->setRowCount(static_cast<int>(m_entries.size()));
    for (int index = 0; index < m_entries.size(); ++index)
    {
        const Entry &entry = m_entries.at(index);
        const bool isDefault = (entry.name == m_defaultName);

        // The star. One favourite and no more: this is a choice between rows, never a property to
        // be ticked and unticked, so the row that is already the favourite does nothing.
        QIcon star;
        if (isDefault)
        {
            star.addFile(AccentIconPath(QStringLiteral("star-fill"), dark), QSize(), QIcon::Normal);
            star.addFile(IconResPath(QStringLiteral("star-fill"), true), QSize(), QIcon::Selected);
        }
        else
        {
            star.addFile(AccentIconPath(QStringLiteral("star"), dark), QSize(), QIcon::Normal);
            star.addFile(IconResPath(QStringLiteral("star"), true), QSize(), QIcon::Selected);
        }
        auto *starItem = new QTableWidgetItem(star, QString());
        starItem->setToolTip(isDefault ? tr("This is the default template")
                                       : tr("Make this the default template"));
        m_table->setItem(index, 0, starItem);

        auto *nameItem = new QTableWidgetItem(entry.name);
        nameItem->setData(Qt::UserRole, entry.path);
        m_table->setItem(index, 1, nameItem);

        auto *editItem = new QTableWidgetItem(RowIcon(QStringLiteral("gear-six"), dark), QString());
        editItem->setToolTip(tr("Edit"));
        m_table->setItem(index, 2, editItem);

        auto *deleteItem = new QTableWidgetItem(RowIcon(QStringLiteral("trash"), dark), QString());
        deleteItem->setToolTip(tr("Delete"));
        m_table->setItem(index, 3, deleteItem);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatesPage::FilterTable(const QString &filter)
{
    // The rows are hidden, never removed: a row's index is its index in m_entries, and the click
    // handler reads the entry back by that index.
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        const QTableWidgetItem *nameItem = m_table->item(row, 1);
        const bool matches =
            nameItem != nullptr && nameItem->text().contains(filter, Qt::CaseInsensitive);
        m_table->setRowHidden(row, !matches);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatesPage::OnOpenFolder()
{
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(OCoverPageTemplateStorage::DefaultDirectory()));
}

//---------------------------------------------------------------------------------------------------------------------

void OCoverPageTemplatesPage::OnCellClicked(int row, int column)
{
    if (row < 0 || row >= m_entries.size())
    {
        return;
    }

    const Entry &entry = m_entries.at(row);

    switch (column)
    {
        case 0: // the favourite
            if (entry.name != m_defaultName)
            {
                VAbstractApplication::VApp()->Settings()->SetDefaultCoverPageTemplate(entry.name);
                Refresh();
            }
            break;

        case 2: // edit
            emit OpenTemplate(entry.path);
            break;

        case 3: // delete
        {
            const QMessageBox::StandardButton answer = QMessageBox::question(
                this, tr("Confirm deletion"),
                tr("Are you sure you want to delete \"%1\"?\n\nThis action cannot be undone.")
                    .arg(entry.name),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
            {
                break;
            }

            if (QFile::remove(entry.path))
            {
                // The favourite is a name, and the name now names nothing.
                if (entry.name == m_defaultName)
                {
                    VAbstractApplication::VApp()->Settings()->SetDefaultCoverPageTemplate(QString());
                }
                Refresh();
            }
            else
            {
                QMessageBox::warning(this, tr("Error"), tr("Could not delete the file."));
            }
            break;
        }

        default:
            break;
    }
}
