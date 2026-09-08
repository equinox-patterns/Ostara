/************************************************************************
 **
 **  @file   dialogpiecepathoptions.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   08/09/2026
 **
 **  @brief  Edit dialog for the attribute block of an existing internal path
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

#include "dialogpiecepathoptions.h"

#include "../dialogtoolbox.h"
#include "../../support/dialogeditwrongformula.h"
#include "../vpatterndb/calculator.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractapplication.h"
#include "../vpatterndb/vcontainer.h"
#include "vtheme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPalette>
#include <QPushButton>
#include <QScopedPointer>
#include <QSize>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <qnumeric.h>

//---------------------------------------------------------------------------------------------------------------------
DialogPiecePathOptions::DialogPiecePathOptions(const VContainer *data, quint32 pathId, QWidget *parent)
  : QDialog(parent),
    m_data(data),
    m_pathId(pathId),
    m_visibleTimer(new QTimer(this))
{
    BuildUi();

    connect(m_visibleEdit, &QLineEdit::textChanged, this, [this]() { m_visibleTimer->start(formulaTimerTimeout); });
    m_visibleTimer->setSingleShot(true);
    connect(m_visibleTimer, &QTimer::timeout, this, &DialogPiecePathOptions::EvalVisible);
    connect(m_visibleFx, &QToolButton::clicked, this, &DialogPiecePathOptions::FXVisible);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPiecePathOptions::SetPath(const VPiecePath &path)
{
    m_path = path;

    m_nameEdit->setText(path.GetName());
    m_nameEdit->setCursorPosition(0);
    SetPenStyle(path.GetPenType());
    m_firstCheck->setChecked(path.IsFirstToCuttingContour());
    m_lastCheck->setChecked(path.IsLastToCuttingContour());
    m_cutCheck->setChecked(path.IsCutPath());
    m_notMirroredCheck->setChecked(path.GetType() == PiecePathType::InternalPath ? path.IsNotMirrored() : false);
    SetFormulaVisible(path.GetVisibilityTrigger());
    EvalVisible(); // refresh the OK state
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPiecePathOptions::GetPath() const -> VPiecePath
{
    VPiecePath path = m_path; // keeps the type and the nodes — only attributes are edited
    path.SetName(m_nameEdit->text());
    path.SetPenType(PenStyle());
    path.SetFirstToCuttingContour(m_firstCheck->isChecked());
    path.SetLastToCuttingContour(m_lastCheck->isChecked());
    path.SetCutPath(m_cutCheck->isChecked());
    path.SetNotMirrored(m_notMirroredCheck->isChecked());
    path.SetVisibilityTrigger(GetFormulaVisible());
    return path;
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPiecePathOptions::BuildUi()
{
    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(6);
    grid->setVerticalSpacing(6);
    grid->setColumnStretch(1, 1);

    int row = 0;

    // Name
    m_nameLabel = new QLabel(this);
    m_nameEdit = new QLineEdit(this);
    grid->addWidget(m_nameLabel, row, 0);
    grid->addWidget(m_nameEdit, row, 1, 1, 2);
    ++row;

    // Type of pen
    m_penLabel = new QLabel(this);
    m_penCombo = MakeComboBox(this);
    ApplyComboBoxView(m_penCombo);
    FillComboBoxTypeLine(m_penCombo,
                         CurvePenStylesPics(m_penCombo->palette().color(QPalette::Base),
                                            m_penCombo->palette().color(QPalette::Text)));
    m_penCombo->setIconSize(QSize(80, 14)); // line-style swatches, same as DialogPiecePath
    m_penCombo->setMinimumWidth(80);
    grid->addWidget(m_penLabel, row, 0);
    grid->addWidget(m_penCombo, row, 1, 1, 2);
    ++row;

    // Continue / cut / mirror check boxes
    m_firstCheck = new QCheckBox(this);
    grid->addWidget(m_firstCheck, row, 0, 1, 3);
    ++row;

    m_lastCheck = new QCheckBox(this);
    grid->addWidget(m_lastCheck, row, 0, 1, 3);
    ++row;

    m_cutCheck = new QCheckBox(this);
    grid->addWidget(m_cutCheck, row, 0, 1, 3);
    ++row;

    m_notMirroredCheck = new QCheckBox(this);
    grid->addWidget(m_notMirroredCheck, row, 0, 1, 3);
    ++row;

    // Separator
    auto *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    grid->addWidget(separator, row, 0, 1, 3);
    ++row;

    // Visibility — same block as the "Internal mark" dialog (DialogPlaceLabel)
    m_visibleLabel = new QLabel(this);
    m_visibleEdit = new QLineEdit(this);
    m_visibleFx = new QToolButton(this);
    grid->addWidget(m_visibleLabel, row, 0);
    grid->addWidget(m_visibleEdit, row, 1);
    grid->addWidget(m_visibleFx, row, 2);
    ++row;

    // Hint under the field, aligned with it (not under the labels) — 2 lines, no blank line.
    m_visibleHint = new QLabel(this);
    m_visibleHint->setWordWrap(true);
    grid->addWidget(m_visibleHint, row, 1, 1, 2);
    ++row;

    grid->setRowStretch(row, 1);

    m_visibleFx->setFocusPolicy(Qt::NoFocus);
    m_visibleFx->setFixedSize(24, 24);
    m_visibleFx->setIconSize(QSize(16, 16));

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(grid);
    mainLayout->addWidget(buttonBox);

    RetranslateUi();
    SetFxIcons();
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPiecePathOptions::RetranslateUi()
{
    setWindowTitle(tr("Internal outline"));
    m_nameLabel->setText(tr("Name"));
    m_penLabel->setText(tr("Type of pen"));
    m_firstCheck->setText(tr("Continue the first point to the cutting contour"));
    m_lastCheck->setText(tr("Continue the last point to the cutting contour"));
    m_cutCheck->setText(tr("Cut on fabric (DXF export only)"));
    m_notMirroredCheck->setText(tr("Not mirrored"));
    m_visibleLabel->setText(tr("Visibility"));
    m_visibleHint->setText(tr("1 = visible, 0 = invisible, formulas allowed"));
    m_visibleFx->setToolTip(tr("Edit expression"));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPiecePathOptions::SetFxIcons()
{
    m_visibleFx->setIcon(VTheme::GetIconResource(QStringLiteral("icon"), QStringLiteral("24x24/fx.png")));
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPiecePathOptions::SetPenStyle(Qt::PenStyle style)
{
    const QString key = PenStyleToLineStyle(style);
    int idx = m_penCombo->findData(key);
    if (idx < 0)
    {
        idx = m_penCombo->findData(PenStyleToLineStyle(Qt::SolidLine));
    }
    m_penCombo->setCurrentIndex(qMax(idx, 0));
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPiecePathOptions::PenStyle() const -> Qt::PenStyle
{
    const QString data = m_penCombo->currentData().toString();
    const QString key = data.isEmpty() ? PenStyleToLineStyle(Qt::SolidLine) : data;
    return LineStyleToPenStyle(key);
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPiecePathOptions::EvalVisible()
{
    m_flagFormulaVisible = false;
    const QString text = m_visibleEdit->text().trimmed();
    QString reason;

    if (text.isEmpty())
    {
        reason = tr("Empty formula");
    }
    else
    {
        try
        {
            const QString formula = VAbstractApplication::VApp()->TrVars()->FormulaFromUser(
                text, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
            QScopedPointer<Calculator> const cal(new Calculator());
            const qreal result = cal->EvalFormula(m_data->DataVariables(), formula);

            if (qIsInf(result) || qIsNaN(result))
            {
                reason = tr("Invalid result. Value is infinite or NaN. Please, check your calculations.");
            }
            else if (result < 0)
            {
                reason = tr("Value can't be less than 0");
            }
            else
            {
                m_flagFormulaVisible = true;
            }
        }
        catch (qmu::QmuParserError &e)
        {
            reason = tr("Parser error: %1").arg(e.GetMsg());
        }
    }

    if (not m_flagFormulaVisible)
    {
        ChangeColor(m_visibleLabel, errorColor);
        m_visibleEdit->setToolTip(reason);
    }
    else
    {
        ChangeColor(m_visibleLabel, OkColor(this));
        m_visibleEdit->setToolTip(QString());
    }

    if (m_okButton != nullptr)
    {
        m_okButton->setEnabled(m_flagFormulaVisible);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPiecePathOptions::FXVisible()
{
    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(m_data, m_pathId, this));
    dialog->setWindowTitle(tr("Control visibility"));
    dialog->SetFormula(GetFormulaVisible());
    if (dialog->exec() == QDialog::Accepted)
    {
        SetFormulaVisible(dialog->GetFormula());
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto DialogPiecePathOptions::GetFormulaVisible() const -> QString
{
    return VTranslateVars::TryFormulaFromUser(m_visibleEdit->text(),
                                              VAbstractApplication::VApp()->Settings()->GetOsSeparator());
}

//---------------------------------------------------------------------------------------------------------------------
void DialogPiecePathOptions::SetFormulaVisible(const QString &formula)
{
    const QString text = VAbstractApplication::VApp()->TrVars()->FormulaToUser(
        formula, VAbstractApplication::VApp()->Settings()->GetOsSeparator());
    m_visibleEdit->setText(text);
    m_visibleEdit->setToolTip(text);
}
