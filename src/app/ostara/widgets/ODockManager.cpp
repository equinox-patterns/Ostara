/************************************************************************
 **
 **  @file   ODockManager.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/08/2026
 **
 **  @brief  Implementation of ODockManager — dock ownership + orchestration.
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

#include "ODockManager.h"

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QtDebug>

#include "../ui_mainwindow.h"
#include "../xml/vpattern.h"
#include "core/odetailspropertybrowser.h"
#include "core/vtooloptionspropertybrowser.h"
#include "dialogs/vwidgetbackgroundimages.h"
#include "dialogs/vwidgetdependencies.h"
#include "dialogs/vwidgetdetails.h"
#include "dialogs/vwidgetgroups.h"
#include "DockWrapper.h"
#include "ODockTitleBar.h"
#include "projectversion.h"
#include "vabstractvalapplication.h"

//---------------------------------------------------------------------------------------------------------------------
ODockManager::ODockManager(Ui::MainWindow *ui, QMainWindow *host, QObject *parent)
  : QObject(parent),
    m_ui(ui),
    m_host(host)
{
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::init(VContainer *pattern, VPattern *doc)
{
    // ── Content widgets ──

    m_toolOptionsBrowser = new VToolOptionsPropertyBrowser(m_ui->dockWidgetOptions);
    m_detailsOptionsBrowser = new ODetailsPropertyBrowser(m_ui->dockWidgetOptions);
    buildOptionsDock();

    m_groupsContent = new VWidgetGroups(doc, m_host);
    m_detailsContent = new VWidgetDetails(pattern, doc, m_host);
    buildGroupsDock();

    m_backgroundImagesContent = new VWidgetBackgroundImages(doc, m_host);
    m_ui->dockWidgetBackgroundImages->setWidget(m_backgroundImagesContent);

    // Keep only top margin in wrapper layouts — side/bottom margins are handled
    // by QGroupBox QSS (margin-left/right/bottom: 8px). The top margin from the
    // layout compensates for QSS margin-top being clipped by the scroll area viewport.
    if (auto *bgLayout = m_backgroundImagesContent->layout())
    {
        const int top = bgLayout->contentsMargins().top();
        bgLayout->setContentsMargins(0, top, 0, 0);
    }
    if (auto *scroll = m_backgroundImagesContent->findChild<QScrollArea *>())
    {
        if (auto *contents = scroll->widget())
        {
            if (auto *innerLayout = contents->layout())
            {
                const int top = innerLayout->contentsMargins().top();
                innerLayout->setContentsMargins(0, top, 0, 0);
            }
        }
    }

    m_dependenciesContent = new VWidgetDependencies(doc, m_host);
    WrapDockContent(m_ui->dockWidgetDependencies, m_dependenciesContent);

    // ── Messages dock — wrap existing .ui content ──
    {
        QWidget *messagesContent = m_ui->dockWidgetMessages->widget(); // dockWidgetContents_4
        // dockWidgetContents_4 is a plain QWidget, so the viewport's
        // .QWidget { background: $DockTitle } cascade colors it with the
        // wrong background. Shield it with its own stylesheet so the
        // QGroupBox $DockBody background shows through instead.
        messagesContent->setObjectName(QStringLiteral("dockMessagesContent"));
        messagesContent->setStyleSheet(
            QStringLiteral("QWidget#dockMessagesContent { background: transparent; }"));
        WrapDockContent(m_ui->dockWidgetMessages, messagesContent);
    }

    // ── Custom title bars ──
    installTitleBar(m_ui->dockWidgetOptions);
    installTitleBar(m_ui->dockWidgetLayoutPages);
    installTitleBar(m_ui->dockWidgetGroups);
    installTitleBar(m_ui->dockWidgetMessages);
    installTitleBar(m_ui->dockWidgetBackgroundImages);
    installTitleBar(m_ui->dockWidgetDependencies);

    // Window menu: one toggle action per dock (re-opens a closed dock).
    m_ui->menuWindow->addSeparator();

    m_actionOptions = m_ui->dockWidgetOptions->toggleViewAction();
    connect(m_actionOptions, &QAction::triggered, this,
            [this](bool checked) { m_optionsActive = checked; });
    m_ui->menuWindow->addAction(m_actionOptions);

    m_actionGroups = m_ui->dockWidgetGroups->toggleViewAction();
    connect(m_actionGroups, &QAction::triggered, this, [this](bool checked) { m_groupsActive = checked; });
    m_ui->menuWindow->addAction(m_actionGroups);

    m_actionMessages = m_ui->dockWidgetMessages->toggleViewAction();
    connect(m_actionMessages, &QAction::triggered, this,
            [this](bool checked) { m_patternMessagesActive = checked; });
    m_ui->menuWindow->addAction(m_actionMessages);

    m_actionBackgroundImages = m_ui->dockWidgetBackgroundImages->toggleViewAction();
    connect(m_actionBackgroundImages, &QAction::triggered, this,
            [this](bool checked) { m_backgroundImagesActive = checked; });
    m_ui->menuWindow->addAction(m_actionBackgroundImages);

    m_actionDependencies = m_ui->dockWidgetDependencies->toggleViewAction();
    connect(m_actionDependencies, &QAction::triggered, this,
            [this](bool checked) { m_dependenciesActive = checked; });
    m_ui->menuWindow->addAction(m_actionDependencies);
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::installTitleBar(QDockWidget *dock)
{
    auto *titleBar = new ODockTitleBar(dock->windowTitle(), dock);
    dock->setTitleBarWidget(titleBar);

    // Keep title in sync
    connect(dock, &QDockWidget::windowTitleChanged, titleBar, &ODockTitleBar::setTitle);

    // Collapse: constrain content height to 0 instead of hiding, so the dock
    // width (and dock area width) is preserved.
    connect(titleBar, &ODockTitleBar::collapseToggled, dock, [dock](bool collapsed)
            {
                if (auto *w = dock->widget())
                {
                    w->setVisible(true); // must stay visible for width
                    w->setMinimumHeight(0);
                    w->setMaximumHeight(collapsed ? 0 : QWIDGETSIZE_MAX);
                }
            });

    // Detach: toggle floating state
    connect(titleBar, &ODockTitleBar::detachRequested, dock, [dock]() { dock->setFloating(!dock->isFloating()); });

    // Track floating state — disable detach button when already floating
    connect(dock, &QDockWidget::topLevelChanged, titleBar, &ODockTitleBar::setFloating);

    // Close: hide the dock (re-open via Window menu). Re-sync the visibility
    // booleans — a title-bar close does not emit the toggle action's triggered.
    connect(titleBar, &ODockTitleBar::closeRequested, this,
            [this, dock]()
            {
                dock->setVisible(false);
                syncVisibilityFromDocks();
            });
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::buildOptionsDock()
{
    m_optionsStack = new QStackedWidget(m_ui->dockWidgetOptions);
    m_optionsStack->addWidget(m_toolOptionsBrowser->widget());    // page 0 — Draw (tool options)
    m_optionsStack->addWidget(m_detailsOptionsBrowser->widget()); // page 1 — Details

    // Root with a top-only margin, matching the pre-merge dock structure so the
    // $DockBody gap between the title bar and the scroll area is preserved.
    auto *root = new QWidget(m_ui->dockWidgetOptions);
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 8, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(m_optionsStack);
    m_ui->dockWidgetOptions->setWidget(root);
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::buildGroupsDock()
{
    // One stack, two persistent pages — no wrapper rebuild on mode switch.
    m_groupsStack = new QStackedWidget(m_ui->dockWidgetGroups);
    m_groupsStack->addWidget(WrapDockContentWidget(m_groupsContent, m_groupsStack));   // page 0 — Layers
    m_groupsStack->addWidget(WrapDockContentWidget(m_detailsContent, m_groupsStack));  // page 1 — Details
    m_ui->dockWidgetGroups->setWidget(m_groupsStack);
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::applyDockPreferences()
{
    m_ui->dockWidgetOptions->setVisible(m_optionsActive);
    m_ui->dockWidgetGroups->setVisible(m_groupsActive);
    m_ui->dockWidgetMessages->setVisible(m_patternMessagesActive);
    m_ui->dockWidgetBackgroundImages->setVisible(m_backgroundImagesActive);
    m_ui->dockWidgetDependencies->setVisible(m_dependenciesActive);
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::syncVisibilityFromDocks()
{
    m_groupsActive = m_ui->dockWidgetGroups->isVisible();
    m_optionsActive = m_ui->dockWidgetOptions->isVisible();
    m_patternMessagesActive = m_ui->dockWidgetMessages->isVisible();
    m_backgroundImagesActive = m_ui->dockWidgetBackgroundImages->isVisible();
    m_dependenciesActive = m_ui->dockWidgetDependencies->isVisible();
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::hideAllDocks()
{
    m_ui->dockWidgetOptions->setVisible(false);
    m_ui->dockWidgetLayoutPages->setVisible(false);
    m_ui->dockWidgetGroups->setVisible(false);
    m_ui->dockWidgetMessages->setVisible(false);
    m_ui->dockWidgetBackgroundImages->setVisible(false);
    m_ui->dockWidgetDependencies->setVisible(false);
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::captureEditorDockState()
{
    m_editorDockState = m_host->saveState(kToolbarStateVersion);
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::restoreEditorDockState()
{
    if (!m_editorDockState.isEmpty() && !m_host->restoreState(m_editorDockState, kToolbarStateVersion))
    {
        qWarning() << "ODockManager: failed to restore editor dock state";
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::saveDockStateForMode(Draw mode)
{
    // During pattern opening the mode functions run back-to-back to set up the
    // initial layout; the dock state isn't user-established yet, so saving it
    // would capture a wrong intermediate layout.
    if (VAbstractValApplication::VApp()->getOpeningPattern())
    {
        return;
    }

    switch (mode)
    {
        case Draw::Calculation:
            m_drawModeDockState = m_host->saveState(kToolbarStateVersion);
            break;
        case Draw::Modeling:
            m_detailsModeDockState = m_host->saveState(kToolbarStateVersion);
            break;
        case Draw::Layout:
            m_layoutModeDockState = m_host->saveState(kToolbarStateVersion);
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::restoreDockStateForMode(Draw mode)
{
    if (VAbstractValApplication::VApp()->getOpeningPattern())
    {
        return;
    }

    switch (mode)
    {
        case Draw::Calculation:
            if (!m_drawModeDockState.isEmpty() &&
                !m_host->restoreState(m_drawModeDockState, kToolbarStateVersion))
            {
                qWarning() << "ODockManager: failed to restore Draw dock state";
            }
            break;
        case Draw::Modeling:
            if (!m_detailsModeDockState.isEmpty() &&
                !m_host->restoreState(m_detailsModeDockState, kToolbarStateVersion))
            {
                qWarning() << "ODockManager: failed to restore Details dock state";
            }
            break;
        case Draw::Layout:
            if (!m_layoutModeDockState.isEmpty() &&
                !m_host->restoreState(m_layoutModeDockState, kToolbarStateVersion))
            {
                qWarning() << "ODockManager: failed to restore Layout dock state";
            }
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::applyModeDocks(Draw mode)
{
    // init() must have run (it creates the stacks + toggle actions).
    if (m_optionsStack == nullptr || m_groupsStack == nullptr || m_actionOptions == nullptr)
    {
        return;
    }

    switch (mode)
    {
        case Draw::Calculation:
            m_ui->dockWidgetLayoutPages->setVisible(false);
            m_optionsStack->setCurrentIndex(0); // tool options
            m_ui->dockWidgetOptions->setWindowTitle(tr("Tool options"));
            m_ui->dockWidgetOptions->setVisible(m_optionsActive);
            m_groupsStack->setCurrentIndex(0); // Layers
            m_ui->dockWidgetGroups->setWindowTitle(tr("Layers"));
            m_ui->dockWidgetGroups->setVisible(m_groupsActive);
            m_ui->dockWidgetGroups->setToolTip(tr("Contains all layers"));
            m_ui->dockWidgetBackgroundImages->setVisible(m_backgroundImagesActive);
            m_ui->dockWidgetDependencies->setVisible(m_dependenciesActive);
            m_ui->dockWidgetMessages->setVisible(m_patternMessagesActive);
            m_actionOptions->setEnabled(true);
            m_actionGroups->setEnabled(true);
            m_actionBackgroundImages->setEnabled(true);
            m_actionDependencies->setEnabled(true);
            break;
        case Draw::Modeling:
            m_ui->dockWidgetLayoutPages->setVisible(false);
            m_groupsStack->setCurrentIndex(1); // Details
            m_ui->dockWidgetGroups->setWindowTitle(tr("Details"));
            m_ui->dockWidgetGroups->setVisible(m_groupsActive);
            m_ui->dockWidgetGroups->setToolTip(tr("Show which details will go in layout"));
            m_optionsStack->setCurrentIndex(1); // details options
            m_ui->dockWidgetOptions->setWindowTitle(tr("Options"));
            m_ui->dockWidgetOptions->setVisible(m_optionsActive);
            m_ui->dockWidgetBackgroundImages->setVisible(false);
            m_ui->dockWidgetDependencies->setVisible(false);
            m_ui->dockWidgetMessages->setVisible(m_patternMessagesActive);
            m_actionOptions->setEnabled(true);
            m_actionBackgroundImages->setDisabled(true);
            m_actionDependencies->setDisabled(true);
            break;
        case Draw::Layout:
            m_ui->dockWidgetLayoutPages->setVisible(true);
            m_ui->dockWidgetOptions->setVisible(false);
            m_ui->dockWidgetGroups->setVisible(false);
            m_ui->dockWidgetMessages->setVisible(false);
            m_ui->dockWidgetBackgroundImages->setVisible(false);
            m_ui->dockWidgetDependencies->setVisible(false);
            m_actionOptions->setDisabled(true);
            m_actionGroups->setDisabled(true);
            m_actionMessages->setDisabled(true);
            m_actionBackgroundImages->setDisabled(true);
            m_actionDependencies->setDisabled(true);
            break;
    }

    restoreDockStateForMode(mode);
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::retranslate()
{
    switch (VAbstractValApplication::VApp()->GetDrawMode())
    {
        case Draw::Calculation:
            m_ui->dockWidgetGroups->setWindowTitle(tr("Layers"));
            m_ui->dockWidgetGroups->setToolTip(tr("Contains all layers"));
            m_ui->dockWidgetOptions->setWindowTitle(tr("Tool options"));
            break;
        case Draw::Modeling:
            m_ui->dockWidgetGroups->setWindowTitle(tr("Details"));
            m_ui->dockWidgetGroups->setToolTip(tr("Show which details will go in layout"));
            m_ui->dockWidgetOptions->setWindowTitle(tr("Options"));
            break;
        case Draw::Layout:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
QAction *ODockManager::actionOptions() const
{
    return m_actionOptions;
}

//---------------------------------------------------------------------------------------------------------------------
QAction *ODockManager::actionGroups() const
{
    return m_actionGroups;
}

//---------------------------------------------------------------------------------------------------------------------
QAction *ODockManager::actionBackgroundImages() const
{
    return m_actionBackgroundImages;
}

//---------------------------------------------------------------------------------------------------------------------
QByteArray ODockManager::editorDockState() const
{
    return m_editorDockState;
}

//---------------------------------------------------------------------------------------------------------------------
QByteArray ODockManager::drawModeDockState() const
{
    return m_drawModeDockState;
}

//---------------------------------------------------------------------------------------------------------------------
QByteArray ODockManager::detailsModeDockState() const
{
    return m_detailsModeDockState;
}

//---------------------------------------------------------------------------------------------------------------------
QByteArray ODockManager::layoutModeDockState() const
{
    return m_layoutModeDockState;
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::setDrawModeDockState(const QByteArray &state)
{
    m_drawModeDockState = state;
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::setDetailsModeDockState(const QByteArray &state)
{
    m_detailsModeDockState = state;
}

//---------------------------------------------------------------------------------------------------------------------
void ODockManager::setLayoutModeDockState(const QByteArray &state)
{
    m_layoutModeDockState = state;
}

//---------------------------------------------------------------------------------------------------------------------
VToolOptionsPropertyBrowser *ODockManager::toolOptions() const
{
    return m_toolOptionsBrowser;
}

//---------------------------------------------------------------------------------------------------------------------
ODetailsPropertyBrowser *ODockManager::detailsOptions() const
{
    return m_detailsOptionsBrowser;
}

//---------------------------------------------------------------------------------------------------------------------
VWidgetGroups *ODockManager::groupsWidget() const
{
    return m_groupsContent;
}

//---------------------------------------------------------------------------------------------------------------------
VWidgetDetails *ODockManager::detailsWidget() const
{
    return m_detailsContent;
}

//---------------------------------------------------------------------------------------------------------------------
VWidgetBackgroundImages *ODockManager::backgroundImagesWidget() const
{
    return m_backgroundImagesContent;
}

//---------------------------------------------------------------------------------------------------------------------
VWidgetDependencies *ODockManager::dependenciesWidget() const
{
    return m_dependenciesContent;
}
