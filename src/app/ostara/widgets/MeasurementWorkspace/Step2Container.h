/************************************************************************
 **
 **  @file   Step2Container.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Step 2 container — size selectors + table + detail panel.
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

#ifndef STEP2CONTAINER_H
#define STEP2CONTAINER_H

#include <QWidget>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

#include "MeasurementTablePanel.h" // for MeasurementRecord, OMeasurementTableModel

class QComboBox;
class QLabel;
class QLineEdit;
class QModelIndex;
class QPushButton;
class QSplitter;
class MeasurementDetailPanel;

/// Container for step 2: size selectors, table, detail panel, and action buttons.
class Step2Container : public QWidget
{
    Q_OBJECT

public:
    explicit Step2Container(QWidget *parent = nullptr);
    ~Step2Container() override = default;

    /// Re-apply the container's texts, its hint and its two panels in the installed language.
    void RetranslateUI();

    /// Set the measurement unit for column header display.
    void SetUnit(Unit unit);

    /// Table name (stored in the document, editable at both steps).
    auto GetTableName() const -> QString;
    void SetTableName(const QString &name);

    /// Configure axis groups with names, values, and anchor bindings.
    void SetAxisGroups(const QStringList &groupNames,
                       const QList<QStringList> &groupValues,
                       const QMap<QString, QString> &anchorBindings);

    /// Set range groups (from document) for all axis groups.
    void SetRangeGroups(const QMap<QString, QVector<RangeGroup>> &rangeGroups);

    /// Get current range groups (for saving to document).
    auto GetRangeGroups() const -> QMap<QString, QVector<RangeGroup>>;

    /// Load records from document.
    void LoadFromDocument(const QStringList &mNames,
                          const QStringList &fullNames,
                          const QStringList &descriptions,
                          const QList<double> &anchorValues,
                          const QList<QMap<QString, GroupIncrement>> &increments,
                          const QStringList &types);

    /// Get all measurement records.
    auto GetRecords() const -> QVector<MeasurementRecord>;

    /// Import the measurement definitions of an existing table — name, full name, description and the
    /// separator/collapsed flags — and nothing else: values, increments and axes stay behind. Rows whose name
    /// is already in the table are skipped. Returns the number of rows imported; @a skipped, when given,
    /// receives the number of rows skipped. One import path, shared by the "Import from table" button and by
    /// the template chosen when creating a table.
    auto ImportStructure(const QString &filePath, int *skipped = nullptr) -> int;

    /// Toggle group collapsed state and update row visibility.
    void SetGroupCollapsed(int row, bool collapsed);

    /// Get the currently selected size combination.
    auto GetSelectedValues() const -> QMap<QString, QString>;

signals:
    void MeasurementsChanged();

private slots:
    void OnSizeComboChanged();
    void OnTableSelectionChanged();
    void OnDetailMeasurementChanged(const MeasurementRecord &record);
    void OnAddMeasurement();
    void OnAddGroup();
    void OnDeleteMeasurement();
    void OnMoveUp();
    void OnMoveDown();
    void OnGroupRowClicked(const QModelIndex &index);
    void OnImportStructure();

private:
    void SetupUI();
    void UpdateButtonStates();
    void RebuildSizeSelectors();
    void RecomputeTableValues();
    bool eventFilter(QObject *obj, QEvent *event) override;

    // Hint label (updated based on solo/multi mode)
    QLabel *m_hintLabel{nullptr};

    // Table name
    QLineEdit *m_tableNameEdit{nullptr};

    // Size selectors
    QVector<QLabel *> m_sizeLabels;
    QVector<QComboBox *> m_sizeCombos;

    // Splitter
    QSplitter *m_splitter{nullptr};

    // Child panels
    MeasurementTablePanel *m_tablePanel{nullptr};
    MeasurementDetailPanel *m_detailPanel{nullptr};

    // Action buttons
    QPushButton *m_importBtn{nullptr};
    QPushButton *m_addBtn{nullptr};
    QPushButton *m_groupBtn{nullptr};
    QPushButton *m_deleteBtn{nullptr};
    QPushButton *m_upBtn{nullptr};
    QPushButton *m_downBtn{nullptr};

    // Data
    QStringList m_groupNames;
    QList<QStringList> m_groupValues;
    QMap<QString, QString> m_anchorBindings;
};

#endif // STEP2CONTAINER_H
