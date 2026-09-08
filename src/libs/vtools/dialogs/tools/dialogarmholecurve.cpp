/************************************************************************
 **
 **  @file   dialogarmholecurve.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/08/2026
 **
 **  @brief  Dialog for the ArmholeCurve tool.
 **
 **  @copyright
 **  Copyright (C) 2026 Sophie Denys <contact@sophiedenys.com>
 **
 **  This program is free software: you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation, either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **
 *************************************************************************/

#include "dialogarmholecurve.h"

#include <QColor>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>

#include "../../visualization/path/ovistoolarmholecurve.h"
#include "../../visualization/visualization.h"
#include "../../tools/drawTools/toolcurve/otoolarmholecurve.h"
#include "../qmuparser/qmudef.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/vcontainer.h"
#include "dialogtool.h"
#include "ui_dialogarmholecurve.h"

//---------------------------------------------------------------------------------------------------------------------
DialogArmholeCurve::DialogArmholeCurve(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogArmholeCurve)
{
    ui->setupUi(this);
    InitOkCancelApply(ui);

    MakeFilterableComboBox(ui->comboBoxP1);
    MakeFilterableComboBox(ui->comboBoxP2);
    MakeFilterableComboBox(ui->comboBoxP3);
    MakeFilterableComboBox(ui->comboBoxP4);

    FillComboBoxPoints(ui->comboBoxP1);
    FillComboBoxPoints(ui->comboBoxP2);
    FillComboBoxPoints(ui->comboBoxP3);
    FillComboBoxPoints(ui->comboBoxP4);
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonColor);
    FillComboBoxTypeLine(ui->comboBoxPenStyle,
                         CurvePenStylesPics(ui->comboBoxPenStyle->palette().color(QPalette::Base),
                                            ui->comboBoxPenStyle->palette().color(QPalette::Text)));

    connect(ui->comboBoxP1, &QComboBox::currentTextChanged, this, &DialogArmholeCurve::PointNameChanged);
    connect(ui->comboBoxP4, &QComboBox::currentTextChanged, this, &DialogArmholeCurve::PointNameChanged);

    connect(ui->lineEditAlias, &QLineEdit::textEdited, this, &DialogArmholeCurve::ValidateAlias);

    vis = new OVisToolArmholeCurve(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogArmholeCurve::~DialogArmholeCurve()
{
    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArmholeCurve::GetPath() const -> VCubicBezierPath
{
    return m_path;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::SetPath(const VCubicBezierPath &path)
{
    m_path = path;

    m_oldName = m_path.name();

    setCurrentPointId(ui->comboBoxP1, m_path.at(0).id()); // armhole
    setCurrentPointId(ui->comboBoxP2, m_path.at(3).id()); // chest/back width
    setCurrentPointId(ui->comboBoxP3, m_path.at(5).id()); // perpendicular
    setCurrentPointId(ui->comboBoxP4, m_path.at(6).id()); // shoulder

    ChangeCurrentData(ui->comboBoxPenStyle, m_path.GetPenStyle());
    SetColorComboCurrentName(m_colorCombo, m_path.GetColor());

    originAliasSuffix = m_path.GetAliasSuffix();
    ui->lineEditAlias->setText(originAliasSuffix);
    ValidateAlias();

    auto *visPath = qobject_cast<OVisToolArmholeCurve *>(vis);
    SCASSERT(visPath != nullptr)
    visPath->SetApproximationScale(m_path.GetApproximationScale());
    visPath->SetPoint1Id(m_path.at(0).id()); // armhole
    visPath->SetPoint2Id(m_path.at(3).id()); // chest/back width
    visPath->SetPoint3Id(m_path.at(5).id()); // perpendicular
    visPath->SetPoint4Id(m_path.at(6).id()); // shoulder
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArmholeCurve::GetT() const -> double
{
    return ui->doubleSpinBoxTMid->value();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::SetT(double value)
{
    ui->doubleSpinBoxTMid->setValue(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare == false && type == SceneObject::Point) // After first choose we ignore all objects
    {
        auto *visPath = qobject_cast<OVisToolArmholeCurve *>(vis);
        SCASSERT(visPath != nullptr)
        if (Q_UNLIKELY(visPath == nullptr))
        {
            return;
        }

        switch (number)
        {
            case 0:
                if (SetObject(id, ui->comboBoxP1, tr("Select the chest/back width point")))
                {
                    ++number;
                    visPath->VisualMode(id);
                }
                break;
            case 1:
                if (SetObject(id, ui->comboBoxP2, tr("Select the perpendicular point")))
                {
                    ++number;

                    visPath->SetPoint2Id(id);
                    visPath->RefreshGeometry();
                }
                break;
            case 2:
                if (getCurrentObjectId(ui->comboBoxP1) != id
                    && getCurrentObjectId(ui->comboBoxP2) != id
                    && SetObject(id, ui->comboBoxP3, tr("Select the shoulder point")))
                {
                    ++number;

                    visPath->SetPoint3Id(id);
                    visPath->RefreshGeometry();
                }
                break;
            case 3:
                if (getCurrentObjectId(ui->comboBoxP1) != id
                    && getCurrentObjectId(ui->comboBoxP2) != id
                    && getCurrentObjectId(ui->comboBoxP3) != id
                    && SetObject(id, ui->comboBoxP4, QString()))
                {
                    ++number;

                    visPath->SetPoint4Id(id);
                    visPath->RefreshGeometry();
                    prepare = true;
                }
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::PointNameChanged()
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
    ChangeColor(ui->labelFourthPoint, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::ShowDialog(bool click)
{
    if (click && number == 4)
    {
        SavePath();
        emit ToolTip(QString());

        if (not data.IsUnique(m_path.name()))
        {
            m_path.SetDuplicate(DNumber(m_path.name()));
        }

        DialogAccepted();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::ShowVisualization()
{
    AddVisualization<OVisToolArmholeCurve>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::SaveData()
{
    SavePath();

    if (!m_oldName.isEmpty() && m_oldName != m_path.name())
    {
        m_path.SetDuplicate(0);
        if (!data.IsUnique(m_path.name()))
        {
            m_path.SetDuplicate(DNumber(m_path.name()));
            m_oldName = m_path.name();
        }
    }

    auto *visPath = qobject_cast<OVisToolArmholeCurve *>(vis);
    SCASSERT(visPath != nullptr)
    visPath->SetApproximationScale(m_path.GetApproximationScale());
    visPath->SetPoint1Id(GetP1()->id());
    visPath->SetPoint2Id(GetP2()->id()); // chest/back width
    visPath->SetPoint3Id(GetP3()->id()); // perpendicular
    visPath->SetPoint4Id(GetP4()->id()); // shoulder
    visPath->SetMode(Mode::Show);
    visPath->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::ValidateAlias()
{
    VCubicBezierPath path = m_path;
    path.SetAliasSuffix(ui->lineEditAlias->text());
    if (QRegularExpression const rx(NameRegExp());
        !ui->lineEditAlias->text().isEmpty()
        && (!rx.match(path.GetAlias()).hasMatch()
            || (originAliasSuffix != ui->lineEditAlias->text() && !data.IsUnique(path.GetAlias()))))
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
auto DialogArmholeCurve::GetP1() const -> QSharedPointer<VPointF>
{
    return data.GeometricObject<VPointF>(getCurrentObjectId(ui->comboBoxP1)); // armhole
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArmholeCurve::GetP2() const -> QSharedPointer<VPointF>
{
    return data.GeometricObject<VPointF>(getCurrentObjectId(ui->comboBoxP2)); // chest/back width
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArmholeCurve::GetP3() const -> QSharedPointer<VPointF>
{
    return data.GeometricObject<VPointF>(getCurrentObjectId(ui->comboBoxP3)); // perpendicular
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArmholeCurve::GetP4() const -> QSharedPointer<VPointF>
{
    return data.GeometricObject<VPointF>(getCurrentObjectId(ui->comboBoxP4)); // shoulder
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::SavePath()
{
    const quint32 d = m_path.GetDuplicate(); // Save previous value

    const double t = ui->doubleSpinBoxTMid->value();

    // Compute the 3 auto-computed control handles for the 2-segment path.
    // The perpendicular point (path[5]) and shoulder (path[6]) are user-selected.
    const ArmholeHandles handles = OToolArmholeCurve::SolveHandles(*GetP1(), *GetP2(), t);

    // Build the 7-point path: armhole, V, V1, chest/back width, h2a, perpendicular, shoulder
    QVector<VPointF> pathPoints;
    pathPoints.append(*GetP1());    // [0] armhole
    pathPoints.append(handles.h1a); // [1] V
    pathPoints.append(handles.h1b); // [2] V1
    pathPoints.append(*GetP2());    // [3] chest/back width
    pathPoints.append(handles.h2a); // [4] h2a
    pathPoints.append(*GetP3());    // [5] perpendicular (user)
    pathPoints.append(*GetP4());    // [6] shoulder (user)

    const qreal approximationScale = m_path.GetApproximationScale(); // Preserve across rebuild
    m_path = VCubicBezierPath(pathPoints);
    m_path.SetApproximationScale(approximationScale);
    m_path.SetPenStyle(GetComboBoxCurrentData(ui->comboBoxPenStyle, TypeLineLine));
    m_path.SetColor(GetColorComboCurrentName(m_colorCombo));
    m_path.SetAliasSuffix(ui->lineEditAlias->text());
    m_path.SetDuplicate(d);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogArmholeCurve::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::SetDefPenStyle(const QString &value)
{
    ChangeCurrentData(ui->comboBoxPenStyle, value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::SetDefColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogArmholeCurve::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxP1->setEnabled(ready);
    ui->comboBoxP4->setEnabled(ready);
    ui->lineEditAlias->setEnabled(ready);
}
