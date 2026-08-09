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
#include "widgets/DrawToolBar.h"
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

#include "../vpropertyexplorer/qtcolorpicker.h"
#include "../ifc/exception/vexceptionconversionerror.h"
#include "../ifc/exception/vexceptionemptyparameter.h"
#include "../ifc/exception/vexceptioninvalidhistory.h"
#include "../ifc/exception/vexceptionobjecterror.h"
#include "../ifc/exception/vexceptionundo.h"
#include "../ifc/exception/vexceptionwrongid.h"
#include "../ifc/xml/utils.h"
#include "../ifc/xml/vbackgroundpatternimage.h"
#include "../ifc/xml/vpatternconverter.h"
#include "../ifc/xml/vvitconverter.h"
#include "../ifc/xml/vvstconverter.h"
#include "../vformat/vmeasurements.h"
#include "../vformat/vpatternrecipe.h"
#include "../vlayout/dialogs/dialoglayoutscale.h"
#include "../vlayout/dialogs/watermarkwindow.h"
#include "../vlayout/vlayoutexporter.h"
#include "../vmisc/customevents.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "../vmisc/def.h"
#include "../vmisc/dialogs/dialogaskcollectstatistic.h"
#include "../vmisc/dialogs/dialogselectlanguage.h"
#include "../vmisc/qxtcsvmodel.h"
#include "vtheme.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vmodifierkey.h"
#include "../vmisc/vsysexits.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vfontinstaller.h"
#include "../vmisc/projectversion.h"
#include "../vmisc/dialogs/dialogselectmeasurementstype.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vpatterndb/variables/vmeasurement.h"
#include "../vtools/dialogs/support/dialogeditlabel.h"
#include "../vtools/dialogs/tools/dialogalongline.h"
#include "../vtools/dialogs/tools/dialogarc.h"
#include "../vtools/dialogs/tools/dialogarcwithlength.h"
#include "../vtools/dialogs/tools/dialogbisector.h"
#include "../vtools/dialogs/tools/dialogcubicbezier.h"
#include "../vtools/dialogs/tools/dialogcubicbezierpath.h"
#include "../vtools/dialogs/tools/dialogcurveintersectaxis.h"
#include "../vtools/dialogs/tools/dialogcutarc.h"
#include "../vtools/dialogs/tools/dialogcutspline.h"
#include "../vtools/dialogs/tools/dialogcutsplinepath.h"
#include "../vtools/dialogs/tools/dialogellipticalarcwithlength.h"
#include "../vtools/dialogs/tools/dialogellipticalarc.h"
#include "../vtools/dialogs/tools/dialogendline.h"
#include "../vtools/dialogs/tools/dialogflippingbyaxis.h"
#include "../vtools/dialogs/tools/dialogflippingbyline.h"
#include "../vtools/dialogs/tools/dialoggroup.h"
#include "../vtools/dialogs/tools/dialogheight.h"
#include "../vtools/dialogs/tools/dialogline.h"
#include "../vtools/dialogs/tools/dialoglineintersect.h"
#include "../vtools/dialogs/tools/dialoglineintersectaxis.h"
#include "../vtools/dialogs/tools/dialogmove.h"
#include "../vtools/dialogs/tools/dialognormal.h"
#include "../vtools/dialogs/tools/dialogpointfromarcandtangent.h"
#include "../vtools/dialogs/tools/dialogpointfromcircleandtangent.h"
#include "../vtools/dialogs/tools/dialogpointofcontact.h"
#include "../vtools/dialogs/tools/dialogpointofintersection.h"
#include "../vtools/dialogs/tools/dialogpointofintersectionarcs.h"
#include "../vtools/dialogs/tools/dialogpointofintersectioncircles.h"
#include "../vtools/dialogs/tools/dialogpointofintersectioncurves.h"
#include "../vtools/dialogs/tools/dialogrotation.h"
#include "../vtools/dialogs/tools/dialogshoulderpoint.h"
#include "../vtools/dialogs/tools/dialogspline.h"
#include "../vtools/dialogs/tools/dialogsplinepath.h"
#include "../vtools/dialogs/tools/dialogtriangle.h"
#include "../vtools/dialogs/tools/dialogtruedarts.h"
#include "../vtools/dialogs/tools/dialoguniondetails.h"
#include "../vtools/dialogs/tools/piece/dialogduplicatedetail.h"
#include "../vtools/dialogs/tools/piece/dialoginsertnode.h"
#include "../vtools/dialogs/tools/piece/dialogpiecepath.h"
#include "../vtools/dialogs/tools/piece/dialogpin.h"
#include "../vtools/dialogs/tools/piece/dialogplacelabel.h"
#include "../vtools/dialogs/tools/piece/dialogseamallowance.h"
#include "../vtools/tools/backgroundimage/vbackgroundimagecontrols.h"
#include "../vtools/tools/backgroundimage/vbackgroundimageitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundpixmapitem.h"
#include "../vtools/tools/backgroundimage/vbackgroundsvgitem.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyaxis.h"
#include "../vtools/tools/drawTools/operation/flipping/vtoolflippingbyline.h"
#include "../vtools/tools/drawTools/operation/vtoolmove.h"
#include "../vtools/tools/drawTools/operation/vtoolrotation.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezier.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolcubicbezierpath.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarc.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolspline.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/tooldoublepoint/vtooltruedarts.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutarc.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutspline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toolcut/vtoolcutsplinepath.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolalongline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolbisector.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolcurveintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolendline.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolheight.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoollineintersectaxis.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolnormal.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/toollinepoint/vtoolshoulderpoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolbasepoint.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoollineintersect.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromarcandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointfromcircleandtangent.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofcontact.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersection.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectionarcs.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncircles.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtoolpointofintersectioncurves.h"
#include "../vtools/tools/drawTools/toolpoint/toolsinglepoint/vtooltriangle.h"
#include "../vtools/tools/drawTools/vtoolline.h"
#include "../vtools/tools/nodeDetails/vtoolpiecepath.h"
#include "../vtools/tools/nodeDetails/vtoolpin.h"
#include "../vtools/tools/nodeDetails/vtoolplacelabel.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/tools/vtooluniondetails.h"
#include "../vtools/undocommands/image/addbackgroundimage.h"
#include "../vtools/undocommands/image/deletebackgroundimage.h"
#include "../vtools/undocommands/renamepp.h"
#include "../vtools/undocommands/undogroup.h"
#include "../vtools/undocommands/label/movedoublelabel.h"
#include "../vtools/undocommands/label/movelabel.h"
#include "../vtools/undocommands/label/movesegmentlabel.h"
#include "../vtools/undocommands/label/operationmovelabel.h"
#include "../vwidgets/vabstractsimple.h"
#include "../vwidgets/vgraphicssimpletextitem.h"
#include "../vwidgets/labelarrange/vdrawlabelcollector.h"
#include "../vwidgets/labelarrange/vlabelarrangeengine.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vscenepoint.h"
#include "../vwidgets/vsegmentlabel.h"
#include "../vwidgets/vsimplecurve.h"
#include "../vwidgets/vsimplepoint.h"
#include "../vtools/tools/vabstracttool.h"
#include "../vwidgets/vtoolbuttonpopup.h"
#include "../vwidgets/vwidgetpopup.h"
#include "core/vapplication.h"
#include "core/vtooloptionspropertybrowser.h"
#include "dialogs/dialogaboutapp.h"
#include "dialogs/dialogaddbackgroundimage.h"
#include "dialogs/dialogfinalmeasurements.h"
#include "dialogs/dialogincrements.h"
#include "dialogs/DialogOpenMeasurements.h"
#include "dialogs/dialognewpattern.h"
#include "dialogs/dialogpatternproperties.h"
#include "widgets/MeasurementWorkspace/MeasurementWindow.h"
#include "widgets/MeasurementWorkspace/MeasurementWorkspace.h"
#include "WelcomeScreen.h"
#include "StyleGuideWindow.h"
#include <QStackedWidget>
#include "dialogs/dialogpreferences.h"
#include "../vtools/dialogs/tools/dialogparallelcurve.h"
#include "dialogs/vwidgetbackgroundimages.h"
#include "dialogs/vwidgetdetails.h"
#include "dialogs/vwidgetgroups.h"
#include "dialogs/vwidgetdependencies.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolellipticalarcwithlength.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolparallelcurve.h"
#include "../vtools/tools/drawTools/toolcurve/vtoolgraduatedcurve.h"
#include "../vtools/dialogs/tools/dialoggraduatedcurve.h"
#include "vabstractapplication.h"
#include "vabstractshortcutmanager.h"
#include "vsinglelineoutlinechar.h"
#include "../ifc/xml/vpatternblockmapper.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include "../vmisc/vtextcodec.h"
using QTextCodec = VTextCodec;
#else
#include <QTextCodec>
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

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
        {QStringLiteral("spline_cut_point"),             ui->actionSplineCutPointTool},
        {QStringLiteral("splinePath_cut_point"),         ui->actionSplinePathCutPointTool},
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
        {QStringLiteral("spline"),                       ui->actionSplineTool},
        {QStringLiteral("cubic_bezier"),                 ui->actionCubicBezierTool},
        {QStringLiteral("splinePath"),                   ui->actionSplinePathTool},
        {QStringLiteral("cubic_bezier_path"),            ui->actionCubicBezierPathTool},
        {QStringLiteral("arc"),                          ui->actionArcTool},
        {QStringLiteral("arc_with_length"),              ui->actionArcWithLengthTool},
        {QStringLiteral("el_arc"),                       ui->actionEllipticalArcTool},
        {QStringLiteral("el_arc_with_length"),           ui->actionEllipticalArcWithLengthTool},
        {QStringLiteral("parallel_curve"),               ui->actionParallelCurveTool},
        {QStringLiteral("graduated_curve_offseting"),    ui->actionGraduatedCurveTool},
        {QStringLiteral("group_plus"),                   ui->actionGroupTool},
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
    connect(ui->actionReloadLabels, &QAction::triggered, this, &MainWindow::ActionReloadLabels);
    connect(ui->actionAutoArrangeLabels, &QAction::triggered, this, &MainWindow::RunAutoArrangeLabels);

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
    m_stackedWidget->addWidget(oldCentral);      // index 0 — editor (wraps ui->view)
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
void MainWindow::InitWelcomePage()
{
    // Pattern signals
    connect(m_welcomeScreen, &WelcomeScreen::NewPatternClicked, this, &MainWindow::on_actionNew_triggered);
    connect(m_welcomeScreen, &WelcomeScreen::OpenPatternClicked, this, &MainWindow::on_actionOpen_triggered);
    connect(m_welcomeScreen, &WelcomeScreen::OpenRecentPattern, this,
            [this](const QString &path) { LoadPattern(path); });

    // Measurement signals (implemented in mainwindow_welcome.cpp)
    connect(m_welcomeScreen, &WelcomeScreen::NewMeasurementClicked, this, &MainWindow::WelcomeNewMeasurement);
    connect(m_welcomeScreen, &WelcomeScreen::OpenMeasurementClicked, this, &MainWindow::WelcomeOpenMeasurement);
    connect(m_welcomeScreen, &WelcomeScreen::OpenRecentMeasurement, this, &MainWindow::WelcomeOpenRecentMeasurement);

    // Features button (dev-only)
    connect(m_welcomeScreen, &WelcomeScreen::FeaturesClicked, this,
            []() { (new StyleGuideWindow())->show(); });
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowWelcomeScreen()
{
    m_stackedWidget->setCurrentWidget(m_welcomeScreen);

    // Hide editor chrome
    ui->topBar->setVisible(false);
    m_drawToolBar->setVisible(false);
    ui->dockWidgetToolOptions->setVisible(false);
    ui->dockWidgetLayoutPages->setVisible(false);
    ui->dockWidgetGroups->setVisible(false);
    ui->dockWidgetMessages->setVisible(false);
    ui->dockWidgetBackgroundImages->setVisible(false);
    ui->dockWidgetDependencies->setVisible(false);
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

    // Restore dock visibility from saved state booleans
    ui->dockWidgetToolOptions->setVisible(m_toolOptionsActive);
    ui->dockWidgetGroups->setVisible(m_groupsActive);
    ui->dockWidgetMessages->setVisible(m_patternMessagesActive);
    ui->dockWidgetBackgroundImages->setVisible(m_backgroundImagesActive);
    ui->dockWidgetDependencies->setVisible(m_dependenciesActive);
    // LayoutPages dock is hidden except in Layout mode

    UpdateWindowTitle();
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

    if (m_comboBoxDraws->count() == 0)
    {
        m_sceneDraw->InitOrigins();
        m_sceneDetails->InitOrigins();
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

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitScenes()
{
    m_sceneDraw = new VMainGraphicsScene(this);
    currentScene = m_sceneDraw;
    VAbstractValApplication::VApp()->setCurrentScene(&currentScene);
    connect(this, &MainWindow::EnableItemMove, m_sceneDraw, &VMainGraphicsScene::EnableItemMove);
    connect(this, &MainWindow::ItemsSelection, m_sceneDraw, &VMainGraphicsScene::ItemsSelection);

    connect(this, &MainWindow::EnableLabelSelection, m_sceneDraw, &VMainGraphicsScene::ToggleLabelSelection);
    connect(this, &MainWindow::EnablePointSelection, m_sceneDraw, &VMainGraphicsScene::TogglePointSelection);
    connect(this, &MainWindow::EnableLineSelection, m_sceneDraw, &VMainGraphicsScene::ToggleLineSelection);
    connect(this, &MainWindow::EnableArcSelection, m_sceneDraw, &VMainGraphicsScene::ToggleArcSelection);
    connect(this, &MainWindow::EnableElArcSelection, m_sceneDraw, &VMainGraphicsScene::ToggleElArcSelection);
    connect(this, &MainWindow::EnableSplineSelection, m_sceneDraw, &VMainGraphicsScene::ToggleSplineSelection);
    connect(this, &MainWindow::EnableSplinePathSelection, m_sceneDraw, &VMainGraphicsScene::ToggleSplinePathSelection);

    connect(this, &MainWindow::EnableLabelHover, m_sceneDraw, &VMainGraphicsScene::ToggleLabelHover);
    connect(this, &MainWindow::EnablePointHover, m_sceneDraw, &VMainGraphicsScene::TogglePointHover);
    connect(this, &MainWindow::EnableLineHover, m_sceneDraw, &VMainGraphicsScene::ToggleLineHover);
    connect(this, &MainWindow::EnableArcHover, m_sceneDraw, &VMainGraphicsScene::ToggleArcHover);
    connect(this, &MainWindow::EnableElArcHover, m_sceneDraw, &VMainGraphicsScene::ToggleElArcHover);
    connect(this, &MainWindow::EnableSplineHover, m_sceneDraw, &VMainGraphicsScene::ToggleSplineHover);
    connect(this, &MainWindow::EnableSplinePathHover, m_sceneDraw, &VMainGraphicsScene::ToggleSplinePathHover);

    connect(this, &MainWindow::ShowArcSegmentLabel, m_sceneDraw, &VMainGraphicsScene::ToggleArcSegmentVisibility);
    connect(this, &MainWindow::ShowElArcSegmentLabel, m_sceneDraw, &VMainGraphicsScene::ToggleElArcSegmentVisibility);
    connect(this, &MainWindow::ShowSplineSegmentLabel, m_sceneDraw, &VMainGraphicsScene::ToggleSplineSegmentVisibility);
    connect(this, &MainWindow::ShowSplinePathSegmentLabel, m_sceneDraw,
            &VMainGraphicsScene::ToggleSplinePathSegmentVisibility);

    connect(m_sceneDraw, &VMainGraphicsScene::mouseMove, this, &MainWindow::MouseMove);
    connect(m_sceneDraw, &VMainGraphicsScene::AddBackgroundImage, this, &MainWindow::PlaceBackgroundImage);

    m_sceneDetails = new VMainGraphicsScene(this);
    connect(this, &MainWindow::EnableItemMove, m_sceneDetails, &VMainGraphicsScene::EnableItemMove);

    connect(this, &MainWindow::EnableNodeLabelSelection, m_sceneDetails, &VMainGraphicsScene::ToggleNodeLabelSelection);
    connect(this, &MainWindow::EnableNodePointSelection, m_sceneDetails, &VMainGraphicsScene::ToggleNodePointSelection);
    connect(this, &MainWindow::EnableDetailSelection, m_sceneDetails, &VMainGraphicsScene::ToggleDetailSelection);

    connect(this, &MainWindow::EnableNodeLabelHover, m_sceneDetails, &VMainGraphicsScene::ToggleNodeLabelHover);
    connect(this, &MainWindow::EnableNodePointHover, m_sceneDetails, &VMainGraphicsScene::ToggleNodePointHover);
    connect(this, &MainWindow::EnableDetailHover, m_sceneDetails, &VMainGraphicsScene::ToggleDetailHover);

    connect(m_sceneDetails, &VMainGraphicsScene::mouseMove, this, &MainWindow::MouseMove);

    ui->view->setScene(currentScene);

    m_sceneDraw->setTransform(ui->view->transform());
    m_sceneDetails->setTransform(ui->view->transform());

    connect(ui->view, &VMainGraphicsView::MouseRelease, this, [this]() -> void { EndVisualization(true); });
    connect(ui->view, &VMainGraphicsView::ScaleChanged, this, &MainWindow::ScaleChanged);
    connect(ui->view, &VMainGraphicsView::ZoomFitBestCurrent, this, &MainWindow::ZoomFitBestCurrent,
            Qt::QueuedConnection);
    // Shared state: stores a trigger function per item while it's inside the rubber band.
    // QPointer ensures we don't call into deleted objects.
    auto prevBandTriggers = std::make_shared<QMap<quint32, TriggerFn>>();

    connect(ui->view, &VMainGraphicsView::RubberBandSelection, this,
            [this, prevBandTriggers](const QRectF &sceneRect) -> void
            {
                HandleRubberBandSelection(m_sceneDraw, sceneRect, prevBandTriggers);
            });

    connect(ui->view, &VMainGraphicsView::RubberBandSelectionStarted, this,
            [this]() -> void
            {
                if (m_dialogTool)
                {
                    m_dialogTool->ClearSourceObjects();
                }
            });

    connect(ui->view, &VMainGraphicsView::RubberBandSelectionFinished, this,
            [prevBandTriggers]() -> void { prevBandTriggers->clear(); });
    connect(ui->view, &VMainGraphicsView::SceneContextMenuRequested, this, &MainWindow::AutoArrangeLabels);
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    policy.setHorizontalStretch(12);
    ui->view->setSizePolicy(policy);
    VAbstractValApplication::VApp()->setSceneView(ui->view);
}
// Extracted: 


// Extracted: 



//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::UpdateMeasurements(const QSharedPointer<VMeasurements> &mFile, qreal baseA, qreal baseB, qreal baseC,
                                    bool rebuildGradation) -> bool
{
    if (mFile.isNull())
    {
        qCWarning(vMainWindow, "UpdateMeasurements called with null measurement file; keeping existing data.");
        return false;
    }

    m_m = mFile;

    if (VAbstractValApplication::VApp()->GetMeasurementsType() != m_m->Type())
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Measurement files types have not match.")));
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_DATAERR);
        }
        return false;
    }

    try
    {
        ReadMeasurements(baseA, baseB, baseC, rebuildGradation);
    }
    catch (VExceptionEmptyParameter &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("File error.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return false;
    }

    return true;
}
// Extracted: 
// Template extracted to mainwindow_tools.cpp

// Extracted: /**

// Extracted: //---------------------------------------------------------------------------------------------------------------------
// Template extracted to mainwindow_tools.cpp

// Template extracted to mainwindow_tools.cpp

// Template extracted to mainwindow_tools.cpp

// Template extracted to mainwindow_tools.cpp

// Template extracted to mainwindow_tools.cpp

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ZoomFitBestCurrent()
{
    if (m_drawMode)
    {
        const QRectF rect = doc->ActiveDrawBoundingRect();
        if (rect.isEmpty())
        {
            return;
        }

        ui->view->fitInView(rect, Qt::KeepAspectRatio);
        QTransform transform = ui->view->transform();

        const qreal scale = qBound(VMainGraphicsView::MinScale(),
                                   transform.m11(),
                                   VMainGraphicsView::MaxScale());

        transform.setMatrix(scale, transform.m12(), transform.m13(), transform.m21(), scale, transform.m23(),
                            transform.m31(), transform.m32(), transform.m33());
        ui->view->setTransform(transform);

        ScaleChanged(scale);
    }
}
// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------
// Extracted: 

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ShowTool  highlight tool.Tip show tools tooltip.
 * @param toolTip tooltip text.
 */
void MainWindow::ShowToolTip(const QString &toolTip)
{
    m_statusLabel->setText(toolTip);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief triggers the update of the visibility groups
 */
void MainWindow::UpdateVisibilityGroups()
{
    m_groupsWidget->UpdateGroups();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief triggers the update of the details list
 */
void MainWindow::UpdateDetailsList()
{
    m_detailsWidget->UpdateList();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SetToolOptionsDialogVisible(bool visible)
{
    m_toolOptionsDialogVisible = visible;

    // While a tool options dialog is open we must not let the user run undo/redo. An undo/redo emits
    // VPattern::UndoCommand which triggers MainWindow::FullParseFile(), tearing down all tools and the
    // VContainer the open dialog still references. Accepting the now-stale dialog afterwards dereferences
    // freed geometry (use-after-free, e.g. crash in DialogSplinePath::SaveData).
    if (visible)
    {
        undoAction->setEnabled(false);
        redoAction->setEnabled(false);
    }
    else
    {
        undoAction->setEnabled(VAbstractApplication::VApp()->getUndoStack()->canUndo());
        redoAction->setEnabled(VAbstractApplication::VApp()->getUndoStack()->canRedo());
    }
}

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

        if (VApplication::VApp()->GetDrawMode() == Draw::Calculation)
        {
            ui->dockWidgetGroups->setWindowTitle(tr("Groups of visibility"));
            ui->dockWidgetGroups->setToolTip(tr("Contains all visibility groups"));
        }
        else
        {
            ui->dockWidgetGroups->setWindowTitle(tr("Details"));
            ui->dockWidgetGroups->setToolTip(tr("Show which details will go in layout"));
        }

        UpdateMeasurementState();


        if (VApplication::VApp()->ValentinaSettings()->IsUseToolGroups())
        {
            m_drawToolBar->SetMode(VApplication::VApp()->GetDrawMode());
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
void MainWindow::CleanLayout()
{
    m_layoutSettings->CleanLayout();
    gcontours.clear();
    ui->listWidget->clear();
    SetLayoutModeActions();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::PrepareSceneList(PreviewQuatilty quality)
{
    ui->listWidget->clear();
    for (int i = 1; i <= m_layoutSettings->LayoutScenes().size(); ++i)
    {
        auto *item = new QListWidgetItem(ScenePreview(i - 1, ui->listWidget->iconSize(), quality), QString::number(i));
        ui->listWidget->addItem(item);
    }

    if (not m_layoutSettings->LayoutScenes().isEmpty())
    {
        ui->listWidget->setCurrentRow(0);
        SetLayoutModeActions();
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

#if defined(Q_OS_MAC)
//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolBarStyle(QToolBar *bar) const
{
    MainWindowsNoGUI::ToolBarStyle(bar);

    // Temporary fix issue with toolbar black background on mac with OpenGL render
    if (VAbstractValApplication::VApp()->getSceneView() &&
        VAbstractValApplication::VApp()->getSceneView()->IsOpenGLRender())
    {
        bar->setStyle(QStyleFactory::create("fusion"));
    }
}
#endif

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ScaleChanged(qreal scale)
{
    VMainGraphicsScene const *scene = nullptr;
    if (ui->view->scene() == m_sceneDraw)
    {
        scene = m_sceneDraw;
    }
    else if (ui->view->scene() == m_sceneDetails)
    {
        scene = m_sceneDetails;
    }
    else
    {
        return;
    }

    if (not m_doubleSpinBoxScale.isNull())
    {
        const QSignalBlocker blocker(m_doubleSpinBoxScale);
        m_doubleSpinBoxScale->setMaximum(qFloor(VMainGraphicsView::MaxScale() * 1000) / 10.0);
        m_doubleSpinBoxScale->setMinimum(qFloor(VMainGraphicsView::MinScale() * 1000) / 10.0);
        m_doubleSpinBoxScale->setValue(qFloor(scale * 1000) / 10.0);
        m_doubleSpinBoxScale->setSingleStep(1);
    }

    // Scale-dependent item state must be refreshed here, not inside paint(). Since Qt 6
    // mutating geometry/visibility/selection during paint() crashes the paint pipeline on
    // Windows.
    for (QGraphicsItem *item : scene->items())
    {
        // VGraphicsSimpleTextItem and VToolSeamAllowance have a single fixed type(), so a cheap
        // int compare + static_cast is enough. VScenePoint is a base class whose subclasses
        // override type(), so it can only be matched with dynamic_cast.
        if (item->type() == VGraphicsSimpleTextItem::Type)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<VGraphicsSimpleTextItem *>(item)->RefreshScale();
        }
        else if (item->type() == VToolSeamAllowance::Type)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
            static_cast<VToolSeamAllowance *>(item)->RefreshScale();
        }
        else if (auto *point = dynamic_cast<VScenePoint *>(item))
        {
            point->RefreshScale();
        }
    }
}
// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted to mainwindow_watermark.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_watermark.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_watermark.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_watermark.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_watermark.cpp: //---------------------------------------------------------------------------------------------------------------------
// Extracted: 


// Extracted: 



//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreMultisizeMDimension(const QList<MeasurementDimension_p> &dimensions, int index, qreal currentBase)
{
    if (dimensions.size() > index)
    {
        const MeasurementDimension_p &dimension = dimensions.at(index);
        const DimesionLabels labels = dimension->Labels();

        switch (dimension->Type())
        {
            case MeasurementDimension::X:
                VAbstractValApplication::VApp()->SetDimensionHeight(currentBase);
                VAbstractValApplication::VApp()->SetDimensionHeightLabel(
                    VFuzzyValue(labels, currentBase, QString::number(currentBase)));
                break;
            case MeasurementDimension::Y:
            {
                const bool fc = m_m->IsFullCircumference();
                VAbstractValApplication::VApp()->SetDimensionSize(fc ? currentBase * 2 : currentBase);
                VAbstractValApplication::VApp()->SetDimensionSizeLabel(
                    VFuzzyValue(labels, currentBase, QString::number(fc ? currentBase * 2 : currentBase)));
                const bool measurement = dimension->IsBodyMeasurement();
                VAbstractValApplication::VApp()->SetDimensionSizeUnits(measurement ? m_m->Units()
                                                                                   : Unit::LAST_UNIT_DO_NOT_USE);
                break;
            }
            case MeasurementDimension::W:
            {
                const bool fc = m_m->IsFullCircumference();
                VAbstractValApplication::VApp()->SetDimensionWaist(fc ? currentBase * 2 : currentBase);
                VAbstractValApplication::VApp()->SetDimensionWaistLabel(
                    VFuzzyValue(labels, currentBase, QString::number(fc ? currentBase * 2 : currentBase)));
                break;
            }
            case MeasurementDimension::Z:
            {
                const bool fc = m_m->IsFullCircumference();
                VAbstractValApplication::VApp()->SetDimensionHip(fc ? currentBase * 2 : currentBase);
                VAbstractValApplication::VApp()->SetDimensionHipLabel(
                    VFuzzyValue(labels, currentBase, QString::number(fc ? currentBase * 2 : currentBase)));
                break;
            }
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::StoreIndividualMDimension(const QMap<QString, QSharedPointer<VMeasurement>> &measurements, IMD type)
{
    const QString name = VAbstractApplication::VApp()->TrVars()->VarToUser(m_m->MeasurementForDimension(type));
    const bool valid = not name.isEmpty() && measurements.contains(name);
    const qreal value = valid ? *measurements.value(name)->GetValue() : 0;
    switch (type)
    {
        case IMD::X:
            VAbstractValApplication::VApp()->SetDimensionHeight(value);
            VAbstractValApplication::VApp()->SetDimensionHeightLabel(QString::number(value));
            break;
        case IMD::Y:
            VAbstractValApplication::VApp()->SetDimensionSize(value);
            VAbstractValApplication::VApp()->SetDimensionSizeLabel(QString::number(value));
            break;
        case IMD::W:
            VAbstractValApplication::VApp()->SetDimensionWaist(value);
            VAbstractValApplication::VApp()->SetDimensionWaistLabel(QString::number(value));
            break;
        case IMD::Z:
            VAbstractValApplication::VApp()->SetDimensionHip(value);
            VAbstractValApplication::VApp()->SetDimensionHipLabel(QString::number(value));
            break;
        case IMD::N:
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::DimensionRestrictedValues(int index, const MeasurementDimension_p &dimension) -> QVector<qreal>
{
    VDimensionRestriction restriction;
    if (index == 0)
    {
        restriction = m_m->Restriction(0);
    }
    else if (index == 1)
    {
        restriction = m_m->Restriction(m_currentDimensionA);
    }
    else
    {
        restriction = m_m->Restriction(m_currentDimensionA, m_currentDimensionB);
    }

    const QVector<qreal> bases = dimension->ValidBases();

    qreal min = VFuzzyIndexOf(bases, restriction.GetMin()) != -1 ? restriction.GetMin() : dimension->MinValue();
    qreal max = VFuzzyIndexOf(bases, restriction.GetMax()) != -1 ? restriction.GetMax() : dimension->MaxValue();

    if (min > max)
    {
        min = dimension->MinValue();
        max = dimension->MaxValue();
    }

    return VAbstartMeasurementDimension::ValidBases(min, max, dimension->Step(), restriction.GetExcludeValues());
}
// Extracted: 


// Extracted: 


// Extracted: 


// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------


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

    m_curFileFormatVersion = VPatternConverter::PatternMaxVer;
    m_curFileFormatVersionStr = VPatternConverter::PatternMaxVerStr;
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
auto MainWindow::FullParsePattern() -> bool
{
    auto HandleError = [this]()
    {
        SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_DATAERR);
        }
    };

    try
    {
        // QDomDocument is not thread-safe and Parse() rewrites the same document, so this must stay on this thread.
        if (VAbstractValApplication::VApp()->getOpeningPattern())
        {
            doc->TestUniqueId();
        }

        SetEnabledGUI(true);
        doc->Parse(Document::FullParse);
        ParseBackgroundImages();
    }
    catch (const VExceptionUndo &)
    {
        /* If user want undo last operation before undo we need finish broken redo operation. For those we post event
         * myself. Later in method customEvent call undo.*/
        QApplication::postEvent(this, new UndoEvent());
        return false;
    }
    catch (const VExceptionObjectError &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error parsing file.")), //-V807
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (const VExceptionConversionError &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error can't convert value.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (const VExceptionEmptyParameter &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error empty parameter.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (const VExceptionWrongId &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error wrong id.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (VException &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error parsing file.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        HandleError();
        return false;
    }
    catch (const std::bad_alloc &)
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Error parsing file (std::bad_alloc).")));
        HandleError();
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RefreshPieceGeometry()
{
  if(!doc->IsPieceGeometryDirty())
  {
    return;
  }

  doc->RefreshDirtyPieceGeometry(pattern->DataPieces()->keys());
}

//---------------------------------------------------------------------------------------------------------------------
#if defined(Q_OS_MAC)
void MainWindow::OpenAt(QAction *where)
{
    const QString path = VAbstractValApplication::VApp()->GetPatternPath().left(
                             VAbstractValApplication::VApp()->GetPatternPath().indexOf(where->text())) +
                         where->text();
    if (path == VAbstractValApplication::VApp()->GetPatternPath())
    {
        return;
    }
    QProcess process;
    process.start(QStringLiteral("/usr/bin/open"), QStringList() << path, QIODevice::ReadOnly);
    process.waitForFinished();
}
#endif // defined(Q_OS_MAC)
// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitToolButtons()
{
    connect(ui->actionToolSelect, &QAction::triggered, this, &MainWindow::ArrowTool);

    // This check helps to find missed tools
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 64, "Check if all tools were connected.");

    auto InitToolButton = [this](VShortcutAction type, QAction *action, void (MainWindow::*slotFunction)(bool))
    {
        m_shortcutActions.insert(type, action);
        connect(action, &QAction::triggered, this, slotFunction);
    };

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
    InitToolButton(VShortcutAction::ToolRotation, ui->actionRotationTool, &MainWindow::ToolRotation);
    InitToolButton(VShortcutAction::ToolFlippingByLine, ui->actionFlippingByLineTool, &MainWindow::ToolFlippingByLine);
    InitToolButton(VShortcutAction::ToolFlippingByAxis, ui->actionFlippingByAxisTool, &MainWindow::ToolFlippingByAxis);
    InitToolButton(VShortcutAction::ToolMove, ui->actionMoveTool, &MainWindow::ToolMove);
    InitToolButton(VShortcutAction::ToolMidpoint, ui->actionMidpointTool, &MainWindow::ToolMidpoint);
    InitToolButton(VShortcutAction::ExportDrawAs, ui->actionExportDraw, &MainWindow::ExportDrawAs);
    InitToolButton(VShortcutAction::ExportLayoutAs, ui->actionLayoutExportAs, &MainWindow::ExportLayoutAs);
    InitToolButton(VShortcutAction::ExportDetailsAs, ui->actionDetailExportAs, &MainWindow::ExportDetailsAs);
    InitToolButton(VShortcutAction::ToolEllipticalArc, ui->actionEllipticalArcTool, &MainWindow::ToolEllipticalArc);
    InitToolButton(VShortcutAction::ToolPin, ui->actionPinTool, &MainWindow::ToolPin);
    InitToolButton(VShortcutAction::ToolInsertNode, ui->actionInsertNodeTool, &MainWindow::ToolInsertNode);
    InitToolButton(VShortcutAction::ToolPlaceLabel, ui->actionPlaceLabelTool, &MainWindow::ToolPlaceLabel);
    InitToolButton(VShortcutAction::ToolArcStart, ui->actionArcStartPointTool, &MainWindow::ToolArcStart);
    InitToolButton(VShortcutAction::ToolArcEnd, ui->actionArcEndPointTool, &MainWindow::ToolArcEnd);
    InitToolButton(VShortcutAction::ToolEllipticalArcWithLength, ui->actionEllipticalArcWithLengthTool,
                   &MainWindow::ToolEllipticalArcWithLength);
    InitToolButton(VShortcutAction::ToolParallelCurve, ui->actionParallelCurveTool, &MainWindow::ToolParallelCurve);
    InitToolButton(VShortcutAction::ToolGraduatedCurve, ui->actionGraduatedCurveTool, &MainWindow::ToolGraduatedCurve);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitActionShortcuts()
{
    m_shortcutActions.insert(VShortcutAction::New, ui->actionNew);
    m_shortcutActions.insert(VShortcutAction::Open, ui->actionOpen);
    m_shortcutActions.insert(VShortcutAction::Save, ui->actionSave);
    m_shortcutActions.insert(VShortcutAction::SaveAs, ui->actionSaveAs);
    m_shortcutActions.insert(VShortcutAction::DrawMode, ui->actionDraw);
    m_shortcutActions.insert(VShortcutAction::DetailsMode, ui->actionDetails);
    m_shortcutActions.insert(VShortcutAction::LayoutMode, ui->actionLayout);
    m_shortcutActions.insert(VShortcutAction::NewPatternPiece, ui->actionNewDraw);
    m_shortcutActions.insert(VShortcutAction::NextPatternPiece, ui->actionNextPatternPiece);
    m_shortcutActions.insert(VShortcutAction::PreviusPatternPiece, ui->actionPreviousPatternPiece);
    m_shortcutActions.insert(VShortcutAction::TableOfVariables, ui->actionTable);
    m_shortcutActions.insert(VShortcutAction::Quit, ui->actionExit);
    m_shortcutActions.insert(VShortcutAction::LastTool, ui->actionLast_tool);
    m_shortcutActions.insert(VShortcutAction::CurveDetails, ui->actionShowCurveDetails);
    m_shortcutActions.insert(VShortcutAction::FinalMeasurements, ui->actionFinalMeasurements);
    m_shortcutActions.insert(VShortcutAction::ReloadPieceLabels, ui->actionReloadLabels);
    m_shortcutActions.insert(VShortcutAction::AutoArrangeLabels, ui->actionAutoArrangeLabels);
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
 * @brief CancelTool cancel tool.
 */
void MainWindow::CancelTool()
{
    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 64, "Not all tools were handled.");

    qCDebug(vMainWindow, "Canceling tool.");
    if (not m_dialogTool.isNull())
    {
        m_dialogTool->hide();
        m_dialogTool->deleteLater();
    }
    m_dependenciesWidget->SetToolCreationActive(false); // Creation finished/cancelled: allow deletion again.
    qCDebug(vMainWindow, "Dialog closed.");

    currentScene->setFocus(Qt::OtherFocusReason);
    currentScene->clearSelection();
    emit ui->view->itemClicked(nullptr); // Hide visualization to avoid a crash

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (m_currentTool)
    {
        case Tool::Arrow:
            ui->actionToolSelect->setChecked(false);
            m_statusLabel->setText(QString());
            m_comboBoxDraws->setEnabled(false);

            // Crash: using CRTL+Z while using line tool.
            // related bug report:
            // https://bitbucket.org/dismine/valentina/issues/454/crash-using-crtl-z-while-using-line-tool
            undoAction->setEnabled(false);
            redoAction->setEnabled(false);
            VAbstractTool::m_suppressContextMenu = true;
            return;
        case Tool::BasePoint:
        case Tool::SinglePoint:
        case Tool::DoublePoint:
        case Tool::LinePoint:
        case Tool::AbstractSpline:
        case Tool::Cut:
        case Tool::LAST_ONE_DO_NOT_USE:
        case Tool::NodePoint:
        case Tool::NodeArc:
        case Tool::NodeElArc:
        case Tool::NodeSpline:
        case Tool::NodeSplinePath:
        case Tool::BackgroundImage:
        case Tool::BackgroundImageControls:
        case Tool::BackgroundPixmapImage:
        case Tool::BackgroundSVGImage:
            Q_UNREACHABLE(); //-V501
            // Nothing to do here because we can't create this tool from main window.
            break;
        case Tool::EndLine:
            ui->actionEndLineTool->setChecked(false);
            break;
        case Tool::Line:
            ui->actionLineTool->setChecked(false);
            break;
        case Tool::AlongLine:
            ui->actionAlongLineTool->setChecked(false);
            break;
        case Tool::Midpoint:
            ui->actionMidpointTool->setChecked(false);
            break;
        case Tool::ShoulderPoint:
            ui->actionShoulderPointTool->setChecked(false);
            break;
        case Tool::Normal:
            ui->actionNormalTool->setChecked(false);
            break;
        case Tool::Bisector:
            ui->actionBisectorTool->setChecked(false);
            break;
        case Tool::LineIntersect:
            ui->actionLineIntersectTool->setChecked(false);
            break;
        case Tool::Spline:
            ui->actionSplineTool->setChecked(false);
            break;
        case Tool::CubicBezier:
            ui->actionCubicBezierTool->setChecked(false);
            break;
        case Tool::Arc:
            ui->actionArcTool->setChecked(false);
            break;
        case Tool::ArcWithLength:
            ui->actionArcWithLengthTool->setChecked(false);
            break;
        case Tool::SplinePath:
            ui->actionSplinePathTool->setChecked(false);
            break;
        case Tool::CubicBezierPath:
            ui->actionCubicBezierPathTool->setChecked(false);
            break;
        case Tool::PointOfContact:
            ui->actionPointOfContactTool->setChecked(false);
            break;
        case Tool::Piece:
            ui->actionNewDetailTool->setChecked(false);
            break;
        case Tool::PiecePath:
            ui->actionInternalPathTool->setChecked(false);
            break;
        case Tool::Height:
            ui->actionHeightTool->setChecked(false);
            break;
        case Tool::Triangle:
            ui->actionTriangleTool->setChecked(false);
            break;
        case Tool::PointOfIntersection:
            ui->actionPointOfIntersectionTool->setChecked(false);
            break;
        case Tool::CutSpline:
            ui->actionSplineCutPointTool->setChecked(false);
            break;
        case Tool::CutSplinePath:
            ui->actionSplinePathCutPointTool->setChecked(false);
            break;
        case Tool::UnionDetails:
            ui->actionUnionDetailsTool->setChecked(false);
            break;
        case Tool::DuplicateDetail:
            ui->actionDuplicateDetailTool->setChecked(false);
            break;
        case Tool::CutArc:
            ui->actionArcCutPointTool->setChecked(false);
            break;
        case Tool::LineIntersectAxis:
            ui->actionLineIntersectAxisTool->setChecked(false);
            break;
        case Tool::CurveIntersectAxis:
            ui->actionCurveIntersectAxisTool->setChecked(false);
            break;
        case Tool::ArcIntersectAxis:
            ui->actionArcIntersectAxisTool->setChecked(false);
            break;
        case Tool::PointOfIntersectionArcs:
            ui->actionPointOfIntersectionArcsTool->setChecked(false);
            break;
        case Tool::PointOfIntersectionCircles:
            ui->actionPointOfIntersectionCirclesTool->setChecked(false);
            break;
        case Tool::PointOfIntersectionCurves:
            ui->actionIntersectionCurvesTool->setChecked(false);
            break;
        case Tool::PointFromCircleAndTangent:
            ui->actionPointFromCircleAndTangentTool->setChecked(false);
            break;
        case Tool::PointFromArcAndTangent:
            ui->actionPointFromArcAndTangentTool->setChecked(false);
            break;
        case Tool::TrueDarts:
            ui->actionTrueDartsTool->setChecked(false);
            break;
        case Tool::Group:
            ui->actionGroupTool->setChecked(false);
            break;
        case Tool::Rotation:
            ui->actionRotationTool->setChecked(false);
            break;
        case Tool::FlippingByLine:
            ui->actionFlippingByLineTool->setChecked(false);
            break;
        case Tool::FlippingByAxis:
            ui->actionFlippingByAxisTool->setChecked(false);
            break;
        case Tool::Move:
            ui->actionMoveTool->setChecked(false);
            break;
        case Tool::EllipticalArc:
            ui->actionEllipticalArcTool->setChecked(false);
            break;
        case Tool::Pin:
            ui->actionPinTool->setChecked(false);
            break;
        case Tool::InsertNode:
            ui->actionInsertNodeTool->setChecked(false);
            break;
        case Tool::PlaceLabel:
            ui->actionPlaceLabelTool->setChecked(false);
            break;
        case Tool::ArcStart:
            ui->actionArcStartPointTool->setChecked(false);
            break;
        case Tool::ArcEnd:
            ui->actionArcEndPointTool->setChecked(false);
            break;
        case Tool::EllipticalArcWithLength:
            ui->actionEllipticalArcWithLengthTool->setChecked(false);
            break;
        case Tool::ParallelCurve:
            ui->actionParallelCurveTool->setChecked(false);
            break;
        case Tool::GraduatedCurve:
            ui->actionGraduatedCurveTool->setChecked(false);
            break;
    }

    QT_WARNING_POP

    // Crash: using CRTL+Z while using line tool.
    // related bug report:
    // https://bitbucket.org/dismine/valentina/issues/454/crash-using-crtl-z-while-using-line-tool
    undoAction->setEnabled(not m_toolOptionsDialogVisible &&
                           VAbstractApplication::VApp()->getUndoStack()->canUndo());
    redoAction->setEnabled(not m_toolOptionsDialogVisible &&
                           VAbstractApplication::VApp()->getUndoStack()->canRedo());
}

//---------------------------------------------------------------------------------------------------------------------
// SetupDrawToolsIcons extracted to mainwindow_icons.cpp

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ArrowTool enable arrow tool.
 */
void MainWindow::ArrowTool(bool checked)
{
    if (checked && m_currentTool != Tool::Arrow)
    {
        qCDebug(vMainWindow, "Arrow tool.");
        CancelTool();
        ui->actionToolSelect->setChecked(true);
        m_comboBoxDraws->setEnabled(true);
        m_currentTool = Tool::Arrow;
        emit EnableItemMove(true);
        emit ItemsSelection(SelectionType::ByMouseRelease);
        VAbstractTool::m_suppressContextMenu = false;

        // Only true for rubber band selection
        emit EnableLabelSelection(true);
        emit EnablePointSelection(false);
        emit EnableLineSelection(false);
        emit EnableArcSelection(false);
        emit EnableElArcSelection(false);
        emit EnableSplineSelection(false);
        emit EnableSplinePathSelection(false);
        emit EnableNodeLabelSelection(true);
        emit EnableNodePointSelection(true);
        emit EnableDetailSelection(true); // Disable when done visualization details
        emit EnableBackgroundImageSelection(true);

        // Hovering
        emit EnableLabelHover(true);
        emit EnablePointHover(true);
        emit EnableLineHover(true);
        emit EnableArcHover(true);
        emit EnableElArcHover(true);
        emit EnableSplineHover(true);
        emit EnableSplinePathHover(true);
        emit EnableNodeLabelHover(true);
        emit EnableNodePointHover(true);
        emit EnableDetailHover(true);
        emit EnableImageBackgroundHover(true);

        emit ShowArcSegmentLabel(false);
        emit ShowElArcSegmentLabel(false);
        emit ShowSplineSegmentLabel(false);
        emit ShowSplinePathSegmentLabel(false);

        ui->view->AllowRubberBand(true);
        ui->view->viewport()->unsetCursor();
        ui->view->viewport()->setCursor(QCursor(Qt::ArrowCursor));
        ui->view->setCurrentCursorShape(); // Hack to fix problem with a cursor
        m_statusLabel->setText(QString());
        ui->view->setShowToolOptions(true);
        qCDebug(vMainWindow, "Enabled arrow tool.");
    }
    else
    {
        ui->actionToolSelect->setChecked(true);
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
            ArrowTool(true);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            EndVisualization();
            break;
        default:
            break;
    }
    QMainWindow::keyPressEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SaveCurrentScene save scene options before set another.
 */
void MainWindow::SaveCurrentScene()
{
    if (VAbstractValApplication::VApp()->GetDrawMode() == Draw::Calculation ||
        VAbstractValApplication::VApp()->GetDrawMode() == Draw::Modeling)
    {
        auto *scene = qobject_cast<VMainGraphicsScene *>(currentScene);
        SCASSERT(scene != nullptr)

        /*Save transform*/
        scene->setTransform(ui->view->transform());
        /*Save scroll bars value for previous scene.*/
        QScrollBar  const*horScrollBar = ui->view->horizontalScrollBar();
        scene->setHorScrollBar(horScrollBar->value());
        QScrollBar  const*verScrollBar = ui->view->verticalScrollBar();
        scene->setVerScrollBar(verScrollBar->value());
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief RestoreCurrentScene restore scene options after change.
 */
void MainWindow::RestoreCurrentScene()
{
    auto *scene = qobject_cast<VMainGraphicsScene *>(currentScene);
    SCASSERT(scene != nullptr)

    /*Set transform for current scene*/
    ui->view->setTransform(scene->transform());
    /*Set value for current scene scroll bar.*/
    QScrollBar *horScrollBar = ui->view->horizontalScrollBar();
    horScrollBar->setValue(scene->getHorScrollBar());
    QScrollBar *verScrollBar = ui->view->verticalScrollBar();
    verScrollBar->setValue(scene->getVerScrollBar());
    ScaleChanged(ui->view->transform().m11());
}

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
        ArrowTool(true);

        const auto resource = QStringLiteral("icon");

        m_leftGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-next-skip.png")));
        m_rightGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-next.png")));

        ui->actionDraw->setChecked(true);
        ui->actionDetails->setChecked(false);
        ui->actionLayout->setChecked(false);
        SaveCurrentScene();

        currentScene = m_sceneDraw;
        ui->view->setScene(currentScene);
        RestoreCurrentScene();

        // Scale-dependent item state (e.g. label visibility) lives in RefreshScale(), not paint().
        // It is not refreshed while a scene is inactive, so re-run the dispatch now that the scene is
        // shown and the view transform is valid; otherwise stale state lingers until the next zoom.
        ScaleChanged(ui->view->transform().m11());

        VAbstractValApplication::VApp()->SetDrawMode(Draw::Calculation);
        m_comboBoxDraws->setCurrentIndex(m_currentDrawIndex); // restore current pattern peace
        m_drawMode = true;

        SetEnableTool(true);
        SetEnableWidgets(true);
        m_drawToolBar->SetMode(Draw::Calculation);

        if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize)
        {
                    }

        ui->dockWidgetLayoutPages->setVisible(false);
        ui->dockWidgetToolOptions->setVisible(m_toolOptionsActive);

        ui->dockWidgetGroups->setWidget(m_groupsWidget);
        ui->dockWidgetGroups->setWindowTitle(tr("Groups of visibility"));
        ui->dockWidgetGroups->setVisible(m_groupsActive);
        ui->dockWidgetGroups->setToolTip(tr("Contains all visibility groups"));

        ui->dockWidgetBackgroundImages->setVisible(m_backgroundImagesActive);

        ui->dockWidgetDependencies->setVisible(m_dependenciesActive);

        actionDockWidgetToolOptions->setEnabled(true);
        actionDockWidgetGroups->setEnabled(true);
        actionDockWidgetBackgroundImages->setEnabled(true);
        actionDockWidgetDependencies->setEnabled(true);

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

        if (m_drawMode)
        {
            m_currentDrawIndex = m_comboBoxDraws->currentIndex(); // save current pattern piece
            m_drawMode = false;
        }
        m_comboBoxDraws->setCurrentIndex(m_comboBoxDraws->count() - 1); // Need to get data about all details

        const auto resource = QStringLiteral("icon");
        m_leftGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-previous.png")));
        m_rightGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-next.png")));

        ui->actionDraw->setChecked(false);
        ui->actionDetails->setChecked(true);
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

        m_detailsWidget->UpdateList();

        qCDebug(vMainWindow, "Show details scene");
        SaveCurrentScene();

        currentScene = m_sceneDetails;
        emit ui->view->itemClicked(nullptr);
        ui->view->setScene(currentScene);
        RestoreCurrentScene();

        // Scale-dependent item state (e.g. label visibility) lives in RefreshScale(), not paint().
        // It is not refreshed while a scene is inactive, so re-run the dispatch now that the scene is
        // shown and the view transform is valid; otherwise stale state lingers until the next zoom.
        ScaleChanged(ui->view->transform().m11());

        VAbstractValApplication::VApp()->SetDrawMode(Draw::Modeling);
        SetEnableTool(true);
        SetEnableWidgets(true);
        m_drawToolBar->SetMode(Draw::Modeling);

        if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize)
        {
                    }

        ui->dockWidgetLayoutPages->setVisible(false);

        ui->dockWidgetGroups->setWidget(m_detailsWidget);
        ui->dockWidgetGroups->setWindowTitle(tr("Details"));
        ui->dockWidgetGroups->setVisible(m_groupsActive);
        ui->dockWidgetGroups->setToolTip(tr("Show which details will go in layout"));

        ui->dockWidgetToolOptions->setVisible(m_toolOptionsActive);
        ui->dockWidgetBackgroundImages->setVisible(false);
        ui->dockWidgetDependencies->setVisible(false);

        actionDockWidgetBackgroundImages->setDisabled(true);
        actionDockWidgetDependencies->setDisabled(true);

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

    if (m_drawMode)
    {
        m_currentDrawIndex = m_comboBoxDraws->currentIndex(); // save current pattern piece
        m_drawMode = false;
    }
    m_comboBoxDraws->setCurrentIndex(m_comboBoxDraws->count() - 1); // Need to get data about all details

    const auto resource = QStringLiteral("icon");
    m_leftGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-previous.png")));
    m_rightGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-previous-skip.png")));

    ui->actionDraw->setChecked(false);
    ui->actionDetails->setChecked(false);
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
        details = SortDetailsForLayout(allDetails);

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

    m_comboBoxDraws->setCurrentIndex(-1); // Hide pattern pieces

    qCDebug(vMainWindow, "Show layout scene");

    SaveCurrentScene();

    try
    {
        listDetails = PrepareDetailsForLayout(details);
    }
    catch (VException &e)
    {
        listDetails.clear();
        QMessageBox::warning(this, tr("Layout mode"),
                             tr("You can't use Layout mode yet.") + QStringLiteral(" \n") + e.ErrorMessage(),
                             QMessageBox::Ok, QMessageBox::Ok);
        VAbstractValApplication::VApp()->GetDrawMode() == Draw::Calculation ? ActionDraw(true) : ActionDetails(true);
        return;
    }

    currentScene = tempSceneLayout;
    emit ui->view->itemClicked(nullptr);
    ui->view->setScene(currentScene);

    VAbstractValApplication::VApp()->SetDrawMode(Draw::Layout);
    SetEnableTool(true);
    SetEnableWidgets(true);
    m_drawToolBar->SetMode(Draw::Layout);

    if (not m_mouseCoordinate.isNull())
    {
        m_mouseCoordinate->setText(QString());
    }

    ui->dockWidgetLayoutPages->setVisible(true);
    ui->dockWidgetToolOptions->setVisible(false);
    ui->dockWidgetGroups->setVisible(false);
    ui->dockWidgetBackgroundImages->setVisible(false);
    ui->dockWidgetDependencies->setVisible(false);

    actionDockWidgetToolOptions->setDisabled(true);
    actionDockWidgetGroups->setDisabled(true);
    actionDockWidgetBackgroundImages->setDisabled(true);
    actionDockWidgetDependencies->setDisabled(true);

    ShowPaper(ui->listWidget->currentRow());

    if (m_layoutSettings->LayoutScenes().isEmpty() || m_layoutSettings->IsLayoutStale())
    {
        ui->actionLayoutSettings->activate(QAction::Trigger);
    }

    m_statusLabel->setText(QString());
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

    QString newFileName = tr("pattern") + QStringLiteral(".val");
    if (not patternPath.isEmpty())
    {
        newFileName = QFileInfo(patternPath).fileName();
    }

    QString const filters(tr("Pattern files") + QStringLiteral("(*.val)"));
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save as"), dir + '/'_L1 + newFileName, filters, nullptr,
                                                    VAbstractApplication::VApp()->NativeFileDialog());

    if (fileName.isEmpty())
    {
        return false;
    }

    QFileInfo const f(fileName);
    if (f.suffix().isEmpty() && f.suffix() != "val"_L1)
    {
        fileName += ".val"_L1;
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

    if (m_curFileFormatVersion < VPatternConverter::PatternMaxVer &&
        not ContinueFormatRewrite(m_curFileFormatVersionStr, VPatternConverter::PatternMaxVerStr))
    {
        return false;
    }

    if (not CheckFilePermissions(VAbstractValApplication::VApp()->GetPatternPath(), this))
    {
        return false;
    }

    QString error;
    bool const result = SavePattern(VAbstractValApplication::VApp()->GetPatternPath(), error);
    if (result)
    {
        QFile::remove(VAbstractValApplication::VApp()->GetPatternPath() + AutosavePrefix());
        m_curFileFormatVersion = VPatternConverter::PatternMaxVer;
        m_curFileFormatVersionStr = VPatternConverter::PatternMaxVerStr;
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
    const QString filter(tr("Pattern files") + QStringLiteral(" (*.val)"));
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
void MainWindow::on_actionOpenPuzzle_triggered()
{
    QStringList arguments;
    if (isNoScaling)
    {
        arguments.append(QStringLiteral("--") + LONG_OPTION_NO_HDPI_SCALING);
    }

    const QString path = VApplication::PuzzleFilePath();
    qDebug("Opening Puzzle: path = %s.", qUtf8Printable(path));
    VApplication::StartDetachedProcess(path, arguments);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::on_actionCreateManualLayout_triggered()
{
    QTemporaryFile rldFile(QDir::tempPath() + "/puzzle.rld.XXXXXX"_L1);
    if (rldFile.open())
    {
        QVector<DetailForLayout> const detailsInLayout = SortDetailsForLayout(pattern->DataPieces());

        if (detailsInLayout.isEmpty())
        {
            QMessageBox::information(this, tr("Layout mode"),
                                     tr("You don't have enough details to export. Please, "
                                        "include at least one detail in layout."),
                                     QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        QVector<VLayoutPiece> listDetails;
        try
        {
            listDetails = PrepareDetailsForLayout(detailsInLayout);
        }
        catch (VException &e)
        {
            QMessageBox::warning(this, tr("Export details"), tr("Can't export details.") + " \n"_L1 + e.ErrorMessage(),
                                 QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        DialogLayoutScale layoutScale(false, this);
        layoutScale.SetXScale(1);
        layoutScale.SetYScale(1);

        if (int const res = layoutScale.exec(); res == QDialog::Rejected)
        {
            return;
        }

        VLayoutExporter exporter;
        exporter.SetFileName(rldFile.fileName());
        exporter.SetXScale(layoutScale.GetXScale());
        exporter.SetYScale(layoutScale.GetYScale());
        exporter.ExportToRLD(listDetails);

        QStringList arguments{"-r", rldFile.fileName()};
        if (isNoScaling)
        {
            arguments.append(QStringLiteral("--") + LONG_OPTION_NO_HDPI_SCALING);
        }

        rldFile.setAutoRemove(false);

        const QString path = VApplication::PuzzleFilePath();
        qDebug("Opening Puzzle: path = %s.", qUtf8Printable(path));
        VApplication::StartDetachedProcess(path, arguments);
    }
    else
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Unable to prepare raw layout data.")));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::on_actionUpdateManualLayout_triggered()
{
    const QString filter(tr("Manual layout files") + " (*.vlt)"_L1);

    VValentinaSettings  const*settings = VAbstractValApplication::VApp()->ValentinaSettings();

    // Use standard path to manual layouts
    const QString path = settings->GetPathManualLayouts();

    const QString filePath = QFileDialog::getOpenFileName(this, tr("Select manual layout"), path, filter, nullptr);

    if (filePath.isEmpty())
    {
        return;
    }

    settings->SetPathManualLayouts(QFileInfo(filePath).absolutePath());

    QTemporaryFile rldFile(QDir::tempPath() + "/puzzle.rld.XXXXXX"_L1);
    rldFile.setAutoRemove(false);
    if (rldFile.open())
    {
        QVector<DetailForLayout> const detailsInLayout = SortDetailsForLayout(pattern->DataPieces());

        if (detailsInLayout.isEmpty())
        {
            QMessageBox::information(this, tr("Layout mode"),
                                     tr("You don't have enough details to export. Please, "
                                        "include at least one detail in layout."),
                                     QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        QVector<VLayoutPiece> listDetails;
        try
        {
            listDetails = PrepareDetailsForLayout(detailsInLayout);
        }
        catch (VException &e)
        {
            QMessageBox::warning(this, tr("Export details"), tr("Can't export details.") + " \n"_L1 + e.ErrorMessage(),
                                 QMessageBox::Ok, QMessageBox::Ok);
            return;
        }

        DialogLayoutScale layoutScale(false, this);
        layoutScale.SetXScale(1);
        layoutScale.SetYScale(1);
        layoutScale.exec();

        VLayoutExporter exporter;
        exporter.SetFileName(rldFile.fileName());
        exporter.SetXScale(layoutScale.GetXScale());
        exporter.SetYScale(layoutScale.GetYScale());
        exporter.ExportToRLD(listDetails);

        QStringList arguments{filePath, "-r", rldFile.fileName()};
        if (isNoScaling)
        {
            arguments.append("--"_L1 + LONG_OPTION_NO_HDPI_SCALING);
        }

        rldFile.setAutoRemove(false);

        const QString puzzlePath = VApplication::PuzzleFilePath();
        qDebug("Opening Puzzle: path = %s.", qUtf8Printable(puzzlePath));
        VApplication::StartDetachedProcess(puzzlePath, arguments);
    }
    else
    {
        qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Unable to prepare raw layout data.")));
    }
}
// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionExportFontCorrections()
{
    // Use standard path to manual layouts
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    const QString dirPath = settings->GetPathFontCorrections();

    bool usedNotExistedDir = false;
    if (QDir const directory(dirPath); not directory.exists())
    {
        usedNotExistedDir = directory.mkpath(QChar('.'));
    }

    auto RemoveUnsuded = qScopeGuard(
        [usedNotExistedDir, dirPath]() -> void
        {
            if (usedNotExistedDir)
            {
                QDir const directory(dirPath);
                directory.rmpath(QChar('.'));
            }
        });


    if (const QString dir = QFileDialog::getExistingDirectory(
                this, tr("Select folder"), dirPath,
                VAbstractApplication::VApp()->NativeFileDialog(QFileDialog::ShowDirsOnly |
                                                               QFileDialog::DontResolveSymlinks));not dir.isEmpty())
    {
        const QFont labelFont = settings->GetLabelFont();
        if (const QString filePath = QStringLiteral("%1/%2.json").arg(dir, labelFont.family());
                QFileInfo::exists(filePath) &&
                QMessageBox::question(this, tr("Export corrections"),
                                      tr("File '%1' already exists. Overwrite it?").arg(filePath),
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }

        VSingleLineOutlineChar const corrector(labelFont);
        corrector.ExportCorrections(dir);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionReloadLabels()
{
    VSingleLineOutlineChar corrector(VApplication::VApp()->ValentinaSettings()->GetLabelFont());
    corrector.ClearCorrectionsCache();

    emit doc->UpdatePatternLabel();
    emit doc->UpdatePatternLabel();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief Handles selecting multiple font files and installing them one by one.
 * Uses QFileDialog with specific filters for common font types.
 */
void MainWindow::ActionInstallSingleLineFont()
{
    QStringList const filters{
        tr("All Single-Line Fonts") + " (*.ttf *.otf *.svg)"_L1,
        tr("TrueType Fonts") + " (*.ttf)"_L1,
        tr("OpenType Fonts") + " (*.otf)"_L1,
        tr("SVG Fonts") + " (*.svg)"_L1,
        tr("All Files") + " (*)"_L1,
    };

    // 1. Open file dialog to select several font files
    QStringList const selectedFiles = QFileDialog::getOpenFileNames(this,
                                                                    tr("Select Font Files to Install"),
                                                                    QDir::homePath(), // Start in a common location
                                                                    filters.join(";;"_L1),
                                                                    nullptr,
                                                                    VAbstractApplication::VApp()->NativeFileDialog());

    if (selectedFiles.isEmpty())
    {
        qDebug() << "Font installation cancelled by user.";
        return;
    }

    // 2. Initialize installer and counters
    VFontInstaller installer(this);

    // Reset the "Yes to All" / "No to All" state for this new batch.
    installer.ResetOverwriteMode();

    int successCount = 0;
    int failCount = 0;

    // 3. Loop through the list of selected files to install each one
    for (const QString &filePath : selectedFiles)
    {
        // Install the font using the FontInstaller class logic
        if (VFontInstaller::InstallError const result = installer.InstallFont(filePath, this);
            result == VFontInstaller::InstallError::NoError)
        {
            successCount++;
            qCInfo(vMainWindow) << "Successfully installed font:" << filePath;
        }
        else if (result != VFontInstaller::InstallError::UserCancelled)
        {
            failCount++;
            qCWarning(vMainWindow) << tr("Failed to install font file: %1\n\nReason: %2")
                                      .arg(QFileInfo(filePath).fileName(), installer.ErrorMessage());
        }
    }

    // 4. Provide final summary to the user
    if (QString const finalMessage = tr("Font Installation Summary:\n\n"
                                        "Successful Installations: %1\n"
                                        "Failed Installations: %2")
            .arg(successCount)
            .arg(failCount); failCount > 0)
    {
        QMessageBox::warning(this, tr("Installation Complete with Errors"), finalMessage);
    }
    else if (successCount > 0)
    {
        QMessageBox::information(this, tr("Installation Successful"), finalMessage);
    }
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
    m_detailsWidget->UpdateList();
    m_backgroundImagesWidget->UpdateImages();
    m_dependenciesWidget->Clear();
    qCDebug(vMainWindow, "Clearing scenes.");
    m_sceneDraw->clear();
    qCDebug(vMainWindow, "Draw scene cleared.");
    m_sceneDraw->SetAcceptDrop(false);
    m_sceneDetails->clear();
    qCDebug(vMainWindow, "Details scene cleared.");
    ArrowTool(true);
    {
        const QSignalBlocker blocker(m_comboBoxDraws);
        m_comboBoxDraws->clear();
        m_comboBoxDraws->setDisabled(true);
    }
    ui->actionDraw->setEnabled(false);
    ui->actionDetails->setEnabled(false);
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
    ui->actionReloadLabels->setEnabled(false);
    ui->actionAutoArrangeLabels->setEnabled(false);
    SetEnableTool(false);
    VAbstractValApplication::VApp()->SetPatternUnits(Unit::Cm);
    VAbstractValApplication::VApp()->SetMeasurementsType(MeasurementsType::Unknown);

#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif
    CleanLayout();
    listDetails.clear(); // don't move to CleanLayout()
    VAbstractApplication::VApp()->getUndoStack()->clear();
    m_toolOptions->ClearPropertyBrowser();
    m_toolOptions->itemClicked(nullptr);
    m_progressBar->setVisible(false);
#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    m_taskbarProgress->setVisible(false);
#endif
    m_statusLabel->setVisible(true);
    ui->actionIncreaseLabelFont->setEnabled(false);
    ui->actionDecreaseLabelFont->setEnabled(false);
    ui->actionOriginalLabelFont->setEnabled(false);
    ui->actionHideLabels->setEnabled(false);
    ui->plainTextEditPatternMessages->clear();
    ui->actionLoadWatermark->setEnabled(false);
    ui->actionRemoveWatermark->setEnabled(false);
    PatternChangesWereSaved(true);
}
// Extracted: //---------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::FullParseFile()
{
    qCDebug(vMainWindow, "Full parsing file");

    if (m_toolOptionsDialogVisible)
    {
        // A full reparse tears down all tools and the VContainer. Any tool options dialog still open
        // now references freed data, and accepting it would crash. Reject it before teardown.
        const QList<DialogTool *> openDialogs = findChildren<DialogTool *>();
        for (DialogTool *dialog : openDialogs)
        {
            dialog->reject();
        }
    }

    m_toolOptions->ClearPropertyBrowser();

    if (not FullParsePattern())
    {
        return;
    }

    QString patternPiece;
    if (m_comboBoxDraws->currentIndex() != -1)
    {
        patternPiece = m_comboBoxDraws->itemText(m_comboBoxDraws->currentIndex());
    }

    {
        const QSignalBlocker blocker(m_comboBoxDraws);
        m_comboBoxDraws->clear();

        QStringList patternPieceNames = doc->PatternBlockMapper()->GetBlockNames();
        patternPieceNames.sort();
        m_comboBoxDraws->addItems(patternPieceNames);

        if (not m_drawMode)
        {
            m_comboBoxDraws->setCurrentIndex(m_comboBoxDraws->count() - 1);
        }
        else
        {
            const qint32 index = m_comboBoxDraws->findText(patternPiece);
            if (index != -1)
            {
                m_comboBoxDraws->setCurrentIndex(index);
            }
        }
    }
    ui->actionPattern_properties->setEnabled(true);

    GlobalChangePP(patternPiece);

    SetEnableTool(m_comboBoxDraws->count() > 0);
    m_detailsWidget->UpdateList();

    VMainGraphicsView::NewSceneRect(m_sceneDraw, VAbstractValApplication::VApp()->getSceneView());
    VMainGraphicsView::NewSceneRect(m_sceneDetails, VAbstractValApplication::VApp()->getSceneView());
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::GlobalChangePP(const QString &patternPiece)
{
    const qint32 index = m_comboBoxDraws->findText(patternPiece);
    if (index == m_comboBoxDraws->currentIndex() && doc->PatternBlockMapper()->GetActive() == patternPiece)
    {
        return;
    }

    try
    {
        if (index != -1)
        { // -1 for not found
            ChangePP(index, false);
            const QSignalBlocker blocker(m_comboBoxDraws);
            m_comboBoxDraws->setCurrentIndex(index);
        }
        else
        {
            ChangePP(0, false);
        }
    }
    catch (VExceptionBadId &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Bad id.")), qUtf8Printable(e.ErrorMessage()),
                   qUtf8Printable(e.DetailedInformation()));
        SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
    catch (const VExceptionEmptyParameter &e)
    {
        qCCritical(vMainWindow, "%s\n\n%s\n\n%s", qUtf8Printable(tr("Error empty parameter.")),
                   qUtf8Printable(e.ErrorMessage()), qUtf8Printable(e.DetailedInformation()));
        SetEnabledGUI(false);
        if (not VApplication::IsGUIMode())
        {
            QCoreApplication::exit(V_EX_NOINPUT);
        }
        return;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::PreviousPatternPiece()
{
    int index = m_comboBoxDraws->currentIndex();

    if (index == -1 || m_comboBoxDraws->count() <= 1)
    {
        return;
    }

    if (index == 0)
    {
        index = m_comboBoxDraws->count() - 1;
    }
    else
    {
        --index;
    }

    m_comboBoxDraws->setCurrentIndex(index);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::NextPatternPiece()
{
    int index = m_comboBoxDraws->currentIndex();

    if (index == -1 || m_comboBoxDraws->count() <= 1)
    {
        return;
    }

    if (index == m_comboBoxDraws->count() - 1)
    {
        index = 0;
    }
    else
    {
        ++index;
    }

    m_comboBoxDraws->setCurrentIndex(index);
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
    ui->actionLayout->setEnabled(enable);
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
    ui->actionReloadLabels->setEnabled(enableOnDetailsStage);
    ui->actionAutoArrangeLabels->setEnabled(enableOnDesignStage);
    ui->actionIncreaseLabelFont->setEnabled(enable);
    ui->actionDecreaseLabelFont->setEnabled(enable);
    ui->actionOriginalLabelFont->setEnabled(enable);
    ui->actionHideLabels->setEnabled(enable);
    ui->actionCreateManualLayout->setEnabled(enableOnPiecesStage);
    ui->actionUpdateManualLayout->setEnabled(enableOnPiecesStage);

    ui->actionLoadWatermark->setEnabled(enable);
    ui->actionRemoveWatermark->setEnabled(enable && not doc->GetWatermarkPath().isEmpty());

    actionDockWidgetToolOptions->setEnabled(enableOnDesignStage);
    actionDockWidgetGroups->setEnabled(enableOnDesignStage);
    actionDockWidgetBackgroundImages->setEnabled(enableOnDrawStage);

    undoAction->setEnabled(enableOnDesignStage && not m_toolOptionsDialogVisible &&
                           VAbstractApplication::VApp()->getUndoStack()->canUndo());
    redoAction->setEnabled(enableOnDesignStage && not m_toolOptionsDialogVisible &&
                           VAbstractApplication::VApp()->getUndoStack()->canRedo());

    // Now we don't want allow user call context menu
    m_sceneDraw->EnableTools();
    ui->view->setEnabled(enable);
    ui->view->setAcceptDrops(enable);
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

        m_curFileFormatVersion = VPatternConverter::PatternMaxVer;
        m_curFileFormatVersionStr = VPatternConverter::PatternMaxVerStr;

        UpdateMeasurementState();


        m_sceneDraw->SetAcceptDrop(true);
    }
    else
    {
        OpenNewValentina();
    }
}

//---------------------------------------------------------------------------------------------------------------------
bool MainWindow::hasOpenPattern() const
{
    return m_comboBoxDraws->count() > 0;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief haveChange enable action save if we have unsaved change.
 */
void MainWindow::PatternChangesWereSaved(bool saved)
{
    if (m_guiEnabled)
    {
        const bool state = doc->IsModified() || !saved;
        setWindowModified(state);
        not m_patternReadOnly ? ui->actionSave->setEnabled(state) : ui->actionSave->setEnabled(false);
        m_layoutSettings->SetLayoutStale(true);
        isNeedAutosave = not saved;
    }
}
// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 



//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowProgress()
{
    if (m_progressBar->isVisible() && m_progressBar->value() + 1 <= m_progressBar->maximum())
    {
        const int newValue = m_progressBar->value() + 1;
        m_progressBar->setValue(newValue);
#if defined(Q_OS_WIN32) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        m_taskbarProgress->setValue(newValue);
#endif
        QCoreApplication::processEvents();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ClearPatternMessages()
{
    ui->plainTextEditPatternMessages->clear();
    if (not m_unreadPatternMessage.isNull())
    {
        m_unreadPatternMessage->setText(QString());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::AskDefaultSettings()
{
    if (VApplication::IsGUIMode())
    {
        VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
        if (not settings->IsLocaleSelected())
        {
            QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            DialogSelectLanguage dialog(this);
            QGuiApplication::restoreOverrideCursor();
            if (dialog.exec() == QDialog::Accepted)
            {
                QString const locale = dialog.Locale();
                settings->SetLocale(locale);
                VAbstractApplication::VApp()->LoadTranslation(locale);
            }
        }

        // VGAnalyticsLib removed — no tracking, no privacy modal
    }
}
// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------

// Extracted to mainwindow_background.cpp: //---------------------------------------------------------------------------------------------------------------------
// Extracted: 



//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionNewDraw_triggered()
{
    qCDebug(vMainWindow, "New PP.");
    QString patternPieceName = tr("Pattern piece %1").arg(m_comboBoxDraws->count() + 1);
    qCDebug(vMainWindow, "Generated PP name: %s", qUtf8Printable(patternPieceName));

    qCDebug(vMainWindow, "PP count %d", m_comboBoxDraws->count());
    bool const ok = PatternPieceName(patternPieceName);
    qCDebug(vMainWindow, "PP name: %s", qUtf8Printable(patternPieceName));
    if (not ok)
    {
        return;
    }

    AddPP(patternPieceName);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionTable_triggered()
{
    // Because of bug on Mac with Qt 5.11 closing this dialog causes a crash. Instead of closing we will keep
    // dialog in memory.
    if (m_dialogTable.isNull())
    {
        m_dialogTable = new DialogIncrements(pattern, doc, this);
        connect(m_dialogTable.data(), &DialogIncrements::UpdateProperties, m_toolOptions,
                &VToolOptionsPropertyBrowser::RefreshOptions);
        m_dialogTable->show();
    }
    else
    {
        m_dialogTable->FullUpdateFromFile();
        m_dialogTable->RestoreAfterClose(); // Redo some moves after close
        m_dialogTable->isVisible() ? m_dialogTable->activateWindow() : m_dialogTable->show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionFinalMeasurements_triggered()
{
    if (m_dialogFMeasurements.isNull())
    {
        m_dialogFMeasurements = new DialogFinalMeasurements(doc, this);
        m_dialogFMeasurements->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_dialogFMeasurements.data(), &DialogFinalMeasurements::finished, this,
                [this](int result)
                {
                    if (result == QDialog::Accepted)
                    {
                        doc->SetFinalMeasurements(m_dialogFMeasurements->FinalMeasurements());
                        emit doc->UpdatePatternLabel();
                    }
                    m_dialogFMeasurements->close();
                });
        m_dialogFMeasurements->show();
    }
    else
    {
        m_dialogFMeasurements->activateWindow();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionShowMainPath_triggered(bool checked)
{
    VAbstractValApplication::VApp()->ValentinaSettings()->SetPieceShowMainPath(checked);
    const QList<quint32> ids = pattern->DataPieces()->keys();
    const bool updateChildren = false;
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    for (const auto &id : ids)
    {
        try
        {
            if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(id)))
            {
                tool->RefreshGeometry(updateChildren);
            }
        }
        catch (VExceptionBadId &)
        {
        }
    }
    QGuiApplication::restoreOverrideCursor();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ActionBoundaryTogetherWithNotches_triggered(bool checked)
{
    VAbstractValApplication::VApp()->ValentinaSettings()->SetBoundaryTogetherWithNotches(checked);
    const QList<quint32> ids = pattern->DataPieces()->keys();
    const bool updateChildren = false;
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    for (const auto &id : ids)
    {
        try
        {
            if (auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(id)))
            {
                tool->RefreshGeometry(updateChildren);
            }
        }
        catch (VExceptionBadId &)
        {
        }
    }
    QGuiApplication::restoreOverrideCursor();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateShortcuts()
{
    if (VAbstractShortcutManager *manager = VAbstractValApplication::VApp()->GetShortcutManager())
    {
        manager->UpdateActionShortcuts(m_shortcutActions);
    }
}
// Extracted: 



//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitDimensionGradation(int index, const MeasurementDimension_p &dimension,
                                        const QPointer<QComboBox> &control)
{
    SCASSERT(control != nullptr)

    qreal current = -1;
    if (control->currentIndex() != -1)
    {
        current = control->currentData().toDouble();
    }

    QSignalBlocker blocker(control);
    control->clear();

    const QVector<qreal> bases = DimensionRestrictedValues(index, dimension);
    const DimesionLabels labels = dimension->Labels();

    if (dimension->Type() == MeasurementDimension::X)
    {
        InitDimensionXGradation(bases, labels, control);
    }
    else if (dimension->Type() == MeasurementDimension::Y || dimension->Type() == MeasurementDimension::W ||
             dimension->Type() == MeasurementDimension::Z)
    {
        InitDimensionYWZGradation(bases, labels, control, dimension->IsBodyMeasurement());
    }

    // Calculate the width of the largest item using QFontMetrics
    QFontMetrics const fontMetrics(control->font());
    int maxWidth = 0;
    for (int i = 0; i < control->count(); ++i)
    {
        int const itemWidth = fontMetrics.horizontalAdvance(control->itemText(i));
        if (itemWidth > maxWidth)
        {
            maxWidth = itemWidth;
        }
    }

    // Set the minimum width of the view to the largest item width
    control->view()->setMinimumWidth(maxWidth);

    // after initialization the current index is 0. The signal for changing the index will not be triggered if not make
    // it invalid first
    control->setCurrentIndex(-1);

    int const i = control->findData(current);
    if (i != -1)
    {
        control->setCurrentIndex(i);
        blocker.unblock();
    }
    else
    {
        blocker.unblock();
        control->setCurrentIndex(0);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitDimensionXGradation(const QVector<qreal> &bases, const DimesionLabels &labels,
                                         const QPointer<QComboBox> &control)
{
    const QString unit = UnitsToStr(VAbstractValApplication::VApp()->MeasurementsUnits(), true);

    for (auto base : bases)
    {
        if (VFuzzyContains(labels, base) && not VFuzzyValue(labels, base).isEmpty())
        {
            control->addItem(VFuzzyValue(labels, base), base);
        }
        else
        {
            control->addItem(QStringLiteral("%1 %2").arg(base).arg(unit), base);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitDimensionYWZGradation(const QVector<qreal> &bases, const DimesionLabels &labels,
                                           const QPointer<QComboBox> &control, bool bodyMeasurement)
{
    const bool fc = m_m->IsFullCircumference();
    const QString unit = UnitsToStr(VAbstractValApplication::VApp()->MeasurementsUnits(), true);

    for (auto base : bases)
    {
        if (VFuzzyContains(labels, base) && not VFuzzyValue(labels, base).isEmpty())
        {
            control->addItem(VFuzzyValue(labels, base), base);
        }
        else
        {
            if (bodyMeasurement)
            {
                control->addItem(QStringLiteral("%1 %2").arg(fc ? base * 2 : base).arg(unit), base);
            }
            else
            {
                control->addItem(QString::number(base), base);
            }
        }
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
    }

    QT_WARNING_POP

    // This check helps to find missed tools
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 64, "Not all tools were handled.");

    // Drawing Tools
    ui->actionEndLineTool->setEnabled(drawTools);
    ui->actionLineTool->setEnabled(drawTools);
    ui->actionAlongLineTool->setEnabled(drawTools);
    ui->actionShoulderPointTool->setEnabled(drawTools);
    ui->actionNormalTool->setEnabled(drawTools);
    ui->actionBisectorTool->setEnabled(drawTools);
    ui->actionLineIntersectTool->setEnabled(drawTools);
    ui->actionSplineTool->setEnabled(drawTools);
    ui->actionCubicBezierTool->setEnabled(drawTools);
    ui->actionArcTool->setEnabled(drawTools);
    ui->actionSplinePathTool->setEnabled(drawTools);
    ui->actionCubicBezierPathTool->setEnabled(drawTools);
    ui->actionPointOfContactTool->setEnabled(drawTools);
    ui->actionNewDetailTool->setEnabled(drawTools);
    ui->actionInternalPathTool->setEnabled(drawTools);
    ui->actionHeightTool->setEnabled(drawTools);
    ui->actionTriangleTool->setEnabled(drawTools);
    ui->actionPointOfIntersectionTool->setEnabled(drawTools);
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
    ui->actionRotationTool->setEnabled(drawTools);
    ui->actionFlippingByLineTool->setEnabled(drawTools);
    ui->actionFlippingByAxisTool->setEnabled(drawTools);
    ui->actionMoveTool->setEnabled(drawTools);
    ui->actionMidpointTool->setEnabled(drawTools);
    ui->actionEllipticalArcTool->setEnabled(drawTools);
    ui->actionPinTool->setEnabled(drawTools);
    ui->actionInsertNodeTool->setEnabled(drawTools);
    ui->actionPlaceLabelTool->setEnabled(drawTools);
    ui->actionExportDraw->setEnabled(drawTools);
    ui->actionArcStartPointTool->setEnabled(drawTools);
    ui->actionArcEndPointTool->setEnabled(drawTools);
    ui->actionEllipticalArcWithLengthTool->setEnabled(drawTools);
    ui->actionParallelCurveTool->setEnabled(drawTools);
    ui->actionGraduatedCurveTool->setEnabled(drawTools);

    ui->actionLast_tool->setEnabled(drawTools);

    ui->actionToolSelect->setEnabled(drawTools || modelingTools);
    ui->actionToolSelect->setChecked(drawTools || modelingTools);

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
/**
 * @brief MinimumScrollBar set scroll bar to minimum.
 */
void MainWindow::MinimumScrollBar()
{
    QScrollBar *horScrollBar = ui->view->horizontalScrollBar();
    horScrollBar->setValue(horScrollBar->minimum());
    QScrollBar *verScrollBar = ui->view->verticalScrollBar();
    verScrollBar->setValue(verScrollBar->minimum());
}
// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------

// Extracted: //---------------------------------------------------------------------------------------------------------------------


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
    QUndoStack  const*undoStack = VAbstractApplication::VApp()->getUndoStack();
    undoAction = undoStack->createUndoAction(this, tr("&Undo"));
    connect(undoAction, &QAction::triggered, m_toolOptions, &VToolOptionsPropertyBrowser::RefreshOptions);
    m_shortcutActions.insert(VShortcutAction::Undo, undoAction);
    undoAction->setIcon(FromTheme(VThemeIcon::EditUndo));
    ui->menuPatternPiece->insertAction(ui->actionLast_tool, undoAction);

    connect(undoStack, &QUndoStack::canUndoChanged, this, [this]()
    {
        if (!m_guiEnabled)
        {
            undoAction->setDisabled(true);
        }
    });

    redoAction = undoStack->createRedoAction(this, tr("&Redo"));
    connect(redoAction, &QAction::triggered, m_toolOptions, &VToolOptionsPropertyBrowser::RefreshOptions);
    m_shortcutActions.insert(VShortcutAction::Redo, redoAction);
    redoAction->setIcon(FromTheme(VThemeIcon::EditRedo));
    ui->menuPatternPiece->insertAction(ui->actionLast_tool, redoAction);

    connect(undoStack, &QUndoStack::canRedoChanged, this, [this]()
    {
        if (!m_guiEnabled)
        {
            redoAction->setDisabled(true);
        }
    });


    m_separatorAct = new QAction(this);
    m_separatorAct->setSeparator(true);
    ui->menuFile->insertAction(ui->actionPattern_properties, m_separatorAct);

    AddDocks();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::LastUsedTool()
{
    // This check helps to find missed tools in the switch
    Q_STATIC_ASSERT_X(static_cast<int>(Tool::LAST_ONE_DO_NOT_USE) == 64, "Not all tools were handled.");

    if (m_currentTool == m_lastUsedTool)
    {
        return;
    }

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (m_lastUsedTool)
    {
        case Tool::Arrow:
            ui->actionToolSelect->setChecked(true);
            ArrowTool(true);
            break;
        case Tool::BasePoint:
        case Tool::SinglePoint:
        case Tool::DoublePoint:
        case Tool::LinePoint:
        case Tool::AbstractSpline:
        case Tool::Cut:
        case Tool::LAST_ONE_DO_NOT_USE:
        case Tool::NodePoint:
        case Tool::NodeArc:
        case Tool::NodeElArc:
        case Tool::NodeSpline:
        case Tool::NodeSplinePath:
        case Tool::BackgroundImage:
        case Tool::BackgroundImageControls:
        case Tool::BackgroundPixmapImage:
        case Tool::BackgroundSVGImage:
            Q_UNREACHABLE(); //-V501
            // Nothing to do here because we can't create this tool from main window.
            break;
        case Tool::EndLine:
            ui->actionEndLineTool->setChecked(true);
            ToolEndLine(true);
            break;
        case Tool::Line:
            ui->actionLineTool->setChecked(true);
            ToolLine(true);
            break;
        case Tool::AlongLine:
            ui->actionAlongLineTool->setChecked(true);
            ToolAlongLine(true);
            break;
        case Tool::Midpoint:
            ui->actionMidpointTool->setChecked(true);
            ToolMidpoint(true);
            break;
        case Tool::ShoulderPoint:
            ui->actionShoulderPointTool->setChecked(true);
            ToolShoulderPoint(true);
            break;
        case Tool::Normal:
            ui->actionNormalTool->setChecked(true);
            ToolNormal(true);
            break;
        case Tool::Bisector:
            ui->actionBisectorTool->setChecked(true);
            ToolBisector(true);
            break;
        case Tool::LineIntersect:
            ui->actionLineIntersectTool->setChecked(true);
            ToolLineIntersect(true);
            break;
        case Tool::Spline:
            ui->actionSplineTool->setChecked(true);
            ToolSpline(true);
            break;
        case Tool::CubicBezier:
            ui->actionCubicBezierTool->setChecked(true);
            ToolCubicBezier(true);
            break;
        case Tool::Arc:
            ui->actionArcTool->setChecked(true);
            ToolArc(true);
            break;
        case Tool::SplinePath:
            ui->actionSplinePathTool->setChecked(true);
            ToolSplinePath(true);
            break;
        case Tool::CubicBezierPath:
            ui->actionCubicBezierPathTool->setChecked(true);
            ToolCubicBezierPath(true);
            break;
        case Tool::PointOfContact:
            ui->actionPointOfContactTool->setChecked(true);
            ToolPointOfContact(true);
            break;
        case Tool::Piece:
            ui->actionNewDetailTool->setChecked(true);
            ToolDetail(true);
            break;
        case Tool::PiecePath:
            ui->actionInternalPathTool->setChecked(true);
            ToolPiecePath(true);
            break;
        case Tool::Height:
            ui->actionHeightTool->setChecked(true);
            ToolHeight(true);
            break;
        case Tool::Triangle:
            ui->actionTriangleTool->setChecked(true);
            ToolTriangle(true);
            break;
        case Tool::PointOfIntersection:
            ui->actionPointOfIntersectionTool->setChecked(true);
            ToolPointOfIntersection(true);
            break;
        case Tool::PointOfIntersectionArcs:
            ui->actionPointOfIntersectionArcsTool->setChecked(true);
            ToolPointOfIntersectionArcs(true);
            break;
        case Tool::CutSpline:
            ui->actionSplineCutPointTool->setChecked(true);
            ToolCutSpline(true);
            break;
        case Tool::CutSplinePath:
            ui->actionSplinePathCutPointTool->setChecked(true);
            ToolCutSplinePath(true);
            break;
        case Tool::UnionDetails:
            ui->actionUnionDetailsTool->setChecked(true);
            ToolUnionDetails(true);
            break;
        case Tool::DuplicateDetail:
            ui->actionDuplicateDetailTool->setChecked(true);
            ToolDuplicateDetail(true);
            break;
        case Tool::CutArc:
            ui->actionArcCutPointTool->setChecked(true);
            ToolCutArc(true);
            break;
        case Tool::LineIntersectAxis:
            ui->actionLineIntersectAxisTool->setChecked(true);
            ToolLineIntersectAxis(true);
            break;
        case Tool::CurveIntersectAxis:
            ui->actionCurveIntersectAxisTool->setChecked(true);
            ToolCurveIntersectAxis(true);
            break;
        case Tool::ArcIntersectAxis:
            ui->actionArcIntersectAxisTool->setChecked(true);
            ToolArcIntersectAxis(true);
            break;
        case Tool::PointOfIntersectionCircles:
            ui->actionPointOfIntersectionCirclesTool->setChecked(true);
            ToolPointOfIntersectionCircles(true);
            break;
        case Tool::PointOfIntersectionCurves:
            ui->actionIntersectionCurvesTool->setChecked(true);
            ToolPointOfIntersectionCurves(true);
            break;
        case Tool::PointFromCircleAndTangent:
            ui->actionPointFromCircleAndTangentTool->setChecked(true);
            ToolPointFromCircleAndTangent(true);
            break;
        case Tool::PointFromArcAndTangent:
            ui->actionPointFromArcAndTangentTool->setChecked(true);
            ToolPointFromArcAndTangent(true);
            break;
        case Tool::ArcWithLength:
            ui->actionArcWithLengthTool->setChecked(true);
            ToolArcWithLength(true);
            break;
        case Tool::TrueDarts:
            ui->actionTrueDartsTool->setChecked(true);
            ToolTrueDarts(true);
            break;
        case Tool::Group:
            ui->actionGroupTool->setChecked(true);
            ToolGroup(true);
            break;
        case Tool::Rotation:
            ui->actionRotationTool->setChecked(true);
            ToolRotation(true);
            break;
        case Tool::FlippingByLine:
            ui->actionFlippingByLineTool->setChecked(true);
            ToolFlippingByLine(true);
            break;
        case Tool::FlippingByAxis:
            ui->actionFlippingByAxisTool->setChecked(true);
            ToolFlippingByAxis(true);
            break;
        case Tool::Move:
            ui->actionMoveTool->setChecked(true);
            ToolMove(true);
            break;
        case Tool::EllipticalArc:
            ui->actionEllipticalArcTool->setChecked(true);
            ToolEllipticalArc(true);
            break;
        case Tool::Pin:
            ui->actionPinTool->setChecked(true);
            ToolPin(true);
            break;
        case Tool::InsertNode:
            ui->actionInsertNodeTool->setChecked(true);
            ToolInsertNode(true);
            break;
        case Tool::PlaceLabel:
            ui->actionPlaceLabelTool->setChecked(true);
            ToolPlaceLabel(true);
            break;
        case Tool::ArcStart:
            ui->actionArcStartPointTool->setChecked(true);
            ToolArcStart(true);
            break;
        case Tool::ArcEnd:
            ui->actionArcEndPointTool->setChecked(true);
            ToolArcEnd(true);
            break;
        case Tool::EllipticalArcWithLength:
            ui->actionEllipticalArcWithLengthTool->setChecked(true);
            ToolEllipticalArcWithLength(true);
            break;
        case Tool::ParallelCurve:
            ui->actionParallelCurveTool->setChecked(true);
            ToolParallelCurve(true);
            break;
        case Tool::GraduatedCurve:
            ui->actionGraduatedCurveTool->setChecked(true);
            ToolGraduatedCurve(true);
            break;
    }

    QT_WARNING_POP
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::AddDocks()
{
    ui->menuWindow->addSeparator();

    // Add dock
    actionDockWidgetToolOptions = ui->dockWidgetToolOptions->toggleViewAction();
    connect(actionDockWidgetToolOptions, &QAction::triggered, this,
            [this](bool checked) { m_toolOptionsActive = checked; });
    ui->menuWindow->addAction(actionDockWidgetToolOptions);

    actionDockWidgetGroups = ui->dockWidgetGroups->toggleViewAction();
    connect(actionDockWidgetGroups, &QAction::triggered, this, [this](bool checked) { m_groupsActive = checked; });
    ui->menuWindow->addAction(actionDockWidgetGroups);

    QAction *action = ui->dockWidgetMessages->toggleViewAction();
    connect(action, &QAction::triggered, this, [this](bool checked) { m_patternMessagesActive = checked; });
    ui->menuWindow->addAction(action);

    actionDockWidgetBackgroundImages = ui->dockWidgetBackgroundImages->toggleViewAction();
    connect(actionDockWidgetBackgroundImages, &QAction::triggered, this,
            [this](bool checked) -> void { m_backgroundImagesActive = checked; });
    ui->menuWindow->addAction(actionDockWidgetBackgroundImages);

    actionDockWidgetDependencies = ui->dockWidgetDependencies->toggleViewAction();
    connect(actionDockWidgetDependencies, &QAction::triggered, this,
            [this](bool checked) -> void { m_dependenciesActive = checked; });
    ui->menuWindow->addAction(actionDockWidgetDependencies);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitDocksContain()
{
    qCDebug(vMainWindow, "Initialization property browser.");
    m_toolOptions = new VToolOptionsPropertyBrowser(ui->dockWidgetToolOptions);

    connect(ui->view, &VMainGraphicsView::itemClicked, m_toolOptions, &VToolOptionsPropertyBrowser::itemClicked);
    connect(doc, &VPattern::FullUpdateFromFile, m_toolOptions, &VToolOptionsPropertyBrowser::UpdateOptions,
            Qt::QueuedConnection);

    qCDebug(vMainWindow, "Initialization groups dock.");
    m_groupsWidget = new VWidgetGroups(doc, this);
    ui->dockWidgetGroups->setWidget(m_groupsWidget);
    connect(doc, &VAbstractPattern::UpdateGroups, this, &MainWindow::UpdateVisibilityGroups);

    m_detailsWidget = new VWidgetDetails(pattern, doc, this);
    connect(doc, &VPattern::FullUpdateFromFile, m_detailsWidget, &VWidgetDetails::UpdateList);
    connect(doc, &VPattern::UpdateInLayoutList, m_detailsWidget, &VWidgetDetails::UpdateList);
    connect(doc, &VPattern::ShowDetail, m_detailsWidget, &VWidgetDetails::SelectDetail);
    connect(m_detailsWidget, &VWidgetDetails::Highlight, m_sceneDetails, &VMainGraphicsScene::HighlightItem);
    m_detailsWidget->setVisible(false);

    m_backgroundImagesWidget = new VWidgetBackgroundImages(doc, this);
    ui->dockWidgetBackgroundImages->setWidget(m_backgroundImagesWidget);
    connect(m_backgroundImagesWidget, &VWidgetBackgroundImages::DeleteImage, this, &MainWindow::RemoveBackgroundImage);

    m_dependenciesWidget = new VWidgetDependencies(doc, this);
    connect(doc, &VPattern::FullUpdateFromFile, m_dependenciesWidget, &VWidgetDependencies::UpdateDependencies,
            Qt::QueuedConnection);
    connect(doc, &VPattern::PatternDependencyGraphCompleted, m_dependenciesWidget,
            &VWidgetDependencies::UpdateDependencies, Qt::QueuedConnection);
    // Extract the tool ID synchronously while the item is alive, then deliver it
    // via a queued call so ShowDependency does not block the UI thread.
    // Passing a raw QGraphicsItem* through a queued connection is unsafe because
    // the item can be destroyed before the slot fires.
    connect(ui->view, &VMainGraphicsView::itemClicked, this,
            [this](QGraphicsItem *item) -> void
            {
                const vidtype id = VWidgetDependencies::ItemToId(item);
                QMetaObject::invokeMethod(m_dependenciesWidget, [widget = m_dependenciesWidget, id]()-> void
                                          { widget->ShowDependency(id); }, Qt::QueuedConnection);
            });
    connect(m_dependenciesWidget, &VWidgetDependencies::ShowProperties, m_toolOptions,
            &VToolOptionsPropertyBrowser::itemClicked, Qt::QueuedConnection);
    connect(m_dependenciesWidget, &VWidgetDependencies::ShowTool, ui->view, &VMainGraphicsView::EnsureToolVisible,
            Qt::QueuedConnection);
    ui->dockWidgetDependencies->setWidget(m_dependenciesWidget);
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
void MainWindow::CreateActions()
{
    ui->setupUi(this);

    connect(ui->actionArrowTool, &QAction::triggered, this, &MainWindow::ArrowTool);
    connect(ui->actionDraw, &QAction::triggered, this, &MainWindow::ActionDraw);
    connect(ui->actionDetails, &QAction::triggered, this, &MainWindow::ActionDetails);
    connect(ui->actionLayout, &QAction::triggered, this, &MainWindow::ActionLayout);
    connect(ui->actionExportRecipe, &QAction::triggered, this, &MainWindow::ActionExportRecipe_triggered);
    connect(ui->actionNewDraw, &QAction::triggered, this, &MainWindow::ActionNewDraw_triggered);
    connect(ui->actionExportFinalMeasurementsToCSV, &QAction::triggered, this, &MainWindow::ExportFMeasurementsToCSV);
    connect(ui->actionTable, &QAction::triggered, this, &MainWindow::ActionTable_triggered);
    connect(ui->actionFinalMeasurements, &QAction::triggered, this, &MainWindow::ActionFinalMeasurements_triggered);

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

    ui->actionShowSegmentLabels->setChecked(
        VAbstractValApplication::VApp()->ValentinaSettings()->IsShowSegmentLabels());
    connect(ui->actionShowSegmentLabels, &QAction::triggered, this,
            [this](bool checked) -> void
            {
                VAbstractValApplication::VApp()->ValentinaSettings()->SetShowSegmentLabels(checked);
                if (!checked)
                {
                    emit ShowArcSegmentLabel(false);
                    emit ShowElArcSegmentLabel(false);
                    emit ShowSplineSegmentLabel(false);
                    emit ShowSplinePathSegmentLabel(false);
                }
                const bool detailsMode =
                    VAbstractValApplication::VApp()->ValentinaSettings()->IsShowCurveDetails();
                m_sceneDraw->EnableDetailsMode(detailsMode);
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
auto MainWindow::PatternPieceName(QString &name) -> bool
{
    QScopedPointer<QInputDialog> const dlg(new QInputDialog(this));
    dlg->setInputMode(QInputDialog::TextInput);
    dlg->setLabelText(tr("Pattern piece:"));
    dlg->setTextEchoMode(QLineEdit::Normal);
    dlg->setWindowTitle(tr("Enter a new label for the pattern piece."));
    dlg->resize(300, 100);
    dlg->setTextValue(name);
    QString nameDraw;
    while (true)
    {
        const bool bOk = dlg->exec();
        nameDraw = dlg->textValue();
        if (not bOk)
        {
            return false;
        }

        if (nameDraw.isEmpty())
        {
            continue;
        }

        if (m_comboBoxDraws->findText(nameDraw) == -1)
        {
            name = nameDraw;
            break; // unique name
        }
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    CancelTool();

    delete doc;
    delete ui;
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

    QFuture<VPatternConverter *> const futureConverter = QtConcurrent::run(
        [fileName]()
        {
            auto converter = std::make_unique<VPatternConverter>(fileName);
            return converter.release();
        });

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
        const unsigned currentFormatVersion = VDomDocument::GetFormatVersion(doc->GetFormatVersionStr());
        if (currentFormatVersion != VPatternConverter::PatternMaxVer)
        { // Because we rely on the fact that we know where is path to measurements optimization available only for
          // the latest format version
            QScopedPointer<VPatternConverter> const converter(futureConverter.result());
            m_curFileFormatVersion = converter->GetCurrentFormatVersion();
            m_curFileFormatVersionStr = converter->GetFormatVersionStr();
            doc->setXMLContent(converter->Convert());

            // VGAnalyticsLib removed — no tracking
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

        if (currentFormatVersion == VPatternConverter::PatternMaxVer)
        {
            // Real read
            QScopedPointer<VPatternConverter> const converter(futureConverter.result());
            m_curFileFormatVersion = converter->GetCurrentFormatVersion();
            m_curFileFormatVersionStr = converter->GetFormatVersionStr();
            doc->setXMLContent(converter->Convert());
            if (!customMeasureFile.isEmpty())
            {
                doc->SetMPath(RelativeMPath(fileName, customMeasureFile));
            }
            else if (not path.isEmpty() && fixedMPath != path)
            {
                doc->SetMPath(RelativeMPath(fileName, fixedMPath));
            }
            VAbstractValApplication::VApp()->SetPatternUnits(doc->Units());
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
void MainWindow::ToolBarStyles()
{
    MainWindow::ToolBarStyle(ui->topBar);
    ui->topBar->setMovable(false);
    ui->topBar->toggleViewAction()->setVisible(false);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolboxIconSize()
{
    auto SetIconSize = [](QToolBar *bar)
    {
        VCommonSettings  const*settings = VAbstractApplication::VApp()->Settings();
        QSize const size = settings->GetToolboxIconSizeSmall() ? QSize(24, 24) : QSize(32, 32);
        bar->setIconSize(size);
    };

    SetIconSize(ui->topBar);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowPaper(int index)
{
    if (index < 0 || index >= m_layoutSettings->LayoutScenes().size())
    {
        ui->view->setScene(tempSceneLayout);
    }
    else
    {
        ui->view->setScene(m_layoutSettings->LayoutScenes().at(index));
    }

    ui->view->fitInView(ui->view->scene()->sceneRect(), Qt::KeepAspectRatio);
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::Preferences()
{
    // Calling constructor of the dialog take some time. Because of this user have time to call the dialog twice.
    static QPointer<DialogPreferences> guard; // Prevent any second run
    if (guard.isNull())
    {
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        auto *preferences = new DialogPreferences(this);
        // QScopedPointer needs to be sure any exception will never block guard
        QScopedPointer<DialogPreferences> const dlg(preferences);
        guard = preferences;
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, &MainWindow::WindowsLocale); // Must be first
        connect(dlg.data(), &DialogPreferences::UpdateProperties, m_toolOptions,
                &VToolOptionsPropertyBrowser::RefreshOptions);
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, &MainWindow::ToolBarStyles);
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, &MainWindow::ToolboxIconSize);
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, [this]() { emit doc->FullUpdateFromFile(); });
        connect(dlg.data(), &DialogPreferences::UpdateProperties, this, [this]() { emit doc->CheckLayout(); });
        connect(dlg.data(), &DialogPreferences::UpdateProperties, ui->view,
                &VMainGraphicsView::ResetScrollingAnimation);
        QGuiApplication::restoreOverrideCursor();

        if (guard->exec() == QDialog::Accepted)
        {
            InitAutoSave();
        }
    }
}


// Extracted: 


// Extracted: 



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
// Extracted: 



//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ChangePP(int index, bool zoomBestFit)
{
    if (index != -1)
    {
        doc->PatternBlockMapper()->SetActive(m_comboBoxDraws->itemText(index));
        doc->setCurrentData();
        if (m_drawMode && zoomBestFit)
        {
            ZoomFitBestCurrent();
        }
        m_toolOptions->itemClicked(nullptr); // hide options for tool in previous pattern piece
        m_groupsWidget->UpdateGroups();
        m_dependenciesWidget->UpdateDependencies();
    }
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
/**
 * @brief ZoomFirstShow Fit scene size to best size for first show
 */
void MainWindow::ZoomFirstShow()
{
    /* If don't call ZoomFitBest() twice, after first scaling or moving pattern piece, scene change coordinate and whole
     * pattern will be moved. Looks very ugly. It is best solution that i have now.
     */

    if (not pattern->DataPieces()->empty())
    {
        ActionDetails(true);
        ui->view->ZoomFitBest();
    }

    if (not ui->actionDraw->isChecked())
    {
        ActionDraw(true);
    }
    ZoomFitBestCurrent();

    VMainGraphicsView::NewSceneRect(m_sceneDraw, ui->view);
    VMainGraphicsView::NewSceneRect(m_sceneDetails, ui->view);

    if (not pattern->DataPieces()->empty())
    {
        ActionDetails(true);
        ui->view->ZoomFitBest();
    }

    if (not ui->actionDraw->isChecked())
    {
        ActionDraw(true);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::DoExport(const VCommandLinePtr &expParams) -> bool
{
    QVector<DetailForLayout> details;
    if (not VAbstractValApplication::VApp()->getOpeningPattern())
    {
        const QHash<quint32, VPiece> *allDetails = pattern->DataPieces();
        if (allDetails->isEmpty())
        {
            qCCritical(vMainWindow, "%s", qUtf8Printable(tr("You can't export empty scene.")));
            QCoreApplication::exit(V_EX_DATAERR);
            return false;
        }

        details = SortDetailsForLayout(allDetails, expParams->OptExportSuchDetails());

        if (details.isEmpty())
        {
            qCCritical(vMainWindow, "%s",
                       qUtf8Printable(tr("You can't export empty scene. Please, "
                                         "include at least one detail in layout.")));
            QCoreApplication::exit(V_EX_DATAERR);
            return false;
        }
    }
    listDetails = PrepareDetailsForLayout(details);

    if (const bool exportOnlyDetails = expParams->IsExportOnlyDetails(); exportOnlyDetails)
    {
        try
        {
            m_dialogSaveLayout = QSharedPointer<DialogSaveLayout>(
                new DialogSaveLayout(1, Draw::Modeling, expParams->OptBaseName(), this));
            m_dialogSaveLayout->SetDestinationPath(expParams->OptDestinationPath());
            m_dialogSaveLayout->SelectFormat(static_cast<LayoutExportFormats>(expParams->OptExportType()));
            m_dialogSaveLayout->SetBinaryDXFFormat(expParams->IsBinaryDXF());
            m_dialogSaveLayout->SetDxfCompatibility(
                static_cast<DXFApparelCompatibility>(expParams->DXFApparelCompatibilityType()));
            m_dialogSaveLayout->SetShowGrainline(!expParams->IsNoGrainline());
            m_dialogSaveLayout->SetTextAsPaths(expParams->IsTextAsPaths());
            m_dialogSaveLayout->SetXScale(expParams->ExportXScale());
            m_dialogSaveLayout->SetYScale(expParams->ExportYScale());

            if (static_cast<LayoutExportFormats>(expParams->OptExportType()) == LayoutExportFormats::PDFTiled)
            {
                m_dialogSaveLayout->SetTiledExportMode(true);
                m_dialogSaveLayout->SetTiledMargins(expParams->TiledPageMargins());
                m_dialogSaveLayout->SetTiledPageFormat(expParams->OptTiledPaperSize());
                m_dialogSaveLayout->SetTiledPageOrientation(expParams->OptTiledPageOrientation());
            }

            ExportData(listDetails);
            m_dialogSaveLayout.clear();
        }
        catch (const VException &e)
        {
            m_dialogSaveLayout.clear();
            qCCritical(vMainWindow, "%s\n\n%s", qUtf8Printable(tr("Export error.")), qUtf8Printable(e.ErrorMessage()));
            QCoreApplication::exit(V_EX_DATAERR);
            return false;
        }
    }
    else
    {
        auto settings = expParams->DefaultGenerator();
        settings->SetTextAsPaths(expParams->IsTextAsPaths());

        if (GenerateLayout(*settings.get()))
        {
            try
            {
                m_dialogSaveLayout = QSharedPointer<DialogSaveLayout>(
                    new DialogSaveLayout(static_cast<int>(m_layoutSettings->LayoutScenes().size()), Draw::Layout,
                                         expParams->OptBaseName(), this));
                m_dialogSaveLayout->SetDestinationPath(expParams->OptDestinationPath());
                m_dialogSaveLayout->SelectFormat(static_cast<LayoutExportFormats>(expParams->OptExportType()));
                m_dialogSaveLayout->SetBinaryDXFFormat(expParams->IsBinaryDXF());
                m_dialogSaveLayout->SetDxfCompatibility(
                    static_cast<DXFApparelCompatibility>(expParams->DXFApparelCompatibilityType()));
                m_dialogSaveLayout->SetShowGrainline(!expParams->IsNoGrainline());
                m_dialogSaveLayout->SetXScale(expParams->ExportXScale());
                m_dialogSaveLayout->SetYScale(expParams->ExportYScale());

                if (static_cast<LayoutExportFormats>(expParams->OptExportType()) == LayoutExportFormats::PDFTiled)
                {
                    m_dialogSaveLayout->SetTiledExportMode(true);
                    m_dialogSaveLayout->SetTiledMargins(expParams->TiledPageMargins());
                    m_dialogSaveLayout->SetTiledPageFormat(expParams->OptTiledPaperSize());
                    m_dialogSaveLayout->SetTiledPageOrientation(expParams->OptTiledPageOrientation());
                }

                ExportData(listDetails);
                m_dialogSaveLayout.clear();
            }
            catch (const VException &e)
            {
                m_dialogSaveLayout.clear();
                qCCritical(vMainWindow, "%s\n\n%s", qUtf8Printable(tr("Export error.")),
                           qUtf8Printable(e.ErrorMessage()));
                QCoreApplication::exit(V_EX_DATAERR);
                return false;
            }
        }
        else
        {
            QCoreApplication::exit(V_EX_DATAERR);
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DoFMExport process export final measurements
 * @param expParams command line options
 * @return true if succesfull
 */
auto MainWindow::DoFMExport(const VCommandLinePtr &expParams) -> bool
{
    QString filePath = expParams->OptExportFMTo();

    if (filePath.isEmpty())
    {
        qCCritical(vMainWindow, "%s\n\n%s", qUtf8Printable(tr("Export final measurements error.")),
                   qUtf8Printable(tr("Destination path is empty.")));
        QCoreApplication::exit(V_EX_DATAERR);
        return false;
    }

    if (QFileInfo const info(filePath); info.isRelative())
    {
        filePath = QDir::currentPath() + '/'_L1 + filePath;
    }

    const QString codecName = expParams->OptCSVCodecName();
    int mib = QTextCodec::codecForLocale()->mibEnum();
    if (not codecName.isEmpty())
    {
        if (QTextCodec  const*codec = QTextCodec::codecForName(codecName.toLatin1()))
        {
            mib = codec->mibEnum();
        }
    }

    QChar separator = expParams->OptCSVSeparator();
    if (separator.isNull())
    {
        separator = VCommonSettings::GetDefCSVSeparator();
    }

    return ExportFMeasurementsToCSVData(filePath, expParams->IsCSVWithHeader(), mib, separator);
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::SetDimensionA(int value) -> bool
{
    if (not VApplication::IsGUIMode())
    {
        if (this->isWindowModified() || not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
        {
            if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize)
            {
                const qint32 index = m_dimensionA ? m_dimensionA->findData(value) : -1;
                if (index != -1)
                {
                    m_dimensionA->setCurrentIndex(index);
                }
                else
                {
                    qCCritical(
                        vMainWindow, "%s",
                        qUtf8Printable(tr("Not supported dimension A value '%1' for this pattern file.").arg(value)));
                    return false;
                }
            }
            else
            {
                qCCritical(vMainWindow, "%s",
                           qUtf8Printable(tr("Couldn't set dimension A. Need a file with multisize measurements.")));
                return false;
            }
        }
        else
        {
            qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Couldn't set dimension A. File wasn't opened.")));
            return false;
        }
    }
    else
    {
        qCWarning(vMainWindow, "%s", qUtf8Printable(tr("The method %1 does nothing in GUI mode").arg(Q_FUNC_INFO)));
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::SetDimensionB(int value) -> bool
{
    if (not VApplication::IsGUIMode())
    {
        if (this->isWindowModified() || not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
        {
            if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize)
            {
                const qint32 index = m_dimensionB ? m_dimensionB->findData(value) : -1;
                if (index != -1)
                {
                    m_dimensionB->setCurrentIndex(index);
                }
                else
                {
                    qCCritical(
                        vMainWindow, "%s",
                        qUtf8Printable(tr("Not supported dimension B value '%1' for this pattern file.").arg(value)));
                    return false;
                }
            }
            else
            {
                qCCritical(vMainWindow, "%s",
                           qUtf8Printable(tr("Couldn't set dimension B. Need a file with multisize measurements.")));
                return false;
            }
        }
        else
        {
            qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Couldn't set dimension B. File wasn't opened.")));
            return false;
        }
    }
    else
    {
        qCWarning(vMainWindow, "%s", qUtf8Printable(tr("The method %1 does nothing in GUI mode").arg(Q_FUNC_INFO)));
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::SetDimensionC(int value) -> bool
{
    if (not VApplication::IsGUIMode())
    {
        if (this->isWindowModified() || not VAbstractValApplication::VApp()->GetPatternPath().isEmpty())
        {
            if (VAbstractValApplication::VApp()->GetMeasurementsType() == MeasurementsType::Multisize)
            {
                const qint32 index = m_dimensionC ? m_dimensionC->findData(value) : -1;
                if (index != -1)
                {
                    m_dimensionC->setCurrentIndex(index);
                }
                else
                {
                    qCCritical(
                        vMainWindow, "%s",
                        qUtf8Printable(tr("Not supported dimension C value '%1' for this pattern file.").arg(value)));
                    return false;
                }
            }
            else
            {
                qCCritical(vMainWindow, "%s",
                           qUtf8Printable(tr("Couldn't set dimension C. Need a file with multisize measurements.")));
                return false;
            }
        }
        else
        {
            qCCritical(vMainWindow, "%s", qUtf8Printable(tr("Couldn't set dimension C. File wasn't opened.")));
            return false;
        }
    }
    else
    {
        qCWarning(vMainWindow, "%s", qUtf8Printable(tr("The method %1 does nothing in GUI mode").arg(Q_FUNC_INFO)));
        return false;
    }
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ProcessCMD()
{
    const VCommandLinePtr cmd = VApplication::CommandLine();
    auto args = cmd->OptInputFileNames();

    isNoScaling = cmd->IsNoScalingEnabled();

    if (VApplication::IsGUIMode())
    {
        ReopenFilesAfterCrash(args);

        for (auto &arg : args)
        {
            LoadPattern(arg);
        }
    }
    else
    {
        if (args.size() != 1)
        {
            qCritical() << tr("Please, provide one input file.");
            QCoreApplication::exit(V_EX_NOINPUT);
            return;
        }

        VAbstractValApplication::VApp()->SetUserMaterials(cmd->OptUserMaterials());

        if (const bool loaded = LoadPattern(args.constFirst(), cmd->OptMeasurePath()); not loaded)
        {
            return; // process only one input file
        }

        bool aSetted = true;
        bool bSetted = true;
        bool cSetted = true;
        if (cmd->IsSetDimensionA())
        {
            aSetted = SetDimensionA(cmd->OptDimensionA());
        }

        if (cmd->IsSetDimensionB())
        {
            bSetted = SetDimensionB(cmd->OptDimensionB());
        }

        if (cmd->IsSetDimensionC())
        {
            cSetted = SetDimensionB(cmd->OptDimensionC());
        }

        if (not(aSetted && bSetted && cSetted))
        {
            QCoreApplication::exit(V_EX_DATAERR);
            return;
        }

        if (not cmd->IsTestModeEnabled())
        {
            if (cmd->IsExportEnabled() && not DoExport(cmd))
            {
                return;
            }

            if (cmd->IsExportFMEnabled() && not DoFMExport(cmd))
            {
                return;
            }
        }

        QCoreApplication::exit(V_EX_OK); // close program after processing in console mode
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto MainWindow::GetPatternFileName() -> QString
{
    QString shownName = tr("untitled.val");
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
// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 


// Extracted: 



//---------------------------------------------------------------------------------------------------------------------
void MainWindow::PrintPatternMessage(QEvent *event)
{
    SCASSERT(event != nullptr)
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    const auto *patternMessageEvent = static_cast<WarningMessageEvent *>(event);

    QString severity;

    switch (patternMessageEvent->Severity())
    {
        case QtDebugMsg:
            severity = tr("DEBUG");
            break;
        case QtWarningMsg:
            severity = tr("WARNING");
            break;
        case QtCriticalMsg:
            severity = tr("CRITICAL");
            break;
        case QtFatalMsg:
            severity = tr("FATAL");
            break;
        case QtInfoMsg:
            severity = tr("INFO");
            break;
        default:
            break;
    }

    const QString patternMessage = patternMessageEvent->Message()
            .remove(VAbstractValApplication::warningMessageSignature);
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
    const auto message = QStringLiteral("%1: [%2] %3").arg(time, severity, patternMessage);
    ui->plainTextEditPatternMessages->appendFilteredText(message);
    if (not m_unreadPatternMessage.isNull())
    {
        m_unreadPatternMessage->setText(DialogWarningIcon(m_unreadPatternMessage) + tr("Pattern messages"));
    }
}
// Extracted to mainwindow_watermark.cpp: //---------------------------------------------------------------------------------------------------------------------


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::AutoArrangeLabels(QPoint globalPos)
{
    if (m_currentTool != Tool::Arrow)
    {
        return;
    }

    QMenu menu;
    if (QAction const *arrangeAction =
            menu.addAction(FromTheme(VThemeIcon::FormatJustifyCenter), tr("Auto-Arrange Labels"));
            menu.exec(globalPos) != arrangeAction)
    {
        return;
    }

    RunAutoArrangeLabels();
}
// Extracted: 


