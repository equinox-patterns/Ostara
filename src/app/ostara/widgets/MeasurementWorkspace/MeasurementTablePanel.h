/************************************************************************
 **
 **  @file   MeasurementTablePanel.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Step 2 — read-only measurement table with computed values.
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

#ifndef MEASUREMENTTABLEPANEL_H
#define MEASUREMENTTABLEPANEL_H

#include <QAbstractTableModel>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QWidget>

#include "vmisc/def.h"
#include "omeasurementtypes.h"

class QTableView;

/// Table model for measurement records — read-only, with computed value column.
class OMeasurementTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column
    {
        ColName = 0,
        ColFullName = 1,
        ColCalcValue = 2,
        ColAnchorValue = 3,
        ColIncBase = 4 // increment columns start here
    };

    explicit OMeasurementTableModel(QObject *parent = nullptr);

    void SetAxisGroups(const QStringList &groupNames);
    void SetUnit(Unit unit);
    void SetRecords(const QVector<MeasurementRecord> &records);
    auto GetRecords() const -> QVector<MeasurementRecord>;

    /// Set group value lists and anchor bindings for step-offset computation.
    void SetGroupData(const QMap<QString, QStringList> &groupValues,
                      const QMap<QString, QString> &anchorBindings);

    /// Set all range groups for non-linear increment lookup.
    void SetAllRangeGroups(const QMap<QString, QVector<RangeGroup>> &rangeGroups);

    /// Set the currently selected values and recompute step offsets.
    void SetSelectedValues(const QMap<QString, QString> &selected);

    // QAbstractTableModel interface
    auto rowCount(const QModelIndex &parent = {}) const -> int override;
    auto columnCount(const QModelIndex &parent = {}) const -> int override;
    auto data(const QModelIndex &index, int role = Qt::DisplayRole) const -> QVariant override;
    auto headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const -> QVariant override;
    auto flags(const QModelIndex &index) const -> Qt::ItemFlags override;
    auto setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) -> bool override;

    void AddRecord(const MeasurementRecord &record);
    void RemoveRecord(int row);
    void MoveRecordUp(int row);
    void MoveRecordDown(int row);
    void UpdateRecord(int row, const MeasurementRecord &record);

    auto IsSeparatorRow(int row) const -> bool;

    /// Set collapsed state on a group header row (emits dataChanged for redraw).
    void SetGroupCollapsedState(int row, bool collapsed);

    /// Return the row index of the next group header after |fromRow|,
    /// or m_records.size() if no further group exists.
    auto GetNextGroupRow(int fromRow) const -> int;

    /// Return the row indices of all measurements owned by the group at |groupRow|.
    auto GetGroupMemberRows(int groupRow) const -> QVector<int>;

private:
    /// Recompute m_stepOffsets from m_selectedValues and emit dataChanged for ColCalcValue + increment columns.
    void RecomputeStepOffsets();

    QStringList m_groupNames;
    Unit m_unit{Unit::Cm};
    QVector<MeasurementRecord> m_records;

    // Step-offset computation
    QMap<QString, QStringList> m_groupValueLists;
    QMap<QString, QString> m_anchorBindings;
    QMap<QString, QString> m_selectedValues;
    QMap<QString, int> m_stepOffsets;
    QMap<QString, QVector<RangeGroup>> m_allRangeGroups;
};

/// Read-only table view panel for measurement records.
class MeasurementTablePanel : public QWidget
{
    Q_OBJECT

public:
    explicit MeasurementTablePanel(QWidget *parent = nullptr);
    ~MeasurementTablePanel() override = default;

    void SetAxisGroups(const QStringList &groupNames);

    /// Set the measurement unit for column header display.
    void SetUnit(Unit unit);

    /// Load records from document (editing mode).
    void LoadFromDocument(const QStringList &mNames,
                          const QStringList &fullNames,
                          const QStringList &descriptions,
                          const QList<double> &anchorValues,
                          const QList<QMap<QString, GroupIncrement>> &increments,
                          const QStringList &types);

    /// Get all measurement records.
    auto GetRecords() const -> QVector<MeasurementRecord>;

    /// Get the underlying model (for direct manipulation).
    auto GetModel() -> OMeasurementTableModel *;

    /// Get the currently selected row, or -1 if none.
    auto GetSelectedRow() const -> int;

    /// Select a specific row.
    void SelectRow(int row);

    /// Set group data and anchor bindings for step-offset computation.
    void SetGroupData(const QMap<QString, QStringList> &groupValues,
                      const QMap<QString, QString> &anchorBindings);

    /// Set all range groups for non-linear increment lookup.
    void SetAllRangeGroups(const QMap<QString, QVector<RangeGroup>> &rangeGroups);

    /// Set the currently selected size values (triggers ColCalcValue recompute).
    void SetSelectedValues(const QMap<QString, QString> &selected);

    /// Toggle group collapsed state and update row visibility.
    void SetGroupCollapsed(int row, bool collapsed);

    /// Refresh row visibility based on group collapse state and current filter.
    void RefreshVisibility();

    /// Set a filter text — hides rows that don't match (name, fullName).
    /// Empty string clears the filter.
    void SetFilter(const QString &text);

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

signals:
    void SelectionChanged();
    void MeasurementsChanged();

private slots:
    void OnSectionResized(int logicalIndex, int oldSize, int newSize);

private:
    void DistributeColumnWidths();

    QTableView *m_tableView{nullptr};
    OMeasurementTableModel *m_model{nullptr};
    bool m_userResizedColumns{false};
    bool m_distributing{false};
    QString m_filterText;
};

#endif // MEASUREMENTTABLEPANEL_H
