/************************************************************************
 **
 **  @file   oexportcuttingwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   23/09/2026
 **
 **  @brief  The Cutting plan tab of the Export dialog — the width, and one
 **          piece per group.
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

#include "oexportcuttingwidget.h"

#include "../core/opiecegrouplayoutflags.h"
#include "../core/oexportunitformat.h"
#include "../ogradedgeometry.h"
#include "../vformat/vdimensions.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/def.h"
#include "../vpatterndb/opiecegroup.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"

#include <QComboBox>
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace
{
/// A size is compared with a tolerance, never exactly.
constexpr qreal kBaseTolerance = 1e-9;
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportCuttingWidget::OExportCuttingWidget(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                           QWidget *parent)
  : QWidget(parent),
    m_data(data),
    m_measurements(measurements)
{
    BuildUi();

    // The labels are written by Retranslate and nowhere else: a tab that never ran it would come
    // up with every field unlabelled.
    Retranslate();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::BuildUi()
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *form = new QFormLayout();
    layout->addLayout(form);

    // ── Which size — one, never a range ──────────────────────────────────
    PopulateDimensions(form);

    // ── How wide the fabric is ───────────────────────────────────────────
    auto *widthRow = new QHBoxLayout();
    m_fabricWidthEdit = new QLineEdit(this);
    connect(m_fabricWidthEdit, &QLineEdit::editingFinished, this, &OExportCuttingWidget::CommitFabricWidth);
    widthRow->addWidget(m_fabricWidthEdit);

    m_fabricWidthUnit = new QLabel(this);
    widthRow->addWidget(m_fabricWidthUnit);
    widthRow->addStretch();

    m_fabricWidthLabel = new QLabel(this);
    form->addRow(m_fabricWidthLabel, widthRow);

    // ── Which member of each group ───────────────────────────────────────
    PopulateGroups();
    layout->addWidget(m_groupsBox);

    layout->addStretch();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::PopulateDimensions(QFormLayout *form)
{
    if (m_measurements.isNull())
    {
        // A table without axes describes a single size: the document is already on it, and there
        // is nothing to pick along. The run reads the geometry as it stands.
        return;
    }

    // One row per dimension of the measurement table, in the table's own order — the same rows,
    // from the same table, as the Print tab offers for a single size.
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

        auto *rowLabel = new QLabel(this);
        form->addRow(rowLabel, combo);
        m_dimensionLabels.append(rowLabel);

        // Which axis the row feeds. The document is put on a size by naming its axes — X, Y, W —
        // so the row's rank in the table cannot say it: a table missing Y shifts every row below
        // it by one slot. The dimension with no slot of its own (Z) is still shown, and feeds
        // nothing.
        const int dimensionIndex = static_cast<int>(it.key());
        m_dimensionCombos.append(combo);
        m_dimensionSlots.append(dimensionIndex < 3 ? dimensionIndex : -1);

        connect(combo, &QComboBox::currentIndexChanged, this, [this](int) { emit Changed(); });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::PopulateGroups()
{
    m_groupsBox = new QGroupBox(this);

    auto *rows = new QVBoxLayout(m_groupsBox);
    m_groupRows.clear();

    const QHash<quint32, OPieceGroup> *groups = m_data != nullptr ? m_data->DataPieceGroups() : nullptr;
    if (groups != nullptr)
    {
        QList<quint32> groupIds = groups->keys();
        std::sort(groupIds.begin(), groupIds.end());

        for (quint32 groupId : groupIds)
        {
            const OPieceGroup group = groups->value(groupId);
            const QVector<quint32> members = OPieceGroupLayoutFlags::ExistingMemberIds(group.pieceIds, m_data);

            // A group of fewer than two members is not a group: the canvas, the export and the
            // container all read it that way. There is nothing to choose between, so there is no
            // question to ask — its members are cut as ordinary pieces.
            if (members.size() < OGraded::kMinGroupMembers)
            {
                continue;
            }

            auto *row = new QHBoxLayout();

            auto *name = new QLabel(this);
            name->setText(group.GetName().isEmpty() ? tr("Unnamed group") : group.GetName());
            row->addWidget(name);

            auto *combo = MakeComboBox(this);
            for (quint32 pieceId : members)
            {
                const VPiece piece = m_data->GetPiece(pieceId);
                combo->addItem(piece.GetName().isEmpty() ? tr("Unnamed") : piece.GetName(), pieceId);
            }
            // Nothing is chosen to begin with: the user must say which piece she cuts, and a combo
            // that had already answered would let a plan run on a piece nobody picked.
            combo->setCurrentIndex(-1);
            connect(combo, &QComboBox::currentIndexChanged, this, [this](int) { emit Changed(); });
            row->addWidget(combo);

            row->addStretch();
            rows->addLayout(row);

            m_groupRows.append(GroupRow{groupId, name, combo});
        }
    }

    // The box says nothing when there is no group to ask about, and an empty box with a title
    // would be a question with no answer to give.
    m_groupsBox->setVisible(not m_groupRows.isEmpty());
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::CommitFabricWidth()
{
    bool ok = false;
    const qreal widthMm = OExportUnitFormat::Parse(m_fabricWidthEdit->text(), Unit::Cm, &ok);

    // A laize of zero or less is not a narrow fabric, it is a field that was not filled: the
    // previous value stands rather than running the nesting on nothing.
    if (ok && widthMm > 0.0)
    {
        m_fabricWidthMm = widthMm;
    }

    m_fabricWidthEdit->setText(OExportUnitFormat::Format(m_fabricWidthMm, Unit::Cm));

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::FabricWidthMm() const -> qreal
{
    return m_fabricWidthMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::MemberChoices() const -> QMap<quint32, quint32>
{
    QMap<quint32, quint32> choices;
    for (const GroupRow &row : m_groupRows)
    {
        if (row.combo->currentIndex() >= 0)
        {
            choices.insert(row.groupId, row.combo->currentData().toUInt());
        }
    }
    return choices;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::DimensionBase(int index) const -> qreal
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
auto OExportCuttingWidget::DimensionLabel(int index) const -> QString
{
    if (index < 0)
    {
        return {};
    }

    for (int i = 0; i < m_dimensionCombos.size() && i < m_dimensionSlots.size(); ++i)
    {
        if (m_dimensionSlots.at(i) == index)
        {
            return m_dimensionCombos.at(i)->currentText();
        }
    }

    return {}; // the table carries no axis for this slot — the name skips it
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportCuttingWidget::IsValid() const -> bool
{
    if (FabricWidthMm() <= 0.0)
    {
        return false;
    }

    // A table without axes has no size to pick along them: the pattern is on its one size, and the
    // geometry is read as it stands. Requiring a dimension to exist would lock that path.
    for (const QComboBox *combo : std::as_const(m_dimensionCombos))
    {
        if (combo->count() == 0)
        {
            return false;
        }
    }

    // Every group must have its piece: a group left unanswered is a piece the plan would have to
    // pick for the user.
    for (const GroupRow &row : m_groupRows)
    {
        if (row.combo->currentIndex() < 0)
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::Retranslate()
{
    m_fabricWidthLabel->setText(tr("Fabric width:"));
    m_fabricWidthUnit->setText(UnitsToStr(Unit::Cm, true));
    m_groupsBox->setTitle(tr("One piece per group"));

    // Each axis row is named by the measurement table itself, exactly as the Details top bar names
    // them: the table says what an axis is called, and a second name here would be one more thing
    // to keep in step with it.
    if (m_measurements.isNull())
    {
        return;
    }

    const VDimensions dimensions = m_measurements->Dimensions();
    int row = 0;
    for (auto it = dimensions.constBegin(); it != dimensions.constEnd() && row < m_dimensionLabels.size();
         ++it, ++row)
    {
        m_dimensionLabels.at(row)->setText(it.value()->Name() + QLatin1Char(':'));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportCuttingWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
    }

    QWidget::changeEvent(event);
}
