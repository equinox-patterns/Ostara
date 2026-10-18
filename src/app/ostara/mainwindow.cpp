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
#include "widgets/owidgetcoverpage.h"
#include "widgets/owidgetoutputs.h"
#include "widgets/owidgetsizeranges.h"
#include "widgets/owidgetsizestyles.h"
#include "widgets/TopBar.h"

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
#include "../vtools/tools/othumbnailitem.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/undogroup.h"
#include "../vtools/undocommands/unlinkpiecegroup.h"
#include "../vwidgets/vgrainlineitem.h"
#include "../vwidgets/vabstractsimple.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vscenepoint.h"
#include "../vwidgets/scalesceneitems.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vtools/tools/nodeDetails/vnodepoint.h"
#include "../vtools/tools/nodeDetails/onodeplacelabel.h"
#include "../vtools/tools/nodeDetails/vtoolpiecepath.h"
#include "core/vapplication.h"
#include "core/odetailspropertybrowser.h"
#include "core/opiecegroupholderitem.h"
#include "core/vtooloptionspropertybrowser.h"
#include "WelcomeScreen.h"
#include <QStackedWidget>
#include "dialogs/vwidgetbackgroundimages.h"
#include "dialogs/vwidgetdetails.h"
#include "dialogs/vwidgetdependencies.h"
#include "dialogs/vwidgetgroups.h"
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

#if defined(Q_OS_MAC)
//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Builds (or rebuilds) the macOS Dock menu.
 *
 * ⚠️ **Les quatre entrées sont écrites ici, et nulle part ailleurs** : rien de ce menu ne vient de
 * `mainwindow.ui`, donc `ui->retranslateUi` ne peut pas les atteindre. La fonction est rappelée par
 * `RetranslateActions()` — `clear()` supprime les actions précédentes, la reconstruction les recrée
 * et les reconnecte, il n'y a donc pas de doublon.
 */
void MainWindow::BuildDockMenu()
{
    if (m_dockMenu == nullptr)
    {
        m_dockMenu = new QMenu(this);
    }
    m_dockMenu->clear();

    QAction *actionNewPattern = m_dockMenu->addAction(tr("New pattern"));
    actionNewPattern->setMenuRole(QAction::NoRole);
    connect(actionNewPattern, &QAction::triggered, this, &MainWindow::on_actionNew_triggered);

    QAction *actionOpenPattern = m_dockMenu->addAction(tr("Open pattern"));
    actionOpenPattern->setMenuRole(QAction::NoRole);
    connect(actionOpenPattern, &QAction::triggered, this, &MainWindow::on_actionOpen_triggered);

    QAction *actionNewMeasurement = m_dockMenu->addAction(tr("Create/Edit measurements"));
    actionNewMeasurement->setMenuRole(QAction::NoRole);
    connect(actionNewMeasurement, &QAction::triggered, this, &MainWindow::OnNewMeasurements);

    QAction *actionPreferences = m_dockMenu->addAction(tr("Preferences"));
    actionPreferences->setMenuRole(QAction::NoRole);
    connect(actionPreferences, &QAction::triggered, this, &MainWindow::Preferences);

    m_dockMenu->setAsDockMenu();
}
#endif // defined(Q_OS_MAC)

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

    // A piece's geometry is memoized at parse time, so freezing the miniatures on the reference size
    // means standing the document on that size and parsing twice. That is far too expensive to run
    // on every edit: the timer collapses a burst into one round trip.
    m_thumbnailSnapshots = new QTimer(this);
    m_thumbnailSnapshots->setSingleShot(true);
    m_thumbnailSnapshots->setInterval(400);
    connect(m_thumbnailSnapshots, &QTimer::timeout, this,
            [this]() { RefreshThumbnailSnapshots(); });

    // Any committed edit (undo/redo included) can change what a miniature has to show.
    connect(VAbstractApplication::VApp()->getUndoStack(), &QUndoStack::indexChanged, this,
            [this](int)
            {
                ScheduleThumbnailSnapshots();
            });

    doc = new VPattern(pattern, m_sceneDraw, m_sceneDetails);
    // Clear() empties the undo stack, so it must never run inside an undo(): the command would be destroyed while
    // still sitting on the stack. Queued, it lands once the emitting command has returned. Same reason as the
    // sibling connection below.
    connect(doc, &VPattern::ClearMainWindow, this, &MainWindow::Clear, Qt::QueuedConnection);
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
        {QStringLiteral("flipping_axis"),                ui->actionFlippingByAxisTool},
        {QStringLiteral("flipping_line"),                ui->actionFlippingByLineTool},
        {QStringLiteral("rotation"),                     ui->actionRotationTool},
        {QStringLiteral("close_dart"),                   ui->actionCloseDartTool},
        {QStringLiteral("dart_cap"),                     ui->actionDartCapTool},
        {QStringLiteral("rectangle"),                    ui->actionRectangleTool},
        {QStringLiteral("move"),                         ui->actionMoveTool},
        {QStringLiteral("transfert"),                    ui->actionTransfertTool},
        {QStringLiteral("spiral"),                       ui->actionSpiralTool},
        {QStringLiteral("spread"),                       ui->actionSpreadTool},
        {QStringLiteral("move_batch"),                   ui->actionMoveBatchTool},
        {QStringLiteral("pleat"),                        ui->actionPleatTool},
        {QStringLiteral("pleat_mid"),                    ui->actionPleatMidTool},
        {QStringLiteral("flounce_spiral"),               ui->actionFlounceSpiralTool},
        {QStringLiteral("true_darts"),                   ui->actionTrueDartsTool},
        {QStringLiteral("new_detail"),                   ui->actionNewDetailTool},
        {QStringLiteral("path"),                         ui->actionInternalPathTool},
        {QStringLiteral("insert_node"),                  ui->actionInsertNodeTool},
        {QStringLiteral("place_label"),                  ui->actionPlaceLabelTool},
        {QStringLiteral("union"),                        ui->actionUnionDetailsTool},
        {QStringLiteral("duplicate_detail"),             ui->actionDuplicateDetailTool},
        {QStringLiteral("detail_export"),                ui->actionDetailExportAs},
        {QStringLiteral("export_templates"),             ui->actionExportTemplates},
        {QStringLiteral("export_image"),                 ui->actionExportImage},
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

    // Wrap the editor in a vertical layout so the active-layer rule can sit above the canvas.
    auto *editorContainer = new QWidget(this);
    auto *editorLayout = new QVBoxLayout(editorContainer);
    editorLayout->setContentsMargins(0, 0, 0, 0);
    editorLayout->setSpacing(0);

    // The layer the canvas draws into announces itself: a one-pixel rule along the top edge of the canvas, in
    // that layer's own colour. It takes its place in the layout — a scene item would scroll with the drawing.
    m_activeLayerRule = new QFrame(this);
    m_activeLayerRule->setObjectName(QStringLiteral("activeLayerRule"));
    m_activeLayerRule->setFrameShape(QFrame::NoFrame);
    m_activeLayerRule->setFixedHeight(1);
    m_activeLayerRule->setAutoFillBackground(true);
    m_activeLayerRule->hide();
    editorLayout->addWidget(m_activeLayerRule);

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


#if defined(Q_OS_MAC)
    setUnifiedTitleAndToolBarOnMac(true);

    // Mac OS Dock Menu — rebuilt as a whole on a language change (see RetranslateActions()).
    BuildDockMenu();
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
void MainWindow::AddPP(const QString &PPName, const QString &layerName, const QString &layerColor,
                       const QString &layerPenStyle)
{
    VPatternBlockMapper *blocks = doc->PatternBlockMapper();
    if (int const id = blocks->GetId(PPName); !blocks->SetActiveById(id))
    {
        qCDebug(vMainWindow, "Error creating pattern block with the name %s.", qUtf8Printable(PPName));
        return;
    }

    // The piece and its first layer are one gesture: the macro holds the draw block (pushed by
    // VToolBasePoint::Create) and the layer written right after it, so one Ctrl+Z takes both away.
    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoStack->beginMacro(tr("add a pattern piece"));

    QSignalBlocker blocker(m_comboBoxDraws);
    m_comboBoxDraws->addItem(PPName);

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

    // The piece exists in the document from here on (the undo command ran synchronously), so its first
    // layer can be written into it — and the base point, the only object a fresh piece holds, goes in.
    CreateFirstLayer(layerName, layerColor, layerPenStyle, spoint->getId());

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
    UpdatePieceSaveAction();
    m_statusLabel->setText(QString());

    undoStack->endMacro();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::CreateFirstLayer(const QString &layerName, const QString &layerColor,
                                  const QString &layerPenStyle, quint32 basePointId)
{
    const quint32 newId = pattern->getNextId();
    const QDomElement group = doc->CreateGroup(newId, layerName, {}, null_id, layerColor, layerPenStyle);
    if (group.isNull())
    {
        return;
    }

    // Read before the group exists: the piece being born is already the active block, and it holds no layer,
    // so this answers NULL_ID — which is exactly what ChangeActiveGroup will restore on undo.
    const quint32 previous = doc->GetActiveGroup();

    QUndoStack *undoStack = VAbstractApplication::VApp()->getUndoStack();

    // The layer into the tree first, then the base point into the layer, then the layer as the active one:
    // each of the three is a command, and the macro opened by AddPP makes them one step with the piece.
    undoStack->push(new AddGroup(group, doc));

    // A simple point is its own object — (T, T) — which is how LayerableObjectsOfTool reads it back too.
    if (const QDomElement item = doc->AddItemToGroup(basePointId, basePointId, newId); not item.isNull())
    {
        undoStack->push(new ::AddItemToGroup(item, doc, newId));
    }

    undoStack->push(new ChangeActiveGroup(doc, newId, previous));
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::StartPositionNewPP() const -> QPointF
{
    // The first piece of a pattern starts at the origin; every later one stands clear of what is drawn. Both
    // numbers are the canvas's own (pixels), and PositionBeyondDrawn answers the same origin on an empty canvas.
    if (m_comboBoxDraws->count() > 1)
    {
        return PositionBeyondDrawn();
    }

    return {30.0, 40.0};
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Where a construction that brings its own geometry starts: beyond everything the draw scene shows.
 *
 * A piece drawn by hand starts at a bare point and the user draws around it; a piece coming from the library
 * brings a whole construction with it, and dropped where the block it was saved from still stands, the two hide
 * one behind the other. So both start **beyond what is drawn**, and in the same place — one rule, not two.
 *
 * The numbers are the canvas's own, in pixels; a caller writing them into the document converts them
 * (VAbstractValApplication::fromPixel), the way every tool position is written.
 */
auto MainWindow::PositionBeyondDrawn() const -> QPointF
{
    const qreal originX = 30.0;
    const qreal originY = 40.0;
    const qreal margin = 40.0;

    const QRectF drawn = m_sceneDraw->VisibleItemsBoundingRect();
    if (drawn.isEmpty())
    {
        return {originX, originY}; // Nothing to stand clear of yet.
    }

    // To the right when what is drawn is taller than wide, below otherwise: a construction takes roughly its own
    // size, and the shorter side is the one it leaves alone.
    if (drawn.width() <= drawn.height())
    {
        return {drawn.right() + margin, drawn.top()};
    }

    return {drawn.left(), drawn.bottom() + margin};
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
        ui->actionExport->setChecked(false);

        if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Graded)
        {
            m_topBar->SetGradedChrome(false);
        }

        switchScene(Draw::Calculation);

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
        ui->actionExport->setChecked(false);

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

        switchScene(Draw::Modeling);

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
bool MainWindow::hasOpenPattern() const
{
    return m_comboBoxDraws->count() > 0;
}







































//---------------------------------------------------------------------------------------------------------------------
// Any node-picking workflow waiting for canvas clicks. Kept in one place: both the click routing and
// the node selection guard must agree on the list.
bool MainWindow::IsNodePickingActive() const
{
    const auto *options = m_dockManager->detailsOptions();
    if (options != nullptr && (options->IsSelectingPassmarkPoints() || options->IsSelectingFoldLinePoints() ||
                               options->IsSelectingGroupFoldLinePoints() || options->IsSelectingSAPoints() ||
                               options->IsSelectingAlignPoint()))
    {
        return true;
    }

    // The axis of a curve handle is picked on the Draw canvas, not on the Details one, but it is the same
    // gesture for the user: two clicks, and the scene must not read them as a selection.
    const auto *toolOptions = m_dockManager->toolOptions();
    if (toolOptions != nullptr && toolOptions->IsPickingAxisPoints())
    {
        return true;
    }

    const auto *details = m_dockManager->detailsWidget();
    return details != nullptr && (details->IsSelectingLinkPieces() || details->IsSelectingLinkNode());
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

        // A new selection clears any selected place-label handle. Qt single-selection would clear
        // it when another selectable item is picked anyway; this also covers empty-canvas clicks and
        // clicks on non-selectable overlays (grainline / info block / annotation), which leave the
        // piece current and so would otherwise leave the handle highlighted.
        if (pattern != nullptr)
        {
            const auto *pieces = pattern->DataPieces();
            if (pieces != nullptr)
            {
                for (auto it = pieces->constBegin(); it != pieces->constEnd(); ++it)
                {
                    if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(it.key())))
                    {
                        tool->ClearPlaceLabelSelection();
                    }
                }
            }
        }

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
        // A place-label handle (or its name text) selects the owning piece, mirroring the
        // other on-piece overlays (grainline / info block / annotation). The handle is only a
        // canvas point — its options come from the owning piece in the Details dock.
        QGraphicsItem *probe = pItem;
        while (probe != nullptr && qgraphicsitem_cast<ONodePlaceLabel *>(probe) == nullptr)
        {
            probe = probe->parentItem();
        }
        if (probe != nullptr)
        {
            // While the Remove-marks mode is collecting marks, a click on a label belongs to that
            // selection: it must not steal the dock page (the label is still toggled, see
            // MainWindow::HandleItemClicked).
            if (m_dockManager->detailsOptions() != nullptr
                && m_dockManager->detailsOptions()->IsSelectingPassmarkPoints())
            {
                return;
            }

            QGraphicsItem *owner = probe->parentItem();
            while (owner != nullptr && qgraphicsitem_cast<VToolSeamAllowance *>(owner) == nullptr)
            {
                owner = owner->parentItem();
            }
            if (auto *pieceOwner = qgraphicsitem_cast<VToolSeamAllowance *>(owner); pieceOwner != nullptr)
            {
                // Select the piece (dock context), then open the clicked label's own PB page.
                auto *labelNode = qgraphicsitem_cast<ONodePlaceLabel *>(probe);
                doc->SelectedDetail(pieceOwner->getId());
                m_dockManager->detailsOptions()->ShowPlaceLabelOptions(
                    pieceOwner, labelNode != nullptr ? labelNode->getId() : NULL_ID);
            }
            return;
        }

        // A piece contour node opens its own property browser, like a place label handle.
        QGraphicsItem *nodeProbe = pItem;
        VNodePoint *nodeItem = nullptr;
        while (nodeProbe != nullptr && (nodeItem = qgraphicsitem_cast<VNodePoint *>(nodeProbe)) == nullptr)
        {
            nodeProbe = nodeProbe->parentItem();
        }
        if (nodeItem != nullptr)
        {
            QGraphicsItem *owner = nodeItem->parentItem();
            while (owner != nullptr && qgraphicsitem_cast<VToolSeamAllowance *>(owner) == nullptr)
            {
                owner = owner->parentItem();
            }
            if (auto *pieceOwner = qgraphicsitem_cast<VToolSeamAllowance *>(owner); pieceOwner != nullptr)
            {
                // Keep node clicks for the node-picking workflows (link, foldline, passmark, custom SA).
                // The answer is the one frozen when the click was routed, not the live one: a workflow
                // that ends on this very click has already reset its state by the time the scene reads it.
                const bool picking = m_pickingClick || IsNodePickingActive();
                m_pickingClick = false;
                if (!picking)
                {
                    // Select the piece (dock context), then open the clicked node's own PB page.
                    // Non-contour nodes fall back to the piece page inside ShowNodeOptions.
                    doc->SelectedDetail(pieceOwner->getId());
                    m_dockManager->detailsOptions()->ShowNodeOptions(pieceOwner, nodeItem->getId());
                }
            }
            return;
        }

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
        else if (qgraphicsitem_cast<OThumbnailItem *>(pItem) != nullptr)
        {
            // The miniature is one of the piece's children, and clicking it opens the tab that
            // configures it — the same door the grainline, the blocks and the annotations use.
            doc->SelectedDetail(piece->getId());
            m_dockManager->detailsOptions()->ShowThumbnailTab();
        }
    });

    connect(m_dockManager->detailsOptions(), &ODetailsPropertyBrowser::ShowStatusTip, this,
            [this](const QString &text) { m_statusLabel->setText(text); });

    // The Options dock's own gesture, the axis of a curve handle: same two signals, same status label, but the
    // points it highlights live on the Draw canvas and are found by identifier.
    connect(m_dockManager->toolOptions(), &VToolOptionsPropertyBrowser::ShowStatusTip, this,
            [this](const QString &text) { m_statusLabel->setText(text); });

    connect(m_dockManager->toolOptions(), &VToolOptionsPropertyBrowser::HighlightPoint, this,
            [](quint32 pointId, bool highlight)
    {
        QGraphicsScene *scene = VAbstractValApplication::VApp()->getCurrentScene();
        if (scene == nullptr)
        {
            return;
        }

        const QList<QGraphicsItem *> items = scene->items();
        for (QGraphicsItem *item : items)
        {
            // The identifier is read exactly the way the click that routed here reads it (ObjectIdOfItem walks
            // up from the item to the first object that names itself). A Draw-mode point is a TOOL —
            // VToolSinglePoint derives from VAbstractPoint and VScenePoint — and not a VAbstractSimple: testing
            // for the latter matched nothing, so no point was ever marked and neither of the two ever turned.
            if (VAbstractTool::ObjectIdOfItem(item) != pointId)
            {
                continue;
            }

            if (auto *point = dynamic_cast<VScenePoint *>(item))
            {
                // Both marks, and both the same orange. SetActivePoint is the state the pleat's picking wears —
                // what the Options dock drives, and what the rest of the application knows. SetLayerSelected is
                // the scene's own "chosen by a picking workflow" state; nothing but the Layers dock writes it, so
                // the mark holds for as long as the window is open, and either state paints the same orange.
                point->SetActivePoint(highlight);
                point->SetLayerSelected(highlight);

                // The label takes the orange as well: a point that belongs to the curve being edited carries its
                // own transient label colours (magenta among them), and orange has to win there too. On release
                // the three roles are given back the very values VScenePoint's constructor gives them, so nothing
                // has to be remembered.
                point->SetLabelTextColorRole(highlight ? VColorRole::ActivePointColor
                                                       : VColorRole::DraftLabelColor);
                point->SetLabelTextHoverColorRole(highlight ? VColorRole::ActivePointColor
                                                            : VColorRole::DraftLabelHoverColor);
                point->SetLabelLineColorRole(highlight ? VColorRole::ActivePointColor
                                                       : VColorRole::DraftLabelLineColor);

                // And the disc the user sees on a point of the active curve is NOT the point: the curve's
                // visualization lays its own VScaledEllipse on every point of the curve, in the magenta of a
                // support colour (VisToolSplinePath::ApplyActivePoint, VisToolSpline for a simple curve), and it
                // is painted over the real point — so the orange laid on the point stayed hidden underneath.
                //
                // Those overlays step aside for as long as the axis is being taken, and come back at the end.
                // They are found by their type alone, NOT by their position: matching positions was a guess, and
                // the guess was wrong. Which overlay was hidden is remembered ON the item itself, so nothing has
                // to be kept between two calls and no pointer is ever held on to.
                constexpr int kHiddenByAxisPicking = 0;
                for (QGraphicsItem *overlay : scene->items())
                {
                    auto *disc = dynamic_cast<VScaledEllipse *>(overlay);
                    if (disc == nullptr)
                    {
                        continue;
                    }

                    if (highlight)
                    {
                        if (disc->isVisible())
                        {
                            disc->setData(kHiddenByAxisPicking, true);
                            disc->setVisible(false);
                        }
                    }
                    else if (disc->data(kHiddenByAxisPicking).toBool())
                    {
                        disc->setData(kHiddenByAxisPicking, QVariant());
                        disc->setVisible(true);
                    }
                }
            }
        }
    });

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
                // Internal marks highlight the same way: they are marks of the piece too.
                auto *placeLabel = qgraphicsitem_cast<ONodePlaceLabel *>(child);
                if (placeLabel != nullptr && placeLabel->getId() == nodeId)
                {
                    placeLabel->SetActivePoint(highlight);
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
                auto *placeLabel = qgraphicsitem_cast<ONodePlaceLabel *>(child);
                if (placeLabel != nullptr && placeLabel->getId() == nodeId)
                {
                    placeLabel->SetTemporarySelected(selected);
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
                else if (auto *placeLabel = qgraphicsitem_cast<ONodePlaceLabel *>(item))
                {
                    placeLabel->SetActivePoint(false);
                    placeLabel->SetTemporarySelected(false);
                    placeLabel->RefreshScale();
                }
                else if (auto *pathTool = qgraphicsitem_cast<VToolPiecePath *>(item))
                {
                    pathTool->SetHighlight(false);
                }
                for (auto *child : item->childItems())
                {
                    restorePens(child);
                }
            };
            restorePens(tool);
        }
    });

    connect(m_dockManager->detailsOptions(), &ODetailsPropertyBrowser::InternalPathHighlight, this,
            [this](quint32 pieceId, quint32 pathId, bool highlight)
    {
        if (pattern == nullptr)
        {
            return;
        }
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
        if (tool != nullptr)
        {
            tool->SetInternalPathHighlight(pathId, highlight);
        }
    });

    connect(ui->view, &VMainGraphicsView::itemClicked, this, &MainWindow::HandleItemClicked);
    connect(ui->view, &VMainGraphicsView::itemHovered, this, &MainWindow::HandleItemHovered);
    connect(doc, &VPattern::FullUpdateFromFile, m_dockManager->toolOptions(), &VToolOptionsPropertyBrowser::UpdateOptions,
            Qt::QueuedConnection);

    connect(doc, &VAbstractPattern::UpdateGroups, this, &MainWindow::UpdateVisibilityGroups);

    // Layer object marking: the dock holds the list, the canvas paints it and shows the hint. Magenta is what the
    // layer already holds — shown the same way whether the user is adding or removing — and orange is what Enter
    // will act on.
    connect(m_dockManager->groupsWidget(), &VWidgetGroups::ObjectInLayerChanged, this,
            [](quint32 objectId, quint32 toolId, bool inLayer)
    {
        // FindTool, not getTool, and this is not a style choice: getTool() THROWS VExceptionBadId on an id the
        // document does not hold, and an exception thrown from a slot has no handler above it — the application
        // aborts. A layer entry can name such a tool (VWidgetGroups::LayerItems drops them, this is the second
        // line of defence). SetLayerObjectsInLayer tolerates a null item.
        SetLayerObjectsInLayer(dynamic_cast<QGraphicsItem *>(VAbstractPattern::FindTool(toolId)), objectId, inLayer);
    });
    connect(m_dockManager->groupsWidget(), &VWidgetGroups::ObjectSelectedChanged, this,
            [](quint32 objectId, quint32 toolId, bool selected)
    {
        // Same reason as above. The object is passed on: a tool may draw several, and only the one the dock
        // speaks about takes the mark.
        SetLayerObjectsSelected(dynamic_cast<QGraphicsItem *>(VAbstractPattern::FindTool(toolId)), objectId,
                                selected);
    });
    connect(m_dockManager->groupsWidget(), &VWidgetGroups::ShowStatusTip, this,
            [this](const QString &text) { m_statusLabel->setText(text); });
    // The dock's own switch for the active layer's chrome: the setting is software-level, so MainWindow writes it
    // and redraws — the same split the Size Styles dock uses for its "Show in canvas" gate.
    connect(m_dockManager->groupsWidget(), &VWidgetGroups::ShowActiveLayerOnCanvasChanged, this,
            &MainWindow::ShowActiveLayerOnCanvas);

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
    connect(m_dockManager->sizeRangesWidget(), &OWidgetSizeRanges::exportRequested, this,
            &MainWindow::ExportGradedRange);
    connect(m_dockManager->sizeRangesWidget(), &OWidgetSizeRanges::exportAllRequested, this,
            &MainWindow::ExportAllGradedRanges);
    connect(m_dockManager->sizeRangesWidget(), &OWidgetSizeRanges::rangeActivated, this,
            &MainWindow::ActivateGradedRange);

    // Size Styles dock → MainWindow (edit dialog + global "Show in canvas" gate).
    connect(m_dockManager->sizeStylesWidget(), &OWidgetSizeStyles::editRequested, this,
            &MainWindow::EditSizeStyles);
    connect(m_dockManager->sizeStylesWidget(), &OWidgetSizeStyles::showInCanvasChanged, this,
            &MainWindow::ShowGradedSizeStylesInCanvas);

    // Outputs dock → MainWindow (Add + inline rename). AddExportOutput is shared with
    // EnterExportFlow's empty-list auto-open — same code path for both entries.
    connect(m_dockManager->outputsWidget(), &OWidgetOutputs::addRequested, this,
            &MainWindow::AddExportOutput);
    // Queued: the rename is emitted from inside the item editor's commitData — the
    // store write + dock refresh must run only once the editor has closed.
    connect(m_dockManager->outputsWidget(), &OWidgetOutputs::renameRequested, this,
            &MainWindow::RenameExportOutput, Qt::QueuedConnection);
    // Direct: the status icon is a plain click, and it redraws the scene under it.
    connect(m_dockManager->outputsWidget(), &OWidgetOutputs::activeRequested, this,
            &MainWindow::SetActiveExportOutput);
    // Direct too, and for the same reason: the star is a plain click. It writes the software
    // store and refills the dock — no nesting runs behind it, so there is nothing to wait for.
    connect(m_dockManager->outputsWidget(), &OWidgetOutputs::favoriteRequested, this,
            &MainWindow::ToggleExportFavorite);
    // Queued: a refresh runs a nesting behind a modal wait dialog with a nested event
    // loop. Returning into the view's own mouse handling once the run is over — minutes
    // later — is what queuing avoids.
    connect(m_dockManager->outputsWidget(), &OWidgetOutputs::refreshRequested, this,
            &MainWindow::RefreshExportOutput, Qt::QueuedConnection);
    connect(m_dockManager->outputsWidget(), &OWidgetOutputs::deleteRequested, this,
            &MainWindow::DeleteExportOutput);
    connect(m_dockManager->outputsWidget(), &OWidgetOutputs::exportRequested, this,
            &MainWindow::ExportOutputToPdf);
    // Le lot : le même dock, un cran au-dessus. Direct, comme la cellule d'export : le dialogue est
    // modal et l'écriture est synchrone — il n'y a aucun nesting à attendre derrière, donc rien à
    // mettre en file.
    connect(m_dockManager->outputsWidget(), &OWidgetOutputs::exportSeveralRequested, this,
            &MainWindow::ExportSeveralOutputs);

    // Cover page dock → the canvas, and NOT through the document's own signal. Editing the cover
    // page raises `patternChanged`, which the Export mode reads as "the pattern may have moved"
    // and which cannot say that a free text displaces no piece. This dock says what it did.
    //
    // Direct, like the template editor's own connection: the redraw is a scene rebuild and no
    // nesting, so there is no modal run to return out of.
    if (m_dockManager->coverPageWidget() != nullptr)
    {
        connect(m_dockManager->coverPageWidget(), &OWidgetCoverPage::CoverPageChanged, this,
                &MainWindow::RefreshExportDrawing);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshGroupHolders()
{
    const auto *groups = pattern != nullptr ? pattern->DataPieceGroups() : nullptr;
    if (groups == nullptr)
    {
        return;
    }

    // **A link or an unlink moves the members' own geometry, and the graded canvas freezes each
    // piece's base the first time it enters a build.** A frozen base outliving the link would place
    // the members where they stood *before* it, so the group could never reform there — the very
    // defect that reopening the file repairs, since the load empties that cache. Re-freeze the
    // pieces the group set gained or lost. A piece no group ever touched keeps its base, so a plain
    // Details move still never relocates the graded canvas.
    QSet<quint32> groupedPieceIds;
    for (auto git = groups->cbegin(); git != groups->cend(); ++git)
    {
        for (const quint32 pieceId : git.value().pieceIds)
        {
            groupedPieceIds.insert(pieceId);
        }
    }
    if (groupedPieceIds != m_lastGroupedPieceIds)
    {
        for (const quint32 pieceId : groupedPieceIds)
        {
            m_gradedFrozenBases.remove(pieceId);
        }
        for (const quint32 pieceId : m_lastGroupedPieceIds)
        {
            m_gradedFrozenBases.remove(pieceId);
        }
        m_lastGroupedPieceIds = groupedPieceIds;
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
                    [this](quint32 groupId, OPieceGroupHolderItem::Child child)
            {
                // **The frame selects the group, a child selects the child.** A click on the group's
                // own frame shows its selection block on the canvas, like a piece; a click on one of
                // its children draws nothing on the group — exactly what happens on a piece, whose
                // own selection is not drawn either when a block or the miniature is clicked.
                if (child == OPieceGroupHolderItem::Child::None)
                {
                    UpdateGroupHolderSelection(groupId);
                }
                else
                {
                    // Any previous group's block goes too: what is being edited is a child, and the
                    // canvas must show that child's frame and nothing else.
                    UpdateGroupHolderSelection(NULL_ID);
                }
                m_dockManager->detailsOptions()->ShowGroupOptions(groupId, pattern, doc);
                // Like a piece (SelectedDetail → SelectDetail), a canvas click on the group
                // must activate the group node in the pieces tree — it never was, so the
                // group was never visually active (unlike a selected piece).
                m_dockManager->detailsWidget()->SelectDetail(groupId);

                // Then open what configures the thing that was clicked — the same routing a piece's
                // children use. The group page owns those tabs, and it is the current page by now.
                switch (child)
                {
                    case OPieceGroupHolderItem::Child::Grainline:
                        m_dockManager->detailsOptions()->ShowGrainlineTab();
                        break;
                    case OPieceGroupHolderItem::Child::InfoBlock:
                        m_dockManager->detailsOptions()->ShowBlocksTab();
                        break;
                    case OPieceGroupHolderItem::Child::Annotation:
                        m_dockManager->detailsOptions()->ShowAnnotationsTab();
                        break;
                    case OPieceGroupHolderItem::Child::Thumbnail:
                        m_dockManager->detailsOptions()->ShowThumbnailTab();
                        break;
                    case OPieceGroupHolderItem::Child::None:
                        break;
                }
            });
            // Canvas right-click on a group → single action "Unlink all" (mirror of the Pieces
            // tree menu). Queued: the undo push deletes this very holder while it is handling
            // its own context-menu event, so the request must run after that handler returns.
            connect(holder, &OPieceGroupHolderItem::UnlinkAllRequested, this,
                    [this](quint32 groupId)
            {
                // Mirror VWidgetDetails::UnlinkGroup: remember the members, push the undo
                // command, then restore each member's own children and refresh the tree.
                const auto *groups = pattern->DataPieceGroups();
                QVector<quint32> memberIds;
                if (groups != nullptr && groups->contains(groupId))
                {
                    memberIds = groups->value(groupId).pieceIds;
                }

                auto *unlinkCommand = new UnlinkPieceGroup(groupId, doc, pattern);
                VAbstractApplication::VApp()->getUndoStack()->push(unlinkCommand);

                for (quint32 pieceId : memberIds)
                {
                    auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(pieceId));
                    if (tool != nullptr)
                    {
                        tool->RefreshGroupedChildren();
                    }
                }
                m_dockManager->detailsWidget()->UpdateList();
            }, Qt::QueuedConnection);
            // Canvas right-click on a group → "In layout", the same toggle a piece has. Direct
            // connection (nothing is destroyed here): the Pieces dock owns the undo command and
            // immediately gets the whole chain running — tree icon, member rows, then
            // GroupVisibilityChanged, which masks the holder and the member pieces on the canvas.
            connect(holder, &OPieceGroupHolderItem::InLayoutToggleRequested, this, [this](quint32 groupId)
            {
                m_dockManager->detailsWidget()->ToggleGroupInLayout(groupId);
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
    // Silenced before anything else is torn down, for the reason VPMainWindow spells out in its
    // own destructor: destroying an undo stack can emit cleanChanged, and every slot that signal
    // can reach lives on this window — whose derived part is already dismantled by the time its
    // QObject children are destroyed. The group is a second emitter in that same window, and it
    // forwards whatever its active stack says.
    if (m_undoGroup != nullptr)
    {
        m_undoGroup->blockSignals(true);
    }
    if (m_exportCorrection != nullptr)
    {
        m_exportCorrection->UndoStack()->blockSignals(true);
    }

    CancelTool();

    delete doc;
    // Delete the scenes while the window widgets (m_statusLabel lives in the status bar) are still alive: a tool
    // visualization or item emitting ToolTip() during scene teardown would otherwise write into freed memory
    // (upstream 8a5caf9e3).
    delete m_sceneDraw;
    delete m_sceneDetails;
    delete m_sceneGraded;
    delete m_sceneExport;
    delete ui;
}


























































