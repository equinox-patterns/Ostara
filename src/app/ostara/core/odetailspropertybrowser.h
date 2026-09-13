/************************************************************************
 **
 **  @file   odetailspropertybrowser.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Property browser dock for Details mode (Modeling) — contextual
 **          stack (empty / point / piece / place label / group).
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

#ifndef ODETAILSPROPERTYBROWSER_H
#define ODETAILSPROPERTYBROWSER_H

#include <QColor>
#include <QObject>

class QDockWidget;
class QStackedWidget;
class ODetailsGroupWidget;
class ODetailsNodeWidget;
class ODetailsPlaceLabelWidget;
class ODetailsPieceWidget;
class OPatternInfoWidget;
class QScrollArea;
class VAbstractPattern;
class VContainer;
class VToolSeamAllowance;

class ODetailsPropertyBrowser : public QObject
{
    Q_OBJECT // NOLINT

public:
    explicit ODetailsPropertyBrowser(QWidget *parent);
    ~ODetailsPropertyBrowser() override = default;

    /** @brief The scroll area the browser built its content into. Owned by the
     *         dock's QStackedWidget — callers must not delete or reparent it. */
    QWidget *widget() const;

    void ShowEmpty();
    void ShowPieceOptions(VToolSeamAllowance *tool);
    void ShowGroupOptions(quint32 groupId, VContainer *data, VAbstractPattern *doc);
    void ShowGrainlineTab();
    void ShowBlocksTab();
    void ShowAnnotationsTab();
    /** @brief Shows the property page of a clicked place label (an "internal mark"). */
    void ShowPlaceLabelOptions(VToolSeamAllowance *piece, quint32 labelId);
    /** @brief Shows the property page of a clicked piece contour (main path) node. */
    void ShowNodeOptions(VToolSeamAllowance *piece, quint32 nodeId);
    void Clear();

    /** @brief If the piece page is the current page, repopulate it after a full parse recreated
     *         the tools (undo/redo of a delete). No-op otherwise. */
    void RefreshPieceAfterParse();

    void RetranslateUI();

    auto IsSelectingPassmarkPoints() const -> bool;
    void OnNodeSelectedForPassmark(quint32 nodeId);
    void FinishPassmarkSelection();
    void CancelPassmarkSelection();

    auto IsSelectingFoldLinePoints() const -> bool;
    void OnNodeSelectedForFoldLine(quint32 nodeId);
    void CancelFoldLineSelection();

    auto IsSelectingGroupFoldLinePoints() const -> bool;
    void OnNodeSelectedForGroupFoldLine(quint32 nodeId);
    void CancelGroupFoldLineSelection();

    auto IsSelectingSAPoints() const -> bool;
    void OnNodeSelectedForSA(quint32 nodeId);
    void OnNodeHoveredForSA(quint32 nodeId);
    void CancelSASelection();

    auto IsSelectingAlignPoint() const -> bool;
    void OnNodeSelectedForAlignPoint(quint32 nodeId);
    void CancelAlignPointSelection();

signals:
    void ShowStatusTip(const QString &text);
    void HighlightNode(quint32 nodeId, bool highlight, QColor color = Qt::red);
    void HighlightTemporaryNode(quint32 nodeId, bool selected);
    void ClearHighlight();
    void StartFoldLineMember(quint32 pieceId);
    /** @brief Ask the canvas to highlight/clear a whole internal path (trace + points). */
    void InternalPathHighlight(quint32 pieceId, quint32 pathId, bool highlight);

private:
    /** @brief Turns off the highlighted contour node when leaving its page. */
    void ClearNodeHighlight();

    // cppcheck-suppress unknownMacro
    Q_DISABLE_COPY_MOVE(ODetailsPropertyBrowser) // NOLINT

    QStackedWidget *m_stack{nullptr};
    quint32 m_highlightedNodeId{0};
    ODetailsPieceWidget *m_pieceWidget{nullptr};
    ODetailsGroupWidget *m_groupWidget{nullptr};
    ODetailsPlaceLabelWidget *m_placeLabelWidget{nullptr};
    ODetailsNodeWidget *m_nodeWidget{nullptr};
    OPatternInfoWidget *m_patternInfoWidget{nullptr};
    QScrollArea *m_scroll{nullptr};
};

#endif // ODETAILSPROPERTYBROWSER_H
