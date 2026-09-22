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

#include "../core/oexportpaginationsettings.h"

#include "../core/oexportunitformat.h"

#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QSignalBlocker>
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

    // The sheet's two sides on ONE line, in a box that looks like a field and is not one: there
    // is nothing to type, and a QLineEdit would promise otherwise. One row rather than two,
    // because the sheet is a pair of lengths — two labelled fields said the same thing twice.
    m_sidesValue = new QLabel(this);
    m_sidesValue->setObjectName(QStringLiteral("exportSheetSizeValue"));
    layout->addRow(m_sidesValue);

    m_columns = new QSpinBox(this);
    m_columns->setMinimum(1);
    // Twenty-six, and the reason is not the layout: the two-axis form of the page number names a
    // column by a letter, and there is none past 'Z'. A run of more sheets would nest perfectly
    // well and print numbers that cannot be read, so it cannot be asked for. The limit is
    // declared next to the rule it comes from, in the pagination settings.
    m_columns->setMaximum(kPaginationMaxColumns);
    m_columnsLabel = new QLabel(this);
    layout->addRow(m_columnsLabel, m_columns);

    connect(m_columns, &QSpinBox::valueChanged, this, [this](int) {
        m_columnsChosen = true; // the count is the user's from here on
        emit Changed();
    });

    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportTiledFormatWidget::SetSheet(qreal widthMm, qreal heightMm, bool hasColumns)
{
    m_widthMm = widthMm;
    m_heightMm = heightMm;

    m_hasColumns = hasColumns;
    m_columnsLabel->setVisible(hasColumns);
    m_columns->setVisible(hasColumns);

    if (hasColumns && !m_columnsChosen)
    {
        // Enough columns that the plan is at least as wide as a roll, and never fewer
        // than one. A4 lands on five, which is the count the mode was specified around.
        const int defaultColumns =
            widthMm > 0.0 ? static_cast<int>(std::ceil(kRollWidthMm / widthMm)) : 1;

        // Blocked, and that is the whole point of the block: the handler on the spin box
        // marks the count as the user's, and a default written behind her back is not hers.
        // A count she typed outlives the model she typed it on.
        const QSignalBlocker blocker(m_columns);
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
void OExportTiledFormatWidget::Refresh()
{
    // A sentence, because that is what it is: the sheet is a pair of lengths, and the words
    // around the two figures are the ones the two fields carried. The unit is written after
    // EACH figure and not once at the end — "19 cm x 25.9" would read as a single length.
    const QString unit = UnitsToStr(m_unit, true);
    m_sidesValue->setText(tr("Width:") + QLatin1Char(' ') + OExportUnitFormat::Format(m_widthMm, m_unit)
                          + QLatin1Char(' ') + unit + QStringLiteral(" x ") + tr("Height:")
                          + QLatin1Char(' ') + OExportUnitFormat::Format(m_heightMm, m_unit)
                          + QLatin1Char(' ') + unit);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportTiledFormatWidget::Retranslate()
{
    m_columnsLabel->setText(tr("Columns:"));

    // The line of sides is rebuilt, not merely retitled: it carries translated words AND a
    // figure formatted in the unit of the moment, so there is no widget per word to relabel.
    Refresh();
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
