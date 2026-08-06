/************************************************************************
 **
 **  @file   DialogRangeGroups.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/07/2026
 **
 **  @brief  Modal dialog for managing range groups per axis group.
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

#ifndef DIALOGRANGEGROUPS_H
#define DIALOGRANGEGROUPS_H

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QVector>

#include "omeasurementtypes.h"

class QComboBox;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

/// Modal dialog for creating, editing, and deleting range groups for one axis group.
class DialogRangeGroups : public QDialog
{
    Q_OBJECT

public:
    /// @param axisGroupName  Name of the axis group (e.g. "Taille") — for display
    /// @param axisValues     All values of the axis group, in order
    /// @param groups         Existing range groups (modified in-place)
    /// @param parent         Parent widget
    explicit DialogRangeGroups(const QString &axisGroupName,
                               const QStringList &axisValues,
                               QVector<RangeGroup> &groups,
                               QWidget *parent = nullptr);

    ~DialogRangeGroups() override = default;

private slots:
    void OnCreateNew();
    void OnEditGroup(int row);
    void OnDeleteGroup(int row);
    void OnAddRange();
    void OnValidate();
    void OnCancelEdit();

private:
    void SetupUI();
    void RefreshGroupTable();
    void SetEditMode(bool editing);
    auto ValidateRanges(const QVector<RangeDef> &ranges) const -> bool;

    QString m_axisGroupName;
    QStringList m_axisValues;
    QVector<RangeGroup> &m_groups;

    // Group table
    QTableWidget *m_groupTable{nullptr};
    QPushButton *m_createBtn{nullptr};

    // Edit area
    QGroupBox *m_editBox{nullptr};
    QLineEdit *m_nameEdit{nullptr};
    QTableWidget *m_rangesTable{nullptr};
    QPushButton *m_addRangeBtn{nullptr};
    QPushButton *m_validateBtn{nullptr};
    QPushButton *m_cancelBtn{nullptr};

    int m_editingRow{-1}; // -1 = creating new, >=0 = editing existing row
};

#endif // DIALOGRANGEGROUPS_H
