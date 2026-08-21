/************************************************************************
 **
 **  @file   odetailspiecewidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Piece properties widget for the Details mode options dock
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

#ifndef ODETAILSPIECEWIDGET_H
#define ODETAILSPIECEWIDGET_H

#include <QColor>
#include <QSet>
#include <QWidget>

#include <functional>

#include "../vpatterndb/vpiece.h"

class QButtonGroup;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QTabWidget;
class ODetailsFoldLineWidget;
class ODetailsGrainlineWidget;
class ODetailsSeamAllowanceWidget;
class VToolSeamAllowance;

class ODetailsPieceWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsPieceWidget(QWidget *parent = nullptr);
    ~ODetailsPieceWidget() override = default;

    void Clear();
    void Show(VToolSeamAllowance *tool);

    auto IsSelectingPassmarkPoints() const -> bool;
    void OnNodeSelectedForPassmark(quint32 nodeId);
    void FinishPassmarkSelection();
    void CancelPassmarkSelection();

    auto IsSelectingFoldLinePoints() const -> bool;
    void OnNodeSelectedForFoldLine(quint32 nodeId);
    void CancelFoldLineSelection();

    auto IsSelectingSAPoints() const -> bool;
    void OnNodeSelectedForSA(quint32 nodeId);
    void CancelSASelection();

signals:
    void ShowStatusTip(const QString &text);
    void HighlightNode(quint32 nodeId, bool highlight, QColor color = Qt::red);
    void ClearHighlight();

private:
    void ConnectControls();
    void CommitPending();
    void ModifyPiece(std::function<void(VPiece &)> func);

    // Grainline tab
    ODetailsGrainlineWidget *m_grainlineWidget{nullptr};
    ODetailsFoldLineWidget *m_foldlineWidget{nullptr};

    // Seam allowance tab
    ODetailsSeamAllowanceWidget *m_saWidget{nullptr};

    // General tab
    QLineEdit *m_nameEdit{nullptr};
    QSpinBox *m_pieceNumberSpin{nullptr};
    QLabel *m_pieceTotalLabel{nullptr};
    QButtonGroup *m_alignHGroup{nullptr};
    QButtonGroup *m_alignVGroup{nullptr};
    QPushButton *m_alignHBtns[3]{};
    QPushButton *m_alignVBtns[3]{};

    // Manage main path
    QPushButton *m_manageMainPathBtn{nullptr};
    void OnManageMainPath();

    // Passmark multi-selection
    enum class PassmarkSelectState
    {
        Idle,
        Adding,
        Removing
    };
    QPushButton *m_addPassmarksBtn{nullptr};
    QPushButton *m_removePassmarksBtn{nullptr};
    PassmarkSelectState m_passmarkSelectState{PassmarkSelectState::Idle};
    QSet<quint32> m_selectedPassmarkNodeIds;
    void OnAddPassmarks();
    void OnRemovePassmarks();

    QTabWidget *m_tabWidget{nullptr};

    VToolSeamAllowance *m_currentTool{nullptr};
    quint32 m_pieceId{0};
    bool m_updating{false};
    VPiece m_pendingDet{};

    Q_DISABLE_COPY_MOVE(ODetailsPieceWidget) // NOLINT
};

#endif // ODETAILSPIECEWIDGET_H
