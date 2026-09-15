/************************************************************************
 **
 **  @file   oexporttiledformatwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the Tiled shape.
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

#include "oexporttiledformatwidget.h"

#include "../core/oexportunitformat.h"

#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <cmath>

namespace
{
/// The width of the widest sheet the mode can lay out in one piece: the A0 roll. Used as
/// the floor for the default column count, so that picking a tiled model does not quietly
/// ask for a plan narrower than anything the mode would otherwise produce — a run that
/// comes back "the document is too narrow" on a default is a default that wasted a run.
constexpr qreal kRollWidthMm = 841.0;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportTiledFormatWidget::OExportTiledFormatWidget(QWidget *parent)
  : QWidget(parent)
{
    auto *layout = new QFormLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_widthEdit = new QLineEdit(this);
    m_heightEdit = new QLineEdit(this);
    m_widthLabel = new QLabel(this);
    m_heightLabel = new QLabel(this);
    layout->addRow(m_widthLabel, m_widthEdit);
    layout->addRow(m_heightLabel, m_heightEdit);

    m_columns = new QSpinBox(this);
    m_columns->setMinimum(1);
    m_columns->setMaximum(99);
    m_columnsLabel = new QLabel(this);
    layout->addRow(m_columnsLabel, m_columns);

    // Committed on editingFinished, never on textChanged: re-formatting the field while
    // it is being typed into would fight the cursor, and half-typed text ("2" on the way
    // to "21") is not a length yet.
    connect(m_widthEdit, &QLineEdit::editingFinished, this, &OExportTiledFormatWidget::CommitEditedSides);
    connect(m_heightEdit, &QLineEdit::editingFinished, this, &OExportTiledFormatWidget::CommitEditedSides);
    connect(m_columns, &QSpinBox::valueChanged, this, [this](int) { emit Changed(); });

    Retranslate();
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportTiledFormatWidget::SetSheet(qreal widthMm, qreal heightMm, bool editable, bool hasColumns)
{
    m_widthMm = widthMm;
    m_heightMm = heightMm;

    m_widthEdit->setReadOnly(!editable);
    m_heightEdit->setReadOnly(!editable);

    m_hasColumns = hasColumns;
    m_columnsLabel->setVisible(hasColumns);
    m_columns->setVisible(hasColumns);

    if (hasColumns)
    {
        // Enough columns that the plan is at least as wide as a roll, and never fewer
        // than one. A4 lands on five, which is the count the mode was specified around.
        const int defaultColumns =
            widthMm > 0.0 ? static_cast<int>(std::ceil(kRollWidthMm / widthMm)) : 1;
        m_columns->setValue(std::max(1, defaultColumns));
    }

    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportTiledFormatWidget::SheetWidthMm() const -> qreal
{
    return m_widthMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportTiledFormatWidget::SheetHeightMm() const -> qreal
{
    return m_heightMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportTiledFormatWidget::Columns() const -> int
{
    return m_hasColumns ? m_columns->value() : 1;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportTiledFormatWidget::SetUnit(const Unit &unit)
{
    m_unit = unit;
    Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportTiledFormatWidget::CommitEditedSides()
{
    if (m_widthEdit->isReadOnly())
    {
        return; // the model's standard: nothing to commit, and nothing the user could type
    }

    bool widthOk = false;
    bool heightOk = false;
    const qreal widthMm = OExportUnitFormat::Parse(m_widthEdit->text(), m_unit, &widthOk);
    const qreal heightMm = OExportUnitFormat::Parse(m_heightEdit->text(), m_unit, &heightOk);

    // An unreadable field keeps the previous value rather than falling to zero: a frame
    // of zero width is not what a mistyped figure was asking for, and the nesting would
    // take it as a document with no room in it at all.
    if (widthOk && widthMm > 0.0)
    {
        m_widthMm = widthMm;
    }
    if (heightOk && heightMm > 0.0)
    {
        m_heightMm = heightMm;
    }

    Refresh();
    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportTiledFormatWidget::Refresh()
{
    m_widthEdit->setText(OExportUnitFormat::Format(m_widthMm, m_unit));
    m_heightEdit->setText(OExportUnitFormat::Format(m_heightMm, m_unit));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportTiledFormatWidget::Retranslate()
{
    m_widthLabel->setText(tr("Width:"));
    m_heightLabel->setText(tr("Height:"));
    m_columnsLabel->setText(tr("Columns:"));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportTiledFormatWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
