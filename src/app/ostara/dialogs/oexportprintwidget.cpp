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

#include "../core/ocoverpagetemplatestorage.h"
#include "../core/oexporttemplatestorage.h"
#include "../core/oexportunitformat.h"
#include "../vpatterndb/osizerange.h"
#include "../vpatterndb/vcontainer.h"
#include "../vformat/vdimensions.h"
#include "../vformat/vmeasurements.h"
#include "../vmisc/FactoryWidgets.h"
#include "vabstractapplication.h"
#include "vcommonsettings.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QFileInfo>
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
    PopulateMarks();
    PopulateCoverPages();

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

    // ── What the sheets are marked with ─────────────────────────────────
    // The template of assembly marks is a file of the user's, and the plan is drawn with the one
    // she picks here. Offered where there are sheets to mark, and nowhere else: a roll is one
    // continuous sheet, and the A0 model is never assembled side by side — neither has a seam to
    // mark nor a frame to cut. It is the same predicate that decides whether there is a grid.
    m_marksCombo = MakeComboBox(this);
    m_marksLabel = new QLabel(this);
    layout->addRow(m_marksLabel, m_marksCombo);

    // ── The cover page, on EVERY format ─────────────────────────────────
    // Unlike the marks, this row is never hidden: a cover page stands on a roll and on an A0
    // sheet as much as on a framed one — where it stands is the format's own business, and
    // every format has an answer (OExportCoverPage). So the row does not follow the grid, and
    // UpdateModel must not fold it away with the marks.
    //
    // "No cover page" is the first entry and is always present: a plan without a cover page is
    // a state the user chooses, not an empty library.
    m_coverCombo = MakeComboBox(this);
    m_coverLabel = new QLabel(this);
    layout->addRow(m_coverLabel, m_coverCombo);

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
void OExportPrintWidget::PopulateMarks()
{
    const QSignalBlocker blocker(m_marksCombo);
    m_marksCombo->clear();

    const QStringList paths =
        OExportTemplateStorage::ScanDirectory(OExportTemplateStorage::DefaultDirectory());
    for (const QString &path : paths)
    {
        // The base name alone identifies a template, and it is read off the file's name rather
        // than out of the file: listing a library must not open every template in it.
        const QString name = QFileInfo(path).completeBaseName();
        m_marksCombo->addItem(name, name);
    }

    // The chain, in order: the user's favourite, then the template the installation ships, then
    // nothing at all. Nothing is a state and not a failure — a library the user has emptied is
    // one she emptied — and it reads as an empty field rather than as a message about something
    // she has no way to act on.
    const QString favourite = VAbstractApplication::VApp()->Settings()->GetDefaultPdfTemplate();
    const QStringList candidates{favourite, OExportTemplateStorage::DefaultTemplateName()};
    for (const QString &candidate : candidates)
    {
        const int index = candidate.isEmpty() ? -1 : m_marksCombo->findData(candidate);
        if (index >= 0)
        {
            m_marksCombo->setCurrentIndex(index);
            return;
        }
    }

    m_marksCombo->setCurrentIndex(-1);
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::PopulateCoverPages()
{
    const QSignalBlocker blocker(m_coverCombo);
    m_coverCombo->clear();

    // **"No cover page" first, and always present.** Unlike the marks, a plan has a legitimate
    // state in which there is no cover page — it is what the mode did before this existed — so
    // the field can answer "nothing" without being empty, and it opens on that answer when the
    // library holds neither the favourite nor a Default.
    //
    // Empty data, like every "none" entry in this application: what the run carries is the
    // string, and the empty string is the single representation the whole feature tests.
    m_coverCombo->addItem(tr("No cover page"), QString());

    const QStringList paths = OCoverPageTemplateStorage::ScanDirectory(
        OCoverPageTemplateStorage::DefaultDirectory());
    for (const QString &path : paths)
    {
        // The base name alone identifies a template, and it is read off the file's name rather
        // than out of the file: listing a library must not open every template in it.
        const QString name = QFileInfo(path).completeBaseName();
        m_coverCombo->addItem(name, name);
    }

    // The chain, in order: the user's favourite, then the template the installation falls back
    // to. Then — and only because neither exists — no cover page.
    //
    // ⚠️ Note the asymmetry with the marks, and that it is deliberate:
    // GetDefaultCoverPageTemplate answers EMPTY where GetDefaultPdfTemplate answers "Default",
    // because nothing seeds a cover page. The empty favourite is skipped rather than looked up,
    // and the chain falls through to Default — which answers nothing until such a template
    // exists, this library seeding none.
    const QString favourite =
        VAbstractApplication::VApp()->Settings()->GetDefaultCoverPageTemplate();
    const QStringList candidates{favourite, OCoverPageTemplateStorage::DefaultTemplateName()};
    for (const QString &candidate : candidates)
    {
        const int index = candidate.isEmpty() ? -1 : m_coverCombo->findData(candidate);
        if (index >= 0)
        {
            m_coverCombo->setCurrentIndex(index);
            return;
        }
    }

    // Neither exists: the field opens on "No cover page", which is today's behaviour exactly —
    // and the composed case is deliberately not an empty field, unlike the marks. There, an
    // empty library is a state the user made and a message would be about something she cannot
    // act on; here, "there is no cover page" is a thing she can choose, and it says so.
    m_coverCombo->setCurrentIndex(0);
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
        m_tiledWidget->SetSheet(model->widthMm, model->heightMm, model->hasColumns);
        m_tiledWidget->SetUnit(m_unit);
        m_shapeStack->setCurrentWidget(m_tiledWidget);
    }

    // The template row follows the model, exactly as the shape does, and label and field go
    // together: the form then collapses the row instead of leaving a gap where a choice was.
    const bool marks = FrameHeightMm() > 0.0;
    m_marksLabel->setVisible(marks);
    m_marksCombo->setVisible(marks);

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
    // ⚠️ **Lire avec l'ANCIENNE unité, changer, puis réafficher avec la NOUVELLE — dans cet ordre.**
    // L'ordre inverse relit les mêmes chiffres dans l'unité qui vient d'être choisie : « 0.5 » cm
    // deviendrait 0,5 pouce, soit 12,7 mm au lieu de 5 — et **pas un chiffre ne bougerait à
    // l'écran** pour le dire, puisque le champ est réécrit avec la même valeur numérique. Une unité
    // est une façon d'écrire une longueur, jamais une longueur.
    ReadEditedLengths(m_unit);

    m_unit = static_cast<Unit>(m_unitCombo->currentData().toInt());

    m_rollWidget->SetUnit(m_unit);
    m_tiledWidget->SetUnit(m_unit);
    WriteEditedLengths();
    UpdateDocumentWidth();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::CommitEditedLengths()
{
    ReadEditedLengths(m_unit);
    WriteEditedLengths();

    emit Changed();
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::ReadEditedLengths(const Unit &unit)
{
    bool marginOk = false;
    bool gapOk = false;
    const qreal marginMm = OExportUnitFormat::Parse(m_marginEdit->text(), unit, &marginOk);
    const qreal gapMm = OExportUnitFormat::Parse(m_gapEdit->text(), unit, &gapOk);

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
}

//---------------------------------------------------------------------------------------------------------------------
void OExportPrintWidget::WriteEditedLengths()
{
    m_marginEdit->setText(OExportUnitFormat::Format(m_marginMm, m_unit));
    m_gapEdit->setText(OExportUnitFormat::Format(m_gapMm, m_unit));
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
auto OExportPrintWidget::MarksTemplateName() const -> QString
{
    // An emptied combo answers nothing, which is exactly what the run then carries: no template,
    // and a plan drawn without marks.
    return m_marksCombo->currentData().toString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OExportPrintWidget::CoverPageTemplateName() const -> QString
{
    // "No cover page" carries empty data, so an untouched combo answers empty — which is what
    // the run then carries: nothing reserved in the bin, nothing drawn, no page added.
    return m_coverCombo->currentData().toString();
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
auto OExportPrintWidget::FrameHeightMm() const -> qreal
{
    const OExportPrintModel *model = FindPrintModel(PrintModelToken());
    if (model == nullptr || model->shape == OExportPrintShape::Roll || !model->hasColumns)
    {
        return 0.0;
    }

    // The two guards ask two different questions. A roll is one continuous sheet AND the
    // tiled widget is never refreshed for it — reading it there would hand back the sheet of
    // the last tiled model the user looked at, a length she never asked for. The A0 sheet is
    // the other way round: it has its two sides, but it is never placed side by side, so
    // there is no frame to cut a plan into, and it has to answer like the roll.
    return m_tiledWidget->SheetHeightMm();
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
    // « Generate: », et non « Nest: » : la ligne ne dit pas au logiciel comment imbriquer, elle dit
    // CE QUI va être produit — une gamme entière ou une seule taille. C'est aussi le mot des boutons
    // (« Generate this output »), et il ne répète pas les deux entrées (« Size range », « Single
    // size ») qui, elles, sont les réponses.
    m_sourceLabel->setText(tr("Generate:"));
    renameByData(QStringLiteral("range"), tr("Size range"));
    renameByData(QStringLiteral("single"), tr("Single size"));
    m_rangeLabel->setText(tr("Size range:"));
    m_modelLabel->setText(tr("Print model:"));
    m_marksLabel->setText(tr("PDF layout:"));
    m_coverLabel->setText(tr("Cover page:"));
    m_marginLabel->setText(tr("Document margin:"));
    m_gapLabel->setText(tr("Space between pieces:"));
    m_documentWidthLabel->setText(tr("Document width:"));

    // The unit names come from the application, not from this file.
    for (int i = 0; i < m_unitCombo->count(); ++i)
    {
        m_unitCombo->setItemText(i, UnitsToStr(static_cast<Unit>(m_unitCombo->itemData(i).toInt()), true));
    }

    // Each axis row is named by the measurement table itself, exactly as the Details top bar names
    // them: the table says what an axis is called, and a second name here would be one more thing
    // to keep in step with it. The rows and the dimensions are walked in the same order — the table
    // is a map, so its key order is the one PopulateDimensions built the rows in.
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
