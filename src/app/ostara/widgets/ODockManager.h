/************************************************************************
 **
 **  @file   ODockManager.h
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   26/08/2026
 **
 **  @brief  Ownership and orchestration of the MainWindow docks: custom
 **          title bars, Window-menu toggle actions, dock visibility
 **          booleans and per-mode dock layouts.
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

#ifndef ODOCKMANAGER_H
#define ODOCKMANAGER_H

#include <QByteArray>
#include <QList>
#include <QObject>

#include "def.h"

class QAction;
class QDockWidget;
class QMainWindow;
class QStackedWidget;
class VToolOptionsPropertyBrowser;
class ODetailsPropertyBrowser;
class OWidgetSizeRanges;
class OWidgetSizeStyles;
class OWidgetOutputs;
class OWidgetCoverPage;
class VWidgetGroups;
class VWidgetDetails;
class VWidgetDependencies;
class VWidgetBackgroundImages;
class VContainer;
class VPattern;

namespace Ui
{
class MainWindow;
}

/**
 * @brief The ODockManager class owns the MainWindow dock widgets and their
 *        per-mode behaviour: custom title bars, Window-menu toggle actions,
 *        visibility booleans and per-mode dock layouts. Extracted from the
 *        MainWindow god object (dispatch Phase 1 — continuation): MainWindow
 *        stays a composition root, this class carries the dock orchestration.
 */
class ODockManager final : public QObject
{
    Q_OBJECT

public:
    explicit ODockManager(Ui::MainWindow *ui, QMainWindow *host, QObject *parent = nullptr);

    /**
     * @brief Build dock content widgets, install custom title bars and create
     *        the Window-menu toggle actions. Called once from
     *        MainWindow::InitDocksContain().
     */
    void init(VContainer *pattern, VPattern *doc);

    // ── Content widgets (created by the manager; owned by the dock tree) ──

    VToolOptionsPropertyBrowser *toolOptions() const;
    ODetailsPropertyBrowser *detailsOptions() const;
    VWidgetGroups *groupsWidget() const;
    VWidgetDetails *detailsWidget() const;
    OWidgetSizeRanges *sizeRangesWidget() const;
    VWidgetBackgroundImages *backgroundImagesWidget() const;
    VWidgetDependencies *dependenciesWidget() const;
    OWidgetSizeStyles *sizeStylesWidget() const;
    OWidgetOutputs *outputsWidget() const;
    OWidgetCoverPage *coverPageWidget() const;

    // ── Dock orchestration ──

    /** @brief Sync each preference dock's visibility to its boolean member. */
    void applyDockPreferences();

    /** @brief Sync boolean members from the docks' actual visibility. */
    void syncVisibilityFromDocks();

    /// @brief True when at least one of the docks the Draw mode is built around is on screen.
    ///
    /// A saved layout can be restored with every one of them hidden — a session closed at the wrong
    /// moment saves exactly that, and every later start then opens an empty Draw mode. The caller uses
    /// this to reject such a layout instead of trusting it.
    bool anyDrawDockVisible() const;

    /// @brief Puts the default Draw layout back: Options, Layers with its messages tabbed under it.
    void restoreDefaultDockVisibility();

    /** @brief Hide every editor dock (WelcomeScreen state). */
    void hideAllDocks();

    /** @brief Remember the editor dock layout before hiding for the Welcome screen. */
    void captureEditorDockState();

    /** @brief Restore the editor dock layout saved by ShowWelcomeScreen(). */
    void restoreEditorDockState();

    /** @brief Remember the current dock layout for the given mode. */
    void saveDockStateForMode(Draw mode);

    /** @brief Restore the dock layout saved for the given mode. */
    void restoreDockStateForMode(Draw mode);

    /** @brief Apply the per-mode dock content/visibility (one "Options" dock,
     *        per-mode title, Options/Groups stack pages). Ends by restoring the
     *        mode's persisted dock layout. */
    void applyModeDocks(Draw mode);

    /** @brief Re-apply per-mode dock titles/tooltips (LanguageChange). */
    void retranslate();

    // ── Window-menu toggle actions ──

    QAction *actionOptions() const;
    QAction *actionGroups() const;
    QAction *actionBackgroundImages() const;

    // Dock visibility booleans are internal; per-mode visibility persists via
    // the per-mode dock layouts (saveState/restoreState).

    // ── Per-mode + editor dock layouts ──

    QByteArray editorDockState() const;
    QByteArray drawModeDockState() const;
    QByteArray detailsModeDockState() const;
    QByteArray gradedModeDockState() const;
    QByteArray exportModeDockState() const;

    void setDrawModeDockState(const QByteArray &state);
    void setDetailsModeDockState(const QByteArray &state);
    void setGradedModeDockState(const QByteArray &state);
    void setExportModeDockState(const QByteArray &state);

private:
    void installTitleBar(QDockWidget *dock);
    void buildOptionsDock();
    void buildGroupsDock();

    /** @brief Force-hide the docks that can never appear in the given mode, and
     *         hide their toggle action so they do not show up in the Window menu
     *         or in the right-click menu either. Called after restoreState(),
     *         which would otherwise re-show them from the mode's saved layout.
     *         The mode's own docks stay user-arrangeable. */
    void enforceModeDockExclusions(Draw mode);

    /** @brief Every dock the manager orchestrates. */
    QList<QDockWidget *> allDocks() const;

    Ui::MainWindow *m_ui{nullptr};
    QMainWindow *m_host{nullptr};

    VToolOptionsPropertyBrowser *m_toolOptionsBrowser{nullptr};
    ODetailsPropertyBrowser *m_detailsOptionsBrowser{nullptr};
    VWidgetGroups *m_groupsContent{nullptr};
    VWidgetDetails *m_detailsContent{nullptr};
    OWidgetSizeRanges *m_sizeRangesContent{nullptr};
    OWidgetSizeStyles *m_sizeStylesContent{nullptr};
    OWidgetOutputs *m_outputsContent{nullptr};
    OWidgetCoverPage *m_coverPageContent{nullptr};
    VWidgetBackgroundImages *m_backgroundImagesContent{nullptr};
    VWidgetDependencies *m_dependenciesContent{nullptr};

    QAction *m_actionOptions{nullptr};
    QAction *m_actionGroups{nullptr};
    QAction *m_actionSizeRanges{nullptr};
    QAction *m_actionSizeStyles{nullptr};
    QAction *m_actionOutputs{nullptr};
    QAction *m_actionCoverPage{nullptr};
    QAction *m_actionBackgroundImages{nullptr};
    QAction *m_actionDependencies{nullptr};
    QAction *m_actionMessages{nullptr};

    bool m_groupsActive{false};
    bool m_optionsActive{false};
    bool m_patternMessagesActive{false};
    bool m_backgroundImagesActive{false};
    bool m_dependenciesActive{false};
    bool m_sizeRangesActive{false};
    bool m_sizeStylesActive{false};
    bool m_outputsActive{false};
    bool m_coverPageActive{false};

    QByteArray m_editorDockState{};
    QByteArray m_drawModeDockState{};
    QByteArray m_detailsModeDockState{};
    QByteArray m_gradedModeDockState{};
    QByteArray m_exportModeDockState{};

    QStackedWidget *m_optionsStack{nullptr};
    QStackedWidget *m_groupsStack{nullptr};
};

#endif // ODOCKMANAGER_H
