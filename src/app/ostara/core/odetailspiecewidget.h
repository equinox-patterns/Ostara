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
#include <QPointer>
#include <QSet>
#include <QWidget>

#include <functional>
#include <optional>

#include "../vpatterndb/vpiece.h"

class QButtonGroup;
class QGroupBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QTabWidget;
class QTimer;
class OBlockConfigWidget;
class OAnnotationsConfigWidget;
class ODetailsFoldLineWidget;
class ODetailsGrainlineWidget;
class ODetailsInternalPathWidget;
class ODetailsSeamAllowanceWidget;
class OPieceInfoWidget;
class VToolSeamAllowance;

class ODetailsPieceWidget : public QWidget
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsPieceWidget(QWidget *parent = nullptr);
    ~ODetailsPieceWidget() override = default;

    void Clear();
    void Show(VToolSeamAllowance *tool);

    /** @brief Drop the active Internal path row and its canvas highlight — the
     *         Internal path tab / piece page no longer has the focus. */
    void ClearInternalPathActivation();

    /** @brief After a full parse recreated the tools, repopulate the current piece page from the
     *         recreated tool (undo/redo of a delete otherwise leaves the internal path list stale). */
    void RefreshPieceAfterParse();

    /** @brief Show the Grainline tab (outer tab 1). */
    void ShowGrainlineTab();
    /** @brief Show the Information tab (outer tab 3) and its Blocks sub-tab. */
    void ShowBlocksTab();
    /** @brief Show the Information tab (outer tab 3) and its Annotations sub-tab. */
    void ShowAnnotationsTab();

    void RetranslateUI();

    auto IsSelectingPassmarkPoints() const -> bool;
    void OnNodeSelectedForPassmark(quint32 nodeId);
    void FinishPassmarkSelection();
    void CancelPassmarkSelection();

    auto IsSelectingFoldLinePoints() const -> bool;
    void OnNodeSelectedForFoldLine(quint32 nodeId);
    void CancelFoldLineSelection();

    auto IsSelectingSAPoints() const -> bool;
    void OnNodeSelectedForSA(quint32 nodeId);
    void OnNodeHoveredForSA(quint32 nodeId);
    void CancelSASelection();

signals:
    void ShowStatusTip(const QString &text);
    void HighlightNode(quint32 nodeId, bool highlight, QColor color = Qt::red);
    void HighlightTemporaryNode(quint32 nodeId, bool selected);
    void ClearHighlight();
    /** @brief Ask the canvas to highlight/clear a whole internal path (trace + points). */
    void InternalPathHighlight(quint32 pieceId, quint32 pathId, bool highlight);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void ConnectControls();
    void CommitPending();
    void ModifyPiece(std::function<void(VPiece &)> func);
    void ModifyPiece(std::function<void(IPieceInfo &)> func);

    // Grainline activation centering: local center of the piece bbox, or nullopt.
    auto GrainlineCenter(const VPiece &det, const VContainer *data) -> std::optional<QPointF>;

    // Grainline tab
    ODetailsGrainlineWidget *m_grainlineWidget{nullptr};
    ODetailsFoldLineWidget *m_foldlineWidget{nullptr};

    // Seam allowance tab
    ODetailsSeamAllowanceWidget *m_saWidget{nullptr};

    // Internal path tab
    ODetailsInternalPathWidget *m_internalPathWidget{nullptr};
    void OnEditInternalPath(quint32 pathId);
    void DeleteInternalPath(quint32 pathId);
    void OnInternalPathActivated(quint32 pathId);

    // Information tab — Blocks sub-tab
    OBlockConfigWidget *m_blockConfigWidget{nullptr};

    // Information tab — Annotations sub-tab
    OAnnotationsConfigWidget *m_annotationsConfigWidget{nullptr};

    // Piece sub-tab (Information) — name + piece info widget
    QLineEdit *m_nameEdit{nullptr};
    QLabel *m_nameLabel{nullptr};
    OPieceInfoWidget *m_pieceInfoWidget{nullptr};

    // General tab
    QGroupBox *m_alignGroupBox{nullptr};
    QLabel *m_alignHLabel{nullptr};
    QLabel *m_alignVLabel{nullptr};
    QButtonGroup *m_alignHGroup{nullptr};
    QButtonGroup *m_alignVGroup{nullptr};
    QPushButton *m_alignHBtns[3]{};
    QPushButton *m_alignVBtns[3]{};
    void RefreshAlignIcons();

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
    QTabWidget *m_infoSubTabs{nullptr}; // Information tab placeholder (content to be defined)

    QPointer<VToolSeamAllowance> m_currentTool;
    // Debounced tab re-population: a drag pushes a MovePiece per mouse move and each push fires
    // indexChanged, so rebuilding the tabs immediately flickers them. The single-shot collapses
    // the burst into one refresh once the interaction settles.
    QTimer *m_repopulateTimer{nullptr};
    QPointer<VToolSeamAllowance> m_repopulateTool;
    quint32 m_pieceId{0};
    bool m_updating{false};
    VPiece m_pendingDet{};

    Q_DISABLE_COPY_MOVE(ODetailsPieceWidget) // NOLINT
};

#endif // ODETAILSPIECEWIDGET_H
