/************************************************************************
 **
 **  @file   opieceinfowidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   30/08/2026
 **
 **  @brief  Piece information fields — implementation.
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

#include "opieceinfowidget.h"

#include "../vpatterndb/vcontainer.h"

#include <algorithm>

#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OPieceInfoWidget::OPieceInfoWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    // Number — piece number with a dynamic total ("/ N" where N = piece count).
    auto *form = new QFormLayout();
    auto *numberRow = new QHBoxLayout();
    numberRow->setSpacing(4);
    m_numberSpin = new QSpinBox();
    m_numberSpin->setRange(0, 99);
    m_numberSpin->setSpecialValueText(QStringLiteral("—"));
    numberRow->addWidget(m_numberSpin);
    m_totalLabel = new QLabel();
    numberRow->addWidget(m_totalLabel);
    numberRow->addStretch();
    form->addRow(tr("Number"), numberRow);
    m_numberLabel = qobject_cast<QLabel *>(form->labelForField(numberRow));
    layout->addLayout(form);

    // Cut — three fabric layers, each with quantity + on-fold flag (dialog step 2 pattern).
    m_cutTabs = new QTabWidget();
    InitCutTab(0, tr("Fabric"), &m_fabricQtySpin, &m_fabricOnFoldCheck);
    InitCutTab(1, tr("Interfacing"), &m_interfacingQtySpin, &m_interfacingOnFoldCheck);
    InitCutTab(2, tr("Lining"), &m_liningQtySpin, &m_liningOnFoldCheck);
    layout->addWidget(m_cutTabs);

    // Free text — grows with its content (3 visible lines minimum).
    m_freeTextLabel = new QLabel(tr("Free text"));
    layout->addWidget(m_freeTextLabel);
    m_freeTextEdit = new QPlainTextEdit();
    m_freeTextEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    const auto minFreeTextHeight = [this]() {
        return m_freeTextEdit->fontMetrics().lineSpacing() * 3 + m_freeTextEdit->frameWidth() * 2
               + static_cast<int>(m_freeTextEdit->document()->documentMargin()) * 2;
    };
    m_freeTextEdit->setFixedHeight(minFreeTextHeight());

    m_freeTextTimer = new QTimer(this);
    m_freeTextTimer->setSingleShot(true);
    m_freeTextTimer->setInterval(600);
    connect(m_freeTextTimer, &QTimer::timeout, this, [this]() {
        m_freeTextTimer->stop();
        if (m_loading || !m_modifyPending)
        {
            return;
        }
        const QString text = m_freeTextEdit->toPlainText();
        m_modifyPending([text](IPieceInfo &det) { det.SetFreeText(text); });
    });

    connect(m_freeTextEdit, &QPlainTextEdit::textChanged, this, [this, minFreeTextHeight]() {
        const int h = static_cast<int>(m_freeTextEdit->document()->size().height())
                      + m_freeTextEdit->frameWidth() * 2;
        m_freeTextEdit->setFixedHeight((std::max)(h, minFreeTextHeight()));
        if (!m_loading)
        {
            m_freeTextTimer->start();
        }
    });
    layout->addWidget(m_freeTextEdit);

    // Number + cut commits through the pending-mutation pipeline.
    connect(m_numberSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
        if (m_loading || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([value](IPieceInfo &det) { det.SetPieceNumber(static_cast<uint>(value)); });
    });

    connect(m_fabricQtySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int qty) {
        if (m_loading || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([qty](IPieceInfo &det) { det.SetCutFabricQty(qty); });
    });
    connect(m_interfacingQtySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int qty) {
        if (m_loading || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([qty](IPieceInfo &det) { det.SetCutInterfacingQty(qty); });
    });
    connect(m_liningQtySpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int qty) {
        if (m_loading || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([qty](IPieceInfo &det) { det.SetCutLiningQty(qty); });
    });

    connect(m_fabricOnFoldCheck, &QCheckBox::toggled, this, [this](bool on) {
        if (m_loading || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([on](IPieceInfo &det) { det.SetCutFabricOnFold(on); });
    });
    connect(m_interfacingOnFoldCheck, &QCheckBox::toggled, this, [this](bool on) {
        if (m_loading || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([on](IPieceInfo &det) { det.SetCutInterfacingOnFold(on); });
    });
    connect(m_liningOnFoldCheck, &QCheckBox::toggled, this, [this](bool on) {
        if (m_loading || !m_modifyPending)
        {
            return;
        }
        m_modifyPending([on](IPieceInfo &det) { det.SetCutLiningOnFold(on); });
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceInfoWidget::InitCutTab(int index, const QString &title, QSpinBox **qtySpin, QCheckBox **onFoldCheck)
{
    auto *page = new QWidget();
    auto *layout = new QHBoxLayout(page);

    // Quantity + spin + on-fold flag on a single row.
    m_quantityLabels[index] = new QLabel(tr("Quantity"));
    layout->addWidget(m_quantityLabels[index]);
    *qtySpin = new QSpinBox();
    (*qtySpin)->setRange(0, 99);
    layout->addWidget(*qtySpin);
    *onFoldCheck = new QCheckBox(tr("On fold"));
    layout->addWidget(*onFoldCheck);
    layout->addStretch();

    m_cutTabs->addTab(page, title);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceInfoWidget::SetModifyPending(std::function<void(std::function<void(IPieceInfo &)>)> callback)
{
    m_modifyPending = std::move(callback);
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceInfoWidget::Show(const IPieceInfo &detail, const VContainer *data)
{
    m_loading = true;
    m_freeTextTimer->stop();

    m_numberSpin->setValue(static_cast<int>(detail.GetPieceNumber()));
    // « Un groupe = 1 pièce » : the total counts standalone pieces + groups.
    m_totalLabel->setText(
        QStringLiteral("/ %1").arg(QString::number(data != nullptr ? data->GetEffectivePieceCount() : 0)));

    m_fabricQtySpin->setValue(detail.GetCutFabricQty());
    m_fabricOnFoldCheck->setChecked(detail.IsCutFabricOnFold());
    m_interfacingQtySpin->setValue(detail.GetCutInterfacingQty());
    m_interfacingOnFoldCheck->setChecked(detail.IsCutInterfacingOnFold());
    m_liningQtySpin->setValue(detail.GetCutLiningQty());
    m_liningOnFoldCheck->setChecked(detail.IsCutLiningOnFold());

    m_freeTextEdit->setPlainText(detail.GetFreeText());

    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceInfoWidget::Clear()
{
    m_loading = true;
    m_freeTextTimer->stop();

    m_numberSpin->setValue(0);
    m_totalLabel->clear();
    m_fabricQtySpin->setValue(0);
    m_fabricOnFoldCheck->setChecked(false);
    m_interfacingQtySpin->setValue(0);
    m_interfacingOnFoldCheck->setChecked(false);
    m_liningQtySpin->setValue(0);
    m_liningOnFoldCheck->setChecked(false);
    m_freeTextEdit->clear();

    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OPieceInfoWidget::RetranslateUI()
{
    if (m_numberLabel)
    {
        m_numberLabel->setText(tr("Number"));
    }
    for (int i = 0; i < 3; ++i)
    {
        if (m_quantityLabels[i])
        {
            m_quantityLabels[i]->setText(tr("Quantity"));
        }
    }
    m_fabricOnFoldCheck->setText(tr("On fold"));
    m_interfacingOnFoldCheck->setText(tr("On fold"));
    m_liningOnFoldCheck->setText(tr("On fold"));
    if (m_freeTextLabel)
    {
        m_freeTextLabel->setText(tr("Free text"));
    }
    m_cutTabs->setTabText(0, tr("Fabric"));
    m_cutTabs->setTabText(1, tr("Interfacing"));
    m_cutTabs->setTabText(2, tr("Lining"));
}
