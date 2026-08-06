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
#include "widgets/OPhosphorFont.h"
#include "widgets/PhosphorIconButton.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"
#include "widgets/MeasurementWorkspace/MeasurementLibraryScanner.h"

#include <QDateTime>
#include <QDesktopServices>
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
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>
#include <QVBoxLayout>

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

    auto *searchIcon = new QLabel(QString(OPhosphorFont::codepoint("magnifying-glass")));
    searchIcon->setFont(OPhosphorFont::font(OPhosphorFont::Regular, 14));
    searchRow->addWidget(searchIcon);

    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Search tables…"));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &DialogManageMeasurements::OnSearchChanged);
    searchRow->addWidget(m_searchEdit, 1);

    mainLayout->addLayout(searchRow);

    // --- Table ---
    m_table = new QTableWidget();
    m_table->setObjectName(QStringLiteral("measurementTable"));
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        QString(),                     // Column 0: star
        tr("Name"),                    // Column 1: file name
        tr("Creation date"),           // Column 2: creation date
        tr("Modification date"),       // Column 3: modification date
        tr("Actions")                  // Column 4: action buttons
    });

    m_table->setColumnWidth(0, 44);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    m_table->setColumnWidth(4, 148);

    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(36);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    connect(m_table, &QTableWidget::cellDoubleClicked,
            this, &DialogManageMeasurements::OnDoubleClick);

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
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(m_entries.size()));

    for (int i = 0; i < m_entries.size(); ++i)
    {
        const LibraryFileEntry &entry = m_entries.at(i);

        // --- Column 0: Star (favorite toggle) ---
        auto *starBtn = new QPushButton();
        starBtn->setObjectName(QStringLiteral("phosphorIconBtn"));
        starBtn->setCursor(Qt::PointingHandCursor);
        starBtn->setAutoDefault(false);
        starBtn->setFlat(true);
        starBtn->setProperty("filePath", entry.filePath);
        starBtn->setProperty("isFavorite", entry.isFavorite);

        // Font: Fill for favorited, Regular for not
        OPhosphorFont::Style starStyle = entry.isFavorite
            ? OPhosphorFont::Fill : OPhosphorFont::Regular;
        QFont f = OPhosphorFont::font(starStyle);
        f.setPixelSize(20);
        starBtn->setFont(f);
        starBtn->setText(QString(OPhosphorFont::codepoint("star")));

        starBtn->setToolTip(entry.isFavorite
            ? tr("Remove from favorites")
            : tr("Add to favorites"));

        connect(starBtn, &QPushButton::clicked,
                this, &DialogManageMeasurements::OnToggleFavorite);
        m_table->setCellWidget(i, 0, starBtn);

        // --- Column 1: Name ---
        auto *nameItem = new QTableWidgetItem(entry.displayName);
        nameItem->setData(Qt::UserRole, entry.filePath);
        m_table->setItem(i, 1, nameItem);

        // --- Column 2: Creation date ---
        QString createdStr = entry.created.isValid()
            ? entry.created.toString(QStringLiteral("dd/MM/yyyy HH:mm"))
            : QStringLiteral("—"); // em dash
        m_table->setItem(i, 2, new QTableWidgetItem(createdStr));

        // --- Column 3: Modification date ---
        QString modifiedStr = entry.modified.isValid()
            ? entry.modified.toString(QStringLiteral("dd/MM/yyyy HH:mm"))
            : QStringLiteral("—");
        m_table->setItem(i, 3, new QTableWidgetItem(modifiedStr));

        // --- Column 4: Action buttons ---
        auto *actionWidget = new QWidget();
        auto *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setSpacing(4);
        actionLayout->setContentsMargins(4, 0, 4, 0);

        // Modifier
        auto *editBtn = new PhosphorIconButton(QStringLiteral("gear"));
        editBtn->setToolTip(tr("Edit"));
        editBtn->setHoverStyle(OPhosphorFont::Fill);
        connect(editBtn, &QPushButton::clicked, this, [this, path = entry.filePath]() {
            m_selectedPath = path;
            accept();
        });
        actionLayout->addWidget(editBtn);

        // Exporter
        auto *exportBtn = new PhosphorIconButton(QStringLiteral("export"));
        exportBtn->setToolTip(tr("Export"));
        exportBtn->setHoverStyle(OPhosphorFont::Fill);
        connect(exportBtn, &QPushButton::clicked, this, [this, path = entry.filePath]() {
            QFileInfo fi(path);
            QString ext = fi.suffix();
            QString name = fi.completeBaseName();
            QString dstPath = QFileDialog::getSaveFileName(this,
                tr("Export \"%1\"").arg(name),
                QDir::homePath() + QDir::separator() + name + QStringLiteral(".") + ext,
                tr("Ostara files (*.%1);;All files (*)").arg(ext));

            if (dstPath.isEmpty()) return;

            if (QFile::copy(path, dstPath))
            {
                QMessageBox::information(this,
                    tr("Export successful"),
                    tr("File exported to:\n%1").arg(dstPath));
            }
            else
            {
                QMessageBox::warning(this,
                    tr("Export error"),
                    tr("Could not copy file to the selected destination."));
            }
        });
        actionLayout->addWidget(exportBtn);

        // Supprimer
        auto *delBtn = new PhosphorIconButton(QStringLiteral("trash"));
        delBtn->setToolTip(tr("Delete"));
        delBtn->setHoverStyle(OPhosphorFont::Fill);
        connect(delBtn, &QPushButton::clicked, this, [this, path = entry.filePath, name = entry.displayName]() {
            auto result = QMessageBox::question(this,
                tr("Confirm deletion"),
                tr("Are you sure you want to delete \"%1\"?\n\n"
                   "This action cannot be undone.").arg(name),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);

            if (result != QMessageBox::Yes) return;

            if (QFile::remove(path))
            {
                if (m_favorites.contains(path))
                {
                    m_favorites.removeAll(path);
                    VAbstractApplication::VApp()->Settings()->SetMeasurementFavorites(m_favorites);
                }
                RefreshData();
            }
            else
            {
                QMessageBox::warning(this,
                    tr("Error"),
                    tr("Could not delete the file."));
            }
        });
        actionLayout->addWidget(delBtn);

        actionLayout->addStretch();
        m_table->setCellWidget(i, 4, actionWidget);
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

void DialogManageMeasurements::OnToggleFavorite()
{
    auto *btn = qobject_cast<QPushButton *>(sender());
    if (!btn) return;

    const QString path = btn->property("filePath").toString();
    if (path.isEmpty()) return;

    if (m_favorites.contains(path))
    {
        m_favorites.removeAll(path);
    }
    else
    {
        m_favorites.append(path);
    }

    VAbstractApplication::VApp()->Settings()->SetMeasurementFavorites(m_favorites);
    RefreshData();
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
