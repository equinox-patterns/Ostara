/************************************************************************
 **
 **  @file   mainwindow_toolbars.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Toolbar initialization methods extracted from mainwindow.cpp
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

#include <QAction>
#include <QComboBox>
#include <QPointer>
#include <QDesktopServices>
#include <QDoubleSpinBox>
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
#include "../vganalytics/vganalytics.h"
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
#include "dialogs/dialognewpattern.h"
#include "dialogs/dialogpatternproperties.h"
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

#ifdef Q_OS_WIN
#ifdef DocumentProperties
#endif
#endif

#include "ui_mainwindow.h"

using namespace std::chrono_literals;
using namespace Qt::Literals::StringLiterals;

#include "mainwindow_helpers.h"



//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolBarOption enable option toolbar.
 */
void MainWindow::ToolBarOption()
{
    ui->toolBarOption->clear();

    delete m_zoomScale.data();
    delete m_doubleSpinBoxScale.data();
    delete m_mouseCoordinate.data();
    delete m_unreadPatternMessage.data();

    InitDimensionControls();

    m_zoomScale = new QLabel(tr("Scale:"), this);
    ui->toolBarOption->addWidget(m_zoomScale);

    m_doubleSpinBoxScale = new QDoubleSpinBox(this);
    m_doubleSpinBoxScale->setDecimals(1);
    m_doubleSpinBoxScale->setSuffix(QChar('%'));
    ScaleChanged(ui->view->transform().m11());
    connect(m_doubleSpinBoxScale.data(), QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double d) { ui->view->Zoom(d / 100.0); });
    ui->toolBarOption->addWidget(m_doubleSpinBoxScale);

    ui->toolBarOption->addSeparator();

    m_mouseCoordinate =
        new QLabel(QStringLiteral("0, 0 (%1)").arg(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true)));
    ui->toolBarOption->addWidget(m_mouseCoordinate);

    ui->toolBarOption->addSeparator();
    m_unreadPatternMessage = new QLabel();
    ui->toolBarOption->addWidget(m_unreadPatternMessage);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolBarStages()
{
    const auto resource = QStringLiteral("icon");

    m_leftGoToStage = new QLabel(this);
    m_leftGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-next-skip.png")));
    ui->toolBarStages->insertWidget(ui->actionDetails, m_leftGoToStage);

    m_rightGoToStage = new QLabel(this);
    m_rightGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-next.png")));
    ui->toolBarStages->insertWidget(ui->actionLayout, m_rightGoToStage);
}


//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ToolBarDraws enable draw toolbar.
 */
void MainWindow::ToolBarDraws()
{
    m_patternPieceLabel = new QLabel(tr("Pattern Piece:"));
    ui->toolBarDraws->addWidget(m_patternPieceLabel);

    // By using Qt UI Designer we can't add QComboBox to toolbar
    m_comboBoxDraws = new QComboBox;
    ui->toolBarDraws->addWidget(m_comboBoxDraws);
    m_comboBoxDraws->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_comboBoxDraws->setEnabled(false);
    connect(m_comboBoxDraws, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) { ChangePP(index); });

    connect(ui->actionOptionDraw, &QAction::triggered, this,
            [this]()
            {
                QString draw = doc->PatternBlockMapper()->GetActive();
                if (bool const ok = PatternPieceName(draw); not ok)
                {
                    return;
                }
                VAbstractApplication::VApp()->getUndoStack()->push(new RenamePP(doc, draw, m_comboBoxDraws));
            });
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolBarPenOptions()
{
    // Pen style
    m_globalPenStyleLabel = new QLabel(tr("Pen style:"));
    ui->toolBarPenOptions->addWidget(m_globalPenStyleLabel);

    // By using Qt UI Designer we can't add QComboBox to toolbar
    m_comboBoxPenStyle = new QComboBox;
    m_comboBoxPenStyle->setToolTip(tr("Sets the <b>default pen style</b> for all newly created objects. This style is "
                                      "applied the first time an object is drawn."));
    ui->toolBarPenOptions->addWidget(m_comboBoxPenStyle);
    m_comboBoxPenStyle->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_comboBoxPenStyle->setMinimumSize(QSize(80, 0));
    m_comboBoxPenStyle->setIconSize(QSize(80, 14));
    FillComboBoxTypeLine(m_comboBoxPenStyle,
                         LineStylesPics(m_comboBoxPenStyle->palette().color(QPalette::Base),
                                        m_comboBoxPenStyle->palette().color(QPalette::Text)));
    ChangeCurrentData(m_comboBoxPenStyle, VApplication::VApp()->ValentinaSettings()->GetGlobalPenStyle());

    connect(m_comboBoxPenStyle, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this]()
    {
        VValentinaSettings *settings = VApplication::VApp()->ValentinaSettings();
        settings->SetGlobalPenStyle(GetComboBoxCurrentData(m_comboBoxPenStyle, TypeLineLine));
    });

    // Color
    m_globalColorLabel = new QLabel(tr("Color:"));
    ui->toolBarPenOptions->addWidget(m_globalColorLabel);

    m_pushButtonColor = new VPE::QtColorPicker(this);
    m_pushButtonColor->setToolTip(tr("Defines the <b>global object color property</b>. This color is applied "
                                     "automatically the first time a new object is drawn."));
    VValentinaSettings const *settings = VApplication::VApp()->ValentinaSettings();
    InitColorPicker(m_pushButtonColor, settings->GetUserToolColors());
    m_pushButtonColor->setUseNativeDialog(!settings->IsDontUseNativeDialog());
    m_pushButtonColor->setCurrentColor(settings->GetGlobalToolColor());
    ui->toolBarPenOptions->addWidget(m_pushButtonColor);

    connect(m_pushButtonColor, &VPE::QtColorPicker::colorChanged, this, [this]()
    {
        VValentinaSettings *settings = VApplication::VApp()->ValentinaSettings();
        settings->SetGlobalToolColor(m_pushButtonColor->currentColor().name());
        settings->SetUserToolColors(m_pushButtonColor->CustomColors());
    });

    connect(settings, &VValentinaSettings::UserToolColorsChanged, this, [this]()
    {
        QVector<QColor> const colors = VApplication::VApp()->ValentinaSettings()->GetUserToolColors();
        for (const auto &color : colors)
        {
            m_pushButtonColor->insertCustomColor(color);
        }
    }, Qt::QueuedConnection);
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolBarTools()
{
    m_shortcutActions.insert(VShortcutAction::ZoomIn, ui->actionZoomIn);
    connect(ui->actionZoomIn, &QAction::triggered, ui->view, &VMainGraphicsView::ZoomIn);

    m_shortcutActions.insert(VShortcutAction::ZoomOut, ui->actionZoomOut);
    connect(ui->actionZoomOut, &QAction::triggered, ui->view, &VMainGraphicsView::ZoomOut);

    m_shortcutActions.insert(VShortcutAction::ZoomOriginal, ui->actionZoomOriginal);
    connect(ui->actionZoomOriginal, &QAction::triggered, ui->view, &VMainGraphicsView::ZoomOriginal);

    m_shortcutActions.insert(VShortcutAction::ZoomFitBest, ui->actionZoomFitBest);
    connect(ui->actionZoomFitBest, &QAction::triggered, ui->view, &VMainGraphicsView::ZoomFitBest);

    m_shortcutActions.insert(VShortcutAction::ZoomFitBestCurrent, ui->actionZoomFitBestCurrent);
    connect(ui->actionZoomFitBestCurrent, &QAction::triggered, this, &MainWindow::ZoomFitBestCurrent,
            Qt::QueuedConnection);

    connect(ui->actionPreviousPatternPiece, &QAction::triggered, this, &MainWindow::PreviousPatternPiece);
    connect(ui->actionNextPatternPiece, &QAction::triggered, this, &MainWindow::NextPatternPiece);

    m_shortcutActions.insert(VShortcutAction::IncreaseLabelFont, ui->actionIncreaseLabelFont);
    connect(ui->actionIncreaseLabelFont, &QAction::triggered, this,
            [this]()
            {
                VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
                settings->SetPatternLabelFontSize(settings->GetPatternLabelFontSize() + 1);
                // Label visibility depends on the font size and is recomputed in RefreshScale(),
                // not in paint(), so drive the existing refresh dispatch instead of a bare update().
                ScaleChanged(ui->view->transform().m11());
            });

    m_shortcutActions.insert(VShortcutAction::DecreaseLabelFont, ui->actionDecreaseLabelFont);
    connect(ui->actionDecreaseLabelFont, &QAction::triggered, this,
            [this]()
            {
                VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
                settings->SetPatternLabelFontSize(settings->GetPatternLabelFontSize() - 1);
                ScaleChanged(ui->view->transform().m11());
            });

    m_shortcutActions.insert(VShortcutAction::OriginalLabelFont, ui->actionOriginalLabelFont);
    connect(ui->actionOriginalLabelFont, &QAction::triggered, this,
            [this]()
            {
                VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
                settings->SetPatternLabelFontSize(VCommonSettings::GetDefPatternLabelFontSize());
                ScaleChanged(ui->view->transform().m11());
            });

    m_shortcutActions.insert(VShortcutAction::HideLabels, ui->actionHideLabels);
    ui->actionHideLabels->setChecked(VAbstractValApplication::VApp()->ValentinaSettings()->GetHideLabels());
    connect(ui->actionHideLabels, &QAction::triggered, this,
            [this](bool checked)
            {
                VAbstractValApplication::VApp()->ValentinaSettings()->SetHideLabels(checked);
                ScaleChanged(ui->view->transform().m11());
            });
}


//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolBarDrawTools()
{
    SetupDrawToolsIcons();

    VValentinaSettings  const*settings = VApplication::VApp()->ValentinaSettings();

    ui->toolBarPointTools->clear();
    if (settings->IsUseToolGroups())
    {
        // Point tools
        {
            auto *linePointToolMenu = new QMenu(this);
            linePointToolMenu->addAction(ui->actionEndLineTool);
            linePointToolMenu->addAction(ui->actionAlongLineTool);
            linePointToolMenu->addAction(ui->actionMidpointTool);

            auto *linePointTool = new VToolButtonPopup(this);
            linePointTool->AssignMenu(linePointToolMenu);
            linePointTool->setDefaultAction(ui->actionEndLineTool);
            linePointTool->SetToolGroupTooltip(tr("Point at a given distance tools"));

            ui->toolBarPointTools->addWidget(linePointTool);
        }

        {
            auto *angleLinePointToolMenu = new QMenu(this);
            angleLinePointToolMenu->addAction(ui->actionNormalTool);
            angleLinePointToolMenu->addAction(ui->actionBisectorTool);
            angleLinePointToolMenu->addAction(ui->actionHeightTool);

            auto *angleLinePointTool = new VToolButtonPopup(this);
            angleLinePointTool->AssignMenu(angleLinePointToolMenu);
            angleLinePointTool->setDefaultAction(ui->actionNormalTool);
            angleLinePointTool->SetToolGroupTooltip(tr("Perpendicular/Bisector tools"));

            ui->toolBarPointTools->addWidget(angleLinePointTool);
        }

        {
            auto *lineIntersectionPointToolMenu = new QMenu(this);
            lineIntersectionPointToolMenu->addAction(ui->actionPointOfIntersectionTool);
            lineIntersectionPointToolMenu->addAction(ui->actionLineIntersectTool);

            auto *lineIntersectionPointTool = new VToolButtonPopup(this);
            lineIntersectionPointTool->AssignMenu(lineIntersectionPointToolMenu);
            lineIntersectionPointTool->setDefaultAction(ui->actionPointOfIntersectionTool);
            lineIntersectionPointTool->SetToolGroupTooltip(tr("Point at the intersections of lines/axis tools"));

            ui->toolBarPointTools->addWidget(lineIntersectionPointTool);
        }

        {
            auto *specialPointToolMenu = new QMenu(this);
            specialPointToolMenu->addAction(ui->actionShoulderPointTool);
            specialPointToolMenu->addAction(ui->actionTriangleTool);

            auto *specialPointTool = new VToolButtonPopup(this);
            specialPointTool->AssignMenu(specialPointToolMenu);
            specialPointTool->setDefaultAction(ui->actionShoulderPointTool);
            specialPointTool->SetToolGroupTooltip(tr("Triangular tools"));

            ui->toolBarPointTools->addWidget(specialPointTool);
        }

        {
            auto *axisPointToolMenu = new QMenu(this);
            axisPointToolMenu->addAction(ui->actionLineIntersectAxisTool);
            axisPointToolMenu->addAction(ui->actionCurveIntersectAxisTool);
            axisPointToolMenu->addAction(ui->actionArcIntersectAxisTool);

            auto *axisPointTool = new VToolButtonPopup(this);
            axisPointTool->AssignMenu(axisPointToolMenu);
            axisPointTool->setDefaultAction(ui->actionLineIntersectAxisTool);
            axisPointTool->SetToolGroupTooltip(tr("Point intersecting with axis tools"));

            ui->toolBarPointTools->addWidget(axisPointTool);
        }

        {
            auto *curveSegmentPointToolMenu = new QMenu(this);
            curveSegmentPointToolMenu->addAction(ui->actionSplineCutPointTool);
            curveSegmentPointToolMenu->addAction(ui->actionSplinePathCutPointTool);
            curveSegmentPointToolMenu->addAction(ui->actionArcCutPointTool);
            curveSegmentPointToolMenu->addAction(ui->actionArcStartPointTool);
            curveSegmentPointToolMenu->addAction(ui->actionArcEndPointTool);

            auto *curveSegmentPointTool = new VToolButtonPopup(this);
            curveSegmentPointTool->AssignMenu(curveSegmentPointToolMenu);
            curveSegmentPointTool->setDefaultAction(ui->actionSplineCutPointTool);
            curveSegmentPointTool->SetToolGroupTooltip(tr("Point segmenting curves/arcs tools"));

            ui->toolBarPointTools->addWidget(curveSegmentPointTool);
        }

        {
            auto *curveIntersectionPointToolMenu = new QMenu(this);
            curveIntersectionPointToolMenu->addAction(ui->actionIntersectionCurvesTool);
            curveIntersectionPointToolMenu->addAction(ui->actionPointOfIntersectionArcsTool);
            curveIntersectionPointToolMenu->addAction(ui->actionPointOfIntersectionCirclesTool);

            auto *curveIntersectionPointTool = new VToolButtonPopup(this);
            curveIntersectionPointTool->AssignMenu(curveIntersectionPointToolMenu);
            curveIntersectionPointTool->setDefaultAction(ui->actionIntersectionCurvesTool);
            curveIntersectionPointTool->SetToolGroupTooltip(tr("Points intersecting with 2 curves/arcs/circles tools"));

            ui->toolBarPointTools->addWidget(curveIntersectionPointTool);
        }

        {
            auto *tangentPointToolMenu = new QMenu(this);
            tangentPointToolMenu->addAction(ui->actionPointFromArcAndTangentTool);
            tangentPointToolMenu->addAction(ui->actionPointFromCircleAndTangentTool);
            tangentPointToolMenu->addAction(ui->actionPointOfContactTool);

            auto *tangentPointTool = new VToolButtonPopup(this);
            tangentPointTool->AssignMenu(tangentPointToolMenu);
            tangentPointTool->setDefaultAction(ui->actionPointFromArcAndTangentTool);
            tangentPointTool->SetToolGroupTooltip(
                tr("Points intersecting with arcs/curves and tangents/segments tools"));

            ui->toolBarPointTools->addWidget(tangentPointTool);
        }
    }
    else
    {
        ui->toolBarPointTools->addAction(ui->actionEndLineTool);
        ui->toolBarPointTools->addAction(ui->actionAlongLineTool);
        ui->toolBarPointTools->addAction(ui->actionMidpointTool);

        ui->toolBarPointTools->addAction(ui->actionNormalTool);
        ui->toolBarPointTools->addAction(ui->actionBisectorTool);
        ui->toolBarPointTools->addAction(ui->actionHeightTool);

        ui->toolBarPointTools->addAction(ui->actionPointOfIntersectionTool);
        ui->toolBarPointTools->addAction(ui->actionLineIntersectTool);

        ui->toolBarPointTools->addAction(ui->actionShoulderPointTool);
        ui->toolBarPointTools->addAction(ui->actionTriangleTool);

        ui->toolBarPointTools->addAction(ui->actionLineIntersectAxisTool);
        ui->toolBarPointTools->addAction(ui->actionCurveIntersectAxisTool);
        ui->toolBarPointTools->addAction(ui->actionArcIntersectAxisTool);

        ui->toolBarPointTools->addAction(ui->actionSplineCutPointTool);
        ui->toolBarPointTools->addAction(ui->actionSplinePathCutPointTool);
        ui->toolBarPointTools->addAction(ui->actionArcCutPointTool);
        ui->toolBarPointTools->addAction(ui->actionArcStartPointTool);
        ui->toolBarPointTools->addAction(ui->actionArcEndPointTool);

        ui->toolBarPointTools->addAction(ui->actionIntersectionCurvesTool);
        ui->toolBarPointTools->addAction(ui->actionPointOfIntersectionArcsTool);
        ui->toolBarPointTools->addAction(ui->actionPointOfIntersectionCirclesTool);

        ui->toolBarPointTools->addAction(ui->actionPointFromArcAndTangentTool);
        ui->toolBarPointTools->addAction(ui->actionPointFromCircleAndTangentTool);
        ui->toolBarPointTools->addAction(ui->actionPointOfContactTool);
    }

    ui->toolBarCurveTools->clear();
    if (settings->IsUseToolGroups())
    {
        // Curve tools
        {
            auto *curveToolMenu = new QMenu(this);
            curveToolMenu->addAction(ui->actionSplineTool);
            curveToolMenu->addAction(ui->actionCubicBezierTool);
            curveToolMenu->addAction(ui->actionSplinePathTool);
            curveToolMenu->addAction(ui->actionCubicBezierPathTool);
            curveToolMenu->addAction(ui->actionArcTool);
            curveToolMenu->addAction(ui->actionArcWithLengthTool);
            curveToolMenu->addAction(ui->actionEllipticalArcTool);
            curveToolMenu->addAction(ui->actionEllipticalArcWithLengthTool);
            curveToolMenu->addAction(ui->actionParallelCurveTool);
            curveToolMenu->addAction(ui->actionGraduatedCurveTool);

            auto *curvePointTool = new VToolButtonPopup(this);
            curvePointTool->AssignMenu(curveToolMenu);
            curvePointTool->setDefaultAction(ui->actionSplineTool);
            curvePointTool->SetToolGroupTooltip(tr("Curves and arcs tools"));

            ui->toolBarCurveTools->addWidget(curvePointTool);
        }
    }
    else
    {
        ui->toolBarCurveTools->addAction(ui->actionSplineTool);
        ui->toolBarCurveTools->addAction(ui->actionCubicBezierTool);
        ui->toolBarCurveTools->addAction(ui->actionSplinePathTool);
        ui->toolBarCurveTools->addAction(ui->actionCubicBezierPathTool);
        ui->toolBarCurveTools->addAction(ui->actionArcTool);
        ui->toolBarCurveTools->addAction(ui->actionArcWithLengthTool);
        ui->toolBarCurveTools->addAction(ui->actionEllipticalArcTool);
        ui->toolBarCurveTools->addAction(ui->actionEllipticalArcWithLengthTool);
        ui->toolBarCurveTools->addAction(ui->actionParallelCurveTool);
        ui->toolBarCurveTools->addAction(ui->actionGraduatedCurveTool);
    }

    ui->toolBarOperationTools->clear();
    if (settings->IsUseToolGroups())
    {
        // Group tools
        ui->toolBarOperationTools->addAction(ui->actionGroupTool);

        {
            auto *symmetryToolMenu = new QMenu(this);
            symmetryToolMenu->addAction(ui->actionFlippingByAxisTool);
            symmetryToolMenu->addAction(ui->actionFlippingByLineTool);

            auto *symmetryTool = new VToolButtonPopup(this);
            symmetryTool->AssignMenu(symmetryToolMenu);
            symmetryTool->setDefaultAction(ui->actionFlippingByAxisTool);
            symmetryTool->SetToolGroupTooltip(tr("Flipping objects tools"));

            ui->toolBarOperationTools->addWidget(symmetryTool);
        }

        {
            auto *transformToolMenu = new QMenu(this);
            transformToolMenu->addAction(ui->actionRotationTool);
            transformToolMenu->addAction(ui->actionMoveTool);

            auto *transformTool = new VToolButtonPopup(this);
            transformTool->AssignMenu(transformToolMenu);
            transformTool->setDefaultAction(ui->actionRotationTool);
            transformTool->SetToolGroupTooltip(tr("Rotating/moving objects tools"));

            ui->toolBarOperationTools->addWidget(transformTool);
        }

        ui->toolBarOperationTools->addAction(ui->actionTrueDartsTool);
        ui->toolBarOperationTools->addAction(ui->actionExportDraw);
    }
    else
    {
        ui->toolBarOperationTools->addAction(ui->actionGroupTool);

        ui->toolBarOperationTools->addAction(ui->actionFlippingByAxisTool);
        ui->toolBarOperationTools->addAction(ui->actionFlippingByLineTool);

        ui->toolBarOperationTools->addAction(ui->actionRotationTool);
        ui->toolBarOperationTools->addAction(ui->actionMoveTool);

        ui->toolBarOperationTools->addAction(ui->actionTrueDartsTool);
        ui->toolBarOperationTools->addAction(ui->actionExportDraw);
    }

    ui->toolBarDetailTools->clear();
    if (settings->IsUseToolGroups())
    {
        // Detail tools
        ui->toolBarDetailTools->addAction(ui->actionNewDetailTool);
        {
            auto *detailToolMenu = new QMenu(this);
            detailToolMenu->addAction(ui->actionUnionDetailsTool);
            detailToolMenu->addAction(ui->actionDuplicateDetailTool);

            auto *detailTool = new VToolButtonPopup(this);
            detailTool->AssignMenu(detailToolMenu);
            detailTool->setDefaultAction(ui->actionUnionDetailsTool);
            detailTool->SetToolGroupTooltip(tr("Merge/duplicate pieces tools"));

            ui->toolBarDetailTools->addWidget(detailTool);
        }

        {
            auto *internalDetailItemToolMenu = new QMenu(this);
            internalDetailItemToolMenu->addAction(ui->actionInternalPathTool);
            internalDetailItemToolMenu->addAction(ui->actionPinTool);
            internalDetailItemToolMenu->addAction(ui->actionInsertNodeTool);
            internalDetailItemToolMenu->addAction(ui->actionPlaceLabelTool);

            auto *detailTool = new VToolButtonPopup(this);
            detailTool->AssignMenu(internalDetailItemToolMenu);
            detailTool->setDefaultAction(ui->actionInternalPathTool);
            detailTool->SetToolGroupTooltip(tr("Adding objects for Details mode tools"));

            ui->toolBarDetailTools->addWidget(detailTool);
        }

        ui->toolBarDetailTools->addAction(ui->actionDetailExportAs);
    }
    else
    {
        ui->toolBarDetailTools->addAction(ui->actionNewDetailTool);

        ui->toolBarDetailTools->addAction(ui->actionUnionDetailsTool);
        ui->toolBarDetailTools->addAction(ui->actionDuplicateDetailTool);

        ui->toolBarDetailTools->addAction(ui->actionInternalPathTool);
        ui->toolBarDetailTools->addAction(ui->actionPinTool);
        ui->toolBarDetailTools->addAction(ui->actionInsertNodeTool);
        ui->toolBarDetailTools->addAction(ui->actionPlaceLabelTool);

        ui->toolBarDetailTools->addAction(ui->actionDetailExportAs);
    }
}

