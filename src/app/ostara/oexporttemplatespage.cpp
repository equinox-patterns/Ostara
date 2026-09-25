/************************************************************************
 **
 **  @file   oexporttemplatespage.cpp
 **
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   17/09/2026
 **
 **  @brief  The list of saved templates, and what each of its rows does.
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

#include "oexporttemplatespage.h"

#include "core/oexporttemplatestorage.h"

#include "core/otranslation.h"
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

OExportTemplatesPage::OExportTemplatesPage(QWidget *parent) : QWidget(parent)
{
    SetupUI();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatesPage::SetupUI()
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

    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);

    m_searchEdit = new QLineEdit();
    m_searchEdit->addAction(QIcon(IconResPath(QStringLiteral("magnifying-glass"), dark)),
                            QLineEdit::LeadingPosition);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &OExportTemplatesPage::FilterTable);
    layout->addWidget(m_searchEdit);

    m_table = new QTableWidget();
    m_table->setObjectName(QStringLiteral("pdfTemplatesTable"));
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({QString(),   // 0 — the favourite
                                        tr("Name"),  // 1
                                        QString(),   // 2 — edit
                                        QString()}); // 3 — delete
    // ⚠️ **L'en-tête est posé une seule fois** : `RetranslateUI()` ne le rappelait pas, il restait
    // donc dans la langue du premier affichage. Le poseur le couvre.
    OTranslation::SetHeaderText(this, m_table, 1, "Name");

    m_table->setColumnWidth(0, 44);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setColumnWidth(2, 44);
    m_table->setColumnWidth(3, 44);

    m_table->setIconSize(QSize(20, 20));
    m_table->setItemDelegateForColumn(0, new IconCellDelegate(this));
    m_table->setItemDelegateForColumn(2, new IconCellDelegate(this));
    m_table->setItemDelegateForColumn(3, new IconCellDelegate(this));

    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(36);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Double-clicking anywhere in a row opens it, which is how a list is normally read. The
    // columns that carry a button keep their own meaning on a single click.
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (row >= 0 && row < m_entries.size())
        {
            emit OpenTemplate(m_entries.at(row).path);
        }
    });
    connect(m_table, &QTableWidget::cellClicked, this, &OExportTemplatesPage::OnCellClicked);
    layout->addWidget(m_table, 1);

    m_openFolderBtn = new QPushButton();
    connect(m_openFolderBtn, &QPushButton::clicked, this, &OExportTemplatesPage::OnOpenFolder);
    layout->addWidget(m_openFolderBtn, 0, Qt::AlignLeft);

    RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatesPage::RetranslateUI()
{
    m_title->setText(tr("Existing templates"));
    m_searchEdit->setPlaceholderText(tr("Search templates…"));
    m_openFolderBtn->setText(tr("Open folder"));
    PopulateTable();
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatesPage::Refresh()
{
    m_defaultName = VAbstractApplication::VApp()->Settings()->GetDefaultPdfTemplate();

    m_entries.clear();
    const QStringList paths =
        OExportTemplateStorage::ScanDirectory(OExportTemplateStorage::DefaultDirectory());
    m_entries.reserve(paths.size());
    for (const QString &path : paths)
    {
        Entry entry;
        // The base name alone identifies the template, and it is read off the file's name rather
        // than out of the file: listing a library must not open every template in it.
        entry.name = QFileInfo(path).completeBaseName();
        entry.path = path;
        m_entries.append(entry);
    }

    PopulateTable();

    // The search is a property of the view, not of the library — refilling the table must not
    // bring back the rows the user has just filtered away.
    FilterTable(m_searchEdit->text());
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatesPage::PopulateTable()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_table->setRowCount(0);

    if (m_entries.isEmpty())
    {
        m_table->setRowCount(1);
        auto *hint = new QTableWidgetItem(tr("No templates yet. Create one."));
        hint->setFlags(Qt::NoItemFlags);
        m_table->setItem(0, 1, hint);
        m_table->setSpan(0, 1, 1, 3);
        return;
    }

    m_table->setRowCount(static_cast<int>(m_entries.size()));
    for (int index = 0; index < m_entries.size(); ++index)
    {
        const Entry &entry = m_entries.at(index);
        const bool isDefault = (entry.name == m_defaultName);

        // Column 0 — the star. One template carries it, so this is a choice between rows and
        // never a property to tick and untick; clicking another moves it.
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

void OExportTemplatesPage::FilterTable(const QString &filter)
{
    // Rows are hidden, never removed: the row index is the index into m_entries, and rebuilding
    // the list on every keystroke would break that.
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        const QTableWidgetItem *nameItem = m_table->item(row, 1);
        const bool matches =
            nameItem != nullptr && nameItem->text().contains(filter, Qt::CaseInsensitive);
        m_table->setRowHidden(row, !matches);
    }
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatesPage::OnOpenFolder()
{
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(OExportTemplateStorage::DefaultDirectory()));
}

//---------------------------------------------------------------------------------------------------------------------

void OExportTemplatesPage::OnCellClicked(int row, int column)
{
    if (row < 0 || row >= m_entries.size())
    {
        return;
    }

    const Entry &entry = m_entries.at(row);

    switch (column)
    {
        case 0: // the star — the one template the export uses until another is chosen
            if (entry.name != m_defaultName)
            {
                VAbstractApplication::VApp()->Settings()->SetDefaultPdfTemplate(entry.name);
                Refresh();
            }
            break;

        case 2: // edit
            emit OpenTemplate(entry.path);
            break;

        case 3: // delete
        {
            const auto answer =
                QMessageBox::question(this, tr("Confirm deletion"),
                                      tr("Are you sure you want to delete \"%1\"?\n\n"
                                         "This action cannot be undone.")
                                          .arg(entry.name),
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes)
            {
                break;
            }

            if (QFile::remove(entry.path))
            {
                // The favourite is a name, and that name no longer names anything. Leaving it
                // behind would leave the export pointing at a file that is gone.
                if (entry.name == m_defaultName)
                {
                    VAbstractApplication::VApp()->Settings()->SetDefaultPdfTemplate(QString());
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
