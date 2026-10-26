/************************************************************************
 **
 **  @file   dialogmove.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   30 9, 2016
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

#include "dialogmove.h"

#include <QAbstractButton>
#include <QColor>
#include <QComboBox>
#include <QCompleter>
#include <QDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QLineF>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPointF>
#include <QPointer>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSharedPointer>
#include <QStringList>
#include <QTimer>
#include <QToolButton>

#include "../../visualization/line/operation/vistoolmove.h"
#include "../../visualization/visualization.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../qmuparser/qmudef.h"
#include "../support/dialogeditwrongformula.h"
#include "vtheme.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vpropertyexplorer/qtcolorpicker.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../vwidgets/vlineedit.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../vwidgets/vmaingraphicsview.h"
#include "ui_dialogmove.h"

#include <QTabBar>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
DialogMove::DialogMove(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogOperationTool(data, doc, toolId, parent),
    ui(new Ui::DialogMove),
    timerAngle(new QTimer(this)),
    timerRotationAngle(new QTimer(this)),
    timerLength(new QTimer(this))
{
    ui->setupUi(this);

    InitIcons();


    this->formulaBaseHeightAngle = ui->plainTextEditAngle->height();
    ui->plainTextEditAngle->installEventFilter(this);

    this->formulaBaseHeightRotationAngle = ui->plainTextEditRotationAngle->height();
    ui->plainTextEditRotationAngle->installEventFilter(this);

    this->formulaBaseHeightLength = ui->plainTextEditLength->height();
    ui->plainTextEditLength->installEventFilter(this);

    timerAngle->setSingleShot(true);
    connect(timerAngle, &QTimer::timeout, this, &DialogMove::EvalAngle);

    timerRotationAngle->setSingleShot(true);
    connect(timerRotationAngle, &QTimer::timeout, this, &DialogMove::EvalRotationAngle);

    timerLength->setSingleShot(true);
    connect(timerLength, &QTimer::timeout, this, &DialogMove::EvalLength);

    InitOkCancelApply(ui);


    FillComboBoxPoints(ui->comboBoxRotationOriginPoint);

    {
        const QSignalBlocker blocker(ui->comboBoxRotationOriginPoint);
        ui->comboBoxRotationOriginPoint->addItem(tr("Center point"), NULL_ID);
    }

    connect(ui->lineEditVisibilityGroup, &QLineEdit::textChanged, this, &DialogOperationTool::GroupNameChanged);
    connect(ui->toolButtonExprAngle, &QPushButton::clicked, this, &DialogMove::FXAngle);
    connect(ui->toolButtonExprRotationAngle, &QPushButton::clicked, this, &DialogMove::FXRotationAngle);
    connect(ui->toolButtonExprLength, &QPushButton::clicked, this, &DialogMove::FXLength);
    connect(ui->plainTextEditAngle,
            &QPlainTextEdit::textChanged,
            this,
            [this]() -> void { timerAngle->start(formulaTimerTimeout); });

    connect(ui->plainTextEditRotationAngle,
            &QPlainTextEdit::textChanged,
            this,
            [this]() -> void { timerRotationAngle->start(formulaTimerTimeout); });

    connect(ui->plainTextEditLength,
            &QPlainTextEdit::textChanged,
            this,
            [this]() -> void { timerLength->start(formulaTimerTimeout); });

    connect(ui->pushButtonGrowAngle, &QPushButton::clicked, this, &DialogMove::DeployAngleTextEdit);
    connect(ui->pushButtonGrowRotationAngle, &QPushButton::clicked, this, &DialogMove::DeployRotationAngleTextEdit);
    connect(ui->pushButtonGrowLength, &QPushButton::clicked, this, &DialogMove::DeployLengthTextEdit);



    vis = new VisToolMove(&this->data);

    SetRotationOrigPointId(NULL_ID);

    ui->tabWidget->setCurrentIndex(0);
    if (ui->tabWidget->count() <= 1)
    {
        ui->tabWidget->tabBar()->hide(); // Single-page creation/editing window.
    }
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogMove::~DialogMove()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogMove::Widgets() const -> OperationWidgets
{
    return OperationWidgetsFrom(ui);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogMove::GetAngle() const -> QString
{
    return VTranslateVars::TryFormulaFromUser(formulaAngle, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::SetAngle(const QString &value)
{
    formulaAngle = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        value, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (formulaAngle.length() > 80)
    {
        this->DeployAngleTextEdit();
    }
    ui->plainTextEditAngle->setPlainText(formulaAngle);

    if (auto *operation = qobject_cast<VisToolMove *>(vis))
    {
        operation->SetAngle(formulaAngle);
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }

    MoveCursorToEnd(ui->plainTextEditAngle);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogMove::GetLength() const -> QString
{
    return VTranslateVars::TryFormulaFromUser(formulaLength,
                                              VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::SetLength(const QString &value)
{
    formulaLength = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        value, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (formulaLength.length() > 80)
    {
        this->DeployLengthTextEdit();
    }
    ui->plainTextEditLength->setPlainText(formulaLength);

    if (auto *operation = qobject_cast<VisToolMove *>(vis))
    {
        operation->SetLength(formulaLength);
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }

    MoveCursorToEnd(ui->plainTextEditLength);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogMove::GetRotationAngle() const -> QString
{
    return VTranslateVars::TryFormulaFromUser(formulaRotationAngle,
                                              VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::SetRotationAngle(const QString &value)
{
    formulaRotationAngle = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        value, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (formulaRotationAngle.length() > 80)
    {
        this->DeployAngleTextEdit();
    }
    ui->plainTextEditRotationAngle->setPlainText(formulaRotationAngle);

    if (auto *operation = qobject_cast<VisToolMove *>(vis))
    {
        operation->SetRotationAngle(formulaRotationAngle);
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }

    MoveCursorToEnd(ui->plainTextEditRotationAngle);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogMove::GetRotationOrigPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxRotationOriginPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::SetRotationOrigPointId(const quint32 &value)
{
    ChangeCurrentData(ui->comboBoxRotationOriginPoint, value);
    if (auto *operation = qobject_cast<VisToolMove *>(vis))
    {
        operation->SetRotationOriginPointId(value);
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::ShowDialog(bool click)
{
    if (IsStage1() && not click)
    {
        if (SourceObjects().isEmpty())
        {
            return;
        }

        SetStage1(false);
        prepare = true;

        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)
        scene->clearSelection();

        auto *operation = qobject_cast<VisToolMove *>(vis);
        SCASSERT(operation != nullptr)
        operation->StartAction();
        auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
        SCASSERT(window != nullptr)
        connect(operation, &VisToolMove::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

        scene->ToggleArcSegmentVisibility(false);
        scene->ToggleElArcSegmentVisibility(false);
        scene->ToggleSplineSegmentVisibility(false);
        scene->ToggleSplinePathSegmentVisibility(false);

        scene->ToggleArcHover(false);
        scene->ToggleElArcHover(false);
        scene->ToggleSplineHover(false);
        scene->ToggleSplinePathHover(false);

        VAbstractValApplication::VApp()->getSceneView()->AllowRubberBand(false);

        FillDefSourceNames(SourceObjects(), &data, CopySuffix::Move);
    }
    else if (not stage2 && not IsStage1() && prepare && click)
    {
        auto *operation = qobject_cast<VisToolMove *>(vis);
        SCASSERT(operation != nullptr)

        if (operation->LengthValue() > 0)
        {
            formulaAngle = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
                operation->Angle(), VAbstractApplication::VApp()->Settings()->GetOsSeparator());
            formulaLength = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
                operation->Length(), VAbstractApplication::VApp()->Settings()->GetOsSeparator());
            operation->SetAngle(formulaAngle);
            operation->SetLength(formulaLength);
            operation->RefreshGeometry();
            emit ToolTip(operation->CurrentToolTip());
            stage2 = true;
        }
    }
    else if (not IsStage1() && stage2 && prepare && click)
    {
        auto *operation = qobject_cast<VisToolMove *>(vis);
        SCASSERT(operation != nullptr)

        if (QGuiApplication::keyboardModifiers() == Qt::ControlModifier)
        {
            if (not optionalRotationOrigin)
            {
                operation->SetRotationOriginPointId(NULL_ID);
                SetObject(NULL_ID, ui->comboBoxRotationOriginPoint, QString());
                operation->RefreshGeometry();
            }
            optionalRotationOrigin = false; // Handled, next click on empty field will disable selection
        }
        else
        {
            SetAngle(VAbstractApplication::VApp()->TrVars()->FormulaFromUser(
                formulaAngle, VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
            SetLength(VAbstractApplication::VApp()->TrVars()->FormulaFromUser(
                formulaLength, VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
            SetRotationAngle(operation->RotationAngle());
            setModal(true);
            emit ToolTip(QString());
            timerAngle->start();
            timerRotationAngle->start();
            timerLength->start();
            show();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::ChosenObject(quint32 id, const SceneObject &type)
{
    if (IsStage1())
    {
        if (type == SceneObject::Line)
        {
            // A line is not an operated object: it travels beside them, and its two endpoints decide
            // whether it can be transformed at all.
            ToggleLineSource(id);
        }
        else
        {
            ToggleSourceObject(id);
        }

        // Both cases must refresh the preview: a line used to return right here, so neither its highlight
        // nor its ghost could ever appear.
        auto *operation = qobject_cast<VisToolMove *>(vis);
        SCASSERT(operation != nullptr)
        operation->SetObjects(SourceToObjects(SourceObjects()));
        operation->SetPreviewLines(PreviewLines());
        if (!VAbstractValApplication::VApp()->getCurrentScene()->items().contains(operation))
        {
            operation->VisualMode(NULL_ID);
        }
        else
        {
            operation->RefreshGeometry();
        }
    }
    else if (stage2 && prepare && type == SceneObject::Point
             && QGuiApplication::keyboardModifiers() == Qt::ControlModifier
             && SetObject(id, ui->comboBoxRotationOriginPoint, QString())) // After first choose we ignore all objects
    {
        auto *operation = qobject_cast<VisToolMove *>(vis);
        SCASSERT(operation != nullptr)

        operation->SetRotationOriginPointId(id);
        operation->RefreshGeometry();
        optionalRotationOrigin = true;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::DeployAngleTextEdit()
{
    DeployFormula(this, ui->plainTextEditAngle, ui->pushButtonGrowAngle, formulaBaseHeightAngle);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::DeployRotationAngleTextEdit()
{
    DeployFormula(this, ui->plainTextEditRotationAngle, ui->pushButtonGrowRotationAngle,
                  formulaBaseHeightRotationAngle);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::DeployLengthTextEdit()
{
    DeployFormula(this, ui->plainTextEditLength, ui->pushButtonGrowLength, formulaBaseHeightLength);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::FXAngle()
{
    auto *dialog = new DialogEditWrongFormula(&data, toolId, this);
    dialog->setWindowTitle(tr("Edit angle"));
    dialog->SetFormula(GetAngle());
    dialog->setPostfix(degreeSymbol);
    if (dialog->exec() == QDialog::Accepted)
    {
        SetAngle(dialog->GetFormula());
    }
    delete dialog;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::FXRotationAngle()
{
    auto *dialog = new DialogEditWrongFormula(&data, toolId, this);
    dialog->setWindowTitle(tr("Edit rotation angle"));
    dialog->SetFormula(GetRotationAngle());
    dialog->setPostfix(degreeSymbol);
    if (dialog->exec() == QDialog::Accepted)
    {
        SetRotationAngle(dialog->GetFormula());
    }
    delete dialog;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::FXLength()
{
    auto *dialog = new DialogEditWrongFormula(&data, toolId, this);
    dialog->setWindowTitle(tr("Edit length"));
    dialog->SetFormula(GetLength());
    dialog->setPostfix(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
    if (dialog->exec() == QDialog::Accepted)
    {
        SetLength(dialog->GetFormula());
    }
    delete dialog;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::OnSourceObjectsSet()
{
    if (auto *operation = qobject_cast<VisToolMove *>(vis))
    {
        operation->SetObjects(SourceToObjects(SourceObjects()));
        operation->SetPreviewLines(PreviewLines());
        operation->RefreshGeometry();
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::ShowVisualization()
{
    if (vis.isNull())
    {
        // A full parse clears the scene and takes this preview with it, while the dialog stays up (see
        // DialogTool::RebuildVisualization): it is built again, and the values the dialog shows follow.
        vis = new VisToolMove(&this->data);
    }

    AddVisualization<VisToolMove>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::SaveData()
{
    formulaAngle = ui->plainTextEditAngle->toPlainText();
    formulaRotationAngle = ui->plainTextEditRotationAngle->toPlainText();
    formulaLength = ui->plainTextEditLength->toPlainText();

    if (!vis.isNull())
    {
        auto *operation = qobject_cast<VisToolMove *>(vis);
        SCASSERT(operation != nullptr)

        operation->SetObjects(SourceToObjects(SourceObjects()));

        operation->SetPreviewLines(PreviewLines());
        operation->SetAngle(formulaAngle);
        operation->SetLength(formulaLength);
        operation->SetRotationAngle(formulaRotationAngle);
        operation->SetRotationOriginPointId(GetRotationOrigPointId());
        operation->RefreshGeometry();
    }

}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::closeEvent(QCloseEvent *event)
{
    ui->plainTextEditAngle->blockSignals(true);
    ui->plainTextEditRotationAngle->blockSignals(true);
    ui->plainTextEditLength->blockSignals(true);
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::EvalAngle()
{
    Eval({.formula = ui->plainTextEditAngle->toPlainText(),
          .variables = data.DataVariables(),
          .labelEditFormula = ui->labelEditAngle,
          .labelResult = ui->labelResultAngle,
          .postfix = degreeSymbol},
         flagAngle);

    if (not flagAngle)
    {
        ui->labelStatus->setText(tr("Invalid angle formula"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::EvalRotationAngle()
{
    Eval({.formula = ui->plainTextEditRotationAngle->toPlainText(),
          .variables = data.DataVariables(),
          .labelEditFormula = ui->labelEditRotationAngle,
          .labelResult = ui->labelResultRotationAngle,
          .postfix = degreeSymbol},
         flagRotationAngle);

    if (not flagRotationAngle)
    {
        ui->labelStatus->setText(tr("Invalid rotation angle formula"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::EvalLength()
{
    Eval({.formula = ui->plainTextEditLength->toPlainText(),
          .variables = data.DataVariables(),
          .labelEditFormula = ui->labelEditLength,
          .labelResult = ui->labelResultLength,
          .postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true)},
         flagLength);

    if (not flagLength)
    {
        ui->labelStatus->setText(tr("Invalid length formula"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogMove::InitIcons()
{

    ui->toolButtonExprAngle->setIcon(VTheme::ThemedIcon(QStringLiteral("function")));
    ui->toolButtonExprLength->setIcon(VTheme::ThemedIcon(QStringLiteral("function")));
    ui->toolButtonExprRotationAngle->setIcon(VTheme::ThemedIcon(QStringLiteral("function")));

}

//---------------------------------------------------------------------------------------------------------------------
auto DialogMove::IsValid() const -> bool
{
    bool const ready = flagAngle && flagRotationAngle && flagLength && IsFlagGroupName();

    if (ready)
    {
        ui->labelStatus->setText(tr("Ready"));
    }

    return ready;
}
