/************************************************************************
 **
 **  @file   DialogManageMeasurements.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/07/2026
 **
 **  @brief  Dialog to browse, search, favorite, modify, export and delete
 **          .omt/.otu measurement files in the Ostara library.
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

#include "DialogManageMeasurements.h"
#include "vtheme.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"
#include "widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"

#include "../vmisc/FactoryWidgets.h"
#include "vexception.h"
#include "xml/oomtdocument.h"

#include <QComboBox>
#include <QDateTime>
#include <QDesktopServices>
#include <memory>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
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

    /// Size-group (axis) names of a library .omt file, read only. A single-size
    /// .otu file — or any unreadable file — has no axis and returns an empty list.
    auto OmtAxisNames(const QString &filePath) -> QStringList
    {
        const QFileInfo info(filePath);
        if (info.suffix().compare(QStringLiteral("omt"), Qt::CaseInsensitive) != 0)
        {
            return {};
        }

        try
        {
            auto doc = std::make_unique<OOmtDocument>();
            doc->setXMLContent(filePath);
            return doc->GetSizeGroupNames();
        }
        catch (const VException &)
        {
            return {};
        }
    }
} // anonymous namespace

/// Cell delegate for the whole table. For icon-only cells it centers the icon and
/// uses the dark variant when selected. It also clears State_MouseOver so the
/// theme hover (QAbstractItemView::item:hover) never paints on this table.
class IconDelegate : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);
        // No hover feedback on rows or cells: dropping State_MouseOver means the
        // theme rule QAbstractItemView::item:hover never matches for this table.
        option->state &= ~QStyle::State_MouseOver;
        option->icon = QIcon(); // Prevent base paint() from drawing the icon left-aligned
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (icon.isNull()) return;

        QIcon::Mode mode = QIcon::Normal;
        if (option.state & QStyle::State_Selected)
            mode = QIcon::Selected;   // dark variant only when actually selected, never on hover

        QSize iconSize(20, 20);
        QPoint center = option.rect.center();
        QRect iconRect(center.x() - iconSize.width() / 2,
                       center.y() - iconSize.height() / 2,
                       iconSize.width(), iconSize.height());

        icon.paint(painter, iconRect, Qt::AlignCenter, mode);
    }
};

// --------------------------------------------------------------------------
// Constructor / Destructor
// --------------------------------------------------------------------------

DialogManageMeasurements::DialogManageMeasurements(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Measurement tables"));
    setMinimumSize(720, 500);
    resize(900, 600);

    SetupUI();
    RefreshData();
}

// --------------------------------------------------------------------------
// Public Accessors
// --------------------------------------------------------------------------

auto DialogManageMeasurements::selectedPath() const -> QString
{
    return m_selectedPath;
}

// --------------------------------------------------------------------------
// SetupUI
// --------------------------------------------------------------------------

void DialogManageMeasurements::SetupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // --- Search bar ---
    auto *searchRow = new QHBoxLayout();
    searchRow->setSpacing(8);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Search tables…"));
    bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_searchEdit->addAction(QIcon(IconResPath(QStringLiteral("magnifying-glass"), dark)),
                            QLineEdit::LeadingPosition);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &DialogManageMeasurements::OnSearchChanged);
    searchRow->addWidget(m_searchEdit, 1);

    mainLayout->addLayout(searchRow);

    // --- Table ---
    m_table = new QTableWidget();
    m_table->setObjectName(QStringLiteral("measurementTable"));
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        QString(),                     // Column 0: star
        tr("Name"),                    // Column 1: file name
        tr("Grading axis"),            // Column 2: axis a size range grades along
        tr("Created"),                 // Column 3: creation date
        tr("Modified"),                // Column 4: modification date
        QString(),                     // Column 5: edit
        QString(),                     // Column 6: export
        QString()                      // Column 7: delete
    });

    m_table->setColumnWidth(0, 44);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->setColumnWidth(2, 170);
    m_table->setColumnWidth(5, 44);
    m_table->setColumnWidth(6, 44);
    m_table->setColumnWidth(7, 44);

    m_table->setIconSize(QSize(20, 20));
    // Single delegate for the whole table: it strips State_MouseOver so no cell
    // ever reports a hover (no row/cell background, no icon hover, no hover text
    // color — in both themes). Plus the validated QSS rule below for the row whose
    // cell hosts the QComboBox (same pattern as DialogRangeGroups, "combo cells").
    m_table->setItemDelegate(new IconDelegate(this));
    m_table->setStyleSheet(QStringLiteral("QTableWidget::item:hover { background: none; }"));

    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(36);
    // NoSelection + no item:hover (below) — the exact validated recipe for a table
    // whose cells embed a QComboBox (see DialogRangeGroups "combo cells"). Row
    // selection would otherwise stay stuck/highlighted while the mouse is over the
    // interactive child widget (subtilite-code 04 §25).
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &DialogManageMeasurements::OnDoubleClick);
    connect(m_table, &QTableWidget::cellClicked,
            this, [this](int row, int col){
        if (row < 0 || row >= m_entries.size()) return;
        const QString &path = m_entries.at(row).filePath;
        switch (col)
        {
        case 0: // Star — toggle favorite
            if (m_favorites.contains(path))
                m_favorites.removeAll(path);
            else
                m_favorites.append(path);
            VAbstractApplication::VApp()->Settings()->SetMeasurementFavorites(m_favorites);
            QTimer::singleShot(0, this, &DialogManageMeasurements::RefreshData);
            break;
        case 5: // Edit
            m_selectedPath = path;
            accept();
            break;
        case 6: // Export
        {
            QFileInfo fi(path);
            QString ext = fi.suffix();
            QString name = fi.completeBaseName();
            QString dstPath = QFileDialog::getSaveFileName(this,
                tr("Export \"%1\"").arg(name),
                QDir::homePath() + QDir::separator() + name + QStringLiteral(".") + ext,
                tr("Ostara files (*.%1);;All files (*)").arg(ext));
            if (!dstPath.isEmpty())
            {
                if (QFile::copy(path, dstPath))
                    QMessageBox::information(this, tr("Export successful"),
                        tr("File exported to:\n%1").arg(dstPath));
                else
                    QMessageBox::warning(this, tr("Export error"),
                        tr("Could not copy file to the selected destination."));
            }
            break;
        }
        case 7: // Delete
        {
            auto result = QMessageBox::question(this, tr("Confirm deletion"),
                tr("Are you sure you want to delete \"%1\"?\n\n"
                   "This action cannot be undone.").arg(m_entries.at(row).displayName),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (result == QMessageBox::Yes)
            {
                if (QFile::remove(path))
                {
                    if (m_favorites.contains(path))
                    {
                        m_favorites.removeAll(path);
                        VAbstractApplication::VApp()->Settings()->SetMeasurementFavorites(m_favorites);
                    }
                    QTimer::singleShot(0, this, &DialogManageMeasurements::RefreshData);
                }
                else
                {
                    QMessageBox::warning(this, tr("Error"), tr("Could not delete the file."));
                }
            }
            break;
        }
        default:
            break;
        }
    });

    mainLayout->addWidget(m_table, 1);

    // --- Bottom row ---
    auto *bottomRow = new QHBoxLayout();

    m_openFolderBtn = new QPushButton(tr("Open folder"));
    connect(m_openFolderBtn, &QPushButton::clicked,
            this, &DialogManageMeasurements::OnOpenFolder);
    bottomRow->addWidget(m_openFolderBtn);

    bottomRow->addStretch();

    mainLayout->addLayout(bottomRow);

    // --- Close button ---
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

// --------------------------------------------------------------------------
// Data Refresh
// --------------------------------------------------------------------------

void DialogManageMeasurements::RefreshData()
{
    const QString libPath = VAbstractApplication::VApp()->Settings()->GetPathMeasurementLibrary();
    m_favorites = VAbstractApplication::VApp()->Settings()->GetMeasurementFavorites();
    m_entries = MeasurementLibraryScanner::ScanLibrary(libPath, m_favorites);

    PopulateTable();

    // Re-apply current search filter
    const QString searchText = m_searchEdit->text().trimmed();
    if (!searchText.isEmpty())
    {
        FilterTable(searchText);
    }
}

// --------------------------------------------------------------------------
// Table Population
// --------------------------------------------------------------------------

void DialogManageMeasurements::PopulateTable()
{
    bool dark = (VTheme::ColorSheme() == VColorSheme::Dark);
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(m_entries.size()));

    for (int i = 0; i < m_entries.size(); ++i)
    {
        const LibraryFileEntry &entry = m_entries.at(i);

        // --- Column 0: Star (favorite toggle) ---
        {
            QIcon starIcon;
            if (entry.isFavorite)
            {
                starIcon.addFile(AccentIconPath(QStringLiteral("star-fill"), dark),
                                 QSize(), QIcon::Normal);
                starIcon.addFile(IconResPath(QStringLiteral("star-fill"), true),
                                 QSize(), QIcon::Selected);
            }
            else
            {
                starIcon.addFile(AccentIconPath(QStringLiteral("star"), dark),
                                 QSize(), QIcon::Normal);
                starIcon.addFile(IconResPath(QStringLiteral("star"), true),
                                 QSize(), QIcon::Selected);
            }
            auto *starItem = new QTableWidgetItem(starIcon, QString());
            starItem->setToolTip(entry.isFavorite
                ? tr("Remove from favorites") : tr("Add to favorites"));
            m_table->setItem(i, 0, starItem);
        }

        // --- Column 1: Name ---
        auto *nameItem = new QTableWidgetItem(entry.displayName);
        nameItem->setData(Qt::UserRole, entry.filePath);
        m_table->setItem(i, 1, nameItem);

        // --- Column 2: Size axis (graded) ---
        // A single-size table (.otu) has no axis: the cell stays empty. A
        // multi-size (.omt) table gets a combo whose default item is
        // "Not defined"; choosing an axis writes the software store that
        // graded mode reads at entry (once declared, step 1 is skipped).
        auto *axisItem = new QTableWidgetItem();
        axisItem->setTextAlignment(Qt::AlignCenter);
        m_table->setItem(i, 2, axisItem);

        const QStringList axes = OmtAxisNames(entry.filePath);
        if (not axes.isEmpty())
        {
            auto *settings = VAbstractApplication::VApp()->Settings();
            auto *combo = MakeComboBox(m_table);
            combo->addItem(tr("Not defined"));
            combo->addItems(axes);

            const QString declared = settings->GetSizeAxisForMeasurement(entry.filePath);
            const int declaredIndex = combo->findText(declared);
            combo->setCurrentIndex(declaredIndex > 0 ? declaredIndex : 0);

            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [combo, path = entry.filePath](int) {
                        const bool undeclared = combo->currentIndex() == 0;
                        VAbstractApplication::VApp()->Settings()->SetSizeAxisForMeasurement(
                            path, undeclared ? QString() : combo->currentText());
                    });
            m_table->setCellWidget(i, 2, combo);
        }

        // --- Column 3: Creation date ---
        QString createdStr = entry.created.isValid()
            ? entry.created.toString(QStringLiteral("dd/MM/yyyy HH:mm"))
            : QStringLiteral("—");
        m_table->setItem(i, 3, new QTableWidgetItem(createdStr));

        // --- Column 4: Modification date ---
        QString modifiedStr = entry.modified.isValid()
            ? entry.modified.toString(QStringLiteral("dd/MM/yyyy HH:mm"))
            : QStringLiteral("—");
        m_table->setItem(i, 4, new QTableWidgetItem(modifiedStr));

        // --- Column 5: Edit ---
        {
            QIcon editIcon;
            editIcon.addFile(IconResPath(QStringLiteral("gear-six"), dark),
                             QSize(), QIcon::Normal);
            editIcon.addFile(IconResPath(QStringLiteral("gear-six"), true),
                             QSize(), QIcon::Selected);
            auto *editItem = new QTableWidgetItem(editIcon, QString());
            editItem->setToolTip(tr("Edit"));
            m_table->setItem(i, 5, editItem);
        }

        // --- Column 6: Export ---
        {
            QIcon exportIcon;
            exportIcon.addFile(IconResPath(QStringLiteral("floppy-disk"), dark),
                               QSize(), QIcon::Normal);
            exportIcon.addFile(IconResPath(QStringLiteral("floppy-disk"), true),
                               QSize(), QIcon::Selected);
            auto *exportItem = new QTableWidgetItem(exportIcon, QString());
            exportItem->setToolTip(tr("Export"));
            m_table->setItem(i, 6, exportItem);
        }

        // --- Column 7: Delete ---
        {
            QIcon deleteIcon;
            deleteIcon.addFile(IconResPath(QStringLiteral("trash"), dark),
                               QSize(), QIcon::Normal);
            deleteIcon.addFile(IconResPath(QStringLiteral("trash"), true),
                               QSize(), QIcon::Selected);
            auto *deleteItem = new QTableWidgetItem(deleteIcon, QString());
            deleteItem->setToolTip(tr("Delete"));
            m_table->setItem(i, 7, deleteItem);
        }
    }
}

// --------------------------------------------------------------------------
// Slots
// --------------------------------------------------------------------------

void DialogManageMeasurements::OnSearchChanged(const QString &text)
{
    FilterTable(text);
}

void DialogManageMeasurements::OnDoubleClick(int row, int /*column*/)
{
    if (row >= 0 && row < m_entries.size())
    {
        m_selectedPath = m_entries.at(row).filePath;
        accept();
    }
}

void DialogManageMeasurements::OnOpenFolder()
{
    const QString libPath = VAbstractApplication::VApp()->Settings()->GetPathMeasurementLibrary();
    QDesktopServices::openUrl(QUrl::fromLocalFile(libPath));
}

// --------------------------------------------------------------------------
// Filter
// --------------------------------------------------------------------------

void DialogManageMeasurements::FilterTable(const QString &filter)
{
    for (int row = 0; row < m_table->rowCount(); ++row)
    {
        QTableWidgetItem *nameItem = m_table->item(row, 1);
        if (nameItem)
        {
            bool matches = nameItem->text().contains(filter, Qt::CaseInsensitive);
            m_table->setRowHidden(row, !matches);
        }
    }
}
