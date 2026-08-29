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

#include <algorithm>

#include <QCheckBox>
#include <QFontMetrics>
#include <QFormLayout>
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

    m_nameEdit = new QLineEdit();
    form->addRow(tr("Piece name"), m_nameEdit);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() { SetPieceNameInvalid(false); });

    m_numberSpin = new QSpinBox();
    m_numberSpin->setRange(0, 99);
    m_numberSpin->setSpecialValueText(QStringLiteral("—"));
    form->addRow(tr("Piece number"), m_numberSpin);

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

    m_alignWidget = new OGradedAlignWidget();
    mainLayout->addWidget(m_alignWidget);

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
auto OPiecePieceStep::GetPieceNumber() const -> uint
{
    return static_cast<uint>(m_numberSpin->value());
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
