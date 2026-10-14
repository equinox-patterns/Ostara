/************************************************************************
 **
 **  @file   dialogseamallowance_grainline.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief  Grainline tab logic for DialogSeamAllowance.
 **  @note   Extracted from dialogseamallowance.cpp
 **          as part of the Ostara refactoring.
 **
 **  @copyright
 **  This source code is part of the Valentina project, a pattern making
 **  program, whose allow create and modeling patterns of clothing.
 **  Copyright (C) 2016 Valentina project
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

#include "dialogseamallowance.h"
#include "dialogseamallowance_helpers.h"
#include "ui_tabgrainline.h"
#include "../vpatterndb/floatItemData/floatitemdef.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/variables/vmeasurement.h"
#include "../qmuparser/qmuparsererror.h"
#include "../vwidgets/fancytabbar/fancytabbar.h"
#include "../../support/dialogeditwrongformula.h"

#include <QMenu>
#include <QMessageBox>
#include <QTimer>
//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::UpdateGrainlineValues()
{
    std::array<const QPlainTextEdit *, 2> apleSender = {uiTabGrainline->lineEditRotFormula,
                                                        uiTabGrainline->lineEditLenFormula};
    std::array<bool, 2> bFormulasOK = {true, true};

    for (std::size_t i = 0; i < 2; ++i)
    {
        QLabel *plbVal;
        QLabel *plbText;
        QString qsUnit;
        if (i == 0)
        {
            plbVal = uiTabGrainline->labelRot;
            plbText = uiTabGrainline->labelEditRot;
            qsUnit = degreeSymbol;
        }
        else
        {
            plbVal = uiTabGrainline->labelLen;
            plbText = uiTabGrainline->labelEditLen;
            qsUnit = QChar(QChar::Space) + UnitsToStr(VAbstractValApplication::VApp()->patternUnits());
        }

        plbVal->setToolTip(tr("Value"));

        QString qsFormula = apleSender.at(i)->toPlainText().simplified();
        QString qsVal;
        try
        {
            qsFormula = VAbstractApplication::VApp()->TrVars()->FormulaFromUser(
                qsFormula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
            Calculator cal;
            qreal const dVal = cal.EvalFormula(data.DataVariables(), qsFormula);
            if (qIsInf(dVal) || qIsNaN(dVal))
            {
                throw qmu::QmuParserError(tr("Infinite/undefined result"));
            }
            if (i == 1 && dVal <= 0.0)
            {
                throw qmu::QmuParserError(tr("Length should be positive"));
            }

            qsVal.setNum(dVal, 'f', 2);
            ChangeColor(plbText, OkColor(this));
        }
        catch (qmu::QmuParserError &e)
        {
            qsVal = tr("Error");
            not flagGPin ? ChangeColor(plbText, errorColor) : ChangeColor(plbText, OkColor(this));
            bFormulasOK.at(i) = false;
            plbVal->setToolTip(tr("Parser error: %1").arg(e.GetMsg()));
        }

        if (bFormulasOK.at(i) && not qsVal.isEmpty())
        {
            qsVal += qsUnit;
        }
        plbVal->setText(qsVal);
    }

    flagGFormulas = bFormulasOK[0] && bFormulasOK[1];
    if (not flagGFormulas && not flagGPin)
    {
        m_ftb->SetTabText(TabOrder::Grainline, tr("Grainline") + '*');
    }
    else
    {
        ResetGrainlineWarning();
    }
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EnabledGrainline()
{
    if (uiTabGrainline->groupBoxGrainline->isChecked())
    {
        GrainlinePinPointChanged();
    }
    else
    {
        flagGFormulas = true;
        ResetGrainlineWarning();
        CheckState();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EditGrainlineFormula()
{
    QPlainTextEdit *pleFormula;
    bool bCheckZero;
    QString title;

    if (sender() == uiTabGrainline->pushButtonLen)
    {
        pleFormula = uiTabGrainline->lineEditLenFormula;
        bCheckZero = true;
        title = tr("Edit length");
    }
    else if (sender() == uiTabGrainline->pushButtonRot)
    {
        pleFormula = uiTabGrainline->lineEditRotFormula;
        bCheckZero = false;
        title = tr("Edit angle");
    }
    else
    {
        // should not get here!
        return;
    }

    DialogEditWrongFormula dlg(&data, NULL_ID, this);
    dlg.setWindowTitle(title);
    dlg.SetFormula(VTranslateVars::TryFormulaFromUser(pleFormula->toPlainText(),
                                                      VAbstractApplication::VApp()->Settings()->GetOsSeparator()));
    dlg.setCheckZero(bCheckZero);
    if (dlg.exec() == QDialog::Accepted)
    {
        QString const qsFormula = dlg.GetFormula();

        if (sender() == uiTabGrainline->pushButtonLen)
        {
            SetGrainlineLength(qsFormula);
        }
        else if (sender() == uiTabGrainline->pushButtonRot)
        {
            SetGrainlineAngle(qsFormula);
        }
        else
        {
            // should not get here!
            pleFormula->setPlainText(qsFormula);
        }
        UpdateGrainlineValues();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployGrainlineRotation()
{
    DeployFormula(this, uiTabGrainline->lineEditRotFormula, uiTabGrainline->pushButtonShowRot, m_iRotBaseHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::DeployGrainlineLength()
{
    DeployFormula(this, uiTabGrainline->lineEditLenFormula, uiTabGrainline->pushButtonShowLen, m_iLenBaseHeight);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::ResetGrainlineWarning()
{
    if (flagGFormulas || flagGPin)
    {
        m_ftb->SetTabText(TabOrder::Grainline, tr("Grainline"));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::GrainlinePinPointChanged()
{
    QColor color;
    const quint32 topPinId = getCurrentObjectId(uiTabGrainline->comboBoxGrainlineTopPin);
    const quint32 bottomPinId = getCurrentObjectId(uiTabGrainline->comboBoxGrainlineBottomPin);
    if (topPinId != NULL_ID && bottomPinId != NULL_ID && topPinId != bottomPinId)
    {
        flagGPin = true;
        color = OkColor(this);

        ResetGrainlineWarning();
    }
    else
    {
        flagGPin = false;
        topPinId == NULL_ID &&bottomPinId == NULL_ID ? color = OkColor(this) : color = errorColor;

        if (not flagGFormulas && not flagGPin)
        {
            m_ftb->SetTabText(TabOrder::Grainline, tr("Grainline"));
        }
    }
    EnableGrainlineFormulaControls(not flagGPin);
    UpdateGrainlineValues();
    ChangeColor(uiTabGrainline->labelGrainlineTopPin, color);
    ChangeColor(uiTabGrainline->labelGrainlineBottomPin, color);
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitGrainlineTab()
{
    uiTabGrainline->lineEditLenFormula->setPlainText(
        QString().setNum(UnitConvertor(10, Unit::Cm, *data.GetPatternUnit())));

    connect(uiTabGrainline->groupBoxGrainline, &QGroupBox::toggled, this, &DialogSeamAllowance::EnabledGrainline);
    connect(uiTabGrainline->pushButtonRot, &QPushButton::clicked, this, &DialogSeamAllowance::EditGrainlineFormula);
    connect(uiTabGrainline->pushButtonLen, &QPushButton::clicked, this, &DialogSeamAllowance::EditGrainlineFormula);
    connect(uiTabGrainline->lineEditLenFormula, &QPlainTextEdit::textChanged, this,
            &DialogSeamAllowance::UpdateGrainlineValues);
    connect(uiTabGrainline->lineEditRotFormula, &QPlainTextEdit::textChanged, this,
            &DialogSeamAllowance::UpdateGrainlineValues);

    connect(uiTabGrainline->pushButtonShowRot, &QPushButton::clicked, this,
            &DialogSeamAllowance::DeployGrainlineRotation);
    connect(uiTabGrainline->pushButtonShowLen, &QPushButton::clicked, this,
            &DialogSeamAllowance::DeployGrainlineLength);

    EnabledGrainline();

    uiTabGrainline->comboBoxArrow->addItem(tr("Two ways (Up/Down)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::twoWaysUpDown));
    uiTabGrainline->comboBoxArrow->addItem(tr("One way (Up)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::oneWayUp));
    uiTabGrainline->comboBoxArrow->addItem(tr("One way (Down)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::oneWayDown));
    uiTabGrainline->comboBoxArrow->addItem(tr("Four ways", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::fourWays));
    uiTabGrainline->comboBoxArrow->addItem(tr("Two ways (Up/Left)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::twoWaysUpLeft));
    uiTabGrainline->comboBoxArrow->addItem(tr("Two ways (Up/Right)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::twoWaysUpRight));
    uiTabGrainline->comboBoxArrow->addItem(tr("Two ways (Down/Left)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::twoWaysDownLeft));
    uiTabGrainline->comboBoxArrow->addItem(tr("Two ways (Down/Right)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::twoWaysDownRight));
    uiTabGrainline->comboBoxArrow->addItem(tr("Three ways (Up/Down/Left)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::threeWaysUpDownLeft));
    uiTabGrainline->comboBoxArrow->addItem(tr("Three ways (Up/Down/Right)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::threeWaysUpDownRight));
    uiTabGrainline->comboBoxArrow->addItem(tr("Three ways (Up/Left/Right)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::threeWaysUpLeftRight));
    uiTabGrainline->comboBoxArrow->addItem(tr("Three ways (Down/Left/Right)", "grainline direction"),
                                           static_cast<int>(GrainlineArrowDirection::threeWaysDownLeftRight));

    m_iRotBaseHeight = uiTabGrainline->lineEditRotFormula->height();
    m_iLenBaseHeight = uiTabGrainline->lineEditLenFormula->height();

    InitPinPoint(uiTabGrainline->comboBoxGrainlineCenterPin);
    InitPinPoint(uiTabGrainline->comboBoxGrainlineTopPin);
    InitPinPoint(uiTabGrainline->comboBoxGrainlineBottomPin);

    connect(uiTabGrainline->comboBoxGrainlineTopPin, &QComboBox::currentTextChanged, this,
            &DialogSeamAllowance::GrainlinePinPointChanged);
    connect(uiTabGrainline->comboBoxGrainlineBottomPin, &QComboBox::currentTextChanged, this,
            &DialogSeamAllowance::GrainlinePinPointChanged);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetGrainlineAngle(QString angleFormula)
{
    if (angleFormula.isEmpty())
    {
        angleFormula = '0';
    }

    const QString formula = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        angleFormula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (formula.length() > 80)
    {
        this->DeployGrainlineRotation();
    }
    uiTabGrainline->lineEditRotFormula->setPlainText(formula);

    MoveCursorToEnd(uiTabGrainline->lineEditRotFormula);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::SetGrainlineLength(QString lengthFormula)
{
    if (lengthFormula.isEmpty())
    {
        lengthFormula = QString().setNum(UnitConvertor(10, Unit::Cm, *data.GetPatternUnit()));
    }

    const QString formula = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        lengthFormula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    // increase height if needed.
    if (formula.length() > 80)
    {
        this->DeployGrainlineLength();
    }

    uiTabGrainline->lineEditLenFormula->setPlainText(formula);

    MoveCursorToEnd(uiTabGrainline->lineEditLenFormula);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EnableGrainlineFormulaControls(bool enable)
{
    uiTabGrainline->pushButtonRot->setEnabled(enable);
    uiTabGrainline->lineEditRotFormula->setEnabled(enable);
    uiTabGrainline->pushButtonShowRot->setEnabled(enable);

    uiTabGrainline->pushButtonLen->setEnabled(enable);
    uiTabGrainline->lineEditLenFormula->setEnabled(enable);
    uiTabGrainline->pushButtonShowLen->setEnabled(enable);

    uiTabGrainline->comboBoxGrainlineCenterPin->setEnabled(enable);
}

