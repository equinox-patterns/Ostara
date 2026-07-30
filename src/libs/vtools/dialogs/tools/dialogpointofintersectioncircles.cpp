/************************************************************************
 **
 **  @file   dialogpointofintersectioncircles.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   29 5, 2015
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

#include "dialogpointofintersectioncircles.h"

#include <QColor>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <climits>

#include "../../visualization/line/vistoolpointofintersectioncircles.h"
#include "../../visualization/visualization.h"
#include "../vgeometry/vpointf.h"
#include "../widgets/formulainputwidget.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "ui_dialogpointofintersectioncircles.h"

//---------------------------------------------------------------------------------------------------------------------
DialogPointOfIntersectionCircles::DialogPointOfIntersectionCircles(const VContainer *data, VAbstractPattern *doc,
                                                                   quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogPointOfIntersectionCircles),
    m_formulaCircle1(new FormulaInputWidget(this)),
    m_formulaCircle2(new FormulaInputWidget(this))
{
    ui->setupUi(this);

    InitIcons();

    MakeFilterableComboBox(ui->comboBoxCircle1Center);
    MakeFilterableComboBox(ui->comboBoxCircle2Center);

    ui->lineEditNamePoint->setClearButtonEnabled(true);

    ui->lineEditNamePoint->setText(
        VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));

    const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaCircle1->setData(&this->data);
    m_formulaCircle1->setToolId(toolId);
    m_formulaCircle1->setLabelText(tr("Radius of the first circle:"));
    m_formulaCircle1->setPostfix(postfix);
    m_formulaCircle1->setDialogTitle(tr("Edit first circle radius"));
    m_formulaCircle1->setCheckLessThanZero(true);
    connect(m_formulaCircle1, &FormulaInputWidget::valueChanged, this, [this]() { CheckState(); });

    m_formulaCircle2->setData(&this->data);
    m_formulaCircle2->setToolId(toolId);
    m_formulaCircle2->setLabelText(tr("Radius of the second circle:"));
    m_formulaCircle2->setPostfix(postfix);
    m_formulaCircle2->setDialogTitle(tr("Edit second circle radius"));
    m_formulaCircle2->setCheckLessThanZero(true);
    connect(m_formulaCircle2, &FormulaInputWidget::valueChanged, this, [this]() { CheckState(); });

    if (auto *placeholder = ui->formulaPlaceholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formulaCircle1);
            placeholder->deleteLater();
        }
    }
    if (auto *placeholder = ui->formula2Placeholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formulaCircle2);
            placeholder->deleteLater();
        }
    }

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxCircle1Center);
    FillComboBoxPoints(ui->comboBoxCircle2Center);
    FillComboBoxCrossCirclesPoints(ui->comboBoxResult);

    connect(ui->lineEditNamePoint, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckPointLabel(this, ui->lineEditNamePoint, ui->labelEditNamePoint, m_pointName, &this->data,
                                m_flagName);
                CheckState();
            });
    connect(ui->comboBoxCircle1Center, &QComboBox::currentTextChanged, this,
            &DialogPointOfIntersectionCircles::PointChanged);
    connect(ui->comboBoxCircle2Center, &QComboBox::currentTextChanged, this,
            &DialogPointOfIntersectionCircles::PointChanged);

    vis = new VisToolPointOfIntersectionCircles(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogPointOfIntersectionCircles::~DialogPointOfIntersectionCircles()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointOfIntersectionCircles::GetPointName() const -> QString
{
    return m_pointName;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::SetPointName(const QString &value)
{
    m_pointName = value;
    ui->lineEditNamePoint->setText(m_pointName);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointOfIntersectionCircles::GetFirstCircleCenterId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxCircle1Center);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::SetFirstCircleCenterId(const quint32 &value)
{
    setCurrentPointId(ui->comboBoxCircle1Center, value);

    auto *point = qobject_cast<VisToolPointOfIntersectionCircles *>(vis);
    SCASSERT(point != nullptr)
    point->SetCircle1Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointOfIntersectionCircles::GetSecondCircleCenterId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxCircle2Center);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::SetSecondCircleCenterId(const quint32 &value)
{
    setCurrentPointId(ui->comboBoxCircle2Center, value);

    auto *point = qobject_cast<VisToolPointOfIntersectionCircles *>(vis);
    SCASSERT(point != nullptr)
    point->SetCircle2Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointOfIntersectionCircles::GetFirstCircleRadius() const -> QString
{
    return m_formulaCircle1->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::SetFirstCircleRadius(const QString &value)
{
    m_formulaCircle1->setFormula(value);

    if (auto *point = qobject_cast<VisToolPointOfIntersectionCircles *>(vis))
    {
        point->SetC1Radius(value);
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointOfIntersectionCircles::GetSecondCircleRadius() const -> QString
{
    return m_formulaCircle2->formula();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::SetSecondCircleRadius(const QString &value)
{
    m_formulaCircle2->setFormula(value);

    if (auto *point = qobject_cast<VisToolPointOfIntersectionCircles *>(vis))
    {
        point->SetC2Radius(value);
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointOfIntersectionCircles::GetCrossCirclesPoint() const -> CrossCirclesPoint
{
    return getCurrentCrossPoint<CrossCirclesPoint>(ui->comboBoxResult);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::SetCrossCirclesPoint(const CrossCirclesPoint &p)
{
    const qint32 index = ui->comboBoxResult->findData(static_cast<int>(p));
    if (index != -1)
    {
        ui->comboBoxResult->setCurrentIndex(index);

        auto *point = qobject_cast<VisToolPointOfIntersectionCircles *>(vis);
        SCASSERT(point != nullptr)
        point->SetCrossPoint(p);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::ShowDialog(bool click)
{
    if (m_stage == 0 || m_stage == 2)
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

        QSharedPointer<VPointF> const center =
            data.GeometricObject<VPointF>(m_stage == 1 ? GetFirstCircleCenterId() : GetSecondCircleCenterId());

        QLineF const line(static_cast<QPointF>(*center), scene->getScenePos());

        if (m_stage == 1)
        {
            SetFirstCircleRadius(QString::number(FromPixel(line.length(), *data.GetPatternUnit())));
            emit ToolTip(tr("Select second circle center"));
            ++m_stage;
            m_firstRelease = false;
        }
        else
        {
            SetSecondCircleRadius(QString::number(FromPixel(line.length(), *data.GetPatternUnit())));
            ++m_stage;
        }

        if (m_stage > 2)
        {
            FinishCreating();
        }

        return;
    }

    FinishCreating();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditNamePoint->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if (type == SceneObject::Point)
    {
        auto *point = qobject_cast<VisToolPointOfIntersectionCircles *>(vis);
        SCASSERT(point != nullptr)

        switch (m_stage)
        {
            case 0:
                if (SetObject(id, ui->comboBoxCircle1Center, QString()))
                {
                    point->VisualMode(id);

                    if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
                    {
                        m_stage = 2;
                        emit ToolTip(tr("Select second circle center"));
                    }
                    else
                    {
                        ++m_stage;
                        auto *window =
                            qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
                        SCASSERT(window != nullptr)
                        connect(vis.data(), &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);
                    }
                }
                break;
            case 2:
                if (getCurrentObjectId(ui->comboBoxCircle1Center) != id &&
                    SetObject(id, ui->comboBoxCircle2Center, QString()))
                {
                    point->SetCircle2Id(id);
                    point->RefreshGeometry();
                    ++m_stage;
                    prepare = true;

                    if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
                    {
                        FinishCreating();
                        return;
                    }
                }
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::PointChanged()
{
    QColor color;
    if (getCurrentObjectId(ui->comboBoxCircle1Center) == getCurrentObjectId(ui->comboBoxCircle2Center))
    {
        m_flagError = false;
        color = errorColor;
    }
    else
    {
        m_flagError = true;
        color = OkColor(this);
    }
    ChangeColor(ui->labelCircle1Center, color);
    ChangeColor(ui->labelCircle1Center, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::ShowVisualization()
{
    AddVisualization<VisToolPointOfIntersectionCircles>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::SaveData()
{
    m_pointName = ui->lineEditNamePoint->text();

    auto *point = qobject_cast<VisToolPointOfIntersectionCircles *>(vis);
    SCASSERT(point != nullptr)

    point->SetCircle1Id(GetFirstCircleCenterId());
    point->SetCircle2Id(GetSecondCircleCenterId());
    point->SetC1Radius(m_formulaCircle1->formula());
    point->SetC2Radius(m_formulaCircle2->formula());
    point->SetCrossPoint(GetCrossCirclesPoint());
    point->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::closeEvent(QCloseEvent *event)
{
    m_formulaCircle1->blockSignals(true);
    m_formulaCircle2->blockSignals(true);
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::changeEvent(QEvent *event)
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
void DialogPointOfIntersectionCircles::FinishCreating()
{
    vis->SetMode(Mode::Show);
    vis->RefreshGeometry();

    emit ToolTip(QString());

    setModal(true);
    show();
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointOfIntersectionCircles::IsValid() const -> bool
{
    return m_formulaCircle1->isValid() && m_formulaCircle2->isValid() && m_flagName && m_flagError;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::InitIcons()
{
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPointOfIntersectionCircles::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPointOfIntersectionCircles::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}
