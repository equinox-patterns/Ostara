/************************************************************************
 **
 **  @file   mainwindow_welcome.cpp
 **  @date   09-08-2026
 **
 **  @brief  WelcomeScreen measurement slots for MainWindow.
 **
 **  @note   Created by Sophie Denys <contact@sophiedenys.com>
 **          as part of Ostara WelcomeScreen integration.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys
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
#include "WelcomeScreen.h"
#include "widgets/DrawToolBar.h"
#include "widgets/ODockManager.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vabstractvalapplication.h"
#include "dialogs/DialogNewMeasurements.h"
#include "dialogs/DialogOpenMeasurements.h"
#include "widgets/MeasurementWorkspace/MeasurementWindow.h"
#include "widgets/MeasurementWorkspace/MeasurementWorkspace.h"
#include <QFileInfo>
#include <QStackedWidget>

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::AddRecentMeasurementAndRefresh(const QString &path)
{
    if (path.isEmpty() || !QFileInfo::exists(path))
    {
        return;
    }
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    QStringList files = settings->GetRecentMeasurementFileList();
    files.removeAll(path);
    files.prepend(path);
    while (files.size() > MaxRecentFilesStored)
    {
        files.removeLast();
    }
    settings->SetRecentMeasurementFileList(files);

    m_welcomeScreen->UpdateRecentItems(settings->GetRecentFileList(), files);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::WelcomeNewMeasurement()
{
    DialogNewMeasurements dlg(this);
    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }
    auto *mw = new MeasurementWindow(this);
    mw->SetOrigin(MeasurementWorkspace::Origin::FromHome);
    mw->setAttribute(Qt::WA_DeleteOnClose);
    connect(mw, &MeasurementWindow::measurementsSaved, this, &MainWindow::AddRecentMeasurementAndRefresh);
    mw->NewDocument(dlg.MeasurementsType(), dlg.Unit());
    mw->SetTableName(dlg.TableName());
    if (dlg.UseTemplate())
    {
        // The template only lends its measurement names — no values, no increments, no axes. Without this
        // call the chosen template was silently ignored and the new table came out empty.
        mw->ImportStructure(dlg.TemplatePath());
    }
    mw->show();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::WelcomeOpenMeasurement()
{
    DialogOpenMeasurements dlg(this);
    if (dlg.exec() != QDialog::Accepted)
    {
        return;
    }
    const QString path = dlg.selectedPath();
    if (path.isEmpty())
    {
        return;
    }
    auto *mw = new MeasurementWindow(this);
    mw->SetOrigin(MeasurementWorkspace::Origin::FromHome);
    mw->setAttribute(Qt::WA_DeleteOnClose);
    connect(mw, &MeasurementWindow::measurementsSaved, this, &MainWindow::AddRecentMeasurementAndRefresh);
    mw->show();
    mw->LoadFile(path);
    if (path.endsWith(QStringLiteral(".omt"), Qt::CaseInsensitive)
        || path.endsWith(QStringLiteral(".otu"), Qt::CaseInsensitive))
    {
        AddRecentMeasurementAndRefresh(path);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::WelcomeOpenRecentMeasurement(const QString &path)
{
    if (path.isEmpty())
    {
        return;
    }
    auto *mw = new MeasurementWindow(this);
    mw->SetOrigin(MeasurementWorkspace::Origin::FromHome);
    mw->setAttribute(Qt::WA_DeleteOnClose);
    connect(mw, &MeasurementWindow::measurementsSaved, this, &MainWindow::AddRecentMeasurementAndRefresh);
    mw->show();
    mw->LoadFile(path);
    AddRecentMeasurementAndRefresh(path);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitWelcomePage()
{
    // Pattern signals
    connect(m_welcomeScreen, &WelcomeScreen::NewPatternClicked, this, &MainWindow::on_actionNew_triggered);
    // L'écran d'accueil et File > Open tombent sur le même geste : on demande *où* ouvrir, la bibliothèque
    // d'abord, l'ordinateur ensuite. Le panneau du système reste à un clic, dans « Parcourir les fichiers… ».
    connect(m_welcomeScreen, &WelcomeScreen::OpenPatternClicked, this, &MainWindow::on_actionOpen_triggered);
    connect(m_welcomeScreen, &WelcomeScreen::OpenRecentPattern, this,
            [this](const QString &path) { LoadPattern(path); });

    // Measurement signals (implemented in mainwindow_welcome.cpp)
    connect(m_welcomeScreen, &WelcomeScreen::NewMeasurementClicked, this, &MainWindow::WelcomeNewMeasurement);
    connect(m_welcomeScreen, &WelcomeScreen::OpenMeasurementClicked, this, &MainWindow::WelcomeOpenMeasurement);
    connect(m_welcomeScreen, &WelcomeScreen::OpenRecentMeasurement, this, &MainWindow::WelcomeOpenRecentMeasurement);

    // Features button (dev-only)

}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowWelcomeScreen()
{
    // The Graded dock layout is user-arrangeable (Size Ranges + Size Styles) and is
    // persisted per mode like Draw/Details/Layout — it is NOT dropped here, so its
    // order/sizes survive a pattern close and the next entry into Graded restores them.
    m_stackedWidget->setCurrentWidget(m_welcomeScreen);

    // Apply user dock preferences so the saved editor state reflects the user's
    // choices, not the current mode's forced dock visibility (e.g. Tool options
    // hidden in Details mode). Only once the window is shown: during construction
    // isVisible() is false for every dock, so the boolean members are all false
    // and must not be applied to the layout.
    if (isVisible())
    {
        m_dockManager->applyDockPreferences();
    }

    // Save editor dock state before hiding, so ShowEditor() can restore it
    m_dockManager->captureEditorDockState();

    // Hide editor chrome
    ui->topBar->setVisible(false);
    m_drawToolBar->setVisible(false);
    m_dockManager->hideAllDocks();
    statusBar()->setVisible(false);

    // Disable editor-specific actions
    SetEnableTool(false);
    SetEnableWidgets(false);
    ui->actionClosePattern->setEnabled(false);

    // Neutral window title
    setWindowTitle(tr("Ostara"));
    setWindowFilePath(QString());
    setWindowModified(false);

    // Refresh recent lists
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    m_welcomeScreen->UpdateRecentItems(settings->GetRecentFileList(),
                                       settings->GetRecentMeasurementFileList());
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowEditor()
{
    m_stackedWidget->setCurrentIndex(0); // index 0 = oldCentral (wraps ui->view)

    // Show editor chrome
    ui->topBar->setVisible(true);
    m_drawToolBar->setVisible(true);
    statusBar()->setVisible(true);
    ui->actionClosePattern->setEnabled(true);

    // Restore dock layout saved by ShowWelcomeScreen() (or from ReadSettings at startup)
    m_dockManager->restoreEditorDockState();

    // The restored state carries the visibility of the moment it was captured — on the first entry that is
    // the transient "every dock on screen", before any mode has run. Reading the flags back from those docks
    // (the old syncVisibilityFromDocks() call here) turned that accident into the preference, and every mode
    // change then re-showed all five docks. The stored preference is the reference: it is applied over the
    // restored layout, never read back from it.
    m_dockManager->applyDockPreferences();

    // And whatever the restored layout said, the editor never opens with nothing around the canvas: a dock
    // can be "visible" to Qt while sitting entirely outside the window, which is how a saved layout made
    // every launch after a build look like an empty Draw mode. Sophie, 03-11-2026.
    if (!m_dockManager->anyDrawDockVisible())
    {
        m_dockManager->restoreDefaultDockVisibility();
    }

    // LayoutPages dock is hidden except in Layout mode

    // The editor's docks carry sizes of their own, and restoring a layout saved on a bigger window grows
    // the window past the screen — the welcome screen fits, then the first pattern overflows. Clamp again
    // once the layout has settled.
    QTimer::singleShot(0, this, [this]() { FitMainWindowToScreen(); });
    QTimer::singleShot(150, this,
                       [this]()
                       {
                           // The palette's opening width is lost while the dock area is being built: the
                           // request is made once the layout has settled, and only if the user never chose
                           // a width of their own.
                           m_drawToolBar->ApplyDefaultWidthIfUnset();
                       });

    UpdateWindowTitle();
}
