/************************************************************************
 **
 **  @file   olayergroupdialog.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/10/2026
 **
 **  @brief  Name a group of layers and settle the layers it holds — the
 **          dialog behind "Add a group" and behind the group row's gear.
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
#ifndef OLAYERGROUPDIALOG_H
#define OLAYERGROUPDIALOG_H

#include <QDialog>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

/// The group of layers as its dialog holds it: a name, and one combo per layer it gathers. A layer chosen in one
/// row is greyed in the others — a layer is in one group at most, and the dialog refuses to promise otherwise.
class OLayerGroupDialog final : public QDialog
{
    Q_OBJECT // NOLINT

public:
    explicit OLayerGroupDialog(QWidget *parent = nullptr);
    ~OLayerGroupDialog() override = default;

    /// The layers a row may name (id -> name), and the ones the group already holds. For a new group the second
    /// list is empty; when editing, the members are added to the candidates — they are the ones to keep.
    void SetLayers(const QMap<quint32, QString> &available, const QVector<quint32> &selected);
    /// The names the OTHER groups already wear. A group is told from another by its name, so the dialog refuses a
    /// name twice rather than letting two rows be read as one.
    void SetTakenNames(const QStringList &names);

    void SetGroupName(const QString &name);
    auto GroupName() const -> QString;

    /// The layers the group will hold, in the order of the rows — empty is a legitimate answer: a group can be
    /// created empty and filled later.
    auto Layers() const -> QVector<quint32>;

private:
    Q_DISABLE_COPY_MOVE(OLayerGroupDialog) // NOLINT

    /// One row: the layer, and the button that takes the row away (the layer leaves the group). The host is what
    /// is destroyed when the row goes — it owns both, and one pointer cannot be left behind.
    struct Row
    {
        QWidget *host{nullptr};
        QComboBox *combo{nullptr};
        QPushButton *trash{nullptr};
    };

    QLineEdit *m_name{nullptr};
    QLabel *m_problem{nullptr};
    QVBoxLayout *m_rowsLayout{nullptr};
    QPushButton *m_addLayer{nullptr};
    QPushButton *m_acceptButton{nullptr};
    QVector<Row> m_rows;
    /// The layer each row names, straight from the dialog's own state rather than read back from the combos: a
    /// combo's list no longer holds the layers the OTHER rows took, so the value has to be kept here.
    QVector<quint32> m_rowLayers;
    QMap<quint32, QString> m_available;
    QStringList m_takenNames;

    void AddRow(quint32 layerId);
    /// The user moved one row: the row that held the picked layer takes the one this row was showing.
    void OnLayerChanged(QComboBox *combo);
    void RemoveRow(int index);
    auto RowIndex(const QComboBox *combo) const -> int;
    auto ChosenLayers() const -> QVector<quint32>;
    void RefreshRows();
    void RefreshState();
};

#endif // OLAYERGROUPDIALOG_H
