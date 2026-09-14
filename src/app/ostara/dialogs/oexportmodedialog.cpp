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
#include <QSignalBlocker>
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
                                     const QStringList &takenNames, const OGradedSizeRanges::Context &context,
                                     QWidget *parent)
  : QDialog(parent),
    m_data(data),
    m_measurements(measurements),
    m_rangeContext(context),
    m_takenNames(takenNames)
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

    // ── Generation name — the dock lists it; the user names it ────────────
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText(tr("Output name"));
    connect(m_nameEdit, &QLineEdit::textChanged, this, &OExportModeDialog::OnNameChanged);

    m_nameLabel = new QLabel(tr("Output name:"), this);
    m_form->addRow(m_nameLabel, m_nameEdit);

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
    if (index < 0)
    {
        return -1.0;
    }

    for (int i = 0; i < m_dimensionCombos.size() && i < m_dimensionSlots.size(); ++i)
    {
        if (m_dimensionSlots.at(i) != index)
        {
            continue;
        }

        const QComboBox *combo = m_dimensionCombos.at(i);
        if (combo->currentIndex() == -1)
        {
            return -1.0;
        }

        return combo->currentData().toReal();
    }

    return -1.0; // the table carries no axis for this slot
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::Name() const -> QString
{
    return m_nameEdit->text().trimmed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::IsNameTaken(const QString &name) const -> bool
{
    return !name.isEmpty() && m_takenNames.contains(name);
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief OnNameChanged keep the field on a value the run can be filed under.
 *
 * A name already listed is not a name: the field is emptied rather than left holding one
 * the dialog would refuse on OK. Emptying re-enters this slot with an empty text — blocked
 * here, since the cue below reads an empty field as the same refusal.
 */
void OExportModeDialog::OnNameChanged(const QString &text)
{
    if (IsNameTaken(text.trimmed()))
    {
        const QSignalBlocker blocker(m_nameEdit);
        m_nameEdit->clear();
    }

    UpdateNameCue();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
/**
 * @brief UpdateNameCue redden the field while it holds nothing that can name a generation.
 *
 * The string is the one the other required name field of the project already uses,
 * OPiecePieceStep::SetPieceNameInvalid. Written twice rather than shared: a stylesheet
 * rule of its own would need a token, and the palette has no error colour — inventing one
 * is a theme decision, not this field's.
 */
void OExportModeDialog::UpdateNameCue()
{
    // The wipe above leaves the field empty on a taken name, so in practice what reddens is
    // an empty one. The taken test is spelled out all the same: it is the refusal the cue
    // answers to, and it must hold whichever path reaches here.
    const QString name = Name();
    m_nameEdit->setStyleSheet((name.isEmpty() || IsNameTaken(name))
                                  ? QStringLiteral("QLineEdit { border: 1px solid red; }")
                                  : QString());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportModeDialog::PopulateSizeRanges()
{
    m_rangeCombo->clear();

    // The Graded dock's own filter: only the ranges of the pattern's context are offered.
    // A range built against another table — or against another axis of the same table —
    // is not what the user is working on, and nesting it would take its geometry from one
    // axis while its styles come from another.
    const QHash<QString, OSizerange> *ranges = m_data != nullptr ? m_data->SizeRanges() : nullptr;
    if (ranges != nullptr)
    {
        QStringList names;
        for (auto it = ranges->cbegin(); it != ranges->cend(); ++it)
        {
            if (OGradedSizeRanges::Matches(it.value(), m_rangeContext))
            {
                names.append(it.key());
            }
        }
        names.sort();
        for (const QString &name : names)
        {
            m_rangeCombo->addItem(name, name);
        }
    }

    // "Size range" needs a range to walk and axes to walk it along. Offering it otherwise
    // promises a nesting that can only end on a warning about an empty range — and on a
    // table without axes there is nothing but a single size to nest.
    if (m_rangeCombo->count() == 0 || !HasAxes())
    {
        const int rangeItem = m_sourceCombo->findData(QStringLiteral("range"));
        if (rangeItem >= 0)
        {
            m_sourceCombo->removeItem(rangeItem); // "Single size" becomes the only entry
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportModeDialog::HasAxes() const -> bool
{
    return !m_measurements.isNull() && !m_measurements->Dimensions().isEmpty();
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
    const VDimensions dimensions = m_measurements->Dimensions();
    for (auto it = dimensions.constBegin(); it != dimensions.constEnd(); ++it)
    {
        const MeasurementDimension_p &dimension = it.value();

        auto *combo = MakeComboBox(this);
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        const DimesionLabels labels = dimension->Labels();
        for (const qreal base : dimension->ValidBases())
        {
            QString text;
            for (auto label = labels.constBegin(); label != labels.constEnd(); ++label)
            {
                if (qAbs(label.key() - base) < kBaseTolerance)
                {
                    text = label.value();
                    break;
                }
            }
            if (text.isEmpty())
            {
                text = QString::number(base);
            }

            combo->addItem(text, base);
        }

        auto *rowLabel = new QLabel(dimension->Name() + QStringLiteral(":"), this);
        m_form->addRow(rowLabel, combo);
        m_dimensionLabels.append(rowLabel);

        // Which axis the row feeds. The document is put on a size by naming its axes —
        // X, Y, W — so the row's rank in the table cannot say it: a table missing Y
        // shifts every row below it by one slot. The dimension that has no slot of its
        // own (Z) is still shown, and feeds nothing.
        const int dimensionIndex = static_cast<int>(it.key());
        m_dimensionCombos.append(combo);
        m_dimensionSlots.append(dimensionIndex < 3 ? dimensionIndex : -1);
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
    // The name is required, and it is what identifies the row: two generations cannot
    // answer to the same one. The wipe of OnNameChanged already leaves the field empty on
    // a duplicate, but the rule is stated here too — otherwise OK would depend on the cue
    // having run.
    bool valid = DocumentWidth() > 0.0 && !Name().isEmpty() && !IsNameTaken(Name());

    if (IsSingleSize())
    {
        // A table without axes has no size to pick along them: the pattern is on its one
        // size, and the geometry is read as it stands. Requiring a dimension to exist here
        // locked the only path that could work on an .otu — there was never one to add.
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
        m_nameLabel->setText(tr("Output name:"));
        m_nameEdit->setPlaceholderText(tr("Output name"));
        m_sourceLabel->setText(tr("Nest:"));
        m_sourceCombo->setItemText(0, tr("Size range"));
        m_sourceCombo->setItemText(1, tr("Single size"));
        m_rangeLabel->setText(tr("Size range:"));
        UpdateUnitLabel();
    }

    QDialog::changeEvent(event);
}
