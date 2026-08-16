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
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextDocument>
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

namespace
{
// Number of visible lines for each field state.
constexpr int kCollapsedLines = 1;
constexpr int kMinExpandedLines = 2; // floor: an empty / 1-line field stays at 2 lines
constexpr int kMaxLines = 10;        // ceiling: beyond this, the field scrolls
} // namespace

//---------------------------------------------------------------------------------------------------------------------
FormulaInputWidget::FormulaInputWidget(QWidget *parent)
  : QWidget(parent)
  , m_timer(new QTimer(this))
{
    m_timer->setSingleShot(true);
    buildUi();

    connect(m_timer, &QTimer::timeout, this, &FormulaInputWidget::evaluate);
    connect(m_fxButton, &QPushButton::clicked, this, &FormulaInputWidget::openFxDialog);
    connect(m_growButton, &QPushButton::clicked, this, &FormulaInputWidget::toggleExpanded);
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
    const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
    m_fxButton->setIcon(
        QIcon(QStringLiteral(":/icons/%1/function_%1.svg")
                  .arg(isDark ? QStringLiteral("dark") : QStringLiteral("light"))));

    m_resultLabel = new QLabel(QStringLiteral("_"), this);
    m_resultLabel->setMinimumWidth(60);
    m_resultLabel->setToolTip(tr("Value"));

    m_textEdit = new VPlainTextEdit(this);

    QLineEdit reference;
    reference.setFont(m_textEdit->font());
    m_lineHeight = reference.sizeHint().height();

    m_growButton = new QPushButton(this);
    m_growButton->setFixedSize(16, 16);
    m_growButton->setFlat(true);
    updateExpandIcon();

    // --- Layout ---
    auto *row1 = new QHBoxLayout;
    row1->addWidget(m_label);
    row1->addWidget(m_resultLabel);
    row1->addStretch();
    row1->addWidget(m_fxButton);

    auto *row2 = new QHBoxLayout;
    row2->addWidget(m_textEdit);
    row2->addWidget(m_growButton);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addLayout(row1);
    mainLayout->addLayout(row2);
    setLayout(mainLayout);
    // Keep the composite's height exactly at its content (label + field), so it
    // never stretches and leaves the leftover space to the notes field.
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    updateHeight();
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

    m_textEdit->setPlainText(userFormula);
    MoveCursorToEnd(m_textEdit);
    // Evaluation and height update are triggered automatically by the textChanged → timer chain
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
void FormulaInputWidget::toggleExpanded()
{
    m_expanded = !m_expanded;
    updateExpandIcon();
    updateHeight();
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::openFxDialog()
{
    if (m_data == nullptr)
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
    updateHeight();
    m_timer->start(formulaTimerTimeout);
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::updateHeight()
{
    const int lineCount = m_textEdit->document()->lineCount();

    int visibleLines = kCollapsedLines;
    if (m_expanded)
    {
        visibleLines = qBound(kMinExpandedLines, lineCount + 1, kMaxLines);
    }

    m_textEdit->setFixedHeight(heightForLines(visibleLines));
}

//---------------------------------------------------------------------------------------------------------------------
void FormulaInputWidget::updateExpandIcon()
{
    const bool dark = VTheme::ColorSheme() == VColorSheme::Dark;
    const QString theme = dark ? QStringLiteral("dark") : QStringLiteral("light");
    const QString iconName = m_expanded ? QStringLiteral("caret-up") : QStringLiteral("caret-down");

    m_growButton->setIcon(QIcon(QStringLiteral(":/icons/%1/%2_%1.svg").arg(theme, iconName)));
    m_growButton->setToolTip(m_expanded ? tr("Collapse") : tr("Expand"));
}

//---------------------------------------------------------------------------------------------------------------------
auto FormulaInputWidget::heightForLines(int lineCount) const -> int
{
    const int lineSpacing = m_textEdit->fontMetrics().lineSpacing();
    return m_lineHeight + (lineCount - 1) * lineSpacing;
}
