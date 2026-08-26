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

#include "odetailspiecewidget.h"

#include <QDockWidget>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStyle>
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
    // ── Contextual stack — empty page shown until a piece (or point / place label / group) is selected ──
    m_stack = new QStackedWidget();

    auto *emptyPage = new QWidget(m_stack);
    auto *emptyLayout = new QVBoxLayout(emptyPage);
    emptyLayout->setContentsMargins(0, 0, 0, 0);
    m_emptyLabel = new QLabel(tr("Select a piece"), emptyPage);
    m_emptyLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    emptyLayout->addWidget(m_emptyLabel);
    emptyLayout->addStretch();
    m_stack->addWidget(emptyPage);

    // Piece page — the contextual property browser for a selected piece.
    m_pieceWidget = new ODetailsPieceWidget(m_stack);
    m_stack->addWidget(m_pieceWidget);

    // Re-emit piece widget signals through the browser.
    connect(m_pieceWidget, &ODetailsPieceWidget::ShowStatusTip, this, &ODetailsPropertyBrowser::ShowStatusTip);
    connect(m_pieceWidget, &ODetailsPieceWidget::HighlightNode, this, &ODetailsPropertyBrowser::HighlightNode);
    connect(m_pieceWidget, &ODetailsPieceWidget::HighlightTemporaryNode, this, &ODetailsPropertyBrowser::HighlightTemporaryNode);
    connect(m_pieceWidget, &ODetailsPieceWidget::ClearHighlight, this, &ODetailsPropertyBrowser::ClearHighlight);

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
    m_emptyLabel->setText(tr("Select a piece"));
    m_pieceWidget->RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowEmpty()
{
    m_stack->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::Clear()
{
    m_pieceWidget->Clear();
    m_stack->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowPieceOptions(VToolSeamAllowance *tool)
{
    m_pieceWidget->Show(tool);
    m_stack->setCurrentWidget(m_pieceWidget);
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
