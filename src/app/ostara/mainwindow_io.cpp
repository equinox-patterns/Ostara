/************************************************************************
 **
 **  @file   mainwindow_io.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  File I/O methods extracted from mainwindow.cpp
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
#include "widgets/ODockManager.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/def.h"
#include "core/vapplication.h"
#include "core/odetailspropertybrowser.h"
#include "core/vtooloptionspropertybrowser.h"
#include "dialogs/DialogOpenMeasurements.h"
#include "dialogs/dialognewpattern.h"
#include "dialogs/vwidgetbackgroundimages.h"
#include "dialogs/vwidgetdependencies.h"
#include "dialogs/vwidgetdetails.h"
#include "widgets/MeasurementWorkspace/MeasurementWindow.h"
#include "../ifc/xml/vdomdocument.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../vformat/vmeasurements.h"
#include "../vexception/vexception.h"
#include "../vmisc/projectversion.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFuture>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QTimer>
#include <QtConcurrent>
#include <chrono>
#include <memory>

using namespace Qt::Literals::StringLiterals;
using namespace std::chrono_literals;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ReadSettings read setting for app.
 */
void MainWindow::ReadSettings()
{
    qCDebug(vMainWindow, "Reading settings.");
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

    if (settings->status() == QSettings::NoError)
    {
        restoreGeometry(settings->GetGeometry());
        restoreState(settings->GetToolbarsState(), kToolbarStateVersion);

        // Load per-mode dock layouts so the first entry into a mode after startup
        // restores its own sizes/visibility instead of inheriting the current mode's.
        // (At startup every mode function runs while openingPattern is true, so
        // SaveDockStateForMode() skips capturing — without this the mode states
        // would stay empty until the user leaves a mode once.)
        m_dockManager->setDrawModeDockState(settings->GetModeToolbarsState(Draw::Calculation));
        m_dockManager->setDetailsModeDockState(settings->GetModeToolbarsState(Draw::Modeling));
        m_dockManager->setLayoutModeDockState(settings->GetModeToolbarsState(Draw::Layout));
        m_dockManager->setGradedModeDockState(settings->GetModeToolbarsState(Draw::Graded));

        // Dock visibility is handled by saveState()/restoreState() — sync the
        // boolean members from actual dock state so ShowEditor() can restore
        // correctly after ShowWelcomeScreen() hides everything.
        m_dockManager->syncVisibilityFromDocks();

        // Scene antialiasing
        ui->view->SetAntialiasing(settings->GetGraphicalOutput());

        // Stack limit
        VAbstractApplication::VApp()->getUndoStack()->setUndoLimit(settings->GetUndoCount());

        // Text under tool buton icon
        ToolBarStyles();

        ToolboxIconSize();

        QFont f = ui->plainTextEditPatternMessages->font();
        f.setPointSize(qMax(settings->GetPatternMessageFontSize(f.pointSize()), 1));
        ui->plainTextEditPatternMessages->setFont(f);

        if (VAbstractShortcutManager *manager = VAbstractValApplication::VApp()->GetShortcutManager())
        {
            manager->UpdateShortcuts();
        }
    }
    else
    {
        qWarning() << tr("Cannot read settings from a malformed .INI file.");
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief WriteSettings save setting for app.
 */
void MainWindow::WriteSettings()
{
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    settings->SetGeometry(saveGeometry());

    // Capture the current mode's true dock layout (sizes included) BEFORE
    // ApplyDockPreferences() re-shows the docks that the mode force-hidden
    // (e.g. Tool options in Details mode) — that re-layout would shrink the
    // Options dock and leak a wrong height into the persisted state. Only on
    // the editor (not the WelcomeScreen, where every dock is hidden).
    if (m_stackedWidget->currentIndex() == 0 && isVisible())
    {
        m_dockManager->saveDockStateForMode(VAbstractValApplication::VApp()->GetDrawMode());
    }

    // Snapshot the true per-mode layouts: ActionDraw(true) below re-saves the
    // current mode's normalized layout into its member, which must not overwrite
    // the states we persist here.
    const QByteArray drawState = m_dockManager->drawModeDockState();
    const QByteArray detailsState = m_dockManager->detailsModeDockState();
    const QByteArray layoutState = m_dockManager->layoutModeDockState();
    const QByteArray gradedState = m_dockManager->gradedModeDockState();

    // When on the WelcomeScreen all docks are hidden — save the last known editor
    // state instead of the (empty) current layout.
    if (m_stackedWidget->currentIndex() == 1 && !m_dockManager->editorDockState().isEmpty())
    {
        settings->SetToolbarsState(m_dockManager->editorDockState());
    }

    // Persist each mode's own dock layout, so a mode change never overwrites
    // the others' sizes/visibility, and the first entry into a mode after
    // startup (when mode states are empty) restores the saved geometry.
    settings->SetModeToolbarsState(Draw::Calculation, drawState);
    settings->SetModeToolbarsState(Draw::Modeling, detailsState);
    settings->SetModeToolbarsState(Draw::Layout, layoutState);
    settings->SetModeToolbarsState(Draw::Graded, gradedState);

    settings->sync();

    ActionDraw(true);

    // Restore the true per-mode layouts: ActionDraw(true) just overwrote the
    // current mode's member with the normalized layout, which must not leak
    // into the next pattern opened in this session.
    m_dockManager->setDrawModeDockState(drawState);
    m_dockManager->setDetailsModeDockState(detailsState);
    m_dockManager->setLayoutModeDockState(layoutState);
    m_dockManager->setGradedModeDockState(gradedState);

    // Startup layout = the Draw mode every run opens on. Capture the global toolbar
    // state AFTER ActionDraw (which normalized the window back to Draw's own dock
    // geometry) so the previous mode's widths — e.g. the Graded Size Styles/Ranges
    // docks — cannot leak into Draw's first shown layout on the next run.
    if (m_stackedWidget->currentIndex() == 0 && isVisible())
    {
        // Normalize dock visibility to the user's preferences before persisting —
        // Draw may have force-hidden docks (e.g. Tool options in Details/Graded).
        m_dockManager->applyDockPreferences();
        settings->SetToolbarsState(saveState(kToolbarStateVersion));
    }

    if (settings->status() == QSettings::AccessError)
    {
        qWarning() << tr("Cannot save settings. Access denied.");
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SavePattern save pattern file.
 * @param fileName pattern file name.
 * @return true if all is good.
 */
auto MainWindow::SavePattern(const QString &fileName, QString &error) -> bool
{
    qCDebug(vMainWindow, "Saving pattern file %s.", qUtf8Printable(fileName));
    QFileInfo const tempInfo(fileName);

    const QString mPath = AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath());
    if (not mPath.isEmpty() && VAbstractValApplication::VApp()->GetPatternPath() != fileName)
    {
        doc->SetMPath(RelativeMPath(fileName, mPath));
    }

    const bool result = doc->SaveDocument(fileName, error);
    if (result)
    {
        if (tempInfo.suffix() != "autosave"_L1)
        {
            setCurrentFile(fileName);
            statusBar()->showMessage(tr("File saved"), 5000);
            qCDebug(vMainWindow, "File %s saved.", qUtf8Printable(fileName));
            PatternChangesWereSaved(result);
        }
    }
    else
    {
        doc->SetMPath(mPath);
        emit doc->UpdatePatternLabel();
        qCDebug(vMainWindow, "Could not save file %s. %s.", qUtf8Printable(fileName), qUtf8Printable(error));
    }
    return result;
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief AutoSavePattern start safe saving.
 */
void MainWindow::AutoSavePattern()
{
    if (VApplication::IsGUIMode() && not VAbstractValApplication::VApp()->GetPatternPath().isEmpty() &&
        isWindowModified() && isNeedAutosave)
    {
        qCDebug(vMainWindow, "Autosaving pattern.");
        QString error;
        if (SavePattern(VAbstractValApplication::VApp()->GetPatternPath() + AutosavePrefix(), error))
        {
            isNeedAutosave = false;
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setCurrentFile the function is called to reset the state of a few variables when a file
 * is loaded or saved, or when the user starts editing a new file (in which case fileName is empty).
 * @param fileName file name.
 */
void MainWindow::setCurrentFile(const QString &fileName)
{
    qCDebug(vMainWindow, "Set current name to \"%s\"", qUtf8Printable(fileName));
    VAbstractValApplication::VApp()->SetPatternPath(fileName);
    doc->SetPatternWasChanged(true);
    emit doc->UpdatePatternLabel();
    VAbstractApplication::VApp()->getUndoStack()->setClean();

    if (not VAbstractValApplication::VApp()->GetPatternPath().isEmpty() && VApplication::IsGUIMode())
    {
        qCDebug(vMainWindow, "Updating recent file list.");
        VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
        QStringList files = settings->GetRecentFileList();
        files.removeAll(fileName);
        files.prepend(fileName);
        while (files.size() > MaxRecentFilesStored)
        {
            files.removeLast();
        }

        settings->SetRecentFileList(files);
        UpdateRecentFileActions();

        qCDebug(vMainWindow, "Updating restore file list.");
        QStringList restoreFiles = settings->GetRestoreFileList();
        restoreFiles.removeAll(fileName);
        restoreFiles.prepend(fileName);
        settings->SetRestoreFileList(restoreFiles);
    }

    UpdateWindowTitle();
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::FileClosedCorrect()
{
    WriteSettings();

    // File was closed correct.
    QStringList restoreFiles = VAbstractValApplication::VApp()->ValentinaSettings()->GetRestoreFileList();
    restoreFiles.removeAll(VAbstractValApplication::VApp()->GetPatternPath());
    VAbstractValApplication::VApp()->ValentinaSettings()->SetRestoreFileList(restoreFiles);

    // Remove autosave file
    if (QFile autofile(VAbstractValApplication::VApp()->GetPatternPath() + AutosavePrefix()); autofile.exists())
    {
        autofile.remove();
    }
    qCDebug(vMainWindow, "File %s closed correct.", qUtf8Printable(VAbstractValApplication::VApp()->GetPatternPath()));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief on_actionSaveAs_triggered save as pattern file.
 * @return true for successes saving.
 */
auto MainWindow::on_actionSaveAs_triggered() -> bool
{
    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
    QString const patternPath = VAbstractValApplication::VApp()->GetPatternPath();
    QString const dir = patternPath.isEmpty() ? settings->GetPathPattern() : QFileInfo(patternPath).absolutePath();

    QString newFileName;
    if (not patternPath.isEmpty())
    {
        // Start from the original filename but replace .val extension with .ostara
        QString base = QFileInfo(patternPath).fileName();
        if (base.endsWith(".val"_L1))
        {
            base.chop(4); // remove ".val"
        }
        newFileName = base + ".ostara"_L1;
    }
    else
    {
        newFileName = tr("pattern") + QStringLiteral(".ostara");
    }

    QString const filters(tr("Pattern files") + QStringLiteral(" (*.ostara)"));
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save as"), dir + '/'_L1 + newFileName, filters, nullptr,
                                                    VAbstractApplication::VApp()->NativeFileDialog());

    if (fileName.isEmpty())
    {
        return false;
    }

    QFileInfo const f(fileName);
    if (f.suffix().isEmpty() || f.suffix() == "val"_L1)
    {
        // If the user removed the extension or left .val, ensure .ostara
        if (not fileName.endsWith(".ostara"_L1))
        {
            if (f.suffix() == "val"_L1)
            {
                fileName.chop(4); // remove ".val"
            }
            fileName += ".ostara"_L1;
        }
    }

    if (patternPath.isEmpty())
    {
        settings->SetPathPattern(QFileInfo(fileName).absolutePath());
    }

    if (f.exists() && patternPath != fileName)
    {
        // Temporary try to lock the file before saving
        // Also help to rewite current read-only pattern
        VLockGuard<char> const tmp(fileName);
        if (not tmp.IsLocked())
        {
            qCCritical(vMainWindow, "%s",
                       qUtf8Printable(tr("Failed to lock. This file already opened in another window.")));
            return false;
        }
    }

    return SavePatternAs(fileName);
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief on_actionSave_triggered save pattern file.
 * @return true for successes saving.
 */
auto MainWindow::on_actionSave_triggered() -> bool
{
    if (VAbstractValApplication::VApp()->GetPatternPath().isEmpty() || m_patternReadOnly)
    {
        return on_actionSaveAs_triggered();
    }

    // A .val source is never rewritten in place: Save always redirects to a fresh .ostara,
    // so the "format rewrite" warning (a Valentina in-place-save relic) is obsolete here.
    const QString originalPath = VAbstractValApplication::VApp()->GetPatternPath();
    QString savePath = originalPath;
    // If the current file is a .val, convert the path to .ostara
    if (savePath.endsWith(".val"_L1))
    {
        savePath.chop(4);
        savePath += ".ostara"_L1;
    }

    if (not CheckFilePermissions(savePath, this))
    {
        return false;
    }

    QString error;
    bool const result = SavePattern(savePath, error);
    if (result)
    {
        // SavePattern() already switched the current path to savePath (the .ostara). Remove the
        // autosave of the path that was open *before* the redirect, or a stale "foo.val.autosave"
        // (with upgraded content) is left orphaned next to the .val and could feed a crash restore.
        QFile::remove(originalPath + AutosavePrefix());
        m_curFileFormatVersion = FormatVersion(1, 0, 0);
        m_curFileFormatVersionStr = QStringLiteral("1.0.0");
    }
    else
    {
        QMessageBox messageBox(this);
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.setText(tr("Could not save the file"));
        messageBox.setDefaultButton(QMessageBox::Ok);
        messageBox.setDetailedText(error);
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();
    }
    return result;
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief on_actionOpen_triggered ask user select pattern file.
 */
void MainWindow::on_actionOpen_triggered()
{
    // Switch to editor first so the file dialog opens over the canvas,
    // not over the WelcomeScreen. Idempotent if editor is already visible.
    ShowEditor();

    qCDebug(vMainWindow, "Openning new file.");
    const QString filter(tr("Pattern files") + QStringLiteral(" (*.ostara *.val)"));
    // Get list last open files
    const QStringList files = VAbstractValApplication::VApp()->ValentinaSettings()->GetRecentFileList();
    QString dir;
    if (files.isEmpty())
    {
        dir = QDir::homePath();
    }
    else
    {
        // Absolute path to last open file
        dir = QFileInfo(files.constFirst()).absolutePath();
    }
    qCDebug(vMainWindow, "Run QFileDialog::getOpenFileName: dir = %s.", qUtf8Printable(dir));
    const QString filePath = QFileDialog::getOpenFileName(this, tr("Open file"), dir, filter, nullptr,
                                                          VAbstractApplication::VApp()->NativeFileDialog());
    if (filePath.isEmpty())
    {
        // If we came from WelcomeScreen (no pattern open), return to it
        if (not hasOpenPattern())
        {
            emit patternClosed();
        }
        return;
    }
    LoadPattern(filePath);
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Clear reset to default window.
 */
void MainWindow::Clear()
{
    qCDebug(vMainWindow, "Reseting main window.");
    m_lock.reset();
    qCDebug(vMainWindow, "Unlocked pattern file.");
    ActionDraw(true);
    qCDebug(vMainWindow, "Returned to Draw mode.");
    setCurrentFile(QString()); // Keep before cleaning a pattern data to prevent a crash
    pattern->Clear();
    qCDebug(vMainWindow, "Clearing pattern.");
    if (not VAbstractValApplication::VApp()->GetPatternPath().isEmpty() && not doc->MPath().isEmpty())
    {
        m_watcher->removePath(AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath()));
    }
    doc->Clear();
    UpdateWindowTitle();
    UpdateVisibilityGroups();
    m_dockManager->detailsWidget()->UpdateList();
    m_dockManager->backgroundImagesWidget()->UpdateImages();
    m_dockManager->dependenciesWidget()->Clear();
    m_dockManager->detailsOptions()->Clear();
    qCDebug(vMainWindow, "Clearing scenes.");
    m_sceneDraw->clear();
    qCDebug(vMainWindow, "Draw scene cleared.");
    m_sceneDraw->SetAcceptDrop(false);
    // The group holders live in the details scene: forget them BEFORE the scene destroys
    // them, so RefreshGroupHolders (fired by the undo-stack clear below) never touches a
    // dangling holder during teardown (use-after-free, SIGSEGV in QGraphicsItem::setPos).
    m_groupHolders.clear();
    m_sceneDetails->clear();
    qCDebug(vMainWindow, "Details scene cleared.");

    if (m_sceneGraded != nullptr)
    {
        m_sceneGraded->clear();
    }
    m_activeGradedRangeName.clear();
    m_gradedBasePositions.clear();
    m_gradedSceneSignature.clear();
    m_gradedFrozenBases.clear();
    m_gradedForceCapture = false;
    ArrowTool(true);
    {
        const QSignalBlocker blocker(m_comboBoxDraws);
        m_comboBoxDraws->clear();
        m_comboBoxDraws->setDisabled(true);
    }
    ui->actionDraw->setEnabled(false);
    ui->actionDetails->setEnabled(false);
    ui->actionGraded->setEnabled(false);
    ui->actionLayout->setEnabled(false);
    ui->actionNewDraw->setEnabled(false);
    ui->actionOptionDraw->setEnabled(false);
    ui->actionSave->setEnabled(false);
    ui->actionSaveAs->setEnabled(false);
    ui->actionPattern_properties->setEnabled(false);
    ui->actionZoomIn->setEnabled(false);
    ui->actionZoomOut->setEnabled(false);
    ui->actionZoomFitBest->setEnabled(false);
    ui->actionZoomFitBestCurrent->setEnabled(false);
    ui->actionZoomOriginal->setEnabled(false);
    ui->actionExportRecipe->setEnabled(false);
    ui->actionTable->setEnabled(false);
    ui->actionExportFinalMeasurementsToCSV->setEnabled(false);
    ui->actionFinalMeasurements->setEnabled(false);
    ui->actionLast_tool->setEnabled(false);
    ui->actionShowCurveDetails->setEnabled(false);
    ui->actionShowSegmentLabels->setEnabled(false);
    ui->actionShowAccuracyRadius->setEnabled(false);
    ui->actionShowMainPath->setEnabled(false);
    ui->actionBoundaryTogetherWithNotches->setEnabled(false);
    ui->actionEditMeasurements->setEnabled(false);
    ui->actionPreviousPatternPiece->setEnabled(false);
    ui->actionNextPatternPiece->setEnabled(false);
    ui->actionAddBackgroundImage->setEnabled(false);
    SetEnableTool(false);
    VAbstractValApplication::VApp()->SetPatternUnits(Unit::Cm);
    VAbstractValApplication::VApp()->SetMeasurementsType(MeasurementsType::Unknown);

#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif
    CleanLayout();
    listDetails.clear(); // don't move to CleanLayout()
    VAbstractApplication::VApp()->getUndoStack()->clear();
    m_dockManager->toolOptions()->ClearPropertyBrowser();
    m_dockManager->toolOptions()->itemClicked(nullptr);
    m_progressBar->setVisible(false);
#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_taskbarProgress->setVisible(false);
#endif
    m_statusLabel->setVisible(true);
    ui->actionIncreaseLabelFont->setEnabled(false);
    ui->actionDecreaseLabelFont->setEnabled(false);
    ui->actionOriginalLabelFont->setEnabled(false);
    ui->actionShowAllLabels->setEnabled(false);
    ui->plainTextEditPatternMessages->clear();
    ui->actionLoadWatermark->setEnabled(false);
    ui->actionRemoveWatermark->setEnabled(false);
    PatternChangesWereSaved(true);
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief on_actionNew_triggered create new empty pattern.
 */
void MainWindow::on_actionNew_triggered()
{
    if (m_comboBoxDraws->count() == 0)
    {
        // Switch to editor first so the dialog opens over the canvas,
        // not over the WelcomeScreen.
        ShowEditor();

        qCDebug(vMainWindow, "New PP.");
        QString patternPieceName = tr("Pattern piece %1").arg(m_comboBoxDraws->count() + 1);
        qCDebug(vMainWindow, "Generated PP name: %s", qUtf8Printable(patternPieceName));

        qCDebug(vMainWindow, "First PP");
        if (DialogNewPattern newPattern(pattern, patternPieceName, this); newPattern.exec() == QDialog::Accepted)
        {
            patternPieceName = newPattern.name();
            VAbstractValApplication::VApp()->SetPatternUnits(newPattern.PatternUnit());
            qCDebug(vMainWindow, "PP name: %s", qUtf8Printable(patternPieceName));
        }
        else
        {
            qCDebug(vMainWindow, "Creation a new pattern was canceled.");
            emit patternClosed();
            return;
        }

        // Set scene size to size scene view
        VMainGraphicsView::NewSceneRect(m_sceneDraw, ui->view);
        VMainGraphicsView::NewSceneRect(m_sceneDetails, ui->view);

        AddPP(patternPieceName);

        // Normalize to Draw mode (mirrors LoadPattern) so the Details dock/content
        // cannot leak into a freshly created pattern.
        ActionDraw(true);

        m_curFileFormatVersion = FormatVersion(1, 0, 0);
        m_curFileFormatVersionStr = QStringLiteral("1.0.0");

        UpdateMeasurementState();


        m_sceneDraw->SetAcceptDrop(true);
    }
    else
    {
        OpenNewValentina();
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MaybeSave The function is called to save pending changes.
 * @return returns true in all cases, except when the user clicks Cancel.
 */
auto MainWindow::MaybeSave() -> bool
{
    if (this->isWindowModified() && m_guiEnabled)
    {
        QScopedPointer<QMessageBox> const messageBox(
            new QMessageBox(QMessageBox::Warning, tr("Unsaved changes"),
                            tr("The pattern has been modified. Do you want to save your changes?"),
                            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this, Qt::Sheet));

        messageBox->setDefaultButton(QMessageBox::Yes);
        messageBox->setEscapeButton(QMessageBox::Cancel);

        if (QAbstractButton *button = messageBox->button(QMessageBox::Yes))
        {
            button->setText(VAbstractValApplication::VApp()->GetPatternPath().isEmpty() || m_patternReadOnly
                                ? tr("Save…")
                                : tr("Save"));
        }

        if (QAbstractButton *button = messageBox->button(QMessageBox::No))
        {
            button->setText(tr("Don't Save"));
        }

        messageBox->setWindowModality(Qt::ApplicationModal);
        messageBox->setFixedSize(300, 85);

        switch (static_cast<QMessageBox::StandardButton>(messageBox->exec()))
        {
            case QMessageBox::Yes:
                if (m_patternReadOnly)
                {
                    return on_actionSaveAs_triggered();
                }
                else
                {
                    return on_actionSave_triggered();
                }
            case QMessageBox::No:
                return true;
            case QMessageBox::Cancel:
                return false;
            default:
                break;
        }
    }
    return true;
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::OpenNewValentina(const QString &fileName) const -> bool
{
    if (this->isWindowModified() || not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
    {
        VApplication::NewValentina(fileName);
        return true;
    }
    return false;
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief LoadPattern open pattern file.
 * @param fileName name of file.
 */
auto MainWindow::LoadPattern(QString fileName, const QString &customMeasureFile) -> bool
{
    qCDebug(vMainWindow, "Loading new file %s.", qUtf8Printable(fileName));

    { // Convert to absolute path if need
        QFileInfo const info(fileName);
        if (info.exists() && info.isRelative())
        {
            fileName = QFileInfo(QDir::currentPath() + '/'_L1 + fileName).canonicalFilePath();
        }
    }

    const bool isOstaraFile = fileName.endsWith(".ostara"_L1);

    QFuture<VPatternConverter *> futureConverter;
    if (not isOstaraFile)
    {
        futureConverter = QtConcurrent::run(
            [fileName]()
            {
                auto converter = std::make_unique<VPatternConverter>(fileName);
                return converter.release();
            });
    }

    // We have unsaved changes or load more then one file per time
    if (OpenNewValentina(fileName))
    {
        return false;
    }

    if (fileName.isEmpty())
    {
        qCDebug(vMainWindow, "Got empty file.");
        Clear();
        return false;
    }

    if (fileName.endsWith(".vit"_L1) || fileName.endsWith(".vst"_L1))
    {
        try
        {
            // Here comes undocumented Valentina's feature.
            // Because app bundle in Mac OS X doesn't allow setup assosiation for Tape we must do this through Valentina
            VMeasurements m(pattern);
            m.setXMLContent(fileName);

            if (m.Type() == MeasurementsType::Multisize || m.Type() == MeasurementsType::Individual)
            {
                QStringList arguments{fileName};
                if (isNoScaling)
                {
                    arguments.append("--"_L1 + LONG_OPTION_NO_HDPI_SCALING);
                }

                // Legacy Tape external launch removed — Ostara handles .vst/.vit internally
                return false;
            }
        }
        catch (VException &e)
        {
            qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("File error.")),
                       qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
            Clear();
            if (not VApplication::IsGUIMode())
            {
                QCoreApplication::exit(V_EX_NOINPUT);
            }
            return false;
        }
    }

    if (fileName.endsWith(".vkm"_L1))
    {
        // Here comes undocumented Valentina's feature.
        // Because app bundle in Mac OS X doesn't allow setup assosiation for Puzzle we must do this through Valentina
        QStringList arguments{fileName};
        if (isNoScaling)
        {
            arguments.append("--"_L1 + LONG_OPTION_NO_HDPI_SCALING);
            arguments.append("--known"_L1);
        }

        const QString puzzlePath = VApplication::PuzzleFilePath();
        qDebug("Opening Puzzle: path = %s.", qUtf8Printable(puzzlePath));
        VApplication::StartDetachedProcess(puzzlePath, arguments);
        QCoreApplication::exit(V_EX_OK);
        return false; // stop continue processing
    }

    if (fileName.endsWith(".vlt"_L1))
    {
        // Here comes undocumented Valentina's feature.
        // Because app bundle in Mac OS X doesn't allow setup assosiation for Puzzle we must do this through Valentina
        QStringList arguments{fileName};
        if (isNoScaling)
        {
            arguments.append("--"_L1 + LONG_OPTION_NO_HDPI_SCALING);
        }

        const QString path = VApplication::PuzzleFilePath();
        qDebug("Opening Puzzle: path = %s.", qUtf8Printable(path));
        VApplication::StartDetachedProcess(path, arguments);
        QCoreApplication::exit(V_EX_OK);
        return false; // stop continue processing
    }

    qCDebug(vMainWindow, "Loking file");
    VlpCreateLock(m_lock, fileName);

    if (m_lock->IsLocked())
    {
        qCDebug(vMainWindow, "Pattern file %s was locked.", qUtf8Printable(fileName));
    }
    else
    {
        if (not IgnoreLocking(m_lock->GetLockError(), fileName, VApplication::IsGUIMode()))
        {
            return false;
        }
    }

    // On this stage scene empty. Fit scene size to view size
    VMainGraphicsView::NewSceneRect(m_sceneDraw, ui->view);
    VMainGraphicsView::NewSceneRect(m_sceneDetails, ui->view);

    VAbstractValApplication::VApp()->setOpeningPattern(); // Begin opening file
    try
    {
        // Quick reading measurements
        doc->setXMLContent(fileName);

        if (isOstaraFile)
        {
            m_curFileFormatVersion = FormatVersion(1, 0, 0);
            m_curFileFormatVersionStr = QStringLiteral("1.0.0");

            // Migrate old "showLabel" attributes (pre-rename) to the current "showPointName".
            for (const QString &tag : {QStringLiteral("point"), QStringLiteral("item")})
            {
                const QDomNodeList nodes = doc->elementsByTagName(tag);
                for (int i = 0; i < nodes.size(); ++i)
                {
                    QDomElement elem = nodes.at(i).toElement();
                    if (not elem.isNull())
                    {
                        elem.removeAttribute(QStringLiteral("showLabel"));
                        elem.removeAttribute(QStringLiteral("showLabel1"));
                        elem.removeAttribute(QStringLiteral("showLabel2"));
                    }
                }
            }
        }
        else
        {
            const unsigned currentFormatVersion = VDomDocument::GetFormatVersion(doc->GetFormatVersionStr());
            if (currentFormatVersion != VPatternConverter::PatternMaxVer)
            { // Because we rely on the fact that we know where is path to measurements optimization available only for
              // the latest format version
                QScopedPointer<VPatternConverter> const converter(futureConverter.result());
                m_curFileFormatVersion = converter->GetCurrentFormatVersion();
                m_curFileFormatVersionStr = converter->GetFormatVersionStr();
                doc->setXMLContent(converter->Convert());
            }
            else
            {
                m_curFileFormatVersion = VPatternConverter::PatternMaxVer;
                m_curFileFormatVersionStr = VPatternConverter::PatternMaxVerStr;
            }

            // Strip Valentina label elements from the DOM (Ostara does not use labels)
            const bool hadLabels = StripLabelElements(doc);
            if (hadLabels)
            {
                QMessageBox::information(this, tr("Labels not supported"),
                    tr("This pattern contains Valentina labels. Ostara does not use them — this feature "
                       "will be replaced by Information block. Conversion is not automatic, your labels have not been "
                       "preserved. The original .val file remains intact."));
            }
        }

        if (!customMeasureFile.isEmpty())
        {
            doc->SetMPath(RelativeMPath(fileName, customMeasureFile));
        }
        VAbstractValApplication::VApp()->SetPatternUnits(doc->Units());

        const QString path = AbsoluteMPath(fileName, doc->MPath());
        QString fixedMPath;

        // See issue #976. Pattern can lost link to measurements
        if (path.isEmpty() && doc->RequiresMeasurements())
        {
            const auto fakePath = QStringLiteral("unknown_measurements.vit");
            // Check if exist
            fixedMPath = CheckPathToMeasurements(fileName, fakePath);
            if (fixedMPath.isEmpty())
            {
                VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
                Clear();
                qCCritical(vMainWindow, "%s",
                           qUtf8Printable(tr("The measurements file '%1' could not be found.").arg(fakePath)));
                if (not VApplication::IsGUIMode())
                {
                    QCoreApplication::exit(V_EX_NOINPUT);
                }
                return false;
            }
        }
        else if (not path.isEmpty())
        {
          // Check if exist
          fixedMPath = CheckPathToMeasurements(fileName, path);
          if (fixedMPath.isEmpty())
          {
              VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
              Clear();
              qCCritical(vMainWindow, "%s",
                         qUtf8Printable(tr("The measurements file '%1' could not be found.").arg(path)));
              if (not VApplication::IsGUIMode())
              {
                  QCoreApplication::exit(V_EX_NOINPUT);
              }
              return false;
          }
        }

        if (not fixedMPath.isEmpty())
        {
            // Legacy .vst/.vit → offer conversion or replacement with .omt/.otu
            const QFileInfo fi(fixedMPath);
            const QString suffix = fi.suffix().toLower();
            if (suffix == QStringLiteral("vst") || suffix == QStringLiteral("vit"))
            {
                const bool isMulti = (suffix == QStringLiteral("vst"));
                const QString targetExt = isMulti ? QStringLiteral(".omt") : QStringLiteral(".otu");

                QMessageBox msgBox(this);
                msgBox.setWindowTitle(tr("Legacy measurement file"));
                msgBox.setText(tr("The pattern is linked to a legacy Tape file:\n\n%1\n\n"
                                   "Ostara uses .omt/.otu format. Choose how to proceed:")
                                   .arg(fi.fileName()));
                auto *convertBtn = msgBox.addButton(tr("Convert this file"), QMessageBox::AcceptRole);
                auto *replaceBtn = msgBox.addButton(tr("Replace with existing .%1").arg(targetExt),
                                                     QMessageBox::ActionRole);
                msgBox.addButton(QMessageBox::Cancel);
                msgBox.exec();

                QString newPath;
                if (msgBox.clickedButton() == convertBtn)
                {
                    // Open MeasurementWindow for conversion, block until saved
                    auto *mw = new MeasurementWindow(this);
                    mw->SetOrigin(MeasurementWorkspace::Origin::FromDraw);
                    mw->SetModeless(true);
                    mw->LoadFile(fixedMPath);

                    QString savedPath;
                    QEventLoop loop;
                    connect(mw, &MeasurementWindow::measurementsSaved, &loop, [&](const QString &p) {
                        savedPath = p;
                        loop.quit();
                    });
                    connect(mw, &QObject::destroyed, &loop, &QEventLoop::quit);
                    mw->setAttribute(Qt::WA_DeleteOnClose);
                    mw->show();
                    loop.exec();

                    if (savedPath.isEmpty())
                    {
                        VAbstractValApplication::VApp()->setOpeningPattern();
                        Clear();
                        return false;
                    }
                    newPath = savedPath;
                }
                else if (msgBox.clickedButton() == replaceBtn)
                {
                    // Open dialog to pick an existing .omt/.otu
                    DialogOpenMeasurements dlg(this);
                    if (dlg.exec() != QDialog::Accepted)
                    {
                        VAbstractValApplication::VApp()->setOpeningPattern();
                        Clear();
                        return false;
                    }
                    newPath = dlg.selectedPath();
                    if (newPath.isEmpty())
                    {
                        VAbstractValApplication::VApp()->setOpeningPattern();
                        Clear();
                        return false;
                    }
                }
                else
                {
                    // Cancel
                    VAbstractValApplication::VApp()->setOpeningPattern();
                    Clear();
                    return false;
                }

                fixedMPath = newPath;
                doc->SetMPath(RelativeMPath(fileName, newPath));
            }

            if (not LoadMeasurements(fileName, fixedMPath))
            {
                qCCritical(vMainWindow, "%s",
                           qUtf8Printable(tr("The measurements file '%1' could not be found or "
                                             "provides not enough information.")
                                              .arg(fixedMPath)));
                VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
                Clear();
                if (not VApplication::IsGUIMode())
                {
                    QCoreApplication::exit(V_EX_NOINPUT);
                }
                return false;
            }

            m_watcher->addPath(fixedMPath);
            ui->actionEditMeasurements->setEnabled(true);
        }

        if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Unknown)
        { // Show toolbar only if was not uploaded any measurements.
            UpdateMeasurementState();

        }
    }
    catch (VException &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("File error.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
        Clear();
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return false;
    }

    m_progressBar->setVisible(true);
    m_statusLabel->setVisible(false);
    m_progressBar->setValue(0);
    const int elements = doc->ElementsToParse();
    m_progressBar->setMaximum(elements);
#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_taskbarProgress->setVisible(true);
    m_taskbarProgress->setMaximum(elements);
#endif

    doc->SetGBBackupFilePath(fileName);
    FullParseFile();

    m_progressBar->setVisible(false);
#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_taskbarProgress->setVisible(false);
#endif
    m_statusLabel->setVisible(true);

    if (m_guiEnabled)
    { // No errors occurred
        m_patternReadOnly = doc->IsReadOnly();
        m_sceneDraw->SetAcceptDrop(true);
        SetEnableWidgets(true);
        setCurrentFile(fileName);
        qCDebug(vMainWindow, "File loaded.");

        // Make the view visible BEFORE ZoomFirstShow/ActionDraw so that
        // NewSceneRect, ZoomFitBest etc. operate with correct widget dimensions.
        ShowEditor();

        // Fit scene size to best size for first show
        ZoomFirstShow();

        ActionDraw(true);

        VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
        m_statusLabel->setText(QString());
        return true;
    }

    VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
    return false;
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::GetUnlokedRestoreFileList() -> QStringList
{
    QStringList restoreFiles;
    // Take all files that need to be restored
    if (QStringList files = VAbstractValApplication::VApp()->ValentinaSettings()->GetRestoreFileList();
        not files.empty())
    {
        restoreFiles.reserve(files.size());
        for (auto &file : files)
        {
            // Seeking file that realy need reopen
            VLockGuard<char> const tmp(file);
            if (tmp.IsLocked())
            {
                restoreFiles.append(file);
            }
        }

        // Clearing list after filtering
        for (auto &file : restoreFiles)
        {
            files.removeAll(file);
        }

        // Clear all files that do not exist.
        QStringList filtered;
        for (auto &file : files)
        {
            if (QFileInfo::exists(file))
            {
                filtered.append(file);
            }
        }

        VAbstractValApplication::VApp()->ValentinaSettings()->SetRestoreFileList(filtered);
    }
    return restoreFiles;
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ReopenFilesAfterCrash(QStringList &args)
{
    const QStringList files = GetUnlokedRestoreFileList();
    if (files.empty())
    {
        return;
    }

    qCDebug(vMainWindow, "Reopen files after crash.");

    QStringList restoreFiles;
    restoreFiles.reserve(files.size());
    for (const auto &file : files)
    {
        if (QFile::exists(file + AutosavePrefix()))
        {
            restoreFiles.append(file);
        }
    }

    if (restoreFiles.empty())
    {
        return;
    }

    QMessageBox::StandardButton reply;
    const QString mes = tr("Valentina didn't shut down correctly. Do you want reopen files (%1) you had open?")
                            .arg(restoreFiles.size());
    reply = QMessageBox::question(this, tr("Reopen files."), mes, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (reply != QMessageBox::Yes)
    {
        return;
    }

    qCDebug(vMainWindow, "User said Yes.");

    for (auto &file : restoreFiles)
    {
        QString error;
        if (VDomDocument::SafeCopy(file + AutosavePrefix(), file, error))
        {
            QFile autoFile(file + AutosavePrefix());
            autoFile.remove();
            LoadPattern(file);
            args.removeAll(file); // Do not open file twice after we restore him.
        }
        else
        {
            qCDebug(vMainWindow, "Could not copy %s%s to %s %s", qUtf8Printable(file), qUtf8Printable(AutosavePrefix()),
                    qUtf8Printable(file), qUtf8Printable(error));
        }
    }
}


//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::SavePatternAs(const QString &fileName) -> bool
{
    // Need for restoring previous state in case of failure
    const bool wasModified = doc->IsModified(); // Need because SetReadOnly() will change internal state
    const bool readOnly = doc->IsReadOnly();

    doc->SetReadOnly(false); // Save as... disable read only state
    QString error;
    const bool result = SavePattern(fileName, error);
    if (not result)
    {
        QMessageBox messageBox(this);
        messageBox.setIcon(QMessageBox::Warning);
        messageBox.setInformativeText(tr("Could not save file"));
        messageBox.setDefaultButton(QMessageBox::Ok);
        messageBox.setDetailedText(error);
        messageBox.setStandardButtons(QMessageBox::Ok);
        messageBox.exec();

        // Restoring previous state
        doc->SetReadOnly(readOnly);
        doc->SetModified(wasModified);

        return result;
    }

    if (const QString oldFilePath = VAbstractValApplication::VApp()->GetPatternPath(); not oldFilePath.isEmpty())
    {
        qCDebug(vMainWindow, "Updating restore file list.");
        QStringList restoreFiles = VAbstractValApplication::VApp()->ValentinaSettings()->GetRestoreFileList();
        restoreFiles.removeAll(oldFilePath);
        VAbstractValApplication::VApp()->ValentinaSettings()->SetRestoreFileList(restoreFiles);
        QFile::remove(oldFilePath + AutosavePrefix());
    }

    m_curFileFormatVersion = FormatVersion(1, 0, 0);
    m_curFileFormatVersionStr = QStringLiteral("1.0.0");
    m_patternReadOnly = false;

    qCDebug(vMainWindow, "Locking file");
    if (VAbstractValApplication::VApp()->GetPatternPath() == fileName && not m_lock.isNull())
    {
        m_lock->Unlock();
    }
    VlpCreateLock(m_lock, fileName);

    if (m_lock->IsLocked())
    {
        qCDebug(vMainWindow, "Pattern file %s was locked.", qUtf8Printable(fileName));
    }
    else
    {
        qCDebug(vMainWindow, "Failed to lock %s", qUtf8Printable(fileName));
        qCDebug(vMainWindow, "Error type: %d", m_lock->GetLockError());
        qCCritical(vMainWindow, "%s",
                   qUtf8Printable(tr("Failed to lock. This file already opened in another window. Expect "
                                     "collissions when run 2 copies of the program.")));
    }

    return result;
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitAutoSave()
{
    // Autosaving file each 1 minutes
    delete m_autoSaveTimer;
    m_autoSaveTimer = nullptr;

    m_autoSaveTimer = new QTimer(this);
    m_autoSaveTimer->setTimerType(Qt::VeryCoarseTimer);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::AutoSavePattern);
    m_autoSaveTimer->stop();

    if (VAbstractValApplication::VApp()->ValentinaSettings()->GetAutosaveState())
    {
        const qint32 autoTime = VAbstractValApplication::VApp()->ValentinaSettings()->GetAutosaveTime();
        m_autoSaveTimer->start(std::chrono::minutes{autoTime});
        qCDebug(vMainWindow, "Autosaving each %d minutes.", autoTime);
    }
    VApplication::VApp()->setAutoSaveTimer(m_autoSaveTimer);
}
//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::GetPatternFileName() -> QString
{
    QString shownName = tr("untitled.ostara");
    if (not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
    {
        shownName = QFileInfo(VAbstractValApplication::VApp()->GetPatternPath()).fileName();
    }
    shownName += "[*]"_L1;
    return shownName;
}
//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::GetMeasurementFileName() -> QString
{
    if (doc->MPath().isEmpty())
    {
        return {};
    }

    auto shownName = QStringLiteral("  —  ");
    shownName += QFileInfo(AbsoluteMPath(VAbstractValApplication::VApp()->GetPatternPath(), doc->MPath())).fileName();

    return shownName;
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateWindowTitle()
{
    bool isFileWritable = true;
    if (not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
    {
        // #ifdef Q_OS_WIN32
        //         qt_ntfs_permission_lookup++; // turn checking on
        // #endif /*Q_OS_WIN32*/
        isFileWritable = QFileInfo(VAbstractValApplication::VApp()->GetPatternPath()).isWritable();
        // #ifdef Q_OS_WIN32
        //         qt_ntfs_permission_lookup--; // turn it off again
        // #endif /*Q_OS_WIN32*/
    }

    if (not m_patternReadOnly && isFileWritable)
    {
        setWindowTitle(GetPatternFileName() + GetMeasurementFileName());
    }
    else
    {
        setWindowTitle(GetPatternFileName() + GetMeasurementFileName() + " ("_L1 + tr("read only") + ')'_L1);
    }
    setWindowFilePath(VAbstractValApplication::VApp()->GetPatternPath());

#if defined(Q_OS_MAC)
    static auto fileIcon = QIcon(QCoreApplication::applicationDirPath() + "/../Resources/Valentina.icns"_L1);
    QIcon icon;
    if (not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
    {
        if (not isWindowModified())
        {
            icon = fileIcon;
        }
        else
        {
            static QIcon darkIcon;

            if (darkIcon.isNull())
            {
                darkIcon = QIcon(darkenPixmap(fileIcon.pixmap(16, 16)));
            }
            icon = darkIcon;
        }
    }
    setWindowIcon(icon);
#endif // defined(Q_OS_MAC)
}
