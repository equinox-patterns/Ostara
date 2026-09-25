/************************************************************************
 **
 **  @file   odetailspropertybrowser.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Property browser dock for Details mode (Modeling) — implementation.
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

#include "odetailspropertybrowser.h"

#include "odetailsgroupwidget.h"
#include "odetailsnodewidget.h"
#include "odetailsplacelabelwidget.h"
#include "odetailspiecewidget.h"
#include "opatterninfowidget.h"

#include <QDockWidget>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
// The dock's default width is driven by the content's sizeHint(). For a fixed
// layout (buttons/table that cannot reflow), sizeHint() is wider than the
// minimum, so the dock opens wider than necessary. Reporting minimumSizeHint()
// as sizeHint() makes the dock open at its smallest usable width — right before
// the horizontal scrollbar appears — while the user can still widen it.
class ODetailsScrollContents final : public QWidget
{
public:
    explicit ODetailsScrollContents(QWidget *parent = nullptr)
      : QWidget(parent)
    {}

    auto sizeHint() const -> QSize override
    {
        // The scroll area's vertical scrollbar (shown when the content is taller
        // than the viewport) eats into the horizontal viewport, which would make
        // the content overflow and show a spurious horizontal scrollbar. Add its
        // extent so the dock opens just wide enough for the content.
        auto hint = minimumSizeHint();
        hint.setWidth(hint.width() + style()->pixelMetric(QStyle::PM_ScrollBarExtent));
        return hint;
    }
};
} // namespace

ODetailsPropertyBrowser::ODetailsPropertyBrowser(QWidget *parent)
  : QObject(parent)
{
    // ── Contextual stack — pattern information page shown until a piece (or point / place label / group) is selected ──
    m_stack = new QStackedWidget();

    // Pattern information page — pattern name, client, collection, fabric info.
    m_patternInfoWidget = new OPatternInfoWidget(m_stack);
    m_stack->addWidget(m_patternInfoWidget);

    // Piece page — the contextual property browser for a selected piece.
    m_pieceWidget = new ODetailsPieceWidget(m_stack);
    m_stack->addWidget(m_pieceWidget);

    // Group page — the contextual property browser for a selected piece group.
    m_groupWidget = new ODetailsGroupWidget(m_stack);
    m_stack->addWidget(m_groupWidget);

    // Place label page — the contextual property browser of a clicked place label ("internal mark").
    m_placeLabelWidget = new ODetailsPlaceLabelWidget(m_stack);
    m_stack->addWidget(m_placeLabelWidget);

    // Contour node page — the contextual property browser of a clicked piece contour (main path) node.
    m_nodeWidget = new ODetailsNodeWidget(m_stack);
    m_stack->addWidget(m_nodeWidget);

    // Re-emit piece widget signals through the browser.
    connect(m_pieceWidget, &ODetailsPieceWidget::ShowStatusTip, this, &ODetailsPropertyBrowser::ShowStatusTip);
    connect(m_pieceWidget, &ODetailsPieceWidget::HighlightNode, this, &ODetailsPropertyBrowser::HighlightNode);
    connect(m_pieceWidget, &ODetailsPieceWidget::HighlightTemporaryNode, this, &ODetailsPropertyBrowser::HighlightTemporaryNode);
    connect(m_pieceWidget, &ODetailsPieceWidget::ClearHighlight, this, &ODetailsPropertyBrowser::ClearHighlight);
    connect(m_pieceWidget, &ODetailsPieceWidget::InternalPathHighlight, this,
            &ODetailsPropertyBrowser::InternalPathHighlight);

    // Re-emit group widget signals through the browser.
    // Picking the piece's alignment point on the canvas must not leave the dock on the clicked
    // node's page. Deferred by one turn: that click still has to reach the scene, which is what
    // would switch the page — restoring after it is what keeps the piece and its General tab in front.
    connect(m_pieceWidget, &ODetailsPieceWidget::PiecePageRequested, this, [this]()
    {
        QTimer::singleShot(0, this, [this]()
        {
            if (m_pieceWidget->IsShowingPiece())
            {
                m_stack->setCurrentWidget(m_pieceWidget);
                m_pieceWidget->RestoreGeneralTab();
            }
        });
    });

    connect(m_groupWidget, &ODetailsGroupWidget::ShowStatusTip, this, &ODetailsPropertyBrowser::ShowStatusTip);
    connect(m_groupWidget, &ODetailsGroupWidget::StartFoldLineMember, this, &ODetailsPropertyBrowser::StartFoldLineMember);
    connect(m_groupWidget, &ODetailsGroupWidget::HighlightNode, this,
            [this](quint32 nodeId, bool highlight) { emit HighlightNode(nodeId, highlight); });

    // ── Same structure as the draw-mode Options dock (VToolOptionsPropertyBrowser):
    //    QDockWidget → QScrollArea → scrollContents → content ──
    auto *scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);

    auto *scrollContents = new ODetailsScrollContents(scroll);
    auto *scrollLayout = new QVBoxLayout(scrollContents);
    scrollLayout->setContentsMargins(0, 8, 0, 0);
    scrollLayout->setSpacing(0);
    scrollLayout->addWidget(m_stack);
    scroll->setWidget(scrollContents);

    m_scroll = scroll;
}

//---------------------------------------------------------------------------------------------------------------------
QWidget *ODetailsPropertyBrowser::widget() const
{
    return m_scroll;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::RetranslateUI()
{
    m_patternInfoWidget->RetranslateUI();
    m_pieceWidget->RetranslateUI();
    m_placeLabelWidget->RetranslateUI();
    m_nodeWidget->RetranslateUI();
    // ⚠️ La page de groupe manquait à cette liste : ses quatre cases, leurs infobulles, le titre
    // « Member fold lines » et le sélecteur d'alignement gradué restaient dans la langue du
    // lancement, alors que tout le reste du dock revenait en français.
    m_groupWidget->RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowEmpty()
{
    // Leaving the piece page: the internal path highlight only lives while its tab is shown.
    ClearNodeHighlight();
    m_pieceWidget->ClearInternalPathActivation();
    m_stack->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::Clear()
{
    ClearNodeHighlight();
    m_pieceWidget->Clear();
    m_groupWidget->Clear();
    m_stack->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowPieceOptions(VToolSeamAllowance *tool)
{
    ClearNodeHighlight();
    m_pieceWidget->Show(tool);
    m_stack->setCurrentWidget(m_pieceWidget);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowGroupOptions(quint32 groupId, VContainer *data, VAbstractPattern *doc)
{
    // The canvas holder re-emits Clicked on the press that starts a drag. Re-populating the same
    // group would rebuild its lists and flicker them once — skip when it is already displayed.
    if (m_groupWidget->IsShowing(groupId) && m_stack->currentWidget() == m_groupWidget)
    {
        return;
    }
    ClearNodeHighlight();
    m_groupWidget->Show(groupId, data, doc);
    // Leaving the piece page: drop any active internal path highlight.
    m_pieceWidget->ClearInternalPathActivation();
    m_stack->setCurrentWidget(m_groupWidget);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowGrainlineTab()
{
    // **One door, two pages.** The canvas routes a click to "the tab that configures it", and which
    // page owns that tab depends on what the click selected — a piece or a group. The page is already
    // the current one when this is called: the click switches it first (SelectedDetail / ShowGroupOptions).
    if (m_stack->currentWidget() == m_groupWidget)
    {
        m_groupWidget->ShowGrainlineTab();
        return;
    }
    m_pieceWidget->ShowGrainlineTab();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowBlocksTab()
{
    if (m_stack->currentWidget() == m_groupWidget)
    {
        m_groupWidget->ShowBlocksTab();
        return;
    }
    m_pieceWidget->ShowBlocksTab();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowAnnotationsTab()
{
    if (m_stack->currentWidget() == m_groupWidget)
    {
        m_groupWidget->ShowAnnotationsTab();
        return;
    }
    m_pieceWidget->ShowAnnotationsTab();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowThumbnailTab()
{
    if (m_stack->currentWidget() == m_groupWidget)
    {
        m_groupWidget->ShowThumbnailTab();
        return;
    }
    m_pieceWidget->ShowThumbnailTab();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowPlaceLabelOptions(VToolSeamAllowance *piece, quint32 labelId)
{
    ClearNodeHighlight();
    m_placeLabelWidget->Show(piece, labelId);
    // Leaving the piece page: drop any active internal path highlight.
    m_pieceWidget->ClearInternalPathActivation();
    m_stack->setCurrentWidget(m_placeLabelWidget);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowNodeOptions(VToolSeamAllowance *piece, quint32 nodeId)
{
    if (!m_nodeWidget->Show(piece, nodeId))
    {
        // Not a contour (main path) node of this piece — show the piece page instead.
        ShowPieceOptions(piece);
        return;
    }
    // Leaving the piece page: drop any active internal path highlight.
    m_pieceWidget->ClearInternalPathActivation();
    // Keep the edited contour point highlighted (orange) while its page is open.
    m_highlightedNodeId = nodeId;
    emit HighlightNode(nodeId, true);
    m_stack->setCurrentWidget(m_nodeWidget);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ClearNodeHighlight()
{
    if (m_highlightedNodeId != 0)
    {
        emit HighlightNode(m_highlightedNodeId, false);
        m_highlightedNodeId = 0;
    }
    // The forced alignment point carries a highlight of its own: the page that owns it drops it
    // here too, so the two can never fight over the same node. Returning to the piece page
    // re-lights it (ShowPieceOptions → Show → RefreshAlignPointHighlight).
    m_pieceWidget->ClearAlignPointHighlight();
    m_groupWidget->ClearAlignPointHighlight();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::RefreshPieceAfterParse()
{
    if (m_stack->currentWidget() == m_pieceWidget)
    {
        m_pieceWidget->RefreshPieceAfterParse();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPropertyBrowser::IsSelectingPassmarkPoints() const -> bool
{
    return m_pieceWidget->IsSelectingPassmarkPoints();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::OnNodeSelectedForPassmark(quint32 nodeId)
{
    m_pieceWidget->OnNodeSelectedForPassmark(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::FinishPassmarkSelection()
{
    m_pieceWidget->FinishPassmarkSelection();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::CancelPassmarkSelection()
{
    m_pieceWidget->CancelPassmarkSelection();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPropertyBrowser::IsSelectingFoldLinePoints() const -> bool
{
    return m_pieceWidget->IsSelectingFoldLinePoints();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::OnNodeSelectedForFoldLine(quint32 nodeId)
{
    m_pieceWidget->OnNodeSelectedForFoldLine(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::CancelFoldLineSelection()
{
    m_pieceWidget->CancelFoldLineSelection();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPropertyBrowser::IsSelectingGroupFoldLinePoints() const -> bool
{
    return m_groupWidget->IsSelectingFoldLinePoint();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::OnNodeSelectedForGroupFoldLine(quint32 nodeId)
{
    m_groupWidget->OnNodeSelectedForFoldLine(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::CancelGroupFoldLineSelection()
{
    m_groupWidget->CancelFoldLineSelection();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPropertyBrowser::IsSelectingAlignPoint() const -> bool
{
    return m_pieceWidget->IsSelectingAlignPoint();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::OnNodeSelectedForAlignPoint(quint32 nodeId)
{
    m_pieceWidget->OnNodeSelectedForAlignPoint(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::CancelAlignPointSelection()
{
    m_pieceWidget->CancelAlignPointSelection();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPropertyBrowser::IsEditingThumbnail() const -> bool
{
    return m_pieceWidget->IsEditingThumbnail();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::CancelThumbnailEdit()
{
    m_pieceWidget->CancelThumbnailEdit();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsPropertyBrowser::IsSelectingSAPoints() const -> bool
{
    return m_pieceWidget->IsSelectingSAPoints();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::OnNodeSelectedForSA(quint32 nodeId)
{
    m_pieceWidget->OnNodeSelectedForSA(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::OnNodeHoveredForSA(quint32 nodeId)
{
    m_pieceWidget->OnNodeHoveredForSA(nodeId);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::CancelSASelection()
{
    m_pieceWidget->CancelSASelection();
}
