/************************************************************************
 **
 **  @file   formulainputwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   30/07/2026
 **
 **  @brief  Reusable formula input widget for tool creation/editing dialogs.
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
#include "formulainputwidget.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "../dialogtoolbox.h"
#include "../support/dialogeditwrongformula.h"
#include "vpatterndb/vcontainer.h"
#include "vpatterndb/vtranslatevars.h"
#include "vmisc/vabstractapplication.h"
#include "vmisc/vcommonsettings.h"
#include "vwidgets/vplaintextedit.h"
#include "themeDef.h"
#include "vtheme.h"

//---------------------------------------------------------------------------------------------------------------------
FormulaInputWidget::FormulaInputWidget(QWidget *parent)
  : QWidget(parent)
  , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    buildUi();

    connect(m_timer, &QTimer::timeout, this, &FormulaInputWidget::evaluate);
    connect(m_fxButton, &QPushButton::clicked, this, &FormulaInputWidget::openFxDialog);
    connect(m_growButton, &QPushButton::clicked, this, &FormulaInputWidget::deploy);
    connect(m_textEdit, &QPlainTextEdit::textChanged, this, &FormulaInputWidget::onTextChanged);

    m_textEdit->installEventFilter(this);
}

//---------------------------------------------------------------------------------------------------------------------
FormulaInputWidget::~FormulaInputWidget()
{
    m_textEdit->blockSignals(true);
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::buildUi()
{
    // --- Create widgets ---
    m_label = new QLabel(tr("Formula:"), this);

    m_fxButton = new QToolButton(this);
    m_fxButton->setFixedSize(24, 24);
    m_fxButton->setIcon(VTheme::GetIconResource(QStringLiteral("icon"), QStringLiteral("24x24/fx.png")));

    auto *equalsLabel = new QLabel(this);
    equalsLabel->setPixmap(
        VTheme::GetPixmapResource(QStringLiteral("icon"), QStringLiteral("24x24/equal.png")));

    m_resultLabel = new QLabel(QStringLiteral("_"), this);
    m_resultLabel->setMinimumWidth(60);
    m_resultLabel->setToolTip(tr("Value"));

    m_textEdit = new VPlainTextEdit(this);
    m_textEdit->setMaximumHeight(28);
    m_baseHeight = m_textEdit->height();

    m_growButton = new QPushButton(this);
    m_growButton->setFixedSize(16, 16);
    m_growButton->setFlat(true);
    m_growButton->setIcon(FromTheme(VThemeIcon::GoDown));

    // --- Layout ---
    auto *row1 = new QHBoxLayout;
    row1->addWidget(m_label);
    row1->addStretch();
    row1->addWidget(m_fxButton);
    row1->addWidget(equalsLabel);
    row1->addWidget(m_resultLabel);

    auto *row2 = new QHBoxLayout;
    row2->addWidget(m_textEdit);
    row2->addWidget(m_growButton);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(row1);
    mainLayout->addLayout(row2);
    setLayout(mainLayout);
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::setData(const VContainer *data)
{
    m_data = data;
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::setToolId(quint32 id)
{
    m_toolId = id;
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::setPostfix(const QString &postfix)
{
    m_postfix = postfix;
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::setLabelText(const QString &text)
{
    m_label->setText(text);
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::setDialogTitle(const QString &title)
{
    m_dialogTitle = title;
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::setCheckZero(bool check)
{
    m_checkZero = check;
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::setCheckLessThanZero(bool check)
{
    m_checkLessThanZero = check;
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::setFormula(const QString &formula)
{
    const QString userFormula = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());

    if (userFormula.length() > 80)
    {
        deploy();
    }

    m_textEdit->setPlainText(userFormula);
    MoveCursorToEnd(m_textEdit);
    // Evaluation is triggered automatically by the textChanged → timer chain
}

//---------------------------------------------------------------------------------------------------------------------
auto FormulaInputWidget::formula() const -> QString
{
    return VTranslateVars::TryFormulaFromUser(
        m_textEdit->toPlainText(),
        VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
auto FormulaInputWidget::value() const -> qreal
{
    return m_value;
}

//---------------------------------------------------------------------------------------------------------------------
auto FormulaInputWidget::isValid() const -> bool
{
    return m_valid;
}

//---------------------------------------------------------------------------------------------------------------------
auto FormulaInputWidget::eventFilter(QObject *object, QEvent *event) -> bool
{
    if (object == m_textEdit)
    {
        return FilterObject(object, event);
    }
    return QWidget::eventFilter(object, event);
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::evaluate()
{
    if (m_data == nullptr)
    {
        return;
    }

    const FormulaData data = {
        .formula = m_textEdit->toPlainText(),
        .variables = m_data->DataVariables(),
        .labelEditFormula = m_label,
        .labelResult = m_resultLabel,
        .postfix = m_postfix,
        .checkZero = m_checkZero,
        .checkLessThanZero = m_checkLessThanZero,
    };

    m_value = EvalToolFormula(qobject_cast<QDialog *>(window()), data, m_valid);
    emit valueChanged(m_valid);
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::deploy()
{
    auto *dlg = qobject_cast<QDialog *>(window());
    if (dlg != nullptr)
    {
        DeployFormula(dlg, m_textEdit, m_growButton, m_baseHeight);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::openFxDialog()
{
    if (m_data == nullptr || m_toolId == NULL_ID)
    {
        return;
    }

    auto *dialog = new DialogEditWrongFormula(m_data, m_toolId, window());
    dialog->setWindowTitle(m_dialogTitle.isEmpty() ? tr("Edit formula") : m_dialogTitle);
    dialog->SetFormula(formula());
    dialog->setPostfix(m_postfix);
    dialog->setCheckZero(m_checkZero);
    dialog->setCheckLessThanZero(m_checkLessThanZero);
    if (dialog->exec() == QDialog::Accepted)
    {
        setFormula(dialog->GetFormula());
    }
    delete dialog;
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::onTextChanged()
{
    m_timer->start(formulaTimerTimeout);
}
