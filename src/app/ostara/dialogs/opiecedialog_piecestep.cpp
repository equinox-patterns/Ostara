/************************************************************************
 **
 **  @file   opiecedialog_piecestep.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   28 August, 2026
 **
 **  @brief  Step 2 of the piece tool dialog: piece-level information.
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

#include "opiecedialog_piecestep.h"

#include "../widgets/ogradedalignwidget.h"
#include "../vtools/dialogs/dialogtoolbox.h"

#include <algorithm>

#include <QCheckBox>
#include <QFontMetrics>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OPiecePieceStep::OPiecePieceStep(QWidget *parent)
  : QWidget(parent)
{
    auto *mainLayout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    // Why the collected contour is refused: shown at the very top of the step, so the reason is readable as
    // soon as the dialog opens. A disabled OK button without a reason is a dead end.
    m_problemLabel = new QLabel();
    m_problemLabel->setWordWrap(true);
    m_problemLabel->setVisible(false);
    mainLayout->addWidget(m_problemLabel);

    m_nameEdit = new QLineEdit();
    form->addRow(tr("Piece name"), m_nameEdit);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this](const QString &name) {
        SetPieceNameInvalid(false);
        emit NameChanged(name);
    });

    // Piece number with a dynamic total ("/ N" where N = total piece count,
    // including the piece being created) — mirrors OPieceInfoWidget.
    auto *numberRow = new QHBoxLayout();
    numberRow->setSpacing(4);
    m_numberSpin = new QSpinBox();
    m_numberSpin->setRange(0, 99);
    m_numberSpin->setSpecialValueText(QStringLiteral("—"));
    numberRow->addWidget(m_numberSpin);
    m_totalLabel = new QLabel();
    numberRow->addWidget(m_totalLabel);
    numberRow->addStretch();
    form->addRow(tr("Piece number"), numberRow);

    mainLayout->addLayout(form);

    // Cut — three fabric layers, each with quantity + on-fold flag. No "block" here.
    m_cutTabs = new QTabWidget();
    InitCutTab(tr("Fabric"), &m_fabricQtySpin, &m_fabricOnFoldCheck);
    InitCutTab(tr("Interfacing"), &m_interfacingQtySpin, &m_interfacingOnFoldCheck);
    InitCutTab(tr("Lining"), &m_liningQtySpin, &m_liningOnFoldCheck);
    mainLayout->addWidget(m_cutTabs);

    // Free text — grows with its content (3 visible lines minimum).
    m_freeTextEdit = new QPlainTextEdit();
    m_freeTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    const auto minFreeTextHeight = [this]() {
        return m_freeTextEdit->fontMetrics().lineSpacing() * 3 + m_freeTextEdit->frameWidth() * 2
               + static_cast<int>(m_freeTextEdit->document()->documentMargin()) * 2;
    };
    m_freeTextEdit->setFixedHeight(minFreeTextHeight());
    connect(m_freeTextEdit->document(), &QTextDocument::contentsChanged, this, [this, minFreeTextHeight]() {
        // contentsChanged fires after the layout is notified, so document()->size() is current.
        const int h = static_cast<int>(m_freeTextEdit->document()->size().height())
                      + m_freeTextEdit->frameWidth() * 2;
        m_freeTextEdit->setFixedHeight(std::max(h, minFreeTextHeight()));
    });
    mainLayout->addWidget(new QLabel(tr("Free text")));
    mainLayout->addWidget(m_freeTextEdit);

    // Separator between the free text and the graded alignment group.
    m_preAlignSeparator = new QFrame();
    m_preAlignSeparator->setFrameShape(QFrame::HLine);
    m_preAlignSeparator->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(m_preAlignSeparator);

    m_alignWidget = new OGradedAlignWidget();
    mainLayout->addWidget(m_alignWidget);

    // Layout flags — grouped in one container so a host that does not expose them
    // (Link pieces) can hide the whole block in one call.
    m_flagsBlock = new QWidget();
    auto *flagsLayout = new QVBoxLayout(m_flagsBlock);
    flagsLayout->setContentsMargins(0, 0, 0, 0);

    auto *flagsSeparator = new QFrame();
    flagsSeparator->setFrameShape(QFrame::HLine);
    flagsSeparator->setFrameShadow(QFrame::Sunken);
    flagsLayout->addWidget(flagsSeparator);

    m_forceFlippingCheck = new QCheckBox(tr("Force flipping"));
    m_forceFlippingCheck->setToolTip(tr("Force piece to be always flipped in a layout."));
    flagsLayout->addWidget(m_forceFlippingCheck);

    m_symmetricalCopyCheck = new QCheckBox(tr("Symmetrical copy"));
    m_symmetricalCopyCheck->setToolTip(tr("When the number of copies is greater than 1 and 'Force flipping' is selected, each even-numbered copy of the piece will be flipped vertically."));
    // A new piece is born with the symmetrical copy checked (cutting plan prerequisite).
    m_symmetricalCopyCheck->setChecked(true);
    flagsLayout->addWidget(m_symmetricalCopyCheck);

    mainLayout->addWidget(m_flagsBlock);

    mainLayout->addStretch();
}

//---------------------------------------------------------------------------------------------------------------------
void OPiecePieceStep::InitCutTab(const QString &title, QSpinBox **qtySpin, QCheckBox **onFoldCheck)
{
    auto *page = new QWidget();
    auto *layout = new QHBoxLayout(page);

    // Quantity + spin + on-fold flag on a single row.
    layout->addWidget(new QLabel(tr("Quantity")));
    *qtySpin = new QSpinBox();
    (*qtySpin)->setRange(0, 99);
    layout->addWidget(*qtySpin);
    *onFoldCheck = new QCheckBox(tr("On fold"));
    layout->addWidget(*onFoldCheck);
    layout->addStretch();

    m_cutTabs->addTab(page, title);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::GetPieceName() const -> QString
{
    return m_nameEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
void OPiecePieceStep::SetPieceNameInvalid(bool invalid)
{
    m_nameEdit->setStyleSheet(invalid ? QStringLiteral("QLineEdit { border: 1px solid red; }") : QString());
}

//---------------------------------------------------------------------------------------------------------------------
void OPiecePieceStep::SetContourProblem(const QString &problem)
{
    m_problemLabel->setText(problem.isEmpty() ? QString() : DialogWarningIcon(m_problemLabel) + problem);
    m_problemLabel->setVisible(not problem.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::GetPieceNumber() const -> uint
{
    return static_cast<uint>(m_numberSpin->value());
}

//---------------------------------------------------------------------------------------------------------------------
void OPiecePieceStep::SetPieceTotal(int count)
{
    m_totalLabel->setText(QStringLiteral("/ %1").arg(QString::number(count)));
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::GetCutFabricQty() const -> int
{
    return m_fabricQtySpin->value();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::IsCutFabricOnFold() const -> bool
{
    return m_fabricOnFoldCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::GetCutInterfacingQty() const -> int
{
    return m_interfacingQtySpin->value();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::IsCutInterfacingOnFold() const -> bool
{
    return m_interfacingOnFoldCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::GetCutLiningQty() const -> int
{
    return m_liningQtySpin->value();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::IsCutLiningOnFold() const -> bool
{
    return m_liningOnFoldCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::GetFreeText() const -> QString
{
    return m_freeTextEdit->toPlainText();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::GetGradedAlignH() const -> int
{
    return m_alignWidget->GetAlignH();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::GetGradedAlignV() const -> int
{
    return m_alignWidget->GetAlignV();
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::IsForceFlipping() const -> bool
{
    return m_forceFlippingCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void OPiecePieceStep::SetForceFlipping(bool checked)
{
    m_forceFlippingCheck->setChecked(checked);
}

//---------------------------------------------------------------------------------------------------------------------
auto OPiecePieceStep::IsSymmetricalCopy() const -> bool
{
    return m_symmetricalCopyCheck->isChecked();
}

//---------------------------------------------------------------------------------------------------------------------
void OPiecePieceStep::SetSymmetricalCopy(bool checked)
{
    m_symmetricalCopyCheck->setChecked(checked);
}

//---------------------------------------------------------------------------------------------------------------------
void OPiecePieceStep::SetFlippingFlagsVisible(bool visible)
{
    m_preAlignSeparator->setVisible(visible);
    m_flagsBlock->setVisible(visible);
}
