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
#include "ui_mainwindow.h"
#include "mainwindow_helpers.h"
#include "vtheme.h"
#include "widgets/TopBar.h"
#include "vmisc/FactoryWidgets.h"
#include "../vpropertyexplorer/qtcolorpicker.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vmodifierkey.h"
#include "../vtools/dialogs/dialogtoolbox.h"
#include "../vtools/tools/vabstracttool.h"
#include "../ifc/xml/vpatternblockmapper.h"
#include "../vtools/undocommands/renamepp.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "../vwidgets/vwidgetpopup.h"
#include "core/vapplication.h"
#include <QAction>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QStyleFactory>
#include <QToolButton>

using namespace Qt::Literals::StringLiterals;

namespace
{
//---------------------------------------------------------------------------------------------------------------------
auto FormatActionToolTip(const QAction *action) -> QString
{
    if (action == nullptr)
    {
        return {};
    }

    auto text = action->text();
    text.remove(u'&');

    if (const auto shortcuts = action->shortcuts(); !shortcuts.isEmpty())
    {
        const auto sc = VModifierKey::Sequence(shortcuts.constFirst()).toLower();
        return QStringLiteral("%1 (%2)").arg(text, sc);
    }

    return text;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateMeasurementState()
{
    InitDimensionControls();
    UpdateMeasurementFileNameLabel();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::SyncShowAllLabelsText()
{
    ui->actionShowAllLabels->setText(VAbstractValApplication::VApp()->ValentinaSettings()->GetShowAllLabels()
                                         ? tr("Show all point names")
                                         : tr("Hide point names"));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::RetranslateActionToolTips()
{
    ui->actionDraw->setToolTip(FormatActionToolTip(ui->actionDraw));
    ui->actionDetails->setToolTip(FormatActionToolTip(ui->actionDetails));
    ui->actionGraded->setToolTip(FormatActionToolTip(ui->actionGraded));
    ui->actionLayout->setToolTip(FormatActionToolTip(ui->actionLayout));
    ui->actionExport->setToolTip(FormatActionToolTip(ui->actionExport));

    ui->actionNewDraw->setToolTip(FormatActionToolTip(ui->actionNewDraw));

    ui->actionLoadMeasurements->setToolTip(FormatActionToolTip(ui->actionLoadMeasurements));
    ui->actionTable->setToolTip(FormatActionToolTip(ui->actionTable));

    ui->actionZoomIn->setToolTip(FormatActionToolTip(ui->actionZoomIn));
    ui->actionZoomOut->setToolTip(FormatActionToolTip(ui->actionZoomOut));
    ui->actionZoomFitBest->setToolTip(FormatActionToolTip(ui->actionZoomFitBest));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolBarStages()
{
    // Navigation labels (used by mode-switch code, kept alive for pixmap updates)
    // Hidden — they were previously inserted in the old QToolBar between mode buttons.
    m_leftGoToStage = new QLabel(this);
    m_leftGoToStage->setVisible(false);
    m_rightGoToStage = new QLabel(this);
    m_rightGoToStage->setVisible(false);

    m_topBar->SetModeActions(ui->actionDraw, ui->actionDetails, ui->actionGraded, ui->actionLayout,
                             ui->actionExport);

    ui->actionDraw->setToolTip(FormatActionToolTip(ui->actionDraw));
    ui->actionDetails->setToolTip(FormatActionToolTip(ui->actionDetails));
    ui->actionGraded->setToolTip(FormatActionToolTip(ui->actionGraded));
    ui->actionLayout->setToolTip(FormatActionToolTip(ui->actionLayout));
    ui->actionExport->setToolTip(FormatActionToolTip(ui->actionExport));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolBarDraws()
{
    m_patternPieceLabel = new QLabel(tr("Piece:"));
    m_comboBoxDraws = MakeComboBox();
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

    m_topBar->SetPatternPieceWidgets(m_patternPieceLabel, m_comboBoxDraws, ui->actionNewDraw);
    ui->actionNewDraw->setToolTip(FormatActionToolTip(ui->actionNewDraw));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::ToolBarMeasurements()
{
    ui->actionLoadMeasurements->setToolTip(FormatActionToolTip(ui->actionLoadMeasurements));

    m_measurementFileNameLabel = new QLabel();
    m_measurementSeparator = new QLabel();
    UpdateMeasurementSeparatorIcon();

    ui->actionTable->setToolTip(FormatActionToolTip(ui->actionTable));

    m_topBar->SetMeasurementWidgets(ui->actionLoadMeasurements, m_measurementFileNameLabel,
                                     m_measurementSeparator, ui->actionTable);

    UpdateMeasurementFileNameLabel();
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

    m_shortcutActions.insert(VShortcutAction::ShowAllLabels, ui->actionShowAllLabels);
    ui->actionShowAllLabels->setCheckable(true);
    ui->actionShowAllLabels->setChecked(true); // check always visible; the text reflects the state
    SyncShowAllLabelsText();
    connect(ui->actionShowAllLabels, &QAction::triggered, this,
            [this]()
            {
                auto *settings = VAbstractValApplication::VApp()->ValentinaSettings();
                settings->SetShowAllLabels(not settings->GetShowAllLabels());
                SyncShowAllLabelsText();
                ui->actionShowAllLabels->setChecked(true); // keep the check visible
                ScaleChanged(ui->view->transform().m11());
            });

    ui->actionZoomIn->setToolTip(FormatActionToolTip(ui->actionZoomIn));
    ui->actionZoomOut->setToolTip(FormatActionToolTip(ui->actionZoomOut));
    ui->actionZoomFitBest->setToolTip(FormatActionToolTip(ui->actionZoomFitBest));

    // --- Zoom block ----------------------------------------------------------

    auto *zoomBlock = new QWidget();
    zoomBlock->setObjectName(QStringLiteral("zoomBlock"));
    auto *zoomLayout = new QHBoxLayout(zoomBlock);
    zoomLayout->setContentsMargins(0, 0, 0, 0);
    zoomLayout->setSpacing(0);

    m_btnZoomIn = new QToolButton(zoomBlock);
    m_btnZoomIn->setObjectName(QStringLiteral("zoomBlockBtnLeft"));
    m_btnZoomIn->setDefaultAction(ui->actionZoomIn);
    m_btnZoomIn->setAutoRaise(false);
    zoomLayout->addWidget(m_btnZoomIn);

    m_doubleSpinBoxScale = new QDoubleSpinBox(zoomBlock);
    m_doubleSpinBoxScale->setObjectName(QStringLiteral("zoomBlockSpin"));
    m_doubleSpinBoxScale->setDecimals(1);
    m_doubleSpinBoxScale->setSuffix(QChar('%'));
    m_doubleSpinBoxScale->setButtonSymbols(QAbstractSpinBox::NoButtons);
    m_doubleSpinBoxScale->setAlignment(Qt::AlignCenter);
    ScaleChanged(ui->view->transform().m11());
    connect(m_doubleSpinBoxScale.data(), QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
            [this](double d) { ui->view->Zoom(d / 100.0); });
    zoomLayout->addWidget(m_doubleSpinBoxScale);

    m_btnZoomOut = new QToolButton(zoomBlock);
    m_btnZoomOut->setObjectName(QStringLiteral("zoomBlockBtnRight"));
    m_btnZoomOut->setDefaultAction(ui->actionZoomOut);
    m_btnZoomOut->setAutoRaise(false);
    zoomLayout->addWidget(m_btnZoomOut);

    // Force buttons to match spinbox height for the single-block look.
    const int h = m_doubleSpinBoxScale->sizeHint().height();
    m_btnZoomIn->setMinimumHeight(h);
    m_btnZoomIn->setMaximumHeight(h);
    m_btnZoomOut->setMinimumHeight(h);
    m_btnZoomOut->setMaximumHeight(h);

    m_topBar->SetZoomWidget(zoomBlock);

    UpdateTopBarIcons();
    UpdateMenuIcons();
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateTopBarIcons()
{
    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    const auto themeDir = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));

    ui->actionDraw->setIcon(
        QIcon(QStringLiteral(":/icons/%1/polygon-fill_%1.svg").arg(themeDir)));
    ui->actionDetails->setIcon(
        QIcon(QStringLiteral(":/icons/%1/sliders-fill_%1.svg").arg(themeDir)));
    ui->actionGraded->setIcon(
        QIcon(QStringLiteral(":/icons/%1/stack-fill_%1.svg").arg(themeDir)));
    ui->actionLayout->setIcon(
        QIcon(QStringLiteral(":/icons/%1/printer-fill_%1.svg").arg(themeDir)));
    // Export mode reuses the Layout icon.
    ui->actionExport->setIcon(
        QIcon(QStringLiteral(":/icons/%1/printer-fill_%1.svg").arg(themeDir)));
    ui->actionNewDraw->setIcon(
        QIcon(QStringLiteral(":/icons/%1/plus-circle-fill_%1.svg").arg(themeDir)));

    ui->actionZoomIn->setIcon(
        QIcon(QStringLiteral(":/icons/%1/magnifying-glass-plus-fill_%1.svg").arg(themeDir)));
    ui->actionZoomOut->setIcon(
        QIcon(QStringLiteral(":/icons/%1/magnifying-glass-minus-fill_%1.svg").arg(themeDir)));

    ui->actionLoadMeasurements->setIcon(
        QIcon(QStringLiteral(":/icons/%1/ruler-fill_%1.svg").arg(themeDir)));
    ui->actionTable->setIcon(
        QIcon(QStringLiteral(":/icons/%1/table-fill_%1.svg").arg(themeDir)));

    UpdateMeasurementSeparatorIcon();

    if (m_topBar)
    {
        m_topBar->ReloadIcons();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateMenuIcons()
{
    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    const auto themeDir = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));

    ui->actionNew->setIcon(
        QIcon(QStringLiteral(":/icons/%1/file-plus_%1.svg").arg(themeDir)));
    ui->actionOpen->setIcon(
        QIcon(QStringLiteral(":/icons/%1/folder-open_%1.svg").arg(themeDir)));
    ui->actionSave->setIcon(
        QIcon(QStringLiteral(":/icons/%1/floppy-disk_%1.svg").arg(themeDir)));
    ui->actionSaveAs->setIcon(
        QIcon(QStringLiteral(":/icons/%1/file-arrow-up_%1.svg").arg(themeDir)));
    ui->actionPattern_properties->setIcon(
        QIcon(QStringLiteral(":/icons/%1/info_%1.svg").arg(themeDir)));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateMessagesIcons()
{
    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    const auto themeDir = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));

    ui->toolButtonClearMessages->setIcon(
        QIcon(QStringLiteral(":/icons/%1/broom_%1.svg").arg(themeDir)));
    ui->toolButtonAutoRefresh->setIcon(
        QIcon(QStringLiteral(":/icons/%1/arrows-clockwise_%1.svg").arg(themeDir)));
    ui->toolButtonMessagesZoomInFont->setIcon(
        QIcon(QStringLiteral(":/icons/%1/magnifying-glass-plus_%1.svg").arg(themeDir)));
    ui->toolButtonMessagesZoomOutFont->setIcon(
        QIcon(QStringLiteral(":/icons/%1/magnifying-glass-minus_%1.svg").arg(themeDir)));
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateMeasurementFileNameLabel()
{
    if (m_measurementFileNameLabel)
    {
        const auto mpath = doc->MPath();
        if (mpath.isEmpty())
        {
            const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
            const auto themeDir = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));
            const auto caretPath = QStringLiteral(":/icons/%1/caret-left_%1.svg").arg(themeDir);
            const auto caretImg = QStringLiteral("<img src=\"%1\" width=\"12\" height=\"12\" "
                                                   "style=\"vertical-align:middle;\">").arg(caretPath);
            m_measurementFileNameLabel->setText(
                caretImg + QStringLiteral(" ") + tr("Load measurements table"));
        }
        else
        {
            m_measurementFileNameLabel->setText(QFileInfo(mpath).fileName());
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void MainWindow::UpdateMeasurementSeparatorIcon()
{
    if (!m_measurementSeparator)
    {
        return;
    }

    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    const auto themeDir = QString(isDark ? QStringLiteral("dark") : QStringLiteral("light"));
    const auto path = QStringLiteral(":/icons/%1/line-vertical_%1.svg").arg(themeDir);

    QPixmap pix(path);
    if (pix.isNull())
    {
        m_measurementSeparator->setText(QStringLiteral("|"));
        return;
    }

    const int h = m_measurementSeparator->fontMetrics().height();
    m_measurementSeparator->setPixmap(pix.scaledToHeight(h, Qt::SmoothTransformation));
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
