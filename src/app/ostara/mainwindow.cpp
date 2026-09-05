/************************************************************************
 **
 **  @file   mainwindow.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   November 15, 2013
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2013-2015 Valentina project
 **  <https://gitlab.com/smart-pattern/valentina> All Rights Reserved.
 **
 **  Valentina is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Valentina is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Valentina.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "mainwindow.h"
#include "../vexception/vexception.h"
#include "widgets/DrawToolBar.h"
#include "widgets/ODockManager.h"
#include "widgets/owidgetsizeranges.h"
#include "widgets/TopBar.h"
#include "widgets/BreadcrumbBar.h"

#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QPointer>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QPainter>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QEventLoop>
#include <QFuture>
#include <QFutureWatcher>
#include <QGlobalStatic>
#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QScopeGuard>
#include <QScrollBar>
#include <QSettings>
#include <QShowEvent>
#include <QStyleFactory>
#include <QTimer>
#include <QUndoStack>
#include <QUuid>
#include <QtConcurrent>
#include <QtDebug>
#include <QtGlobal>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#include "../ifc/xml/utils.h"
#include "../vmisc/customevents.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "../vmisc/def.h"
#include "vtheme.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolbasepoint.h"
#include "../vtools/tools/oannotationitem.h"
#include "../vtools/tools/oinfoblockitem.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/undoattachment.h"
#include "../vwidgets/vgrainlineitem.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vtools/tools/nodeDetails/vnodepoint.h"
#include "core/vapplication.h"
#include "core/odetailspropertybrowser.h"
#include "core/opiecegroupholderitem.h"
#include "core/vtooloptionspropertybrowser.h"
#include "WelcomeScreen.h"
#include <QStackedWidget>
#include "dialogs/vwidgetbackgroundimages.h"
#include "dialogs/vwidgetdetails.h"
#include "dialogs/vwidgetdependencies.h"
#include "vabstractapplication.h"
#include "vabstractshortcutmanager.h"
#include "../ifc/xml/vpatternblockmapper.h"


#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QSourceLocation>
#endif

#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#endif

#if defined(Q_OS_MAC)
#include <QDrag>
#include <QMimeData>
#endif // defined(Q_OS_MAC)

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

// On Windows, <windows.h> (included indirectly by some Qt headers or libraries)
// defines a macro named 'DocumentProperties' (and 'DocumentPropertiesW').
// This conflicts with the Qt enum value QIcon::ThemeIcon::DocumentProperties,
// causing a compiler error like:
//   error C2838: 'DocumentPropertiesW': illegal qualified name in member declaration
//
// To fix this, we undefine the conflicting macro before including UI header.

#ifdef Q_OS_WIN
#ifdef DocumentProperties
#  undef DocumentProperties
#endif
#endif

#include "ui_mainwindow.h"

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

#include "mainwindow_helpers.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief MainWindow constructor.
 * @param parent parent widget.
 */
MainWindow::MainWindow(QWidget *parent)
  : MainWindowsNoGUI(parent),
    ui(new Ui::MainWindow),
    m_dialogTable(nullptr),
    m_dialogFMeasurements(nullptr),
    m_progressBar(new QProgressBar(this)),
    m_statusLabel(new QLabel(this)),
    m_gradation(new QTimer(this))
{
    CreateActions();
    InitScenes();

    connect(m_gradation, &QTimer::timeout, this, &MainWindow::GradationChanged);

    doc = new VPattern(pattern, m_sceneDraw, m_sceneDetails);
    connect(doc, &VPattern::ClearMainWindow, this, &MainWindow::Clear);
    connect(doc, &VPattern::patternChanged, this, &MainWindow::PatternChangesWereSaved);
    connect(doc, &VPattern::UndoCommand, this, &MainWindow::FullParseFile, Qt::QueuedConnection);
    connect(doc, &VPattern::SetEnabledGUI, this, &MainWindow::SetEnabledGUI);
    connect(doc, &VPattern::CheckLayout, this,
            [this]()
            {
                if (pattern->DataPieces()->isEmpty() && not ui->actionDraw->isChecked())
                {
                    ActionDraw(true);
                }
            });
    connect(doc, &VPattern::ShowPatternBlock, this, &MainWindow::GlobalChangePP);
    connect(doc, &VPattern::MadeProgress, this, &MainWindow::ShowProgress);
    VAbstractValApplication::VApp()->setCurrentDocument(doc);

    ui->view->setFrameStyle(QFrame::NoFrame);
    ui->view->viewport()->setContentsMargins(0, 0, 0, 0);

    // Created before InitDocksContain() — the breadcrumb signal connections live there.
    m_breadcrumb = new BreadcrumbBar(this);

    // Owns dock content, title bars, toggle actions and per-mode dock state.
    m_dockManager = new ODockManager(ui, this, this);

    InitDocksContain();
    CreateMenus();

    m_topBar = new TopBar(this);
    ui->topBar->addWidget(m_topBar);

    ToolBarDraws();
    ToolBarStages();
    ToolBarMeasurements();
    m_drawToolBar = new DrawToolBar(this);
    addDockWidget(Qt::LeftDockWidgetArea, m_drawToolBar);
    m_drawToolBar->RestoreSavedWidth();
    m_drawToolBar->SetActions({
        {QStringLiteral("arrow_cursor"),                  ui->actionToolSelect},
        {QStringLiteral("segment"),                      ui->actionEndLineTool},
        {QStringLiteral("along_line"),                   ui->actionAlongLineTool},
        {QStringLiteral("midpoint"),                     ui->actionMidpointTool},
        {QStringLiteral("normal"),                       ui->actionNormalTool},
        {QStringLiteral("bisector"),                     ui->actionBisectorTool},
        {QStringLiteral("height"),                       ui->actionHeightTool},
        {QStringLiteral("point_of_intersection"),        ui->actionPointOfIntersectionTool},
        {QStringLiteral("intersect"),                    ui->actionLineIntersectTool},
        {QStringLiteral("shoulder"),                     ui->actionShoulderPointTool},
        {QStringLiteral("triangle"),                     ui->actionTriangleTool},
        {QStringLiteral("line_intersect_axis"),          ui->actionLineIntersectAxisTool},
        {QStringLiteral("curve_intersect_axis"),         ui->actionCurveIntersectAxisTool},
        {QStringLiteral("arc_intersect_axis"),           ui->actionArcIntersectAxisTool},
        {QStringLiteral("curve_cut_point"),              ui->actionCutCurveTool},
        {QStringLiteral("arc_cut"),                      ui->actionArcCutPointTool},
        {QStringLiteral("arc_start"),                    ui->actionArcStartPointTool},
        {QStringLiteral("arc_end"),                      ui->actionArcEndPointTool},
        {QStringLiteral("intersection_curves"),          ui->actionIntersectionCurvesTool},
        {QStringLiteral("point_of_intersection_arcs"),   ui->actionPointOfIntersectionArcsTool},
        {QStringLiteral("point_of_intersection_circles"),ui->actionPointOfIntersectionCirclesTool},
        {QStringLiteral("point_from_arc_and_tangent"),   ui->actionPointFromArcAndTangentTool},
        {QStringLiteral("point_from_circle_and_tangent"),ui->actionPointFromCircleAndTangentTool},
        {QStringLiteral("point_of_contact"),             ui->actionPointOfContactTool},
        {QStringLiteral("line"),                         ui->actionLineTool},
        {QStringLiteral("curve"),                        ui->actionCurveTool},
        {QStringLiteral("cubic_bezier"),                 ui->actionCubicBezierTool},
        {QStringLiteral("armhole_curve"),               ui->actionArmholeCurveTool},
        {QStringLiteral("cubic_bezier_path"),            ui->actionCubicBezierPathTool},
        {QStringLiteral("arc"),                          ui->actionArcTool},
        {QStringLiteral("arc_with_length"),              ui->actionArcWithLengthTool},
        {QStringLiteral("el_arc"),                       ui->actionEllipticalArcTool},
        {QStringLiteral("el_arc_with_length"),           ui->actionEllipticalArcWithLengthTool},
        {QStringLiteral("parallel_curve"),               ui->actionParallelCurveTool},
        {QStringLiteral("graduated_curve_offseting"),    ui->actionGraduatedCurveTool},
        {QStringLiteral("group_plus"),                   ui->actionGroupTool},
        {QStringLiteral("attach"),                       ui->actionAttachTool},
        {QStringLiteral("flipping_axis"),                ui->actionFlippingByAxisTool},
        {QStringLiteral("flipping_line"),                ui->actionFlippingByLineTool},
        {QStringLiteral("rotation"),                     ui->actionRotationTool},
        {QStringLiteral("move"),                         ui->actionMoveTool},
        {QStringLiteral("true_darts"),                   ui->actionTrueDartsTool},
        {QStringLiteral("new_detail"),                   ui->actionNewDetailTool},
        {QStringLiteral("path"),                         ui->actionInternalPathTool},
        {QStringLiteral("pin"),                          ui->actionPinTool},
        {QStringLiteral("insert_node"),                  ui->actionInsertNodeTool},
        {QStringLiteral("place_label"),                  ui->actionPlaceLabelTool},
        {QStringLiteral("union"),                        ui->actionUnionDetailsTool},
        {QStringLiteral("duplicate_detail"),             ui->actionDuplicateDetailTool},
        {QStringLiteral("detail_export"),                ui->actionDetailExportAs},
    });
    InitToolButtons();
    InitActionShortcuts();

    connect(ui->actionAddBackgroundImage, &QAction::triggered, this, &MainWindow::ActionAddBackgroundImage);

    m_progressBar->setVisible(false);
#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_taskbarProgress->setVisible(false);
#endif
    m_statusLabel->setText(tr("Create new pattern piece to start working."));
    statusBar()->addPermanentWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_progressBar, 1);

    ToolBarTools();

    connect(VAbstractApplication::VApp()->getUndoStack(), &QUndoStack::cleanChanged, this,
            &MainWindow::PatternChangesWereSaved);

    // --- WelcomeScreen integration (single-window) ---
    // MUST run before ReadSettings() — restoreState() snapshots the layout,
    // and the snapshot must capture the QStackedWidget as central widget,
    // not the old central widget which takeCentralWidget() destroys.
    m_stackedWidget = new QStackedWidget(this);
    m_welcomeScreen = new WelcomeScreen();

    QWidget *oldCentral = takeCentralWidget(); // detaches without deleting
    oldCentral->setContentsMargins(0, 0, 0, 0);
    if (auto *oldLayout = oldCentral->layout())
    {
        oldLayout->setContentsMargins(0, 0, 0, 0);
        oldLayout->setSpacing(0);
    }

    // Wrap the editor in a vertical layout with the attachment breadcrumb above the canvas.
    auto *editorContainer = new QWidget(this);
    auto *editorLayout = new QVBoxLayout(editorContainer);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);
    editorLayout->addWidget(m_breadcrumb);
    editorLayout->addWidget(oldCentral);

    m_stackedWidget->addWidget(editorContainer);  // index 0 — editor (wraps ui->view)
    m_stackedWidget->addWidget(m_welcomeScreen); // index 1 — welcome
    setCentralWidget(m_stackedWidget);
    m_stackedWidget->setCurrentWidget(m_welcomeScreen); // start on welcome

    InitWelcomePage();

    connect(this, &MainWindow::patternClosed, this, &MainWindow::ShowWelcomeScreen);

    // macOS: set MenuRoles so the app menu (About, Preferences, Quit) works
    ui->actionExit->setMenuRole(QAction::QuitRole);
    ui->actionAbout_Valentina->setMenuRole(QAction::AboutRole);
    ui->actionPreferences->setMenuRole(QAction::PreferencesRole);

    InitAutoSave();

    ReadSettings();

    setCurrentFile(QString());
    WindowsLocale();

    connect(ui->listWidget, &QListWidget::currentRowChanged, this, &MainWindow::ShowPaper);
    ui->dockWidgetLayoutPages->setVisible(false);

#if defined(Q_OS_MAC)
    setUnifiedTitleAndToolBarOnMac(true);

    // Mac OS Dock Menu
    QMenu *menu = new QMenu(this);

    QAction *actionNewPattern = menu->addAction(tr("New pattern"));
    actionNewPattern->setMenuRole(QAction::NoRole);
    connect(actionNewPattern, &QAction::triggered, this, &MainWindow::on_actionNew_triggered);

    QAction *actionOpenPattern = menu->addAction(tr("Open pattern"));
    actionOpenPattern->setMenuRole(QAction::NoRole);
    connect(actionOpenPattern, &QAction::triggered, this, &MainWindow::on_actionOpen_triggered);

    QAction *actionNewMeasurement = menu->addAction(tr("Create/Edit measurements"));
    actionNewMeasurement->setMenuRole(QAction::NoRole);
    connect(actionNewMeasurement, &QAction::triggered, this, &MainWindow::OnNewMeasurements);

    QAction *actionPreferences = menu->addAction(tr("Preferences"));
    actionPreferences->setMenuRole(QAction::NoRole);
    connect(actionPreferences, &QAction::triggered, this, &MainWindow::Preferences);

    menu->setAsDockMenu();
#endif // defined(Q_OS_MAC)

    connect(ui->toolButtonMessagesZoomInFont, &QToolButton::clicked, this,
            [this]()
            {
                VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
                QFont f = ui->plainTextEditPatternMessages->font();
                if (f.pointSize() < VValentinaSettings::GetDefMaxPatternMessageFontSize())
                {
                    f.setPointSize(qMax(f.pointSize() + 1, 1));
                    ui->plainTextEditPatternMessages->setFont(f);
                    settings->SetPatternMessageFontSize(f.pointSize());
                }
            });

    connect(ui->toolButtonMessagesZoomOutFont, &QToolButton::clicked, this,
            [this]()
            {
                VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
                QFont f = ui->plainTextEditPatternMessages->font();
                if (f.pointSize() > VValentinaSettings::GetDefMinPatternMessageFontSize())
                {
                    f.setPointSize(qMax(f.pointSize() - 1, 1));
                    ui->plainTextEditPatternMessages->setFont(f);
                    settings->SetPatternMessageFontSize(f.pointSize());
                }
            });

    connect(ui->lineEditMessagesFilter, &QLineEdit::textChanged, this,
            [this](const QString &text) { ui->plainTextEditPatternMessages->SetFilter(text); });

    connect(ui->toolButtonClearMessages, &QToolButton::clicked, this, &MainWindow::ClearPatternMessages);

    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();
    ui->toolButtonAutoRefresh->setChecked(settings->GetAutoRefreshPatternMessage());
    connect(ui->toolButtonAutoRefresh, &QToolButton::clicked, this,
            [](bool checked)
            { VAbstractValApplication::VApp()->ValentinaSettings()->SetAutoRefreshPatternMessage(checked); });

    connect(doc, &VPattern::PreParseState, this,
            [this]()
            {
                if (ui->toolButtonAutoRefresh->isChecked())
                {
                    ClearPatternMessages();
                }
            });

    UpdateMessagesIcons();

    if (VApplication::IsGUIMode())
    {
        QTimer::singleShot(1s, this, &MainWindow::AskDefaultSettings);
    }

    ui->actionExportFinalMeasurementsToCSV->setIcon(FromTheme(VThemeIcon::DocumentExport));

    if (VAbstractShortcutManager  const*manager = VAbstractValApplication::VApp()->GetShortcutManager())
    {
        connect(manager, &VAbstractShortcutManager::ShortcutsUpdated, this, &MainWindow::UpdateShortcuts);
        UpdateShortcuts();
    }

    // Start in welcome state: hide all editor chrome
    ShowWelcomeScreen();
}





//---------------------------------------------------------------------------------------------------------------------
void MainWindow::AddPP(const QString &PPName)
{
    VPatternBlockMapper *blocks = doc->PatternBlockMapper();
    if (int const id = blocks->GetId(PPName); !blocks->SetActiveById(id))
    {
        qCDebug(vMainWindow, "Error creating pattern block with the name %s.", qUtf8Printable(PPName));
        return;
    }

    QSignalBlocker blocker(m_comboBoxDraws);
    m_comboBoxDraws->addItem(PPName);

    pattern->ClearCalculationGObjects();
    // Create single point
    emit ui->view->itemClicked(nullptr); // hide options previous tool
    const QString label = doc->GenerateLabel(LabelType::NewPatternPiece);
    const QPointF startPosition = StartPositionNewPP();

    VToolBasePointInitData initData;
    initData.scene = m_sceneDraw;
    initData.doc = doc;
    initData.data = pattern;
    initData.parse = Document::FullParse;
    initData.typeCreation = Source::FromGui;
    initData.x = startPosition.x();
    initData.y = startPosition.y();
    initData.name = label;

    auto *spoint = VToolBasePoint::Create(initData);
    emit ui->view->itemClicked(spoint);

    SetEnableTool(true);
    SetEnableWidgets(true);

    if (m_comboBoxDraws)
    {
        const qint32 index = m_comboBoxDraws->findText(PPName);
        if (index != -1)
        { // -1 for not found
            m_comboBoxDraws->setCurrentIndex(index);
        }
    }
    else
    {
        m_comboBoxDraws->setCurrentIndex(0);
    }
    blocker.unblock();

    // Show best for new PP
    VMainGraphicsView::NewSceneRect(ui->view->scene(), ui->view, spoint);
    // ZoomFitBestCurrent();
    QApplication::postEvent(this, new FitBestCurrentEvent());

    ui->actionNewDraw->setEnabled(true);
    m_statusLabel->setText(QString());
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::StartPositionNewPP() const -> QPointF
{
    const qreal originX = 30.0;
    const qreal originY = 40.0;
    const qreal margin = 40.0;
    if (m_comboBoxDraws->count() > 1)
    {
        const QRectF rect = m_sceneDraw->VisibleItemsBoundingRect();
        if (rect.width() <= rect.height())
        {
            return {rect.width() + margin, originY};
        }

        return {originX, rect.height() + margin};
    }

    return {originX, originY};
}






// Template extracted to mainwindow_tools.cpp


// Template extracted to mainwindow_tools.cpp

// Template extracted to mainwindow_tools.cpp

// Template extracted to mainwindow_tools.cpp

// Template extracted to mainwindow_tools.cpp

// Template extracted to mainwindow_tools.cpp


























































































































//---------------------------------------------------------------------------------------------------------------------
// SetupDrawToolsIcons extracted to mainwindow_icons.cpp



//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ActionDraw show draw scene.
 * @param checked true - button checked.
 */
void MainWindow::ActionDraw(bool checked)
{
    if (checked)
    {
        qCDebug(vMainWindow, "Show draw scene");
        m_dockManager->saveDockStateForMode(VAbstractValApplication::VApp()->GetDrawMode());
        ArrowTool(true);

        ui->actionDraw->setChecked(true);
        ui->actionDetails->setChecked(false);
        ui->actionGraded->setChecked(false);
        ui->actionLayout->setChecked(false);

        if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Graded)
        {
            m_topBar->SetGradedChrome(false);
        }

        if (!switchScene(Draw::Calculation))
        {
            return;
        }

        SetEnableWidgets(true);
        m_dockManager->applyModeDocks(Draw::Calculation);
        // The work toolbar is not managed by the dock manager — make sure leaving Graded (or
        // any state where it was hidden) never leaves Draw without it.
        m_drawToolBar->show();
        m_topBar->SetPatternPieceVisible(true);
    }
    else
    {
        ui->actionDraw->setChecked(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ActionDetails show details scene.
 * @param checked true - button checked.
 */
void MainWindow::ActionDetails(bool checked)
{
    if (checked)
    {
        ArrowTool(true);

        ui->actionDraw->setChecked(false);
        ui->actionDetails->setChecked(true);
        ui->actionGraded->setChecked(false);
        ui->actionLayout->setChecked(false);

        if (not VAbstractValApplication::VApp()->getOpeningPattern() && pattern->DataPieces()->isEmpty())
        {
            QMessageBox::information(this, tr("Detail mode"),
                                     tr("You can't use Detail mode yet. "
                                        "Please, create at least one workpiece."),
                                     QMessageBox::Ok, QMessageBox::Ok);
            ActionDraw(true);
            return;
        }

        m_dockManager->detailsWidget()->UpdateList();

        qCDebug(vMainWindow, "Show details scene");
        m_dockManager->saveDockStateForMode(VAbstractValApplication::VApp()->GetDrawMode());

        if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Graded)
        {
            m_topBar->SetGradedChrome(false);
        }

        if (!switchScene(Draw::Modeling))
        {
            return;
        }

        SetEnableWidgets(true);
        m_dockManager->applyModeDocks(Draw::Modeling);
        // Same guarantee as ActionDraw: the work toolbar must be back when leaving Graded.
        m_drawToolBar->show();

        m_statusLabel->setText(QString());
        WarningNotUniquePieceName(pattern->DataPieces());
        RefreshPieceGeometry();
        m_topBar->SetPatternPieceVisible(false);
    }
    else
    {
        ui->actionDetails->setChecked(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ActionLayout begin creation layout.
 * @param checked true - button checked.
 */
void MainWindow::ActionLayout(bool checked)
{
    if (not checked)
    {
        ui->actionLayout->setChecked(true);
        return;
    }

    ArrowTool(true);

    ui->actionDraw->setChecked(false);
    ui->actionDetails->setChecked(false);
    ui->actionGraded->setChecked(false);
    ui->actionLayout->setChecked(true);

    QVector<DetailForLayout> details;
    if (not VAbstractValApplication::VApp()->getOpeningPattern())
    {
        const QHash<quint32, VPiece> *allDetails = pattern->DataPieces();
        if (allDetails->isEmpty())
        {
            QMessageBox::information(this, tr("Layout mode"),
                                     tr("You can't use Layout mode yet. "
                                        "Please, create at least one workpiece."),
                                     QMessageBox::Ok, QMessageBox::Ok);
            ActionDraw(true);
            return;
        }

        WarningNotUniquePieceName(allDetails);
        details = SortDetailsForLayout(allDetails, pattern);

        if (details.isEmpty())
        {
            QMessageBox::information(this, tr("Layout mode"),
                                     tr("You can't use Layout mode yet. Please, "
                                        "include at least one detail in layout."),
                                     QMessageBox::Ok, QMessageBox::Ok);
            VAbstractValApplication::VApp()->GetDrawMode() == Draw::Calculation ? ActionDraw(true)
                                                                                : ActionDetails(true);
            return;
        }
    }

    qCDebug(vMainWindow, "Show layout scene");
    m_dockManager->saveDockStateForMode(VAbstractValApplication::VApp()->GetDrawMode());

    if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Graded)
    {
        m_topBar->SetGradedChrome(false);
        m_drawToolBar->show();
    }

    if (not switchScene(Draw::Layout, details))
    {
        return;
    }

    SetEnableWidgets(true);
    m_dockManager->applyModeDocks(Draw::Layout);

    ShowPaper(ui->listWidget->currentRow());

    if (m_layoutSettings->LayoutScenes().isEmpty() || m_layoutSettings->IsLayoutStale())
    {
        ui->actionLayoutSettings->activate(QAction::Trigger);
    }

    m_statusLabel->setText(QString());
}

















//---------------------------------------------------------------------------------------------------------------------
bool MainWindow::hasOpenPattern() const
{
    return m_comboBoxDraws->count() > 0;
}







































//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitDocksContain()
{
    // Owns dock content widgets, per-dock structure, title bars and toggle actions.
    m_dockManager->init(pattern, doc);

    // At construction no pattern is loaded yet, so this refresh is a no-op; the holders are
    // (re)built on every CheckLayout once the pieces' geometry is applied.
    RefreshGroupHolders();

    // Keep the group holders in sync with any undo/redo (Link, Unlink, group edits).
    connect(VAbstractApplication::VApp()->getUndoStack(), &QUndoStack::indexChanged, this,
            [this](int) { RefreshGroupHolders(); });

    // The pieces' geometry (positions) is applied asynchronously after a parse (debounce timer,
    // and only in Modeling mode), so holders built earlier use members still at their default
    // position — their frames would be wrong and scatter the group's children. Rebuild them from
    // the final geometry on every CheckLayout (emitted at the end of the geometry application).
    connect(doc, &VAbstractPattern::CheckLayout, this, &MainWindow::RefreshGroupHolders);

    // The group behaves like one piece: its in-layout state (or the "Hide not in layout"
    // checkbox) drives the visibility of the whole group on the canvas — the holder (the
    // group's children) and the member pieces.
    connect(m_dockManager->detailsWidget(), &VWidgetDetails::GroupVisibilityChanged, this,
            [this](quint32 groupId, bool visible)
    {
        if (auto *holder = m_groupHolders.value(groupId, nullptr); holder != nullptr)
        {
            holder->setVisible(visible);
        }
        if (auto *groups = pattern != nullptr ? pattern->DataPieceGroups() : nullptr;
            groups != nullptr && groups->contains(groupId))
        {
            for (quint32 pid : groups->value(groupId).pieceIds)
            {
                try
                {
                    if (auto *memberTool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pid)))
                    {
                        memberTool->setVisible(visible);
                    }
                }
                catch (const VExceptionBadId &)
                {
                    // Member tool not registered (mid-parse) — skip it.
                }
            }
        }
    });

    // During the Link tool the group holders stay VISIBLE (their children must not vanish) but
    // become click-transparent, so the clicks on a member reach the piece beneath (which the
    // workflow rejects, being already grouped).
    connect(m_dockManager->detailsWidget(), &VWidgetDetails::GroupToolStateChanged, this,
            [this](VWidgetDetails::GroupToolState state)
    {
        SetAllGroupHoldersInteractive(state == VWidgetDetails::GroupToolState::Idle);
    });

    // ── Details options browser: piece selection + passmark routing ──
    connect(doc, &VPattern::ShowDetail, this, [this](quint32 id)
    {
        // A piece (or empty canvas) selection deselects any group on the canvas.
        UpdateGroupHolderSelection(NULL_ID);
        if (id == NULL_ID)
        {
            m_dockManager->detailsOptions()->ShowEmpty();
            return;
        }
        if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(id)))
        {
            // A member piece opens its own browser; the group's browser is reached via the
            // group node in the tree or the canvas holder (Clicked → ShowGroupOptions).
            m_dockManager->detailsOptions()->ShowPieceOptions(tool);
        }
    });

    // Clicking a child overlay (grainline or info block) selects the parent piece
    // in the Details dock and shows the matching tab; the child itself stays
    // active (edit handles) on the canvas.
    connect(m_sceneDetails, &VMainGraphicsScene::ItemByMousePress, this,
            [this](QGraphicsItem *pItem)
    {
        VToolSeamAllowance *piece = qgraphicsitem_cast<VToolSeamAllowance *>(pItem);
        if (piece == nullptr && pItem != nullptr)
        {
            piece = qgraphicsitem_cast<VToolSeamAllowance *>(pItem->parentItem());
        }
        if (piece == nullptr)
        {
            return;
        }

        if (qgraphicsitem_cast<VGrainlineItem *>(pItem) != nullptr)
        {
            doc->SelectedDetail(piece->getId());
            m_dockManager->detailsOptions()->ShowGrainlineTab();
        }
        else if (qgraphicsitem_cast<OInfoBlockItem *>(pItem) != nullptr)
        {
            doc->SelectedDetail(piece->getId());
            m_dockManager->detailsOptions()->ShowBlocksTab();
        }
        else if (qgraphicsitem_cast<OAnnotationItem *>(pItem) != nullptr)
        {
            doc->SelectedDetail(piece->getId());
            m_dockManager->detailsOptions()->ShowAnnotationsTab();
        }
    });

    connect(m_dockManager->detailsOptions(), &ODetailsPropertyBrowser::ShowStatusTip, this,
            [this](const QString &text) { m_statusLabel->setText(text); });

    connect(m_dockManager->detailsOptions(), &ODetailsPropertyBrowser::HighlightNode, this,
            [this](quint32 nodeId, bool highlight)
    {
        if (pattern == nullptr)
        {
            return;
        }
        const auto *pieces = pattern->DataPieces();
        if (pieces == nullptr)
        {
            return;
        }
        for (auto it = pieces->constBegin(); it != pieces->constEnd(); ++it)
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(it.key()));
            if (tool == nullptr)
            {
                continue;
            }
            for (auto *child : tool->childItems())
            {
                auto *nodePoint = dynamic_cast<VNodePoint *>(child);
                if (nodePoint != nullptr && nodePoint->getId() == nodeId)
                {
                    nodePoint->SetActivePoint(highlight);
                    return;
                }
            }
        }
    });

    connect(m_dockManager->detailsOptions(), &ODetailsPropertyBrowser::HighlightTemporaryNode, this,
            [this](quint32 nodeId, bool selected)
    {
        if (pattern == nullptr)
        {
            return;
        }
        const auto *pieces = pattern->DataPieces();
        if (pieces == nullptr)
        {
            return;
        }
        for (auto it = pieces->constBegin(); it != pieces->constEnd(); ++it)
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(it.key()));
            if (tool == nullptr)
            {
                continue;
            }
            for (auto *child : tool->childItems())
            {
                auto *nodePoint = dynamic_cast<VNodePoint *>(child);
                if (nodePoint != nullptr && nodePoint->getId() == nodeId)
                {
                    nodePoint->SetTemporarySelected(selected);
                    return;
                }
            }
        }
    });

    connect(m_dockManager->detailsOptions(), &ODetailsPropertyBrowser::ClearHighlight, this, [this]()
    {
        if (pattern == nullptr)
        {
            return;
        }
        const auto *pieces = pattern->DataPieces();
        if (pieces == nullptr)
        {
            return;
        }
        for (auto it = pieces->constBegin(); it != pieces->constEnd(); ++it)
        {
            auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(it.key()));
            if (tool == nullptr)
            {
                continue;
            }
            std::function<void(QGraphicsItem *)> restorePens = [&](QGraphicsItem *item)
            {
                if (auto *nodePoint = dynamic_cast<VNodePoint *>(item))
                {
                    nodePoint->SetActivePoint(false);
                    nodePoint->SetTemporarySelected(false);
                    nodePoint->RefreshScale();
                }
                for (auto *child : item->childItems())
                {
                    restorePens(child);
                }
            };
            restorePens(tool);
        }
    });

    connect(ui->view, &VMainGraphicsView::itemClicked, this, &MainWindow::HandleItemClicked);
    connect(ui->view, &VMainGraphicsView::itemHovered, this, &MainWindow::HandleItemHovered);
    connect(ui->view, &VMainGraphicsView::itemDoubleClicked, this, &MainWindow::ItemDoubleClicked);
    connect(m_breadcrumb, &BreadcrumbBar::canvasClicked, this, [this]()
            {
                doc->ClearIsolation();
                m_breadcrumb->hide();
                m_dockManager->toolOptions()->itemClicked(nullptr);
            });
    connect(m_breadcrumb, &BreadcrumbBar::groupClicked, this, [this]()
            {
                const quint32 attachmentId = doc->GetIsolatedAttachment();
                doc->SetIsolatedAttachment(attachmentId);
                const QString groupName = doc->GetAttachmentName(attachmentId);
                m_dockManager->toolOptions()->ShowGroupOptions(groupName, attachmentId);
                m_breadcrumb->Update(groupName, QString());
            });
    connect(m_dockManager->toolOptions(), &VToolOptionsPropertyBrowser::DetachAllRequested, this, [this](quint32 attachmentId)
            {
                auto *delAttachment = new DelAttachment(doc, attachmentId);
                VAbstractApplication::VApp()->getUndoStack()->push(delAttachment);
                doc->ClearIsolation();
                m_breadcrumb->hide();
                m_dockManager->toolOptions()->itemClicked(nullptr);
            });
    connect(m_dockManager->toolOptions(), &VToolOptionsPropertyBrowser::RenameRequested, this,
            [this](quint32 attachmentId, const QString &name)
            {
                if (doc->GetAttachmentName(attachmentId) == name)
                {
                    return;
                }
                auto *renameAttachment = new RenameAttachment(doc, attachmentId, name);
                VAbstractApplication::VApp()->getUndoStack()->push(renameAttachment);

                // Reflect the new name in the breadcrumb (keep the drilled-down object, if any).
                QString objectName;
                const quint32 objectId = doc->GetIsolatedObject();
                if (objectId != NULL_ID && pattern != nullptr)
                {
                    if (const auto gobject = pattern->GetGObject(objectId); not gobject.isNull())
                    {
                        objectName = gobject->ObjectName();
                    }
                }
                m_breadcrumb->Update(name, objectName);
            });
    connect(doc, &VPattern::FullUpdateFromFile, m_dockManager->toolOptions(), &VToolOptionsPropertyBrowser::UpdateOptions,
            Qt::QueuedConnection);

    connect(doc, &VAbstractPattern::UpdateGroups, this, &MainWindow::UpdateVisibilityGroups);

    connect(doc, &VPattern::FullUpdateFromFile, m_dockManager->detailsWidget(), &VWidgetDetails::UpdateList);
    connect(doc, &VPattern::UpdateInLayoutList, m_dockManager->detailsWidget(), &VWidgetDetails::UpdateList);
    connect(doc, &VPattern::ShowDetail, m_dockManager->detailsWidget(), &VWidgetDetails::SelectDetail);
    connect(m_dockManager->detailsWidget(), &VWidgetDetails::Highlight, m_sceneDetails, &VMainGraphicsScene::HighlightItem);
    connect(m_dockManager->detailsWidget(), &VWidgetDetails::ShowStatusTip, this,
            [this](const QString &text) { m_statusLabel->setText(text); });
    connect(m_dockManager->detailsWidget(), &VWidgetDetails::GroupSelected, this,
            [this](quint32 groupId)
    {
        UpdateGroupHolderSelection(groupId);
        m_dockManager->detailsOptions()->ShowGroupOptions(groupId, pattern, doc);
        m_dockManager->detailsWidget()->SelectDetail(groupId);
    });
    // A member piece in the tree opens its OWN property browser (Val Fork OnItemClicked);
    // the group's browser is only opened via the group node or the canvas holder.
    connect(m_dockManager->detailsWidget(), &VWidgetDetails::PieceSelected, this,
            [this](quint32 id)
    {
        if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(id)))
        {
            m_dockManager->detailsOptions()->ShowPieceOptions(tool);
        }
    });

    connect(m_dockManager->backgroundImagesWidget(), &VWidgetBackgroundImages::DeleteImage, this, &MainWindow::RemoveBackgroundImage);

    connect(doc, &VPattern::FullUpdateFromFile, m_dockManager->dependenciesWidget(), &VWidgetDependencies::UpdateDependencies,
            Qt::QueuedConnection);
    connect(doc, &VPattern::PatternDependencyGraphCompleted, m_dockManager->dependenciesWidget(),
            &VWidgetDependencies::UpdateDependencies, Qt::QueuedConnection);
    // Extract the tool ID synchronously while the item is alive, then deliver it
    // via a queued call so ShowDependency does not block the UI thread.
    // Passing a raw QGraphicsItem* through a queued connection is unsafe because
    // the item can be destroyed before the slot fires.
    connect(ui->view, &VMainGraphicsView::itemClicked, this,
            [this](QGraphicsItem *item) -> void
            {
                const vidtype id = VWidgetDependencies::ItemToId(item);
                QMetaObject::invokeMethod(m_dockManager->dependenciesWidget(), [widget = m_dockManager->dependenciesWidget(), id]()-> void
                                          { widget->ShowDependency(id); }, Qt::QueuedConnection);
            });
    connect(m_dockManager->dependenciesWidget(), &VWidgetDependencies::ShowProperties, m_dockManager->toolOptions(),
            &VToolOptionsPropertyBrowser::itemClicked, Qt::QueuedConnection);
    connect(m_dockManager->dependenciesWidget(), &VWidgetDependencies::ShowTool, ui->view, &VMainGraphicsView::EnsureToolVisible,
            Qt::QueuedConnection);

    // Size Ranges dock → MainWindow (Add / Edit / Rename / Delete wired; the dock
    // refreshes its list on Graded entry and after each of those).
    connect(m_dockManager->sizeRangesWidget(), &OWidgetSizeRanges::addRequested, this,
            &MainWindow::AddGradedRange);
    connect(m_dockManager->sizeRangesWidget(), &OWidgetSizeRanges::editRequested, this,
            &MainWindow::EditGradedRange);
    // Queued: the rename is emitted from inside the item editor's commitData — the
    // pattern write + list refresh must run only once the editor has closed.
    connect(m_dockManager->sizeRangesWidget(), &OWidgetSizeRanges::renameRequested, this,
            &MainWindow::RenameGradedRange, Qt::QueuedConnection);
    connect(m_dockManager->sizeRangesWidget(), &OWidgetSizeRanges::duplicateRequested, this,
            &MainWindow::DuplicateGradedRange);
    connect(m_dockManager->sizeRangesWidget(), &OWidgetSizeRanges::deleteRequested, this,
            &MainWindow::DeleteGradedRange);
    connect(m_dockManager->sizeRangesWidget(), &OWidgetSizeRanges::rangeActivated, this,
            &MainWindow::ActivateGradedRange);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGroupHolders()
{
    const auto *groups = pattern != nullptr ? pattern->DataPieceGroups() : nullptr;
    if (groups == nullptr)
    {
        return;
    }

    // Remove holders for deleted groups.
    for (auto it = m_groupHolders.cbegin(); it != m_groupHolders.cend();)
    {
        if (!groups->contains(it.key()))
        {
            m_sceneDetails->removeItem(it.value());
            delete it.value();
            it = m_groupHolders.erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto git = groups->cbegin(); git != groups->cend(); ++git)
    {
        OPieceGroupHolderItem *holder = m_groupHolders.value(git.key(), nullptr);
        if (holder == nullptr)
        {
            holder = new OPieceGroupHolderItem(git.key());
            m_sceneDetails->addItem(holder);
            // Activate a clicked group child (outline + resize handles), like a piece.
            connect(m_sceneDetails, &VMainGraphicsScene::ItemByMousePress, holder,
                    &OPieceGroupHolderItem::ResetChildren);
            connect(holder, &OPieceGroupHolderItem::Clicked, this,
                    [this](quint32 groupId)
            {
                // A selected group shows its selection block on the canvas, like a piece.
                UpdateGroupHolderSelection(groupId);
                m_dockManager->detailsOptions()->ShowGroupOptions(groupId, pattern, doc);
                // Like a piece (SelectedDetail → SelectDetail), a canvas click on the group
                // must activate the group node in the pieces tree — it never was, so the
                // group was never visually active (unlike a selected piece).
                m_dockManager->detailsWidget()->SelectDetail(groupId);
            });
            m_groupHolders.insert(git.key(), holder);
        }
        holder->Refresh(git.value(), doc, pattern);
    }

    // « Un groupe = 1 pièce » : creating/removing a group changes the effective piece
    // count. Re-render every piece's information blocks (their "/ N" total) — the
    // resolver already masks a member's own blocks. Only re-renders when the count
    // actually changed, and never touches the geometry.
    if (pattern != nullptr)
    {
        const int effectiveCount = pattern->GetEffectivePieceCount();
        if (effectiveCount != m_lastEffectivePieceCount)
        {
            m_lastEffectivePieceCount = effectiveCount;
            const auto *pieces = pattern->DataPieces();
            if (pieces != nullptr)
            {
                for (auto it = pieces->cbegin(); it != pieces->cend(); ++it)
                {
                    try
                    {
                        // A piece may be in the container but its tool not yet registered (a
                        // creation/reparse is mid-flight when this runs via undo indexChanged).
                        if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(it.key())))
                        {
                            tool->UpdateInfoBlocks();
                        }
                    }
                    catch (const VExceptionBadId &)
                    {
                        // Tool not created yet — skip it.
                    }
                }
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SetAllGroupHoldersInteractive(bool interactive)
{
    for (OPieceGroupHolderItem *holder : m_groupHolders)
    {
        if (holder != nullptr)
        {
            holder->SetInteractive(interactive);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateGroupHolderSelection(quint32 groupId)
{
    for (auto it = m_groupHolders.cbegin(); it != m_groupHolders.cend(); ++it)
    {
        if (it.value() != nullptr)
        {
            it.value()->SetSelected(it.key() == groupId);
        }
    }
}





//---------------------------------------------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    CancelTool();

    delete doc;
    delete ui;
}


























































