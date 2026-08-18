/************************************************************************
 **
 **  @file   dialogheight.cpp
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

#include "dialogheight.h"

#include <QColor>
#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLineF>
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QSharedPointer>

#include "../../visualization/line/visline.h"
#include "../../visualization/line/vistoolheight.h"
#include "../../visualization/visualization.h"
#include "../ifc/xml/vabstractpattern.h"
#include "../vgeometry/vgobject.h"
#include "../vgeometry/vpointf.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "../vpatterndb/vcontainer.h"
#include "dialogtool.h"
#include "ui_dialogheight.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogHeight create dialog
 * @param data container with data
 * @param parent parent widget
 */
DialogHeight::DialogHeight(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogHeight)
{
    ui->setupUi(this);
    connect(AddShowPointNameCheckbox(ui->formLayoutPoint), &QCheckBox::toggled, this,
            [this](bool checked) { SetShowLabel(checked); });

    MakeFilterableComboBox(ui->comboBoxP1Line);
    MakeFilterableComboBox(ui->comboBoxP2Line);


    ui->lineEditNamePoint->setText(
        VAbstractValApplication::VApp()->getCurrentDocument()->GenerateLabel(LabelType::NewLabel));
    InitOkCancelApply(ui);

    FillComboBoxPoints(ui->comboBoxP1Line);
    FillComboBoxPoints(ui->comboBoxP2Line);
    FillComboBoxTypeLine(ui->comboBoxLineType, LineStylesPics(ui->comboBoxLineType->palette().color(QPalette::Base),
                                                              ui->comboBoxLineType->palette().color(QPalette::Text)));
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonLineColor);

    connect(ui->lineEditNamePoint, &QLineEdit::textChanged, this,
            [this]()
            {
                CheckPointLabel(this, ui->lineEditNamePoint, ui->labelEditNamePoint, m_pointName, &this->data,
                                m_flagName);
                CheckState();
            });
    connect(ui->comboBoxP1Line, &QComboBox::currentTextChanged, this, &DialogHeight::PointNameChanged);
    connect(ui->comboBoxP2Line, &QComboBox::currentTextChanged, this, &DialogHeight::PointNameChanged);

    // Role name field (Line_CarrureDevant)
    m_labelRoleName = new QLabel(tr("Line:"));
    m_labelRoleName->setMinimumWidth(90);
    auto *lineEditRoleName = new QLineEdit();
    lineEditRoleName->setObjectName(QStringLiteral("lineEditRoleName"));
    ui->formLayout_2->insertRow(0, m_labelRoleName, lineEditRoleName);
    connect(lineEditRoleName, &QLineEdit::textEdited, this, &DialogHeight::ValidateLineRoleName);

    // Separator between the line name field and the next field
    auto *lineSeparator = new QFrame();
    lineSeparator->setObjectName(QStringLiteral("notesSeparator"));
    lineSeparator->setFixedHeight(1);
    auto *lineSeparatorLayout = new QHBoxLayout;
    lineSeparatorLayout->setContentsMargins(33, 0, 33, 0);
    lineSeparatorLayout->addWidget(lineSeparator);
    ui->formLayout_2->insertRow(1, lineSeparatorLayout);

    vis = new VisToolHeight(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogHeight::~DialogHeight()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogHeight::GetPointName() const -> QString
{
    return m_pointName;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetPointName set name of point
 * @param value name
 */
void DialogHeight::SetPointName(const QString &value)
{
    m_pointName = value;
    ui->lineEditNamePoint->setText(m_pointName);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetTypeLine set type of line
 * @param value type
 */
void DialogHeight::SetTypeLine(const QString &value)
{
    ChangeCurrentData(ui->comboBoxLineType, value);
    vis->SetLineStyle(LineStyleToPenStyle(value));
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetBasePointId set id base point of height
 * @param value id
 */
void DialogHeight::SetBasePointId(const quint32 &value)
{
    m_basePointId = value;

    auto *line = qobject_cast<VisToolHeight *>(vis);
    SCASSERT(line != nullptr)
    line->SetBasePointId(value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetP1LineId set id first point of line
 * @param value id
 */
void DialogHeight::SetP1LineId(const quint32 &value)
{
    setCurrentPointId(ui->comboBoxP1Line, value);

    auto *line = qobject_cast<VisToolHeight *>(vis);
    SCASSERT(line != nullptr)
    line->SetLineP1Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetP2LineId set id second point of line
 * @param value id
 */
void DialogHeight::SetP2LineId(const quint32 &value)
{
    setCurrentPointId(ui->comboBoxP2Line, value);

    auto *line = qobject_cast<VisToolHeight *>(vis);
    SCASSERT(line != nullptr)
    line->SetLineP2Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogHeight::GetLineColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogHeight::SetLineColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedObject gets id and type of selected object. Save right data and ignore wrong.
 * @param id id of point or detail
 * @param type type of object
 */
void DialogHeight::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare) // After first choose we ignore all objects
    {
        return;
    }

    if (type == SceneObject::Point)
    {
        auto *line = qobject_cast<VisToolHeight *>(vis);
        SCASSERT(line != nullptr)

        switch (m_number)
        {
            case 0:
                m_basePointId = id;
                PointNameChanged();
                m_number++;
                line->VisualMode(id);
                break;
            case 1:
                if (m_basePointId != id &&
                    SetObject(id, ui->comboBoxP1Line, tr("Select second point of line")))
                {
                    m_number++;
                    line->SetLineP1Id(id);
                    line->RefreshGeometry();
                }
                break;
            case 2:
            {
                QSet<quint32> set;
                set.insert(m_basePointId);
                set.insert(getCurrentObjectId(ui->comboBoxP1Line));
                set.insert(id);

                if (set.size() == 3 && SetObject(id, ui->comboBoxP2Line, QString()))
                {
                    line->SetLineP2Id(id);
                    line->RefreshGeometry();
                    prepare = true;
                    DialogAccepted();
                }
            }
            break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogHeight::SaveData()
{
    m_pointName = ui->lineEditNamePoint->text();

    auto *line = qobject_cast<VisToolHeight *>(vis);
    SCASSERT(line != nullptr)

    line->SetBasePointId(GetBasePointId());
    line->SetLineP1Id(GetP1LineId());
    line->SetLineP2Id(GetP2LineId());
    line->SetLineStyle(LineStyleToPenStyle(GetTypeLine()));
    line->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogHeight::PointNameChanged()
{
    QSet<quint32> set;
    const quint32 basePointId = m_basePointId;
    const quint32 p1LineId = getCurrentObjectId(ui->comboBoxP1Line);
    const quint32 p2LineId = getCurrentObjectId(ui->comboBoxP2Line);

    set.insert(basePointId);
    set.insert(p1LineId);
    set.insert(p2LineId);

    QColor color;

    try
    {
        const auto basePoint = static_cast<QPointF>(*data.GeometricObject<VPointF>(basePointId));
        const auto p1Line = static_cast<QPointF>(*data.GeometricObject<VPointF>(p1LineId));
        const auto p2Line = static_cast<QPointF>(*data.GeometricObject<VPointF>(p2LineId));

        if (set.size() != 3 || VGObject::ClosestPoint(QLineF(p1Line, p2Line), basePoint) == QPointF())
        {
            m_flagError = false;
            color = errorColor;
        }
        else
        {
            m_flagError = true;
            color = OkColor(this);
        }
    }
    catch (const VExceptionBadId &)
    {
        m_flagError = false;
        color = errorColor;
    }

    ChangeColor(ui->labelFirstLinePoint, color);
    ChangeColor(ui->labelSecondLinePoint, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogHeight::ShowVisualization()
{
    AddVisualization<VisLine>();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetTypeLine return type of line
 * @return type
 */
auto DialogHeight::GetTypeLine() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxLineType, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetBasePointId return id base point of height
 * @return id
 */
auto DialogHeight::GetBasePointId() const -> quint32
{
    return m_basePointId;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetP1LineId return id first point of line
 * @return id id
 */
auto DialogHeight::GetP1LineId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxP1Line);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetP2LineId return id second point of line
 * @return id
 */
auto DialogHeight::GetP2LineId() const -> quint32
{
    return getCurrentObjectId(ui->comboBoxP2Line);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogHeight::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogHeight::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogHeight::SetLineRoleName(const QString &suffix)
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        edit->setText(suffix);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogHeight::GetLineRoleName() const -> QString
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        return edit->text().trimmed();
    }
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void DialogHeight::ValidateLineRoleName()
{
    auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName"));
    if (!edit)
    {
        return;
    }
    const QString suffix = edit->text().trimmed();
    if (suffix.isEmpty())
    {
        flagRoleName = true;
        if (edit) { edit->setStyleSheet({}); }
        ChangeColor(m_labelRoleName, OkColor(this));
        CheckState();
        return;
    }

    QRegularExpression const rx(NameRegExp());
    if (!rx.match(suffix).hasMatch())
    {
        flagRoleName = false;
        if (edit) { edit->setStyleSheet(QStringLiteral("border: 1px solid red;")); }
        ChangeColor(m_labelRoleName, errorColor);
        CheckState();
        return;
    }

    // Uniqueness: Line_<suffix> must not exist on another line
    const QString aliasKey = line_ + suffix;
    if (data.DataLengthLines().contains(aliasKey))
    {
        const QSharedPointer<VLengthLine> var = data.DataLengthLines().value(aliasKey);
        const QVector<quint32> refs = var->GetReferences();
        if (refs.size() >= 2 && refs.at(0) != NULL_ID && refs.at(1) != NULL_ID)
        {
            flagRoleName = false;
            if (edit) { edit->setStyleSheet(QStringLiteral("border: 1px solid red;")); }
            ChangeColor(m_labelRoleName, errorColor);
            CheckState();
            return;
        }
    }
    flagRoleName = true;
    if (edit) { edit->setStyleSheet({}); }
    ChangeColor(m_labelRoleName, OkColor(this));
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogHeight::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->lineEditNamePoint->setEnabled(ready);
    ui->comboBoxP1Line->setEnabled(ready);
    ui->comboBoxP2Line->setEnabled(ready);
}
