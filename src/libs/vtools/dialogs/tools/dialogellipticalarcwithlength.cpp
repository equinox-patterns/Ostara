/************************************************************************
 **
 **  @file   dialogellipticalarcwithlength.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   27 10, 2025
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

#include "dialogellipticalarcwithlength.h"

#include <QLabel>
#include <QPointer>
#include <QPushButton>

#include "../../visualization/path/vistoolellipticalarcwithlength.h"
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
#include "ui_dialogellipticalarcwithlength.h"

//---------------------------------------------------------------------------------------------------------------------
DialogEllipticalArcWithLength::DialogEllipticalArcWithLength(const VContainer *data,
                                                             VAbstractPattern *doc,
                                                             quint32 toolId,
                                                             QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogEllipticalArcWithLength),
    m_formulaRadius1(new FormulaInputWidget(this)),
    m_formulaRadius2(new FormulaInputWidget(this)),
    m_formulaF1(new FormulaInputWidget(this)),
    m_formulaLength(new FormulaInputWidget(this)),
    m_formulaRotationAngle(new FormulaInputWidget(this))
{
    ui->setupUi(this);

    MakeFilterableComboBox(ui->comboBoxCenter);

    ui->doubleSpinBoxApproximationScale->setMaximum(maxCurveApproximationScale);

    // --- Configure formula widgets ---
    const QString lengthPostfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaRadius1->setData(&this->data);
    m_formulaRadius1->setToolId(toolId);
    m_formulaRadius1->setLabelText(tr("Radius1:"));
    m_formulaRadius1->setPostfix(lengthPostfix);
    m_formulaRadius1->setDialogTitle(tr("Edit radius1"));
    m_formulaRadius1->setCheckLessThanZero(true);
    connect(m_formulaRadius1, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    m_formulaRadius2->setData(&this->data);
    m_formulaRadius2->setToolId(toolId);
    m_formulaRadius2->setLabelText(tr("Radius2:"));
    m_formulaRadius2->setPostfix(lengthPostfix);
    m_formulaRadius2->setDialogTitle(tr("Edit radius2"));
    m_formulaRadius2->setCheckLessThanZero(true);
    connect(m_formulaRadius2, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    m_formulaF1->setData(&this->data);
    m_formulaF1->setToolId(toolId);
    m_formulaF1->setLabelText(tr("First angle:"));
    m_formulaF1->setPostfix(degreeSymbol);
    m_formulaF1->setDialogTitle(tr("Edit the first angle"));
    connect(m_formulaF1, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    m_formulaLength->setData(&this->data);
    m_formulaLength->setToolId(toolId);
    m_formulaLength->setLabelText(tr("Length:"));
    m_formulaLength->setPostfix(lengthPostfix);
    m_formulaLength->setDialogTitle(tr("Edit the elliptical arc length"));
    connect(m_formulaLength, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    m_formulaRotationAngle->setData(&this->data);
    m_formulaRotationAngle->setToolId(toolId);
    m_formulaRotationAngle->setLabelText(tr("Rotation angle:"));
    m_formulaRotationAngle->setPostfix(degreeSymbol);
    m_formulaRotationAngle->setDialogTitle(tr("Edit rotation angle"));
    connect(m_formulaRotationAngle, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    // Replace formula placeholders in the UI layout with the actual widgets.
    auto replacePlaceholder = [this](const char *name, FormulaInputWidget *widget)
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
    replacePlaceholder("formulaLengthPlaceholder", m_formulaLength);
    replacePlaceholder("formulaRotationAnglePlaceholder", m_formulaRotationAngle);

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxCenter);
    InitColorPicker(ui->pushButtonColor, VAbstractValApplication::VApp()->ValentinaSettings()->GetUserToolColors());
    ui->pushButtonColor->setUseNativeDialog(!VAbstractApplication::VApp()->Settings()->IsDontUseNativeDialog());
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &DialogEllipticalArcWithLength::ValidateAlias);

    vis = new VisToolEllipticalArcWithLength(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::IsValid() const -> bool
{
    return m_formulaRadius1->isValid() && m_formulaRadius2->isValid() && m_formulaF1->isValid()
           && m_formulaLength->isValid() && m_formulaRotationAngle->isValid() && m_flagAlias;
}

//---------------------------------------------------------------------------------------------------------------------
DialogEllipticalArcWithLength::~DialogEllipticalArcWithLength()
{
    VAbstractValApplication::VApp()->ValentinaSettings()->SetUserToolColors(ui->pushButtonColor->CustomColors());
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetCenter() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxCenter);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetCenter(const quint32 &value)
{
    ChangeCurrentData(ui->comboBoxCenter, value);
    auto *path = qobject_cast<VisToolEllipticalArcWithLength *>(vis);
    SCASSERT(path != nullptr)
    path->SetCenterId(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetRadius1() const -> QString
{
    return m_formulaRadius1->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetRadius1(const QString &value)
{
    m_formulaRadius1->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArcWithLength *>(vis))
    {
        path->SetRadius1(m_formulaRadius1->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetRadius2() const -> QString
{
    return m_formulaRadius2->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetRadius2(const QString &value)
{
    m_formulaRadius2->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArcWithLength *>(vis))
    {
        path->SetRadius2(m_formulaRadius2->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetF1() const -> QString
{
    return m_formulaF1->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetF1(const QString &value)
{
    m_formulaF1->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArcWithLength *>(vis))
    {
        path->SetF1(m_formulaF1->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetLength() const -> QString
{
    return m_formulaLength->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetLength(const QString &value)
{
    m_formulaLength->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArcWithLength *>(vis))
    {
        path->SetLength(m_formulaLength->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetRotationAngle() const -> QString
{
    return m_formulaRotationAngle->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetRotationAngle(const QString &value)
{
    m_formulaRotationAngle->setFormula(value);

    if (auto *path = qobject_cast<VisToolEllipticalArcWithLength *>(vis))
    {
        path->SetRotationAngle(m_formulaRotationAngle->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetPenStyle() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetColor() const -> QString
{
    return ui->pushButtonColor->currentColor().name();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetColor(const QString &value)
{
    ui->pushButtonColor->setCurrentColor(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetApproximationScale() const -> qreal
{
    return ui->doubleSpinBoxApproximationScale->value();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetApproximationScale(qreal value)
{
    ui->doubleSpinBoxApproximationScale->setValue(value);

    auto *path = qobject_cast<VisToolEllipticalArcWithLength *>(vis);
    SCASSERT(path != nullptr)
    path->SetApproximationScale(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SetAliasSuffix(const QString &alias)
{
    m_originAliasSuffix = alias;
    ui->lineEditAlias->setText(m_originAliasSuffix);
    ValidateAlias();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEllipticalArcWithLength::GetAliasSuffix() const -> QString
{
    return ui->lineEditAlias->text();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::ShowDialog(bool click)
{
    if (!prepare)
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

        /*We will ignore click if pointer is in point circle*/
        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)
        const QSharedPointer<VPointF> center = data.GeometricObject<VPointF>(GetCenter());
        auto line = QLineF(static_cast<QPointF>(*center), scene->getScenePos());

        auto Angle = [&line]()
        {
            if (QGuiApplication::keyboardModifiers() == Qt::ShiftModifier)
            {
                QLineF correction = line;
                correction.setAngle(VisToolEllipticalArcWithLength::CorrectAngle(correction.angle()));
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

        if (m_stage == 3) // length
        {
            const qreal r1 = Visualization::FindLengthFromUser(m_formulaRadius1->formula(), data.DataVariables());
            const qreal r2 = Visualization::FindLengthFromUser(m_formulaRadius2->formula(), data.DataVariables());
            const qreal angle1 = Visualization::FindValFromUser(m_formulaF1->formula(), data.DataVariables());
            VEllipticalArc const arc(*center, r1, r2, angle1, line.angle(), 0);

            SetLength(QString::number(VAbstractValApplication::VApp()->fromPixel(arc.GetLength())));
            vis->RefreshGeometry();
            ++m_stage;
            return;
        }

        auto *path = qobject_cast<VisToolEllipticalArcWithLength *>(vis);
        SCASSERT(path != nullptr)

        SetRotationAngle(QString::number(Angle() - path->StartingRotationAngle()));
    }

    FinishCreating();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxCenter->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if (type != SceneObject::Point)
    {
        return;
    }

    if (SetObject(id, ui->comboBoxCenter, QString()))
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
void DialogEllipticalArcWithLength::ShowVisualization()
{
    AddVisualization<VisToolEllipticalArcWithLength>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::SaveData()
{
    auto *path = qobject_cast<VisToolEllipticalArcWithLength *>(vis);
    SCASSERT(path != nullptr)

    const QString radius1 = m_formulaRadius1->formula();
    const QString radius2 = m_formulaRadius2->formula();
    const QString f1 = m_formulaF1->formula();
    const QString length = m_formulaLength->formula();
    const QString rotationAngle = m_formulaRotationAngle->formula();

    path->SetCenterId(GetCenter());
    path->SetRadius1(radius1);
    path->SetRadius2(radius2);
    path->SetF1(f1);
    path->SetLength(length);
    path->SetRotationAngle(rotationAngle);
    path->SetApproximationScale(ui->doubleSpinBoxApproximationScale->value());
    path->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::changeEvent(QEvent *event)
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
void DialogEllipticalArcWithLength::ValidateAlias()
{
    VEllipticalArc arc;
    arc.SetAliasSuffix(GetAliasSuffix());
    if (QRegularExpression const rx(NameRegExp());
        not GetAliasSuffix().isEmpty()
        && (not rx.match(arc.GetAlias()).hasMatch()
            || (m_originAliasSuffix != GetAliasSuffix() && not data.IsUnique(arc.GetAlias()))))
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
void DialogEllipticalArcWithLength::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();

    emit ToolTip(QString());

    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEllipticalArcWithLength::InitIcons()
{
    // Formula-related icons are now managed by FormulaInputWidget.
}
