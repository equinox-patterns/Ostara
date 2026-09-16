/************************************************************************
 **
 **  @file   mainwindow_events.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   26/08/2026
 **
 **  @brief  Event and item-interaction methods extracted from mainwindow.cpp
 **
 **  @note   Extracted from mainwindow.cpp by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara refactoring.
 **
 **  @copyright
 **  Copyright (C) 2013-2015 Valentina project
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mainwindow_helpers.h"
#include "mainwindow_graded_helpers.h"

#include "WelcomeScreen.h"
#include "core/odetailspropertybrowser.h"
#include "core/vapplication.h"
#include "core/vtooloptionspropertybrowser.h"
#include "dialogs/vwidgetdependencies.h"
#include "dialogs/vwidgetdetails.h"
#include "vtheme.h"
#include "widgets/BreadcrumbBar.h"
#include "widgets/DrawToolBar.h"
#include "widgets/ODockManager.h"

#include "../ifc/ifcdef.h"
#include "../vmisc/customevents.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "../vtools/dialogs/tools/dialogtool.h"
#include "../vtools/tools/nodeDetails/vnodepoint.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vwidgets/vmaingraphicsscene.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMimeType>
#include <QPainter>
#include <QPalette>
#include <QShowEvent>
#include <QUndoStack>
#include <QUrl>

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief showEvent handle after show window.
 * @param event show event.
 */
void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (event->spontaneous())
    {
        return;
    }

#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_taskbarButton->setWindow(windowHandle());
#endif

    if (m_isInitialized)
    {
        return;
    }
    // do your init stuff here

    MinimumScrollBar();

    VTheme::Instance()->ApplyThemeToWindows();

    // Notify ourselves that the theme is applied so the pen style combo
    // and other palette-dependent widgets are populated with correct colors
    QEvent themeEvent(QEvent::ThemeChange);
    QCoreApplication::sendEvent(this, &themeEvent);

    m_isInitialized = true; // first show windows are held
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().window().color());
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        // retranslate designer form (single inheritance approach)
        ui->retranslateUi(this);
        m_welcomeScreen->RetranslateUI();
        undoAction->setText(tr("&Undo"));
        redoAction->setText(tr("&Redo"));
        statusBar()->showMessage(tr("Changes applied."), 5000);
        m_patternPieceLabel->setText(tr("Piece:"));

        m_dockManager->retranslate();

        UpdateMeasurementState();


        if (VApplication::VApp()->ValentinaSettings()->IsUseToolGroups())
        {
            m_drawToolBar->SetMode(VApplication::VApp()->GetDrawMode());
        }

        // Re-translate the docks and toolbar (children of the QMainWindow do not
        // receive QEvent::LanguageChange on their own).
        m_drawToolBar->RetranslateGroups();
        if (m_dockManager->detailsOptions())
        {
            m_dockManager->detailsOptions()->RetranslateUI();
        }
        if (m_dockManager->toolOptions())
        {
            m_dockManager->toolOptions()->RetranslateUI();
        }

        UpdateWindowTitle();
        emit m_sceneDetails->LanguageChanged();

        if (m_globalPenStyleLabel)
        {
            m_globalPenStyleLabel->setText(tr("Pen style:"));
        }
        if (m_globalColorLabel)
        {
            m_globalColorLabel->setText(tr("Color:"));
        }

        // Rebuild color combo for translated names.
        if (m_colorCombo)
        {
            const QString storedName = GetColorComboCurrentName(m_colorCombo);
            InitColorCombo(m_colorCombo);
            SetColorComboCurrentName(m_colorCombo, storedName);
        }
    }

    if (event->type() == QEvent::ThemeChange)
    {
        VTheme::Instance()->ResetColorScheme();

        if (m_comboBoxPenStyle)
        {
            const QSignalBlocker blocker(m_comboBoxPenStyle);
            m_comboBoxPenStyle->clear();
            FillComboBoxTypeLine(m_comboBoxPenStyle,
                                 LineStylesPics(m_comboBoxPenStyle->palette().color(QPalette::Base),
                                                m_comboBoxPenStyle->palette().color(QPalette::Text)));
            ChangeCurrentData(m_comboBoxPenStyle, VApplication::VApp()->ValentinaSettings()->GetGlobalPenStyle());
        }

        // Rebuild color combo icons for new theme.
        if (m_colorCombo)
        {
            const QString storedName = GetColorComboCurrentName(m_colorCombo);
            InitColorCombo(m_colorCombo);
            SetColorComboCurrentName(m_colorCombo, storedName);
        }

        UpdateTopBarIcons();
        UpdateMenuIcons();
        UpdateMessagesIcons();

        // Re-resolve the color of every canvas item from the freshly loaded theme tables.
        RefreshSceneItemPens(m_sceneDraw);
        RefreshSceneItemPens(m_sceneDetails);

        // The graded silhouettes are layout items (not VToolSeamAllowance) → re-tint them with the
        // freshly loaded piece color so Graded follows Light/Dark like Draw/Details do. This is also
        // the "gate" for the per-size trace styles: their fallback color is the very same piece color.
        if (m_sceneGraded != nullptr && not m_sceneGraded->items().isEmpty())
        {
            ApplyGradedSizeStylesToScene();
        }
    }

    // remember to call base class implementation
    QMainWindow::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief closeEvent handle after close window.
 * @param event close event.
 */
void MainWindow::closeEvent(QCloseEvent *event)
{
    qCDebug(vMainWindow, "Closing main window");
    if (MaybeSave())
    {
        FileClosedCorrect();

        event->accept();
        QApplication::closeAllWindows();
    }
    else
    {
        qCDebug(vMainWindow, "Closing canceled.");
        event->ignore();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::customEvent(QEvent *event)
{
    if (event->type() == UNDO_EVENT)
    {
        VAbstractApplication::VApp()->getUndoStack()->undo();
    }
    else if (event->type() == FIT_BEST_CURRENT_EVENT)
    {
        ZoomFitBestCurrent();
    }
    else if (event->type() == WARNING_MESSAGE_EVENT)
    {
        PrintPatternMessage(event);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (const QMimeData *mime = event->mimeData(); mime != nullptr && mime->hasText())
    {
        if (QUrl const urlPath(mime->text().simplified()); urlPath.isLocalFile())
        {
            if (QMimeType const mimeType = QMimeDatabase().mimeTypeForFile(urlPath.toLocalFile(),
                                                                     QMimeDatabase::MatchDefault);
                mimeType.name() == "application/xml"_L1 || mimeType.name().endsWith("+xml"_L1))
            {
                event->acceptProposedAction();
            }
            else
            {
                event->ignore();
            }
        }
    }

    MainWindowsNoGUI::dragEnterEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::dragMoveEvent(QDragMoveEvent* event)
{
    if (const QMimeData *mime = event->mimeData(); mime != nullptr && mime->hasText())
    {
        if (QUrl const urlPath(mime->text().simplified()); urlPath.isLocalFile())
        {
            if (QMimeType const mimeType = QMimeDatabase().mimeTypeForFile(urlPath.toLocalFile(),
                                                                           QMimeDatabase::MatchDefault);
                mimeType.name() == "application/xml"_L1 || mimeType.name().endsWith("+xml"_L1))
            {
                event->acceptProposedAction();
            }
            else
            {
                event->ignore();
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    event->accept();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::dropEvent(QDropEvent* event)
{
    if (const QMimeData *mime = event->mimeData(); mime != nullptr && mime->hasText())
    {
        if (QUrl const urlPath(mime->text().simplified()); urlPath.isLocalFile())
        {
            QString const filePath = urlPath.toLocalFile();
            if (QMimeType const mimeType = QMimeDatabase().mimeTypeForFile(filePath, QMimeDatabase::MatchDefault);
                mimeType.name() == "application/xml"_L1 || mimeType.name().endsWith("+xml"_L1))
            {
                LoadPattern(filePath);
                event->acceptProposedAction();
            }
            else
            {
                event->ignore();
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::HandleItemClicked(QGraphicsItem *item)
{
    // Export mode has no piece to select. Its canvas is a drawing of a nesting result, not a working
    // scene: the items are neither tools nor nodes, and nothing downstream of this handler has anything
    // to say about them. Letting the generic path run cleared the isolation, deselected the detail and
    // hid the breadcrumb of a pattern the user was not even looking at. The mode's own clicks — a
    // footprint to correct by hand — are routed by OExportCorrection, never from here.
    //
    // Deliberately before the line below: no node-picking workflow is ever open in Export, so arming
    // m_pickingClick there would leave a flag set that nothing ever reads or clears.
    if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Export)
    {
        return;
    }

    // This click is routed to a node-picking workflow (if any is waiting for clicks) before the scene
    // sees it. Freeze that answer here: the workflow may end on this very click — the second point of a
    // custom seam allowance, a foldline, a group link — and reset its state before the scene re-reads it,
    // which would select the clicked node and drop the Details dock tab the user is working in.
    m_pickingClick = IsNodePickingActive();

    // Group Link — step 1: route piece clicks to the pieces dock.
    if (item != nullptr && m_dockManager->detailsWidget() != nullptr && m_dockManager->detailsWidget()->IsSelectingLinkPieces())
    {
        auto *pieceTool = dynamic_cast<VToolSeamAllowance *>(item);
        if (pieceTool == nullptr)
        {
            if (auto *parentItem = item->parentItem())
            {
                pieceTool = dynamic_cast<VToolSeamAllowance *>(parentItem);
            }
        }
        if (pieceTool != nullptr)
        {
            m_dockManager->detailsWidget()->OnPieceClickedForGroup(pieceTool->getId());
            return;
        }
    }

    // Group Link — step 2: route the common-point clicks to the pieces dock.
    if (item != nullptr && m_dockManager->detailsWidget() != nullptr && m_dockManager->detailsWidget()->IsSelectingLinkNode())
    {
        auto *node = dynamic_cast<VNodePoint *>(item);
        if (node == nullptr)
        {
            if (auto *parentItem = item->parentItem())
            {
                node = dynamic_cast<VNodePoint *>(parentItem);
            }
        }
        if (node != nullptr)
        {
            m_dockManager->detailsWidget()->OnNodeSelected(node->getId());
            return;
        }
    }

    // Group member foldline definition — route node clicks to the group widget.
    if (item != nullptr && m_dockManager->detailsOptions() != nullptr &&
        m_dockManager->detailsOptions()->IsSelectingGroupFoldLinePoints())
    {
        auto *node = dynamic_cast<VNodePoint *>(item);
        if (node == nullptr)
        {
            if (auto *parentItem = item->parentItem())
            {
                node = dynamic_cast<VNodePoint *>(parentItem);
            }
        }
        if (node != nullptr)
        {
            m_dockManager->detailsOptions()->OnNodeSelectedForGroupFoldLine(node->getId());
            return;
        }
    }

    // Forced alignment point — route node clicks to the details property browser.
    if (item != nullptr && m_dockManager->detailsOptions() != nullptr
        && m_dockManager->detailsOptions()->IsSelectingAlignPoint())
    {
        auto *node = dynamic_cast<VNodePoint *>(item);
        if (node == nullptr)
        {
            if (auto *parentItem = item->parentItem())
            {
                node = dynamic_cast<VNodePoint *>(parentItem);
            }
        }
        if (node != nullptr)
        {
            m_dockManager->detailsOptions()->OnNodeSelectedForAlignPoint(node->getId());
            return;
        }
    }

    // Passmark multi-selection — route node clicks to the details property browser.
    if (item != nullptr && m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsSelectingPassmarkPoints())
    {
        auto *node = dynamic_cast<VNodePoint *>(item);
        if (node == nullptr)
        {
            if (auto *parentItem = item->parentItem())
            {
                node = dynamic_cast<VNodePoint *>(parentItem);
            }
        }
        if (node != nullptr)
        {
            m_dockManager->detailsOptions()->OnNodeSelectedForPassmark(node->getId());
            return;
        }
    }

    // Foldline definition — route node clicks to the details property browser.
    if (item != nullptr && m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsSelectingFoldLinePoints())
    {
        auto *node = dynamic_cast<VNodePoint *>(item);
        if (node == nullptr)
        {
            if (auto *parentItem = item->parentItem())
            {
                node = dynamic_cast<VNodePoint *>(parentItem);
            }
        }
        if (node != nullptr)
        {
            m_dockManager->detailsOptions()->OnNodeSelectedForFoldLine(node->getId());
            return;
        }
    }

    // Custom margin selection — route node clicks to the details property browser.
    if (item != nullptr && m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsSelectingSAPoints())
    {
        auto *node = dynamic_cast<VNodePoint *>(item);
        if (node == nullptr)
        {
            if (auto *parentItem = item->parentItem())
            {
                node = dynamic_cast<VNodePoint *>(parentItem);
            }
        }
        if (node != nullptr)
        {
            m_dockManager->detailsOptions()->OnNodeSelectedForSA(node->getId());
            return;
        }
    }

    if (doc == nullptr || m_breadcrumb == nullptr)
    {
        m_dockManager->toolOptions()->itemClicked(item);
        return;
    }

    const vidtype id = VWidgetDependencies::ItemToId(item);

    // Clicking a member of an attachment enters the group level.
    if (id != NULL_ID && not doc->GetAttachmentItemsForObject(id).isEmpty())
    {
        const QMap<quint32, QString> attachments = doc->GetAttachmentsContainingItem(id, id, true);
        if (not attachments.isEmpty())
        {
            const quint32 attachmentId = attachments.firstKey();
            const QString groupName = attachments.first();
            doc->SetIsolatedAttachment(attachmentId);
            m_dockManager->toolOptions()->ShowGroupOptions(groupName, attachmentId);
            m_breadcrumb->Update(groupName, QString());
            return;
        }
    }

    // Otherwise: normal object selection, back to the canvas level.
    doc->ClearIsolation();
    m_dockManager->toolOptions()->itemClicked(item);
    m_breadcrumb->hide();

    // Empty canvas click → deselect the current piece (Details mode).
    if (item == nullptr)
    {
        doc->SelectedDetail(NULL_ID);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::HandleItemHovered(QGraphicsItem *item)
{
    // Custom seam allowance selection — route node hover to the details property browser.
    if (m_dockManager->detailsOptions() == nullptr || !m_dockManager->detailsOptions()->IsSelectingSAPoints())
    {
        return;
    }

    if (item == nullptr)
    {
        m_dockManager->detailsOptions()->OnNodeHoveredForSA(0);
        return;
    }

    auto *node = dynamic_cast<VNodePoint *>(item);
    if (node == nullptr)
    {
        if (auto *parentItem = item->parentItem())
        {
            node = dynamic_cast<VNodePoint *>(parentItem);
        }
    }

    m_dockManager->detailsOptions()->OnNodeHoveredForSA(node != nullptr ? node->getId() : 0);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ItemDoubleClicked(QGraphicsItem *item)
{
    if (doc == nullptr || m_breadcrumb == nullptr)
    {
        return;
    }

    const vidtype id = VWidgetDependencies::ItemToId(item);
    if (id == NULL_ID || doc->GetAttachmentItemsForObject(id).isEmpty())
    {
        return;
    }

    const QMap<quint32, QString> attachments = doc->GetAttachmentsContainingItem(id, id, true);
    const quint32 attachmentId = attachments.isEmpty() ? NULL_ID : attachments.firstKey();
    const QString groupName = attachments.isEmpty() ? tr("Group") : attachments.first();

    doc->SetIsolatedAttachment(attachmentId);
    doc->SetIsolatedObject(id);

    m_dockManager->toolOptions()->itemClicked(item);

    QString objectName = tr("Object");
    if (pattern != nullptr)
    {
        if (const auto gobject = pattern->GetGObject(id); not gobject.isNull())
        {
            objectName = gobject->ObjectName();
        }
    }
    m_breadcrumb->Update(groupName, objectName);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief mouseMove save mouse position and show user.
 * @param scenePos position mouse.
 */
void MainWindow::MouseMove(const QPointF &scenePos)
{
    if (not m_mouseCoordinate.isNull())
    {
        m_mouseCoordinate->setText(
            QStringLiteral("%1, %2 (%3)")
                .arg(static_cast<qint32>(VAbstractValApplication::VApp()->fromPixel(scenePos.x())))
                .arg(static_cast<qint32>(VAbstractValApplication::VApp()->fromPixel(scenePos.y())))
                .arg(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true)));
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief keyPressEvent handle key press events.
 * @param event key event.
 */
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
        case Qt::Key_Escape:
            if (m_dockManager->detailsWidget() != nullptr &&
                (m_dockManager->detailsWidget()->IsSelectingLinkPieces() ||
                 m_dockManager->detailsWidget()->IsSelectingLinkNode()))
            {
                m_dockManager->detailsWidget()->CancelGroupSelection();
                break;
            }
            if (m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsSelectingPassmarkPoints())
            {
                m_dockManager->detailsOptions()->CancelPassmarkSelection();
                break;
            }
            if (m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsSelectingFoldLinePoints())
            {
                m_dockManager->detailsOptions()->CancelFoldLineSelection();
                break;
            }
            if (m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsSelectingGroupFoldLinePoints())
            {
                m_dockManager->detailsOptions()->CancelGroupFoldLineSelection();
                break;
            }
            if (m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsSelectingSAPoints())
            {
                m_dockManager->detailsOptions()->CancelSASelection();
                break;
            }
            if (m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsSelectingAlignPoint())
            {
                m_dockManager->detailsOptions()->CancelAlignPointSelection();
                break;
            }
            ArrowTool(true);
            break;
        case Qt::Key_V:
            ArrowTool(true);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (m_dockManager->detailsWidget() != nullptr &&
                (m_dockManager->detailsWidget()->IsSelectingLinkPieces() ||
                 m_dockManager->detailsWidget()->IsSelectingLinkNode()))
            {
                m_dockManager->detailsWidget()->FinishGroupSelection();
                break;
            }
            if (m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsSelectingPassmarkPoints())
            {
                m_dockManager->detailsOptions()->FinishPassmarkSelection();
                break;
            }
            EndVisualization();
            break;
        default:
            break;
    }
    QMainWindow::keyPressEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::EndVisualization(bool click)
{
    if (m_dialogTool.isNull())
    {
        return;
    }

    if (m_dialogTool->VisualizationBroken())
    {
        // The scene was fully reparsed (e.g. another tool was deleted) while this creation dialog was open,
        // which deleted its visualization item. Its ShowDialog() override would dereference the now-freed vis
        // and crash. Ignore the event; the user can still cancel the half-finished tool.
        qCDebug(vMainWindow, "Ignoring scene event: the open tool dialog's visualization was destroyed by a "
                             "reparse (scene cleared). Nothing to finish.");
        return;
    }

    m_dialogTool->ShowDialog(click);
}
