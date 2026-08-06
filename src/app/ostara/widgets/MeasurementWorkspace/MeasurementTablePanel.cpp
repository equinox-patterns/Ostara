/************************************************************************
 **
 **  @file   MeasurementTablePanel.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Step 2 — read-only measurement table implementation.
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

#include "MeasurementTablePanel.h"

#include <QHeaderView>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTableView>
#include <QVBoxLayout>

// ===== OMeasurementTableModel =====

OMeasurementTableModel::OMeasurementTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void OMeasurementTableModel::SetAxisGroups(const QStringList &groupNames)
{
    beginResetModel();
    m_groupNames = groupNames;
    endResetModel();
}

void OMeasurementTableModel::SetUnit(Unit unit)
{
    m_unit = unit;
    // Refresh column headers to show unit
    emit headerDataChanged(Qt::Horizontal, 0, columnCount() - 1);
}

void OMeasurementTableModel::SetRecords(const QVector<MeasurementRecord> &records)
{
    beginResetModel();
    m_records = records;
    endResetModel();
}

auto OMeasurementTableModel::GetRecords() const -> QVector<MeasurementRecord>
{
    return m_records;
}

void OMeasurementTableModel::SetGroupData(const QMap<QString, QStringList> &groupValues,
                                           const QMap<QString, QString> &anchorBindings)
{
    m_groupValueLists = groupValues;
    m_anchorBindings = anchorBindings;
}

void OMeasurementTableModel::SetAllRangeGroups(const QMap<QString, QVector<RangeGroup>> &rangeGroups)
{
    m_allRangeGroups = rangeGroups;
}

void OMeasurementTableModel::SetSelectedValues(const QMap<QString, QString> &selected)
{
    m_selectedValues = selected;
    RecomputeStepOffsets();
}

void OMeasurementTableModel::RecomputeStepOffsets()
{
    m_stepOffsets.clear();
    for (auto it = m_selectedValues.cbegin(); it != m_selectedValues.cend(); ++it)
    {
        const QString &group = it.key();
        const QStringList &vals = m_groupValueLists.value(group);
        int selIdx = static_cast<int>(vals.indexOf(it.value()));
        int ancIdx = static_cast<int>(vals.indexOf(m_anchorBindings.value(group)));
        m_stepOffsets[group] = (selIdx >= 0 && ancIdx >= 0) ? (selIdx - ancIdx) : 0;
    }

    // Notify ColCalcValue + increment columns for all rows
    // (increment display depends on selected values for non-linear mode)
    if (!m_records.isEmpty())
    {
        QModelIndex topLeft = index(0, ColCalcValue);
        QModelIndex bottomRight = index(static_cast<int>(m_records.size()) - 1, columnCount() - 1);
        emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole});
    }
}

auto OMeasurementTableModel::rowCount(const QModelIndex &parent) const -> int
{
    if (parent.isValid())
    {
        return 0;
    }
    return static_cast<int>(m_records.size());
}

auto OMeasurementTableModel::columnCount(const QModelIndex &parent) const -> int
{
    if (parent.isValid())
    {
        return 0;
    }
    // 4 fixed columns + 1 per axis group
    return static_cast<int>(ColIncBase + m_groupNames.size());
}

auto OMeasurementTableModel::data(const QModelIndex &index, int role) const -> QVariant
{
    if (!index.isValid() || index.row() >= m_records.size())
    {
        return {};
    }

    const MeasurementRecord &rec = m_records.at(index.row());

    if (role == Qt::DisplayRole || role == Qt::EditRole)
    {
        int col = index.column();
        if (rec.isSeparator && col != ColName)
        {
            return {};
        }

        switch (col)
        {
            case ColName:
            {
                if (rec.isSeparator)
                {
                    QString indicator = rec.isCollapsed
                        ? QStringLiteral("▶ ")  // ▶
                        : QStringLiteral("▼ "); // ▼
                    return indicator + rec.name;
                }
                return rec.name;
            }
            case ColFullName: return rec.fullName;
            case ColCalcValue:
            {
                // Compute value by cascading from anchor, transition by transition
                double val = rec.anchorValue;
                for (auto it = m_stepOffsets.cbegin(); it != m_stepOffsets.cend(); ++it)
                {
                    const QString &group = it.key();
                    int steps = it.value();
                    if (steps == 0)
                    {
                        continue;
                    }
                    int direction = (steps > 0) ? 1 : -1;
                    const QStringList &vals = m_groupValueLists.value(group);
                    int ancIdx = static_cast<int>(vals.indexOf(m_anchorBindings.value(group)));
                    for (int s = 0; s != steps; s += direction)
                    {
                        int srcIdx = ancIdx + s;
                        if (srcIdx >= 0 && srcIdx < vals.size())
                        {
                            const GroupIncrement &gi = rec.increments.value(group);
                            // Find the RangeGroup matching this measurement's rangeGroupName
                            const QVector<RangeGroup> &rgs = m_allRangeGroups.value(group);
                            QVector<RangeDef> ranges;
                            for (const auto &rg : rgs)
                            {
                                if (rg.name == gi.rangeGroupName)
                                {
                                    ranges = rg.ranges;
                                    break;
                                }
                            }
                            double inc = FindIncrementForTransition(gi, vals.at(srcIdx), vals, ranges, direction);
                            val += inc * direction;
                        }
                    }
                }
                return QString::number(val, 'f', 2);
            }
            case ColAnchorValue: return rec.anchorValue;
            default:
            {
                int incIdx = col - ColIncBase;
                if (incIdx >= 0 && incIdx < m_groupNames.size())
                {
                    const QString &groupName = m_groupNames.at(incIdx);
                    const GroupIncrement &gi = rec.increments.value(groupName);
                    if (gi.isLinear)
                    {
                        return QString::number(gi.linearValue, 'f', 2);
                    }

                    // Non-linear — show -X / +Y for currently selected size
                    if (m_selectedValues.contains(groupName) && m_groupValueLists.contains(groupName))
                    {
                        const QString &selVal = m_selectedValues.value(groupName);
                        const QStringList &allVals = m_groupValueLists.value(groupName);

                        // Resolve RangeGroup → ranges
                        const QVector<RangeGroup> &rgs = m_allRangeGroups.value(groupName);
                        QVector<RangeDef> ranges;
                        for (const auto &rg : rgs)
                        {
                            if (rg.name == gi.rangeGroupName)
                            {
                                ranges = rg.ranges;
                                break;
                            }
                        }

                        if (!ranges.isEmpty() && !allVals.isEmpty())
                        {
                            double dec = FindIncrementForTransition(gi, selVal, allVals, ranges, -1);
                            double inc = FindIncrementForTransition(gi, selVal, allVals, ranges, +1);

                            if (qFuzzyCompare(dec, inc))
                            {
                                return QStringLiteral("±%1").arg(dec, 0, 'f', 1);
                            }
                            return QStringLiteral("−%1 / +%2")
                                .arg(dec, 0, 'f', 1)
                                .arg(inc, 0, 'f', 1);
                        }
                    }

                    // Fallback — no selection or ranges not configured yet
                    return tr("Ranges (%1)").arg(gi.rangeValues.size());
                }
                return {};
            }
        }
    }

    if (role == Qt::ToolTipRole && index.column() == ColName)
    {
        if (rec.isSeparator)
        {
            return tr("Group: %1").arg(rec.name);
        }
        return rec.name;
    }

    if (role == Qt::BackgroundRole && rec.isSeparator)
    {
        return QColor(236, 236, 239); // fond_groupe in light theme
    }

    if (role == Qt::FontRole && rec.isSeparator)
    {
        QFont f;
        f.setBold(true);
        return f;
    }

    if (role == Qt::TextAlignmentRole)
    {
        return static_cast<int>(Qt::AlignCenter);
    }

    return {};
}

auto OMeasurementTableModel::headerData(int section, Qt::Orientation orientation, int role) const -> QVariant
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return {};
    }

    switch (section)
    {
        case ColName: return tr("Name");
        case ColFullName: return tr("Full name");
        case ColCalcValue:
        {
            QString unitStr = UnitsToStr(m_unit, true);
            return tr("Calculated value (%1)").arg(unitStr);
        }
        case ColAnchorValue:
        {
            QString unitStr = UnitsToStr(m_unit, true);
            return m_groupNames.isEmpty()
                       ? tr("Value (%1)").arg(unitStr)
                       : tr("Base value (%1)").arg(unitStr);
        }
        default:
        {
            int incIdx = section - ColIncBase;
            if (incIdx >= 0 && incIdx < m_groupNames.size())
            {
                return tr("Interval \"%1\"").arg(m_groupNames.at(incIdx));
            }
            return {};
        }
    }
}

auto OMeasurementTableModel::flags(const QModelIndex &index) const -> Qt::ItemFlags
{
    auto f = QAbstractTableModel::flags(index);
    if (!index.isValid() || index.row() >= m_records.size())
    {
        return f;
    }

    // All cells are read-only — editing happens in the detail panel
    return f & ~Qt::ItemIsEditable;
}

auto OMeasurementTableModel::setData(const QModelIndex &index, const QVariant &value, int role) -> bool
{
    Q_UNUSED(index)
    Q_UNUSED(value)
    Q_UNUSED(role)
    // Read-only model — editing happens in MeasurementDetailPanel
    return false;
}

void OMeasurementTableModel::AddRecord(const MeasurementRecord &record)
{
    int row = static_cast<int>(m_records.size());
    beginInsertRows({}, row, row);
    m_records.append(record);
    // Auto-add @ prefix if missing
    if (!m_records.last().name.startsWith(QLatin1Char('@')) && !m_records.last().isSeparator)
    {
        m_records.last().name.prepend(QLatin1Char('@'));
    }
    endInsertRows();
}

void OMeasurementTableModel::RemoveRecord(int row)
{
    if (row < 0 || row >= m_records.size())
    {
        return;
    }
    beginRemoveRows({}, row, row);
    m_records.removeAt(row);
    endRemoveRows();
}

void OMeasurementTableModel::MoveRecordUp(int row)
{
    if (row <= 0 || row >= m_records.size())
    {
        return;
    }
    beginMoveRows({}, row, row, {}, row - 1);
    m_records.swapItemsAt(row, row - 1);
    endMoveRows();
}

void OMeasurementTableModel::MoveRecordDown(int row)
{
    if (row < 0 || row >= m_records.size() - 1)
    {
        return;
    }
    // Qt docs: when moving down in same parent, destinationChild must be sourceLast + 2
    // because rows are placed BEFORE destinationChild
    beginMoveRows({}, row, row, {}, row + 2);
    m_records.swapItemsAt(row, row + 1);
    endMoveRows();
}

void OMeasurementTableModel::UpdateRecord(int row, const MeasurementRecord &record)
{
    if (row < 0 || row >= m_records.size())
    {
        return;
    }

    m_records[row] = record;
    // Auto-add @ prefix if missing
    if (!m_records[row].name.startsWith(QLatin1Char('@')) && !m_records[row].isSeparator)
    {
        m_records[row].name.prepend(QLatin1Char('@'));
    }

    QModelIndex topLeft = index(row, 0);
    QModelIndex bottomRight = index(row, columnCount() - 1);
    emit dataChanged(topLeft, bottomRight, {Qt::DisplayRole});
}

auto OMeasurementTableModel::IsSeparatorRow(int row) const -> bool
{
    if (row < 0 || row >= m_records.size())
    {
        return false;
    }
    return m_records.at(row).isSeparator;
}

void OMeasurementTableModel::SetGroupCollapsedState(int row, bool collapsed)
{
    if (row < 0 || row >= m_records.size() || !m_records[row].isSeparator)
    {
        return;
    }
    m_records[row].isCollapsed = collapsed;
    QModelIndex idx = index(row, ColName);
    emit dataChanged(idx, idx, {Qt::DisplayRole});
}

auto OMeasurementTableModel::GetNextGroupRow(int fromRow) const -> int
{
    for (int i = fromRow + 1; i < m_records.size(); ++i)
    {
        if (m_records[i].isSeparator)
        {
            return i;
        }
    }
    return static_cast<int>(m_records.size());
}

auto OMeasurementTableModel::GetGroupMemberRows(int groupRow) const -> QVector<int>
{
    QVector<int> members;
    if (groupRow < 0 || groupRow >= m_records.size() || !m_records[groupRow].isSeparator)
    {
        return members;
    }
    int nextGroup = GetNextGroupRow(groupRow);
    for (int i = groupRow + 1; i < nextGroup; ++i)
    {
        members.append(i);
    }
    return members;
}

// ===== MeasurementTablePanel =====

MeasurementTablePanel::MeasurementTablePanel(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Table view
    m_model = new OMeasurementTableModel(this);
    connect(m_model, &QAbstractTableModel::dataChanged, this, &MeasurementTablePanel::MeasurementsChanged);
    connect(m_model, &QAbstractTableModel::modelReset, this, &MeasurementTablePanel::RefreshVisibility);
    connect(m_model, &QAbstractTableModel::rowsInserted, this, &MeasurementTablePanel::RefreshVisibility);
    connect(m_model, &QAbstractTableModel::rowsRemoved, this, &MeasurementTablePanel::RefreshVisibility);

    m_tableView = new QTableView();
    m_tableView->setModel(m_model);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_tableView->verticalHeader()->hide();
    connect(m_tableView->horizontalHeader(), &QHeaderView::sectionResized,
            this, &MeasurementTablePanel::OnSectionResized);
    m_tableView->setAlternatingRowColors(true);

    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MeasurementTablePanel::SelectionChanged);

    mainLayout->addWidget(m_tableView, 1);
}

void MeasurementTablePanel::SetAxisGroups(const QStringList &groupNames)
{
    m_model->SetAxisGroups(groupNames);

    // Solo mode (no groups): hide computed value column, rename anchor value → "Valeur"
    if (groupNames.isEmpty())
    {
        m_tableView->setColumnHidden(OMeasurementTableModel::ColCalcValue, true);
    }
    else
    {
        m_tableView->setColumnHidden(OMeasurementTableModel::ColCalcValue, false);
    }

    // Redistribute column widths to fill available space evenly
    m_userResizedColumns = false;
    DistributeColumnWidths();
}

void MeasurementTablePanel::SetUnit(Unit unit)
{
    m_model->SetUnit(unit);
}

void MeasurementTablePanel::LoadFromDocument(const QStringList &mNames,
                                              const QStringList &fullNames,
                                              const QStringList &descriptions,
                                              const QList<double> &anchorValues,
                                              const QList<QMap<QString, GroupIncrement>> &increments,
                                              const QStringList &types)
{
    QVector<MeasurementRecord> records;

    for (int i = 0; i < mNames.size(); ++i)
    {
        MeasurementRecord rec;
        rec.name = mNames.at(i);
        rec.fullName = (i < fullNames.size()) ? fullNames.at(i) : QString();
        rec.description = (i < descriptions.size()) ? descriptions.at(i) : QString();
        rec.anchorValue = (i < anchorValues.size()) ? anchorValues.at(i) : 0.0;
        rec.increments = (i < increments.size()) ? increments.at(i) : QMap<QString, GroupIncrement>{};
        rec.isSeparator = (i < types.size() && types.at(i) == QStringLiteral("separator"));

        records.append(rec);
    }

    m_model->SetRecords(records);
}

auto MeasurementTablePanel::GetRecords() const -> QVector<MeasurementRecord>
{
    return m_model->GetRecords();
}

auto MeasurementTablePanel::GetModel() -> OMeasurementTableModel *
{
    return m_model;
}

auto MeasurementTablePanel::GetSelectedRow() const -> int
{
    QModelIndexList sel = m_tableView->selectionModel()->selectedRows();
    return sel.isEmpty() ? -1 : sel.first().row();
}

void MeasurementTablePanel::SelectRow(int row)
{
    if (row >= 0 && row < m_model->rowCount())
    {
        m_tableView->selectRow(row);
    }
}

void MeasurementTablePanel::SetGroupData(const QMap<QString, QStringList> &groupValues,
                                          const QMap<QString, QString> &anchorBindings)
{
    m_model->SetGroupData(groupValues, anchorBindings);
}

void MeasurementTablePanel::SetAllRangeGroups(const QMap<QString, QVector<RangeGroup>> &rangeGroups)
{
    m_model->SetAllRangeGroups(rangeGroups);
}

void MeasurementTablePanel::SetSelectedValues(const QMap<QString, QString> &selected)
{
    m_model->SetSelectedValues(selected);
}

void MeasurementTablePanel::SetGroupCollapsed(int row, bool collapsed)
{
    if (row < 0)
    {
        return;
    }
    m_model->SetGroupCollapsedState(row, collapsed);
    RefreshVisibility();
}

void MeasurementTablePanel::RefreshVisibility()
{
    const auto &records = m_model->GetRecords();
    // First pass: determine which rows match the filter
    QVector<bool> matchesFilter(records.size(), m_filterText.isEmpty());

    if (!m_filterText.isEmpty())
    {
        for (int i = 0; i < records.size(); ++i)
        {
            if (records[i].isSeparator)
            {
                // Groups match if they contain the filter text in their name
                matchesFilter[i] = records[i].name.contains(m_filterText, Qt::CaseInsensitive);
            }
            else
            {
                matchesFilter[i] = records[i].name.contains(m_filterText, Qt::CaseInsensitive) ||
                                   records[i].fullName.contains(m_filterText, Qt::CaseInsensitive);
            }
        }
    }

    // Second pass: hide rows based on collapse state + filter match
    for (int i = 0; i < records.size(); ++i)
    {
        bool hidden = !matchesFilter[i];

        // Hide members of collapsed groups
        if (!hidden && i > 0)
        {
            // Check if this row is inside a collapsed group
            for (int g = i - 1; g >= 0; --g)
            {
                if (records[g].isSeparator)
                {
                    if (records[g].isCollapsed)
                    {
                        int nextGroup = m_model->GetNextGroupRow(g);
                        if (i < nextGroup)
                        {
                            hidden = true;
                        }
                    }
                    break;
                }
            }
        }

        m_tableView->setRowHidden(i, hidden);
    }
}

void MeasurementTablePanel::SetFilter(const QString &text)
{
    m_filterText = text;
    RefreshVisibility();
}

void MeasurementTablePanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_userResizedColumns)
    {
        DistributeColumnWidths();
    }
}

void MeasurementTablePanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!m_userResizedColumns)
    {
        DistributeColumnWidths();
    }
}

void MeasurementTablePanel::OnSectionResized(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(logicalIndex)
    Q_UNUSED(oldSize)
    Q_UNUSED(newSize)
    if (!m_distributing)
    {
        m_userResizedColumns = true;
    }
}

void MeasurementTablePanel::DistributeColumnWidths()
{
    int cols = m_model->columnCount();
    if (cols <= 0) return;

    // Count only visible columns (hidden columns shouldn't consume width)
    int visibleCols = 0;
    for (int i = 0; i < cols; ++i)
    {
        if (!m_tableView->isColumnHidden(i))
        {
            ++visibleCols;
        }
    }
    if (visibleCols <= 0) return;

    // Use header width (full table width) rather than viewport,
    // and account for the vertical scrollbar if visible.
    int totalWidth = m_tableView->horizontalHeader()->width();
    if (totalWidth <= 0) totalWidth = m_tableView->width();
    int width = qMax(40, totalWidth / visibleCols);

    m_distributing = true;
    for (int i = 0; i < cols; ++i)
    {
        if (!m_tableView->isColumnHidden(i))
        {
            m_tableView->setColumnWidth(i, width);
        }
    }
    m_distributing = false;
}
