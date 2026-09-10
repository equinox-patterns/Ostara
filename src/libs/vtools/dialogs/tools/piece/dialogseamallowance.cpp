/************************************************************************
 **
 **  @file   dialogseamallowance.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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

// Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>

#include "dialogseamallowance.h"
#include "dialogseamallowance_helpers.h"
#include "../../../tools/vtoolseamallowance.h"
#include "../../../tools/drawTools/toolcurve/vtoolabstractcurve.h"
#include "../../../tools/drawTools/toolpoint/vabstractpoint.h"
#include "../../../undocommands/savepiecepathoptions.h"
#include "../../../undocommands/saveplacelabeloptions.h"
#include "../../../visualization/path/vispiecespecialpoints.h"
#include "../../../visualization/path/vistoolpiece.h"
#include "../../dialogtoolbox.h"
#include "../../support/dialogeditwrongformula.h"
#include "../vgeometry/vplacelabelitem.h"
#include "../vmisc/def.h"
#include "vtheme.h"
#include "../vmisc/typedef.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vmodifierkey.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/floatItemData/vgrainlinedata.h"
#include "../vpatterndb/variables/vincrement.h"
#include "../vpatterndb/variables/vmeasurement.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vwidgets/fancytabbar/fancytabbar.h"
#include "../vwidgets/vmousewheelwidgetadjustmentguard.h"
#include "dialogpiecepath.h"
#include "dialogplacelabel.h"
#include "ui_dialogseamallowance.h"
#include "ui_tabfoldline.h"
#include "ui_tabgrainline.h"
#include "ui_tabpassmarks.h"
#include "ui_tabpaths.h"
#include "ui_tabpins.h"
#include "ui_tabplacelabels.h"

#include <QTimer>
#include <QUuid>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;



//---------------------------------------------------------------------------------------------------------------------
DialogSeamAllowance::DialogSeamAllowance(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogSeamAllowance),
    uiTabPaths(new Ui::TabPaths),
    uiTabGrainline(new Ui::TabGrainline),
    uiTabPins(new Ui::TabPins),
    uiTabPassmarks(new Ui::TabPassmarks),
    uiTabPlaceLabels(new Ui::TabPlaceLabels),
    uiTabFoldLine(new Ui::TabFoldLine),
    m_tabPaths(new QWidget),
    m_tabGrainline(new QWidget),
    m_tabPins(new QWidget),
    m_tabPassmarks(new QWidget),
    m_tabPlaceLabels(new QWidget),
    m_tabFoldLine(new QWidget),
    m_ftb(new FancyTabBar(FancyTabBar::Left, this)),
    m_timerWidth(new QTimer(this)),
    m_timerWidthBefore(new QTimer(this)),
    m_timerWidthAfter(new QTimer(this)),
    m_timerPassmarkLength(new QTimer(this)),
    m_timerPassmarkWidth(new QTimer(this)),
    m_timerPassmarkAngle(new QTimer(this)),
    m_timerPassmarkVisible(new QTimer(this)),
    m_timerFoldHeight(new QTimer(this)),
    m_timerFoldWidth(new QTimer(this)),
    m_timerFoldCenter(new QTimer(this))
{
    ui->setupUi(this);

    connect(&m_automation, &OPieceAutomation::ListChanged, this, [this]() {
        SyncMainPathWidget();
        ValidObjects(MainPathIsValid());
    });
    connect(&m_automation, &OPieceAutomation::ToolTipMessage, this,
            [this](const QString &msg) { emit ToolTip(msg); });

    InitOkCancelApply(ui);
    InitFancyTabBar();
    EnableApply(applyAllowed);

    InitMainPathTab();
    InitPieceTab();
    InitSeamAllowanceTab();
    InitInternalPathsTab();
    InitGrainlineTab();
    InitPinsTab();
    InitPassmarksTab();
    InitPlaceLabelsTab();
    InitFoldLineTab();

    InitIcons();

    // Prevent stealing focus when scrolling
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabGrainline->comboBoxGrainlineCenterPin);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabGrainline->comboBoxGrainlineTopPin);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabGrainline->comboBoxGrainlineBottomPin);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabGrainline->comboBoxArrow);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabPassmarks->comboBoxPassmarks);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabPaths->comboBoxMLStartPoint);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabPaths->comboBoxMLEndPoint);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabPaths->spinBoxPriority);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabPaths->comboBoxNodes);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabPaths->comboBoxAngle);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabPaths->comboBoxStartPoint);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabPaths->comboBoxEndPoint);
    VMouseWheelWidgetAdjustmentGuard::InstallEventFilter(uiTabPaths->comboBoxIncludeType);

    ChangeColor(uiTabPaths->labelEditName, OkColor(this));
    flagMainPathIsValid = MainPathIsValid();

    m_ftb->SetCurrentIndex(TabOrder::Paths); // Show always first tab active on start.
}

//---------------------------------------------------------------------------------------------------------------------
DialogSeamAllowance::~DialogSeamAllowance()
{
    delete m_visSpecialPoints.data();
    delete m_tabFoldLine;
    delete m_tabPlaceLabels;
    delete m_tabPassmarks;
    delete m_tabPins;
    delete m_tabGrainline;
    delete m_tabPaths;
    delete uiTabPassmarks;
    delete uiTabPins;
    delete uiTabGrainline;
    delete uiTabPaths;
    delete ui;

    for (auto &command : m_undoStack)
    {
        delete command.data();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EnableApply(bool enable)
{
    SCASSERT(bApply != nullptr);
    bApply->setEnabled(enable);
    applyAllowed = enable;

    uiTabPaths->tabSeamAllowance->setEnabled(applyAllowed);
    uiTabPaths->tabInternalPaths->setEnabled(applyAllowed);
    uiTabPaths->groupBoxMirrorLine->setEnabled(applyAllowed);
    m_ftb->SetTabEnabled(TabOrder::Pins, applyAllowed);
    m_ftb->SetTabEnabled(TabOrder::Grainline, applyAllowed);
    m_ftb->SetTabEnabled(TabOrder::Passmarks, applyAllowed);
    m_ftb->SetTabEnabled(TabOrder::PlaceLabels, applyAllowed);
    m_ftb->SetTabEnabled(TabOrder::FoldLine, applyAllowed);

    if (not applyAllowed && vis.isNull())
    {
        vis = new VisToolPiece(&data);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::GetPiece() const -> VPiece
{
    return CreatePiece();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetPiece(const VPiece &piece)
{
    uiTabPaths->listWidgetMainPath->clear();
    for (int i = 0; i < piece.GetPath().CountNodes(); ++i)
    {
        NewMainPathItem(piece.GetPath().at(i));
    }
    // Keep the shared automation in sync with the loaded path (edit mode).
    m_automation.SetNodes(GetListInternals<VPieceNode>(uiTabPaths->listWidgetMainPath));

    uiTabPaths->checkBoxHideMainPath->setChecked(piece.IsHideMainPath());
    {
        const QSignalBlocker blocker(uiTabPaths->listWidgetCustomSA);
        uiTabPaths->listWidgetCustomSA->clear();
        QVector<CustomSARecord> const records = piece.GetCustomSARecords();
        for (auto record : records)
        {
            if (record.path > NULL_ID)
            {
                const QString name = GetPathName(record.path, record.reverse);

                auto *item = new QListWidgetItem(name);
                item->setFont(NodeFont(item->font()));
                item->setData(Qt::UserRole, QVariant::fromValue(record));
                uiTabPaths->listWidgetCustomSA->addItem(item);
                uiTabPaths->listWidgetCustomSA->setCurrentRow(uiTabPaths->listWidgetCustomSA->count() - 1);
            }
        }
    }

    uiTabPaths->listWidgetInternalPaths->clear();
    const QVector<quint32> paths = piece.GetInternalPaths();
    for (auto path : paths)
    {
        if (path > NULL_ID)
        {
            const QString name = GetPathName(path);

            auto *item = new QListWidgetItem(name);
            item->setFont(NodeFont(item->font()));
            item->setData(Qt::UserRole, QVariant::fromValue(path));
            uiTabPaths->listWidgetInternalPaths->addItem(item);
            uiTabPaths->listWidgetInternalPaths->setCurrentRow(uiTabPaths->listWidgetInternalPaths->count() - 1);
        }
    }

    auto NewSpecialPoint = [this](QListWidget *listWidget, quint32 point)
    {
        if (point > NULL_ID)
        {
            try
            {
                const QSharedPointer<VGObject> p = data.GetGObject(point);

                auto *item = new QListWidgetItem(p->name());
                item->setFont(NodeFont(item->font()));
                item->setData(Qt::UserRole, QVariant::fromValue(point));
                listWidget->addItem(item);
                listWidget->setCurrentRow(uiTabPins->listWidgetPins->count() - 1);
            }
            catch (const VExceptionBadId &e)
            {
                qWarning() << qUtf8Printable(e.ErrorMessage());
            }
        }
    };

    uiTabPins->listWidgetPins->clear();

    InitAllPinComboboxes();

    uiTabPlaceLabels->listWidgetPlaceLabels->clear();
    const QVector<quint32> labels = piece.GetPlaceLabels();
    for (auto label : labels)
    {
        NewSpecialPoint(uiTabPlaceLabels->listWidgetPlaceLabels, label);
    }

    if (!piece.GetPlaceLabels().empty())
    {
        uiTabPlaceLabels->listWidgetPlaceLabels->setCurrentRow(0);
    }

    {
        const QSignalBlocker blocker(uiTabPaths->comboBoxStartPoint);
        uiTabPaths->comboBoxStartPoint->clear();
    }

    {
        const QSignalBlocker blocker(uiTabPaths->comboBoxEndPoint);
        uiTabPaths->comboBoxEndPoint->clear();
    }

    CustomSAChanged(0);

    uiTabPaths->checkBoxForbidFlipping->setChecked(piece.IsForbidFlipping());
    uiTabPaths->checkBoxForceFlipping->setChecked(piece.IsForceFlipping());
    uiTabPaths->checkBoxSymmetricalCopy->setChecked(piece.IsSymmetricalCopy());
    uiTabPaths->checkBoxFollowGrainline->setChecked(piece.IsFollowGrainline());
    uiTabPaths->checkBoxOnDrawing->setChecked(piece.IsSewLineOnDrawing());
    uiTabPaths->checkBoxSeams->setChecked(piece.IsSeamAllowance());
    uiTabPaths->lineEditName->setText(piece.GetName());
    uiTabPaths->lineEditUUID->setText(piece.GetUUID().toString());
    uiTabPaths->spinBoxPriority->setValue(static_cast<int>(piece.GetPriority()));

    InitFold(piece);

    uiTabPaths->plainTextEditFormulaWidth->setPlainText(VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        piece.GetFormulaSAWidth(), VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
    m_saWidth = piece.GetSAWidth();

    int index = uiTabGrainline->comboBoxArrow->findData(static_cast<int>(piece.GetGrainlineGeometry().GetArrowType()));
    if (index == -1)
    {
        index = 0;
    }
    uiTabGrainline->comboBoxArrow->setCurrentIndex(index);

    const VGrainlineData &grainlineGeometry = piece.GetGrainlineGeometry();
    uiTabGrainline->groupBoxGrainline->setChecked(grainlineGeometry.IsEnabled());
    SetGrainlineAngle(grainlineGeometry.GetRotation());
    SetGrainlineLength(grainlineGeometry.GetLength());
    uiTabGrainline->checkBoxGrainlineVisible->setChecked(grainlineGeometry.IsVisible());

    ValidObjects(MainPathIsValid());
    EnabledGrainline();

    ListChanged();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedObject gets id and type of selected object. Save right data and ignore wrong.
 * @param id id of objects (points, arcs, splines, spline paths)
 * @param type type of object
 */
void DialogSeamAllowance::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare)
    {
        return;
    }

    const QVector<QPair<quint32, quint32>> attachmentItems =
        m_doc->IsIsolated() ? QVector<QPair<quint32, quint32>>{} : m_doc->GetAttachmentItemsForObject(id);

    m_automation.HandleObjectChosen(id, type, attachmentItems);
    SyncMainPathWidget();
    ValidObjects(MainPathIsValid());

    if (not applyAllowed)
    {
        auto *visPath = qobject_cast<VisToolPiece *>(vis);
        SCASSERT(visPath != nullptr);
        const VPiece p = CreatePiece();
        visPath->SetPiece(p);

        if (p.GetPath().CountNodes() == 1)
        {
            emit ToolTip(tr("Select main path objects clockwise, <b>%1</b> - finish creation")
                             .arg(VModifierKey::EnterKey()));

            if (not VAbstractValApplication::VApp()->getCurrentScene()->items().contains(visPath))
            {
                visPath->VisualMode(NULL_ID);
            }
            else
            {
                visPath->RefreshGeometry();
            }
        }
        else
        {
            visPath->RefreshGeometry();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ShowDialog(bool click)
{
    if (!click && m_automation.NodeCount() > 0)
    {
        emit ToolTip(QString());
        prepare = true;

        // The contour is complete: resolve the sense of curves whose entry was unknown.
        m_automation.ReconcilePendingReverse();
        // A straight closing edge that follows an already used curve completes the loop.
        m_automation.CompleteClosingCurve(false);
        if (not m_automation.PendingReverseCurves().isEmpty())
        {
            m_automation.ShowSenseMessage();
        }
        m_automation.ClearPendingReverse();
        SyncMainPathWidget();

        if (not applyAllowed)
        {
            auto *visPath = qobject_cast<VisToolPiece *>(vis);
            SCASSERT(visPath != nullptr);
            visPath->SetMode(Mode::Show);
            visPath->RefreshGeometry();
        }

        // Fix issue #526. Dialog Detail is not on top after selection second object on Mac.
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
        show();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::eventFilter(QObject* obj, QEvent* event) -> bool
{
    if (obj == uiTabPaths->listWidgetMainPath && event->type() == QEvent::KeyPress)
    {
        if (const auto* keyEvent = static_cast<QKeyEvent*>(event); keyEvent->key() == Qt::Key_Delete)
        {
            if (QListWidgetItem* selectedItem = uiTabPaths->listWidgetMainPath->currentItem();selectedItem)
            {
                delete selectedItem;
            }
            return true; // Event is handled
        }
    }
    return DialogTool::eventFilter(obj, event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SaveData()
{
    if (m_doc != nullptr)
    {
        emit m_doc->UpdatePatternLabel();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::CheckTabPathsState()
{
    bool const isValid = flagFormula && flagFormulaBefore && flagFormulaAfter;
    bool const isMainPathValid = flagMainPathIsValid && flagMirrorLineIsValid;
    bool const isNameAndUUIDValid = flagName && flagUUID;

    m_ftb->SetTabText(TabOrder::Paths,
                      (isValid && flagMainPathIsValid && isNameAndUUIDValid) ? tr("Paths") : tr("Paths") + '*'_L1);

    const QIcon warningIcon = FromTheme(VThemeIcon::DialogWarning);

    uiTabPaths->tabWidget->setTabIcon(uiTabPaths->tabWidget->indexOf(uiTabPaths->tabSeamAllowance),
                                      isValid ? QIcon() : warningIcon);
    uiTabPaths->tabWidget->setTabIcon(uiTabPaths->tabWidget->indexOf(uiTabPaths->tabMainPath),
                                      isMainPathValid ? QIcon() : warningIcon);
    uiTabPaths->tabWidget->setTabIcon(uiTabPaths->tabWidget->indexOf(uiTabPaths->tabPiece),
                                      isNameAndUUIDValid ? QIcon() : warningIcon);

    if (flagMainPathIsValid && flagMirrorLineIsValid)
    {
        QString tooltip = tr("Ready!");
        if (!applyAllowed)
        {
            tooltip += "  <b>"_L1 +
                       tr("To open all detail's features complete creating the main path. Please, press OK.") +
                       "</b>"_L1;
        }
        uiTabPaths->helpLabel->setText(tooltip);
    }

    uiTabPaths->comboBoxNodes->setEnabled(flagFormulaBefore && flagFormulaAfter);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::CheckTabPassmarksState()
{
    if (uiTabPassmarks->comboBoxPassmarks->count() == 0)
    {
        flagFormulaPassmarkLength = true;
        flagFormulaPassmarkWidth = true;
        flagFormulaPassmarkAngle = true;
        flagFormulaPassmarkVisible = true;
    }

    const bool allFormulasValid = flagFormulaPassmarkLength && flagFormulaPassmarkWidth && flagFormulaPassmarkAngle &&
            flagFormulaPassmarkVisible;

    m_ftb->SetTabText(TabOrder::Passmarks, tr("Passmarks") + (allFormulasValid ? QString() : QChar('*')));

    const QIcon icon = FromTheme(VThemeIcon::DialogWarning);
    uiTabPassmarks->tabWidget->setTabIcon(uiTabPassmarks->tabWidget->indexOf(uiTabPassmarks->tabControl),
                                          flagFormulaPassmarkVisible ? QIcon() : icon);
    bool const allShapeFormulasValid = flagFormulaPassmarkLength && flagFormulaPassmarkWidth && flagFormulaPassmarkAngle;
    uiTabPassmarks->tabWidget->setTabIcon(uiTabPassmarks->tabWidget->indexOf(uiTabPassmarks->tabManualShape),
                                          allShapeFormulasValid ? QIcon() : icon);

    uiTabPassmarks->comboBoxPassmarks->setEnabled(allFormulasValid);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::CheckTabFoldLineState()
{
    if (!uiTabFoldLine->groupBoxManualHeight->isChecked())
    {
        flagFormulaFoldHeight = true;
    }

    if (!uiTabFoldLine->groupBoxManualWidth->isChecked())
    {
        flagFormulaFoldWidth = true;
    }

    if (!uiTabFoldLine->groupBoxManualCenter->isChecked())
    {
        flagFormulaFoldCenter = true;
    }

    const bool allFormulasValid = flagFormulaFoldHeight && flagFormulaFoldWidth && flagFormulaFoldCenter;

    m_ftb->SetTabText(TabOrder::FoldLine, tr("Fold line") + (allFormulasValid ? QString() : QChar('*')));

    const QIcon icon = FromTheme(VThemeIcon::DialogWarning);
    uiTabFoldLine->tabWidget->setTabIcon(uiTabFoldLine->tabWidget->indexOf(uiTabFoldLine->tabShape),
                                         allFormulasValid ? QIcon() : icon);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::GetMirrorLineStartPoint() const -> quint32
{
    const quint32 startPoint = getCurrentObjectId(uiTabPaths->comboBoxMLStartPoint);
    const quint32 endPoint = getCurrentObjectId(uiTabPaths->comboBoxMLEndPoint);

    if (startPoint == endPoint || startPoint == NULL_ID || endPoint == NULL_ID)
    {
        return startPoint;
    }

    const int index = NodeRowIndex(uiTabPaths->listWidgetMainPath, startPoint);
    if (index == -1)
    {
        return startPoint;
    }

    int nextIndex = index + 1;
    if (nextIndex >= uiTabPaths->listWidgetMainPath->count())
    {
        nextIndex = 0;
    }

    if (const int next = FindNotExcludedNeighborNodeDown(uiTabPaths->listWidgetMainPath, nextIndex);
        next >= 0 && RowNode(uiTabPaths->listWidgetMainPath, next).GetId() == endPoint)
    {
        return startPoint;
    }

    return endPoint;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::GetMirrorLineEndPoint() const -> quint32
{
    const quint32 startPoint = getCurrentObjectId(uiTabPaths->comboBoxMLStartPoint);
    const quint32 endPoint = getCurrentObjectId(uiTabPaths->comboBoxMLEndPoint);

    if (startPoint == endPoint || startPoint == NULL_ID || endPoint == NULL_ID)
    {
        return endPoint;
    }

    const int index = NodeRowIndex(uiTabPaths->listWidgetMainPath, endPoint);
    if (index == -1)
    {
        return endPoint;
    }

    int prevIndex = index - 1;
    if (prevIndex < 0)
    {
        prevIndex = uiTabPaths->listWidgetMainPath->count() - 1;
    }

    if (const int prev = FindNotExcludedNeighborNodeUp(uiTabPaths->listWidgetMainPath, prevIndex);
        prev >= 0 && RowNode(uiTabPaths->listWidgetMainPath, prev).GetId() == startPoint)
    {
        return endPoint;
    }

    return startPoint;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::CheckState()
{
    CheckTabPathsState();
    CheckTabPassmarksState();
    CheckTabFoldLineState();

    SCASSERT(bOk != nullptr);
    bOk->setEnabled(IsValid());
    // In case dialog hasn't apply button
    if (bApply != nullptr && applyAllowed)
    {
        bApply->setEnabled(bOk->isEnabled());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::closeEvent(QCloseEvent *event)
{
    uiTabPaths->plainTextEditFormulaWidth->blockSignals(true);
    uiTabPaths->plainTextEditFormulaWidthBefore->blockSignals(true);
    uiTabPaths->plainTextEditFormulaWidthAfter->blockSignals(true);
    uiTabGrainline->lineEditRotFormula->blockSignals(true);
    uiTabGrainline->lineEditLenFormula->blockSignals(true);
    uiTabPassmarks->plainTextEditPassmarkLength->blockSignals(true);
    uiTabPassmarks->plainTextEditPassmarkWidth->blockSignals(true);
    uiTabPassmarks->plainTextEditPassmarkAngle->blockSignals(true);
    uiTabPassmarks->plainTextEditPassmarkVisible->blockSignals(true);
    uiTabFoldLine->plainTextEditHeight->blockSignals(true);
    uiTabFoldLine->plainTextEditWidth->blockSignals(true);
    uiTabFoldLine->plainTextEditCenter->blockSignals(true);
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event); // clazy:exclude=skipped-base-method NOLINT(bugprone-parent-virtual-call)
    if (event->spontaneous())
    {
        return;
    }

    if (isInitialized)
    {
        return;
    }
    // do your init stuff here

    const QSize sz = VAbstractApplication::VApp()->Settings()->GetToolSeamAllowanceDialogSize();
    if (not sz.isEmpty())
    {
        resize(sz);
    }

    isInitialized = true; // first show windows are held
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::resizeEvent(QResizeEvent *event)
{
    // remember the size for the next time this dialog is opened, but only
    // if widget was already initialized, which rules out the resize at
    // dialog creating, which would
    if (isInitialized)
    {
        VAbstractApplication::VApp()->Settings()->SetToolSeamAllowanceDialogSize(size());
    }
    DialogTool::resizeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
        InitIcons();
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetPatternDoc(VAbstractPattern *doc)
{
    SCASSERT(doc != nullptr)
    m_doc = doc;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::CheckDependencyTreeComplete()
{
    // does nothing
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::NameDetailChanged()
{
    if (const auto *edit = qobject_cast<QLineEdit *>(sender());edit)
    {
        if (edit->text().isEmpty())
        {
            flagName = false;
            ChangeColor(uiTabPaths->labelEditName, errorColor);
        }
        else
        {
            flagName = true;
            ChangeColor(uiTabPaths->labelEditName, OkColor(this));
        }
    }
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DetailUUIDChanged()
{
    if (const auto *edit = qobject_cast<QLineEdit *>(sender());edit)
    {
        static QRegularExpression const re("^$|^{[0-9a-fA-F]{8}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-[0-9a-fA-F]{4}\\-"
                                           "[0-9a-fA-F]{12}}$");
        QRegularExpressionMatch const match = re.match(edit->text());

        if (match.hasMatch())
        {
            flagUUID = true;
            ChangeColor(uiTabPaths->labelEditUUID, OkColor(this));
        }
        else
        {
            flagUUID = false;
            ChangeColor(uiTabPaths->labelEditUUID, errorColor);
        }
    }
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FancyTabChanged(int index)
{
    if (index < 0 || index >= TabOrder::Count)
    {
        return;
    }

    m_tabPaths->hide();
    m_tabGrainline->hide();
    m_tabPins->hide();
    m_tabPassmarks->hide();
    m_tabPlaceLabels->hide();
    m_tabFoldLine->hide();

    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wswitch-default")
    QT_WARNING_DISABLE_CLANG("-Wswitch-default")

    switch (index) // NOLINT(hicpp-multiway-paths-covered)
    {
        case TabOrder::Paths:
            m_tabPaths->show();
            break;
        case TabOrder::Pins:
            m_tabPins->show();
            break;
        case TabOrder::Grainline:
            m_tabGrainline->show();
            break;
        case TabOrder::Passmarks:
            m_tabPassmarks->show();
            break;
        case TabOrder::PlaceLabels:
            m_tabPlaceLabels->show();
            break;
        case TabOrder::FoldLine:
            m_tabFoldLine->show();
            break;
        case TabOrder::Count:
            Q_UNREACHABLE();
            break;
    }

    QT_WARNING_POP

    if (index == TabOrder::Pins || index == TabOrder::Grainline)
    {
        ShowPieceSpecialPointsWithRect(uiTabPins->listWidgetPins, false);
    }
    else if (index == TabOrder::PlaceLabels)
    {
        ShowPieceSpecialPointsWithRect(uiTabPlaceLabels->listWidgetPlaceLabels, true);
    }
    else
    {
        delete m_visSpecialPoints.data();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EnabledManualFoldHeight()
{
    uiTabFoldLine->toolButtonExprHeight->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());
    uiTabFoldLine->plainTextEditHeight->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());
    uiTabFoldLine->pushButtonGrowHeight->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());
    uiTabFoldLine->labelEditHeight->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());
    uiTabFoldLine->label_3->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());

    EvalFoldHeight();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EnabledManualFoldWidth()
{
    uiTabFoldLine->toolButtonExprWidth->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());
    uiTabFoldLine->plainTextEditWidth->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());
    uiTabFoldLine->pushButtonGrowWidth->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());
    uiTabFoldLine->labelEditWidth->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());
    uiTabFoldLine->label_4->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());

    EvalFoldWidth();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EnabledManualFoldCenter()
{
    uiTabFoldLine->toolButtonExprCenter->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());
    uiTabFoldLine->plainTextEditCenter->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());
    uiTabFoldLine->pushButtonGrowCenter->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());
    uiTabFoldLine->labelEditCenter->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());
    uiTabFoldLine->label_5->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());

    EvalFoldCenter();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalWidth()
{
    // Avoid circular dependency. CurrentSeamAllowance value based on the width field value. On this stage, we must
    // hide from user the existence of CurrentSeamAllowance variable.
    QHash<QString, QSharedPointer<VInternalVariable>> variables = *data.DataVariables();
    variables.remove(currentSeamAllowance);

    m_saWidth = Eval({.formula = uiTabPaths->plainTextEditFormulaWidth->toPlainText(),
                      .variables = &variables,
                      .labelEditFormula = uiTabPaths->labelEditWidth,
                      .labelResult = uiTabPaths->labelResultWidth,
                      .postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true),
                      .checkLessThanZero = true}, flagFormula);

    if (m_saWidth >= 0)
    {
        auto *locData = const_cast<VContainer *>(&data); // NOLINT(cppcoreguidelines-pro-type-const-cast)

        auto *currentSA = new VIncrement(locData, currentSeamAllowance);
        currentSA->SetFormula(m_saWidth, QString().setNum(m_saWidth), true);
        currentSA->SetDescription(tr("Current seam allowance"));

        locData->AddVariable(currentSA);

        EvalWidthBefore();
        EvalWidthAfter();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalWidthBefore()
{
    if (uiTabPaths->checkBoxSeams->isChecked())
    {
        if (uiTabPaths->comboBoxNodes->count() > 0)
        {
            Eval({.formula = uiTabPaths->plainTextEditFormulaWidthBefore->toPlainText(),
                  .variables = data.DataVariables(),
                  .labelEditFormula = uiTabPaths->labelEditBefore,
                  .labelResult = uiTabPaths->labelResultBefore,
                  .postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true),
                  .checkLessThanZero = true}, flagFormulaBefore);

            const QString formulaSABefore = GetFormulaFromUser(uiTabPaths->plainTextEditFormulaWidthBefore);
            UpdateNodeSABefore(formulaSABefore);
            EnableDefButton(uiTabPaths->pushButtonDefBefore, formulaSABefore);
        }
        else
        {
            ChangeColor(uiTabPaths->labelEditBefore, OkColor(this));
            uiTabPaths->labelResultBefore->setText(tr("<Empty>"));
            flagFormulaBefore = true;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalWidthAfter()
{
    if (uiTabPaths->checkBoxSeams->isChecked())
    {
        if (uiTabPaths->comboBoxNodes->count() > 0)
        {
            Eval({.formula = uiTabPaths->plainTextEditFormulaWidthAfter->toPlainText(),
                  .variables = data.DataVariables(),
                  .labelEditFormula = uiTabPaths->labelEditAfter,
                  .labelResult = uiTabPaths->labelResultAfter,
                  .postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true),
                  .checkLessThanZero = true}, flagFormulaAfter);

            const QString formulaSAAfter = GetFormulaFromUser(uiTabPaths->plainTextEditFormulaWidthAfter);
            UpdateNodeSAAfter(formulaSAAfter);
            EnableDefButton(uiTabPaths->pushButtonDefAfter, formulaSAAfter);
        }
        else
        {
            ChangeColor(uiTabPaths->labelEditAfter, OkColor(this));
            uiTabPaths->labelResultAfter->setText(tr("<Empty>"));
            flagFormulaAfter = true;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalPassmarkLength()
{
    if (uiTabPassmarks->groupBoxManualLength->isChecked())
    {
        if (uiTabPassmarks->comboBoxPassmarks->count() > 0)
        {
            Eval({.formula = uiTabPassmarks->plainTextEditPassmarkLength->toPlainText(),
                  .variables = data.DataVariables(),
                  .labelEditFormula = uiTabPassmarks->labelEditPassmarkLength,
                  .labelResult = uiTabPassmarks->labelResultPassmarkLength,
                  .postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true),
                  .checkZero = true,
                  .checkLessThanZero = true}, flagFormulaPassmarkLength);

            UpdateNodePassmarkLength(GetFormulaFromUser(uiTabPassmarks->plainTextEditPassmarkLength));
        }
        else
        {
            ChangeColor(uiTabPassmarks->labelEditPassmarkLength, OkColor(this));
            uiTabPassmarks->labelResultPassmarkLength->setText(tr("<Empty>"));
            flagFormulaPassmarkLength = true;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalPassmarkWidth()
{
    if (uiTabPassmarks->groupBoxManualWidth->isChecked())
    {
        if (uiTabPassmarks->comboBoxPassmarks->count() > 0)
        {
            Eval({.formula = uiTabPassmarks->plainTextEditPassmarkWidth->toPlainText(),
                  .variables = data.DataVariables(),
                  .labelEditFormula = uiTabPassmarks->labelEditPassmarkWidth,
                  .labelResult = uiTabPassmarks->labelResultPassmarkWidth,
                  .postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true),
                  .checkZero = true}, flagFormulaPassmarkWidth);

            UpdateNodePassmarkWidth(GetFormulaFromUser(uiTabPassmarks->plainTextEditPassmarkWidth));
        }
        else
        {
            ChangeColor(uiTabPassmarks->labelEditPassmarkWidth, OkColor(this));
            uiTabPassmarks->labelResultPassmarkWidth->setText(tr("<Empty>"));
            flagFormulaPassmarkWidth = true;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalPassmarkAngle()
{
    if (uiTabPassmarks->groupBoxManualAngle->isChecked())
    {
        if (uiTabPassmarks->comboBoxPassmarks->count() > 0)
        {
            Eval({.formula = uiTabPassmarks->plainTextEditPassmarkAngle->toPlainText(),
                  .variables = data.DataVariables(),
                  .labelEditFormula = uiTabPassmarks->labelEditPassmarkAngle,
                  .labelResult = uiTabPassmarks->labelResultPassmarkAngle,
                  .postfix = degreeSymbol}, flagFormulaPassmarkAngle);

            UpdateNodePassmarkAngle(GetFormulaFromUser(uiTabPassmarks->plainTextEditPassmarkAngle));
        }
        else
        {
            ChangeColor(uiTabPassmarks->labelEditPassmarkAngle, OkColor(this));
            uiTabPassmarks->labelResultPassmarkAngle->setText(tr("<Empty>"));
            flagFormulaPassmarkAngle = true;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalPassmarkVisible()
{
    if (uiTabPassmarks->comboBoxPassmarks->count() > 0)
    {
        Eval({.formula = uiTabPassmarks->plainTextEditPassmarkVisible->toPlainText(),
              .variables = data.DataVariables(),
              .labelEditFormula = uiTabPassmarks->labelEditPassmarkVisible,
              .labelResult = uiTabPassmarks->labelResultPassmarkVisible,
              .checkLessThanZero = true}, flagFormulaPassmarkVisible);

        UpdateNodePassmarkVisibility(GetFormulaFromUser(uiTabPassmarks->plainTextEditPassmarkVisible));
    }
    else
    {
        ChangeColor(uiTabPassmarks->labelEditPassmarkVisible, OkColor(this));
        uiTabPassmarks->labelResultPassmarkVisible->setText(tr("<Empty>"));
        flagFormulaPassmarkVisible = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalFoldHeight()
{
    if (uiTabFoldLine->groupBoxManualHeight->isChecked())
    {
        Eval({.formula = uiTabFoldLine->plainTextEditHeight->toPlainText(),
              .variables = data.DataVariables(),
              .labelEditFormula = uiTabFoldLine->labelEditHeight,
              .labelResult = uiTabFoldLine->labelResultHeight,
              .postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true),
              .checkLessThanZero = true}, flagFormulaFoldHeight);
    }
    else
    {
        CheckState(); // Disable Ok and Apply buttons if something wrong.
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalFoldWidth()
{
    if (uiTabFoldLine->groupBoxManualWidth->isChecked())
    {
        Eval({.formula = uiTabFoldLine->plainTextEditWidth->toPlainText(),
              .variables = data.DataVariables(),
              .labelEditFormula = uiTabFoldLine->labelEditWidth,
              .labelResult = uiTabFoldLine->labelResultWidth,
              .postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true),
              .checkLessThanZero = true}, flagFormulaFoldWidth);
    }
    else
    {
        CheckState(); // Disable Ok and Apply buttons if something wrong.
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EvalFoldCenter()
{
    if (uiTabFoldLine->groupBoxManualCenter->isChecked())
    {
        Eval({.formula = uiTabFoldLine->plainTextEditCenter->toPlainText(),
              .variables = data.DataVariables(),
              .labelEditFormula = uiTabFoldLine->labelEditCenter,
              .labelResult = uiTabFoldLine->labelResultCenter,
              .checkLessThanZero = true}, flagFormulaFoldCenter);
    }
    else
    {
        CheckState(); // Disable Ok and Apply buttons if something wrong.
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXWidth()
{
    // Avoid circular dependency. CurrentSeamAllowance value based on the width field value. On this stage, we must
    // hide from user the existence of CurrentSeamAllowance variable.
    VContainer localData = data;
    localData.RemoveVariable(currentSeamAllowance);

    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&localData, toolId, this));
    dialog->setWindowTitle(tr("Edit seam allowance width"));
    dialog->SetFormula(GetFormulaSAWidth());
    dialog->setCheckLessThanZero(true);
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaSAWidth(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXWidthBefore()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit seam allowance width before"));
    dialog->SetFormula(GetFormulaFromUser(uiTabPaths->plainTextEditFormulaWidthBefore));
    dialog->setCheckLessThanZero(true);
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetCurrentSABefore(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXWidthAfter()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit seam allowance width after"));
    dialog->SetFormula(GetFormulaFromUser(uiTabPaths->plainTextEditFormulaWidthAfter));
    dialog->setCheckLessThanZero(true);
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetCurrentSAAfter(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXPassmarkLength()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit passmark length"));
    dialog->SetFormula(GetFormulaFromUser(uiTabPassmarks->plainTextEditPassmarkLength));
    dialog->setCheckLessThanZero(true);
    dialog->setCheckZero(true);
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaPassmarkLength(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXPassmarkWidth()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit passmark width"));
    dialog->SetFormula(GetFormulaFromUser(uiTabPassmarks->plainTextEditPassmarkWidth));
    dialog->setCheckZero(true);
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaPassmarkWidth(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXPassmarkAngle()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit passmark angle"));
    dialog->SetFormula(GetFormulaFromUser(uiTabPassmarks->plainTextEditPassmarkAngle));
    dialog->setPostfix(degreeSymbol);
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaPassmarkAngle(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXPassmarkVisible()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit notch visibility"));
    dialog->SetFormula(GetFormulaFromUser(uiTabPassmarks->plainTextEditPassmarkVisible));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaPassmarkVisibility(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXFoldHeight()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit fold line height"));
    dialog->SetFormula(GetFormulaFromUser(uiTabFoldLine->plainTextEditHeight));
    dialog->setCheckLessThanZero(true);
    dialog->setCheckZero(true);
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaFoldHeight(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXFoldWidth()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit fold line width"));
    dialog->SetFormula(GetFormulaFromUser(uiTabFoldLine->plainTextEditWidth));
    dialog->setCheckLessThanZero(true);
    dialog->setCheckZero(true);
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaFoldWidth(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::FXFoldCenter()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(&data, toolId, this));
    dialog->setWindowTitle(tr("Edit fold line center position"));
    dialog->SetFormula(GetFormulaFromUser(uiTabFoldLine->plainTextEditCenter));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaFoldCenter(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployWidthFormulaTextEdit()
{
    DeployFormula(this, uiTabPaths->plainTextEditFormulaWidth, uiTabPaths->pushButtonGrowWidth, m_formulaBaseWidth);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployWidthBeforeFormulaTextEdit()
{
    DeployFormula(this, uiTabPaths->plainTextEditFormulaWidthBefore, uiTabPaths->pushButtonGrowWidthBefore,
                  m_formulaBaseWidthBefore);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployWidthAfterFormulaTextEdit()
{
    DeployFormula(this, uiTabPaths->plainTextEditFormulaWidthAfter, uiTabPaths->pushButtonGrowWidthAfter,
                  m_formulaBaseWidthAfter);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployPassmarkLength()
{
    DeployFormula(this, uiTabPassmarks->plainTextEditPassmarkLength, uiTabPassmarks->pushButtonGrowPassmarkLength,
                  m_formulaBasePassmarkLength);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployPassmarkWidth()
{
    DeployFormula(this, uiTabPassmarks->plainTextEditPassmarkWidth, uiTabPassmarks->pushButtonGrowPassmarkWidth,
                  m_formulaBasePassmarkWidth);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployPassmarkAngle()
{
    DeployFormula(this, uiTabPassmarks->plainTextEditPassmarkAngle, uiTabPassmarks->pushButtonGrowPassmarkAngle,
                  m_formulaBasePassmarkAngle);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployPassmarkVisible()
{
    DeployFormula(this, uiTabPassmarks->plainTextEditPassmarkVisible, uiTabPassmarks->pushButtonGrowPassmarkVisible,
                  m_formulaBasePassmarkVisible);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployFoldHeight()
{
    DeployFormula(this, uiTabFoldLine->plainTextEditHeight, uiTabFoldLine->pushButtonGrowHeight,
                  m_formulaBaseFoldHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployFoldWidth()
{
    DeployFormula(this, uiTabFoldLine->plainTextEditWidth, uiTabFoldLine->pushButtonGrowWidth, m_formulaBaseFoldWidth);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployFoldCenter()
{
    DeployFormula(this, uiTabFoldLine->plainTextEditCenter, uiTabFoldLine->pushButtonGrowCenter,
                  m_formulaBaseFoldCenter);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::CreatePiece() const -> VPiece
{
    VPiece piece = data.DataPieces()->value(toolId); // Get options we do not control with the dialog
    piece.GetPath().SetNodes(GetListInternals<VPieceNode>(uiTabPaths->listWidgetMainPath));
    piece.SetCustomSARecords(GetListInternals<CustomSARecord>(uiTabPaths->listWidgetCustomSA));
    piece.SetInternalPaths(GetListInternals<quint32>(uiTabPaths->listWidgetInternalPaths));
    piece.SetPlaceLabels(GetListInternals<quint32>(uiTabPlaceLabels->listWidgetPlaceLabels));
    piece.SetForbidFlipping(uiTabPaths->checkBoxForbidFlipping->isChecked());
    piece.SetForceFlipping(uiTabPaths->checkBoxForceFlipping->isChecked());
    piece.SetSymmetricalCopy(uiTabPaths->checkBoxSymmetricalCopy->isChecked());
    piece.SetFollowGrainline(uiTabPaths->checkBoxFollowGrainline->isChecked());
    piece.SetSewLineOnDrawing(uiTabPaths->checkBoxOnDrawing->isChecked());
    piece.SetSeamAllowance(uiTabPaths->checkBoxSeams->isChecked());
    piece.SetHideMainPath(uiTabPaths->checkBoxHideMainPath->isChecked());
    piece.SetName(uiTabPaths->lineEditName->text());
    piece.SetUUID(uiTabPaths->lineEditUUID->text());
    piece.SetPriority(static_cast<uint>(uiTabPaths->spinBoxPriority->value()));
    piece.SetFormulaSAWidth(GetFormulaFromUser(uiTabPaths->plainTextEditFormulaWidth), m_saWidth);
    piece.SetMirrorLineStartPoint(GetMirrorLineStartPoint());
    piece.SetMirrorLineEndPoint(GetMirrorLineEndPoint());
    piece.SetShowMirrorLine(uiTabPaths->checkBoxShowMirrorLine->isChecked());
    piece.SetShowFullPiece(
        !piece.SeamAllowanceMirrorLine(&data).isNull() ? uiTabPaths->checkBoxShowFullPiece->isChecked() : true);
    piece.SetManualFoldHeight(uiTabFoldLine->groupBoxManualHeight->isChecked());
    piece.SetManualFoldWidth(uiTabFoldLine->groupBoxManualWidth->isChecked());
    piece.SetManualFoldCenter(uiTabFoldLine->groupBoxManualCenter->isChecked());
    piece.SetFormulaFoldHeight(uiTabFoldLine->groupBoxManualHeight->isChecked()
                                   ? GetFormulaFromUser(uiTabFoldLine->plainTextEditHeight)
                                   : QString());
    piece.SetFormulaFoldWidth(uiTabFoldLine->groupBoxManualWidth->isChecked()
                                  ? GetFormulaFromUser(uiTabFoldLine->plainTextEditWidth)
                                  : QString());
    piece.SetFormulaFoldCenter(uiTabFoldLine->groupBoxManualCenter->isChecked()
                                   ? GetFormulaFromUser(uiTabFoldLine->plainTextEditCenter)
                                   : QString());
    piece.SetFoldLineType(static_cast<FoldLineType>(uiTabFoldLine->comboBoxType->currentData().toUInt()));
    piece.SetFoldLineLabel(uiTabFoldLine->lineEditLabel->text());

    piece.GetGrainlineGeometry().SetEnabled(uiTabGrainline->groupBoxGrainline->isChecked());
    piece.GetGrainlineGeometry().SetArrowType(
        static_cast<GrainlineArrowDirection>(uiTabGrainline->comboBoxArrow->currentData().toInt()));
    piece.GetGrainlineGeometry().SetVisible(uiTabGrainline->checkBoxGrainlineVisible->isChecked());

    piece.GetGrainlineGeometry().SetRotation(GetFormulaFromUser(uiTabGrainline->lineEditRotFormula));
    piece.GetGrainlineGeometry().SetLength(GetFormulaFromUser(uiTabGrainline->lineEditLenFormula));

    return piece;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::NewMainPathItem(const VPieceNode &node)
{
    NewNodeItem(uiTabPaths->listWidgetMainPath, node);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SyncMainPathWidget()
{
    QListWidget *const list = uiTabPaths->listWidgetMainPath;
    list->clear();
    for (const auto &node : m_automation.Nodes())
    {
        NewMainPathItem(node);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::GetPathName(quint32 path, bool reverse) const -> QString
{
    QString name;

    if (path > NULL_ID)
    {
        name = CurrentPath(path).GetName();

        if (reverse)
        {
            name = QStringLiteral("- ") + name;
        }
    }

    return name;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::MainPathIsValid() const -> bool
{
    if (CreatePiece().MainPathPoints(&data).count() < 3)
    {
        uiTabPaths->helpLabel->setText(DialogWarningIcon(uiTabPaths->helpLabel) + tr("You need more points!"));
        return false;
    }

    if (not MainPathIsClockwise())
    {
        uiTabPaths->helpLabel->setText(DialogWarningIcon(uiTabPaths->helpLabel) +
                                       tr("You have to choose points in a clockwise direction!"));
        return false;
    }

    QString error;
    if (FirstPointEqualLast(uiTabPaths->listWidgetMainPath, &data, error))
    {
        uiTabPaths->helpLabel->setText(QStringLiteral("%1%2 %3").arg(
            DialogWarningIcon(uiTabPaths->helpLabel), tr("First point cannot be equal to the last point!"), error));
        return false;
    }

    error.clear();
    if (DoublePoints(uiTabPaths->listWidgetMainPath, &data, error))
    {
        uiTabPaths->helpLabel->setText(QStringLiteral("%1%2 %3").arg(DialogWarningIcon(uiTabPaths->helpLabel),
                                                                     tr("You have double points!"), error));
        return false;
    }

    error.clear();
    if (DoubleCurves(uiTabPaths->listWidgetMainPath, &data, error))
    {
        uiTabPaths->helpLabel->setText(QStringLiteral("%1%2 %3").arg(DialogWarningIcon(uiTabPaths->helpLabel),
                                                                     tr("The same curve repeats twice!"), error));
        return false;
    }

    if (not EachPointLabelIsUnique(uiTabPaths->listWidgetMainPath))
    {
        uiTabPaths->helpLabel->setText(DialogWarningIcon(uiTabPaths->helpLabel) +
                                       tr("Each point in the path must be unique!"));
        return false;
    }

    error.clear();
    if (InvalidSegment(uiTabPaths->listWidgetMainPath, &data, error))
    {
        uiTabPaths->helpLabel->setText(
            QStringLiteral("%1%2 %3").arg(DialogWarningIcon(uiTabPaths->helpLabel), tr("Invalid segment!"), error));
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::MirrorLineIsValid() const -> bool
{
    ChangeColor(uiTabPaths->labelMLStartPoint, OkColor(this));
    ChangeColor(uiTabPaths->labelMLEndPoint, OkColor(this));

    const quint32 startPoint = getCurrentObjectId(uiTabPaths->comboBoxMLStartPoint);
    const quint32 endPoint = getCurrentObjectId(uiTabPaths->comboBoxMLEndPoint);

    if (startPoint == NULL_ID && endPoint == NULL_ID)
    {
        return true;
    }

    if (startPoint == NULL_ID && endPoint != NULL_ID)
    {
        ChangeColor(uiTabPaths->labelMLStartPoint, errorColor);
        uiTabPaths->helpLabel->setText(DialogWarningIcon(uiTabPaths->helpLabel) +
                                       tr("Invalid mirror line start point!"));
        return false;
    }

    if (startPoint != NULL_ID && endPoint == NULL_ID)
    {
        ChangeColor(uiTabPaths->labelMLEndPoint, errorColor);
        uiTabPaths->helpLabel->setText(DialogWarningIcon(uiTabPaths->helpLabel) + tr("Invalid mirror line end point!"));
        return false;
    }

    if (startPoint == endPoint)
    {
        ChangeColor(uiTabPaths->labelMLStartPoint, errorColor);
        ChangeColor(uiTabPaths->labelMLEndPoint, errorColor);
        uiTabPaths->helpLabel->setText(DialogWarningIcon(uiTabPaths->helpLabel) +
                                       tr("Start and end mirror line points must be unique!"));
        return false;
    }

    QSet<quint32> uniquePoints;
    for (int i = 0; i < uiTabPaths->listWidgetMainPath->count(); ++i)
    {
        const QListWidgetItem *rowItem = uiTabPaths->listWidgetMainPath->item(i);
        SCASSERT(rowItem != nullptr)
        const auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
        if (rowNode.GetTypeTool() == Tool::NodePoint && not rowNode.IsExcluded())
        {
            uniquePoints.insert(rowNode.GetId());
        }
    }

    if (!uniquePoints.contains(startPoint))
    {
        ChangeColor(uiTabPaths->labelMLStartPoint, errorColor);
        uiTabPaths->helpLabel->setText(DialogWarningIcon(uiTabPaths->helpLabel) +
                                       tr("Invalid mirror line start point!"));
        return false;
    }

    if (!uniquePoints.contains(endPoint))
    {
        ChangeColor(uiTabPaths->labelMLEndPoint, errorColor);
        uiTabPaths->helpLabel->setText(DialogWarningIcon(uiTabPaths->helpLabel) + tr("Invalid mirror line end point!"));
        return false;
    }

    if (!MirrorLinePointsNeighbors(uiTabPaths->listWidgetMainPath, startPoint, endPoint))
    {
        ChangeColor(uiTabPaths->labelMLStartPoint, errorColor);
        ChangeColor(uiTabPaths->labelMLEndPoint, errorColor);
        uiTabPaths->helpLabel->setText(DialogWarningIcon(uiTabPaths->helpLabel) +
                                       tr("Mirror points must be neighbors!"));
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ValidObjects(bool value)
{
    flagMainPathIsValid = value;
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::MainPathIsClockwise() const -> bool
{
    QVector<QPointF> points;
    CastTo(CreatePiece().MainPathPoints(&data), points);

    if (points.count() < 3)
    {
        return false;
    }

    const qreal res = VPiece::SumTrapezoids(points);
    return res < 0;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitNodesList()
{
    const quint32 id = uiTabPaths->comboBoxNodes->currentData().toUInt();

    {
        const QSignalBlocker blocker(uiTabPaths->comboBoxNodes);
        uiTabPaths->comboBoxNodes->clear();

        const QVector<VPieceNode> nodes = GetListInternals<VPieceNode>(uiTabPaths->listWidgetMainPath);

        for (const auto &node : nodes)
        {
            if (node.GetTypeTool() == Tool::NodePoint && not node.IsExcluded())
            {
                const QString name = GetNodeName(&data, node);

                uiTabPaths->comboBoxNodes->addItem(name, node.GetId());
            }
        }
    }

    const int index = uiTabPaths->comboBoxNodes->findData(id);
    if (index != -1)
    {
        uiTabPaths->comboBoxNodes->setCurrentIndex(index);
        NodeChanged(index); // Need in case combox index was not changed
    }
    else
    {
        uiTabPaths->comboBoxNodes->count() > 0 ? NodeChanged(0) : NodeChanged(-1);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::GetItemById(quint32 id) -> QListWidgetItem *
{
    for (qint32 i = 0; i < uiTabPaths->listWidgetMainPath->count(); ++i)
    {
        QListWidgetItem  *item = uiTabPaths->listWidgetMainPath->item(i);
        const auto node = qvariant_cast<VPieceNode>(item->data(Qt::UserRole));

        if (node.GetId() == id)
        {
            return item;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::GetLastId() const -> quint32
{
    const int count = uiTabPaths->listWidgetMainPath->count();
    if (count > 0)
    {
        QListWidgetItem  const*item = uiTabPaths->listWidgetMainPath->item(count - 1);
        const auto node = qvariant_cast<VPieceNode>(item->data(Qt::UserRole));
        return node.GetId();
    }

    return NULL_ID;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetCurrentSABefore(const QString &formula)
{
    UpdateNodeSABefore(formula);
    ListChanged();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetCurrentSAAfter(const QString &formula)
{
    UpdateNodeSAAfter(formula);
    ListChanged();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateNodeSABefore(const QString &formula)
{
    if (const int index = uiTabPaths->comboBoxNodes->currentIndex();index != -1)
    {
        if (QListWidgetItem *rowItem = GetItemById(uiTabPaths->comboBoxNodes->currentData().toUInt()))
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetFormulaSABefore(formula);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateNodeSAAfter(const QString &formula)
{
    if (const int index = uiTabPaths->comboBoxNodes->currentIndex();index != -1)
    {
        if (QListWidgetItem *rowItem = GetItemById(uiTabPaths->comboBoxNodes->currentData().toUInt()))
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetFormulaSAAfter(formula);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateNodePassmarkLength(const QString &formula)
{
    if (const int index = uiTabPassmarks->comboBoxPassmarks->currentIndex();index != -1)
    {
        if (QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt()))
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetFormulaPassmarkLength(formula);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateNodePassmarkWidth(const QString &formula)
{
    if (const int index = uiTabPassmarks->comboBoxPassmarks->currentIndex();index != -1)
    {
        if (QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt()))
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetFormulaPassmarkWidth(formula);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateNodePassmarkAngle(const QString &formula)
{
    if (const int index = uiTabPassmarks->comboBoxPassmarks->currentIndex();index != -1)
    {
        if (QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt()))
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetFormulaPassmarkAngle(formula);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateNodePassmarkVisibility(const QString& formula)
{
    if (const int index = uiTabPassmarks->comboBoxPassmarks->currentIndex(); index != -1)
    {
        if (QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt()))
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetFormulaPassmarkVisibility(formula);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitFancyTabBar()
{
    m_ftb->InsertTab(TabOrder::Paths, QIcon("://icon/32x32/paths.png"), tr("Paths"));
    m_ftb->InsertTab(TabOrder::Pins, QIcon("://icon/32x32/pins.png"), tr("Pins"));
    m_ftb->InsertTab(TabOrder::Grainline, QIcon("://icon/32x32/grainline.png"), tr("Grainline"));
    m_ftb->InsertTab(TabOrder::Passmarks, QIcon("://icon/32x32/passmark.png"), tr("Passmarks"));
    m_ftb->InsertTab(TabOrder::PlaceLabels, QIcon("://icon/32x32/button.png"), tr("Place label"));
    m_ftb->InsertTab(TabOrder::FoldLine, QIcon("://icon/32x32/fold.png"), tr("Fold line"));

    ui->horizontalLayout->addWidget(m_ftb, 0, Qt::AlignLeft);

    m_ftb->SetTabEnabled(TabOrder::Paths, true);

    m_tabPaths->hide();
    uiTabPaths->setupUi(m_tabPaths);
    ui->horizontalLayout->addWidget(m_tabPaths, 1);

    m_tabGrainline->hide();
    uiTabGrainline->setupUi(m_tabGrainline);
    ui->horizontalLayout->addWidget(m_tabGrainline, 1);

    m_tabPins->hide();
    uiTabPins->setupUi(m_tabPins);
    ui->horizontalLayout->addWidget(m_tabPins, 1);

    m_tabPassmarks->hide();
    uiTabPassmarks->setupUi(m_tabPassmarks);
    ui->horizontalLayout->addWidget(m_tabPassmarks, 1);

    m_tabPlaceLabels->hide();
    uiTabPlaceLabels->setupUi(m_tabPlaceLabels);
    ui->horizontalLayout->addWidget(m_tabPlaceLabels, 1);

    m_tabFoldLine->hide();
    uiTabFoldLine->setupUi(m_tabFoldLine);
    ui->horizontalLayout->addWidget(m_tabFoldLine, 1);

    connect(m_ftb, &FancyTabBar::CurrentChanged, this, &DialogSeamAllowance::FancyTabChanged);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitMainPathTab()
{
    uiTabPaths->checkBoxHideMainPath->setChecked(VAbstractApplication::VApp()->Settings()->IsHideMainPath());

    uiTabPaths->listWidgetMainPath->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(uiTabPaths->listWidgetMainPath, &QListWidget::customContextMenuRequested, this,
            &DialogSeamAllowance::ShowMainPathContextMenu);
    connect(uiTabPaths->listWidgetMainPath->model(), &QAbstractItemModel::rowsMoved, this,
            &DialogSeamAllowance::ListChanged);
    connect(uiTabPaths->listWidgetMainPath, &QListWidget::itemSelectionChanged, this,
            &DialogSeamAllowance::SetMoveControls);
    connect(uiTabPaths->listWidgetMainPath, &QListWidget::itemSelectionChanged, this,
            &DialogSeamAllowance::SetOptionControls);

    connect(uiTabPaths->listWidgetMainPath->model(), &QAbstractItemModel::rowsMoved, this,
            [this]() { ValidObjects(MainPathIsValid()); });

    connect(uiTabPaths->toolButtonTop, &QToolButton::clicked, this,
            [this]()
            {
                MoveListRowTop(uiTabPaths->listWidgetMainPath);
                ValidObjects(MainPathIsValid());
            });
    connect(uiTabPaths->toolButtonUp, &QToolButton::clicked, this,
            [this]()
            {
                MoveListRowUp(uiTabPaths->listWidgetMainPath);
                ValidObjects(MainPathIsValid());
            });
    connect(uiTabPaths->toolButtonDown, &QToolButton::clicked, this,
            [this]()
            {
                MoveListRowDown(uiTabPaths->listWidgetMainPath);
                ValidObjects(MainPathIsValid());
            });
    connect(uiTabPaths->toolButtonBottom, &QToolButton::clicked, this,
            [this]()
            {
                MoveListRowBottom(uiTabPaths->listWidgetMainPath);
                ValidObjects(MainPathIsValid());
            });

    connect(uiTabPaths->comboBoxMLStartPoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSeamAllowance::MirrorLinePointChanged);
    connect(uiTabPaths->comboBoxMLEndPoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSeamAllowance::MirrorLinePointChanged);

    connect(uiTabPaths->toolButtonReverse, &QToolButton::toggled, this,
            [this]()
            {
                const int row = uiTabPaths->listWidgetMainPath->currentRow();
                if (row < 0)
                {
                    uiTabPaths->toolButtonReverse->setEnabled(false);
                    return;
                }

                QListWidgetItem *rowItem = uiTabPaths->listWidgetMainPath->item(row);
                SCASSERT(rowItem != nullptr)
                auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

                rowNode.SetReverse(not rowNode.GetReverse());
                rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
                rowItem->setText(GetNodeName(&data, rowNode, true));

                ValidObjects(MainPathIsValid());
                ListChanged();
            });

    connect(uiTabPaths->toolButtonExcluded, &QToolButton::toggled, this,
            [this]()
            {
                const int row = uiTabPaths->listWidgetMainPath->currentRow();
                if (row < 0)
                {
                    uiTabPaths->toolButtonExcluded->setEnabled(false);
                    return;
                }

                QListWidgetItem *rowItem = uiTabPaths->listWidgetMainPath->item(row);
                SCASSERT(rowItem != nullptr)
                auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

                rowNode.SetExcluded(not rowNode.IsExcluded());
                rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
                rowItem->setText(GetNodeName(&data, rowNode, true));
                rowItem->setFont(NodeFont(rowItem->font(), rowNode.IsExcluded()));

                ValidObjects(MainPathIsValid());
                ListChanged();
            });

    connect(uiTabPaths->toolButtonTurnPoint, &QToolButton::toggled, this,
            [this]()
            {
                const int row = uiTabPaths->listWidgetMainPath->currentRow();
                if (row < 0)
                {
                    uiTabPaths->toolButtonTurnPoint->setEnabled(false);
                    return;
                }

                QListWidgetItem *rowItem = uiTabPaths->listWidgetMainPath->item(row);
                SCASSERT(rowItem != nullptr)
                auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

                rowNode.SetTurnPoint(not rowNode.IsTurnPoint());
                rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
                rowItem->setText(GetNodeName(&data, rowNode, true));

                ValidObjects(MainPathIsValid());
                ListChanged();
            });

    connect(uiTabPaths->toolButtonCheckUniqness, &QToolButton::toggled, this,
            [this]()
            {
                const int row = uiTabPaths->listWidgetMainPath->currentRow();
                if (row < 0)
                {
                    uiTabPaths->toolButtonCheckUniqness->setEnabled(false);
                    return;
                }

                QListWidgetItem *rowItem = uiTabPaths->listWidgetMainPath->item(row);
                SCASSERT(rowItem != nullptr)
                auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

                rowNode.SetCheckUniqueness(not rowNode.IsCheckUniqueness());
                rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
                rowItem->setText(GetNodeName(&data, rowNode, true));

                ValidObjects(MainPathIsValid());
                ListChanged();
            });

    connect(uiTabPaths->toolButtonPassmark, &QToolButton::toggled, this,
            [this](bool checked)
            {
                const int row = uiTabPaths->listWidgetMainPath->currentRow();
                if (row < 0)
                {
                    uiTabPaths->toolButtonPassmark->setEnabled(false);
                    return;
                }

                QListWidgetItem *rowItem = uiTabPaths->listWidgetMainPath->item(row);
                SCASSERT(rowItem != nullptr)
                auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

                rowNode.SetPassmark(checked);
                rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
                rowItem->setText(GetNodeName(&data, rowNode, true));

                ValidObjects(MainPathIsValid());
                ListChanged();
            });

    connect(uiTabPaths->toolButtonDuplicateNode, &QToolButton::toggled, this,
            [this]()
            {
                const int row = uiTabPaths->listWidgetMainPath->currentRow();
                if (row < 0)
                {
                    uiTabPaths->toolButtonDuplicateNode->setEnabled(false);
                    return;
                }

                const QListWidgetItem *rowItem = uiTabPaths->listWidgetMainPath->item(row);
                SCASSERT(rowItem != nullptr)
                auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

                VPieceNode duplicate = rowNode;
                duplicate.SetExcluded(true);

                auto *item = new QListWidgetItem(GetNodeName(&data, duplicate, true));
                item->setFont(NodeFont(item->font(), true));
                item->setData(Qt::UserRole, QVariant::fromValue(duplicate));
                uiTabPaths->listWidgetMainPath->addItem(item);

                ValidObjects(MainPathIsValid());
                ListChanged();
            });

    connect(uiTabPaths->toolButtonDelete, &QToolButton::clicked, this,
            [this]()
            {
                const int row = uiTabPaths->listWidgetMainPath->currentRow();
                if (row < 0)
                {
                    uiTabPaths->toolButtonDelete->setEnabled(false);
                    return;
                }

                delete uiTabPaths->listWidgetMainPath->item(row);

                ValidObjects(MainPathIsValid());
                ListChanged();
            });

    uiTabPaths->listWidgetMainPath->installEventFilter(this);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPieceTab()
{
    connect(uiTabPaths->lineEditName, &QLineEdit::textChanged, this, &DialogSeamAllowance::NameDetailChanged);

    uiTabPaths->lineEditName->setText(GetDefaultPieceName());

    connect(uiTabPaths->checkBoxForbidFlipping, CHECKBOX_STATE_CHANGED, this,
            [this](int state)
            {
                if (state == Qt::Checked)
                {
                    uiTabPaths->checkBoxForceFlipping->setChecked(false);
                }
            });

    connect(uiTabPaths->checkBoxForceFlipping, CHECKBOX_STATE_CHANGED, this,
            [this](int state)
            {
                if (state == Qt::Checked)
                {
                    uiTabPaths->checkBoxForbidFlipping->setChecked(false);
                }
            });

    VCommonSettings  const*settings = VAbstractApplication::VApp()->Settings();
    uiTabPaths->checkBoxForbidFlipping->setChecked(settings->GetForbidWorkpieceFlipping());
    uiTabPaths->checkBoxForceFlipping->setChecked(settings->GetForceWorkpieceFlipping());
    uiTabPaths->checkBoxOnDrawing->setChecked(settings->GetSewLineOnDrawing());

    connect(uiTabPaths->lineEditUUID, &QLineEdit::textChanged, this, &DialogSeamAllowance::DetailUUIDChanged);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitSeamAllowanceTab()
{
    this->m_formulaBaseWidth = uiTabPaths->plainTextEditFormulaWidth->height();
    this->m_formulaBaseWidthBefore = uiTabPaths->plainTextEditFormulaWidthBefore->height();
    this->m_formulaBaseWidthAfter = uiTabPaths->plainTextEditFormulaWidthAfter->height();

    uiTabPaths->plainTextEditFormulaWidth->installEventFilter(this);
    uiTabPaths->plainTextEditFormulaWidthBefore->installEventFilter(this);
    uiTabPaths->plainTextEditFormulaWidthAfter->installEventFilter(this);

    m_timerWidth->setSingleShot(true);
    connect(m_timerWidth, &QTimer::timeout, this, &DialogSeamAllowance::EvalWidth);

    m_timerWidthBefore->setSingleShot(true);
    connect(m_timerWidthBefore, &QTimer::timeout, this, &DialogSeamAllowance::EvalWidthBefore);

    m_timerWidthAfter->setSingleShot(true);
    connect(m_timerWidthAfter, &QTimer::timeout, this, &DialogSeamAllowance::EvalWidthAfter);

    connect(uiTabPaths->checkBoxSeams, &QCheckBox::toggled, this,
            [this](bool enable)
            {
                uiTabPaths->checkBoxBuiltIn->setEnabled(enable);

                if (not enable)
                {
                    uiTabPaths->groupBoxAutomatic->setEnabled(enable);
                    uiTabPaths->groupBoxCustom->setEnabled(enable);
                }
                else
                {
                    emit uiTabPaths->checkBoxBuiltIn->toggled(uiTabPaths->checkBoxBuiltIn->isChecked());
                }
            });

    connect(uiTabPaths->checkBoxBuiltIn, &QCheckBox::toggled, this,
            [this](bool enable)
            {
                uiTabPaths->groupBoxAutomatic->setEnabled(not enable);
                uiTabPaths->groupBoxCustom->setEnabled(not enable);

                if (not enable)
                {
                    InitNodesList();
                }
            });

    // init the default seam allowance, convert the value if app unit is different than pattern unit
    m_saWidth = UnitConvertor(VAbstractApplication::VApp()->Settings()->GetDefaultSeamAllowance(),
                              StrToUnits(VAbstractApplication::VApp()->Settings()->GetUnit()),
                              VAbstractValApplication::VApp()->patternUnits());

    uiTabPaths->plainTextEditFormulaWidth->setPlainText(VAbstractApplication::VApp()->LocaleToString(m_saWidth));

    InitNodesList();
    connect(uiTabPaths->comboBoxNodes, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSeamAllowance::NodeChanged);

    connect(uiTabPaths->pushButtonDefBefore, &QPushButton::clicked, this, &DialogSeamAllowance::ReturnDefBefore);
    connect(uiTabPaths->pushButtonDefAfter, &QPushButton::clicked, this, &DialogSeamAllowance::ReturnDefAfter);

    InitNodeAngles(uiTabPaths->comboBoxAngle);
    connect(uiTabPaths->comboBoxAngle, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSeamAllowance::NodeAngleChanged);

    uiTabPaths->listWidgetCustomSA->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(uiTabPaths->listWidgetCustomSA, &QListWidget::customContextMenuRequested, this,
            &DialogSeamAllowance::ShowCustomSAContextMenu);
    connect(uiTabPaths->listWidgetCustomSA, &QListWidget::currentRowChanged, this,
            &DialogSeamAllowance::CustomSAChanged);
    connect(uiTabPaths->comboBoxStartPoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSeamAllowance::CSAStartPointChanged);
    connect(uiTabPaths->comboBoxEndPoint, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSeamAllowance::CSAEndPointChanged);
    connect(uiTabPaths->comboBoxIncludeType, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSeamAllowance::CSAIncludeTypeChanged);

    connect(uiTabPaths->toolButtonExprWidth, &QPushButton::clicked, this, &DialogSeamAllowance::FXWidth);
    connect(uiTabPaths->toolButtonExprBefore, &QPushButton::clicked, this, &DialogSeamAllowance::FXWidthBefore);
    connect(uiTabPaths->toolButtonExprAfter, &QPushButton::clicked, this, &DialogSeamAllowance::FXWidthAfter);

    connect(uiTabPaths->plainTextEditFormulaWidth, &QPlainTextEdit::textChanged, this,
            [this]() { m_timerWidth->start(formulaTimerTimeout); });

    connect(uiTabPaths->plainTextEditFormulaWidthBefore, &QPlainTextEdit::textChanged, this,
            [this]() { m_timerWidthBefore->start(formulaTimerTimeout); });

    connect(uiTabPaths->plainTextEditFormulaWidthAfter, &QPlainTextEdit::textChanged, this,
            [this]() { m_timerWidthAfter->start(formulaTimerTimeout); });

    connect(uiTabPaths->pushButtonGrowWidth, &QPushButton::clicked, this,
            &DialogSeamAllowance::DeployWidthFormulaTextEdit);
    connect(uiTabPaths->pushButtonGrowWidthBefore, &QPushButton::clicked, this,
            &DialogSeamAllowance::DeployWidthBeforeFormulaTextEdit);
    connect(uiTabPaths->pushButtonGrowWidthAfter, &QPushButton::clicked, this,
            &DialogSeamAllowance::DeployWidthAfterFormulaTextEdit);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitCSAPoint(QComboBox *box)
{
    SCASSERT(box != nullptr);
    box->clear();
    box->addItem(tr("Empty"), NULL_ID);

    const QVector<VPieceNode> nodes = GetListInternals<VPieceNode>(uiTabPaths->listWidgetMainPath);

    for (const auto &node : nodes)
    {
        if (node.GetTypeTool() == Tool::NodePoint && not node.IsExcluded())
        {
            const QString name = GetNodeName(&data, node);
            box->addItem(name, node.GetId());
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPinPoint(QComboBox *box)
{
    SCASSERT(box != nullptr);

    quint32 currentId = NULL_ID;
    if (box->count() > 0)
    {
        currentId = box->currentData().toUInt();
    }

    box->clear();
    box->addItem('<'_L1 + tr("no pin") + '>'_L1, NULL_ID);

    const QVector<quint32> pins = GetListInternals<quint32>(uiTabPins->listWidgetPins);

    for (auto pin : pins)
    {
        box->addItem(data.GetGObject(pin)->name(), pin);
    }

    const int index = box->findData(currentId);
    if (index != -1)
    {
        box->setCurrentIndex(index);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitMirrorLine()
{
    const QVariant idStartPoint = uiTabPaths->comboBoxMLStartPoint->currentData();
    const QVariant idEndPoint = uiTabPaths->comboBoxMLEndPoint->currentData();

    InitMirrorLinePoint(uiTabPaths->comboBoxMLStartPoint);
    InitMirrorLinePoint(uiTabPaths->comboBoxMLEndPoint);

    uiTabPaths->comboBoxMLStartPoint->setCurrentIndex(uiTabPaths->comboBoxMLStartPoint->findData(idStartPoint));
    uiTabPaths->comboBoxMLEndPoint->setCurrentIndex(uiTabPaths->comboBoxMLEndPoint->findData(idEndPoint));

    flagMirrorLineIsValid = MirrorLineIsValid();
    uiTabPaths->checkBoxShowFullPiece->setEnabled(flagMirrorLineIsValid);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitMirrorLinePoint(QComboBox *box)
{
    SCASSERT(box != nullptr);
    const QSignalBlocker blocker(box);
    box->clear();

    box->addItem(tr("None"), QUuid());

    const QVector<VPieceNode> nodes = GetListInternals<VPieceNode>(uiTabPaths->listWidgetMainPath);

    for (const auto &node : nodes)
    {
        if (node.GetTypeTool() == Tool::NodePoint && not node.IsExcluded())
        {
            const QString name = GetNodeName(&data, node);
            box->addItem(name, node.GetId());
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitSAIncludeType()
{
    uiTabPaths->comboBoxIncludeType->clear();

    uiTabPaths->comboBoxIncludeType->addItem(tr("main path"),
                                             static_cast<unsigned char>(PiecePathIncludeType::AsMainPath));
    uiTabPaths->comboBoxIncludeType->addItem(tr("custom seam allowance"),
                                             static_cast<unsigned char>(PiecePathIncludeType::AsCustomSA));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitInternalPathsTab()
{
    uiTabPaths->listWidgetInternalPaths->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(uiTabPaths->listWidgetInternalPaths, &QListWidget::customContextMenuRequested, this,
            &DialogSeamAllowance::ShowInternalPathsContextMenu);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPinsTab()
{
    uiTabPins->listWidgetPins->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(uiTabPins->listWidgetPins, &QListWidget::customContextMenuRequested, this,
            &DialogSeamAllowance::ShowPinsContextMenu);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPlaceLabelsTab()
{
    uiTabPlaceLabels->listWidgetPlaceLabels->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(uiTabPlaceLabels->listWidgetPlaceLabels, &QListWidget::currentRowChanged, this,
            [this]()
            {
                if (not m_visSpecialPoints.isNull())
                {
                    m_visSpecialPoints->SetShowRect(true);
                    m_visSpecialPoints->SetRect(CurrentRect());
                    m_visSpecialPoints->RefreshGeometry();
                }
            });
    connect(uiTabPlaceLabels->listWidgetPlaceLabels, &QListWidget::customContextMenuRequested, this,
            &DialogSeamAllowance::ShowPlaceLabelsContextMenu);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitAllPinComboboxes()
{
    InitPinPoint(uiTabGrainline->comboBoxGrainlineCenterPin);
    InitPinPoint(uiTabGrainline->comboBoxGrainlineTopPin);
    InitPinPoint(uiTabGrainline->comboBoxGrainlineBottomPin);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::GetFormulaSAWidth() const -> QString
{
    QString const width = uiTabPaths->plainTextEditFormulaWidth->toPlainText();
    return VTranslateVars::TryFormulaFromUser(width, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::UndoStack() -> QVector<QPointer<VUndoCommand>> &
{
    return m_undoStack;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetFormulaSAWidth(const QString &formula)
{
    const QString width = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (width.length() > 80)
    {
        this->DeployWidthFormulaTextEdit();
    }
    uiTabPaths->plainTextEditFormulaWidth->setPlainText(width);

    if (not applyAllowed)
    {
        auto *path = qobject_cast<VisToolPiece *>(vis);
        SCASSERT(path != nullptr)
        const VPiece p = CreatePiece();
        path->SetPiece(p);
    }

    MoveCursorToEnd(uiTabPaths->plainTextEditFormulaWidth);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetFormulaPassmarkLength(const QString &formula)
{
    const QString width = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (width.length() > 80)
    {
        this->DeployPassmarkLength();
    }
    uiTabPassmarks->plainTextEditPassmarkLength->setPlainText(width);

    MoveCursorToEnd(uiTabPassmarks->plainTextEditPassmarkLength);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetFormulaPassmarkWidth(const QString &formula)
{
    const QString width = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (width.length() > 80)
    {
        this->DeployPassmarkWidth();
    }
    uiTabPassmarks->plainTextEditPassmarkWidth->setPlainText(width);

    MoveCursorToEnd(uiTabPassmarks->plainTextEditPassmarkWidth);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetFormulaPassmarkAngle(const QString &formula)
{
    const QString angle = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (angle.length() > 80)
    {
        this->DeployPassmarkAngle();
    }
    uiTabPassmarks->plainTextEditPassmarkAngle->setPlainText(angle);

    MoveCursorToEnd(uiTabPassmarks->plainTextEditPassmarkAngle);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetFormulaPassmarkVisibility(const QString& formula)
{
    const QString visibility = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (visibility.length() > 80)
    {
        this->DeployPassmarkVisible();
    }
    uiTabPassmarks->plainTextEditPassmarkVisible->setPlainText(visibility);

    MoveCursorToEnd(uiTabPassmarks->plainTextEditPassmarkVisible);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetFormulaFoldHeight(const QString &formula)
{
    const QString width = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (width.length() > 80)
    {
        this->DeployFoldHeight();
    }
    uiTabFoldLine->plainTextEditHeight->setPlainText(width);

    MoveCursorToEnd(uiTabFoldLine->plainTextEditHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetFormulaFoldWidth(const QString &formula)
{
    const QString width = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (width.length() > 80)
    {
        this->DeployFoldWidth();
    }
    uiTabFoldLine->plainTextEditWidth->setPlainText(width);

    MoveCursorToEnd(uiTabFoldLine->plainTextEditWidth);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetFormulaFoldCenter(const QString &formula)
{
    const QString width = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (width.length() > 80)
    {
        this->DeployFoldCenter();
    }
    uiTabFoldLine->plainTextEditCenter->setPlainText(width);

    MoveCursorToEnd(uiTabFoldLine->plainTextEditCenter);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateCurrentCustomSARecord()
{
    const int row = uiTabPaths->listWidgetCustomSA->currentRow();
    if (uiTabPaths->listWidgetCustomSA->count() == 0 || row == -1)
    {
        return;
    }

    QListWidgetItem *item = uiTabPaths->listWidgetCustomSA->item(row);
    SCASSERT(item != nullptr);
    const auto record = qvariant_cast<CustomSARecord>(item->data(Qt::UserRole));
    item->setText(GetPathName(record.path, record.reverse));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateCurrentInternalPathRecord()
{
    const int row = uiTabPaths->listWidgetInternalPaths->currentRow();
    if (uiTabPaths->listWidgetInternalPaths->count() == 0 || row == -1)
    {
        return;
    }

    QListWidgetItem *item = uiTabPaths->listWidgetInternalPaths->item(row);
    SCASSERT(item != nullptr);
    const auto path = qvariant_cast<quint32>(item->data(Qt::UserRole));
    item->setText(GetPathName(path));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateCurrentPlaceLabelRecords()
{
    const int row = uiTabPlaceLabels->listWidgetPlaceLabels->currentRow();
    if (uiTabPlaceLabels->listWidgetPlaceLabels->count() == 0 || row == -1)
    {
        return;
    }

    QListWidgetItem *item = uiTabPlaceLabels->listWidgetPlaceLabels->item(row);
    SCASSERT(item != nullptr);
    const auto labelId = qvariant_cast<quint32>(item->data(Qt::UserRole));
    item->setText(CurrentPlaceLabel(labelId).name());
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::CurrentRect() const -> QRectF
{
    QRectF rect;
    if (QListWidgetItem  const*item = uiTabPlaceLabels->listWidgetPlaceLabels->currentItem())
    {
        VPlaceLabelItem const label = CurrentPlaceLabel(qvariant_cast<quint32>(item->data(Qt::UserRole)));
        rect = QRectF(QPointF(label.x() - label.GetWidth() / 2.0, label.y() - label.GetHeight() / 2.0),
                      QPointF(label.x() + label.GetWidth() / 2.0, label.y() + label.GetHeight() / 2.0));
    }
    return rect;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ShowPieceSpecialPointsWithRect(const QListWidget *list, bool showRect)
{
    SCASSERT(list != nullptr)
    if (m_visSpecialPoints.isNull())
    {
        m_visSpecialPoints = new VisPieceSpecialPoints(&data);
    }

    m_visSpecialPoints->SetSpecialPoints(GetListInternals<quint32>(list));
    m_visSpecialPoints->SetShowRect(showRect);
    m_visSpecialPoints->SetRect(CurrentRect());

    if (not VAbstractValApplication::VApp()->getCurrentScene()->items().contains(m_visSpecialPoints))
    {
        m_visSpecialPoints->VisualMode(NULL_ID);
        m_visSpecialPoints->setZValue(10); // pins should be on top
        auto *tool = qobject_cast<VToolSeamAllowance *>(VAbstractPattern::getTool(toolId));
        SCASSERT(tool != nullptr);
        m_visSpecialPoints->setParentItem(tool);
    }
    else
    {
        m_visSpecialPoints->RefreshGeometry();
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::CurrentPath(quint32 id) const -> VPiecePath
{
    return m_newPaths.contains(id) ? m_newPaths.value(id) : data.GetPiecePath(id);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::CurrentPlaceLabel(quint32 id) const -> VPlaceLabelItem
{
    return m_newPlaceLabels.contains(id) ? m_newPlaceLabels.value(id) : *data.GeometricObject<VPlaceLabelItem>(id);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSeamAllowance::GetDefaultPieceName() const -> QString
{
    return DefaultPieceName(&data);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetMoveControls()
{
    uiTabPaths->toolButtonTop->setEnabled(false);
    uiTabPaths->toolButtonUp->setEnabled(false);
    uiTabPaths->toolButtonDown->setEnabled(false);
    uiTabPaths->toolButtonBottom->setEnabled(false);

    if (uiTabPaths->listWidgetMainPath->count() >= 2)
    {
        if (uiTabPaths->listWidgetMainPath->currentRow() == 0)
        {
            uiTabPaths->toolButtonDown->setEnabled(true);
            uiTabPaths->toolButtonBottom->setEnabled(true);
        }
        else if (uiTabPaths->listWidgetMainPath->currentRow() == uiTabPaths->listWidgetMainPath->count() - 1)
        {
            uiTabPaths->toolButtonTop->setEnabled(true);
            uiTabPaths->toolButtonUp->setEnabled(true);
        }
        else
        {
            uiTabPaths->toolButtonTop->setEnabled(true);
            uiTabPaths->toolButtonUp->setEnabled(true);
            uiTabPaths->toolButtonDown->setEnabled(true);
            uiTabPaths->toolButtonBottom->setEnabled(true);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetOptionControls()
{
    uiTabPaths->toolButtonReverse->setEnabled(false);
    uiTabPaths->toolButtonExcluded->setEnabled(false);
    uiTabPaths->toolButtonTurnPoint->setEnabled(false);
    uiTabPaths->toolButtonCheckUniqness->setEnabled(false);
    uiTabPaths->toolButtonPassmark->setEnabled(false);
    uiTabPaths->toolButtonDuplicateNode->setEnabled(false);
    uiTabPaths->toolButtonDelete->setEnabled(false);

    auto SetChecked = [](QToolButton *toolButton, bool checked = false)
    {
        const QSignalBlocker blocker(toolButton);
        toolButton->setChecked(checked);
    };

    SetChecked(uiTabPaths->toolButtonReverse);
    SetChecked(uiTabPaths->toolButtonExcluded);
    SetChecked(uiTabPaths->toolButtonTurnPoint);
    SetChecked(uiTabPaths->toolButtonCheckUniqness);
    SetChecked(uiTabPaths->toolButtonPassmark);
    SetChecked(uiTabPaths->toolButtonDuplicateNode);
    SetChecked(uiTabPaths->toolButtonDelete);

    const int row = uiTabPaths->listWidgetMainPath->currentRow();
    if (row < 0)
    {
        return;
    }

    const QListWidgetItem *rowItem = uiTabPaths->listWidgetMainPath->item(row);
    SCASSERT(rowItem != nullptr)
    auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

    if (rowNode.GetTypeTool() != Tool::NodePoint)
    {
        uiTabPaths->toolButtonReverse->setEnabled(true);
        uiTabPaths->toolButtonDuplicateNode->setEnabled(true);
        SetChecked(uiTabPaths->toolButtonReverse, rowNode.GetReverse());
    }
    else
    {
        uiTabPaths->toolButtonPassmark->setEnabled(true);
        SetChecked(uiTabPaths->toolButtonPassmark, rowNode.IsPassmark());

        uiTabPaths->toolButtonCheckUniqness->setEnabled(true);
        SetChecked(uiTabPaths->toolButtonCheckUniqness, rowNode.IsCheckUniqueness());

        uiTabPaths->toolButtonTurnPoint->setEnabled(true);
        SetChecked(uiTabPaths->toolButtonTurnPoint, rowNode.IsTurnPoint());
    }

    uiTabPaths->toolButtonExcluded->setEnabled(true);
    SetChecked(uiTabPaths->toolButtonExcluded, rowNode.IsExcluded());

    uiTabPaths->toolButtonDelete->setEnabled(true);
}

