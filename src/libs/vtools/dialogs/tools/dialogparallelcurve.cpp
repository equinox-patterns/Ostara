/************************************************************************
 **
 **  @file   dialogparallelcurve.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   28 11, 2025
 **
 **  @brief
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2025 Valentina project
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
#include "dialogparallelcurve.h"
#include "../../visualization/path/vistoolparallelcurve.h"
#include "../../visualization/visualization.h"
#include "../support/dialogeditwrongformula.h"
#include "../vgeometry/vsplinepath.h"
#include "vtheme.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../widgets/formulainputwidget.h"
#include "ui_dialogparallelcurve.h"

#include <QTimer>

#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
#include "../vmisc/compatibility.h"
#endif

using namespace Qt::Literals::StringLiterals;

//---------------------------------------------------------------------------------------------------------------------
DialogParallelCurve::DialogParallelCurve(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogParallelCurve),
    m_formula(new FormulaInputWidget(this))
{
    ui->setupUi(this);

    InitIcons();

    MakeFilterableComboBox(ui->comboBoxCurve);

    // --- Configure formula widget ---
    const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);
    m_formula->setData(&this->data);
    m_formula->setToolId(toolId);
    m_formula->setLabelText(tr("Width:"));
    m_formula->setPostfix(postfix);
    m_formula->setDialogTitle(tr("Edit width to parallel curve"));
    connect(m_formula, &FormulaInputWidget::valueChanged, this, [this]() { CheckState(); });

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

    FillComboBoxCurves(ui->comboBoxCurve);

    InitColorPicker(ui->pushButtonColor, VAbstractValApplication::VApp()->ValentinaSettings()->GetUserToolColors());
    ui->pushButtonColor->setUseNativeDialog(!VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    ui->doubleSpinBoxApproximationScale->setMaximum(maxCurveApproximationScale);

    connect(ui->lineEditName, &QLineEdit::textEdited, this, &DialogParallelCurve::ValidateName);
    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &DialogParallelCurve::ValidateAlias);

    vis = new VisToolParallelCurve(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogParallelCurve::~DialogParallelCurve()
{
    VAbstractValApplication::VApp()->ValentinaSettings()->SetUserToolColors(ui->pushButtonColor->CustomColors());
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogParallelCurve::GetOriginCurveId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxCurve);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::SetOriginCurveId(const quint32 &value)
{
    ChangeCurrentData(ui->comboBoxCurve, value);
    auto *path = qobject_cast<VisToolParallelCurve *>(vis);
    SCASSERT(path != nullptr)
    path->SetCurveId(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogParallelCurve::GetFormulaWidth() const -> QString
{
    return m_formula->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::SetFormulaWidth(const QString &value)
{
    m_formula->setFormula(value);

    if (auto *path = qobject_cast<VisToolParallelCurve *>(vis))
    {
        path->SetOffsetWidth(m_formula->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogParallelCurve::GetPenStyle() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::SetPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogParallelCurve::GetColor() const -> QString
{
    return ui->pushButtonColor->currentColor().name();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::SetColor(const QString &value)
{
    ui->pushButtonColor->setCurrentColor(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogParallelCurve::GetApproximationScale() const -> qreal
{
    return ui->doubleSpinBoxApproximationScale->value();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::SetApproximationScale(qreal value)
{
    ui->doubleSpinBoxApproximationScale->setValue(value);

    auto *path = qobject_cast<VisToolParallelCurve *>(vis);
    SCASSERT(path != nullptr)
    path->SetApproximationScale(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogParallelCurve::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::SetName(const QString &name)
{
    m_originName = name;
    ui->lineEditName->setText(m_originName);
    ValidateName();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogParallelCurve::GetName() const -> QString
{
    return ui->lineEditName->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::SetAliasSuffix(const QString &alias)
{
    m_originAliasSuffix = alias;
    ui->lineEditAlias->setText(m_originAliasSuffix);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogParallelCurve::GetAliasSuffix() const -> QString
{
    return ui->lineEditAlias->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::ShowDialog(bool click)
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

        const QSharedPointer<VAbstractCurve> curve = data.GeometricObject<VAbstractCurve>(GetOriginCurveId());
        QPointF const p = curve->ClosestPoint(scene->getScenePos());

        auto const line = QLineF(p, scene->getScenePos());
        if (line.length() <= ScaledRadius(SceneScale(VAbstractValApplication::VApp()->getCurrentScene())) * 1.5)
        {
            return;
        }

        qreal len = line.length();
        if (QGuiApplication::keyboardModifiers() == Qt::ShiftModifier)
        {
            len = -len;
        }

        SetFormulaWidth(QString::number(FromPixel(len, *data.GetPatternUnit())));
    }

    FinishCreating();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditName->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if ((type == SceneObject::Arc || type == SceneObject::ElArc || type == SceneObject::Spline
         || type == SceneObject::SplinePath)
        && SetObject(id, ui->comboBoxCurve, QString()))
    {
        if (vis != nullptr)
        {
            vis->VisualMode(id);
        }
        prepare = true;

        SetName(GenerateDefOffsetCurveName(&data, GetOriginCurveId(), "__o"_L1, "Curve"_L1 + offset_));

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
auto DialogParallelCurve::IsValid() const -> bool
{
    return m_formula->isValid() && m_flagName && m_flagAlias;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::ShowVisualization()
{
    AddVisualization<VisToolParallelCurve>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::SaveData()
{
    auto *path = qobject_cast<VisToolParallelCurve *>(vis);
    SCASSERT(path != nullptr)

    path->SetCurveId(GetOriginCurveId());
    path->SetOffsetWidth(m_formula->formula());
    path->SetApproximationScale(ui->doubleSpinBoxApproximationScale->value());
    path->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
        InitDialogButtonBoxIcons(ui->buttonBox);
    }

    // remember to call base class implementation
    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::ValidateName()
{
    const QSharedPointer<VAbstractCurve> curve = data.GeometricObject<VAbstractCurve>(GetOriginCurveId());
    const QString namePrefix = GetName();
    VSplinePath const splPath = curve->Offset(0, namePrefix);

    if (QRegularExpression const rx(NameRegExp()); namePrefix.isEmpty() || not rx.match(splPath.name()).hasMatch()
                                                   || (m_originName != namePrefix && not data.IsUnique(splPath.name())))
    {
        m_flagName = false;
        ChangeColor(ui->labelName, errorColor);
    }
    else
    {
        m_flagName = true;
        ChangeColor(ui->labelName, OkColor(this));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::ValidateAlias()
{
    const QSharedPointer<VAbstractCurve> curve = data.GeometricObject<VAbstractCurve>(GetOriginCurveId());
    VSplinePath splPath = curve->Offset(0, GetName());

    splPath.SetAliasSuffix(GetAliasSuffix());
    if (QRegularExpression const rx(NameRegExp());
        not GetAliasSuffix().isEmpty()
        && (not rx.match(splPath.GetAlias()).hasMatch()
            || (m_originAliasSuffix != GetAliasSuffix() && not data.IsUnique(splPath.GetAlias()))))
    {
        m_flagAlias = false;
        ChangeColor(ui->labelAlias, errorColor);
    }
    else
    {
        m_flagAlias = true;
        ChangeColor(ui->labelAlias, OkColor(this));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();

    emit ToolTip(QString());

    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogParallelCurve::InitIcons()
{
    // Formula-related icons are now managed by FormulaInputWidget.
    // PaletteChange re-initialization is handled by FormulaInputWidget internally.
}
