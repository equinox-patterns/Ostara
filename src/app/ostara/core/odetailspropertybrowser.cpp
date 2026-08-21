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
#include <QVBoxLayout>

ODetailsPropertyBrowser::ODetailsPropertyBrowser(QDockWidget *parent)
  : QObject(parent)
{
    // ── Contextual stack — empty page shown until a piece (or point / place label / group) is selected ──
    m_stack = new QStackedWidget();

    auto *emptyPage = new QWidget(m_stack);
    auto *emptyLayout = new QVBoxLayout(emptyPage);
    emptyLayout->setContentsMargins(0, 0, 0, 0);
    auto *emptyLabel = new QLabel(tr("Select a piece"), emptyPage);
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLayout->addWidget(emptyLabel);
    m_stack->addWidget(emptyPage);

    // Piece page — the contextual property browser for a selected piece.
    m_pieceWidget = new ODetailsPieceWidget(m_stack);
    m_stack->addWidget(m_pieceWidget);

    // Re-emit piece widget signals through the browser.
    connect(m_pieceWidget, &ODetailsPieceWidget::ShowStatusTip, this, &ODetailsPropertyBrowser::ShowStatusTip);
    connect(m_pieceWidget, &ODetailsPieceWidget::HighlightNode, this, &ODetailsPropertyBrowser::HighlightNode);
    connect(m_pieceWidget, &ODetailsPieceWidget::ClearHighlight, this, &ODetailsPropertyBrowser::ClearHighlight);

    // ── Same structure as the draw-mode Options dock (VToolOptionsPropertyBrowser):
    //    QDockWidget → QScrollArea → scrollContents → content ──
    auto *scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);

    auto *scrollContents = new QWidget(scroll);
    auto *scrollLayout = new QVBoxLayout(scrollContents);
    scrollLayout->setContentsMargins(0, 8, 0, 0);
    scrollLayout->setSpacing(0);
    scrollLayout->addWidget(m_stack);
    scroll->setWidget(scrollContents);

    parent->setWidget(scroll);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPropertyBrowser::ShowEmpty()
{
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
