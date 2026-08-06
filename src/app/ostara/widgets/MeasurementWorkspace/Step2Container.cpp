/************************************************************************
 **
 **  @file   Step2Container.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Step 2 container implementation.
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

#include "Step2Container.h"
#include "MeasurementTablePanel.h"
#include "MeasurementDetailPanel.h"
#include "MeasurementLibraryScanner.h"
#include "dialogues/DialogImportStructure.h"
#include "widgets/OPhosphorFont.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QSet>
#include <QSplitter>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

// ===== Constructor =====

Step2Container::Step2Container(QWidget *parent)
    : QWidget(parent)
{
    SetupUI();
}

void Step2Container::SetupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Title
    auto *titleLabel = new QLabel(tr("Edit measurements"));
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    // Hint
    m_hintLabel = new QLabel(tr("Select a size in the bar below, "
                                 "then edit the measurements in the details panel."));
    m_hintLabel->setWordWrap(true);
    mainLayout->addWidget(m_hintLabel);

    // Table name — editable, synced with the document
    auto *nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel(tr("Name:")));
    m_tableNameEdit = new QLineEdit();
    m_tableNameEdit->setPlaceholderText(tr("Table name"));
    m_tableNameEdit->setMaximumWidth(380);
    nameRow->addWidget(m_tableNameEdit, 1);
    nameRow->addStretch();
    mainLayout->addLayout(nameRow);

    // Size selector bar — placeholder, rebuilt in SetAxisGroups

    // Buttons row (above the table)
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    // Import button — same pattern as Supprimer: QPushButton with Phosphor codepoint in text
    m_importBtn = new QPushButton(OPhosphorFont::codepoint("download")
                                  + QStringLiteral("  ") + tr("Import from table"));
    connect(m_importBtn, &QPushButton::clicked, this, &Step2Container::OnImportStructure);
    buttonLayout->addWidget(m_importBtn);

    // Vertical separator
    auto *sep = new QFrame();
    sep->setFrameShape(QFrame::VLine);
    sep->setFrameShadow(QFrame::Sunken);
    buttonLayout->addWidget(sep);

    m_addBtn = new QPushButton(tr("+ Add measurement"));
    connect(m_addBtn, &QPushButton::clicked, this, &Step2Container::OnAddMeasurement);
    buttonLayout->addWidget(m_addBtn);

    m_groupBtn = new QPushButton(tr("+ Add group"));
    m_groupBtn->setToolTip(tr("Add a measurement group"));
    connect(m_groupBtn, &QPushButton::clicked, this, &Step2Container::OnAddGroup);
    buttonLayout->addWidget(m_groupBtn);

    m_deleteBtn = new QPushButton(OPhosphorFont::codepoint("trash")
                                  + QStringLiteral("  ") + tr("Delete"));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &Step2Container::OnDeleteMeasurement);
    buttonLayout->addWidget(m_deleteBtn);

    buttonLayout->addStretch();

    m_upBtn = new QPushButton(tr("↑"));
    m_upBtn->setToolTip(tr("Move up"));
    m_upBtn->setEnabled(false);
    connect(m_upBtn, &QPushButton::clicked, this, &Step2Container::OnMoveUp);
    buttonLayout->addWidget(m_upBtn);

    m_downBtn = new QPushButton(tr("↓"));
    m_downBtn->setToolTip(tr("Move down"));
    m_downBtn->setEnabled(false);
    connect(m_downBtn, &QPushButton::clicked, this, &Step2Container::OnMoveDown);
    buttonLayout->addWidget(m_downBtn);

    mainLayout->addLayout(buttonLayout);

    // Splitter — table + detail side by side
    m_splitter = new QSplitter(Qt::Horizontal);

    m_tablePanel = new MeasurementTablePanel();
    connect(m_tablePanel, &MeasurementTablePanel::SelectionChanged,
            this, &Step2Container::OnTableSelectionChanged);
    connect(m_tablePanel, &MeasurementTablePanel::MeasurementsChanged,
            this, &Step2Container::MeasurementsChanged);
    m_splitter->addWidget(m_tablePanel);

    m_detailPanel = new MeasurementDetailPanel();
    connect(m_detailPanel, &MeasurementDetailPanel::MeasurementChanged,
            this, &Step2Container::OnDetailMeasurementChanged);
    m_splitter->addWidget(m_detailPanel);

    // Search bar with magnifying-glass icon inside the QLineEdit
    auto *searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(tr("Search measurements…"));
    searchEdit->setClearButtonEnabled(true);

    // Render the Phosphor glyph into a QIcon for the leading action
    QPixmap pix(20, 20);
    pix.fill(Qt::transparent);
    {
        QPainter p(&pix);
        p.setFont(OPhosphorFont::font(OPhosphorFont::Regular, 14));
        p.setPen(searchEdit->palette().color(QPalette::Text));
        p.drawText(pix.rect(), Qt::AlignCenter, QString(OPhosphorFont::codepoint("magnifying-glass")));
    }
    searchEdit->addAction(QIcon(pix), QLineEdit::LeadingPosition);

    connect(searchEdit, &QLineEdit::textChanged, m_tablePanel, &MeasurementTablePanel::SetFilter);
    mainLayout->addWidget(searchEdit);

    // Table gets ~70%, detail ~30% (user can adjust)
    m_splitter->setStretchFactor(0, 7);
    m_splitter->setStretchFactor(1, 3);

    mainLayout->addWidget(m_splitter, 1);

    // Context menu on table
    m_tablePanel->findChild<QTableView *>()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_tablePanel->findChild<QTableView *>(), &QTableView::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        QTableView *tv = m_tablePanel->findChild<QTableView *>();
        QModelIndex idx = tv->indexAt(pos);
        QMenu menu;
        menu.addAction(tr("+ Add measurement"), this, &Step2Container::OnAddMeasurement);
        menu.addAction(tr("+ Insert group"), this, &Step2Container::OnAddGroup);
        if (idx.isValid())
        {
            menu.addSeparator();
            menu.addAction(tr("Delete"), this, &Step2Container::OnDeleteMeasurement);
            menu.addAction(tr("Move up"), this, &Step2Container::OnMoveUp);
            menu.addAction(tr("Move down"), this, &Step2Container::OnMoveDown);
        }
        menu.exec(tv->viewport()->mapToGlobal(pos));
    });

    // Group row click → toggle collapse/expand
    connect(m_tablePanel->findChild<QTableView *>(), &QTableView::clicked,
            this, &Step2Container::OnGroupRowClicked);
}

// ===== Axis Groups =====

void Step2Container::SetAxisGroups(const QStringList &groupNames,
                                    const QList<QStringList> &groupValues,
                                    const QMap<QString, QString> &anchorBindings)
{
    m_groupNames = groupNames;
    m_groupValues = groupValues;
    m_anchorBindings = anchorBindings;

    // Update hint based on mode
    if (m_hintLabel)
    {
        if (groupNames.isEmpty())
        {
            m_hintLabel->setText(tr("Add and edit measurements in the details panel."));
        }
        else
        {
            m_hintLabel->setText(tr("Select a size in the bar below, "
                                    "then edit the measurements in the details panel."));
        }
    }

    // Forward to panels
    m_tablePanel->SetAxisGroups(groupNames);
    m_detailPanel->SetAxisGroups(groupNames);

    // Build group data map
    QMap<QString, QStringList> gvMap;
    for (int i = 0; i < groupNames.size() && i < groupValues.size(); ++i)
    {
        gvMap.insert(groupNames.at(i), groupValues.at(i));
    }
    m_detailPanel->SetGroupValues(gvMap);
    m_detailPanel->SetAllRecords(m_tablePanel->GetRecords());
    m_tablePanel->SetGroupData(gvMap, anchorBindings);

    RebuildSizeSelectors();
    RecomputeTableValues();
}

void Step2Container::SetRangeGroups(const QMap<QString, QVector<RangeGroup>> &rangeGroups)
{
    m_detailPanel->SetRangeGroups(rangeGroups);
    m_tablePanel->SetAllRangeGroups(rangeGroups);
}

auto Step2Container::GetRangeGroups() const -> QMap<QString, QVector<RangeGroup>>
{
    return m_detailPanel->GetRangeGroups();
}

void Step2Container::RebuildSizeSelectors()
{
    // Remove old widgets from the layout (find the size selector bar)
    // The size selector bar is at index 2 in the main layout (after title and hint)
    auto *mainLayout = qobject_cast<QVBoxLayout *>(layout());
    if (!mainLayout)
    {
        return;
    }

    // Remove old size label (kept as member for tracking)
    for (auto *label : m_sizeLabels)
    {
        if (label)
        {
            mainLayout->removeWidget(label);
            delete label;
        }
    }
    for (auto *combo : m_sizeCombos)
    {
        if (combo)
        {
            mainLayout->removeWidget(combo);
            delete combo;
        }
    }
    m_sizeLabels.clear();
    m_sizeCombos.clear();

    if (m_groupNames.isEmpty())
    {
        return;
    }

    // Create a horizontal bar for size selectors
    auto *selectorBar = new QHBoxLayout();
    selectorBar->setSpacing(8);

    for (int i = 0; i < m_groupNames.size(); ++i)
    {
        auto *groupLabel = new QLabel(m_groupNames.at(i) + QStringLiteral(" :"));
        selectorBar->addWidget(groupLabel);
        m_sizeLabels.append(groupLabel);

        auto *combo = new QComboBox();
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
        combo->setView(new QListView()); // macOS popup QSS fix

        if (i < m_groupValues.size())
        {
            combo->addItems(m_groupValues.at(i));
            // Select anchor value by default
            QString anchor = m_anchorBindings.value(m_groupNames.at(i));
            if (!anchor.isEmpty())
            {
                int idx = combo->findText(anchor);
                if (idx >= 0)
                {
                    combo->setCurrentIndex(idx);
                }
            }
        }

        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &Step2Container::OnSizeComboChanged);

        selectorBar->addWidget(combo);
        m_sizeCombos.append(combo);
    }

    selectorBar->addStretch();

    // Insert after hint label (index 2)
    mainLayout->insertLayout(2, selectorBar);
}

// ===== Size Selection =====

void Step2Container::OnSizeComboChanged()
{
    RecomputeTableValues();
}

void Step2Container::RecomputeTableValues()
{
    QMap<QString, QString> selected;
    for (int i = 0; i < m_groupNames.size() && i < m_sizeCombos.size(); ++i)
    {
        selected[m_groupNames.at(i)] = m_sizeCombos.at(i)->currentText();
    }
    m_tablePanel->SetSelectedValues(selected);

    // Refresh detail panel if a row is selected (computed value may have changed)
    int row = m_tablePanel->GetSelectedRow();
    if (row >= 0)
    {
        auto records = m_tablePanel->GetRecords();
        if (row < records.size())
        {
            m_detailPanel->ShowMeasurement(records.at(row));
        }
    }
}

auto Step2Container::GetSelectedValues() const -> QMap<QString, QString>
{
    QMap<QString, QString> selected;
    for (int i = 0; i < m_groupNames.size() && i < m_sizeCombos.size(); ++i)
    {
        selected[m_groupNames.at(i)] = m_sizeCombos.at(i)->currentText();
    }
    return selected;
}

// ===== Table Selection =====

void Step2Container::OnTableSelectionChanged()
{
    int row = m_tablePanel->GetSelectedRow();
    if (row < 0)
    {
        m_detailPanel->Clear();
        UpdateButtonStates();
        return;
    }

    auto records = m_tablePanel->GetRecords();
    if (row < records.size())
    {
        const MeasurementRecord &rec = records.at(row);
        if (rec.isSeparator)
        {
            m_detailPanel->ShowGroupHeader(rec);
        }
        else
        {
            m_detailPanel->ShowMeasurement(rec);
        }
    }

    UpdateButtonStates();
}

// ===== Detail Edited =====

void Step2Container::OnDetailMeasurementChanged(const MeasurementRecord &record)
{
    int row = m_tablePanel->GetSelectedRow();
    if (row < 0)
    {
        return;
    }

    m_tablePanel->GetModel()->UpdateRecord(row, record);
    // Keep range groups in sync — they may have been modified via DialogRangeGroups
    m_tablePanel->SetAllRangeGroups(m_detailPanel->GetRangeGroups());
    emit MeasurementsChanged();
}

// ===== Button Handlers =====

void Step2Container::OnAddMeasurement()
{
    MeasurementRecord rec;
    rec.name = QStringLiteral("@");
    rec.anchorValue = 0.0;
    // Initialize increments to 0 for each group
    for (const QString &groupName : m_groupNames)
    {
        rec.increments[groupName] = GroupIncrement();
    }

    m_tablePanel->GetModel()->AddRecord(rec);
    int lastRow = m_tablePanel->GetModel()->rowCount() - 1;
    m_tablePanel->SelectRow(lastRow);
    // Trigger selection manually since we just added outside normal selection flow
    OnTableSelectionChanged();
    emit MeasurementsChanged();
}

void Step2Container::OnAddGroup()
{
    MeasurementRecord rec;
    rec.name = tr("New group");
    rec.isSeparator = true;
    rec.isCollapsed = false;

    m_tablePanel->GetModel()->AddRecord(rec);
    int lastRow = m_tablePanel->GetModel()->rowCount() - 1;
    m_tablePanel->SelectRow(lastRow);
    OnTableSelectionChanged();
    emit MeasurementsChanged();
}

void Step2Container::OnImportStructure()
{
    const QString libPath = VAbstractApplication::VApp()->Settings()->GetPathMeasurementLibrary();
    QStringList favs = VAbstractApplication::VApp()->Settings()->GetMeasurementFavorites();
    const QList<LibraryFileEntry> entries = MeasurementLibraryScanner::ScanLibrary(libPath, favs);

    DialogImportStructure dlg(entries, favs, this);
    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }

    const QString selectedPath = dlg.selectedFilePath();
    if (selectedPath.isEmpty())
    {
        return;
    }

    // Update favorites
    VAbstractApplication::VApp()->Settings()->SetMeasurementFavorites(dlg.favorites());

    // Extract structure from source file
    const QVector<MeasurementRecord> sourceRecords = MeasurementLibraryScanner::ExtractStructure(
        selectedPath);
    if (sourceRecords.isEmpty())
    {
        return;
    }

    auto *model = m_tablePanel->GetModel();
    const QVector<MeasurementRecord> existingRecords = model->GetRecords();

    // Build existing name set for conflict detection
    QSet<QString> existingNames;
    for (const auto &rec : existingRecords)
    {
        existingNames.insert(rec.name);
    }

    int importedCount = 0;
    int skippedCount = 0;

    for (const auto &srcRec : sourceRecords)
    {
        if (existingNames.contains(srcRec.name))
        {
            ++skippedCount;
            continue;
        }

        MeasurementRecord newRec;
        newRec.name = srcRec.name;
        newRec.fullName = srcRec.fullName;
        newRec.description = srcRec.description;
        newRec.isSeparator = srcRec.isSeparator;
        newRec.isCollapsed = srcRec.isCollapsed;
        newRec.anchorValue = 0.0;
        newRec.increments = {};

        model->AddRecord(newRec);
        existingNames.insert(newRec.name);
        ++importedCount;
    }

    m_tablePanel->RefreshVisibility();
    UpdateButtonStates();
    emit MeasurementsChanged();

    if (skippedCount > 0)
    {
        QMessageBox::information(this,
            tr("Import complete"),
            tr("%1 measurement(s)/group(s) imported.\n"
               "%2 measurement(s)/group(s) skipped (name already exists).")
                .arg(importedCount).arg(skippedCount));
    }
}

void Step2Container::OnGroupRowClicked(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }

    int row = index.row();
    auto records = m_tablePanel->GetRecords();
    if (row < 0 || row >= records.size())
    {
        return;
    }

    if (records[row].isSeparator)
    {
        bool newState = !records[row].isCollapsed;
        m_tablePanel->SetGroupCollapsed(row, newState);
        emit MeasurementsChanged();
    }
}

void Step2Container::OnDeleteMeasurement()
{
    int row = m_tablePanel->GetSelectedRow();
    if (row < 0)
    {
        return;
    }

    auto records = m_tablePanel->GetRecords();
    if (row < records.size() && records[row].isSeparator)
    {
        // Group deletion: find members and confirm
        auto *model = m_tablePanel->GetModel();
        int nextGroup = model->GetNextGroupRow(row);
        int memberCount = nextGroup - row - 1;

        QString message;
        if (memberCount > 0)
        {
            message = tr("Group \"%1\" contains %2 measurement(s).\n"
                         "Deleting the group will also delete all its measurements.\n"
                         "Do you want to continue?")
                          .arg(records[row].name)
                          .arg(memberCount);
        }
        else
        {
            message = tr("Do you want to delete group \"%1\"?")
                          .arg(records[row].name);
        }

        auto result = QMessageBox::question(this,
                                            tr("Delete group"),
                                            message,
                                            QMessageBox::Yes | QMessageBox::No,
                                            QMessageBox::No);
        if (result != QMessageBox::Yes)
        {
            return;
        }

        // Remove from last to first to maintain correct indices
        for (int i = nextGroup - 1; i >= row; --i)
        {
            model->RemoveRecord(i);
        }
    }
    else
    {
        m_tablePanel->GetModel()->RemoveRecord(row);
    }

    UpdateButtonStates();
    emit MeasurementsChanged();
}

void Step2Container::OnMoveUp()
{
    int row = m_tablePanel->GetSelectedRow();
    if (row <= 0)
    {
        return;
    }

    m_tablePanel->GetModel()->MoveRecordUp(row);
    m_tablePanel->SelectRow(row - 1);
    UpdateButtonStates();
    emit MeasurementsChanged();
}

void Step2Container::OnMoveDown()
{
    int row = m_tablePanel->GetSelectedRow();
    if (row < 0 || row >= m_tablePanel->GetModel()->rowCount() - 1)
    {
        return;
    }

    m_tablePanel->GetModel()->MoveRecordDown(row);
    m_tablePanel->SelectRow(row + 1);
    UpdateButtonStates();
    emit MeasurementsChanged();
}

void Step2Container::UpdateButtonStates()
{
    int row = m_tablePanel->GetSelectedRow();
    int count = m_tablePanel->GetModel()->rowCount();
    bool hasSelection = (row >= 0);

    m_deleteBtn->setEnabled(hasSelection);
    m_upBtn->setEnabled(hasSelection && row > 0);
    m_downBtn->setEnabled(hasSelection && row < count - 1);

    if (hasSelection)
    {
        auto records = m_tablePanel->GetRecords();
        if (row < records.size() && records[row].isSeparator)
        {
            m_deleteBtn->setToolTip(tr("Delete group and its measurements"));
        }
        else
        {
            m_deleteBtn->setToolTip(tr("Delete measurement"));
        }
    }
}

// ===== Load / Get Records =====

void Step2Container::LoadFromDocument(const QStringList &mNames,
                                       const QStringList &fullNames,
                                       const QStringList &descriptions,
                                       const QList<double> &anchorValues,
                                       const QList<QMap<QString, GroupIncrement>> &increments,
                                       const QStringList &types)
{
    m_tablePanel->LoadFromDocument(mNames, fullNames, descriptions,
                                    anchorValues, increments, types);
    UpdateButtonStates();
}

auto Step2Container::GetRecords() const -> QVector<MeasurementRecord>
{
    return m_tablePanel->GetRecords();
}

void Step2Container::SetGroupCollapsed(int row, bool collapsed)
{
    m_tablePanel->SetGroupCollapsed(row, collapsed);
}

auto Step2Container::GetTableName() const -> QString
{
    return m_tableNameEdit ? m_tableNameEdit->text().trimmed() : QString();
}

void Step2Container::SetTableName(const QString &name)
{
    if (m_tableNameEdit)
    {
        m_tableNameEdit->setText(name);
    }
}

void Step2Container::SetUnit(Unit unit)
{
    m_tablePanel->SetUnit(unit);
}
