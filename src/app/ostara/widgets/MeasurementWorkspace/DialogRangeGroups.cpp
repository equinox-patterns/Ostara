/************************************************************************
 **
 **  @file   DialogRangeGroups.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Modal dialog for managing range groups.
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

#include "DialogRangeGroups.h"
#include "MeasurementTablePanel.h" // for MeasurementRecord, RangeGroup, RangeDef
#include "widgets/PhosphorIconButton.h"
#include "widgets/OPhosphorFont.h"

#include <QComboBox>
#include <QListView>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

// ===== Constructor =====

DialogRangeGroups::DialogRangeGroups(const QString &axisGroupName,
                                     const QStringList &axisValues,
                                     QVector<RangeGroup> &groups,
                                     QWidget *parent)
    : QDialog(parent)
    , m_axisGroupName(axisGroupName)
    , m_axisValues(axisValues)
    , m_groups(groups)
{
    setWindowTitle(tr("Range groups for \"%1\"").arg(axisGroupName));
    setMinimumSize(550, 550);
    SetupUI();
    RefreshGroupTable();
}

void DialogRangeGroups::SetupUI()
{
    auto *mainLayout = new QVBoxLayout(this);

    // Title
    auto *titleLabel = new QLabel(tr("Define range groups for <b>\"%1\"</b>.")
                                     .arg(m_axisGroupName));
    titleLabel->setWordWrap(true);
    mainLayout->addWidget(titleLabel);

    // Existing groups table
    m_groupTable = new QTableWidget(0, 4);
    m_groupTable->setObjectName(QStringLiteral("rangeGroupTable"));
    m_groupTable->setHorizontalHeaderLabels({tr("Name"), tr("Ranges"), QString(), QString()});
    m_groupTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_groupTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_groupTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_groupTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    m_groupTable->setColumnWidth(2, 80);
    m_groupTable->setColumnWidth(3, 80);
    m_groupTable->verticalHeader()->hide();
    m_groupTable->verticalHeader()->setDefaultSectionSize(36);
    m_groupTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainLayout->addWidget(m_groupTable, 1);

    // Create button
    m_createBtn = new QPushButton(tr("+ Create group"));
    connect(m_createBtn, &QPushButton::clicked, this, &DialogRangeGroups::OnCreateNew);
    mainLayout->addWidget(m_createBtn);

    // Edit area (hidden initially)
    m_editBox = new QGroupBox(tr("New group"));
    m_editBox->setVisible(false);
    auto *editLayout = new QVBoxLayout(m_editBox);

    auto *nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel(tr("Name:")));
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText(tr("e.g., Standard"));
    nameRow->addWidget(m_nameEdit, 1);
    editLayout->addLayout(nameRow);

    // Ranges mini-table
    m_rangesTable = new QTableWidget(0, 2);
    m_rangesTable->setObjectName(QStringLiteral("rangeGroupTable"));
    m_rangesTable->setHorizontalHeaderLabels({tr("From"), tr("To")});
    m_rangesTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_rangesTable->verticalHeader()->hide();
    m_rangesTable->verticalHeader()->setDefaultSectionSize(32);
    m_rangesTable->setMinimumHeight(120);
    editLayout->addWidget(m_rangesTable);

    m_addRangeBtn = new QPushButton(tr("+ Add range"));
    connect(m_addRangeBtn, &QPushButton::clicked, this, &DialogRangeGroups::OnAddRange);
    editLayout->addWidget(m_addRangeBtn);

    auto *btnRow = new QHBoxLayout();
    m_validateBtn = new QPushButton(tr("Validate"));
    connect(m_validateBtn, &QPushButton::clicked, this, &DialogRangeGroups::OnValidate);
    btnRow->addWidget(m_validateBtn);

    m_cancelBtn = new QPushButton(tr("Cancel"));
    connect(m_cancelBtn, &QPushButton::clicked, this, &DialogRangeGroups::OnCancelEdit);
    btnRow->addWidget(m_cancelBtn);
    editLayout->addLayout(btnRow);

    mainLayout->addWidget(m_editBox);

    // Close button
    auto *closeRow = new QHBoxLayout();
    closeRow->addStretch();
    auto *closeBtn = new QPushButton(tr("Close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    closeRow->addWidget(closeBtn);
    mainLayout->addLayout(closeRow);
}

// ===== Group Table =====

void DialogRangeGroups::RefreshGroupTable()
{
    m_groupTable->setRowCount(static_cast<int>(m_groups.size()));
    for (int i = 0; i < static_cast<int>(m_groups.size()); ++i)
    {
        const RangeGroup &rg = m_groups.at(i);

        auto *nameItem = new QTableWidgetItem(rg.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_groupTable->setItem(i, 0, nameItem);

        QStringList rangeDescs;
        for (const auto &rd : rg.ranges)
        {
            rangeDescs.append(QStringLiteral("%1-%2").arg(rd.fromValue, rd.toValue));
        }
        auto *rangesItem = new QTableWidgetItem(rangeDescs.join(QStringLiteral(", ")));
        rangesItem->setFlags(rangesItem->flags() & ~Qt::ItemIsEditable);
        m_groupTable->setItem(i, 1, rangesItem);

        // Edit button — gear Regular, accent on hover (QSS)
        auto *editBtn = new PhosphorIconButton(QStringLiteral("gear"));
        editBtn->setToolTip(tr("Edit"));
        int row = i;
        connect(editBtn, &QPushButton::clicked, this, [this, row](){ OnEditGroup(row); });
        m_groupTable->setCellWidget(i, 2, editBtn);

        // Delete button — trash Regular / Fill + accent on hover
        auto *delBtn = new PhosphorIconButton(QStringLiteral("trash"));
        delBtn->setToolTip(tr("Delete"));
        delBtn->setHoverStyle(OPhosphorFont::Fill);
        connect(delBtn, &QPushButton::clicked, this, [this, row](){ OnDeleteGroup(row); });
        m_groupTable->setCellWidget(i, 3, delBtn);
        }
    }

    void DialogRangeGroups::OnCreateNew()
    {
        m_editingRow = -1;
        m_editBox->setTitle(tr("New group"));
        m_nameEdit->clear();
        m_rangesTable->setRowCount(0);
        SetEditMode(true);
}

void DialogRangeGroups::OnEditGroup(int row)
{
    if (row < 0 || row >= m_groups.size()) return;

    m_editingRow = row;
    const RangeGroup &rg = m_groups.at(row);
    m_editBox->setTitle(tr("Edit \"%1\"").arg(rg.name));
    m_nameEdit->setText(rg.name);

    // Populate ranges table
    m_rangesTable->setRowCount(static_cast<int>(rg.ranges.size()));
    for (int i = 0; i < static_cast<int>(rg.ranges.size()); ++i)
    {
        auto *fromCombo = new QComboBox();
        fromCombo->setView(new QListView());
        fromCombo->addItems(m_axisValues);
        int idx = static_cast<int>(m_axisValues.indexOf(rg.ranges.at(i).fromValue));
        if (idx >= 0) fromCombo->setCurrentIndex(idx);
        m_rangesTable->setCellWidget(i, 0, fromCombo);

        auto *toCombo = new QComboBox();
        toCombo->setView(new QListView());
        toCombo->addItems(m_axisValues);
        idx = static_cast<int>(m_axisValues.indexOf(rg.ranges.at(i).toValue));
        if (idx >= 0) toCombo->setCurrentIndex(idx);
        m_rangesTable->setCellWidget(i, 1, toCombo);
    }

    SetEditMode(true);
}

void DialogRangeGroups::OnDeleteGroup(int row)
{
    if (row < 0 || row >= m_groups.size()) return;

    const QString &name = m_groups.at(row).name;
    if (QMessageBox::question(this, tr("Confirm"),
                              tr("Delete group \"%1\"?").arg(name))
        == QMessageBox::Yes)
    {
        m_groups.removeAt(row);
        RefreshGroupTable();
    }
}

void DialogRangeGroups::SetEditMode(bool editing)
{
    m_editBox->setVisible(editing);
    m_createBtn->setVisible(!editing);
    m_groupTable->setEnabled(!editing);
    if (!editing)
    {
        m_editingRow = -1;
    }
}

void DialogRangeGroups::OnCancelEdit()
{
    SetEditMode(false);
}

// ===== Range Operations =====

void DialogRangeGroups::OnAddRange()
{
    int row = m_rangesTable->rowCount();
    m_rangesTable->insertRow(row);

    auto *fromCombo = new QComboBox();
    fromCombo->setView(new QListView());
    fromCombo->addItems(m_axisValues);
    m_rangesTable->setCellWidget(row, 0, fromCombo);

    auto *toCombo = new QComboBox();
    toCombo->setView(new QListView());
    toCombo->addItems(m_axisValues);
    // Default to same index as fromCombo (single-value range)
    toCombo->setCurrentIndex(fromCombo->currentIndex());
    m_rangesTable->setCellWidget(row, 1, toCombo);
}

// ===== Validation =====

auto DialogRangeGroups::ValidateRanges(const QVector<RangeDef> &ranges) const -> bool
{
    if (ranges.isEmpty())
    {
        QMessageBox::warning(const_cast<DialogRangeGroups *>(this),
                             tr("Validation"), tr("Define at least one range."));
        return false;
    }

    // Build covered set from ranges
    QSet<QString> covered;
    for (const auto &rd : ranges)
    {
        int fromIdx = static_cast<int>(m_axisValues.indexOf(rd.fromValue));
        int toIdx = static_cast<int>(m_axisValues.indexOf(rd.toValue));
        if (fromIdx < 0 || toIdx < 0 || fromIdx > toIdx)
        {
            QMessageBox::warning(const_cast<DialogRangeGroups *>(this),
                                 tr("Validation"),
                                 tr("Invalid range: %1 → %2.").arg(rd.fromValue, rd.toValue));
            return false;
        }
        for (int i = fromIdx; i <= toIdx; ++i)
        {
            covered.insert(m_axisValues.at(i));
        }
    }

    // Check all values are covered
    QSet<QString> allValues(m_axisValues.begin(), m_axisValues.end());
    if (covered != allValues)
    {
        QSet<QString> missing = allValues - covered;
        QStringList missingList(missing.begin(), missing.end());
        missingList.sort();
        QMessageBox::warning(const_cast<DialogRangeGroups *>(this),
                             tr("Validation"),
                             tr("All values must be covered. Missing values: %1")
                                 .arg(missingList.join(QStringLiteral(", "))));
        return false;
    }

    // Check no overlaps (each value covered exactly once)
    // Since we already verified covered == allValues, and each value has a unique index,
    // overlaps would cause some values to appear multiple times without affecting coverage.
    // Let's check by counting: covered size should equal sum of range sizes.
    int totalInRanges = 0;
    for (const auto &rd : ranges)
    {
        int fromIdx = static_cast<int>(m_axisValues.indexOf(rd.fromValue));
        int toIdx = static_cast<int>(m_axisValues.indexOf(rd.toValue));
        totalInRanges += (toIdx - fromIdx + 1);
    }
    if (totalInRanges != m_axisValues.size())
    {
        QMessageBox::warning(const_cast<DialogRangeGroups *>(this),
                             tr("Validation"),
                             tr("Ranges overlap. Each value must belong to a single range."));
        return false;
    }

    return true;
}

void DialogRangeGroups::OnValidate()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty())
    {
        QMessageBox::warning(this, tr("Validation"), tr("Group name cannot be empty."));
        return;
    }

    // Read ranges from table
    QVector<RangeDef> ranges;
    for (int r = 0; r < m_rangesTable->rowCount(); ++r)
    {
        auto *fromCombo = qobject_cast<QComboBox *>(m_rangesTable->cellWidget(r, 0));
        auto *toCombo = qobject_cast<QComboBox *>(m_rangesTable->cellWidget(r, 1));
        if (fromCombo && toCombo)
        {
            RangeDef rd;
            rd.fromValue = fromCombo->currentText();
            rd.toValue = toCombo->currentText();
            ranges.append(rd);
        }
    }

    if (!ValidateRanges(ranges))
    {
        return;
    }

    // Check duplicate name (unless editing same row)
    for (int i = 0; i < static_cast<int>(m_groups.size()); ++i)
    {
        if (i != m_editingRow && m_groups.at(i).name == name)
        {
            QMessageBox::warning(this, tr("Validation"),
                                 tr("A group named \"%1\" already exists.").arg(name));
            return;
        }
    }

    RangeGroup rg;
    rg.name = name;
    rg.ranges = ranges;

    if (m_editingRow >= 0 && m_editingRow < m_groups.size())
    {
        m_groups[m_editingRow] = rg;
    }
    else
    {
        m_groups.append(rg);
    }

    RefreshGroupTable();
    SetEditMode(false);
}
