/************************************************************************
 **
 **  @file   dialogellipticalarc.cpp
 **  @author Valentina Zhuravska <zhuravska19(at)gmail.com>
 **  @date   15 9, 2016
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

#include "dialogellipticalarc.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>

#include "../../visualization/path/vistoolellipticalarc.h"
#include "../../visualization/visualization.h"
#include "../qmuparser/qmudef.h"
#include "../vgeometry/vellipticalarc.h"
#include "vtheme.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../widgets/formulainputwidget.h"
#include "../widgets/angleformulawidget.h"
#include "ui_dialogellipticalarc.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogEllipticalArc create dialog
 * @param data container with data
 * @param parent parent widget
 */
DialogEllipticalArc::DialogEllipticalArc(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogEllipticalArc),
    m_formulaRadius1(new FormulaInputWidget(this)),
    m_formulaRadius2(new FormulaInputWidget(this)),
    m_formulaF1(new AngleFormulaWidget(this)),
    m_formulaF2(new AngleFormulaWidget(this)),
    m_formulaRotationAngle(new AngleFormulaWidget(this))
{
    ui->setupUi(this);

    MakeFilterableComboBox(ui->comboBoxBasePoint);

    // --- Configure formula widgets ---
    const QString lengthPostfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaRadius1->setData(&this->data);
    m_formulaRadius1->setToolId(toolId);
    m_formulaRadius1->setLabelText(tr("Radius1:"));
    m_formulaRadius1->setPostfix(lengthPostfix);
    m_formulaRadius1->setDialogTitle(tr("Edit radius1"));
    connect(m_formulaRadius1, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    m_formulaRadius2->setData(&this->data);
    m_formulaRadius2->setToolId(toolId);
    m_formulaRadius2->setLabelText(tr("Radius2:"));
    m_formulaRadius2->setPostfix(lengthPostfix);
    m_formulaRadius2->setDialogTitle(tr("Edit radius2"));
    connect(m_formulaRadius2, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    m_formulaF1->setData(&this->data);
    m_formulaF1->setToolId(toolId);
    m_formulaF1->setLabelText(tr("First angle:"));
    m_formulaF1->setPostfix(degreeSymbol);
    m_formulaF1->setDialogTitle(tr("Edit first angle"));
    connect(m_formulaF1, &AngleFormulaWidget::valueChanged, this,
            [this]() { m_angleF1 = m_formulaF1->value(); CheckState(); UpdatePointControls(); });

    m_formulaF2->setData(&this->data);
    m_formulaF2->setToolId(toolId);
    m_formulaF2->setLabelText(tr("Second angle:"));
    m_formulaF2->setPostfix(degreeSymbol);
    m_formulaF2->setDialogTitle(tr("Edit second angle"));
    connect(m_formulaF2, &AngleFormulaWidget::valueChanged, this,
            [this]() { m_angleF2 = m_formulaF2->value(); CheckState(); UpdatePointControls(); });

    m_formulaRotationAngle->setData(&this->data);
    m_formulaRotationAngle->setToolId(toolId);
    m_formulaRotationAngle->setLabelText(tr("Rotation angle:"));
    m_formulaRotationAngle->setPostfix(degreeSymbol);
    m_formulaRotationAngle->setDialogTitle(tr("Edit rotation angle"));
    connect(m_formulaRotationAngle, &AngleFormulaWidget::valueChanged, this,
            [this]() { m_angleRotation = m_formulaRotationAngle->value(); CheckState(); });

    // Replace formula placeholders in the UI layout with the actual widgets.
    auto replacePlaceholder = [this](const char *name, QWidget *widget)
    {
        auto *placeholder = findChild<QWidget *>(QString::fromLatin1(name));
        if (placeholder)
        {
            auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
            if (parentLayout != nullptr)
            {
                const int idx = parentLayout->indexOf(placeholder);
                parentLayout->insertWidget(idx, widget);
                placeholder->deleteLater();
            }
        }
    };

    replacePlaceholder("formulaRadius1Placeholder", m_formulaRadius1);
    replacePlaceholder("formulaRadius2Placeholder", m_formulaRadius2);
    replacePlaceholder("formulaF1Placeholder", m_formulaF1);
    replacePlaceholder("formulaF2Placeholder", m_formulaF2);
    replacePlaceholder("formulaRotationAnglePlaceholder", m_formulaRotationAngle);

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxBasePoint);
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &DialogEllipticalArc::ValidateAlias);

    // Move the "ElArc:" field to the top of the Tool tab (above the radius formula).
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

    vis = new VisToolEllipticalArc(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::IsValid() const -> bool
{
    return m_formulaRadius1->isValid() && m_formulaRadius2->isValid() && m_formulaF1->isValid()
           && m_formulaF2->isValid() && m_formulaRotationAngle->isValid() && m_flagAlias;
}

//---------------------------------------------------------------------------------------------------------------------
DialogEllipticalArc::~DialogEllipticalArc()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetCenter return id of center point
 * @return id id
 */
auto DialogEllipticalArc::GetCenter() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxBasePoint);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetCenter set id of center point
 * @param value id
 */
void DialogEllipticalArc::SetCenter(quint32 value)
{
    ChangeCurrentData(ui->comboBoxBasePoint, value);

    auto *path = qobject_cast<VisToolEllipticalArc *>(vis);
    SCASSERT(path != nullptr)
    path->SetCenterId(value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetRadius1 return formula of radius1
 * @return formula
 */
auto DialogEllipticalArc::GetRadius1() const -> QString
{
    return m_formulaRadius1->formula();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetRadius1 set formula of radius1
 * @param value formula
 */
void DialogEllipticalArc::SetRadius1(const QString &value)
{
    m_formulaRadius1->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArc *>(vis))
    {
        path->SetRadius1(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formulaRadius1->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetRadius2 return formula of radius2
 * @return formula
 */
auto DialogEllipticalArc::GetRadius2() const -> QString
{
    return m_formulaRadius2->formula();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetRadius2 set formula of radius2
 * @param value formula
 */
void DialogEllipticalArc::SetRadius2(const QString &value)
{
    m_formulaRadius2->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArc *>(vis))
    {
        path->SetRadius2(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formulaRadius2->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetF1 return formula first angle of elliptical arc
 * @return formula
 */
auto DialogEllipticalArc::GetF1() const -> QString
{
    return m_formulaF1->formula();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetF1 set formula first angle of elliptical arc
 * @param value formula
 */
void DialogEllipticalArc::SetF1(const QString &value)
{
    m_formulaF1->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArc *>(vis))
    {
        path->SetF1(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formulaF1->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetF2 return formula second angle of elliptical arc
 * @return formula
 */
auto DialogEllipticalArc::GetF2() const -> QString
{
    return m_formulaF2->formula();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetF2 set formula second angle of elliptical arc
 * @param value formula
 */
void DialogEllipticalArc::SetF2(const QString &value)
{
    m_formulaF2->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArc *>(vis))
    {
        path->SetF2(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formulaF2->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetRotationAngle return formula rotation angle of elliptical arc
 * @return formula
 */
auto DialogEllipticalArc::GetRotationAngle() const -> QString
{
    return m_formulaRotationAngle->formula();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetRotationAngle set formula rotation angle of elliptical arc
 * @param value formula
 */
void DialogEllipticalArc::SetRotationAngle(const QString &value)
{
    m_formulaRotationAngle->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArc *>(vis))
    {
        path->SetRotationAngle(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formulaRotationAngle->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::GetPenStyle() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::SetPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetColor return color of elliptical arc
 * @return formula
 */
auto DialogEllipticalArc::GetColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetColor set color of elliptical arc
 * @param value formula
 */
void DialogEllipticalArc::SetColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief EvalRadiuses calculate value of radiuses
 */
void DialogEllipticalArc::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();

    emit ToolTip(QString());

    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::InitIcons()
{
    // Formula-related icons are now managed by FormulaInputWidget.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::SaveData()
{
    auto *path = qobject_cast<VisToolEllipticalArc *>(vis);
    SCASSERT(path != nullptr)

    const VTranslateVars *trVars = VAbstractValApplication::VApp()->TrVars();
    const bool osSeparator = VAbstractValApplication::VApp()->Settings()->GetOsSeparator();
    const QString radius1 = trVars->FormulaToUser(m_formulaRadius1->formula(), osSeparator);
    const QString radius2 = trVars->FormulaToUser(m_formulaRadius2->formula(), osSeparator);
    const QString f1 = trVars->FormulaToUser(m_formulaF1->formula(), osSeparator);
    const QString f2 = trVars->FormulaToUser(m_formulaF2->formula(), osSeparator);
    const QString rotationAngle = trVars->FormulaToUser(m_formulaRotationAngle->formula(), osSeparator);

    path->SetCenterId(GetCenter());
    path->SetRadius1(radius1);
    path->SetRadius2(radius2);
    path->SetF1(f1);
    path->SetF2(f2);
    path->SetRotationAngle(rotationAngle);
    path->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::ShowDialog(bool click)
{
    if (not prepare)
    {
        return;
    }

    if (click)
    {
        // The check need to ignore first release of mouse button.
        // User can select point by clicking on a label.
        if (not m_firstRelease)
        {
            m_firstRelease = true;
            return;
        }

        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)

        const QSharedPointer<VPointF> center = data.GeometricObject<VPointF>(GetCenter());
        auto line = QLineF(static_cast<QPointF>(*center), scene->getScenePos());

        auto Angle = [&line]()
        {
            if (QGuiApplication::keyboardModifiers() == Qt::ShiftModifier)
            {
                QLineF correction = line;
                correction.setAngle(Visualization::CorrectAngle(correction.angle()));
                return correction.angle();
            }

            return line.angle();
        };

        if (m_stage == 0) // radius 1
        {
            // Radius of point circle, but little bigger. Need to handle with hover sizes.
            if (line.length() <= ScaledRadius(SceneScale(VAbstractValApplication::VApp()->getCurrentScene())) * 1.5)
            {
                return;
            }

            SetRadius1(QString::number(FromPixel(line.length(), *data.GetPatternUnit())));
            vis->RefreshGeometry();
            ++m_stage;
            return;
        }

        if (m_stage == 1) // radius 2
        {
            QLineF const radius2Line(center->x(), center->y(), center->x(), center->y() - 100);
            QPointF const p = VGObject::ClosestPoint(radius2Line, scene->getScenePos());
            line = QLineF(static_cast<QPointF>(*center), p);

            // Radius of point circle, but little bigger. Need to handle with hover sizes.
            if (line.length() <= ScaledRadius(SceneScale(VAbstractValApplication::VApp()->getCurrentScene())) * 1.5)
            {
                return;
            }

            SetRadius2(QString::number(FromPixel(line.length(), *data.GetPatternUnit())));
            vis->RefreshGeometry();
            ++m_stage;
            return;
        }

        if (m_stage == 2) // f1
        {
            SetF1(QString::number(Angle()));
            vis->RefreshGeometry();
            ++m_stage;
            return;
        }

        if (m_stage == 3) // f2
        {
            SetF2(QString::number(Angle()));
            vis->RefreshGeometry();
            ++m_stage;
            return;
        }

        auto *path = qobject_cast<VisToolEllipticalArc *>(vis);
        SCASSERT(path != nullptr)

        SetRotationAngle(QString::number(Angle() - path->StartingRotationAngle()));
    }

    FinishCreating();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxBasePoint->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedObject gets id and type of selected object. Save right data and ignore wrong.
 * @param id id of point or detail
 * @param type type of object
 */
void DialogEllipticalArc::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if (type != SceneObject::Point)
    {
        return;
    }

    if (SetObject(id, ui->comboBoxBasePoint, QString()))
    {
        if (vis != nullptr)
        {
            auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
            SCASSERT(window != nullptr)
            connect(vis.data(), &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

            vis->VisualMode(id);
        }
        prepare = true;

        if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
        {
            FinishCreating();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::ShowVisualization()
{
    AddVisualization<VisToolEllipticalArc>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::ValidateAlias()
{
    VEllipticalArc arc;
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
void DialogEllipticalArc::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::SetAliasSuffix(const QString &alias)
{
    m_originAliasSuffix = alias;
    ui->lineEditAlias->setText(m_originAliasSuffix);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::GetAliasSuffix() const -> QString
{
    return ui->lineEditAlias->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::IsCreateStartPoint() const -> bool
{
    return ui->checkBoxCreateStartPoint->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::IsCreateEndPoint() const -> bool
{
    return ui->checkBoxCreateEndPoint->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::GetStartPointName() const -> QString
{
    return ui->lineEditStartPointName->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::GetEndPointName() const -> QString
{
    return ui->lineEditEndPointName->text();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::GetStartShowLabel() const -> bool
{
    return m_showLabelStart;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArc::GetEndShowLabel() const -> bool
{
    return m_showLabelEnd;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArc::UpdatePointControls()
{
    const bool fullEllipse = (!m_formulaF1->formula().isEmpty() && !m_formulaF2->formula().isEmpty() &&
                              qFuzzyIsNull(m_angleF1 - m_angleF2));

    ui->checkBoxCreateStartPoint->setEnabled(not fullEllipse);
    ui->checkBoxCreateEndPoint->setEnabled(not fullEllipse);

    const bool startOn = ui->checkBoxCreateStartPoint->isChecked() && not fullEllipse;
    const bool endOn = ui->checkBoxCreateEndPoint->isChecked() && not fullEllipse;

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
