/************************************************************************
 **
 **  @file   ocovergradedaxiseditor.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   20/09/2026
 **
 **  @brief  The block that edits how a cover page draws the axis it grades along.
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

#include "ocovergradedaxiseditor.h"

#include "core/osizetable.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

namespace
{
/// The option a two-column map is stored as — the same shape the reader takes back.
auto StoredMap(const QHash<QString, QString> &map) -> QVariant
{
    QVariantMap stored;
    for (auto it = map.cbegin(); it != map.cend(); ++it)
    {
        if (!it.value().isEmpty())
        {
            stored.insert(it.key(), it.value());
        }
    }
    return stored;
}
} // namespace

OCoverGradedAxisEditor::OCoverGradedAxisEditor(const OTemplateOptionContext &context, QWidget *parent)
  : OTemplateOptionEditor(parent), m_context(context)
{
    BuildUi();
    Refresh(context);
}

void OCoverGradedAxisEditor::BuildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *rangeRow = new QFormLayout();
    rangeRow->setLabelAlignment(Qt::AlignLeft);

    m_traceLength = new QDoubleSpinBox(this);
    m_traceLength->setRange(0.1, 30.0);
    m_traceLength->setDecimals(1);
    m_traceLength->setSingleStep(0.5);
    m_traceLength->setSuffix(tr(" cm"));
    connect(m_traceLength, &QDoubleSpinBox::valueChanged, this, [this](double value) {
        Write(OCoverGradedAxisOptions::kTraceLength, value);
    });
    rangeRow->addRow(tr("Trace length:"), m_traceLength);
    layout->addLayout(rangeRow);

    // One row per value of the axis, built and rebuilt by Refresh: the question is asked of the
    // table, so its answers only exist once a table and an axis do.
    m_namesHost = new QWidget(this);
    m_namesLayout = new QVBoxLayout(m_namesHost);
    m_namesLayout->setContentsMargins(0, 0, 0, 0);
    m_namesLayout->setSpacing(4);
    layout->addWidget(m_namesHost);
}

void OCoverGradedAxisEditor::Write(const QString &key, const QVariant &value)
{
    // A control being refreshed has nobody to write to: the section editor hands `apply` over only
    // when the field is live, and Refresh is also called while the form is being built.
    if (m_context.apply)
    {
        m_context.apply(key, value);
    }
}

void OCoverGradedAxisEditor::ApplyNames(const QHash<QString, QString> &names)
{
    Write(OCoverGradedAxisOptions::kNames, StoredMap(names));
}

void OCoverGradedAxisEditor::RebuildNameRows(const OCoverGradedAxis &range)
{
    // The rows are thrown away and built again: the axis is what says which sizes exist, and a row
    // left over from another axis would name a size the page can never list.
    while (QLayoutItem *item = m_namesLayout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }

    const OSizeTable *table = m_context.table;
    if (table == nullptr || range.axis.isEmpty())
    {
        return;
    }

    // **Every value of the axis, not only the ones the range holds.** A page is designed once and
    // printed from many ranges — the same template, run against 36–44 today and 46–52 next month —
    // and the names written here are the names it will show then. Offering only what this range
    // happens to list would mean renaming the page again for every range it is ever printed from,
    // which is the work the alias exists to save.
    const QStringList values = table->AxisValues(range.axis);
    if (values.isEmpty())
    {
        return;
    }

    m_namesLayout->addWidget(new QLabel(tr("Size names:"), m_namesHost));

    for (const QString &label : values)
    {
        auto *row = new QWidget(m_namesHost);
        auto *layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);

        auto *word = new QLabel(label, row);
        word->setMinimumWidth(60);
        layout->addWidget(word);

        auto *edit = new QLineEdit(row);
        // The table's word is the placeholder, not the text: what the field holds is the *alias*,
        // and an empty field is a size the page writes exactly as the table does. Prefilling it
        // would turn every size into an override the moment the template was saved.
        edit->setPlaceholderText(label);
        edit->setText(range.names.value(label));
        connect(edit, &QLineEdit::textEdited, this, [this, label](const QString &text) {
            QHash<QString, QString> names =
                OCoverGradedAxisOptions::FromOptions(m_context.options).names;
            if (text.isEmpty())
            {
                names.remove(label);
            }
            else
            {
                names.insert(label, text);
            }
            ApplyNames(names);
        });
        layout->addWidget(edit, 1);
        m_namesLayout->addWidget(row);
    }
}

void OCoverGradedAxisEditor::Refresh(const OTemplateOptionContext &context)
{
    m_context = context;

    const OCoverGradedAxis range = OCoverGradedAxisOptions::FromOptions(context.options);
    m_traceLength->blockSignals(true);
    m_traceLength->setValue(range.traceLengthCm);
    m_traceLength->blockSignals(false);

    RebuildNameRows(range);
}
