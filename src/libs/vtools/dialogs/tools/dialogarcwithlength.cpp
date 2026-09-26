/************************************************************************
 **
 **  @file   dialogarcwithlength.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   9 6, 2015
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2015 Valentina project
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

#include "dialogarcwithlength.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>

#include "../../visualization/path/vistoolarcwithlength.h"
#include "../../visualization/visualization.h"
#include "../qmuparser/qmudef.h"
#include "../support/dialogeditwrongformula.h"
#include "../vgeometry/varc.h"
#include "vtheme.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vmath.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../widgets/formulainputwidget.h"
#include "../widgets/angleformulawidget.h"
#include "ui_dialogarcwithlength.h"

//---------------------------------------------------------------------------------------------------------------------
DialogArcWithLength::DialogArcWithLength(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogArcWithLength),
    m_formula(new FormulaInputWidget(this)),
    m_formula2(new AngleFormulaWidget(this)),
    m_formula3(new FormulaInputWidget(this))
{
    ui->setupUi(this);

    MakeFilterableComboBox(ui->comboBoxCenter);

    // --- Configure formula widgets ---
    const QString lengthPostfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formula->setData(&this->data);
    m_formula->setToolId(toolId);
    m_formula->setLabelText(tr("Radius:"));
    m_formula->setPostfix(lengthPostfix);
    m_formula->setDialogTitle(tr("Edit radius"));
    m_formula->setCheckLessThanZero(true);
    m_formula->setCheckZero(true);
    connect(m_formula, &FormulaInputWidget::valueChanged, this, [this]() { CheckState(); UpdatePointControls(); });

    m_formula2->setData(&this->data);
    m_formula2->setToolId(toolId);
    m_formula2->setLabelText(tr("First angle:"));
    m_formula2->setPostfix(degreeSymbol);
    m_formula2->setDialogTitle(tr("Edit the first angle"));
    connect(m_formula2, &AngleFormulaWidget::valueChanged, this, [this]() { CheckState(); });

    m_formula3->setData(&this->data);
    m_formula3->setToolId(toolId);
    m_formula3->setLabelText(tr("Length:"));
    m_formula3->setPostfix(lengthPostfix);
    m_formula3->setDialogTitle(tr("Edit the arc length"));
    m_formula3->setCheckZero(true);
    m_formula3->setCheckLessThanZero(true);
    connect(m_formula3, &FormulaInputWidget::valueChanged, this, [this]() { CheckState(); UpdatePointControls(); });

    // Replace formula placeholders in the UI layout with the actual widgets.
    if (auto *placeholder = ui->formulaPlaceholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formula);
            placeholder->deleteLater();
        }
    }
    if (auto *placeholder = ui->formula2Placeholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formula2);
            placeholder->deleteLater();
        }
    }
    if (auto *placeholder = ui->formula3Placeholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formula3);
            placeholder->deleteLater();
        }
    }

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxCenter);
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &DialogArcWithLength::ValidateAlias);

    // Move the "Arc:" field to the top of the Tool tab (above the radius formula).
    auto aliasRow = ui->formLayout_2->takeRow(3);
    QWidget *aliasLabel = aliasRow.labelItem->widget();
    QWidget *aliasField = aliasRow.fieldItem->widget();
    delete aliasRow.labelItem;
    delete aliasRow.fieldItem;

    auto *aliasLayout = new QHBoxLayout;
    aliasLayout->setContentsMargins(0, 0, 0, 0);
    aliasLayout->addWidget(aliasLabel);
    aliasLayout->addWidget(aliasField);
    ui->verticalLayout->insertLayout(0, aliasLayout);

    // Start/End point creation controls.
    m_startShowLabelCheckbox = AddShowPointNameCheckbox(ui->formLayoutStartPoint);
    connect(m_startShowLabelCheckbox, &QCheckBox::toggled, this, [this](bool checked) { m_showLabelStart = checked; });

    m_endShowLabelCheckbox = AddShowPointNameCheckbox(ui->formLayoutEndPoint);
    connect(m_endShowLabelCheckbox, &QCheckBox::toggled, this, [this](bool checked) { m_showLabelEnd = checked; });

    connect(ui->checkBoxCreateStartPoint, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && ui->lineEditStartPointName->text().isEmpty())
        {
            ui->lineEditStartPointName->setText(
                VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));
        }
        UpdatePointControls();
    });

    connect(ui->checkBoxCreateEndPoint, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && ui->lineEditEndPointName->text().isEmpty())
        {
            ui->lineEditEndPointName->setText(
                VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));
        }
        UpdatePointControls();
    });

    UpdatePointControls();

    vis = new VisToolArcWithLength(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogArcWithLength::~DialogArcWithLength()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetCenter() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxCenter);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::SetCenter(const quint32 &value)
{
    ChangeCurrentData(ui->comboBoxCenter, value);
    if (auto *path = qobject_cast<VisToolArcWithLength *>(vis))
    {
        path->SetCenterId(value);
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetRadius() const -> QString
{
    return m_formula->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::SetRadius(const QString &value)
{
    m_formula->setFormula(value);

    if (auto *path = qobject_cast<VisToolArcWithLength *>(vis))
    {
        path->SetRadius(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetF1() const -> QString
{
    return m_formula2->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::SetF1(const QString &value)
{
    m_formula2->setFormula(value);

    if (auto *path = qobject_cast<VisToolArcWithLength *>(vis))
    {
        path->SetF1(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula2->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetLength() const -> QString
{
    return m_formula3->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::SetLength(const QString &value)
{
    m_formula3->setFormula(value);

    if (auto *path = qobject_cast<VisToolArcWithLength *>(vis))
    {
        path->SetLength(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula3->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetPenStyle() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::SetPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::SetColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::SetAliasSuffix(const QString &alias)
{
    m_originAliasSuffix = alias;
    ui->lineEditAlias->setText(m_originAliasSuffix);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetAliasSuffix() const -> QString
{
    return ui->lineEditAlias->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::ShowDialog(bool click)
{
    if (prepare)
    {
        if (click)
        {
            // The check need to ignore first release of mouse button.
            // User can select point by clicking on a label.
            if (not m_firstRelease)
            {
                m_firstRelease = true;
                return;
            }

            /*We will ignore click if pointer is in point circle*/
            auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
            SCASSERT(scene != nullptr)
            const QSharedPointer<VPointF> point = data.GeometricObject<VPointF>(GetCenter());
            auto const line = QLineF(static_cast<QPointF>(*point), scene->getScenePos());

            auto Angle = [&line]()
            {
                if (QGuiApplication::keyboardModifiers() == Qt::ShiftModifier)
                {
                    QLineF correction = line;
                    correction.setAngle(VisToolArcWithLength::CorrectAngle(correction.angle()));
                    return correction.angle();
                }

                return line.angle();
            };

            if (m_stageRadius)
            {
                // Radius of point circle, but little bigger. Need handle with hover sizes.
                if (line.length() <= ScaledRadius(SceneScale(VAbstractValApplication::VApp()->getCurrentScene())) * 1.5)
                {
                    return;
                }

                SetRadius(QString::number(RoundForDisplay(VAbstractValApplication::VApp()->fromPixel(line.length()))));
                vis->RefreshGeometry();

                m_stageRadius = false;
                m_stageF1 = true;
            }
            else if (m_stageF1)
            {

                SetF1(QString::number(RoundForDisplay(Angle())));
                vis->RefreshGeometry();

                m_stageF1 = false;
            }
            else
            {
                const qreal r = Visualization::FindLengthFromUser(m_formula->formula(), data.DataVariables(), false);
                const qreal angle1 = Visualization::FindValFromUser(m_formula2->formula(), data.DataVariables(), false);
                VArc const arc(*point, r, angle1, line.angle());

                SetLength(QString::number(RoundForDisplay(VAbstractValApplication::VApp()->fromPixel(arc.GetLength()))));

                FinishCreating();
            }

            return;
        }

        FinishCreating();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxCenter->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::ChosenObject(quint32 id, const SceneObject &type)
{
    if (not prepare && type == SceneObject::Point &&
        SetObject(id, ui->comboBoxCenter, QString())) // After first choose we ignore all objects
    {
        if (vis != nullptr)
        {
            const auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
            SCASSERT(window != nullptr)
            connect(vis.data(), &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

            vis->VisualMode(id);
            vis->RefreshToolTip();
        }

        prepare = true;

        if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
        {
            FinishCreating();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::IsValid() const -> bool
{
    return m_formula->isValid() && m_formula2->isValid() && m_formula3->isValid() && m_flagAlias;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::ShowVisualization()
{
    AddVisualization<VisToolArcWithLength>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::SaveData()
{
    if (auto *path = qobject_cast<VisToolArcWithLength *>(vis))
    {
        path->SetCenterId(GetCenter());
        path->SetRadius(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
        path->SetF1(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula2->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
        path->SetLength(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula3->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
        path->RefreshGeometry();
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping save.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::ValidateAlias()
{
    VArc arc;
    arc.SetAliasSuffix(GetAliasSuffix());
    if (QRegularExpression const rx(NameRegExp());
        not GetAliasSuffix().isEmpty() &&
        (not rx.match(arc.GetAlias()).hasMatch() ||
         (m_originAliasSuffix != GetAliasSuffix() && not data.IsUnique(arc.GetAlias()))))
    {
        m_flagAlias = false;
        ui->lineEditAlias->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelAlias, errorColor);
    }
    else
    {
        m_flagAlias = true;
        ui->lineEditAlias->setStyleSheet({});
        ChangeColor(ui->labelAlias, OkColor(this));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::FinishCreating()
{
    if (vis != nullptr)
    {
        vis->SetMode(Mode::Show);
        vis->RefreshGeometry();
    }

    emit ToolTip(QString());

    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::IsCreateStartPoint() const -> bool
{
    return ui->checkBoxCreateStartPoint->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::IsCreateEndPoint() const -> bool
{
    return ui->checkBoxCreateEndPoint->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetStartPointName() const -> QString
{
    return ui->lineEditStartPointName->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetEndPointName() const -> QString
{
    return ui->lineEditEndPointName->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetStartShowLabel() const -> bool
{
    return m_showLabelStart;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArcWithLength::GetEndShowLabel() const -> bool
{
    return m_showLabelEnd;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArcWithLength::UpdatePointControls()
{
    const bool hasFormulas = !m_formula->formula().isEmpty() && !m_formula3->formula().isEmpty();

    qreal radius = 0;
    qreal length = 0;
    if (hasFormulas)
    {
        radius = m_formula->value();
        length = m_formula3->value();
    }

    // Full circle when the length reaches the full circumference (VArc::FindF2 clamps it to a full circle).
    const bool fullCircle = hasFormulas && radius > 0 && length >= M_2PI * radius;

    ui->checkBoxCreateStartPoint->setEnabled(not fullCircle);
    ui->checkBoxCreateEndPoint->setEnabled(not fullCircle);

    const bool startOn = ui->checkBoxCreateStartPoint->isChecked() && not fullCircle;
    const bool endOn = ui->checkBoxCreateEndPoint->isChecked() && not fullCircle;

    ui->lineEditStartPointName->setEnabled(startOn);
    ui->lineEditEndPointName->setEnabled(endOn);

    if (m_startShowLabelCheckbox != nullptr)
    {
        m_startShowLabelCheckbox->setEnabled(startOn);
    }
    if (m_endShowLabelCheckbox != nullptr)
    {
        m_endShowLabelCheckbox->setEnabled(endOn);
    }
}
