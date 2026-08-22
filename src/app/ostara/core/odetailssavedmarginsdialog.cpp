/************************************************************************
 **
 **  @file   odetailssavedmarginsdialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   21/08/2026
 **
 **  @brief  Saved custom seam allowances management dialog
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

#include "odetailssavedmarginsdialog.h"

#include "odetailsseamallowancewidget.h"

#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vmisc/vvalentinasettings.h"

#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
ODetailsSavedMarginsDialog::ODetailsSavedMarginsDialog(QWidget *parent)
  : QDialog(parent)
{
    setWindowTitle(tr("Saved custom seam allowances"));
    resize(520, 300);

    auto *layout = new QVBoxLayout(this);

    m_table = new QTableWidget(0, 3);
    m_table->setHorizontalHeaderLabels({tr("Name"), tr("Value"), tr("Color")});
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    m_table->setColumnWidth(1, 70);
    m_table->setColumnWidth(2, 55);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(m_table);

    const Unit patternUnit = VAbstractValApplication::VApp()->patternUnits();
    const QVector<SavedSeamAllowance> saved =
        VAbstractValApplication::VApp()->ValentinaSettings()->GetSavedSeamAllowances();
    for (const auto &s : saved)
    {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        m_table->setItem(row, 0, new QTableWidgetItem(s.name));

        const qreal displayVal = UnitConvertor(s.value, Unit::Mm, patternUnit);
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(displayVal, 'f', 1)));

        const int colorIdx = (s.colorIndex >= 0 && s.colorIndex < kCustomMarginColors.size()) ? s.colorIndex : 0;
        auto *colorItem = new QTableWidgetItem();
        colorItem->setBackground(kCustomMarginColors.at(colorIdx));
        colorItem->setData(Qt::UserRole, colorIdx);
        m_table->setItem(row, 2, colorItem);
    }

    auto *deleteBtn = new QPushButton(tr("Delete selected"));
    layout->addWidget(deleteBtn);
    connect(deleteBtn, &QPushButton::clicked, this, &ODetailsSavedMarginsDialog::DeleteSelected);

    auto *closeBtn = new QPushButton(tr("Close"));
    layout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSavedMarginsDialog::DeleteSelected()
{
    const int row = m_table->currentRow();
    if (row < 0)
    {
        return;
    }
    m_table->removeRow(row);
    Persist();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsSavedMarginsDialog::Persist()
{
    QVector<SavedSeamAllowance> list;
    const Unit patternUnit = VAbstractValApplication::VApp()->patternUnits();
    for (int r = 0; r < m_table->rowCount(); ++r)
    {
        SavedSeamAllowance s;
        s.name = m_table->item(r, 0)->text();
        const double v = m_table->item(r, 1)->text().toDouble();
        s.value = UnitConvertor(v, patternUnit, Unit::Mm);
        s.colorIndex = m_table->item(r, 2)->data(Qt::UserRole).toInt();
        list.append(s);
    }
    VAbstractValApplication::VApp()->ValentinaSettings()->SetSavedSeamAllowances(list);
}
