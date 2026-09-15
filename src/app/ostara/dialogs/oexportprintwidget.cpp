/************************************************************************
 **
 **  @file   oexportprintwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   15/09/2026
 **
 **  @brief  Implementation of the Print tab.
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

#include "oexportprintwidget.h"

#include "oexportprintmodel.h"
#include "oexportrollformatwidget.h"
#include "oexporttiledformatwidget.h"

#include "../core/oexportunitformat.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vformat/vdimensions.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/FactoryWidgets.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStandardItemModel>

#include <utility>

namespace
{
/// Axis labels and bases are both doubles coming from the measurements table: they are
/// compared with a tolerance, never exactly.
constexpr qreal kBaseTolerance = 1e-9;

/// Where the model labels are filed for translation. Spelled out here as well as in
/// oexportprintmodel.cpp: lupdate reads the literal, and this is where it is used.
constexpr auto kModelContext = "OExportPrintModel";
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OExportPrintWidget::OExportPrintWidget(const VContainer *data, const QSharedPointer<VMeasurements> &measurements,
                                       const OGradedSizeRanges::Context &context, QWidget *parent)
  : QWidget(parent),
    m_data(data),
    m_measurements(measurements),
    m_rangeContext(context)
{
    BuildUi();

    // Before anything is populated: the labels are written by Retranslate and nowhere else,
    // so a tab that never ran it would come up with every field unlabelled.
    Retranslate();

    PopulateSizeRanges();
    PopulateModels();

    // The default model comes from the table itself — the first entry — so that changing
    // what the combo lists is enough to change what the tab opens on.
    UpdateModel();
    UpdateSourceVisibility();
    UpdateUnit();
    UpdateDocumentWidth();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::BuildUi()
{
    auto *layout = new QFormLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // ── Unit — the tab's, not the pattern's ──────────────────────────────
    m_unitCombo = MakeComboBox(this);
    for (const Unit &unit : OExportUnitFormat::SelectableUnits())
    {
        m_unitCombo->addItem(UnitsToStr(unit, true), static_cast<int>(unit));
    }
    m_unitCombo->setCurrentIndex(m_unitCombo->findData(static_cast<int>(m_unit)));
    connect(m_unitCombo, &QComboBox::currentIndexChanged, this, [this](int) { UpdateUnit(); });
    m_unitLabel = new QLabel(this);
    layout->addRow(m_unitLabel, m_unitCombo);

    // ── What to nest ─────────────────────────────────────────────────────
    m_sourceCombo = MakeComboBox(this);
    m_sourceCombo->addItem(QString(), QStringLiteral("range"));
    m_sourceCombo->addItem(QString(), QStringLiteral("single"));
    connect(m_sourceCombo, &QComboBox::currentIndexChanged, this, [this](int) { UpdateSourceVisibility(); });
    m_sourceLabel = new QLabel(this);
    layout->addRow(m_sourceLabel, m_sourceCombo);

    m_rangeCombo = MakeComboBox(this);
    connect(m_rangeCombo, &QComboBox::currentIndexChanged, this, [this](int) { emit Changed(); });
    m_rangeLabel = new QLabel(this);
    layout->addRow(m_rangeLabel, m_rangeCombo);

    // The axis rows belong to the question just above — they appear when the answer is
    // "single size", and sit between it and the print model. Built here rather than after
    // the whole form: appended later they would land under the model's own fields, and the
    // cascade would read in the wrong order.
    PopulateDimensions();

    // ── How it comes out ─────────────────────────────────────────────────
    m_modelCombo = MakeComboBox(this);
    connect(m_modelCombo, &QComboBox::currentIndexChanged, this, [this](int) { UpdateModel(); });
    m_modelLabel = new QLabel(this);
    layout->addRow(m_modelLabel, m_modelCombo);

    // The two shapes are stacked rather than built and torn down: switching model swaps
    // which one is shown, and each keeps what the user typed in it while it is away.
    m_shapeStack = new QStackedWidget(this);
    m_rollWidget = new OExportRollFormatWidget(m_shapeStack);
    m_tiledWidget = new OExportTiledFormatWidget(m_shapeStack);
    m_shapeStack->addWidget(m_rollWidget);
    m_shapeStack->addWidget(m_tiledWidget);
    connect(m_tiledWidget, &OExportTiledFormatWidget::Changed, this, &OExportPrintWidget::UpdateDocumentWidth);
    layout->addRow(m_shapeStack);

    // ── The two lengths the nesting takes, whatever the model ────────────
    m_marginEdit = new QLineEdit(this);
    connect(m_marginEdit, &QLineEdit::editingFinished, this, &OExportPrintWidget::CommitEditedLengths);
    m_marginLabel = new QLabel(this);
    layout->addRow(m_marginLabel, m_marginEdit);

    m_gapEdit = new QLineEdit(this);
    connect(m_gapEdit, &QLineEdit::editingFinished, this, &OExportPrintWidget::CommitEditedLengths);
    m_gapLabel = new QLabel(this);
    layout->addRow(m_gapLabel, m_gapEdit);

    // ── What it adds up to ───────────────────────────────────────────────
    // Shown because it is nowhere else: on a tiled model the width the nesting runs on is
    // the product of two fields, and a user who cannot see the product has no way to tell
    // a plan that came out narrow from one she asked for narrow.
    m_documentWidthValue = new QLabel(this);
    m_documentWidthLabel = new QLabel(this);
    layout->addRow(m_documentWidthLabel, m_documentWidthValue);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::PopulateSizeRanges()
{
    m_rangeCombo->clear();

    // The Graded dock's own filter: only the ranges of the pattern's context are offered.
    // A range built against another table — or another axis of the same table — is not
    // what the user is working on, and nesting it would take its geometry from one axis
    // while its styles come from another.
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
    // table with no axis there is nothing but a single size to nest.
    if (m_rangeCombo->count() == 0 || !HasAxes())
    {
        const int rangeItem = m_sourceCombo->findData(QStringLiteral("range"));
        if (rangeItem >= 0)
        {
            m_sourceCombo->removeItem(rangeItem);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::HasAxes() const -> bool
{
    return !m_measurements.isNull() && !m_measurements->Dimensions().isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::PopulateDimensions()
{
    if (m_measurements.isNull())
    {
        return;
    }

    auto *form = qobject_cast<QFormLayout *>(layout());

    // One row per dimension of the measurement table, in the table's own order: this is
    // what Details offers in its top bar, and the size the user picks here is applied
    // through the very same path.
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

        // Which axis the row feeds. The document is put on a size by naming its axes —
        // X, Y, W — so the row's rank in the table cannot say it: a table missing Y
        // shifts every row below it by one slot. The dimension that has no slot of its
        // own (Z) is still shown, and feeds nothing.
        const int dimensionIndex = static_cast<int>(it.key());
        m_dimensionCombos.append(combo);
        m_dimensionSlots.append(dimensionIndex < 3 ? dimensionIndex : -1);

        connect(combo, &QComboBox::currentIndexChanged, this, [this](int) { emit Changed(); });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::PopulateModels()
{
    const QString previous = PrintModelToken();

    const QSignalBlocker blocker(m_modelCombo);
    m_modelCombo->clear();

    const QVector<OExportPrintModel> &models = OExportPrintModels();
    for (int i = 0; i < models.size(); ++i)
    {
        const OExportPrintModel &model = models.at(i);
        m_modelCombo->addItem(QCoreApplication::translate(kModelContext, model.label),
                              QLatin1String(model.token));

        if (!model.selectable)
        {
            // Present and greyed rather than absent: a model that silently vanished would
            // read as one that was never planned, when it is one that is not finished.
            if (auto *standard = qobject_cast<QStandardItemModel *>(m_modelCombo->model()))
            {
                if (QStandardItem *item = standard->item(i))
                {
                    item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
                }
            }
        }
    }

    // Keep the user where she was, unless the model is gone or was never selectable.
    const int restored = previous.isEmpty() ? -1 : m_modelCombo->findData(previous);
    const OExportPrintModel *model = restored >= 0 ? FindPrintModel(previous) : nullptr;
    if (restored >= 0 && model != nullptr && model->selectable)
    {
        m_modelCombo->setCurrentIndex(restored);
    }
    else
    {
        m_modelCombo->setCurrentIndex(0);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::UpdateModel()
{
    const OExportPrintModel *model = FindPrintModel(PrintModelToken());
    if (model == nullptr)
    {
        return;
    }

    if (model->shape == OExportPrintShape::Roll)
    {
        m_rollWidget->SetWidthMm(model->widthMm);
        m_rollWidget->SetUnit(m_unit);
        m_shapeStack->setCurrentWidget(m_rollWidget);
    }
    else
    {
        m_tiledWidget->SetSheet(model->widthMm, model->heightMm, model->dimsEditable, model->hasColumns);
        m_tiledWidget->SetUnit(m_unit);
        m_shapeStack->setCurrentWidget(m_tiledWidget);
    }

    UpdateDocumentWidth();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::UpdateSourceVisibility()
{
    const bool single = IsSingleSize();

    // The question goes away when there is only one answer to it. On a table with no axis
    // there is no range to walk — only a single size — and a combo holding one entry asks
    // the user nothing. Label and field both, so the form collapses the row and leaves no
    // gap where a choice used to be.
    const bool hasChoice = m_sourceCombo->count() > 1;
    m_sourceLabel->setVisible(hasChoice);
    m_sourceCombo->setVisible(hasChoice);

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

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::UpdateUnit()
{
    const int raw = m_unitCombo->currentData().toInt();
    m_unit = static_cast<Unit>(raw);

    m_rollWidget->SetUnit(m_unit);
    m_tiledWidget->SetUnit(m_unit);
    CommitEditedLengths();
    UpdateDocumentWidth();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::CommitEditedLengths()
{
    bool marginOk = false;
    bool gapOk = false;
    const qreal marginMm = OExportUnitFormat::Parse(m_marginEdit->text(), m_unit, &marginOk);
    const qreal gapMm = OExportUnitFormat::Parse(m_gapEdit->text(), m_unit, &gapOk);

    // A margin of zero is legitimate — it is a plan run to the edge — so only a genuinely
    // unreadable field is ignored, and the previous value stands.
    if (marginOk && marginMm >= 0.0)
    {
        m_marginMm = marginMm;
    }
    if (gapOk && gapMm >= 0.0)
    {
        m_gapMm = gapMm;
    }

    m_marginEdit->setText(OExportUnitFormat::Format(m_marginMm, m_unit));
    m_gapEdit->setText(OExportUnitFormat::Format(m_gapMm, m_unit));

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::UpdateDocumentWidth()
{
    m_documentWidthValue->setText(OExportUnitFormat::Format(DocumentWidthMm(), m_unit)
                                  + QLatin1Char(' ') + UnitsToStr(m_unit, true));
    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::DocumentWidthMm() const -> qreal
{
    const OExportPrintModel *model = FindPrintModel(PrintModelToken());
    if (model == nullptr)
    {
        return 0.0;
    }

    if (model->shape == OExportPrintShape::Roll)
    {
        return m_rollWidget->WidthMm();
    }

    // The nesting knows one width and nothing of sheets: the sheets are laid out side by
    // side, so the plan is as wide as the columns put together — the small side each time,
    // every model being portrait.
    return m_tiledWidget->SheetWidthMm() * static_cast<qreal>(m_tiledWidget->Columns());
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::MarginMm() const -> qreal
{
    return m_marginMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::GapMm() const -> qreal
{
    return m_gapMm;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::SizeRangeName() const -> QString
{
    if (IsSingleSize())
    {
        return {};
    }

    return m_rangeCombo->currentData().toString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::IsSingleSize() const -> bool
{
    // No combo at all means there was no choice to make: the tab only removed it once
    // "Size range" could not be honoured, which leaves a single size as the only reading.
    if (m_sourceCombo->count() < 2)
    {
        return true;
    }

    return m_sourceCombo->currentData().toString() == QStringLiteral("single");
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::DimensionBase(int index) const -> qreal
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
auto OExportPrintWidget::DimensionLabel(int index) const -> QString
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
auto OExportPrintWidget::PrintModelToken() const -> QString
{
    return m_modelCombo->currentData().toString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::Columns() const -> int
{
    const OExportPrintModel *model = FindPrintModel(PrintModelToken());
    if (model == nullptr || model->shape == OExportPrintShape::Roll)
    {
        return 1; // a roll is one continuous sheet: there is nothing to place beside it
    }

    return m_tiledWidget->Columns();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::DisplayUnit() const -> Unit
{
    return m_unit;
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::IsValid() const -> bool
{
    if (DocumentWidthMm() <= 0.0)
    {
        return false;
    }

    if (IsSingleSize())
    {
        // A table without axes has no size to pick along them: the pattern is on its one
        // size, and the geometry is read as it stands. Requiring a dimension to exist here
        // would lock the only path that can work on a single-size table.
        for (const QComboBox *combo : std::as_const(m_dimensionCombos))
        {
            if (combo->count() == 0)
            {
                return false;
            }
        }
        return true;
    }

    return !SizeRangeName().isEmpty();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::Retranslate()
{
    // The source entries are addressed by their data, never by their rank: "Size range" is
    // removed once it cannot be honoured, and an index-based write would then rename
    // "Single size" — the one entry left — to the label of the one that is gone.
    const auto renameByData = [this](const QString &data, const QString &text)
    {
        const int index = m_sourceCombo->findData(data);
        if (index >= 0)
        {
            m_sourceCombo->setItemText(index, text);
        }
    };

    m_unitLabel->setText(tr("Unit:"));
    m_sourceLabel->setText(tr("Nest:"));
    renameByData(QStringLiteral("range"), tr("Size range"));
    renameByData(QStringLiteral("single"), tr("Single size"));
    m_rangeLabel->setText(tr("Size range:"));
    m_modelLabel->setText(tr("Print model:"));
    m_marginLabel->setText(tr("Document margin:"));
    m_gapLabel->setText(tr("Space between pieces:"));
    m_documentWidthLabel->setText(tr("Document width:"));

    // The unit names come from the application, not from this file.
    for (int i = 0; i < m_unitCombo->count(); ++i)
    {
        m_unitCombo->setItemText(i, UnitsToStr(static_cast<Unit>(m_unitCombo->itemData(i).toInt()), true));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        Retranslate();
        // The model labels are filed under their own context and are not reached by this
        // widget's tr() — they are re-read from the table, which is also what carries the
        // current selection across.
        PopulateModels();
    }

    QWidget::changeEvent(event);
}
