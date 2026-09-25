/************************************************************************
 **
 **  @file   MeasurementDetailPanel.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Detail panel for editing a measurement's parameters.
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

#ifndef MEASUREMENTDETAILPANEL_H
#define MEASUREMENTDETAILPANEL_H

#include <QGroupBox>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

class OSelectAllSpinBox;
class QButtonGroup;
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QRadioButton;
class QVBoxLayout;

struct MeasurementRecord;
struct GroupIncrement;
struct RangeGroup;
struct RangeDef;

/// Detail form for editing a measurement. QGroupBox with QFormLayout.
class MeasurementDetailPanel : public QGroupBox
{
    Q_OBJECT

public:
    explicit MeasurementDetailPanel(QWidget *parent = nullptr);
    ~MeasurementDetailPanel() override = default;

    /// Re-apply the panel's texts, its increment sections and its title in the installed language.
    void RetranslateUI();

    void SetAxisGroups(const QStringList &groupNames);
    void SetGroupValues(const QMap<QString, QStringList> &groupValues);
    void SetAllRecords(const QVector<MeasurementRecord> &records);
    void SetRangeGroups(const QMap<QString, QVector<RangeGroup>> &rangeGroups);
    auto GetRangeGroups() const -> QMap<QString, QVector<RangeGroup>>;

    void ShowMeasurement(const MeasurementRecord &rec);
    void ShowGroupHeader(const MeasurementRecord &rec);
    void Clear();
    auto BuildRecord() const -> MeasurementRecord;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void MeasurementChanged(const MeasurementRecord &record);

private slots:
    void OnFieldChanged();
    void OnModeToggled(int groupIndex);
    void OnRangeGroupChanged(int groupIndex);
    void OnRangeValueChanged(int groupIndex, int rangeIndex, double value);

private:
    void SetupUI();
    void RebuildIncrementSections();
    void RefreshRangeGroupCombo(int groupIndex);
    void ClearRangeValuesWidgets(int groupIndex);

    struct GroupWidgets
    {
        QWidget *container{nullptr};
        QLabel *titleLabel{nullptr};
        QRadioButton *linearRadio{nullptr};
        QRadioButton *rangesRadio{nullptr};
        QButtonGroup *modeGroup{nullptr};
        OSelectAllSpinBox *linearSpin{nullptr};
        QPushButton *configBtn{nullptr};
        QComboBox *rangeGroupCombo{nullptr};
        QLabel *noRangesLabel{nullptr};
        QVBoxLayout *rangeValuesLayout{nullptr};
        QVector<RangeDef> currentRanges;
        QVector<double> currentRangeValues;
    };

    QFormLayout *m_form{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QLineEdit *m_fullNameEdit{nullptr};
    QPlainTextEdit *m_descriptionEdit{nullptr};
    OSelectAllSpinBox *m_anchorValueSpin{nullptr};
    QVBoxLayout *m_incrementArea{nullptr};
    QLabel *m_placeholderLabel{nullptr};

    QVector<GroupWidgets> m_groupWidgets;
    QStringList m_groupNames;
    QMap<QString, QStringList> m_groupValues;
    QMap<QString, QVector<RangeGroup>> m_rangeGroups;
    QVector<MeasurementRecord> m_allRecords;
    bool m_populating{false};
    bool m_isGroupMode{false};

    // Description resize grip
    QWidget *m_descGrip{nullptr};
    bool m_descResizing{false};
    int m_descResizeStartY{0};
    int m_descStartHeight{0};
};

#endif // MEASUREMENTDETAILPANEL_H
