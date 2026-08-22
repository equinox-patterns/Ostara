/************************************************************************
 **
 **  @file   odetailsfoldlinewidget.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Foldline editor widget for the Details mode options dock
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

#ifndef ODETAILSFOLDLINEWIDGET_H
#define ODETAILSFOLDLINEWIDGET_H

#include <QWidget>

#include <functional>

#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecepath.h"

class QButtonGroup;
class QCheckBox;
class QLabel;
class QLineEdit;
class QPushButton;

class ODetailsFoldLineWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsFoldLineWidget(QWidget *parent = nullptr);
    ~ODetailsFoldLineWidget() override = default;

    void Show(const VPiece &det);
    void Clear();

    void SetModifyPending(std::function<void(std::function<void(VPiece &)>)> modify);

    auto IsSelectingPoints() const -> bool;
    void OnNodeSelected(quint32 nodeId);
    void CancelSelection();

signals:
    void ShowStatusTip(const QString &text);
    void HighlightNode(quint32 nodeId, bool highlight);

private:
    void ConnectControls();
    void UpdateFoldLineStatus(bool defined);
    void ResetSelectionHighlight();
    auto ValidateMirrorLinePoints(quint32 startPoint, quint32 endPoint) const -> bool;

    enum class FoldLineSelectState
    {
        Idle,
        WaitingFirstPoint,
        WaitingSecondPoint
    };

    QPushButton *m_defineBtn{nullptr};
    QPushButton *m_deactivateBtn{nullptr};
    QLabel *m_statusLabel{nullptr};
    QCheckBox *m_showFullPieceCheck{nullptr};
    QCheckBox *m_showMirrorLineCheck{nullptr};
    QButtonGroup *m_typeGroup{nullptr};
    QLineEdit *m_textEdit{nullptr};

    std::function<void(std::function<void(VPiece &)>)> m_modifyPending;
    VPiecePath m_path{};
    FoldLineSelectState m_selectState{FoldLineSelectState::Idle};
    quint32 m_firstPoint{0};
    bool m_updating{false};

    Q_DISABLE_COPY_MOVE(ODetailsFoldLineWidget) // NOLINT
};

#endif // ODETAILSFOLDLINEWIDGET_H
