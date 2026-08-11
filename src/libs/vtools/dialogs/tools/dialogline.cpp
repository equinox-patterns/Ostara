/************************************************************************
 **
 **  @file   dialogline.cpp
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

#include "dialogline.h"

#include <QColor>
#include <QComboBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPointer>
#include <QRegularExpression>
#include <QVariant>

#include "../../visualization/line/vistoolline.h"
#include "../../visualization/visualization.h"
#include "../ifc/ifcdef.h"
#include "../vmisc/vvalentinasettings.h"
#include "../vpatterndb/variables/vlinelength.h"
#include "dialogtool.h"
#include "ui_dialogline.h"

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief DialogLine create dialog
 * @param data container with data
 * @param parent parent widget
 */
DialogLine::DialogLine(const VContainer *data, VAbstractPattern *doc, quint32 toolId, QWidget *parent)
  : DialogTool(data, doc, toolId, parent),
    ui(new Ui::DialogLine),
    flagError(true)
{
    ui->setupUi(this);
    InitOkCancelApply(ui);

    // Re-add spanning 2 rows
    ui->gridLayout->removeItem(ui->gridLayout->itemAtPosition(0, 2));
    ui->gridLayout->addLayout(ui->verticalLayoutSwap, 0, 2, 2, 1);

    MakeFilterableComboBox(ui->comboBoxFirstPoint);
    MakeFilterableComboBox(ui->comboBoxSecondPoint);

    FillComboBoxPoints(ui->comboBoxFirstPoint);
    FillComboBoxPoints(ui->comboBoxSecondPoint);
    m_colorCombo = ReplaceColorPickerWithCombo(this, ui->pushButtonLineColor);

    QMap<QString, QIcon> stylesPics = LineStylesPics(ui->comboBoxLineType->palette().color(QPalette::Base),
                                                     ui->comboBoxLineType->palette().color(QPalette::Text));
    stylesPics.remove(TypeLineNone); // Prevent hiding line
    FillComboBoxTypeLine(ui->comboBoxLineType, stylesPics);

    connect(ui->comboBoxFirstPoint, &QComboBox::currentTextChanged, this, &DialogLine::PointNameChanged);
    connect(ui->comboBoxSecondPoint, &QComboBox::currentTextChanged, this, &DialogLine::PointNameChanged);
    connect(ui->toolButtonSwapPoints, &QToolButton::clicked, this, &DialogLine::SwapPoints);

    // Role name field (Line_CarrureDevant)
    m_labelRoleName = new QLabel(tr("Name:"));
    m_labelRoleName->setMinimumWidth(90);
    auto *lineEditRoleName = new QLineEdit();
    lineEditRoleName->setObjectName(QStringLiteral("lineEditRoleName"));
    ui->gridLayout->addWidget(m_labelRoleName, 4, 0);
    ui->gridLayout->addWidget(lineEditRoleName, 4, 1);
    connect(lineEditRoleName, &QLineEdit::textEdited, this, &DialogLine::ValidateLineRoleName);

    vis = new VisToolLine(&this->data);

    ui->tabWidget->setCurrentIndex(0);
    SetTabStopDistance(ui->plainTextEditToolNotes);
}

//---------------------------------------------------------------------------------------------------------------------
DialogLine::~DialogLine()
{    delete ui;
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetSecondPoint set id second point
 * @param value id
 */
void DialogLine::SetSecondPoint(quint32 value)
{
    setCurrentPointId(ui->comboBoxSecondPoint, value);

    auto *line = qobject_cast<VisToolLine *>(vis);
    SCASSERT(line != nullptr)
    line->SetPoint2Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetTypeLine set type of line
 * @param value type
 */
void DialogLine::SetTypeLine(const QString &value)
{
    ChangeCurrentData(ui->comboBoxLineType, value);
    vis->SetLineStyle(LineStyleToPenStyle(value));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogLine::GetLineColor() const -> QString
{
    return GetColorComboCurrentName(m_colorCombo);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogLine::SetLineColor(const QString &value)
{
    SetColorComboCurrentName(m_colorCombo, value);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief SetFirstPoint set id first point
 * @param value id
 */
void DialogLine::SetFirstPoint(quint32 value)
{
    setCurrentPointId(ui->comboBoxFirstPoint, value);

    auto *line = qobject_cast<VisToolLine *>(vis);
    SCASSERT(line != nullptr)
    line->SetPoint1Id(value);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogLine::PointNameChanged()
{
    QColor color;
    if (getCurrentObjectId(ui->comboBoxFirstPoint) == getCurrentObjectId(ui->comboBoxSecondPoint))
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
void DialogLine::ShowVisualization()
{
    AddVisualization<VisToolLine>();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogLine::SaveData()
{
    auto *line = qobject_cast<VisToolLine *>(vis);
    SCASSERT(line != nullptr)

    line->SetPoint1Id(GetFirstPoint());
    line->SetPoint2Id(GetSecondPoint());
    line->SetLineStyle(LineStyleToPenStyle(GetTypeLine()));
    line->RefreshGeometry();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief ChoosedObject gets id and type of selected object. Save right data and ignore wrong.
 * @param id id of point or detail
 * @param type type of object
 */
void DialogLine::ChosenObject(quint32 id, const SceneObject &type)
{
    if (prepare == false && type == SceneObject::Point) // After first choose we ignore all objects
    {
        switch (number)
        {
            case 0:
                if (SetObject(id, ui->comboBoxFirstPoint, tr("Select second point")))
                {
                    number++;
                    if (vis != nullptr)
                    {
                        vis->VisualMode(id);
                    }
                }
                break;
            case 1:
                if (getCurrentObjectId(ui->comboBoxFirstPoint) != id &&
                    SetObject(id, ui->comboBoxSecondPoint, QString()) && flagError)
                {
                    number = 0;
                    prepare = true;
                    DialogAccepted();
                }
                break;
            default:
                break;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetFirstPoint return id first point
 * @return id
 */
auto DialogLine::GetFirstPoint() const -> quint32
{
    return qvariant_cast<quint32>(ui->comboBoxFirstPoint->currentData());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetSecondPoint return id second point
 * @return id
 */
auto DialogLine::GetSecondPoint() const -> quint32
{
    return qvariant_cast<quint32>(ui->comboBoxSecondPoint->currentData());
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief GetTypeLine return type of line
 * @return type
 */
auto DialogLine::GetTypeLine() const -> QString
{
    return GetComboBoxCurrentData(ui->comboBoxLineType, TypeLineLine);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogLine::SetNotes(const QString &notes)
{
    ui->plainTextEditToolNotes->setPlainText(notes);
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogLine::GetNotes() const -> QString
{
    return ui->plainTextEditToolNotes->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogLine::SwapPoints()
{
    const quint32 first = GetFirstPoint();
    const quint32 second = GetSecondPoint();
    SetFirstPoint(second);
    SetSecondPoint(first);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogLine::CheckDependencyTreeComplete()
{
    const bool ready = m_doc->IsPatternGraphComplete();
    ui->comboBoxFirstPoint->setEnabled(ready);
    ui->comboBoxSecondPoint->setEnabled(ready);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogLine::SetLineRoleName(const QString &suffix)
{
    if (auto *edit = findChild<QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        edit->setText(suffix);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogLine::GetLineRoleName() const -> QString
{
    if (const auto *edit = findChild<const QLineEdit *>(QStringLiteral("lineEditRoleName")))
    {
        return edit->text().trimmed();
    }
    return {};
}

//---------------------------------------------------------------------------------------------------------------------
void DialogLine::ValidateLineRoleName()
{
    const auto *edit = findChild<const QLineEdit *>(QStringLiteral("lineEditRoleName"));
    const QString suffix = edit ? edit->text().trimmed() : QString();
    if (suffix.isEmpty())
    {
        flagRoleName = true;
        ChangeColor(m_labelRoleName, OkColor(this));
        CheckState();
        return;
    }

    QRegularExpression const rx(NameRegExp());
    if (!rx.match(suffix).hasMatch())
    {
        flagRoleName = false;
        ChangeColor(m_labelRoleName, errorColor);
        CheckState();
        return;
    }

    // Check uniqueness: Line_<suffix> must not collide with an existing Line_ variable
    // from a different line (OK if it's the same line)
    const QMap<QString, QSharedPointer<VLengthLine>> lines = data.DataLengthLines();
    const QString aliasKey = line_ + suffix;
    if (lines.contains(aliasKey))
    {
        // Check if it's our own line
        const quint32 firstId = GetFirstPoint();
        const quint32 secondId = GetSecondPoint();
        const QSharedPointer<VLengthLine> var = lines.value(aliasKey);
        const QVector<quint32> refs = var->GetReferences();
        if (refs.size() >= 2 && refs.at(0) == firstId && refs.at(1) == secondId)
        {
            // Same line, alias matches — OK
            flagRoleName = true;
            ChangeColor(m_labelRoleName, OkColor(this));
        }
        else
        {
            flagRoleName = false;
            ChangeColor(m_labelRoleName, errorColor);
        }
    }
    else
    {
        flagRoleName = true;
        ChangeColor(m_labelRoleName, OkColor(this));
    }

    CheckState();
}
