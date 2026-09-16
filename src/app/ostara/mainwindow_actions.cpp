/************************************************************************
 **
 **  @file   mainwindow_actions.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   26/08/2026
 **
 **  @brief  Menu/action-building and enable-state methods extracted from mainwindow.cpp
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
#include "themeDef.h"
#include "widgets/ODockManager.h"
#include "widgets/TopBar.h"
#include "widgets/DrawToolBar.h"
#include "core/vapplication.h"
#include "core/vtooloptionspropertybrowser.h"
#include "dialogs/dialogaboutapp.h"
#include "dialogs/dialogpatternproperties.h"
#include "vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/def.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"

#include <QAction>
#include <QComboBox>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QMessageBox>
#include <QTabWidget>
#include <QUndoStack>
#include <QUrl>
#include <QtDebug>

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitActionShortcuts()
{
    m_shortcutActions.insert(VShortcutAction::New, ui->actionNew);
    m_shortcutActions.insert(VShortcutAction::Open, ui->actionOpen);
    m_shortcutActions.insert(VShortcutAction::Save, ui->actionSave);
    m_shortcutActions.insert(VShortcutAction::SaveAs, ui->actionSaveAs);
    m_shortcutActions.insert(VShortcutAction::DrawMode, ui->actionDraw);
    m_shortcutActions.insert(VShortcutAction::DetailsMode, ui->actionDetails);
    m_shortcutActions.insert(VShortcutAction::GradedMode, ui->actionGraded);
    m_shortcutActions.insert(VShortcutAction::LayoutMode, ui->actionLayout);
    m_shortcutActions.insert(VShortcutAction::NewPatternPiece, ui->actionNewDraw);
    m_shortcutActions.insert(VShortcutAction::NextPatternPiece, ui->actionNextPatternPiece);
    m_shortcutActions.insert(VShortcutAction::PreviusPatternPiece, ui->actionPreviousPatternPiece);
    m_shortcutActions.insert(VShortcutAction::TableOfVariables, ui->actionTable);
    m_shortcutActions.insert(VShortcutAction::Quit, ui->actionExit);
    m_shortcutActions.insert(VShortcutAction::LastTool, ui->actionLast_tool);
    m_shortcutActions.insert(VShortcutAction::CurveDetails, ui->actionShowCurveDetails);
    m_shortcutActions.insert(VShortcutAction::FinalMeasurements, ui->actionFinalMeasurements);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateUndoRedoActions(bool enabled)
{
    // The mode decides which histories are available at all. Calculation and Modeling are what
    // the condition here used to call the design stage; Export joins them because it now HAS a
    // history of its own — the hand corrections made on the plan. Layout and Graded stay out,
    // exactly as they were: this was not the place to widen a policy nobody asked to change.
    const Draw mode = VAbstractValApplication::VApp()->GetDrawMode();
    const bool historyStage = (mode == Draw::Calculation || mode == Draw::Modeling
                               || mode == Draw::Export);

    const bool allowed = enabled && historyStage && not m_toolOptionsDialogVisible;

    // canUndo() and canRedo() are asked of the GROUP, and that is the whole reason it exists: it
    // answers for whichever stack the mode on screen is using. Asking the application's stack
    // directly — as all four sites this replaces did — is precisely what let a Ctrl+Z pressed in
    // Export reach the pattern's history.
    undoAction->setEnabled(allowed && m_undoGroup->canUndo());
    redoAction->setEnabled(allowed && m_undoGroup->canRedo());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetEnableWidgets enable action button.
 * @param enable enable value.
 */
void MainWindow::SetEnableWidgets(bool enable)
{
    const bool drawStage = (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Calculation);
    const bool detailsStage = (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Modeling);
    const bool layoutStage = (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Layout);
    const bool designStage = (drawStage || detailsStage);
    const bool piecesStage = (detailsStage || layoutStage);

    const bool enableOnDrawStage = (enable && drawStage);
    const bool enableOnDesignStage = (enable && designStage);
    const bool enableOnDetailsStage = (enable && detailsStage);
    const bool enableOnPiecesStage = (enable && piecesStage);

    m_comboBoxDraws->setEnabled(enableOnDrawStage);
    ui->actionOptionDraw->setEnabled(enableOnDrawStage);
    ui->actionSave->setEnabled(isWindowModified() && enable && not m_patternReadOnly);
    ui->actionSaveAs->setEnabled(enable);
    ui->actionPattern_properties->setEnabled(enableOnDesignStage);
    ui->actionZoomIn->setEnabled(enable);
    ui->actionZoomOut->setEnabled(enable);
    ui->actionArrowTool->setEnabled(enableOnDesignStage);
    ui->actionExportRecipe->setEnabled(enableOnDrawStage);
    ui->actionNewDraw->setEnabled(enableOnDrawStage);
    ui->actionDraw->setEnabled(enable);
    ui->actionDetails->setEnabled(enable);
    ui->actionGraded->setEnabled(enable);
    ui->actionLayout->setEnabled(enable);
    ui->actionExport->setEnabled(enable);
    ui->actionTable->setEnabled(enableOnDesignStage);
    ui->actionExportFinalMeasurementsToCSV->setEnabled(enable);
    ui->actionFinalMeasurements->setEnabled(enable);
    ui->actionZoomFitBest->setEnabled(enable);
    ui->actionZoomFitBestCurrent->setEnabled(enableOnDrawStage);
    ui->actionZoomOriginal->setEnabled(enable);
    ui->actionShowCurveDetails->setEnabled(enableOnDrawStage);
    ui->actionShowSegmentLabels->setEnabled(enableOnDrawStage);
    ui->actionShowAccuracyRadius->setEnabled(enableOnDesignStage);
    ui->actionShowMainPath->setEnabled(enableOnDetailsStage);
    ui->actionBoundaryTogetherWithNotches->setEnabled(enableOnDetailsStage);
    ui->actionPreviousPatternPiece->setEnabled(enableOnDrawStage);
    ui->actionNextPatternPiece->setEnabled(enableOnDrawStage);
    ui->actionAddBackgroundImage->setEnabled(enableOnDrawStage);
    ui->actionIncreaseLabelFont->setEnabled(enable);
    ui->actionDecreaseLabelFont->setEnabled(enable);
    ui->actionOriginalLabelFont->setEnabled(enable);
    ui->actionShowAllLabels->setEnabled(enable);
    ui->actionCreateManualLayout->setEnabled(enableOnPiecesStage);
    ui->actionUpdateManualLayout->setEnabled(enableOnPiecesStage);

    ui->actionLoadWatermark->setEnabled(enable);
    ui->actionRemoveWatermark->setEnabled(enable && not doc->GetWatermarkPath().isEmpty());

    m_dockManager->actionOptions()->setEnabled(enableOnDesignStage);
    m_dockManager->actionGroups()->setEnabled(enableOnDesignStage);
    m_dockManager->actionBackgroundImages()->setEnabled(enableOnDrawStage);

    // Not decided here any more. Undo and Redo follow the mode's own history, and
    // UpdateUndoRedoActions is the single place that answers — this function merely runs on
    // every mode change, so it is one of the paths that has to ask again.
    UpdateUndoRedoActions(enable);

    // Now we don't want allow user call context menu
    m_sceneDraw->EnableTools();
    ui->view->setEnabled(enable);
    ui->view->setAcceptDrops(enable);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateShortcuts()
{
    if (VAbstractShortcutManager *manager = VAbstractValApplication::VApp()->GetShortcutManager())
    {
        manager->UpdateActionShortcuts(m_shortcutActions);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetEnableTool enable button.
 * @param enable enable value.
 */
void MainWindow::SetEnableTool(bool enable)
{
    bool drawTools = false;
    bool modelingTools = false;
    bool layoutTools = false;
    bool exportTools = false;

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (VAbstractValApplication::VApp()->GetDrawMode())
    {
        case Draw::Calculation:
            drawTools = enable;
            break;
        case Draw::Modeling:
            modelingTools = enable;
            break;
        case Draw::Layout:
            layoutTools = enable;
            break;
        case Draw::Graded:
            // Non-creative mode: no tool is enabled in Graded.
            break;
        case Draw::Export:
            // Export mode exposes the selection tool only — no creation tool.
            exportTools = enable;
            break;
    }

    QT_WARNING_POP

    // This check helps to find missed tools
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 69, "Not all tools were handled.");

    // Drawing Tools
    ui->actionEndLineTool->setEnabled(drawTools);
    ui->actionLineTool->setEnabled(drawTools);
    ui->actionAlongLineTool->setEnabled(drawTools);
    ui->actionShoulderPointTool->setEnabled(drawTools);
    ui->actionNormalTool->setEnabled(drawTools);
    ui->actionBisectorTool->setEnabled(drawTools);
    ui->actionLineIntersectTool->setEnabled(drawTools);
    ui->actionCurveTool->setEnabled(drawTools);
    ui->actionSplineTool->setEnabled(drawTools);
    ui->actionCubicBezierTool->setEnabled(drawTools);
    ui->actionArmholeCurveTool->setEnabled(drawTools);
    ui->actionArcTool->setEnabled(drawTools);
    ui->actionSplinePathTool->setEnabled(drawTools);
    ui->actionCubicBezierPathTool->setEnabled(drawTools);
    ui->actionPointOfContactTool->setEnabled(drawTools);
    ui->actionNewDetailTool->setEnabled(drawTools);
    ui->actionInternalPathTool->setEnabled(drawTools);
    ui->actionHeightTool->setEnabled(drawTools);
    ui->actionTriangleTool->setEnabled(drawTools);
    ui->actionPointOfIntersectionTool->setEnabled(drawTools);
    ui->actionCutCurveTool->setEnabled(drawTools);
    ui->actionSplineCutPointTool->setEnabled(drawTools);
    ui->actionSplinePathCutPointTool->setEnabled(drawTools);
    ui->actionArcCutPointTool->setEnabled(drawTools);
    ui->actionLineIntersectAxisTool->setEnabled(drawTools);
    ui->actionCurveIntersectAxisTool->setEnabled(drawTools);
    ui->actionArcIntersectAxisTool->setEnabled(drawTools);
    ui->actionPointOfIntersectionArcsTool->setEnabled(drawTools);
    ui->actionIntersectionCurvesTool->setEnabled(drawTools);
    ui->actionPointOfIntersectionCirclesTool->setEnabled(drawTools);
    ui->actionPointFromCircleAndTangentTool->setEnabled(drawTools);
    ui->actionPointFromArcAndTangentTool->setEnabled(drawTools);
    ui->actionArcWithLengthTool->setEnabled(drawTools);
    ui->actionTrueDartsTool->setEnabled(drawTools);
    ui->actionGroupTool->setEnabled(drawTools);
    ui->actionAttachTool->setEnabled(drawTools);
    ui->actionRotationTool->setEnabled(drawTools);
    ui->actionFlippingByLineTool->setEnabled(drawTools);
    ui->actionFlippingByAxisTool->setEnabled(drawTools);
    ui->actionMoveTool->setEnabled(drawTools);
    ui->actionMidpointTool->setEnabled(drawTools);
    ui->actionEllipticalArcTool->setEnabled(drawTools);
    ui->actionInsertNodeTool->setEnabled(drawTools);
    ui->actionPlaceLabelTool->setEnabled(drawTools);
    ui->actionExportDraw->setEnabled(drawTools);
    ui->actionArcStartPointTool->setEnabled(drawTools);
    ui->actionArcEndPointTool->setEnabled(drawTools);
    ui->actionEllipticalArcWithLengthTool->setEnabled(drawTools);
    ui->actionParallelCurveTool->setEnabled(drawTools);
    ui->actionGraduatedCurveTool->setEnabled(drawTools);

    ui->actionLast_tool->setEnabled(drawTools);

    ui->actionToolSelect->setEnabled(drawTools || modelingTools || exportTools);
    ui->actionToolSelect->setChecked(drawTools || modelingTools || exportTools);

    // Modeling Tools
    ui->actionUnionDetailsTool->setEnabled(modelingTools);
    ui->actionDetailExportAs->setEnabled(modelingTools);
    ui->actionDuplicateDetailTool->setEnabled(modelingTools);

    // Layout tools
    ui->actionLayoutSettings->setEnabled(layoutTools);
    ui->actionLayoutExportAs->setEnabled(layoutTools && not m_layoutSettings->LayoutScenes().isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SetLayoutModeActions()
{
    const bool enabled = not m_layoutSettings->LayoutScenes().isEmpty();

    ui->actionLayoutExportAs->setEnabled(enabled);
    ui->actionExportAs->setEnabled(enabled);
    ui->actionPrintPreview->setEnabled(enabled);
    ui->actionPrintPreviewTiled->setEnabled(enabled);
    ui->actionPrint->setEnabled(enabled);
    ui->actionPrintTiled->setEnabled(enabled);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::CreateMenus()
{
    for (int i = 0; i < MaxRecentFiles; ++i)
    {
        ui->menuFile->insertAction(ui->actionPreferences, m_recentFileActs.at(i));
    }
    m_separatorAct = new QAction(this);
    m_separatorAct->setSeparator(true);
    ui->menuFile->insertAction(ui->actionPreferences, m_separatorAct);
    UpdateRecentFileActions();

    // Add Undo/Redo actions.
    //
    // Built from the GROUP, not from a stack. QUndoStack::createUndoAction wires the action to
    // itself — triggering it calls that stack's undo() — so keeping the pattern's stack here
    // would have left that wiring in place, and re-connecting `triggered` would not have removed
    // it: the action would go on reaching the pattern's history no matter what Ctrl+Z was meant
    // to reach. The group's version does the same thing against whichever stack is active, and
    // switchScene is what decides which one that is.
    undoAction = m_undoGroup->createUndoAction(this, tr("&Undo"));
    connect(undoAction, &QAction::triggered, m_dockManager->toolOptions(), &VToolOptionsPropertyBrowser::RefreshOptions);
    m_shortcutActions.insert(VShortcutAction::Undo, undoAction);
    undoAction->setIcon(FromTheme(VThemeIcon::EditUndo));
    ui->menuPatternPiece->insertAction(ui->actionLast_tool, undoAction);

    redoAction = m_undoGroup->createRedoAction(this, tr("&Redo"));
    connect(redoAction, &QAction::triggered, m_dockManager->toolOptions(), &VToolOptionsPropertyBrowser::RefreshOptions);
    m_shortcutActions.insert(VShortcutAction::Redo, redoAction);
    redoAction->setIcon(FromTheme(VThemeIcon::EditRedo));
    ui->menuPatternPiece->insertAction(ui->actionLast_tool, redoAction);

    // The actions' enabled state belongs to UpdateUndoRedoActions from here on, and these two
    // connections are how it hears that the answer may have changed. They replace the pair of
    // lambdas that used to watch the stack for `!m_guiEnabled`: those only ever disabled, and
    // the group re-derives the state on every activeStackChanged, so an isolated setDisabled was
    // overwritten the next time the user changed mode.
    //
    // Order matters and works out: the group's own connection, installed inside createUndoAction,
    // runs first and sets enabled from canUndo(); these run after and apply the mode's policy on
    // top of it. Qt invokes slots in connection order, and these are connected later.
    connect(m_undoGroup, &QUndoGroup::canUndoChanged, this, [this]() { UpdateUndoRedoActions(m_guiEnabled); });
    connect(m_undoGroup, &QUndoGroup::canRedoChanged, this, [this]() { UpdateUndoRedoActions(m_guiEnabled); });


    m_separatorAct = new QAction(this);
    m_separatorAct->setSeparator(true);
    ui->menuFile->insertAction(ui->actionPattern_properties, m_separatorAct);

    setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::CreateActions()
{
    ui->setupUi(this);

    connect(ui->actionArrowTool, &QAction::triggered, this, &MainWindow::ArrowTool);
    connect(ui->actionDraw, &QAction::triggered, this, &MainWindow::ActionDraw);
    connect(ui->actionDetails, &QAction::triggered, this, &MainWindow::ActionDetails);

    // Leaving Export mode must give the TopBar chrome and the work toolbar back. The
    // Graded handler lives in the Graded files and knows nothing about Export, so the
    // exit is wired on the actions themselves: every handler that leaves the mode
    // unchecks actionExport, and that lands in the lambda below. Registered before the
    // Graded handler so it runs first, while the mode is still Export.
    connect(ui->actionGraded, &QAction::triggered, this, [this](bool checked) {
        if (checked)
        {
            ui->actionExport->setChecked(false);
        }
    });
    connect(ui->actionExport, &QAction::toggled, this, [this](bool checked) {
        if (checked || m_topBar == nullptr)
        {
            return;
        }
        m_topBar->SetExportChrome(false);
        m_drawToolBar->show();
    });

    connect(ui->actionGraded, &QAction::triggered, this, &MainWindow::ActionGraded);
    connect(ui->actionLayout, &QAction::triggered, this, &MainWindow::ActionLayout);
    connect(ui->actionExport, &QAction::triggered, this, &MainWindow::ActionExport);
    connect(ui->actionExportRecipe, &QAction::triggered, this, &MainWindow::ActionExportRecipe_triggered);
    connect(ui->actionNewDraw, &QAction::triggered, this, &MainWindow::ActionNewDraw_triggered);
    connect(ui->actionExportFinalMeasurementsToCSV, &QAction::triggered, this, &MainWindow::ExportFMeasurementsToCSV);
    connect(ui->actionTable, &QAction::triggered, this, &MainWindow::ActionTable_triggered);
    connect(ui->actionFinalMeasurements, &QAction::triggered, this, &MainWindow::ActionFinalMeasurements_triggered);
    connect(ui->actionInfoBlockTemplates, &QAction::triggered, this, [this]() { ActionInfoBlock_triggered(0); });
    connect(ui->actionInfoBlockCreate, &QAction::triggered, this, [this]() { ActionInfoBlock_triggered(1); });
    connect(ui->actionInfoBlockLogo, &QAction::triggered, this, [this]() { ActionInfoBlock_triggered(2); });
    connect(ui->actionInfoBlockClients, &QAction::triggered, this, [this]() { ActionInfoBlock_triggered(3); });
    connect(ui->actionInfoBlockCollections, &QAction::triggered, this, [this]() { ActionInfoBlock_triggered(4); });
    connect(ui->actionInfoBlockAnnotations, &QAction::triggered, this, [this]() { ActionInfoBlock_triggered(5); });

    connect(ui->actionAbout_Qt, &QAction::triggered, this, [this]() { QMessageBox::aboutQt(this, tr("About Qt")); });

    connect(ui->actionAbout_Valentina, &QAction::triggered, this,
            [this]()
            {
                auto *aboutDialog = new DialogAboutApp(this);
                aboutDialog->setAttribute(Qt::WA_DeleteOnClose, true);
                aboutDialog->show();
            });

    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::close);

    connect(ui->actionPreferences, &QAction::triggered, this, &MainWindow::Preferences);
    connect(ui->actionReportBug, &QAction::triggered, this,
            []()
            {
                qCDebug(vMainWindow, "Reporting bug");
                QDesktopServices::openUrl(
                    QUrl(QStringLiteral("https://gitlab.com/smart-pattern/valentina/issues/new")));
            });
    connect(ui->actionShop, &QAction::triggered, this,
            []()
            {
                qCDebug(vMainWindow, "Open shop");
                QDesktopServices::openUrl(QUrl(QStringLiteral("https://smart-pattern.com.ua/catalogue/")));
            });
    connect(ui->actionSettingsDirectory, &QAction::triggered, this,
            []()
            {
                ShowInGraphicalShell(VAbstractValApplication::VApp()->ValentinaSettings()->fileName());
            });

    connect(ui->actionLast_tool, &QAction::triggered, this, &MainWindow::LastUsedTool);

    connect(ui->actionPattern_properties, &QAction::triggered, this,
            [this]()
            {
                DialogPatternProperties proper(doc, pattern, this);
                connect(&proper, &DialogPatternProperties::UpddatePieces, m_sceneDetails,
                        &VMainGraphicsScene::UpdatePiecePassmarks);
                proper.exec();
            });

    ui->actionPattern_properties->setEnabled(false);
    connect(ui->actionClosePattern, &QAction::triggered, this,
            [this]()
            {
                if (MaybeSave())
                {
                    FileClosedCorrect();
                    Clear();
                    emit patternClosed();
                }
            });

    ui->actionShowCurveDetails->setChecked(VAbstractValApplication::VApp()->ValentinaSettings()->IsShowCurveDetails());
    connect(ui->actionShowCurveDetails, &QAction::triggered, this,
            [this](bool checked) -> void
            {
                emit ui->view->itemClicked(nullptr);
                VAbstractValApplication::VApp()->ValentinaSettings()->SetShowCurveDetails(checked);
                m_sceneDraw->EnableDetailsMode(checked);
            });

    ui->actionShowAccuracyRadius->setChecked(
        VAbstractValApplication::VApp()->ValentinaSettings()->GetShowAccuracyRadius());
    connect(ui->actionShowAccuracyRadius, &QAction::triggered, this,
            [this](bool checked)
            {
                VAbstractValApplication::VApp()->ValentinaSettings()->SetShowAccuracyRadius(checked);
                m_sceneDetails->update();
                m_sceneDraw->update();
            });

    ui->actionShowMainPath->setChecked(VAbstractValApplication::VApp()->ValentinaSettings()->IsPieceShowMainPath());
    connect(ui->actionShowMainPath, &QAction::triggered, this, &MainWindow::ActionShowMainPath_triggered);

    ui->actionBoundaryTogetherWithNotches->setChecked(
        VAbstractValApplication::VApp()->ValentinaSettings()->IsBoundaryTogetherWithNotches());
    connect(ui->actionBoundaryTogetherWithNotches, &QAction::triggered, this,
            &MainWindow::ActionBoundaryTogetherWithNotches_triggered);

    // Ostara Measurement (not Tape)
    connect(ui->actionLoadMeasurements, &QAction::triggered, this, &MainWindow::OnLoadMeasurements);
    connect(ui->actionEditMeasurements, &QAction::triggered, this, &MainWindow::OnEditMeasurements);
    connect(ui->actionCreateMeasurements, &QAction::triggered, this, &MainWindow::OnNewMeasurements);
    connect(ui->actionManageMeasurements, &QAction::triggered, this, &MainWindow::ActionManageMeasurements);
    connect(ui->actionExportAs, &QAction::triggered, this, &MainWindow::ExportLayoutAs);
    connect(ui->actionPrintPreview, &QAction::triggered, this, &MainWindow::PrintPreviewOrigin);
    connect(ui->actionPrintPreviewTiled, &QAction::triggered, this, &MainWindow::PrintPreviewTiled);
    connect(ui->actionPrint, &QAction::triggered, this, &MainWindow::PrintOrigin);
    connect(ui->actionPrintTiled, &QAction::triggered, this, &MainWindow::PrintTiled);

    // Actions for recent files loaded by a main window application.
    for (int i = 0; i < MaxRecentFiles; ++i)
    {
        auto *action = new QAction(this);
        action->setVisible(false);
        m_recentFileActs[i] = action;
        connect(m_recentFileActs[i], &QAction::triggered, this,
                [this]()
                {
                    if (auto *action = qobject_cast<QAction *>(sender()))
                    {
                        const QString filePath = action->data().toString();
                        if (not filePath.isEmpty())
                        {
                            LoadPattern(filePath);
                        }
                    }
                });
    }


    connect(ui->actionWatermarkEditor, &QAction::triggered, this, &MainWindow::CreateWatermark);
    connect(ui->actionLoadWatermark, &QAction::triggered, this, &MainWindow::LoadWatermark);
    connect(ui->actionRemoveWatermark, &QAction::triggered, this, &MainWindow::RemoveWatermark);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SetToolOptionsDialogVisible(bool visible)
{
    m_toolOptionsDialogVisible = visible;

    // While a tool options dialog is open we must not let the user run undo/redo. An undo/redo emits
    // VPattern::UndoCommand which triggers MainWindow::FullParseFile(), tearing down all tools and the
    // VContainer the open dialog still references. Accepting the now-stale dialog afterwards dereferences
    // freed geometry (use-after-free, e.g. crash in DialogSplinePath::SaveData).
    // m_toolOptionsDialogVisible has just been set above, so this one call carries the whole
    // decision — including the mode. The old pair of branches reached straight for the pattern's
    // stack and ignored the mode entirely: opening and closing a tool options dialog in Export
    // re-enabled Undo on that stack's contents, which is how the leak showed in practice.
    UpdateUndoRedoActions(m_guiEnabled);
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SetEnabledGUI(bool enabled)
{
    if (m_guiEnabled != enabled)
    {
        if (not enabled)
        {
            ArrowTool(true);
            VAbstractApplication::VApp()->getUndoStack()->clear();

            // The pattern is going away, and it takes the generations with it. A selection
            // naming a footprint of one, and a history of gestures made on it, would both be
            // left pointing at a result that no longer exists — and the undo stack would go on
            // offering them in the menu.
            //
            // Reset() is documented for exactly this ("the pattern is closing, or the GUI is
            // going away") and had no caller until now: the intent was written, the wiring
            // never was.
            m_exportCorrection->Reset();
        }
        SetEnableWidgets(enabled);

        m_guiEnabled = enabled;

        SetEnableTool(enabled);

#ifndef QT_NO_CURSOR
        if (not enabled)
        {
            QGuiApplication::setOverrideCursor(Qt::ArrowCursor);
        }
        else
        {
            QGuiApplication::restoreOverrideCursor();
        }
#endif
    }
}
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitToolButtons()
{
    // This check helps to find missed tools
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 69, "Check if all tools were connected.");

    auto InitToolButton = [this](VShortcutAction type, QAction *action, void (MainWindow::*slotFunction)(bool))
    {
        m_shortcutActions.insert(type, action);
        connect(action, &QAction::triggered, this, slotFunction);
    };

    InitToolButton(VShortcutAction::ToolArrow, ui->actionToolSelect, &MainWindow::ArrowTool);
    // ApplicationShortcut so "V" also fires while a tool dialog (a separate window) has focus.
    ui->actionToolSelect->setShortcutContext(Qt::ApplicationShortcut);
    InitToolButton(VShortcutAction::ToolEndLine, ui->actionEndLineTool, &MainWindow::ToolEndLine);
    InitToolButton(VShortcutAction::ToolLine, ui->actionLineTool, &MainWindow::ToolLine);
    InitToolButton(VShortcutAction::ToolAlongLine, ui->actionAlongLineTool, &MainWindow::ToolAlongLine);
    InitToolButton(VShortcutAction::ToolShoulderPoint, ui->actionShoulderPointTool, &MainWindow::ToolShoulderPoint);
    InitToolButton(VShortcutAction::ToolNormal, ui->actionNormalTool, &MainWindow::ToolNormal);
    InitToolButton(VShortcutAction::ToolBisector, ui->actionBisectorTool, &MainWindow::ToolBisector);
    InitToolButton(VShortcutAction::ToolLineIntersect, ui->actionLineIntersectTool, &MainWindow::ToolLineIntersect);
    InitToolButton(VShortcutAction::ToolSpline, ui->actionSplineTool, &MainWindow::ToolSpline);
    InitToolButton(VShortcutAction::ToolCubicBezier, ui->actionCubicBezierTool, &MainWindow::ToolCubicBezier);
    InitToolButton(VShortcutAction::ToolArc, ui->actionArcTool, &MainWindow::ToolArc);
    InitToolButton(VShortcutAction::ToolSplinePath, ui->actionSplinePathTool, &MainWindow::ToolSplinePath);
    InitToolButton(VShortcutAction::ToolCurve, ui->actionCurveTool, &MainWindow::ToolCurve);
    InitToolButton(VShortcutAction::ToolCubicBezierPath, ui->actionCubicBezierPathTool,
                   &MainWindow::ToolCubicBezierPath);
    InitToolButton(VShortcutAction::ToolPointOfContact, ui->actionPointOfContactTool, &MainWindow::ToolPointOfContact);
    InitToolButton(VShortcutAction::ToolPiece, ui->actionNewDetailTool, &MainWindow::ToolDetail);
    InitToolButton(VShortcutAction::ToolPiecePath, ui->actionInternalPathTool, &MainWindow::ToolPiecePath);
    InitToolButton(VShortcutAction::ToolHeight, ui->actionHeightTool, &MainWindow::ToolHeight);
    InitToolButton(VShortcutAction::ToolTriangle, ui->actionTriangleTool, &MainWindow::ToolTriangle);
    InitToolButton(VShortcutAction::ToolPointOfIntersection, ui->actionPointOfIntersectionTool,
                   &MainWindow::ToolPointOfIntersection);
    InitToolButton(VShortcutAction::ToolCutSpline, ui->actionSplineCutPointTool, &MainWindow::ToolCutSpline);
    InitToolButton(VShortcutAction::ToolCutSplinePath, ui->actionSplinePathCutPointTool,
                   &MainWindow::ToolCutSplinePath);
    InitToolButton(VShortcutAction::ToolCutCurve, ui->actionCutCurveTool, &MainWindow::ToolCutCurve);
    InitToolButton(VShortcutAction::ToolUnionDetails, ui->actionUnionDetailsTool, &MainWindow::ToolUnionDetails);
    InitToolButton(VShortcutAction::ToolDuplicateDetail, ui->actionDuplicateDetailTool,
                   &MainWindow::ToolDuplicateDetail);
    InitToolButton(VShortcutAction::ToolCutArc, ui->actionArcCutPointTool, &MainWindow::ToolCutArc);
    InitToolButton(VShortcutAction::ToolLineIntersectAxis, ui->actionLineIntersectAxisTool,
                   &MainWindow::ToolLineIntersectAxis);
    InitToolButton(VShortcutAction::ToolCurveIntersectAxis, ui->actionCurveIntersectAxisTool,
                   &MainWindow::ToolCurveIntersectAxis);
    InitToolButton(VShortcutAction::ToolArcIntersectAxis, ui->actionArcIntersectAxisTool,
                   &MainWindow::ToolArcIntersectAxis);
    InitToolButton(VShortcutAction::LayoutSettings, ui->actionLayoutSettings, &MainWindow::ToolLayoutSettings);
    InitToolButton(VShortcutAction::ToolPointOfIntersectionArcs, ui->actionPointOfIntersectionArcsTool,
                   &MainWindow::ToolPointOfIntersectionArcs);
    InitToolButton(VShortcutAction::ToolPointOfIntersectionCircles, ui->actionPointOfIntersectionCirclesTool,
                   &MainWindow::ToolPointOfIntersectionCircles);
    InitToolButton(VShortcutAction::ToolPointOfIntersectionCurves, ui->actionIntersectionCurvesTool,
                   &MainWindow::ToolPointOfIntersectionCurves);
    InitToolButton(VShortcutAction::ToolPointFromCircleAndTangent, ui->actionPointFromCircleAndTangentTool,
                   &MainWindow::ToolPointFromCircleAndTangent);
    InitToolButton(VShortcutAction::ToolPointFromArcAndTangent, ui->actionPointFromArcAndTangentTool,
                   &MainWindow::ToolPointFromArcAndTangent);
    InitToolButton(VShortcutAction::ToolArcWithLength, ui->actionArcWithLengthTool, &MainWindow::ToolArcWithLength);
    InitToolButton(VShortcutAction::ToolTrueDarts, ui->actionTrueDartsTool, &MainWindow::ToolTrueDarts);
    InitToolButton(VShortcutAction::ToolGroup, ui->actionGroupTool, &MainWindow::ToolGroup);
    InitToolButton(VShortcutAction::ToolAttach, ui->actionAttachTool, &MainWindow::ToolAttach);
    InitToolButton(VShortcutAction::ToolRotation, ui->actionRotationTool, &MainWindow::ToolRotation);
    InitToolButton(VShortcutAction::ToolFlippingByLine, ui->actionFlippingByLineTool, &MainWindow::ToolFlippingByLine);
    InitToolButton(VShortcutAction::ToolFlippingByAxis, ui->actionFlippingByAxisTool, &MainWindow::ToolFlippingByAxis);
    InitToolButton(VShortcutAction::ToolMove, ui->actionMoveTool, &MainWindow::ToolMove);
    InitToolButton(VShortcutAction::ToolMidpoint, ui->actionMidpointTool, &MainWindow::ToolMidpoint);
    InitToolButton(VShortcutAction::ExportDrawAs, ui->actionExportDraw, &MainWindow::ExportDrawAs);
    InitToolButton(VShortcutAction::ExportLayoutAs, ui->actionLayoutExportAs, &MainWindow::ExportLayoutAs);
    InitToolButton(VShortcutAction::ExportDetailsAs, ui->actionDetailExportAs, &MainWindow::ExportDetailsAs);
    InitToolButton(VShortcutAction::ToolEllipticalArc, ui->actionEllipticalArcTool, &MainWindow::ToolEllipticalArc);
    InitToolButton(VShortcutAction::ToolInsertNode, ui->actionInsertNodeTool, &MainWindow::ToolInsertNode);
    InitToolButton(VShortcutAction::ToolPlaceLabel, ui->actionPlaceLabelTool, &MainWindow::ToolPlaceLabel);
    InitToolButton(VShortcutAction::ToolArcStart, ui->actionArcStartPointTool, &MainWindow::ToolArcStart);
    InitToolButton(VShortcutAction::ToolArcEnd, ui->actionArcEndPointTool, &MainWindow::ToolArcEnd);
    InitToolButton(VShortcutAction::ToolEllipticalArcWithLength, ui->actionEllipticalArcWithLengthTool,
                   &MainWindow::ToolEllipticalArcWithLength);
    InitToolButton(VShortcutAction::ToolParallelCurve, ui->actionParallelCurveTool, &MainWindow::ToolParallelCurve);
    InitToolButton(VShortcutAction::ToolGraduatedCurve, ui->actionGraduatedCurveTool, &MainWindow::ToolGraduatedCurve);
    InitToolButton(VShortcutAction::ToolArmholeCurve, ui->actionArmholeCurveTool, &MainWindow::ToolArmholeCurve);
}
