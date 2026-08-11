/************************************************************************
 **
 **  @file   dialogspline.cpp
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

#include "dialogspline.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QSet>

#include "../../visualization/path/vistoolspline.h"
#include "../../visualization/visualization.h"
#include "../qmuparser/qmudef.h"
#include "../vgeometry/vpointf.h"
#include "../vgeometry/vspline.h"
#include "../vmisc/def.h"
#include "../widgets/formulainputwidget.h"
#include "vtheme.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "ui_dialogspline.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogSpline create dialog
 * @param data container with data
 * @param parent parent widget
 */
DialogSpline::DialogSpline(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogSpline),
    m_formulaLength1(new FormulaInputWidget(this)),
    m_formulaAngle1(new FormulaInputWidget(this)),
    m_formulaLength2(new FormulaInputWidget(this)),
    m_formulaAngle2(new FormulaInputWidget(this))
{
    ui->setupUi(this);

    InitIcons();

    MakeFilterableComboBox(ui->comboBoxP1);
    MakeFilterableComboBox(ui->comboBoxP4);

    const QString lengthPostfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaLength1->setData(&this->data);
    m_formulaLength1->setToolId(toolId);
    m_formulaLength1->setLabelText(tr("Length:"));
    m_formulaLength1->setPostfix(lengthPostfix);
    m_formulaLength1->setDialogTitle(tr("Edit first control point length"));
    m_formulaLength1->setCheckLessThanZero(true);
    connect(m_formulaLength1, &FormulaInputWidget::valueChanged, this, [this](bool) { CheckState(); });

    m_formulaAngle1->setData(&this->data);
    m_formulaAngle1->setToolId(toolId);
    m_formulaAngle1->setLabelText(tr("Angle:"));
    m_formulaAngle1->setPostfix(degreeSymbol);
    m_formulaAngle1->setDialogTitle(tr("Edit first control point angle"));
    connect(m_formulaAngle1, &FormulaInputWidget::valueChanged, this, [this](bool) { CheckState(); });

    m_formulaLength2->setData(&this->data);
    m_formulaLength2->setToolId(toolId);
    m_formulaLength2->setLabelText(tr("Length:"));
    m_formulaLength2->setPostfix(lengthPostfix);
    m_formulaLength2->setDialogTitle(tr("Edit second control point length"));
    m_formulaLength2->setCheckLessThanZero(true);
    connect(m_formulaLength2, &FormulaInputWidget::valueChanged, this, [this](bool) { CheckState(); });

    m_formulaAngle2->setData(&this->data);
    m_formulaAngle2->setToolId(toolId);
    m_formulaAngle2->setLabelText(tr("Angle:"));
    m_formulaAngle2->setPostfix(degreeSymbol);
    m_formulaAngle2->setDialogTitle(tr("Edit second control point angle"));
    connect(m_formulaAngle2, &FormulaInputWidget::valueChanged, this, [this](bool) { CheckState(); });

    // Replace placeholders with actual formula widgets
    auto replacePlaceholder = [this](const char *name, FormulaInputWidget *widget) {
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

    replacePlaceholder("formulaLength1Placeholder", m_formulaLength1);
    replacePlaceholder("formulaAngle1Placeholder", m_formulaAngle1);
    replacePlaceholder("formulaLength2Placeholder", m_formulaLength2);
    replacePlaceholder("formulaAngle2Placeholder", m_formulaAngle2);

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxP1);
    FillComboBoxPoints(ui->comboBoxP4);
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    ui->doubleSpinBoxApproximationScale->setMaximum(maxCurveApproximationScale);

    connect(ui->comboBoxP1, &QComboBox::currentTextChanged, this, &DialogSpline::PointNameChanged);
    connect(ui->comboBoxP4, &QComboBox::currentTextChanged, this, &DialogSpline::PointNameChanged);

    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &DialogSpline::ValidateAlias);

    vis = new VisToolSpline(&this->data);
    auto *path = qobject_cast<VisToolSpline *>(vis);
    SCASSERT(path != nullptr)

    auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
    SCASSERT(scene != nullptr)
    connect(scene, &VMainGraphicsScene::MouseLeftPressed, path, &VisToolSpline::MouseLeftPressed);
    connect(scene, &VMainGraphicsScene::MouseLeftReleased, path, &VisToolSpline::MouseLeftReleased);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogSpline::~DialogSpline()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedObject gets id and type of selected object. Save right data and ignore wrong.
 * @param id id of point or detail
 * @param type type of object
 */
void DialogSpline::ChosenObject(quint32 id, const SceneObject &type)
{
    if (!prepare && type == SceneObject::Point) // After first choose we ignore all objects
    {
        auto *path = qobject_cast<VisToolSpline *>(vis);
        SCASSERT(path != nullptr)

        switch (number)
        {
            case 0:
                if (SetObject(id, ui->comboBoxP1, tr("Select last point of curve")))
                {
                    ++number;
                    path->VisualMode(id);
                }
                break;
            case 1:
            {
                if (getCurrentObjectId(ui->comboBoxP1) != id && SetObject(id, ui->comboBoxP4, QString()))
                {
                    ++number;

                    path->SetPoint4Id(id);
                    path->RefreshGeometry();
                    prepare = true;
                }
                break;
            }
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::SaveData()
{
    SaveSpline();

    if (!m_oldName.isEmpty() && m_oldName != spl.name())
    {
        spl.SetDuplicate(0);
        if (!data.IsUnique(spl.name()))
        {
            spl.SetDuplicate(DNumber(spl.name()));
            m_oldName = spl.name();
        }
    }

    auto *path = qobject_cast<VisToolSpline *>(vis);
    SCASSERT(path != nullptr)

    path->SetPoint1Id(GetP1()->id());
    path->SetPoint4Id(GetP4()->id());
    path->SetAngle1(spl.GetStartAngle());
    path->SetAngle2(spl.GetEndAngle());
    path->SetKAsm1(spl.GetKasm1());
    path->SetKAsm2(spl.GetKasm2());
    path->SetKCurve(spl.GetKcurve());
    path->SetApproximationScale(spl.GetApproximationScale());
    path->SetMode(Mode::Show);
    path->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
        InitIcons();
        InitDialogButtonBoxIcons(ui->buttonBox);
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSpline::GetP1() const -> QSharedPointer<VPointF>
{
    return data.GeometricObject<VPointF>(getCurrentObjectId(ui->comboBoxP1));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSpline::GetP4() const -> QSharedPointer<VPointF>
{
    return data.GeometricObject<VPointF>(getCurrentObjectId(ui->comboBoxP4));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::ValidateAlias()
{
    VSpline spline = spl;
    spline.SetAliasSuffix(ui->lineEditAlias->text());

    if (QRegularExpression const rx(NameRegExp());
        !ui->lineEditAlias->text().isEmpty()
        && (!rx.match(spline.GetAlias()).hasMatch()
            || (originAliasSuffix != ui->lineEditAlias->text() && !data.IsUnique(spline.GetAlias()))))
    {
        flagAlias = false;
        ui->lineEditAlias->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelAlias, errorColor);
    }
    else
    {
        flagAlias = true;
        ui->lineEditAlias->setStyleSheet({});
        ChangeColor(ui->labelAlias, OkColor(this));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::SaveSpline()
{
    const QString angle1F = m_formulaAngle1->formula();
    const QString angle2F = m_formulaAngle2->formula();
    const QString length1F = m_formulaLength1->formula();
    const QString length2F = m_formulaLength2->formula();

    const qreal angle1 = m_formulaAngle1->value();
    const qreal angle2 = m_formulaAngle2->value();
    const qreal length1 = m_formulaLength1->value();
    const qreal length2 = m_formulaLength2->value();

    const quint32 d = spl.GetDuplicate(); // Save previous value

    spl = VSpline(*GetP1(), *GetP4(), angle1, angle1F, angle2, angle2F, length1, length1F, length2, length2F);
    spl.SetApproximationScale(ui->doubleSpinBoxApproximationScale->value());
    spl.SetPenStyle(GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine));
    spl.SetColor(GetColorComboCurrentName(m_colorCombo));
    spl.SetAliasSuffix(ui->lineEditAlias->text());
    spl.SetDuplicate(d);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::InitIcons()
{
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::PointNameChanged()
{
    QColor color;
    if (getCurrentObjectId(ui->comboBoxP1) == getCurrentObjectId(ui->comboBoxP4))
    {
        flagError = false;
        color = errorColor;
    }
    else
    {
        flagError = true;
        color = OkColor(this);
    }
    ChangeColor(ui->labelFirstPoint, color);
    ChangeColor(ui->labelSecondPoint, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::ShowDialog(bool click)
{
    if (prepare && click)
    {
        auto *path = qobject_cast<VisToolSpline *>(vis);
        SCASSERT(path != nullptr)

        spl = VSpline(*GetP1(), path->GetP2(), path->GetP3(), *GetP4());

        m_formulaAngle1->setFormula(spl.GetStartAngleFormula());
        m_formulaAngle2->setFormula(spl.GetEndAngleFormula());
        m_formulaLength1->setFormula(spl.GetC1LengthFormula());
        m_formulaLength2->setFormula(spl.GetC2LengthFormula());

        if (not data.IsUnique(spl.name()))
        {
            spl.SetDuplicate(DNumber(spl.name()));
        }

        DialogAccepted();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::ShowVisualization()
{
    AddVisualization<VisToolSpline>();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSpline::GetSpline() const -> VSpline
{
    return spl;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::SetSpline(const VSpline &spline)
{
    spl = spline;

    m_oldName = spl.name();

    ui->doubleSpinBoxApproximationScale->setValue(spl.GetApproximationScale());
    SetColorComboCurrentName(m_colorCombo, spl.GetColor());
    ChangeCurrentData(ui->comboBoxPenStyle, spl.GetPenStyle());

    setCurrentPointId(ui->comboBoxP1, spl.GetP1().id());
    setCurrentPointId(ui->comboBoxP4, spl.GetP4().id());

    m_formulaAngle1->setFormula(spl.GetStartAngleFormula());
    m_formulaAngle2->setFormula(spl.GetEndAngleFormula());
    m_formulaLength1->setFormula(spl.GetC1LengthFormula());
    m_formulaLength2->setFormula(spl.GetC2LengthFormula());

    originAliasSuffix = spl.GetAliasSuffix();
    ui->lineEditAlias->setText(originAliasSuffix);
    ValidateAlias();

    auto *path = qobject_cast<VisToolSpline *>(vis);
    SCASSERT(path != nullptr)

    path->SetPoint1Id(spl.GetP1().id());
    path->SetPoint4Id(spl.GetP4().id());
    path->SetAngle1(spl.GetStartAngle());
    path->SetAngle2(spl.GetEndAngle());
    path->SetKAsm1(spl.GetKasm1());
    path->SetKAsm2(spl.GetKasm2());
    path->SetKCurve(spl.GetKcurve());
    path->SetApproximationScale(spl.GetApproximationScale());

    PointNameChanged();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSpline::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::SetDefPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::SetDefColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogSpline::IsValid() const -> bool
{
    return m_formulaLength1->isValid() && m_formulaLength2->isValid()
           && m_formulaAngle1->isValid() && m_formulaAngle2->isValid()
           && flagError && flagAlias;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSpline::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxP1->setEnabled(ready);
    ui->comboBoxP4->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
}
