/************************************************************************
 **
 **  @file   dialogendline.cpp
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

#include "dialogendline.h"

#include <QDialog>
#include <QLineEdit>
#include <QLineF>
#include <QPointF>
#include <QPointer>
#include <QPushButton>
#include <QSharedPointer>

#include "../../visualization/line/vistoolendline.h"
#include "../../visualization/visualization.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "../vwidgets/global.h"
#include "../vwidgets/vabstractmainwindow.h"
#include "../vwidgets/vmaingraphicsscene.h"
#include "../widgets/formulainputwidget.h"
#include "ui_dialogendline.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogEndLine create dialog
 * @param data container with data
 * @param parent parent widget
 */
DialogEndLine::DialogEndLine(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogEndLine),
    m_formulaLength(new FormulaInputWidget(this)),
    m_formulaAngle(new FormulaInputWidget(this)),
    pointName(),
    m_firstRelease(false),
    flagName(true)
{
    ui->setupUi(this);

    MakeFilterableComboBox(ui->comboBoxBasePoint);

    ui->lineEditNamePoint->setClearButtonEnabled(true);

    ui->lineEditNamePoint->setText(
        VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));

    // --- Configure formula widgets ---
    const QString postfix = UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true);

    m_formulaLength->setData(&this->data);
    m_formulaLength->setToolId(toolId);
    m_formulaLength->setLabelText(tr("Length:"));
    m_formulaLength->setPostfix(postfix);
    m_formulaLength->setDialogTitle(tr("Edit length"));
    connect(m_formulaLength, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    m_formulaAngle->setData(&this->data);
    m_formulaAngle->setToolId(toolId);
    m_formulaAngle->setLabelText(tr("Angle:"));
    m_formulaAngle->setPostfix(degreeSymbol);
    m_formulaAngle->setDialogTitle(tr("Edit angle"));
    connect(m_formulaAngle, &FormulaInputWidget::valueChanged, this,
            [this]() { CheckState(); });

    // Replace formula placeholders in the UI layout with the actual widgets.
    if (auto *placeholder = ui->formulaLengthPlaceholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formulaLength);
            placeholder->deleteLater();
        }
    }
    if (auto *placeholder = ui->formulaAnglePlaceholder)
    {
        auto *parentLayout = qobject_cast<QVBoxLayout *>(placeholder->parentWidget()->layout());
        if (parentLayout != nullptr)
        {
            const int idx = parentLayout->indexOf(placeholder);
            parentLayout->insertWidget(idx, m_formulaAngle);
            placeholder->deleteLater();
        }
    }

    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxBasePoint);
    FillComboBoxTypeLine(ui->comboBoxLineType, LineStylesPics(ui->comboBoxLineType->palette().color(QPalette::Base),
                                                              ui->comboBoxLineType->palette().color(QPalette::Text)));
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonLineColor);

    connect(ui->lineEditNamePoint, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckPointLabel(this, ui->lineEditNamePoint, ui->labelEditNamePoint, pointName, &this->data, flagName);
                CheckState();
            });

    vis = new VisToolEndLine(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEndLine::IsValid() const -> bool
{
    return m_formulaLength->isValid() && m_formulaAngle->isValid() && flagName;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedObject gets id and type of selected object. Save right data and ignore wrong.
 * @param id id of point or detail
 * @param type type of object
 */
void DialogEndLine::ChosenObject(quint32 id, const SceneObject &type)
{
    if (not prepare && type == SceneObject::Point &&
        SetObject(id, ui->comboBoxBasePoint, QString())) // After first choose we ignore all objects
    {
        if (vis != nullptr)
        {
            vis->VisualMode(id);
        }
        auto *window = qobject_cast<VAbstractMainWindow *>(VAbstractValApplication::VApp()->getMainWindow());
        SCASSERT(window != nullptr)
        connect(vis.data(), &Visualization::ToolTip, window, &VAbstractMainWindow::ShowToolTip);
        prepare = true;

        if (not VAbstractValApplication::VApp()->Settings()->IsInteractiveTools())
        {
            FinishCreating();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetPointName set name of point
 * @param value name
 */
void DialogEndLine::SetPointName(const QString &value)
{
    pointName = value;
    ui->lineEditNamePoint->setText(pointName);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetTypeLine set type of line
 * @param value type
 */
void DialogEndLine::SetTypeLine(const QString &value)
{
    ChangeCurrentData(ui->comboBoxLineType, value);
    vis->SetLineStyle(LineStyleToPenStyle(value));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetFormula set string of formula
 * @param value formula
 */
void DialogEndLine::SetFormula(const QString &value)
{
    m_formulaLength->setFormula(value);

    if (auto *line = qobject_cast<VisToolEndLine *>(vis))
    {
        line->SetLength(m_formulaLength->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetAngle set angle of line
 * @param value angle in degree
 */
void DialogEndLine::SetAngle(const QString &value)
{
    m_formulaAngle->setFormula(value);

    if (auto *line = qobject_cast<VisToolEndLine *>(vis))
    {
        line->SetAngle(m_formulaAngle->formula());
    }
    else
    {
        qCWarning(vDialog, "%s: visualization unavailable, skipping preview update.", Q_FUNC_INFO);
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetBasePointId set id base point of line
 * @param value id
 */
void DialogEndLine::SetBasePointId(const quint32 &value)
{
    setCurrentPointId(ui->comboBoxBasePoint, value);

    auto *line = qobject_cast<VisToolEndLine *>(vis);
    SCASSERT(line != nullptr)
    line->SetBasePointId(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEndLine::GetLineColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEndLine::SetLineColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogEndLine::ShowDialog show dialog after finish working with visual part
 * @param click true if need show dialog after click mouse
 */
void DialogEndLine::ShowDialog(bool click)
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
            const QSharedPointer<VPointF> point = data.GeometricObject<VPointF>(GetBasePointId());
            auto const line = QLineF(static_cast<QPointF>(*point), scene->getScenePos());

            // Radius of point circle, but little bigger. Need handle with hover sizes.
            if (line.length() <= ScaledRadius(SceneScale(VAbstractValApplication::VApp()->getCurrentScene())) * 1.5)
            {
                return;
            }
        }
        auto *line = qobject_cast<VisToolEndLine *>(vis);
        SCASSERT(line != nullptr)
        SetAngle(line->Angle()); // Show in dialog angle what user choose
        SetFormula(line->Length());

        FinishCreating();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEndLine::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxBasePoint->setEnabled(ready);
    ui->lineEditNamePoint->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEndLine::ShowVisualization()
{
    AddVisualization<VisToolEndLine>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEndLine::SaveData()
{
    pointName = ui->lineEditNamePoint->text();

    auto *line = qobject_cast<VisToolEndLine *>(vis);
    SCASSERT(line != nullptr)

    const QString lengthFormula = m_formulaLength->formula();
    const QString angleFormula = m_formulaAngle->formula();

    line->SetBasePointId(GetBasePointId());
    line->SetLength(lengthFormula);
    line->SetAngle(angleFormula);
    line->SetLineStyle(LineStyleToPenStyle(GetTypeLine()));
    line->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEndLine::closeEvent(QCloseEvent *event)
{
    DialogTool::closeEvent(event);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEndLine::changeEvent(QEvent *event)
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
void DialogEndLine::FinishCreating()
{
    setModal(true);
    emit ToolTip(QString());
    show();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEndLine::InitIcons()
{
    // Formula-related icons are now managed by FormulaInputWidget.
    // PaletteChange re-initialization is handled by FormulaInputWidget internally.
}

//---------------------------------------------------------------------------------------------------------------------
DialogEndLine::~DialogEndLine()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEndLine::GetPointName() const -> QString
{
    return pointName;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetTypeLine return type of line
 * @return type
 */
auto DialogEndLine::GetTypeLine() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxLineType, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetFormula return string of formula
 * @return formula
 */
auto DialogEndLine::GetFormula() const -> QString
{
    return m_formulaLength->formula();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetAngle return formula angle of line
 * @return angle formula
 */
auto DialogEndLine::GetAngle() const -> QString
{
    return m_formulaAngle->formula();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetBasePointId return id base point of line
 * @return id
 */
auto DialogEndLine::GetBasePointId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxBasePoint);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogEndLine::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogEndLine::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}
