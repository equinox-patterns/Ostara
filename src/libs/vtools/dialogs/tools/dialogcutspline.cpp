/************************************************************************
 **
 **  @file   dialogcutspline.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   15 12, 2013
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

#include "dialogcutspline.h"

#include <QDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>

#include "../../visualization/path/vistoolcutspline.h"
#include "../../visualization/visualization.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../qmuparser/qmudef.h"
#include "../support/dialogeditwrongformula.h"
#include "../vgeometry/vspline.h"
#include "vtheme.h"
#include "../vtheme/vscenestylesheet.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../widgets/formulainputwidget.h"
#include "ui_dialogcutspline.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogCutSpline create dialog.
 * @param data container with data
 * @param parent parent widget
 */
DialogCutSpline::DialogCutSpline(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogCutSpline),
    m_formula(new FormulaInputWidget(this))
{
    ui->setupUi(this);
    connect(AddShowPointNameCheckbox(ui->formLayoutPoint), &QCheckBox::toggled, this,
            [this](bool checked) { SetShowLabel(checked); });
    InitShowTwoSegmentsCheckBox(ui->verticalLayout, ui->plainTextEditToolNotes);
    InitProlongationWidgets(ui->verticalLayout, ui->plainTextEditToolNotes);

    InitIcons();

    MakeFilterableComboBox(ui->comboBoxSpline);


    SetPointName(VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));

    // --- Configure formula widget ---
    const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);
    m_formula->setData(&this->data);
    m_formula->setToolId(toolId);
    m_formula->setLabelText(tr("Length:"));
    m_formula->setPostfix(postfix);
    m_formula->setDialogTitle(tr("Edit length"));
    connect(m_formula,
            &FormulaInputWidget::valueChanged,
            this,
            [this]()
            {
                CheckState();
                UpdateProlongation(getSplineId(), m_formula->formula());
            });

    // Replace placeholder
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

    InitOkCancelApply(ui);

    FillComboBoxSplines(ui->comboBoxSpline);

    // Name1/Name2 are auto-generated from segment points (RoleName step 4)
    m_flagCurveName1 = true;
    m_flagCurveName2 = true;

    connect(ui->lineEditNamePoint, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckPointLabel(this, ui->lineEditNamePoint, ui->labelEditNamePoint, m_pointName, &this->data,
                                m_flagName);
                CheckState();
            });
    connect(ui->comboBoxSpline, &QComboBox::currentTextChanged, this, &DialogCutSpline::SplineChanged);

    connect(ui->lineEditAlias1, &QLineEdit::textEdited, this, &DialogCutSpline::ValidateAlias);
    connect(ui->lineEditAlias2, &QLineEdit::textEdited, this, &DialogCutSpline::ValidateAlias);

    // Move the two curve-name fields to the top of the Tool tab (above the formula)
    ui->labelAlias1->setMinimumWidth(80);
    ui->labelAlias2->setMinimumWidth(80);

    auto alias1Row = ui->formLayout->takeRow(1);
    auto alias2Row = ui->formLayout->takeRow(1);

    QWidget *alias1Label = alias1Row.labelItem->widget();
    QWidget *alias1Field = alias1Row.fieldItem->widget();
    QWidget *alias2Label = alias2Row.labelItem->widget();
    QWidget *alias2Field = alias2Row.fieldItem->widget();
    delete alias1Row.labelItem;
    delete alias1Row.fieldItem;
    delete alias2Row.labelItem;
    delete alias2Row.fieldItem;

    auto *alias1Layout = new QHBoxLayout;
    alias1Layout->setContentsMargins(0, 0, 0, 0);
    alias1Layout->addWidget(alias1Label);
    alias1Layout->addWidget(alias1Field);
    ui->verticalLayout->insertLayout(0, alias1Layout);

    auto *alias2Layout = new QHBoxLayout;
    alias2Layout->setContentsMargins(0, 0, 0, 0);
    alias2Layout->addWidget(alias2Label);
    alias2Layout->addWidget(alias2Field);
    ui->verticalLayout->insertLayout(1, alias2Layout);

    // Separator between the curve-name fields and the formula
    auto *lineSeparator = new QFrame();
    lineSeparator->setObjectName(QStringLiteral("notesSeparator"));
    lineSeparator->setFixedHeight(1);
    auto *lineSeparatorLayout = new QHBoxLayout;
    lineSeparatorLayout->setContentsMargins(33, 0, 33, 0);
    lineSeparatorLayout->addWidget(lineSeparator);
    ui->verticalLayout->insertLayout(2, lineSeparatorLayout);

    vis = new VisToolCutSpline(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutSpline::IsValid() const -> bool
{
    return m_formula->isValid() && m_flagName && m_flagAlias1 && m_flagAlias2;
}

//---------------------------------------------------------------------------------------------------------------------
DialogCutSpline::~DialogCutSpline()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutSpline::GetPointName() const -> QString
{
    return m_pointName;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetPointName set name of point
 * @param value name
 */
void DialogCutSpline::SetPointName(const QString &value)
{
    m_pointName = value;
    ui->lineEditNamePoint->setText(m_pointName);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::SetName1(const QString &name)
{
    m_originName1 = name;
    ui->lineEditName1->setText(m_originName1);
    ValidateCurveNames();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutSpline::GetName1() const -> QString
{
    return ui->lineEditName1->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::SetName2(const QString &name)
{
    m_originName2 = name;
    ui->lineEditName2->setText(m_originName2);
    ValidateCurveNames();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutSpline::GetName2() const -> QString
{
    return ui->lineEditName2->text();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetFormula set string of formula
 * @param value formula
 */
void DialogCutSpline::SetFormula(const QString &value)
{
    m_formula->setFormula(value);

    if (auto *path = qobject_cast<VisToolCutSpline *>(vis))
    {
        path->SetLength(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief setSplineId set id spline
 * @param value id
 */
void DialogCutSpline::setSplineId(quint32 value)
{
    setCurrentSplineId(ui->comboBoxSpline, value);

    auto *path = qobject_cast<VisToolCutSpline *>(vis);
    SCASSERT(path != nullptr)
    path->SetSplineId(value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedObject gets id and type of selected object. Save right data and ignore wrong.
 * @param id id of point or detail
 * @param type type of object
 */
void DialogCutSpline::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if (type == SceneObject::Spline && SetObject(id, ui->comboBoxSpline, QString()))
    {
        if (vis != nullptr)
        {
            vis->VisualMode(id);
        }
        prepare = true;

        SetName1(GenerateDefLeftSubName());
        SetName2(GenerateDefRightSubName());

        UpdateProlongation(getSplineId(), m_formula->formula());

        auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
        SCASSERT(window != nullptr)
        connect(vis.data(), &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

        if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
        {
            FinishCreating();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::SaveData()
{
    m_pointName = ui->lineEditNamePoint->text();

    auto *path = qobject_cast<VisToolCutSpline *>(vis);
    SCASSERT(path != nullptr)

    path->SetSplineId(getSplineId());
    path->SetLength(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
        m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    path->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::changeEvent(QEvent *event)
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
void DialogCutSpline::SplineChanged()
{
    CurrentCurveLength(getSplineId(), const_cast<VContainer *>(&data));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::RestoreTransientVariables()
{
    SplineChanged();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::ValidateAlias()
{
    QRegularExpression const rx(NameRegExp());

    VSpline spl1;
    spl1.SetAliasSuffix(GetAliasSuffix1());

    VSpline spl2;
    spl2.SetAliasSuffix(GetAliasSuffix2());

    if (not GetAliasSuffix1().isEmpty() &&
        (not rx.match(spl1.GetAlias()).hasMatch() ||
         (m_originAliasSuffix1 != GetAliasSuffix1() && not data.IsUnique(spl1.GetAlias())) ||
         spl1.GetAlias() == spl2.GetAlias()))
    {
        m_flagAlias1 = false;
        ui->lineEditAlias1->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelAlias1, errorColor);
    }
    else
    {
        m_flagAlias1 = true;
        ui->lineEditAlias1->setStyleSheet({});
        ChangeColor(ui->labelAlias1, OkColor(this));
    }

    if (not GetAliasSuffix2().isEmpty() &&
        (not rx.match(spl2.GetAlias()).hasMatch() ||
         (m_originAliasSuffix2 != GetAliasSuffix2() && not data.IsUnique(spl2.GetAlias())) ||
         spl1.GetAlias() == spl2.GetAlias()))
    {
        m_flagAlias2 = false;
        ui->lineEditAlias2->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelAlias2, errorColor);
    }
    else
    {
        m_flagAlias2 = true;
        ui->lineEditAlias2->setStyleSheet({});
        ChangeColor(ui->labelAlias2, OkColor(this));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::ValidateCurveNames()
{
    QRegularExpression const rx(NameRegExp());

    VSpline spl1;
    spl1.SetNameSuffix(GetName1());

    VSpline spl2;
    spl2.SetNameSuffix(GetName2());

    if (GetName1().isEmpty() || !rx.match(spl1.name()).hasMatch()
        || (m_originName1 != GetName1() && not data.IsUnique(spl1.name())) || spl1.name() == spl2.name())
    {
        m_flagCurveName1 = false;
        ui->lineEditName1->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelName1, errorColor);
    }
    else
    {
        m_flagCurveName1 = true;
        ui->lineEditName1->setStyleSheet({});
        ChangeColor(ui->labelName1, OkColor(this));
    }

    if (GetName2().isEmpty() || !rx.match(spl2.name()).hasMatch()
        || (m_originName2 != GetName2() && not data.IsUnique(spl2.name())) || spl1.name() == spl2.name())
    {
        m_flagCurveName2 = false;
        ui->lineEditName2->setStyleSheet(QStringLiteral("border: 1px solid red;"));
        ChangeColor(ui->labelName2, errorColor);
    }
    else
    {
        m_flagCurveName2 = true;
        ui->lineEditName2->setStyleSheet({});
        ChangeColor(ui->labelName2, OkColor(this));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();
    emit ToolTip(QString());
    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::InitIcons()
{
    ui->labelAlias1Dot->setPixmap(GetColorDotPixmap(VSceneStylesheet::Color(VColorRole::VisSupportColor2)));
    ui->labelAlias2Dot->setPixmap(GetColorDotPixmap(VSceneStylesheet::Color(VColorRole::VisSupportColor4)));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutSpline::GenerateDefLeftSubName() const -> QString
{
    return GenerateDefSubCurveName(&data, getSplineId(), "__ls"_L1, "LSubCurve"_L1, GetPointName());
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutSpline::GenerateDefRightSubName() const -> QString
{
    return GenerateDefSubCurveName(&data, getSplineId(), "__rs"_L1, "RSubCurve"_L1, GetPointName());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::ShowVisualization()
{
    AddVisualization<VisToolCutSpline>();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetFormula return string of formula
 * @return formula
 */
auto DialogCutSpline::GetFormula() const -> QString
{
    return m_formula->formula();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief getSplineId return id base point of line
 * @return id
 */
auto DialogCutSpline::getSplineId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxSpline);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutSpline::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::SetAliasSuffix1(const QString &alias)
{
    m_originAliasSuffix1 = alias;
    ui->lineEditAlias1->setText(m_originAliasSuffix1);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutSpline::GetAliasSuffix1() const -> QString
{
    return ui->lineEditAlias1->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::SetAliasSuffix2(const QString &alias)
{
    m_originAliasSuffix2 = alias;
    ui->lineEditAlias2->setText(m_originAliasSuffix2);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogCutSpline::GetAliasSuffix2() const -> QString
{
    return ui->lineEditAlias2->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::ShowDialog(bool click)
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

        const QSharedPointer<VAbstractCubicBezier> spl = data.GeometricObject<VAbstractCubicBezier>(getSplineId());
        QPointF const p = spl->ClosestPoint(scene->getScenePos());
        qreal const len = spl->GetLengthByPoint(p);
        SetFormula(QString::number(FromPixel(len, *data.GetPatternUnit())));
    }

    FinishCreating();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogCutSpline::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditNamePoint->setEnabled(ready);
    ui->lineEditName1->setEnabled(ready);
    ui->lineEditName2->setEnabled(ready);
    ui->lineEditAlias1->setEnabled(ready);
    ui->lineEditAlias2->setEnabled(ready);
}
