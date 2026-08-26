/************************************************************************
 **
 **  @file   OInfoBlockTemplatesPage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/08/2026
 **
 **  @brief  Cartouche template library — search, single favorite, open, delete.
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

#include "OInfoBlockTemplatesPage.h"

#include "OPieceCartoucheStorage.h"

#include "def.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"
#include "vtheme.h"

#include <QDesktopServices>
#include <QFile>
#include <QFileInfo>
#include <QFontMetricsF>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
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
}  // namespace

/// Delegate for icon-only cells: centers the icon, uses dark variant on hover/selected.
class IconDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        option->icon = QIcon();  // Prevent base paint() from drawing the icon left-aligned
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
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
        const QPoint center = option.rect.center();
        const QRect iconRect(center.x() - iconSize.width() / 2, center.y() - iconSize.height() / 2, iconSize.width(),
                             iconSize.height());
        icon.paint(painter, iconRect, Qt::AlignCenter, mode);
    }
};

const int kLanguagesRole = Qt::UserRole + 1;  // QStringList of locale codes on the Languages item

/// Paints the block languages (flag + native name) with the same text color the
/// table applies to its items on hover/selection (the QSS colors the items via
/// $AccentText; the style mirrors it into QPalette::HighlightedText).
class LanguagesDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);  // background + selection

        const QStringList languages = index.data(kLanguagesRole).toStringList();
        const bool highlighted = option.state & (QStyle::State_Selected | QStyle::State_MouseOver);

        painter->save();
        painter->setPen(option.palette.color(highlighted ? QPalette::HighlightedText : QPalette::Text));

        if (languages.isEmpty())
        {
            painter->drawText(option.rect.adjusted(4, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                              QStringLiteral("—"));
            painter->restore();
            return;
        }

        const int flagSize = 16;
        const int spacing = 4;
        int x = option.rect.left() + 4;
        const int rowTop = option.rect.top();
        const int rowHeight = option.rect.height();
        const QFontMetricsF metrics(option.font);

        for (const QString &code : languages)
        {
            const QPixmap flag = LanguageFlagIcon(code).pixmap(flagSize, flagSize);
            painter->drawPixmap(x, rowTop + (rowHeight - flagSize) / 2, flag);
            x += flagSize + spacing;

            const QString name = QLocale(code).nativeLanguageName();
            painter->drawText(QRectF(x, rowTop, 1000.0, rowHeight), Qt::AlignVCenter | Qt::AlignLeft, name);
            x += static_cast<int>(metrics.horizontalAdvance(name)) + 2 * spacing;
        }

        painter->restore();
    }
};

OInfoBlockTemplatesPage::OInfoBlockTemplatesPage(QWidget *parent) : QWidget(parent)
{
    SetupUI();
    Refresh();
}

void OInfoBlockTemplatesPage::Refresh()
{
    m_defaultName = VAbstractApplication::VApp()->Settings()->GetDefaultCartoucheTemplate();

    const QStringList paths = OPieceCartoucheStorage::ScanDirectory(OPieceCartoucheStorage::DefaultDirectory());
    m_entries.clear();
    m_entries.reserve(paths.size());
    for (const QString &path : paths)
    {
        Entry entry;
        entry.name = QFileInfo(path).completeBaseName();
        entry.path = path;

        OPieceCartoucheTemplate tpl;
        QString error;
        if (OPieceCartoucheStorage::Load(path, &tpl, &error))
        {
            entry.languages = tpl.Languages();
        }
        m_entries.append(entry);
    }

    PopulateTable();

    const QString searchText = m_searchEdit->text().trimmed();
    if (!searchText.isEmpty())
    {
        FilterTable(searchText);
    }
}

void OInfoBlockTemplatesPage::RetranslateUI()
{
    m_searchEdit->setPlaceholderText(tr("Search templates…"));
    m_openFolderBtn->setText(tr("Open folder"));
    PopulateTable();
}

void OInfoBlockTemplatesPage::SetupUI()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(8);

    auto *title = new QLabel(tr("Existing templates"), this);
    QFont titleFont = title->font();
    titleFont.setPointSize(13);
    titleFont.setBold(true);
    title->setFont(titleFont);
    layout->addWidget(title);

    // --- Search bar ---
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Search templates…"));
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_searchEdit->addAction(QIcon(IconResPath(QStringLiteral("magnifying-glass"), dark)), QLineEdit::LeadingPosition);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &OInfoBlockTemplatesPage::FilterTable);
    layout->addWidget(m_searchEdit);

    // --- Table ---
    m_table = new QTableWidget(this);
    m_table->setObjectName(QStringLiteral("cartoucheTemplatesTable"));
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({QString(),       // 0: star (default)
                                        tr("Name"),      // 1
                                        tr("Languages"), // 2
                                        QString(),       // 3: edit
                                        QString()});     // 4: delete

    m_table->setColumnWidth(0, 44);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);  // Name: modest, user-resizable
    m_table->setColumnWidth(1, 180);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);  // Languages: takes the remaining room
    m_table->setColumnWidth(3, 44);
    m_table->setColumnWidth(4, 44);

    m_table->setIconSize(QSize(20, 20));
    m_table->setItemDelegateForColumn(0, new IconDelegate(this));
    m_table->setItemDelegateForColumn(2, new LanguagesDelegate(this));
    m_table->setItemDelegateForColumn(3, new IconDelegate(this));
    m_table->setItemDelegateForColumn(4, new IconDelegate(this));

    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(36);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (row >= 0 && row < m_entries.size())
        {
            emit OpenTemplate(m_entries.at(row).path);
        }
    });
    connect(m_table, &QTableWidget::cellClicked, this, &OInfoBlockTemplatesPage::OnCellClicked);
    layout->addWidget(m_table, 1);

    // --- Bottom row: open folder ---
    m_openFolderBtn = new QPushButton(tr("Open folder"), this);
    connect(m_openFolderBtn, &QPushButton::clicked, this, &OInfoBlockTemplatesPage::OnOpenFolder);
    layout->addWidget(m_openFolderBtn, 0, Qt::AlignLeft);
}

void OInfoBlockTemplatesPage::PopulateTable()
{
    const bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_table->setRowCount(0);  // removes rows, items and the languages cell widgets

    if (m_entries.isEmpty())
    {
        m_table->setRowCount(1);
        auto *hint = new QTableWidgetItem(tr("No templates yet. Create one."));
        hint->setFlags(Qt::NoItemFlags);
        m_table->setItem(0, 1, hint);
        m_table->setSpan(0, 1, 1, 4);
        return;
    }

    m_table->setRowCount(static_cast<int>(m_entries.size()));
    for (int i = 0; i < m_entries.size(); ++i)
    {
        const Entry &entry = m_entries.at(i);
        const bool isDefault = (entry.name == m_defaultName);

        // --- Column 0: star (the single default cartouche) ---
        QIcon starIcon;
        if (isDefault)
        {
            starIcon.addFile(AccentIconPath(QStringLiteral("star-fill"), dark), QSize(), QIcon::Normal);
            starIcon.addFile(IconResPath(QStringLiteral("star-fill"), true), QSize(), QIcon::Selected);
        }
        else
        {
            starIcon.addFile(AccentIconPath(QStringLiteral("star"), dark), QSize(), QIcon::Normal);
            starIcon.addFile(IconResPath(QStringLiteral("star"), true), QSize(), QIcon::Selected);
        }
        auto *starItem = new QTableWidgetItem(starIcon, QString());
        starItem->setToolTip(isDefault ? tr("This is the default cartouche") : tr("Make this the default cartouche"));
        m_table->setItem(i, 0, starItem);

        // --- Column 1: name ---
        auto *nameItem = new QTableWidgetItem(entry.name);
        nameItem->setData(Qt::UserRole, entry.path);
        m_table->setItem(i, 1, nameItem);

        // --- Column 2: languages (flag + native name, painted by LanguagesDelegate) ---
        auto *languagesItem = new QTableWidgetItem();
        languagesItem->setData(kLanguagesRole, entry.languages);
        m_table->setItem(i, 2, languagesItem);

        // --- Column 3: edit ---
        QIcon editIcon;
        editIcon.addFile(IconResPath(QStringLiteral("gear-six"), dark), QSize(), QIcon::Normal);
        editIcon.addFile(IconResPath(QStringLiteral("gear-six"), true), QSize(), QIcon::Selected);
        auto *editItem = new QTableWidgetItem(editIcon, QString());
        editItem->setToolTip(tr("Edit"));
        m_table->setItem(i, 3, editItem);

        // --- Column 4: delete ---
        QIcon deleteIcon;
        deleteIcon.addFile(IconResPath(QStringLiteral("trash"), dark), QSize(), QIcon::Normal);
        deleteIcon.addFile(IconResPath(QStringLiteral("trash"), true), QSize(), QIcon::Selected);
        auto *deleteItem = new QTableWidgetItem(deleteIcon, QString());
        deleteItem->setToolTip(tr("Delete"));
        m_table->setItem(i, 4, deleteItem);
    }
}

void OInfoBlockTemplatesPage::FilterTable(const QString &filter)
{
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        QTableWidgetItem *nameItem = m_table->item(row, 1);
        const bool matches = nameItem != nullptr && nameItem->text().contains(filter, Qt::CaseInsensitive);
        m_table->setRowHidden(row, !matches);
    }
}

void OInfoBlockTemplatesPage::OnOpenFolder()
{
    const QString dir = OPieceCartoucheStorage::DefaultDirectory();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void OInfoBlockTemplatesPage::OnCellClicked(int row, int column)
{
    if (row < 0 || row >= m_entries.size())
    {
        return;
    }
    const Entry &entry = m_entries.at(row);

    switch (column)
    {
        case 0:  // star — the single default cartouche
            if (entry.name != m_defaultName)
            {
                VAbstractApplication::VApp()->Settings()->SetDefaultCartoucheTemplate(entry.name);
                Refresh();
            }
            break;
        case 3:  // edit
            emit OpenTemplate(entry.path);
            break;
        case 4:  // delete
        {
            const auto answer = QMessageBox::question(this, tr("Confirm deletion"),
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
                if (entry.name == m_defaultName)
                {
                    VAbstractApplication::VApp()->Settings()->SetDefaultCartoucheTemplate(QString());
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
