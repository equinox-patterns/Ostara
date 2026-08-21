/************************************************************************
 **
 **  @file   dialogmainpath.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Modal dialog for editing the main path nodes of a piece
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

#ifndef DIALOGMAINPATH_H
#define DIALOGMAINPATH_H

#include <QDialog>
#include <QVector>

class QLabel;
class QListWidget;
class QToolButton;
class QDialogButtonBox;
class VContainer;
class VPieceNode;

class DialogMainPath : public QDialog
{
    Q_OBJECT // NOLINT

public:
    explicit DialogMainPath(const VContainer *data, QWidget *parent = nullptr);
    ~DialogMainPath() override = default;

    void SetNodes(const QVector<VPieceNode> &nodes);
    auto GetNodes() const -> QVector<VPieceNode>;

private slots:
    void ShowContextMenu(const QPoint &pos);
    void OnSelectionChanged();

    void OnReverseToggled(bool checked);
    void OnExcludedToggled(bool checked);
    void OnTurnPointToggled(bool checked);
    void OnCheckUniquenessToggled(bool checked);
    void OnPassmarkToggled(bool checked);
    void OnDuplicateNode();
    void OnDeleteNode();

    void OnMoveTop();
    void OnMoveUp();
    void OnMoveDown();
    void OnMoveBottom();

private:
    void BuildUi();
    void ConnectSignals();
    void EnableControls();
    void UpdateValidation();
    auto MainPathIsValid() const -> bool;
    auto MainPathIsClockwise() const -> bool;

    const VContainer *m_data;

    // Toolbar
    QToolButton *m_btnReverse{nullptr};
    QToolButton *m_btnExcluded{nullptr};
    QToolButton *m_btnTurnPoint{nullptr};
    QToolButton *m_btnCheckUniqueness{nullptr};
    QToolButton *m_btnPassmark{nullptr};
    QToolButton *m_btnDuplicateNode{nullptr};
    QToolButton *m_btnDelete{nullptr};

    // Navigation
    QToolButton *m_btnTop{nullptr};
    QToolButton *m_btnUp{nullptr};
    QToolButton *m_btnDown{nullptr};
    QToolButton *m_btnBottom{nullptr};

    QListWidget *m_listWidget{nullptr};
    QLabel *m_helpLabel{nullptr};
    QDialogButtonBox *m_buttonBox{nullptr};

    Q_DISABLE_COPY_MOVE(DialogMainPath) // NOLINT
};

#endif // DIALOGMAINPATH_H
