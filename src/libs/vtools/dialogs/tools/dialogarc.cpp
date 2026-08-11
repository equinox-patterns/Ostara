/************************************************************************
 **
 **  @file   dialogarc.cpp
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

#include "dialogarc.h"

#include <QDialog>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QTimer>

#include "../../visualization/path/vistoolarc.h"
#include "../../visualization/visualization.h"
#include "../qmuparser/qmudef.h"
#include "../vgeometry/varc.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "vtheme.h"
#include "../widgets/formulainputwidget.h"
#include "ui_dialogarc.h"

//---------------------------------------------------------------------------------------------------------------------
DialogArc::DialogArc(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogArc),
    m_formulaRadius(new FormulaInputWidget(this)),
    m_formulaF1(new FormulaInputWidget(this)),
    m_formulaF2(new FormulaInputWidget(this))
{
    ui->setupUi(this);

    MakeFilterableComboBox(ui->comboBoxBasePoint);

    ui->doubleSpinBoxApproximationScale->setMaximum(maxCurveApproximationScale);

    // --- Configure formula widgets ---
    const QString lengthPostfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaRadius->setData(&this->data);
    m_formulaRadius->setToolId(toolId);
    m_formulaRadius->setLabelText(tr("Radius:"));
    m_formulaRadius->setPostfix(lengthPostfix);
    m_formulaRadius->setDialogTitle(tr("Edit radius"));
    connect(m_formulaRadius, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    m_formulaF1->setData(&this->data);
    m_formulaF1->setToolId(toolId);
    m_formulaF1->setLabelText(tr("First angle:"));
    m_formulaF1->setPostfix(degreeSymbol);
    m_formulaF1->setDialogTitle(tr("Edit first angle"));
    connect(m_formulaF1, &FormulaInputWidget::valueChanged, this,
            [this]() { m_angleF1 = m_formulaF1->value(); CheckState(); });

    m_formulaF2->setData(&this->data);
    m_formulaF2->setToolId(toolId);
    m_formulaF2->setLabelText(tr("Second angle:"));
    m_formulaF2->setPostfix(degreeSymbol);
    m_formulaF2->setDialogTitle(tr("Edit second angle"));
    connect(m_formulaF2, &FormulaInputWidget::valueChanged, this,
            [this]() { m_angleF2 = m_formulaF2->value(); CheckState(); });

    // Replace formula placeholders in the UI layout with the actual widgets.
    if (auto *placeholder = ui->formulaRadiusPlaceholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formulaRadius);
            placeholder->deleteLater();
        }
    }
    if (auto *placeholder = ui->formulaF1Placeholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formulaF1);
            placeholder->deleteLater();
        }
    }
    if (auto *placeholder = ui->formulaF2Placeholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formulaF2);
            placeholder->deleteLater();
        }
    }

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxBasePoint);
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &DialogArc::ValidateAlias);

    vis = new VisToolArc(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::IsValid() const -> bool
{
    return m_formulaRadius->isValid() && m_formulaF1->isValid() && m_formulaF2->isValid() && m_flagAlias;
}

//---------------------------------------------------------------------------------------------------------------------
DialogArc::~DialogArc()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SetCenter(const quint32 &value)
{
    ChangeCurrentData(ui->comboBoxBasePoint, value);

    auto *path = qobject_cast<VisToolArc *>(vis);
    SCASSERT(path != nullptr)
    path->SetCenterId(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SetF2(const QString &value)
{
    m_formulaF2->setFormula(value);

    if (auto *path = qobject_cast<VisToolArc *>(vis))
    {
        path->SetF2(m_formulaF2->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::GetPenStyle() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SetPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::GetColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SetColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::GetApproximationScale() const -> qreal
{
    return ui->doubleSpinBoxApproximationScale->value();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SetApproximationScale(qreal value)
{
    ui->doubleSpinBoxApproximationScale->setValue(value);

    auto *path = qobject_cast<VisToolArc *>(vis);
    SCASSERT(path != nullptr)
    path->SetApproximationScale(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SetAliasSuffix(const QString &alias)
{
    m_originAliasSuffix = alias;
    ui->lineEditAlias->setText(m_originAliasSuffix);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::GetAliasSuffix() const -> QString
{
    return ui->lineEditAlias->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::ShowDialog(bool click)
{
    if (prepare)
    {
        auto *arcVis = qobject_cast<VisToolArc *>(vis);
        SCASSERT(arcVis != nullptr)

        if (click)
        {
            if (not m_firstRelease)
            {
                m_firstRelease = true;
                return;
            }

            auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
            SCASSERT(scene != nullptr)
            const QSharedPointer<VPointF> point = data.GeometricObject<VPointF>(GetCenter());
            auto const line = QLineF(static_cast<QPointF>(*point), scene->getScenePos());

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

            if (m_stageRadius)
            {
                if (line.length() <= ScaledRadius(SceneScale(VAbstractValApplication::VApp()->getCurrentScene())) * 1.5)
                {
                    return;
                }

                SetRadius(QString::number(VAbstractValApplication::VApp()->fromPixel(line.length())));
                arcVis->RefreshGeometry();
                m_stageRadius = false;
                m_stageF1 = true;
            }
            else if (m_stageF1)
            {
                SetF1(QString::number(Angle()));
                arcVis->RefreshGeometry();
                m_stageF1 = false;
            }
            else
            {
                SetF2(QString::number(arcVis->StickyEnd(Angle())));

                FinishCreating();
            }
            return;
        }

        FinishCreating();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxBasePoint->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SetF1(const QString &value)
{
    m_formulaF1->setFormula(value);

    if (auto *path = qobject_cast<VisToolArc *>(vis))
    {
        path->SetF1(m_formulaF1->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SetRadius(const QString &value)
{
    m_formulaRadius->setFormula(value);

    if (auto *path = qobject_cast<VisToolArc *>(vis))
    {
        path->SetRadius(m_formulaRadius->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::ChosenObject(quint32 id, const SceneObject &type)
{
    if (not prepare && type == SceneObject::Point &&
        SetObject(id, ui->comboBoxBasePoint, QString()))
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
void DialogArc::ShowVisualization()
{
    AddVisualization<VisToolArc>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::SaveData()
{
    auto *path = qobject_cast<VisToolArc *>(vis);
    SCASSERT(path != nullptr)

    const QString radius = m_formulaRadius->formula();
    const QString f1 = m_formulaF1->formula();
    const QString f2 = m_formulaF2->formula();

    path->SetCenterId(GetCenter());
    path->SetRadius(radius);
    path->SetF1(f1);
    path->SetF2(f2);
    path->SetApproximationScale(ui->doubleSpinBoxApproximationScale->value());
    path->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }

    if (event->type() == QEvent::PaletteChange)
    {
        InitDialogButtonBoxIcons(ui->buttonBox);
    }

    DialogTool::changeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::ValidateAlias()
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
void DialogArc::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();

    emit ToolTip(QString());

    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArc::InitIcons()
{
    // Formula-related icons are now managed by FormulaInputWidget.
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::GetCenter() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxBasePoint);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::GetRadius() const -> QString
{
    return m_formulaRadius->formula();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::GetF1() const -> QString
{
    return m_formulaF1->formula();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArc::GetF2() const -> QString
{
    return m_formulaF2->formula();
}
