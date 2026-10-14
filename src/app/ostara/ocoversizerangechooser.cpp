/************************************************************************
 **
 **  @file   ocoversizerangechooser.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The block that asks which sizes a cover page previews, and says what it is for.
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

#include "ocoversizerangechooser.h"

#include "core/osizetable.h"
#include "dialogs/ocoversizerangedialog.h"
#include "vmisc/vabstractapplication.h"
#include "vmisc/vcommonsettings.h"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace
{
/// What the block says the page currently lists.
auto SummaryOf(const OCoverGradedAxis &range) -> QString
{
    if (!range.HasRange())
    {
        return QCoreApplication::translate("OCoverSizeRangeChooser", "No size range chosen");
    }
    return QCoreApplication::translate("OCoverSizeRangeChooser", "%1 - from %2 to %3")
        .arg(range.axis, range.minValue, range.maxValue);
}
} // namespace

OCoverSizeRangeChooser::OCoverSizeRangeChooser(const OTemplateOptionContext &context, QWidget *parent)
  : OTemplateOptionEditor(parent), m_context(context)
{
    BuildUi();
    Refresh(context);
}

void OCoverSizeRangeChooser::BuildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *line = new QWidget(this);
    auto *lineLayout = new QHBoxLayout(line);
    lineLayout->setContentsMargins(0, 0, 0, 0);
    lineLayout->setSpacing(8);

    m_summary = new QLabel(this);
    m_summary->setWordWrap(true);
    m_chooseButton = new QPushButton(tr("Choose a size range…"), this);
    connect(m_chooseButton, &QPushButton::clicked, this, &OCoverSizeRangeChooser::ChooseRange);

    lineLayout->addWidget(m_summary, 1);
    lineLayout->addWidget(m_chooseButton);
    layout->addWidget(line);

    // **The note is not decoration.** The sizes below are the ones the designer wants to see, and
    // the printed page takes the ones the output was run with — a difference worth saying out loud,
    // or the user would believe they had just chosen what the sheet will hold.
    m_note = new QLabel(tr("For the preview only. When printing, the page takes the size range the "
                           "export is run with."),
                        this);
    m_note->setWordWrap(true);
    layout->addWidget(m_note);

    m_noTable = new QLabel(tr("Load a multi-size measurements table (.omt) to list sizes."), this);
    m_noTable->setWordWrap(true);
    layout->addWidget(m_noTable);
}

void OCoverSizeRangeChooser::ChooseRange()
{
    const OSizeTable *table = m_context.table;
    if (table == nullptr || !table->HasMultiSizeTable())
    {
        return;
    }

    OCoverSizeRangeDialog::Config config;
    config.canonicalPath = table->SourcePath();
    config.axisNames = table->AxisNames();
    for (const QString &axis : config.axisNames)
    {
        config.axisValues.insert(axis, table->AxisValues(axis));
    }
    // The axis a pattern grades along by default: the table's own declaration, when it is in the
    // library behind the Size Styles dock. Asking again for an axis the software already remembers
    // would be asking the user to repeat themselves.
    config.declaredAxis =
        VAbstractApplication::VApp()->Settings()->GetSizeAxisForMeasurement(config.canonicalPath);
    config.current = OCoverGradedAxisOptions::FromOptions(m_context.options);

    OCoverSizeRangeDialog dialog(config, this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    // The names and the sample length are the other block's answers, not this one's: what is
    // written here is the range, and writing the rest would be this block claiming them.
    const OCoverGradedAxis chosen = dialog.Chosen();
    if (m_context.apply)
    {
        m_context.apply(OCoverGradedAxisOptions::kRange, chosen.measurement);
        m_context.apply(OCoverGradedAxisOptions::kAxis, chosen.axis);
        m_context.apply(OCoverGradedAxisOptions::kMin, chosen.minValue);
        m_context.apply(OCoverGradedAxisOptions::kMax, chosen.maxValue);
    }
    Refresh(m_context);
}

void OCoverSizeRangeChooser::Refresh(const OTemplateOptionContext &context)
{
    m_context = context;

    const bool hasTable = context.table != nullptr && context.table->HasMultiSizeTable();
    m_summary->setText(SummaryOf(OCoverGradedAxisOptions::FromOptions(context.options)));
    m_chooseButton->setEnabled(hasTable);
    m_noTable->setVisible(!hasTable);
    m_note->setVisible(hasTable);
}
