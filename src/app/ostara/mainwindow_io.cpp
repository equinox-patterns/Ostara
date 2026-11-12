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
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **  Modified for Ostara (fork of Valentina) on 27/10/2026.
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
#include "widgets/InfoBlock/OPieceCartoucheStorage.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/def.h"
#include "core/opiecelibrary.h"
#include "core/opatternlibrary.h"
#include "core/vapplication.h"
#include "core/odetailspropertybrowser.h"
#include "core/ogradedrangerenames.h"
#include "core/vtooloptionspropertybrowser.h"
#include "dialogs/DialogOpenMeasurements.h"
#include "dialogs/opatternopendialog.h"
#include "dialogs/osavepatterndialog.h"
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
#include "../vtools/tools/othumbnailcache.h"
#include "../vtools/tools/vtoolseamallowance.h"
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
#include <QClipboard>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QScreen>
#include <QSet>
#include <QTimer>
#include <QtConcurrent>
#include <chrono>
#include <memory>

using namespace Qt::Literals::StringLiterals;
using namespace std::chrono_literals;

//---------------------------------------------------------------------------------------------------------------------
/** @brief Tell the reader a pattern could not be opened, without the raw XML in their face.
 *
 * One sentence they can act on; the exception's own text — element, line, expected value — stays behind
 * « Show details » and can be copied for a bug report. It is written to the log as well.
 */
static void ShowPatternOpenError(QWidget *parent, const QString &details)
{
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(QCoreApplication::translate("MainWindow", "Pattern could not be opened"));
    box.setText(QCoreApplication::translate("MainWindow", "This pattern could not be opened."));
    box.setInformativeText(
        QCoreApplication::translate("MainWindow", "It may be damaged, or created with a newer version of Ostara."));
    box.setDetailedText(details);

    QPushButton *copy = box.addButton(QCoreApplication::translate("MainWindow", "Copy the details"),
                                      QMessageBox::ActionRole);
    box.addButton(QMessageBox::Close);

    box.exec();

    if (box.clickedButton() == copy)
    {
        QGuiApplication::clipboard()->setText(details);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief The resources a pattern names and that are nowhere in their folder.
 *
 * A cartouche whose template file is missing renders nothing at all — no border, no block, no message — and an
 * annotation whose style is missing is drawn in the default one without saying so. Both names stay in the pattern
 * (that is right: an edit must never overwrite them), so the only moment the absence can be said is at open.
 *
 * Reading must not write: the style library is read through `ReadAnnotationStyles()`, not through the loader that
 * installs the built-in Default on a first launch — opening a pattern is not the moment to create files in a
 * library. And a folder that does not exist says nothing about the files it should hold: accusing a template of
 * being absent would name the wrong culprit, the folder being the problem the library dialogs already explain.
 *
 * The template is resolved the way the renderer resolves it (OInfoBlockResolver): the pattern's own name, then the
 * favourite from the settings, then "Default". A piece-level override is not checked — the pattern's own name is
 * the one that covers the whole document.
 *
 * @return one sentence per missing resource, worded for the status bar; empty when everything resolves.
 */
static auto MissingLibraryResources(const VAbstractPattern *doc) -> QStringList
{
    QStringList missing;
    if (doc == nullptr)
    {
        return missing;
    }

    QString templateName = doc->GetCartoucheTemplate();
    if (templateName.isEmpty())
    {
        templateName = VAbstractApplication::VApp()->Settings()->GetDefaultCartoucheTemplate();
    }
    if (templateName.isEmpty())
    {
        templateName = QStringLiteral("Default");
    }

    const QString templatesDirectory = OPieceCartoucheStorage::DefaultDirectory();
    if (QDir(templatesDirectory).exists())
    {
        bool found = false;
        for (const QString &path : OPieceCartoucheStorage::ScanDirectory(templatesDirectory))
        {
            if (QFileInfo(path).completeBaseName() == templateName)
            {
                found = true;
                break;
            }
        }
        if (not found)
        {
            missing << QCoreApplication::translate("MainWindow", "cartouche template « %1 »").arg(templateName);
        }
    }

    // Every distinct style the document names, missing from the library's own file. Sorted, so the sentence
    // does not change shape from one opening to the next.
    const QString stylesDirectory = OPieceCartoucheStorage::AnnotationStylesDirectory();
    if (QDir(stylesDirectory).exists())
    {
        const QDomNodeList annotations = doc->elementsByTagName(VToolSeamAllowance::TagAnnotation);
        if (not annotations.isEmpty())
        {
            QSet<QString> known;
            for (const OPieceCartoucheStorage::AnnotationStyleEntry &entry : OPieceCartoucheStorage::ReadAnnotationStyles())
            {
                known.insert(entry.name);
            }

            QStringList absent;
            for (int i = 0; i < annotations.size(); ++i)
            {
                const QString styleName = annotations.at(i).toElement().attribute(VAbstractPattern::AttrTemplate);
                if (styleName.isEmpty() || styleName == QStringLiteral("Default") || known.contains(styleName)
                    || absent.contains(styleName))
                {
                    continue;
                }
                absent << styleName;
            }

            absent.sort();
            for (const QString &styleName : absent)
            {
                missing << QCoreApplication::translate("MainWindow", "annotation style « %1 »").arg(styleName);
            }
        }
    }

    return missing;
}

//---------------------------------------------------------------------------------------------------------------------
/** @brief Keep the main window inside the screen's available area.
 *
 * The default size in mainwindow.ui is 1380x1051 — wider and taller than many laptop screens — and a restored
 * geometry may come from a larger display. Windows then puts the bottom of the window under the taskbar, and the
 * only way out is to minimise and restore so that the window manager re-lays it out.
 *
 * The frame (title bar, borders) is part of what has to fit: clamping the client area alone still leaves the
 * window over the taskbar by the height of its own title bar. That is why this runs after the window is mapped.
 *
 * Sophie, 03-11-2026: a 1024x768 screen. The size is clamped only when it does not fit; a window that already
 * fits is left exactly where the user put it, and one that hangs off an edge is moved back inside.
 *
 * The ninth-tenths margin is for the size the window carries when nothing has chosen one yet — the default of
 * mainwindow.ui. A size restored from the preferences, or given by the reader by dragging the window, is kept
 * whole as long as it fits: it is not the screen's business to shrink it. Applying the margin to every window
 * undid every resize twice, at start-up and again when the first pattern was opened. Sophie, 09-11-2026.
 */
void MainWindow::FitMainWindowToScreen()
{
    // A maximised or full-screen window is already the screen's own geometry: resizing it here would only
    // rewrite the geometry it returns to when it is restored.
    if (isHidden() || isMaximized() || isFullScreen())
    {
        return;
    }

    const QScreen *screen = this->screen();
    if (screen == nullptr)
    {
        return;
    }

    const QRect available = screen->availableGeometry();
    QRect frame = frameGeometry();

    const int frameWidth = qMax(0, frame.width() - width());
    const int frameHeight = qMax(0, frame.height() - height());
    const int maxWidth = qMax(1, available.width() - frameWidth);
    // The whole usable height is the limit; nine tenths of it is the limit only for the default size of
    // mainwindow.ui — a window whose bottom touches the taskbar reads as maximised and leaves no air around
    // the canvas. Sophie, 03-11-2026, revised 09-11-2026: a size already settled is honoured as it is.
    const int fitHeight = qMax(1, available.height() - frameHeight);
    const int maxHeight = m_initialSizeSettled ? fitHeight : qMax(1, fitHeight * 9 / 10);

    if (width() > maxWidth || height() > maxHeight)
    {
        resize(qMin(width(), maxWidth), qMin(height(), maxHeight));
        frame = frameGeometry();
        // Centred vertically now that it no longer fills the screen.
        const int centred = available.top() + qMax(0, (available.height() - frame.height()) / 2);
        move(frame.left(), centred);
        frame = frameGeometry();
    }

    const int x = qBound(available.left(), frame.left(), qMax(available.left(), available.right() - frame.width() + 1));
    const int y = qBound(available.top(), frame.top(), qMax(available.top(), available.bottom() - frame.height() + 1));
    if (x != frame.left() || y != frame.top())
    {
        move(x, y);
    }

    // From now on the window has a size of its own — the one it was just given, or the one it came with.
    m_initialSizeSettled = true;
}

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
        // A geometry that was really restored is a size of the reader's own: FitMainWindowToScreen() keeps it
        // whole instead of applying the ninth-tenths opening margin. Only the default of mainwindow.ui, which
        // is what a first start carries, gets that margin. Sophie, 09-11-2026.
        m_initialSizeSettled = restoreGeometry(settings->GetGeometry());
        const bool dockLayoutRestored = restoreState(settings->GetToolbarsState(), kToolbarStateVersion);

        // Load per-mode dock layouts so the first entry into a mode after startup
        // restores its own sizes/visibility instead of inheriting the current mode's.
        // (At startup every mode function runs while openingPattern is true, so
        // SaveDockStateForMode() skips capturing — without this the mode states
        // would stay empty until the user leaves a mode once.)
        m_dockManager->setDrawModeDockState(settings->GetModeToolbarsState(Draw::Calculation));
        m_dockManager->setDetailsModeDockState(settings->GetModeToolbarsState(Draw::Modeling));
        m_dockManager->setGradedModeDockState(settings->GetModeToolbarsState(Draw::Graded));
        m_dockManager->setExportModeDockState(settings->GetModeToolbarsState(Draw::Export));

        // Dock visibility is handled by saveState()/restoreState(). The boolean members are read back from
        // the docks ONLY when a saved layout was actually restored: with no saved state the docks are still
        // as mainwindow.ui declares them — all nine visible — and copying that into the preference replaced
        // the default layout of a fresh install (Options, Layers with its messages tabbed under it).
        if (dockLayoutRestored)
        {
            m_dockManager->syncVisibilityFromDocks();

            // A restored layout with none of the Draw docks on screen is not something the reader can work
            // in: it is what a session that ended at the wrong moment saved, and every later start then
            // opened an empty Draw mode. The default set takes over instead of trusting it.
            if (!m_dockManager->anyDrawDockVisible())
            {
                m_dockManager->restoreDefaultDockVisibility();
            }
        }

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
    const QByteArray gradedState = m_dockManager->gradedModeDockState();
    const QByteArray exportState = m_dockManager->exportModeDockState();

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
    settings->SetModeToolbarsState(Draw::Graded, gradedState);
    settings->SetModeToolbarsState(Draw::Export, exportState);

    settings->sync();

    ActionDraw(true);

    // Restore the true per-mode layouts: ActionDraw(true) just overwrote the
    // current mode's member with the normalized layout, which must not leak
    // into the next pattern opened in this session.
    m_dockManager->setDrawModeDockState(drawState);
    m_dockManager->setDetailsModeDockState(detailsState);
    m_dockManager->setGradedModeDockState(gradedState);
    m_dockManager->setExportModeDockState(exportState);

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
        // One entry per file, and the file just opened becomes the first one: the settings move the entry
        // that already holds this file instead of adding a second one (same file, another spelling of its
        // path — macOS keeps the case it was created with).
        settings->AddRecentFile(fileName, MaxRecentFilesStored);
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
        // The proposed name carries exactly one .ostara: every extension we know is dropped first, then one is
        // written back. Appending blindly proposed "name.ostara.ostara" for a pattern already saved as .ostara —
        // and that doubled name is the one that got saved. A legacy .val source loses the extension it was read
        // under, and a file that already carries the doubled name comes back to a single one.
        QString base = QFileInfo(patternPath).fileName();
        while (base.endsWith(".ostara"_L1) || base.endsWith(".val"_L1))
        {
            base.chop(base.endsWith(".ostara"_L1) ? 7 : 4); // remove ".ostara" or ".val"
        }
        newFileName = base + ".ostara"_L1;
    }
    else
    {
        // The first save proposes the pattern's own name when it has one: that is the name its reader has just
        // written in the Property Browser, and the file is worth being called after it. A file name cannot hold
        // a separator — they are taken out here, and the dialog refuses them again on its side.
        QString proposed = doc != nullptr ? doc->GetPatternName().trimmed() : QString();
        proposed.remove(QLatin1Char('/'));
        proposed.remove(QLatin1Char('\\'));
        newFileName = proposed.isEmpty() ? tr("pattern") + QStringLiteral(".ostara")
                                         : proposed + QStringLiteral(".ostara");
    }

    // Where the file goes is asked by our dialog, which **offers the pattern library** — and which hands the
    // question back to the system's own panel when the reader says "not the library". The two paths below are
    // the same they always were: one name, then the same forcing, the same lock, the same write.
    QString fileName;
    OSavePatternDialog saveDialog(OPatternLibrary::DefaultDirectory(), newFileName, patternPath, this);
    saveDialog.exec(); // ce qui compte est ce qu'elle a répondu, pas le code de retour du dialog
    switch (saveDialog.ChosenDestination())
    {
        case OSavePatternDialog::Destination::Library:
            fileName = saveDialog.LibraryPath();
            break;
        case OSavePatternDialog::Destination::Elsewhere:
        {
            QString const filters(tr("Pattern files") + QStringLiteral(" (*.ostara)"));
            fileName = QFileDialog::getSaveFileName(this, tr("Save as"), dir + '/'_L1 + newFileName, filters,
                                                    nullptr, VAbstractApplication::VApp()->NativeFileDialog());
            break;
        }
        case OSavePatternDialog::Destination::Cancelled:
            return false;
    }

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

    const QString savePath = VAbstractValApplication::VApp()->GetPatternPath();

    // A .val is only ever read, never rewritten: saving one creates a new .ostara beside it. Which name, and
    // in which folder, is the reader's decision — Save must not drop an .ostara next to her .val without a
    // word. Save As asks exactly that: same folder, same name, .val replaced by .ostara.
    if (savePath.endsWith(".val"_L1))
    {
        return on_actionSaveAs_triggered();
    }

    if (not CheckFilePermissions(savePath, this))
    {
        return false;
    }

    QString error;
    bool const result = SavePattern(savePath, error);
    if (result)
    {
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
 * @brief on_actionOpen_triggered asks *where* to open: the pattern library first, the computer second.
 *
 * File > Open, its Cmd+O shortcut, its toolbar button, the macOS Dock menu and the Welcome screen's
 * "Open a pattern" all land here — one gesture, one dialog. Ostara has a pattern library, so looking for a
 * stored pattern is the common case; the system panel is one click away inside the dialog ("Browse files…"),
 * which is also the only door left for a legacy `.val`, since the library lists `.ostara` alone.
 */
void MainWindow::on_actionOpen_triggered()
{
    OPatternOpenDialog dialog(OPatternLibrary::DefaultDirectory(), this);
    if (dialog.exec() != QDialog::Accepted)
    {
        // Nothing was asked, so nothing moves: the pattern being edited stays put, and a Welcome screen that
        // opened the dialog stays on screen.
        return;
    }

    const QString filePath = dialog.SelectedPath();
    if (filePath.isEmpty())
    {
        return;
    }

    // LoadPattern shows the editor itself once a pattern is actually open — showing it first made the canvas
    // flash for nothing when the opening was then refused.
    const auto result = LoadPattern(filePath);
    if (result == PatternLoadResult::Failed)
    {
        // The opening was refused — no usable measurement table, or the reader gave it up. An empty editor
        // is not an answer: back to the welcome screen, from where the file can be tried again.
        ShowWelcomeScreen();
    }
    // PatternLoadResult::Delegated is not a refusal: this window keeps the pattern it already shows, and the
    // chosen file opens in another process's window. Switching to the Welcome screen here would hide the
    // pattern being edited with no way back to it.
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
    // The miniature cache is a global keyed by piece id alone: the drawings of the document being
    // cleared must not outlive it, or the next document's pieces — whose ids restart at 1 — would
    // paint them until the first snapshot round trip replaces them.
    OThumbnailCache::Clear();
    // The remembered "what the miniatures draw" belongs to the document being cleared: keeping it
    // would make the next file's first edit look unchanged, and no drawing would ever be captured.
    m_thumbnailSnapshotDigest.clear();
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
    // The scene has just deleted the background image items and their controls. Forget them here, or
    // NewBackgroundImageItem() reuses the dangling controls — and connects an item to a destroyed QObject —
    // as soon as the next pattern gets its first background image.
    m_backgroudcontrols = nullptr;
    m_backgroundImages.clear();
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
    if (m_sceneExport != nullptr)
    {
        m_sceneExport->clear();
    }
    // Generations belong to the session: closing the pattern takes them with the rest,
    // and none of them was ever written to the .ostara.
    m_exportOutputs.Clear();

    // The renames followed the ranges of the pattern that is going away: keeping them would
    // rename the ranges of the next one (see OGradedRangeRenames).
    OGradedRangeRenames::Forget();

    // The generations are gone, so the selection naming a footprint of one and the history of
    // gestures made on it are gone too. Reset() is documented for this exact moment; until now
    // it had no caller anywhere in the program. Without it the plan's undo stack outlived the
    // result it described and the menu went on offering to undo gestures on a plan that no
    // longer existed.
    m_exportCorrection->Reset();

    m_activeGradedRangeName.clear();
    m_gradedBasePositions.clear();
    m_gradedSceneSignature.clear();
    m_gradedFrozenBases.clear();
    m_lastGroupedPieceIds.clear();
    m_gradedForceCapture = false;
    ArrowTool(true);
    {
        const QSignalBlocker blocker(m_comboBoxDraws);
        m_comboBoxDraws->clear();
        m_comboBoxDraws->setDisabled(true);
    }
    UpdatePieceSaveAction();
    ui->actionDraw->setEnabled(false);
    ui->actionDetails->setEnabled(false);
    ui->actionGraded->setEnabled(false);
    ui->actionExport->setEnabled(false);
    ui->actionNewDraw->setEnabled(false);
    ui->actionOptionDraw->setEnabled(false);
    ui->actionSave->setEnabled(false);
    ui->actionSaveAs->setEnabled(false);
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
    if (m_comboBoxDraws->count() != 0)
    {
        OpenNewValentina();
        return;
    }

    // Switch to editor first so the dialog opens over the canvas,
    // not over the WelcomeScreen.
    ShowEditor();

    // The editor is shown *before* the dialog, so whatever the mode left on screen is what the reader sees
    // behind it. Without applying the Draw layout here, the background is the union of every mode's docks and
    // the tool palette is built on the width it happens to have — it visibly reshuffles itself when the dialog
    // closes. Sophie, 03-11-2026: applied now, not at validation.
    switchScene(Draw::Calculation);
    m_dockManager->applyModeDocks(Draw::Calculation);

    // The dialog, and the piece it may carry. It is asked again as long as that piece cannot enter: a pattern was
    // asked for **from a piece**, and a pattern born without it is not what was asked for — the empty block the
    // old fallback created was a substitute nobody wanted. The document is cleared and the question asked again,
    // so the reader is back at the dialog with the table he loaded still in hand.
    while (true)
    {
        qCDebug(vMainWindow, "New PP.");
        QString patternPieceName = tr("Pattern piece %1").arg(m_comboBoxDraws->count() + 1);
        qCDebug(vMainWindow, "Generated PP name: %s", qUtf8Printable(patternPieceName));

        qCDebug(vMainWindow, "First PP");
        DialogNewPattern newPattern(pattern, patternPieceName, DialogNewPattern::Creation::Pattern, this);
        // The library is given to the dialog here as well, and the unit is the one the dialog is asking about:
        // changing it re-reads the library, since a piece of another unit could not enter this pattern.
        newPattern.SetLibraryDirectory(OPieceLibrary::DefaultDirectory());
        if (newPattern.exec() != QDialog::Accepted)
        {
            qCDebug(vMainWindow, "Creation a new pattern was canceled.");
            emit patternClosed();
            return;
        }

        patternPieceName = newPattern.name();
        VAbstractValApplication::VApp()->SetPatternUnits(newPattern.PatternUnit());
        qCDebug(vMainWindow, "PP name: %s", qUtf8Printable(patternPieceName));

        // Set scene size to size scene view
        VMainGraphicsView::NewSceneRect(m_sceneDraw, ui->view);
        VMainGraphicsView::NewSceneRect(m_sceneDetails, ui->view);

        if (const QString imported = newPattern.ImportedPath(); not imported.isEmpty())
        {
            // The pattern is born from a piece of the library instead of one drawn here. The file has to exist
            // **before** the block: a block is inserted into a document, and a pattern that has never been
            // written has no root at all — the same CreateEmptyFile the first base point asks for. It also has
            // to exist **before** the measurement table can be chosen: the path of that table is written in the
            // document.
            if (doc->CountPatternBlockTags() == 0)
            {
                doc->CreateEmptyFile();
            }

            if (not InsertLibraryPiece(imported))
            {
                // Nothing entered, and nothing was asked for: the document born to hold the piece has nothing
                // left to be. Cleared, and the dialog asked again.
                Clear();
                continue;
            }
        }
        else
        {
            // The piece and the layer it starts in, in one gesture — see AddPP.
            AddPP(patternPieceName, newPattern.LayerName(), newPattern.LayerColor(), newPattern.LayerPenStyle());
        }

        // Normalize to Draw mode (mirrors LoadPattern) so the Details dock/content
        // cannot leak into a freshly created pattern.
        ActionDraw(true);

        m_curFileFormatVersion = FormatVersion(1, 0, 0);
        m_curFileFormatVersionStr = QStringLiteral("1.0.0");

        UpdateMeasurementState();


        m_sceneDraw->SetAcceptDrop(true);

        return;
    }
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MaybeSave The function is called to save pending changes.
 * @return returns true in all cases, except when the user clicks Cancel.
 */
auto MainWindow::MaybeSave() -> bool
{
    // Console mode has nobody to answer the question — its window is closed by the program itself once
    // the export is over, with no event loop left to run a modal in. Nothing is written behind the user's
    // back either: such a run only reads the pattern and writes the files it was asked for.
    if (not VApplication::IsGUIMode())
    {
        return true;
    }

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
auto MainWindow::LoadPattern(QString fileName, const QString &customMeasureFile) -> PatternLoadResult
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

    // We have unsaved changes or load more than one file at a time: this window keeps its pattern and the
    // file opens in another process's window. Not a failure — the caller must not treat it as one.
    if (OpenNewValentina(fileName))
    {
        return PatternLoadResult::Delegated;
    }

    if (fileName.isEmpty())
    {
        qCDebug(vMainWindow, "Got empty file.");
        Clear();
        return PatternLoadResult::Failed;
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
                return PatternLoadResult::Failed;
            }
        }
        catch (VException &e)
        {
            const QString details = QStringLiteral("%1\n\n%2").arg(e.ErrorMessage(), e.DetailedInformation());
            qCDebug(vMainWindow, "File error.\n\n%s", qUtf8Printable(details));
            if (VApplication::IsGUIMode())
            {
                ShowPatternOpenError(this, details);
            }
            Clear();
            if (not VApplication::IsGUIMode())
            {
                QCoreApplication::exit(V_EX_NOINPUT);
            }
            return PatternLoadResult::Failed;
        }
    }

    qCDebug(vMainWindow, "Locking file");
    VlpCreateLock(m_lock, fileName);

    if (m_lock->IsLocked())
    {
        qCDebug(vMainWindow, "Pattern file %s was locked.", qUtf8Printable(fileName));
    }
    else
    {
        if (not IgnoreLocking(m_lock->GetLockError(), fileName, VApplication::IsGUIMode()))
        {
            return PatternLoadResult::Failed;
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

            // Strip legacy label elements from the DOM (Ostara does not use labels)
            const bool hadLabels = StripLabelElements(doc);
            if (hadLabels)
            {
                QMessageBox::information(this, tr("Labels not supported"),
                    tr("This pattern contains legacy labels. Ostara does not use them - they have been "
                       "replaced by the Information block. Conversion is not automatic, your labels have not been "
                       "preserved. The original .val file remains intact."));
            }

            // Strip pin nodes and neutralize pin-anchored grainlines (Ostara no longer supports pins)
            const bool hadPinGrainlines = StripPinElements(doc);
            if (hadPinGrainlines)
            {
                QMessageBox::information(this, tr("Grainlines anchored to pins"),
                    tr("This pattern contains grainlines anchored to pins. Ostara no longer uses them - these "
                       "grainlines have been disabled. Please place them again in the Details mode. Conversion is not "
                       "automatic. The original .val file remains intact."));
            }

            // Drop the legacy piece short name (Ostara names its area variables from the piece name). Silent by
            // design: the attribute has no effect the user could observe, so there is nothing to warn about.
            StripShortNameAttributes(doc);

            // Convert legacy built-in seam allowance pieces to pieces without a seam allowance. Silent by design:
            // the geometry is unchanged, manual notch lengths are kept, and a notch that had no length simply
            // reappears at the Ostara default — nothing the user has to act on.
            StripBuiltInSeamAllowanceAttributes(doc);
        }

        // Convert the fold-line types Ostara dropped. Deliberately OUTSIDE the block above: the Strip* passes only
        // run for a legacy .val, but this one must run for a .ostara too — an older Ostara build could have written
        // one of the removed values, and the save schema no longer accepts them, so an unconverted DOM would make
        // the pattern impossible to save. Silent by design, see the function.
        ConvertRemovedFoldLineTypes(doc);

        // Drop the two layout flags Ostara no longer offers. Same reasoning as above: they are dropped from the DOM
        // for a .ostara too, because Ostara itself wrote them until now, and the save schema no longer accepts them.
        // Silent by design, see the functions.
        StripForbidFlippingAttributes(doc);
        StripFollowGrainlineAttributes(doc);

        // Drop the "show copies" attribute the slash-and-spread used to write. Same reasoning again, and the reason
        // it cannot be left to the tool: the attribute is only removed when the operation's options dialog is
        // applied, while a save serialises the element as it came from the file — so an element an older Ostara build
        // wrote would block the pattern's save for good. Silent by design, see the function.
        StripShowCopiesAttributes(doc);

        // Drop the layer tags the layers used to carry. Same reasoning, and the same need for both formats: a .val
        // is read through a schema that still declares the attribute, and an .ostara written by an earlier Ostara
        // build carries it too, while the save schema no longer declares it. Silent by design, see the function.
        StripGroupTags(doc);

        // A cut's two halves, the pieces of an armhole curve and of a segmented spline path, and the pieces a copy
        // of a cut curve is drawn with are objects of their own for the layers; a file written before that rule
        // names none of them. Give an unlayered piece the layers of the drawing it replaces, so it stays with it
        // instead of falling into the automatic "Unnamed" layer.
        //
        // And the copy that NO piece replaces — every cut box off, so the canvas still draws the copy whole —
        // takes the layers of the curve it copies. A build that read the pieces' attribute alone had let the
        // loading pass throw that copy out of its layer, and a curve no layer names is drawn whatever the layers
        // say: an .ostara written by it, and a .val converted by it, came back with curves on screen that their
        // hidden layers had no hold on. This pass puts them back before the stale entries are judged.
        //
        // This runs FIRST, before the stale entries are taken out, and the order is the rule: a layer that still
        // names the copy a piece replaces is exactly the layer that piece belongs to, and cutting the copy's entry
        // beforehand would leave the pass nothing to inherit from.
        CoupleCutSegmentsToBaseLayers(doc);

        // Then take out the layer items that no tool of the piece can produce — a pair naming a tool the file no
        // longer holds, or an object its tool does not draw. Both are acted on when a layer is hidden, and the
        // second one can pin an object of a hidden layer on screen; a copy the canvas replaced with its pieces is
        // one of them, so it leaves the layers here. Silent by design, and it still runs BEFORE the two passes
        // below so they count the orphans on what is left.
        CleanStaleLayerItems(doc);

        // The automatic layer can be left empty by the pass above (a pair no tool can produce was its only
        // content): it is a technical remnant, not a layer of the piece, and it goes before the normalisation
        // below counts the orphans on what is left.
        RemoveEmptyAutomaticLayers(doc);

        // Give every object of every piece a layer. Deliberately OUTSIDE the block above, for the same
        // reason: a .val arrives without layers, and an .ostara written before this rule carries objects
        // that belong to none. Such an object is visible by default and unreachable from the Layers dock,
        // so each piece gets an "Unnamed" layer holding exactly what no layer references. Silent by
        // design, see the function.
        NormalizeUnlayeredObjects(doc);

        // Then pick the layer the canvas will draw into. Same place, same reasoning — and it runs after the pass
        // above so that a freshly created "Unnamed" layer can be the one chosen.
        NormalizeActiveLayer(doc);

        if (!customMeasureFile.isEmpty())
        {
            doc->SetMPath(RelativeMPath(fileName, customMeasureFile));
        }
        VAbstractValApplication::VApp()->SetPatternUnits(doc->Units());

        const QString path = AbsoluteMPath(fileName, doc->MPath());
        QString fixedMPath;

        // A pattern that reads measurements needs a table: the one it points at, or one the reader designates when
        // that link cannot be used. A pattern that reads none is left alone. A reader who gives up aborts the
        // opening — the drawing's formulas read measurements nobody would hold.
        if (not path.isEmpty() || doc->RequiresMeasurements())
        {
            fixedMPath = ResolveMeasurementTable(fileName, path);
            if (fixedMPath.isEmpty())
            {
                VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
                Clear();
                if (path.isEmpty())
                {
                    qCWarning(vMainWindow, "%s",
                              qUtf8Printable(tr("The pattern reads measurements and no table was chosen.")));
                }
                else
                {
                    qCWarning(vMainWindow, "%s",
                              qUtf8Printable(tr("The measurements file '%1' could not be found.").arg(path)));
                }
                if (not VApplication::IsGUIMode())
                {
                    QCoreApplication::exit(V_EX_NOINPUT);
                }
                return PatternLoadResult::Failed;
            }
        }

        if (not fixedMPath.isEmpty())
        {
            // Legacy .vst/.vit → offer conversion or replacement with .omt/.otu
            const QFileInfo fi(fixedMPath);
            const QString suffix = fi.suffix().toLower();
            if (suffix == QStringLiteral("vst") || suffix == QStringLiteral("vit"))
            {

                // Asked again as long as the reader has not settled the question: abandoning the conversion
                // used to drop the whole opening and leave an empty Draw mode behind. Only the Cancel button
                // ends it — and the caller then returns to the Welcome screen.
                QString newPath;
                bool aborted = false;
                while (newPath.isEmpty())
                {
                    QMessageBox msgBox(this);
                    msgBox.setWindowTitle(tr("Legacy measurement file"));
                    msgBox.setText(tr("The pattern is linked to a legacy Tape file:\n\n%1\n\n"
                                       "Ostara uses .omt/.otu format. Choose how to proceed:")
                                       .arg(fi.fileName()));
                    auto *convertBtn = msgBox.addButton(tr("Convert this file"), QMessageBox::AcceptRole);
                    // "Replace with existing .omt" read as "your .vst will be converted in place": a reader
                    // thought she would lose the file she had just opened. The label says what the button
                    // does — point the pattern at a table that already exists. Sophie, 03-11-2026.
                    auto *replaceBtn = msgBox.addButton(tr("Use an existing .omt/.otu"),
                                                         QMessageBox::ActionRole);
                    msgBox.addButton(QMessageBox::Cancel);
                    msgBox.exec();

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

                        // Nothing saved: the conversion was abandoned, not the opening. Back to the question
                        // instead of dropping the reader on an empty canvas.
                        newPath = savedPath;
                    }
                    else if (msgBox.clickedButton() == replaceBtn)
                    {
                        // Open dialog to pick an existing .omt/.otu
                        DialogOpenMeasurements dlg(this);
                        if (dlg.exec() == QDialog::Accepted)
                        {
                            newPath = dlg.selectedPath();
                        }
                        // Refused or empty: the question is asked again.
                    }
                    else
                    {
                        // Cancel: the reader gives the opening up.
                        aborted = true;
                        break;
                    }
                }

                if (aborted)
                {
                    VAbstractValApplication::VApp()->setOpeningPattern();
                    Clear();
                    return PatternLoadResult::Failed;
                }

                fixedMPath = newPath;
                doc->SetMPath(RelativeMPath(fileName, newPath));
            }

            if (not LoadMeasurements(fileName, fixedMPath))
            {
                qCWarning(vMainWindow, "%s",
                          qUtf8Printable(tr("The measurements file '%1' could not be found or "
                                            "provides not enough information.")
                                             .arg(fixedMPath)));
                VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
                Clear();
                if (not VApplication::IsGUIMode())
                {
                    QCoreApplication::exit(V_EX_NOINPUT);
                }
                return PatternLoadResult::Failed;
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
        const QString details = QStringLiteral("%1\n\n%2").arg(e.ErrorMessage(), e.DetailedInformation());
        qCDebug(vMainWindow, "File error.\n\n%s", qUtf8Printable(details));
        if (VApplication::IsGUIMode())
        {
            ShowPatternOpenError(this, details);
        }
        VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
        Clear();
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return PatternLoadResult::Failed;
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

        // A template or a style the pattern names and that is not in its folder draws nothing, silently: said
        // once here, as the pattern becomes readable, because the names stay in the pattern and nothing else
        // would ever mention them.
        if (const QStringList missing = MissingLibraryResources(doc); not missing.isEmpty())
        {
            statusBar()->showMessage(tr("Not found in the library: %1.").arg(missing.join(QStringLiteral(", "))),
                                     10000);
        }

        // Fit scene size to best size for first show
        ZoomFirstShow();

        ActionDraw(true);

        VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
        m_statusLabel->setText(QString());
        return PatternLoadResult::Loaded;
    }

    VAbstractValApplication::VApp()->setOpeningPattern(); // End opening file
    return PatternLoadResult::Failed;
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
    const QString mes = tr("Ostara didn't shut down correctly. Do you want to reopen the files (%1) you had open?")
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
        // The pattern was saved; only the lock could not be taken. See the same case in the watermark window.
        qCWarning(vMainWindow, "%s",
                  qUtf8Printable(tr("Failed to lock. This file already opened in another window. Expect "
                                    "collissions when run 2 copies of the program.")));
    }

    return result;
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitAutoSave()
{
    // Autosaving the pattern every N minutes. N comes from the settings, and is 1 by default.
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
        const QString minute = autoTime == 1 ? QStringLiteral("minute") : QStringLiteral("minutes");
        qCDebug(vMainWindow, "Autosaving every %d %s.", autoTime, qUtf8Printable(minute));
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

    auto shownName = QStringLiteral("  -  ");
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
    static auto fileIcon = QIcon(QCoreApplication::applicationDirPath() + "/../Resources/Ostara.icns"_L1);
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
