/************************************************************************
 **
 **  @file   oexportmodedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   12/09/2026
 **
 **  @brief  Export mode entry dialog — what to nest, on which bin.
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

#include "oexportmodedialog.h"

#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vformat/vdimensions.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/def.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractvalapplication.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include <utility>

namespace
{
/// Labels and bases are both doubles coming from the measurements table: they are
/// compared with a tolerance, never exactly.
constexpr qreal kBaseTolerance = 1e-9;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportModeDialog::OExportModeDialog(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                     QWidget *parent)
  : QDialog(parent),
    m_data(data),
    m_measurements(measurements)
{
    setWindowTitle(tr("Export"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);
    m_form = new QFormLayout();

    // ── Document width — typed in the pattern's unit ─────────────────────
    m_widthEdit = new QLineEdit(this);
    m_widthEdit->setPlaceholderText(tr("Document width"));
    connect(m_widthEdit, &QLineEdit::textChanged, this, [this](const QString &) { CheckState(); });

    m_unitLabel = new QLabel(this);

    auto *widthRow = new QWidget(this);
    auto *widthLayout = new QHBoxLayout(widthRow);
    widthLayout->setContentsMargins(0, 0, 0, 0);
    widthLayout->addWidget(m_widthEdit);
    widthLayout->addWidget(m_unitLabel);

    m_widthLabel = new QLabel(tr("Document width:"), this);
    m_form->addRow(m_widthLabel, widthRow);

    // ── What to nest: a whole range, or one size ─────────────────────────
    m_sourceCombo = MakeComboBox(this);
    m_sourceCombo->addItem(tr("Size range"), QStringLiteral("range"));
    m_sourceCombo->addItem(tr("Single size"), QStringLiteral("single"));
    connect(m_sourceCombo, &QComboBox::currentIndexChanged, this,
            [this](int)
            {
                UpdateSourceVisibility();
                CheckState();
            });

    m_sourceLabel = new QLabel(tr("Nest:"), this);
    m_form->addRow(m_sourceLabel, m_sourceCombo);

    // ── Size range ───────────────────────────────────────────────────────
    m_rangeCombo = MakeComboBox(this);
    connect(m_rangeCombo, &QComboBox::currentIndexChanged, this, [this](int) { CheckState(); });

    m_rangeLabel = new QLabel(tr("Size range:"), this);
    m_form->addRow(m_rangeLabel, m_rangeCombo);

    // ── Single size: the same dimension drop-downs Details shows on top ──
    PopulateDimensions();

    layout->addLayout(m_form);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(m_buttonBox);

    UpdateUnitLabel();
    PopulateSizeRanges();
    UpdateSourceVisibility();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::DocumentWidth() const -> qreal
{
    // The pattern unit is what the user typed in; the decimal separator is the one
    // the keyboard offers, which is not always the C locale's.
    QString text = m_widthEdit->text().trimmed();
    text.replace(QLatin1Char(','), QLatin1Char('.'));

    bool ok = false;
    const qreal value = text.toDouble(&ok);
    return ok ? value : 0.0;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::SizeRangeName() const -> QString
{
    if (IsSingleSize())
    {
        return {};
    }

    return m_rangeCombo->currentData().toString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::IsSingleSize() const -> bool
{
    return m_sourceCombo != nullptr && m_sourceCombo->currentData().toString() == QStringLiteral("single");
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::DimensionBase(int index) const -> qreal
{
    if (index < 0 || index >= m_dimensionCombos.size())
    {
        return -1.0;
    }

    const QComboBox *combo = m_dimensionCombos.at(index);
    if (combo->currentIndex() == -1)
    {
        return -1.0;
    }

    return combo->currentData().toReal();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::PopulateSizeRanges()
{
    m_rangeCombo->clear();

    if (m_data == nullptr)
    {
        return;
    }

    const QHash<QString, OSizerange> *ranges = m_data->SizeRanges();
    if (ranges == nullptr)
    {
        return;
    }

    QStringList names = ranges->keys();
    names.sort();
    for (const QString &name : names)
    {
        m_rangeCombo->addItem(name, name);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::PopulateDimensions()
{
    if (m_measurements.isNull())
    {
        return;
    }

    // One row per dimension of the measurement table, in the table's own order:
    // this is what Details offers in its top bar, and the size the user picks here
    // is applied through the very same path.
    const QList<MeasurementDimension_p> dimensions = m_measurements->Dimensions().values();
    for (const MeasurementDimension_p &dimension : dimensions)
    {
        auto *combo = MakeComboBox(this);
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        const DimesionLabels labels = dimension->Labels();
        for (const qreal base : dimension->ValidBases())
        {
            QString text;
            for (auto it = labels.constBegin(); it != labels.constEnd(); ++it)
            {
                if (qAbs(it.key() - base) < kBaseTolerance)
                {
                    text = it.value();
                    break;
                }
            }
            if (text.isEmpty())
            {
                text = QString::number(base);
            }

            combo->addItem(text, base);
        }

        auto *label = new QLabel(dimension->Name() + QStringLiteral(":"), this);
        m_form->addRow(label, combo);
        m_dimensionLabels.append(label);
        m_dimensionCombos.append(combo);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::UpdateSourceVisibility()
{
    const bool single = IsSingleSize();

    m_rangeLabel->setVisible(!single);
    m_rangeCombo->setVisible(!single);

    for (QLabel *label : std::as_const(m_dimensionLabels))
    {
        label->setVisible(single);
    }
    for (QComboBox *combo : std::as_const(m_dimensionCombos))
    {
        combo->setVisible(single);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::UpdateUnitLabel()
{
    m_unitLabel->setText(UnitsToStr(VAbstractValApplication::VApp()->patternUnits(), true));
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::CheckState()
{
    bool valid = DocumentWidth() > 0.0;

    if (IsSingleSize())
    {
        valid = valid && !m_dimensionCombos.isEmpty();
        for (const QComboBox *combo : std::as_const(m_dimensionCombos))
        {
            valid = valid && combo->count() > 0;
        }
    }
    else
    {
        valid = valid && !SizeRangeName().isEmpty();
    }

    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        setWindowTitle(tr("Export"));
        m_widthLabel->setText(tr("Document width:"));
        m_sourceLabel->setText(tr("Nest:"));
        m_sourceCombo->setItemText(0, tr("Size range"));
        m_sourceCombo->setItemText(1, tr("Single size"));
        m_rangeLabel->setText(tr("Size range:"));
        UpdateUnitLabel();
    }

    QDialog::changeEvent(event);
}
