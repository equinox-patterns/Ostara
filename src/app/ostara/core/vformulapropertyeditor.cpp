/************************************************************************
 **
 **  @file   vformulapropertyeditor.cpp
 **  @author Roman Telezhynskyi <dismine(at)gmail.com>
 **  @date   28 8, 2014
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

#include "vformulapropertyeditor.h"

#include <QApplication>
#include <QColorDialog>
#include <QDebug>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpacerItem>
#include <QVBoxLayout>

#include "vtheme.h"
#include "../vmisc/literals.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpropertyexplorer/vproperty.h"
#include "../vtools/dialogs/support/dialogeditwrongformula.h"

// VFormulaPropertyEditor
//---------------------------------------------------------------------------------------------------------------------
VFormulaPropertyEditor::VFormulaPropertyEditor(QWidget *parent)
  : QWidget(parent)
{
    // Create the tool button
    m_ToolButton = new QToolButton(this);
    m_ToolButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_ToolButton->setText(QStringLiteral("..."));
    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    m_ToolButton->setIcon(
        QIcon(QStringLiteral(":/icons/%1/function_%1.svg")
                  .arg(isDark ? QStringLiteral("dark") : QStringLiteral("light"))));
    m_ToolButton->setIconSize(QSize(16, 16));
    m_ToolButton->setFixedSize(24, 24);
    m_ToolButton->installEventFilter(this);
    setFocusProxy(m_ToolButton); // Make the ToolButton the focus proxy
    setFocusPolicy(m_ToolButton->focusPolicy());
    connect(m_ToolButton, &QToolButton::clicked, this, &VFormulaPropertyEditor::onToolButtonClicked);

    // Create the text label
    m_TextLabel = new QLabel(this);
    m_TextLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_TextLabel->setText(m_formula.getStringValue());

    // Value + fx button row
    auto *valueLayout = new QHBoxLayout;
    valueLayout->setSpacing(3);
    valueLayout->setContentsMargins(0, 0, 0, 0);
    valueLayout->addWidget(m_TextLabel);
    // Spacer (this is needed for proper display of the label and button)
    valueLayout->addSpacerItem(new QSpacerItem(1000000000, 0, QSizePolicy::Expanding, QSizePolicy::Preferred));
    valueLayout->addWidget(m_ToolButton);

    // Preset angle buttons row (0/90/180/270°), shown only for angle fields.
    m_presetRow = new QWidget(this);
    auto *presetLayout = new QHBoxLayout(m_presetRow);
    presetLayout->setContentsMargins(0, 0, 0, 0);
    presetLayout->setSpacing(4);
    constexpr int kPresetAngles[] = {0, 90, 180, 270};
    constexpr const char *kPresetArrows[] = {"right", "up", "left", "down"};
    for (int i = 0; i < 4; ++i)
    {
        const int angle = kPresetAngles[i];
        auto *button = new QPushButton(QString::number(angle) + degreeSymbol, m_presetRow);
        button->setObjectName(QStringLiteral("anglePresetBtn") + QString::number(angle));
        button->setIcon(QIcon(QStringLiteral(":/icons/%1/arrow-%2_%1.svg")
                                  .arg(isDark ? QStringLiteral("dark") : QStringLiteral("light"),
                                       QString::fromLatin1(kPresetArrows[i]))));
        button->setLayoutDirection(Qt::RightToLeft);
        button->setCursor(Qt::ArrowCursor);
        connect(button, &QPushButton::clicked, this, [this, angle]() {
            m_formula.SetFormula(QString::number(angle), FormulaType::FromUser);
            m_formula.Eval();
            m_TextLabel->setText(m_formula.getStringValue());
            emit dataChangedByUser(m_formula, this);
            QCoreApplication::postEvent(this, new VPE::UserChangeEvent());
        });
        presetLayout->addWidget(button);
    }
    m_presetRow->setVisible(false);

    // The editor lays out only the value + fx row. The preset row is kept as a child but
    // rendered by the form view below the "Formula" editor (VProperty::bottomWidget()).
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(2);
    mainLayout->addLayout(valueLayout);
}

//---------------------------------------------------------------------------------------------------------------------
void VFormulaPropertyEditor::SetFormula(const VFormula &formula)
{
    if (this->m_formula != formula)
    {
        this->m_formula = formula;
        m_TextLabel->setText(this->m_formula.getStringValue());
        m_presetRow->setVisible(this->m_formula.getPostfix() == degreeSymbol);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void VFormulaPropertyEditor::onToolButtonClicked()
{
    QScopedPointer<DialogEditWrongFormula> const tmpWidget(new DialogEditWrongFormula(
        m_formula.getData(), m_formula.getToolId(), VAbstractValApplication::VApp()->getMainWindow()));
    tmpWidget->setCheckZero(m_formula.getCheckZero());
    tmpWidget->setPostfix(m_formula.getPostfix());
    tmpWidget->SetFormula(m_formula.GetFormula(FormulaType::FromUser));

    if (tmpWidget->exec() == QDialog::Accepted)
    {
        m_formula.SetFormula(tmpWidget->GetFormula(), FormulaType::ToUser);
        m_formula.Eval();
        m_TextLabel->setText(m_formula.getStringValue());
        emit dataChangedByUser(m_formula, this);
        QCoreApplication::postEvent(this, new VPE::UserChangeEvent());
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto VFormulaPropertyEditor::eventFilter(QObject *obj, QEvent *ev) -> bool
{
    if (obj == m_ToolButton && ev->type() == QEvent::KeyPress)
    {
        // Ignore the event, so that eventually the delegate gets the event.
        ev->ignore();
        return true;
    }

    return QWidget::eventFilter(obj, ev);
}

//---------------------------------------------------------------------------------------------------------------------
auto VFormulaPropertyEditor::GetFormula() const -> VFormula
{
    return m_formula;
}

//---------------------------------------------------------------------------------------------------------------------
auto VFormulaPropertyEditor::presetRow() const -> QWidget *
{
    return m_presetRow;
}
