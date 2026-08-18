/************************************************************************
 **
 **  @file   dialogpointfromcircleandtangent.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 6, 2015
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

#include "dialogpointfromcircleandtangent.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <climits>

#include "../../visualization/line/vistoolpointfromcircleandtangent.h"
#include "../../visualization/visualization.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vtranslatevars.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../widgets/formulainputwidget.h"
#include "ui_dialogpointfromcircleandtangent.h"

//---------------------------------------------------------------------------------------------------------------------
DialogPointFromCircleAndTangent::DialogPointFromCircleAndTangent(const VContainer *data, VAbstractPattern *doc,
                                                                 quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogPointFromCircleAndTangent),
    m_formula(new FormulaInputWidget(this))
{
    ui->setupUi(this);
    connect(AddShowPointNameCheckbox(ui->formLayoutPoint), &QCheckBox::toggled, this,
            [this](bool checked) { SetShowLabel(checked); });

    const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);
    m_formula->setData(&this->data);
    m_formula->setToolId(toolId);
    m_formula->setLabelText(tr("Radius:"));
    m_formula->setPostfix(postfix);
    m_formula->setDialogTitle(tr("Edit radius"));
    connect(m_formula, &FormulaInputWidget::valueChanged, this, [this]() { CheckState(); });

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

    InitIcons();

    MakeFilterableComboBox(ui->comboBoxCircleCenter);
    MakeFilterableComboBox(ui->comboBoxTangentPoint);


    ui->lineEditNamePoint->setText(
        VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxCircleCenter);
    FillComboBoxPoints(ui->comboBoxTangentPoint);
    FillComboBoxCrossCirclesPoints(ui->comboBoxResult);

    connect(ui->lineEditNamePoint, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckPointLabel(this, ui->lineEditNamePoint, ui->labelEditNamePoint, m_pointName, &this->data,
                                m_flagName);
                CheckState();
            });
    connect(ui->comboBoxCircleCenter, &QComboBox::currentTextChanged, this,
            &DialogPointFromCircleAndTangent::PointChanged);

    vis = new VisToolPointFromCircleAndTangent(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogPointFromCircleAndTangent::~DialogPointFromCircleAndTangent()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointFromCircleAndTangent::GetPointName() const -> QString
{
    return m_pointName;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::SetPointName(const QString &value)
{
    m_pointName = value;
    ui->lineEditNamePoint->setText(m_pointName);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointFromCircleAndTangent::GetCircleCenterId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxCircleCenter);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::SetCircleCenterId(const quint32 &value)
{
    setCurrentPointId(ui->comboBoxCircleCenter, value);

    auto *point = qobject_cast<VisToolPointFromCircleAndTangent *>(vis);
    SCASSERT(point != nullptr)
    point->SetCenterId(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointFromCircleAndTangent::GetCircleRadius() const -> QString
{
    return m_formula->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::SetCircleRadius(const QString &value)
{
    m_formula->setFormula(value);

    if (auto *point = qobject_cast<VisToolPointFromCircleAndTangent *>(vis))
    {
        point->SetCRadius(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
            m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointFromCircleAndTangent::GetTangentPointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxTangentPoint);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::SetTangentPointId(quint32 value)
{
    setCurrentPointId(ui->comboBoxTangentPoint, value);

    auto *point = qobject_cast<VisToolPointFromCircleAndTangent *>(vis);
    SCASSERT(point != nullptr)
    point->SetPointId(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointFromCircleAndTangent::GetCrossCirclesPoint() const -> CrossCirclesPoint
{
    return getCurrentCrossPoint<CrossCirclesPoint>(ui->comboBoxResult);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::SetCrossCirclesPoint(CrossCirclesPoint p)
{
    const qint32 index = ui->comboBoxResult->findData(static_cast<int>(p));
    if (index != -1)
    {
        ui->comboBoxResult->setCurrentIndex(index);

        auto *point = qobject_cast<VisToolPointFromCircleAndTangent *>(vis);
        SCASSERT(point != nullptr)
        point->SetCrossPoint(p);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::ShowDialog(bool click)
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

        /*We will ignore click if pointer is in point circle*/
        auto *scene = qobject_cast<VMainGraphicsScene *>(VAbstractValApplication::VApp()->getCurrentScene());
        SCASSERT(scene != nullptr)
        const QSharedPointer<VPointF> center = data.GeometricObject<VPointF>(GetCircleCenterId());
        auto const line = QLineF(static_cast<QPointF>(*center), scene->getScenePos());

        SetCircleRadius(QString::number(FromPixel(line.length(), *data.GetPatternUnit())));
    }

    FinishCreating();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditNamePoint->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if (type == SceneObject::Point)
    {
        auto *point = qobject_cast<VisToolPointFromCircleAndTangent *>(vis);
        SCASSERT(point != nullptr)

        switch (m_number)
        {
            case 0:
                if (SetObject(id, ui->comboBoxTangentPoint, tr("Select a circle center")))
                {
                    m_number++;
                    point->VisualMode(id);
                }
                break;
            case 1:
                if (getCurrentObjectId(ui->comboBoxTangentPoint) != id &&
                    SetObject(id, ui->comboBoxCircleCenter, QString()))
                {
                    auto *window =
                        qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
                    SCASSERT(window != nullptr)
                    connect(vis.data(), &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);

                    m_number = 0;
                    point->SetCenterId(id);
                    point->RefreshGeometry();
                    prepare = true;

                    if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
                    {
                        FinishCreating();
                    }
                }
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::PointChanged()
{
    QColor color;
    if (getCurrentObjectId(ui->comboBoxCircleCenter) == getCurrentObjectId(ui->comboBoxTangentPoint))
    {
        m_flagError = false;
        color = errorColor;
    }
    else
    {
        m_flagError = true;
        color = OkColor(this);
    }
    ChangeColor(ui->labelCircleCenter, color);
    ChangeColor(ui->labelTangentPoint, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::ShowVisualization()
{
    AddVisualization<VisToolPointFromCircleAndTangent>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::SaveData()
{
    m_pointName = ui->lineEditNamePoint->text();

    auto *point = qobject_cast<VisToolPointFromCircleAndTangent *>(vis);
    SCASSERT(point != nullptr)

    point->SetPointId(GetTangentPointId());
    point->SetCenterId(GetCircleCenterId());
    point->SetCRadius(VAbstractValApplication::VApp()->TrVars()->FormulaToUser(
        m_formula->formula(), VAbstractValApplication::VApp()->Settings()->GetOsSeparator()));
    point->SetCrossPoint(GetCrossCirclesPoint());
    point->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::changeEvent(QEvent *event)
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
auto DialogPointFromCircleAndTangent::IsValid() const -> bool
{
    return m_formula->isValid() && m_flagName && m_flagError;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();

    emit ToolTip(QString());

    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::InitIcons()
{
    // Formula icons are managed by FormulaInputWidget.
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointFromCircleAndTangent::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointFromCircleAndTangent::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}
