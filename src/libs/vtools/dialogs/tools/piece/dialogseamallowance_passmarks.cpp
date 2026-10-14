/************************************************************************
 **
 **  @file   dialogseamallowance_passmarks.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   3 11, 2016
 **
 **  @brief  Passmarks tab logic for DialogSeamAllowance.
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
#include "ui_tabpassmarks.h"
#include "ui_tabpaths.h"
#include "../vmisc/def.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vmisc/vabstractvalapplication.h"
//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::PassmarkChanged(int index)
{
    uiTabPassmarks->groupBoxMarkType->setDisabled(true);
    uiTabPassmarks->groupBoxAngleType->setDisabled(true);
    uiTabPassmarks->groupBoxManualLength->setDisabled(true);
    uiTabPassmarks->groupBoxManualWidth->setDisabled(true);
    uiTabPassmarks->groupBoxManualAngle->setDisabled(true);

    uiTabPassmarks->labelEditPassmarkLength->setDisabled(true);
    uiTabPassmarks->labelEditPassmarkWidth->setDisabled(true);
    uiTabPassmarks->labelEditPassmarkAngle->setDisabled(true);

    uiTabPassmarks->labelEditPassmarkVisible->setDisabled(true);
    uiTabPassmarks->toolButtonExprPassmarkVisible->setDisabled(true);
    uiTabPassmarks->labelEqual_6->setDisabled(true);
    uiTabPassmarks->labelResultPassmarkVisible->setDisabled(true);
    uiTabPassmarks->plainTextEditPassmarkVisible->setDisabled(true);
    uiTabPassmarks->pushButtonGrowPassmarkVisible->setDisabled(true);

    uiTabPassmarks->checkBoxClockwiseOpening->setDisabled(true);
    uiTabPassmarks->checkBoxNotMirrored->setDisabled(true);
    uiTabPassmarks->checkBoxShowSecondPassmark->setDisabled(true);

    const QSignalBlocker blockerNotMirrored(uiTabPassmarks->checkBoxNotMirrored);
    const QSignalBlocker blockerClockwiseOpening(uiTabPassmarks->checkBoxClockwiseOpening);
    const QSignalBlocker blockerShowSecondPassmark(uiTabPassmarks->checkBoxShowSecondPassmark);
    const QSignalBlocker blockerManualLength(uiTabPassmarks->groupBoxManualLength);
    const QSignalBlocker blockerManualWidth(uiTabPassmarks->groupBoxManualWidth);
    const QSignalBlocker blockerManualAngle(uiTabPassmarks->groupBoxManualAngle);
    const QSignalBlocker blockerMarkType(uiTabPassmarks->groupBoxMarkType);
    const QSignalBlocker blockerAngleType(uiTabPassmarks->groupBoxAngleType);

    uiTabPassmarks->checkBoxClockwiseOpening->setChecked(false);
    uiTabPassmarks->checkBoxNotMirrored->setChecked(false);

    uiTabPassmarks->groupBoxManualLength->setChecked(false);
    uiTabPassmarks->groupBoxManualWidth->setChecked(false);
    uiTabPassmarks->groupBoxManualAngle->setChecked(false);

    if (index == -1)
    {
        return;
    }

    const VPiece piece = CreatePiece();
    const int nodeIndex = piece.GetPath().indexOfNode(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt());
    if (nodeIndex == -1)
    {
        return;
    }

    const VPiecePath &path = piece.GetPath();
    const VPieceNode &node = path.at(nodeIndex);

    InitPassmarkLengthFormula(node);
    InitPassmarkWidthFormula(node);
    InitPassmarkAngleFormula(node);
    InitPassmarkVisibilityFormula(node);
    InitPassmarkShapeType(node);
    InitPassmarkAngleType(node);

    uiTabPassmarks->checkBoxNotMirrored->setEnabled(true);
    uiTabPassmarks->checkBoxNotMirrored->setChecked(node.IsPassmarkNotMirrored());

    if (node.GetPassmarkLineType() == PassmarkLineType::CheckMark)
    {
        uiTabPassmarks->checkBoxClockwiseOpening->setEnabled(true);
        uiTabPassmarks->checkBoxClockwiseOpening->setChecked(node.IsPassmarkClockwiseOpening());
    }

    // Show the second option
    uiTabPassmarks->checkBoxShowSecondPassmark->setEnabled(true);
    uiTabPassmarks->checkBoxShowSecondPassmark->setChecked(node.IsShowSecondPassmark());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::PassmarkLineTypeChanged(int id)
{
    const int i = uiTabPassmarks->comboBoxPassmarks->currentIndex();
    if (i != -1)
    {
        QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt());
        if (rowItem)
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

            PassmarkLineType lineType = PassmarkLineType::OneLine;
            if (id == uiTabPassmarks->buttonGroupLineType->id(uiTabPassmarks->radioButtonOneLine))
            {
                lineType = PassmarkLineType::OneLine;
            }
            else if (id == uiTabPassmarks->buttonGroupLineType->id(uiTabPassmarks->radioButtonTwoLines))
            {
                lineType = PassmarkLineType::TwoLines;
            }
            else if (id == uiTabPassmarks->buttonGroupLineType->id(uiTabPassmarks->radioButtonThreeLines))
            {
                lineType = PassmarkLineType::ThreeLines;
            }
            else if (id == uiTabPassmarks->buttonGroupLineType->id(uiTabPassmarks->radioButtonTMark))
            {
                lineType = PassmarkLineType::TMark;
            }
            else if (id == uiTabPassmarks->buttonGroupLineType->id(uiTabPassmarks->radioButtonVMark))
            {
                lineType = PassmarkLineType::ExternalVMark;
            }
            else if (id == uiTabPassmarks->buttonGroupLineType->id(uiTabPassmarks->radioButtonVMark2))
            {
                lineType = PassmarkLineType::InternalVMark;
            }
            else if (id == uiTabPassmarks->buttonGroupLineType->id(uiTabPassmarks->radioButtonUMark))
            {
                lineType = PassmarkLineType::UMark;
            }
            else if (id == uiTabPassmarks->buttonGroupLineType->id(uiTabPassmarks->radioButtonBoxMark))
            {
                lineType = PassmarkLineType::BoxMark;
            }
            else if (id == uiTabPassmarks->buttonGroupLineType->id(uiTabPassmarks->radioButtonCheckMark))
            {
                lineType = PassmarkLineType::CheckMark;
            }

            rowNode.SetPassmarkLineType(lineType);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));
            rowItem->setText(GetNodeName(&data, rowNode, true));

            ListChanged();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::PassmarkAngleTypeChanged(int id)
{
    const int i = uiTabPassmarks->comboBoxPassmarks->currentIndex();
    if (i != -1)
    {
        QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt());
        if (rowItem)
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));

            PassmarkAngleType angleType = PassmarkAngleType::Straightforward;
            if (id == uiTabPassmarks->buttonGroupAngleType->id(uiTabPassmarks->radioButtonStraightforward))
            {
                angleType = PassmarkAngleType::Straightforward;
            }
            else if (id == uiTabPassmarks->buttonGroupAngleType->id(uiTabPassmarks->radioButtonBisector))
            {
                angleType = PassmarkAngleType::Bisector;
            }
            else if (id == uiTabPassmarks->buttonGroupAngleType->id(uiTabPassmarks->radioButtonIntersection))
            {
                angleType = PassmarkAngleType::Intersection;
            }
            else if (id == uiTabPassmarks->buttonGroupAngleType->id(uiTabPassmarks->radioButtonIntersectionOnlyLeft))
            {
                angleType = PassmarkAngleType::IntersectionOnlyLeft;
            }
            else if (id == uiTabPassmarks->buttonGroupAngleType->id(uiTabPassmarks->radioButtonIntersectionOnlyRight))
            {
                angleType = PassmarkAngleType::IntersectionOnlyRight;
            }
            else if (id == uiTabPassmarks->buttonGroupAngleType->id(uiTabPassmarks->radioButtonIntersection2))
            {
                angleType = PassmarkAngleType::Intersection2;
            }
            else if (id == uiTabPassmarks->buttonGroupAngleType->id(uiTabPassmarks->radioButtonIntersection2OnlyLeft))
            {
                angleType = PassmarkAngleType::Intersection2OnlyLeft;
            }
            else if (id == uiTabPassmarks->buttonGroupAngleType->id(uiTabPassmarks->radioButtonIntersection2OnlyRight))
            {
                angleType = PassmarkAngleType::Intersection2OnlyRight;
            }

            rowNode.SetPassmarkAngleType(angleType);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));

            ListChanged();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::PassmarkShowSecondChanged(int state)
{
    const int i = uiTabPassmarks->comboBoxPassmarks->currentIndex();
    if (i != -1)
    {
        QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt());
        if (rowItem)
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetShowSecondPassmark(state);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));

            ListChanged();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::PassmarkClockwiseOrientationChanged(int state)
{
    const int i = uiTabPassmarks->comboBoxPassmarks->currentIndex();
    if (i != -1)
    {
        QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt());
        if (rowItem)
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetPassmarkClockwiseOpening(state);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));

            ListChanged();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::PassmarkNotMirroredStateChanged(int state)
{
    const int i = uiTabPassmarks->comboBoxPassmarks->currentIndex();
    if (i != -1)
    {
        QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt());
        if (rowItem)
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetPassmarkNotMirrored(state);
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));

            ListChanged();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EnabledManualPassmarkLength()
{
    const int index = uiTabPassmarks->comboBoxPassmarks->currentIndex();
    if (index != -1)
    {
        QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt());
        if (rowItem)
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetManualPassmarkLength(uiTabPassmarks->groupBoxManualLength->isChecked());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));

            uiTabPassmarks->toolButtonExprLength->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());
            uiTabPassmarks->plainTextEditPassmarkLength->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());
            uiTabPassmarks->pushButtonGrowPassmarkLength->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());
            uiTabPassmarks->labelEditPassmarkLength->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());
            uiTabPassmarks->label_3->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());

            EvalPassmarkLength();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EnabledManualPassmarkWidth()
{
    const int index = uiTabPassmarks->comboBoxPassmarks->currentIndex();
    if (index != -1)
    {
        QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt());
        if (rowItem)
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetManualPassmarkWidth(uiTabPassmarks->groupBoxManualWidth->isChecked());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));

            uiTabPassmarks->toolButtonExprWidth->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());
            uiTabPassmarks->plainTextEditPassmarkWidth->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());
            uiTabPassmarks->pushButtonGrowPassmarkWidth->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());
            uiTabPassmarks->labelEditPassmarkWidth->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());
            uiTabPassmarks->label_4->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());

            EvalPassmarkWidth();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::EnabledManualPassmarkAngle()
{
    const int index = uiTabPassmarks->comboBoxPassmarks->currentIndex();
    if (index != -1)
    {
        QListWidgetItem *rowItem = GetItemById(uiTabPassmarks->comboBoxPassmarks->currentData().toUInt());
        if (rowItem)
        {
            auto rowNode = qvariant_cast<VPieceNode>(rowItem->data(Qt::UserRole));
            rowNode.SetManualPassmarkAngle(uiTabPassmarks->groupBoxManualAngle->isChecked());
            rowItem->setData(Qt::UserRole, QVariant::fromValue(rowNode));

            uiTabPassmarks->toolButtonExprAngle->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());
            uiTabPassmarks->plainTextEditPassmarkAngle->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());
            uiTabPassmarks->pushButtonGrowPassmarkAngle->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());
            uiTabPassmarks->labelEditPassmarkAngle->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());
            uiTabPassmarks->label_5->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());

            EvalPassmarkAngle();
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPassmarksList()
{
    const quint32 id = uiTabPassmarks->comboBoxPassmarks->currentData().toUInt();

    {
        const QSignalBlocker blocker(uiTabPassmarks->comboBoxPassmarks);
        uiTabPassmarks->comboBoxPassmarks->clear();

        const QVector<VPieceNode> nodes = GetListInternals<VPieceNode>(uiTabPaths->listWidgetMainPath);

        for (const auto &node : nodes)
        {
            if (node.GetTypeTool() == Tool::NodePoint && node.IsPassmark())
            {
                const QString name = GetNodeName(&data, node);

                uiTabPassmarks->comboBoxPassmarks->addItem(name, node.GetId());
            }
        }
    }

    const int index = uiTabPassmarks->comboBoxPassmarks->findData(id);
    if (index != -1)
    {
        uiTabPassmarks->comboBoxPassmarks->setCurrentIndex(index);
        PassmarkChanged(index); // Need in case combox index was not changed
    }
    else
    {
        uiTabPassmarks->comboBoxPassmarks->count() > 0 ? PassmarkChanged(0) : PassmarkChanged(-1);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPassmarksTab()
{
    // Length formula
    this->m_formulaBasePassmarkLength = uiTabPassmarks->plainTextEditPassmarkLength->height();
    uiTabPassmarks->plainTextEditPassmarkLength->installEventFilter(this);
    m_timerPassmarkLength->setSingleShot(true);

    connect(m_timerPassmarkLength, &QTimer::timeout, this, &DialogSeamAllowance::EvalPassmarkLength);
    connect(uiTabPassmarks->groupBoxManualLength, &QGroupBox::toggled, this,
            &DialogSeamAllowance::EnabledManualPassmarkLength);
    connect(uiTabPassmarks->toolButtonExprLength, &QPushButton::clicked, this, &DialogSeamAllowance::FXPassmarkLength);
    connect(uiTabPassmarks->plainTextEditPassmarkLength, &QPlainTextEdit::textChanged, this,
            [this]() -> void { m_timerPassmarkLength->start(formulaTimerTimeout); });
    connect(uiTabPassmarks->pushButtonGrowPassmarkLength, &QPushButton::clicked, this,
            &DialogSeamAllowance::DeployPassmarkLength);

    // Width formula
    this->m_formulaBasePassmarkWidth = uiTabPassmarks->plainTextEditPassmarkWidth->height();
    uiTabPassmarks->plainTextEditPassmarkWidth->installEventFilter(this);
    m_timerPassmarkWidth->setSingleShot(true);

    connect(m_timerPassmarkWidth, &QTimer::timeout, this, &DialogSeamAllowance::EvalPassmarkWidth);
    connect(uiTabPassmarks->groupBoxManualWidth, &QGroupBox::toggled, this,
            &DialogSeamAllowance::EnabledManualPassmarkWidth);
    connect(uiTabPassmarks->toolButtonExprWidth, &QPushButton::clicked, this, &DialogSeamAllowance::FXPassmarkWidth);
    connect(uiTabPassmarks->plainTextEditPassmarkWidth, &QPlainTextEdit::textChanged, this,
            [this]() -> void { m_timerPassmarkWidth->start(formulaTimerTimeout); });
    connect(uiTabPassmarks->pushButtonGrowPassmarkWidth, &QPushButton::clicked, this,
            &DialogSeamAllowance::DeployPassmarkWidth);

    // Angle formula
    this->m_formulaBasePassmarkAngle = uiTabPassmarks->plainTextEditPassmarkAngle->height();
    uiTabPassmarks->plainTextEditPassmarkAngle->installEventFilter(this);
    m_timerPassmarkAngle->setSingleShot(true);

    connect(m_timerPassmarkAngle, &QTimer::timeout, this, &DialogSeamAllowance::EvalPassmarkAngle);
    connect(uiTabPassmarks->groupBoxManualAngle, &QGroupBox::toggled, this,
            &DialogSeamAllowance::EnabledManualPassmarkAngle);
    connect(uiTabPassmarks->toolButtonExprAngle, &QPushButton::clicked, this, &DialogSeamAllowance::FXPassmarkAngle);
    connect(uiTabPassmarks->plainTextEditPassmarkAngle, &QPlainTextEdit::textChanged, this,
            [this]() -> void { m_timerPassmarkAngle->start(formulaTimerTimeout); });
    connect(uiTabPassmarks->pushButtonGrowPassmarkAngle, &QPushButton::clicked, this,
            &DialogSeamAllowance::DeployPassmarkAngle);

    // notch list
    InitPassmarksList();
    connect(uiTabPassmarks->comboBoxPassmarks, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &DialogSeamAllowance::PassmarkChanged);

    connect(uiTabPassmarks->buttonGroupLineType, &QButtonGroup::idClicked, this,
            &DialogSeamAllowance::PassmarkLineTypeChanged);
    connect(uiTabPassmarks->buttonGroupAngleType, &QButtonGroup::idClicked, this,
            &DialogSeamAllowance::PassmarkAngleTypeChanged);
    connect(uiTabPassmarks->checkBoxShowSecondPassmark, CHECKBOX_STATE_CHANGED, this,
            &DialogSeamAllowance::PassmarkShowSecondChanged);
    connect(uiTabPassmarks->checkBoxClockwiseOpening, CHECKBOX_STATE_CHANGED, this,
            &DialogSeamAllowance::PassmarkClockwiseOrientationChanged);
    connect(uiTabPassmarks->checkBoxNotMirrored, CHECKBOX_STATE_CHANGED, this,
            &DialogSeamAllowance::PassmarkNotMirroredStateChanged);

    // Visibility formula
    this->m_formulaBasePassmarkVisible = uiTabPassmarks->plainTextEditPassmarkVisible->height();
    uiTabPassmarks->plainTextEditPassmarkVisible->installEventFilter(this);
    m_timerPassmarkVisible->setSingleShot(true);

    connect(m_timerPassmarkVisible, &QTimer::timeout, this, &DialogSeamAllowance::EvalPassmarkVisible);
    connect(uiTabPassmarks->toolButtonExprPassmarkVisible, &QPushButton::clicked, this,
            &DialogSeamAllowance::FXPassmarkVisible);
    connect(uiTabPassmarks->plainTextEditPassmarkVisible, &QPlainTextEdit::textChanged, this,
            [this]() -> void { m_timerPassmarkVisible->start(formulaTimerTimeout); });
    connect(uiTabPassmarks->pushButtonGrowPassmarkVisible, &QPushButton::clicked, this,
            &DialogSeamAllowance::DeployPassmarkVisible);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPassmarkLengthFormula(const VPieceNode &node)
{
    // notch depth
    uiTabPassmarks->groupBoxManualLength->setEnabled(true);

    if (node.IsManualPassmarkLength())
    {
        uiTabPassmarks->groupBoxManualLength->setChecked(true);

        uiTabPassmarks->toolButtonExprLength->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());
        uiTabPassmarks->plainTextEditPassmarkLength->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());
        uiTabPassmarks->pushButtonGrowPassmarkLength->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());
        uiTabPassmarks->labelEditPassmarkLength->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());
        uiTabPassmarks->label_3->setEnabled(uiTabPassmarks->groupBoxManualLength->isChecked());

        QString passmarkLength = node.GetFormulaPassmarkLength();
        passmarkLength = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
            passmarkLength, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
        if (passmarkLength.length() > 80) // increase height if needed.
        {
            this->DeployPassmarkLength();
        }

        if (passmarkLength.isEmpty())
        {
            qreal const length = UnitConvertor(1, Unit::Cm, VAbstractValApplication::VApp()->patternUnits());
            uiTabPassmarks->plainTextEditPassmarkLength->setPlainText(
                VAbstractApplication::VApp()->LocaleToString(length));
        }
        else
        {
            uiTabPassmarks->plainTextEditPassmarkLength->setPlainText(passmarkLength);
        }
    }
    else
    {
        qreal const length = UnitConvertor(1, Unit::Cm, VAbstractValApplication::VApp()->patternUnits());
        uiTabPassmarks->plainTextEditPassmarkLength->setPlainText(VAbstractApplication::VApp()->LocaleToString(length));
    }

    MoveCursorToEnd(uiTabPassmarks->plainTextEditPassmarkLength);
    ChangeColor(uiTabPassmarks->labelEditPassmarkLength, OkColor(this));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPassmarkWidthFormula(const VPieceNode &node)
{
    // notch width
    if (node.GetPassmarkLineType() != PassmarkLineType::OneLine)
    {
        uiTabPassmarks->groupBoxManualWidth->setEnabled(true);

        if (node.IsManualPassmarkWidth())
        {
            uiTabPassmarks->groupBoxManualWidth->setChecked(true);

            uiTabPassmarks->toolButtonExprWidth->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());
            uiTabPassmarks->plainTextEditPassmarkWidth->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());
            uiTabPassmarks->pushButtonGrowPassmarkWidth->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());
            uiTabPassmarks->labelEditPassmarkWidth->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());
            uiTabPassmarks->label_4->setEnabled(uiTabPassmarks->groupBoxManualWidth->isChecked());

            QString passmarkWidth = node.GetFormulaPassmarkWidth();
            passmarkWidth = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
                passmarkWidth, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
            if (passmarkWidth.length() > 80) // increase height if needed.
            {
                this->DeployPassmarkWidth();
            }

            if (passmarkWidth.isEmpty())
            {
                qreal const width = UnitConvertor(0.85, Unit::Cm, VAbstractValApplication::VApp()->patternUnits());
                uiTabPassmarks->plainTextEditPassmarkWidth->setPlainText(
                    VAbstractApplication::VApp()->LocaleToString(width));
            }
            else
            {
                uiTabPassmarks->plainTextEditPassmarkWidth->setPlainText(passmarkWidth);
            }
        }
        else
        {
            qreal const length = UnitConvertor(0.85, Unit::Cm, VAbstractValApplication::VApp()->patternUnits());
            uiTabPassmarks->plainTextEditPassmarkWidth->setPlainText(
                VAbstractApplication::VApp()->LocaleToString(length));
        }

        MoveCursorToEnd(uiTabPassmarks->plainTextEditPassmarkWidth);
    }
    else
    {
        qreal const length = UnitConvertor(0.85, Unit::Cm, VAbstractValApplication::VApp()->patternUnits());
        uiTabPassmarks->plainTextEditPassmarkWidth->setPlainText(VAbstractApplication::VApp()->LocaleToString(length));
    }
    ChangeColor(uiTabPassmarks->labelEditPassmarkWidth, OkColor(this));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPassmarkAngleFormula(const VPieceNode &node)
{
    // notch angle
    if (node.GetPassmarkAngleType() == PassmarkAngleType::Straightforward)
    {
        uiTabPassmarks->groupBoxManualAngle->setEnabled(true);

        if (node.IsManualPassmarkAngle())
        {
            uiTabPassmarks->groupBoxManualAngle->setChecked(true);

            uiTabPassmarks->toolButtonExprAngle->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());
            uiTabPassmarks->plainTextEditPassmarkAngle->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());
            uiTabPassmarks->pushButtonGrowPassmarkAngle->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());
            uiTabPassmarks->labelEditPassmarkAngle->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());
            uiTabPassmarks->label_5->setEnabled(uiTabPassmarks->groupBoxManualAngle->isChecked());

            QString passmarkAngle = node.GetFormulaPassmarkAngle();
            passmarkAngle = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
                passmarkAngle, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
            if (passmarkAngle.length() > 80) // increase height if needed.
            {
                this->DeployPassmarkAngle();
            }

            uiTabPassmarks->plainTextEditPassmarkAngle->setPlainText(passmarkAngle.isEmpty() ? QString::number(0)
                                                                                             : passmarkAngle);
        }
        else
        {
            uiTabPassmarks->plainTextEditPassmarkAngle->setPlainText(QString::number(0));
        }

        MoveCursorToEnd(uiTabPassmarks->plainTextEditPassmarkAngle);
    }
    else
    {
        uiTabPassmarks->plainTextEditPassmarkAngle->setPlainText(QString::number(0));
    }
    ChangeColor(uiTabPassmarks->labelEditPassmarkAngle, OkColor(this));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPassmarkVisibilityFormula(const VPieceNode& node)
{
    uiTabPassmarks->labelEditPassmarkVisible->setEnabled(true);
    uiTabPassmarks->toolButtonExprPassmarkVisible->setEnabled(true);
    uiTabPassmarks->labelEqual_6->setEnabled(true);
    uiTabPassmarks->labelResultPassmarkVisible->setEnabled(true);
    uiTabPassmarks->plainTextEditPassmarkVisible->setEnabled(true);
    uiTabPassmarks->pushButtonGrowPassmarkVisible->setEnabled(true);

    QString passmarkVisibility = node.GetFormulaPassmarkVisibility();
    passmarkVisibility = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        passmarkVisibility, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    if (passmarkVisibility.length() > 80) // increase height if needed.
    {
        this->DeployPassmarkVisible();
    }

    uiTabPassmarks->plainTextEditPassmarkVisible->setPlainText(passmarkVisibility.isEmpty() ? QString::number(1)
                                                                                            : passmarkVisibility);

    MoveCursorToEnd(uiTabPassmarks->plainTextEditPassmarkVisible);

    ChangeColor(uiTabPassmarks->labelEditPassmarkVisible, OkColor(this));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPassmarkShapeType(const VPieceNode &node)
{
    // Line type
    uiTabPassmarks->groupBoxMarkType->setEnabled(true);

    switch (node.GetPassmarkLineType())
    {
        case PassmarkLineType::OneLine:
            uiTabPassmarks->radioButtonOneLine->setChecked(true);
            break;
        case PassmarkLineType::TwoLines:
            uiTabPassmarks->radioButtonTwoLines->setChecked(true);
            break;
        case PassmarkLineType::ThreeLines:
            uiTabPassmarks->radioButtonThreeLines->setChecked(true);
            break;
        case PassmarkLineType::TMark:
            uiTabPassmarks->radioButtonTMark->setChecked(true);
            break;
        case PassmarkLineType::ExternalVMark:
            uiTabPassmarks->radioButtonVMark->setChecked(true);
            break;
        case PassmarkLineType::InternalVMark:
            uiTabPassmarks->radioButtonVMark2->setChecked(true);
            break;
        case PassmarkLineType::UMark:
            uiTabPassmarks->radioButtonUMark->setChecked(true);
            break;
        case PassmarkLineType::BoxMark:
            uiTabPassmarks->radioButtonBoxMark->setChecked(true);
            break;
        case PassmarkLineType::CheckMark:
            uiTabPassmarks->radioButtonCheckMark->setChecked(true);
            break;
        default:
            break;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogSeamAllowance::InitPassmarkAngleType(const VPieceNode &node)
{
    // Angle type
    uiTabPassmarks->groupBoxAngleType->setEnabled(true);

    switch (node.GetPassmarkAngleType())
    {
        case PassmarkAngleType::Straightforward:
            uiTabPassmarks->radioButtonStraightforward->setChecked(true);
            break;
        case PassmarkAngleType::Bisector:
            uiTabPassmarks->radioButtonBisector->setChecked(true);
            break;
        case PassmarkAngleType::Intersection:
            uiTabPassmarks->radioButtonIntersection->setChecked(true);
            break;
        case PassmarkAngleType::IntersectionOnlyLeft:
            uiTabPassmarks->radioButtonIntersectionOnlyLeft->setChecked(true);
            break;
        case PassmarkAngleType::IntersectionOnlyRight:
            uiTabPassmarks->radioButtonIntersectionOnlyRight->setChecked(true);
            break;
        case PassmarkAngleType::Intersection2:
            uiTabPassmarks->radioButtonIntersection2->setChecked(true);
            break;
        case PassmarkAngleType::Intersection2OnlyLeft:
            uiTabPassmarks->radioButtonIntersection2OnlyLeft->setChecked(true);
            break;
        case PassmarkAngleType::Intersection2OnlyRight:
            uiTabPassmarks->radioButtonIntersection2OnlyRight->setChecked(true);
            break;
        default:
            break;
    }
}

