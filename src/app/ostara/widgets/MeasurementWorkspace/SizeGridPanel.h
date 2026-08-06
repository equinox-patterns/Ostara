/************************************************************************
 **
 **  @file   SizeGridPanel.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Step 1 of measurement creation — axis group configuration (3 fixed columns).
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

#ifndef SIZEGRIDPANEL_H
#define SIZEGRIDPANEL_H

#include <QWidget>
#include <QMap>
#include <QString>
#include <QStringList>

class QComboBox;
class QFrame;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;

/**
 * @brief Panel for configuring size axis groups — 3 fixed columns.
 *
 * Each column is a group: name, values (comma-separated), Generate button,
 * and Base dropdown to select the reference value.
 * A group is active if it has a name and at least 2 values — no checkbox needed.
 */
class SizeGridPanel : public QWidget
{
    Q_OBJECT

public:
    static constexpr int NumGroups = 3;

    explicit SizeGridPanel(QWidget *parent = nullptr);
    ~SizeGridPanel() override = default;

    /// Table name (stored in the document, editable at both steps).
    auto GetTableName() const -> QString;
    void SetTableName(const QString &name);

    /// Populate from an existing document (editing mode).
    void LoadFromDocument(const QStringList &groupNames,
                          const QList<QStringList> &groupValues,
                          const QMap<QString, QString> &anchorBindings);

    /// Clear all groups for a fresh start.
    void ClearAll();

    /// Get names of groups that have a name + generated values, in column order.
    auto GetActiveGroupNames() const -> QStringList;

    /// Get parsed values for a group by name.
    auto GetGroupValues(const QString &groupName) const -> QStringList;

    /// Get the selected base (reference) value for a group by name.
    auto GetBaseBinding(const QString &groupName) const -> QString;

    /// Validate: at least one group with name, >=2 values, and base selected.
    auto IsValid() -> bool;

signals:
    void GroupsChanged();

private slots:
    void OnGenerateClicked(int index);
    void OnGroupDataChanged();

private:
    struct GroupColumn
    {
        QGroupBox *box{nullptr};
        QLineEdit *nameEdit{nullptr};
        QLineEdit *valuesEdit{nullptr};
        QPushButton *generateBtn{nullptr};
        QLabel *baseLabel{nullptr};
        QComboBox *baseCombo{nullptr};
        bool valuesGenerated{false};
        // Feedback area (shown after "Générer")
        QFrame *feedbackSeparator{nullptr};
        QLabel *feedbackSummary{nullptr};
        QLabel *feedbackValues{nullptr};
    };

    void SetupColumn(int index, const QString &title);
    auto FindColumnByName(const QString &name) -> GroupColumn *;
    auto FindColumnByName(const QString &name) const -> const GroupColumn *;
    static auto ParseValues(const QString &text) -> QStringList;

    GroupColumn m_columns[NumGroups];
    QLabel *m_validationLabel{nullptr};
    QLineEdit *m_tableNameEdit{nullptr};
};

#endif // SIZEGRIDPANEL_H
