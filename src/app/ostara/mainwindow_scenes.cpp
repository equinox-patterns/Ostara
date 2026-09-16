/************************************************************************
 **
 **  @file   mainwindow_scenes.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30/07/2026
 **
 **  @brief  Scene mode methods extracted from mainwindow.cpp
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
#include "widgets/DrawToolBar.h"
#include "vtheme.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vexception/vexception.h"
#include "../vtools/dialogs/tools/dialogtool.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "mainwindow_helpers.h"

#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QMap>
#include <QMessageBox>
#include <QPointer>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTransform>
#include <QtGlobal>
#include <memory>

using namespace Qt::Literals::StringLiterals;

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
    else if (ui->view->scene() == m_sceneGraded)
    {
        scene = m_sceneGraded;
    }
    else if (ui->view->scene() == m_sceneExport)
    {
        scene = m_sceneExport;
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

    RefreshSceneItemPens(scene);

    // The holder draws its handles in scene units, so it has to be told the scale or they
    // shrink with the plan — and the Export mode frames a plan thousands of units wide, which
    // leaves a five-unit square a single pixel across. Guarded on the scene, because the
    // holder lives in the Export scene whether or not that scene is the one on screen.
    if (scene == m_sceneExport && m_exportCorrection != nullptr)
    {
        m_exportCorrection->SetViewScale(scale);
    }
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
 * @brief switchScene activates the given mode's graphics scene and its
 *        mode-specific view state. Returns false if the mode entry was
 *        aborted (only Layout can abort).
 */
auto MainWindow::switchScene(Draw mode, const QVector<DetailForLayout> &details) -> bool
{
    const auto resource = QStringLiteral("icon");

    switch (mode)
    {
        case Draw::Calculation:
            m_leftGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-next-skip.png")));
            m_rightGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-next.png")));
            SaveCurrentScene();
            currentScene = m_sceneDraw;
            ui->view->setScene(currentScene);
            RestoreCurrentScene();
            VAbstractValApplication::VApp()->SetDrawMode(Draw::Calculation);
            m_comboBoxDraws->setCurrentIndex(m_currentDrawIndex); // restore current pattern piece
            m_drawMode = true;
            SetEnableTool(true);
            m_drawToolBar->SetMode(Draw::Calculation);
            break;
        case Draw::Modeling:
            if (m_drawMode)
            {
                m_currentDrawIndex = m_comboBoxDraws->currentIndex(); // save current pattern piece
                m_drawMode = false;
            }
            m_comboBoxDraws->setCurrentIndex(m_comboBoxDraws->count() - 1); // Need to get data about all details
            m_leftGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-previous.png")));
            m_rightGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-next.png")));
            SaveCurrentScene();
            currentScene = m_sceneDetails;
            emit ui->view->itemClicked(nullptr);
            ui->view->setScene(currentScene);
            RestoreCurrentScene();
            VAbstractValApplication::VApp()->SetDrawMode(Draw::Modeling);
            SetEnableTool(true);
            m_drawToolBar->SetMode(Draw::Modeling);
            break;
        case Draw::Graded:
            if (m_drawMode)
            {
                m_currentDrawIndex = m_comboBoxDraws->currentIndex(); // save current pattern piece
                m_drawMode = false;
            }
            m_comboBoxDraws->setCurrentIndex(m_comboBoxDraws->count() - 1); // Need to get data about all details
            m_leftGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-previous.png")));
            m_rightGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-next.png")));
            SaveCurrentScene();
            currentScene = m_sceneGraded;
            ui->view->setScene(currentScene);
            RestoreCurrentScene();
            VAbstractValApplication::VApp()->SetDrawMode(Draw::Graded);
            SetEnableTool(true);
            m_drawToolBar->SetMode(Draw::Graded);
            m_drawToolBar->hide();
            break;
        case Draw::Layout:
            if (m_drawMode)
            {
                m_currentDrawIndex = m_comboBoxDraws->currentIndex(); // save current pattern piece
                m_drawMode = false;
            }
            m_comboBoxDraws->setCurrentIndex(m_comboBoxDraws->count() - 1); // Need to get data about all details
            m_comboBoxDraws->setCurrentIndex(-1); // Hide pattern pieces
            m_leftGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-previous.png")));
            m_rightGoToStage->setPixmap(VTheme::GetPixmapResource(resource, QStringLiteral("24x24/go-previous-skip.png")));
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
                VAbstractValApplication::VApp()->GetDrawMode() == Draw::Calculation ? ActionDraw(true)
                                                                                    : ActionDetails(true);
                return false;
            }
            currentScene = tempSceneLayout;
            emit ui->view->itemClicked(nullptr);
            ui->view->setScene(currentScene);
            VAbstractValApplication::VApp()->SetDrawMode(Draw::Layout);
            SetEnableTool(true);
            m_drawToolBar->SetMode(Draw::Layout);
            if (not m_mouseCoordinate.isNull())
            {
                m_mouseCoordinate->setText(QString());
            }
            break;
        case Draw::Export:
            // Shell only for now: an empty scene, no dock, the selection tool.
            // Nothing is drawn yet, so the pattern-piece combo is left alone —
            // coming back to Draw lands on the piece the user was on.
            if (m_drawMode)
            {
                m_currentDrawIndex = m_comboBoxDraws->currentIndex(); // save current pattern piece
                m_drawMode = false;
            }
            SaveCurrentScene();
            currentScene = m_sceneExport;
            ui->view->setScene(currentScene);
            VAbstractValApplication::VApp()->SetDrawMode(Draw::Export);
            SetEnableTool(true);
            m_drawToolBar->SetMode(Draw::Export);
            break;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::InitScenes()
{
    m_sceneDraw = new VMainGraphicsScene(this);
    currentScene = m_sceneDraw;
    VAbstractValApplication::VApp()->setCurrentScene(&currentScene);
    connect(this, &MainWindow::EnableItemMove, m_sceneDraw, &VMainGraphicsScene::EnableItemMove);
    connect(this, &MainWindow::ItemsSelection, m_sceneDraw, &VMainGraphicsScene::ItemsSelection);

    connect(this, &MainWindow::EnablePointSelection, m_sceneDraw, &VMainGraphicsScene::TogglePointSelection);
    connect(this, &MainWindow::EnableLineSelection, m_sceneDraw, &VMainGraphicsScene::ToggleLineSelection);
    connect(this, &MainWindow::EnableArcSelection, m_sceneDraw, &VMainGraphicsScene::ToggleArcSelection);
    connect(this, &MainWindow::EnableElArcSelection, m_sceneDraw, &VMainGraphicsScene::ToggleElArcSelection);
    connect(this, &MainWindow::EnableSplineSelection, m_sceneDraw, &VMainGraphicsScene::ToggleSplineSelection);
    connect(this, &MainWindow::EnableSplinePathSelection, m_sceneDraw, &VMainGraphicsScene::ToggleSplinePathSelection);

    connect(this, &MainWindow::EnablePointHover, m_sceneDraw, &VMainGraphicsScene::TogglePointHover);
    connect(this, &MainWindow::EnableLineHover, m_sceneDraw, &VMainGraphicsScene::ToggleLineHover);
    connect(this, &MainWindow::EnableArcHover, m_sceneDraw, &VMainGraphicsScene::ToggleArcHover);
    connect(this, &MainWindow::EnableElArcHover, m_sceneDraw, &VMainGraphicsScene::ToggleElArcHover);
    connect(this, &MainWindow::EnableSplineHover, m_sceneDraw, &VMainGraphicsScene::ToggleSplineHover);
    connect(this, &MainWindow::EnableSplinePathHover, m_sceneDraw, &VMainGraphicsScene::ToggleSplinePathHover);

    connect(m_sceneDraw, &VMainGraphicsScene::mouseMove, this, &MainWindow::MouseMove);
    connect(m_sceneDraw, &VMainGraphicsScene::AddBackgroundImage, this, &MainWindow::PlaceBackgroundImage);

    m_sceneDetails = new VMainGraphicsScene(this);
    connect(this, &MainWindow::EnableItemMove, m_sceneDetails, &VMainGraphicsScene::EnableItemMove);

    connect(this, &MainWindow::EnableNodePointSelection, m_sceneDetails, &VMainGraphicsScene::ToggleNodePointSelection);
    connect(this, &MainWindow::EnableDetailSelection, m_sceneDetails, &VMainGraphicsScene::ToggleDetailSelection);

    connect(this, &MainWindow::EnableNodePointHover, m_sceneDetails, &VMainGraphicsScene::ToggleNodePointHover);
    connect(this, &MainWindow::EnableDetailHover, m_sceneDetails, &VMainGraphicsScene::ToggleDetailHover);

    connect(m_sceneDetails, &VMainGraphicsScene::mouseMove, this, &MainWindow::MouseMove);

    m_sceneGraded = new VMainGraphicsScene(this);
    connect(m_sceneGraded, &VMainGraphicsScene::mouseMove, this, &MainWindow::MouseMove);

    m_sceneExport = new VMainGraphicsScene(this);
    connect(m_sceneExport, &VMainGraphicsScene::mouseMove, this, &MainWindow::MouseMove);

    // The hand on the plan. It is given a repaint callback rather than a MainWindow pointer:
    // the store it writes into cannot announce anything, and only this window knows whether
    // the generation written is the one on screen and whether the mode is still Export.
    m_exportCorrection = new OExportCorrection(
        m_sceneExport, &m_exportOutputs,
        [this](quint32 id)
        {
            if (VAbstractValApplication::VApp()->GetDrawMode() != Draw::Export)
            {
                return;
            }
            // An undo can reach a correction made on another generation — the stack outlives
            // the selection — and repainting a plan that is not the one shown would put a
            // different result on screen than the one the user is looking at.
            if (m_exportOutputs.ActiveId() != id)
            {
                return;
            }
            if (const OExportOutput *output = m_exportOutputs.Find(id); output != nullptr)
            {
                ShowExportOutput(*output, false);
            }
        },
        this);

    ui->view->setScene(currentScene);

    m_sceneDraw->setTransform(ui->view->transform());
    m_sceneDetails->setTransform(ui->view->transform());
    m_sceneGraded->setTransform(ui->view->transform());
    m_sceneExport->setTransform(ui->view->transform());

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
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    policy.setHorizontalStretch(12);
    ui->view->setSizePolicy(policy);
    VAbstractValApplication::VApp()->setSceneView(ui->view);
}
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
