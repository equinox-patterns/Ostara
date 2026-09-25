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
#include "core/opiecegroupholderitem.h"
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
#include <QTimer>
#include <QUndoStack>
#include <QUrl>

namespace
{
/// How long a close remembered during a run waits before asking again whether a dialog has gone.
constexpr int kDeferredCloseRetryMs = 50;
} // namespace

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
        // Instrumentation laissée volontairement dans le journal : elle répond à la seule question
        // qui compte quand l'interface ne se retraduit pas — l'événement est-il arrivé jusqu'ici ?
        // Si cette ligne n'apparaît pas après une bascule de langue, c'est que Qt ne l'a pas posté
        // (ou l'a posté sans le livrer), et c'est `Preferences()` qui fait le travail à la place.
        qDebug() << "MainWindow::changeEvent — LanguageChange reçu";
        RetranslateAll();
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

    // **A close is not carried out while a run of our own is in flight.** The window is deleted on
    // close, and a deferred deletion is performed by whichever event loop is running when control
    // returns to one — Qt, QObject::deleteLater(). The export flow waits on a loop of its own, and
    // the nesting engine has no abort hook, so that loop outlives the dialog that announces it: a
    // close asked for during a run was carried out by the run's own loop, destroying this window
    // while the run was still working through it. The request is remembered and granted the moment
    // the run is over — see OWorkInFlight, which does both.
    if (m_workInFlight > 0)
    {
        qCDebug(vMainWindow, "Closing deferred: a run is in flight.");
        m_closeRequested = true;
        event->ignore();
        return;
    }

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
void MainWindow::BeginWorkInFlight()
{
    ++m_workInFlight;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::EndWorkInFlight()
{
    if (m_workInFlight > 0)
    {
        --m_workInFlight;
    }

    if (m_workInFlight > 0 || !m_closeRequested)
    {
        return;
    }

    // **Granted from the loop that asked, not from this call.** A run unwinds through methods of
    // this window — its own callers among them — so closing here would delete the window while they
    // are still on the stack, which is the very thing being prevented. A queued call lands once
    // control is back on the event loop the run was started from.
    QTimer::singleShot(0, this, &MainWindow::ApplyDeferredClose);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ApplyDeferredClose()
{
    if (m_workInFlight > 0 || !m_closeRequested)
    {
        return;
    }

    // **A dialog of its own would carry out the deletion this is deferring.** A modal runs a loop
    // of its own, and Qt performs a deferred deletion in whichever loop is running when control
    // returns to one — closing now, with a dialog up, would therefore delete this window inside the
    // dialog's loop, while the method that opened the dialog is still on the stack. The moment
    // passes when the dialog does.
    if (QApplication::activeModalWidget() != nullptr)
    {
        QTimer::singleShot(kDeferredCloseRetryMs, this, &MainWindow::ApplyDeferredClose);
        return;
    }

    m_closeRequested = false;
    close();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::customEvent(QEvent *event)
{
    if (event->type() == UNDO_EVENT)
    {
        // Through the group, never the application's stack directly. This is a second undo path
        // that bypasses the Undo action entirely, so pointing it at the pattern's stack would
        // have left it able to undo a Draw-mode edit from inside Export long after the action
        // itself had learned better.
        //
        // The group answers for whatever stack the mode on screen is using, which is also why
        // this needs no Draw::Export branch of its own.
        m_undoGroup->undo();
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
//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::CancelGroupThumbnailEdit() -> bool
{
    // A group's miniatures are edited on the group's own frame, so the piece page knows nothing about
    // them: Escape has to ask the holders themselves.
    for (auto it = m_groupHolders.cbegin(); it != m_groupHolders.cend(); ++it)
    {
        OPieceGroupHolderItem *holder = it.value();
        if (holder != nullptr && holder->IsEditingThumbnail())
        {
            holder->CancelThumbnailEdit();
            return true;
        }
    }
    return false;
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
            // Export first, and before the picking cancellations below. Those all belong to the
            // Details canvases, and not one of them can be armed on a plan — letting Escape fall
            // through to a workflow the user cannot even see would be answering a question
            // nobody asked. On the plan, Escape means one thing: let go of the piece.
            if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Export)
            {
                m_exportCorrection->Deselect();
                break;
            }
            if (m_dockManager->detailsWidget() != nullptr &&
                (m_dockManager->detailsWidget()->IsSelectingLinkPieces() ||
                 m_dockManager->detailsWidget()->IsSelectingLinkNode()))
            {
                m_dockManager->detailsWidget()->CancelGroupSelection();
                break;
            }
            if (m_dockManager->detailsOptions() != nullptr && m_dockManager->detailsOptions()->IsEditingThumbnail())
            {
                // The innermost gesture on the Details canvas: Escape gave the miniature, Escape
                // takes it back.
                m_dockManager->detailsOptions()->CancelThumbnailEdit();
                break;
            }
            if (CancelGroupThumbnailEdit())
            {
                // Same gesture, same key — but the block belongs to a group, whose miniatures are
                // edited on the group's frame and not through the piece page.
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

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RetranslateAll()
{
    // ⚠️ **Cette fonction existe parce que l'événement ne suffit pas.**
    //
    // La doc Qt dit qu'un `LanguageChange` posté parcourt tout l'arbre des widgets. En pratique, dans
    // cette application, une bascule de langue ne retraduisait RIEN : le journal s'arrête net à
    // « Checked locale ». Elle est donc appelée de deux façons — par `changeEvent` quand l'événement
    // arrive bien, et **directement** par `MainWindow::Preferences()`, juste après la fermeture du
    // dialogue, là où la langue vient d'être changée. Elle est idempotente : l'appeler deux fois ne
    // coûte rien et ne casse rien.
    ui->retranslateUi(this);
    m_welcomeScreen->RetranslateUI();
    // Undo and Redo are plain actions whose text nothing else ever writes, so this is the one
    // place that retranslates them — and it works here, which it did not while they were
    // built by a QUndoGroup: the group rewrote the text on every index change and would have
    // undone this at the next gesture.
    undoAction->setText(tr("&Undo"));
    redoAction->setText(tr("&Redo"));
    statusBar()->showMessage(tr("Changes applied."), 5000);
    m_patternPieceLabel->setText(tr("Piece:"));

    // Ce que le .ui ne porte pas : menu du Dock macOS, libellé d'état de « Show all labels », et les
    // infobulles construites depuis le texte de leur action. Appelé après `retranslateUi`, qui écrase
    // justement ces dernières.
    RetranslateActions();
    if (m_breadcrumb != nullptr)
    {
        m_breadcrumb->RetranslateUI();
    }

    m_dockManager->retranslate();

    UpdateMeasurementState();

    if (VApplication::VApp()->ValentinaSettings()->IsUseToolGroups())
    {
        m_drawToolBar->SetMode(VApplication::VApp()->GetDrawMode());
    }

    // Le dock manager est un QObject, pas un widget : il ne reçoit jamais `LanguageChange`, c'est son
    // appelant qui doit le reprendre — d'où `retranslate()` juste au-dessus.
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
