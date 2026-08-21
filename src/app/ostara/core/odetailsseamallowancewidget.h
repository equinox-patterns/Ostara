/************************************************************************
 **
 **  @file   odetailsseamallowancewidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Seam allowance editor widget for the Details mode options dock
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

#ifndef ODETAILSSEAMALLOWANCEWIDGET_H
#define ODETAILSSEAMALLOWANCEWIDGET_H

#include <QColor>
#include <QWidget>

#include <functional>

#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecepath.h"

class QCheckBox;
class QLineEdit;
class QPushButton;
class QTableWidget;

//---------------------------------------------------------------------------------------------------------------------
/** @brief A detected custom seam-allowance margin group on a piece contour. */
struct CustomMarginGroup
{
    QVector<quint32> nodeIds; ///< Node IDs defining the segment (ordered along contour)
    QString value;            ///< The SA formula value for this group
};

auto FindCustomMarginGroups(const VPiece &detail) -> QVector<CustomMarginGroup>;

extern const QVector<QColor> kCustomMarginColors;

class ODetailsSeamAllowanceWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsSeamAllowanceWidget(QWidget *parent = nullptr);
    ~ODetailsSeamAllowanceWidget() override = default;

    void Show(const VPiece &det);
    void Clear();

    void SetModifyPending(std::function<void(std::function<void(VPiece &)>)> modify);

    auto IsSelectingPoints() const -> bool;
    void OnNodeSelected(quint32 nodeId);
    void CancelSelection();

    void RefreshTable(const VPiece &detail);

signals:
    void ShowStatusTip(const QString &text);
    void HighlightNode(quint32 nodeId, bool highlight);
    void ClearHighlight();

private:
    void ConnectControls();
    void PopulateTable(const VPiece &detail);
    void CollectGroupsFromTable(VPiece &det) const;
    void FinishSelection();

    // UI
    QCheckBox *m_activeCheck{nullptr};
    QCheckBox *m_hideMainPathCheck{nullptr};
    QLineEdit *m_valueEdit{nullptr};
    QPushButton *m_applyValueBtn{nullptr};
    QPushButton *m_addCustomBtn{nullptr};
    QTableWidget *m_table{nullptr};
    QPushButton *m_manageSavedBtn{nullptr};

    // State
    enum class State
    {
        Idle,
        SelectingPoints
    };
    State m_state{State::Idle};
    QVector<quint32> m_selectedNodeIds{};

    VPiecePath m_path{};
    std::function<void(std::function<void(VPiece &)>)> m_modifyPending;
    bool m_updating{false};

    Q_DISABLE_COPY_MOVE(ODetailsSeamAllowanceWidget) // NOLINT
};

#endif // ODETAILSSEAMALLOWANCEWIDGET_H
