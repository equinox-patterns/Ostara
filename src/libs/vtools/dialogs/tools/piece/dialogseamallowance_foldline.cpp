/************************************************************************
 **
 **  @file   dialogseamallowance_foldline.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief  Fold line tab logic for DialogSeamAllowance.
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
#include "ui_tabfoldline.h"
#include "ui_tabpaths.h"
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vcommonsettings.h"
#include "../vpatterndb/vpiece.h"

#include <QFontDatabase>
//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitFold(const VPiece &piece)
{
    QLineF const mirrorLine = piece.SeamAllowanceMirrorLine(&data);
    uiTabPaths->checkBoxShowFullPiece->setEnabled(not mirrorLine.isNull());
    uiTabPaths->checkBoxShowFullPiece->setChecked(true);
    if (not mirrorLine.isNull())
    {
        uiTabPaths->checkBoxShowFullPiece->setChecked(piece.IsShowFullPiece());
    }

    uiTabPaths->checkBoxShowMirrorLine->setChecked(piece.IsShowMirrorLine());

    InitMirrorLine();

    {
        const int index = uiTabPaths->comboBoxMLStartPoint->findData(piece.GetMirrorLineStartPoint());
        if (index != -1)
        {
            uiTabPaths->comboBoxMLStartPoint->setCurrentIndex(index);
        }
    }

    {
        const int index = uiTabPaths->comboBoxMLEndPoint->findData(piece.GetMirrorLineEndPoint());
        if (index != -1)
        {
            uiTabPaths->comboBoxMLEndPoint->setCurrentIndex(index);
        }
    }

    {
        const QSignalBlocker blockerManualHeight(uiTabFoldLine->groupBoxManualHeight);
        const QSignalBlocker blockerManualWidth(uiTabFoldLine->groupBoxManualWidth);
        const QSignalBlocker blockerManualCenter(uiTabFoldLine->groupBoxManualCenter);

        InitFoldHeightFormula(piece);
        InitFoldWidthFormula(piece);
        InitFoldCenterFormula(piece);
    }

    {
        const int index = uiTabFoldLine->comboBoxType->findData(static_cast<unsigned int>(piece.GetFoldLineType()));
        if (index != -1)
        {
            uiTabFoldLine->comboBoxType->setCurrentIndex(index);
        }
    }

    uiTabFoldLine->lineEditLabel->setText(piece.GetFoldLineLabel());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitFoldHeightFormula(const VPiece &piece)
{
    if (piece.IsManualFoldHeight())
    {
        uiTabFoldLine->groupBoxManualHeight->setChecked(true);

        uiTabFoldLine->toolButtonExprHeight->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());
        uiTabFoldLine->plainTextEditHeight->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());
        uiTabFoldLine->pushButtonGrowHeight->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());
        uiTabFoldLine->labelEditHeight->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());
        uiTabFoldLine->label_3->setEnabled(uiTabFoldLine->groupBoxManualHeight->isChecked());

        QString foldHeight = piece.GetFormulaFoldHeight();
        foldHeight = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            foldHeight, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
        if (foldHeight.length() > 80) // increase height if needed.
        {
            this->DeployFoldHeight();
        }

        uiTabFoldLine->plainTextEditHeight->setPlainText(foldHeight.isEmpty() ? QChar('0') : foldHeight);
    }
    else
    {
        uiTabFoldLine->plainTextEditHeight->setPlainText(QChar('0'));
    }

    MoveCursorToEnd(uiTabFoldLine->plainTextEditHeight);
    ChangeColor(uiTabFoldLine->labelEditHeight, OkColor(this));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitFoldWidthFormula(const VPiece &piece)
{
    if (piece.IsManualFoldWidth())
    {
        uiTabFoldLine->groupBoxManualWidth->setChecked(true);

        uiTabFoldLine->toolButtonExprWidth->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());
        uiTabFoldLine->plainTextEditWidth->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());
        uiTabFoldLine->pushButtonGrowWidth->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());
        uiTabFoldLine->labelEditWidth->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());
        uiTabFoldLine->label_4->setEnabled(uiTabFoldLine->groupBoxManualWidth->isChecked());

        QString foldWidth = piece.GetFormulaFoldWidth();
        foldWidth = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            foldWidth, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
        if (foldWidth.length() > 80) // increase height if needed.
        {
            this->DeployPassmarkWidth();
        }

        uiTabFoldLine->plainTextEditWidth->setPlainText(foldWidth.isEmpty() ? QChar('0') : foldWidth);
    }
    else
    {
        uiTabFoldLine->plainTextEditWidth->setPlainText(QChar('0'));
    }

    MoveCursorToEnd(uiTabFoldLine->plainTextEditWidth);
    ChangeColor(uiTabFoldLine->labelEditWidth, OkColor(this));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitFoldCenterFormula(const VPiece &piece)
{
    const QString defCenterFormula = VTranslateVars::TryFormulaToUser(
        QString::number(0.5), VAbstractApplication::VApp()->Settings()->GetOsSeparator());

    if (piece.IsManualFoldCenter())
    {
        uiTabFoldLine->groupBoxManualCenter->setChecked(true);

        uiTabFoldLine->toolButtonExprCenter->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());
        uiTabFoldLine->plainTextEditCenter->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());
        uiTabFoldLine->pushButtonGrowCenter->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());
        uiTabFoldLine->labelEditCenter->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());
        uiTabFoldLine->label_5->setEnabled(uiTabFoldLine->groupBoxManualCenter->isChecked());

        QString foldCenter = piece.GetFormulaFoldCenter();
        foldCenter = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            foldCenter, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
        if (foldCenter.length() > 80) // increase height if needed.
        {
            this->DeployPassmarkAngle();
        }

        uiTabFoldLine->plainTextEditCenter->setPlainText(foldCenter.isEmpty() ? defCenterFormula : foldCenter);
    }
    else
    {

        uiTabFoldLine->plainTextEditCenter->setPlainText(defCenterFormula);
    }

    MoveCursorToEnd(uiTabFoldLine->plainTextEditCenter);
    ChangeColor(uiTabFoldLine->labelEditCenter, OkColor(this));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitFoldLineTab()
{
    InitFoldLineType();

    // Height formula
    this->m_formulaBaseFoldHeight = uiTabFoldLine->plainTextEditHeight->height();
    uiTabFoldLine->plainTextEditHeight->installEventFilter(this);
    m_timerFoldHeight->setSingleShot(true);

    connect(m_timerFoldHeight, &QTimer::timeout, this, &DialogSeamAllowance::EvalFoldHeight);
    connect(uiTabFoldLine->groupBoxManualHeight, &QGroupBox::toggled, this,
            &DialogSeamAllowance::EnabledManualFoldHeight);
    connect(uiTabFoldLine->toolButtonExprHeight, &QPushButton::clicked, this, &DialogSeamAllowance::FXFoldHeight);
    connect(uiTabFoldLine->plainTextEditHeight, &QPlainTextEdit::textChanged, this,
            [this]() { m_timerFoldHeight->start(formulaTimerTimeout); });
    connect(uiTabFoldLine->pushButtonGrowHeight, &QPushButton::clicked, this, &DialogSeamAllowance::DeployFoldHeight);

    // Width formula
    this->m_formulaBaseFoldWidth = uiTabFoldLine->plainTextEditWidth->height();
    uiTabFoldLine->plainTextEditWidth->installEventFilter(this);
    m_timerFoldWidth->setSingleShot(true);

    connect(m_timerFoldWidth, &QTimer::timeout, this, &DialogSeamAllowance::EvalFoldWidth);
    connect(uiTabFoldLine->groupBoxManualWidth, &QGroupBox::toggled, this,
            &DialogSeamAllowance::EnabledManualFoldWidth);
    connect(uiTabFoldLine->toolButtonExprWidth, &QPushButton::clicked, this, &DialogSeamAllowance::FXFoldWidth);
    connect(uiTabFoldLine->plainTextEditWidth, &QPlainTextEdit::textChanged, this,
            [this]() { m_timerFoldWidth->start(formulaTimerTimeout); });
    connect(uiTabFoldLine->pushButtonGrowWidth, &QPushButton::clicked, this, &DialogSeamAllowance::DeployFoldWidth);

    // Center formula
    this->m_formulaBaseFoldCenter = uiTabFoldLine->plainTextEditCenter->height();
    uiTabFoldLine->plainTextEditCenter->installEventFilter(this);
    m_timerFoldCenter->setSingleShot(true);

    connect(m_timerFoldCenter, &QTimer::timeout, this, &DialogSeamAllowance::EvalFoldCenter);
    connect(uiTabFoldLine->groupBoxManualCenter, &QGroupBox::toggled, this,
            &DialogSeamAllowance::EnabledManualFoldCenter);
    connect(uiTabFoldLine->toolButtonExprCenter, &QPushButton::clicked, this, &DialogSeamAllowance::FXFoldCenter);
    connect(uiTabFoldLine->plainTextEditCenter, &QPlainTextEdit::textChanged, this,
            [this]() { m_timerFoldCenter->start(formulaTimerTimeout); });
    connect(uiTabFoldLine->pushButtonGrowCenter, &QPushButton::clicked, this, &DialogSeamAllowance::DeployFoldCenter);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitFoldLineType()
{
    uiTabFoldLine->comboBoxType->clear();

    uiTabFoldLine->comboBoxType->addItem(tr("Two arrows (text above)"),
                                         static_cast<unsigned int>(FoldLineType::TwoArrowsTextAbove));
    uiTabFoldLine->comboBoxType->addItem(tr("Two arrows"), static_cast<unsigned int>(FoldLineType::TwoArrows));
    uiTabFoldLine->comboBoxType->addItem(tr("None"), static_cast<unsigned int>(FoldLineType::None));
}

