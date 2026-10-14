/************************************************************************
 **
 **  @file   ogradedrangedialog.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   04/09/2026
 **
 **  @brief  Graded-mode size-range dialog (3 steps, favorites aware).
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

#include "ogradedrangedialog.h"

#include "../vmisc/FactoryWidgets.h"
#include "core/ogradedsizerangename.h"
#include "widgets/osizerangeaxis.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QLineEdit>
#include <QtMath>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>

//---------------------------------------------------------------------------------------------------------------------
OGradedRangeDialog::OGradedRangeDialog(const Config &config, QWidget *parent)
  : QDialog(parent)
  , m_config(config)
  , m_sizeAxis(config.declaredAxis)
{
    setWindowTitle(tr("Graded mode - size range"));

    // The step-3 form (range + export settings) must stay wide enough for the
    // export path field — the form would otherwise hug its minimum width and the
    // path edit would barely show the folder (the "Browse…" button eats the rest).
    setMinimumWidth(560);

    auto *rootLayout = new QVBoxLayout(this);

    m_stack = new QStackedWidget(this);
    rootLayout->addWidget(m_stack);

    m_step1 = new QWidget(this);
    m_modePage = new QWidget(this);
    m_step2 = new QWidget(this);
    m_stack->addWidget(m_step1);   // page 0 — axis (only when needed)
    m_stack->addWidget(m_modePage); // page 1 — create / load from favorite
    m_stack->addWidget(m_step2);   // page 2 — range form

    BuildStep1Page();
    BuildModePage();

    // Buttons: Cancel / Next / OK
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch(1);

    auto *cancelButton = new QPushButton(tr("Cancel"), this);
    m_nextButton = new QPushButton(tr("Next"), this);
    m_okButton = new QPushButton(tr("OK"), this);

    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(m_nextButton);
    buttonLayout->addWidget(m_okButton);
    rootLayout->addLayout(buttonLayout);

    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_nextButton, &QPushButton::clicked, this, &OGradedRangeDialog::OnNext);
    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);

    m_okButton->setEnabled(false);

    if (m_config.editing)
    {
        // Edit: open straight on the range form, pre-filled (no axis, no favorite).
        BuildStep2Page();
        m_stack->setCurrentWidget(m_step2);
        CheckState();
        LoadExistingRange();
    }
    else if (m_config.axisStepNeeded)
    {
        m_stack->setCurrentWidget(m_step1); // step 1 first (axis still unknown)
    }
    else
    {
        // Axis already known — ask create vs favorite.
        ShowModePage();
    }

    UpdateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::BuildStep1Page()
{
    auto *layout = new QVBoxLayout(m_step1);

    auto *label = new QLabel(tr("Which axis of the table is the grading axis?"), m_step1);
    label->setWordWrap(true);
    layout->addWidget(label);

    m_axisCombo = MakeComboBox(m_step1);
    m_axisCombo->addItems(m_config.axisNames);
    if (m_axisCombo->count() > 0)
    {
        m_axisCombo->setCurrentIndex(0);
    }
    layout->addWidget(m_axisCombo);
    layout->addStretch(1);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::BuildModePage()
{
    auto *layout = new QVBoxLayout(m_modePage);

    m_newRadio = new QRadioButton(tr("Create a new size range"), m_modePage);
    m_loadRadio = new QRadioButton(tr("Load existing from favorite"), m_modePage);
    m_newRadio->setChecked(true);

    auto *favoriteLabel = new QLabel(tr("Favorite:"), m_modePage);
    m_favoriteCombo = MakeComboBox(m_modePage);
    m_favoriteCombo->setEnabled(false);

    layout->addWidget(m_newRadio);
    layout->addWidget(m_loadRadio);
    layout->addSpacing(6);
    layout->addWidget(favoriteLabel);
    layout->addWidget(m_favoriteCombo);
    layout->addStretch(1);

    auto updateMode = [this]() {
        const bool load = m_loadRadio != nullptr && m_loadRadio->isChecked();
        if (m_favoriteCombo != nullptr)
        {
            m_favoriteCombo->setEnabled(load);
        }
        UpdateModeState();
    };
    connect(m_newRadio, &QRadioButton::toggled, this, updateMode);
    connect(m_loadRadio, &QRadioButton::toggled, this, updateMode);
    connect(m_favoriteCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateMode);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::ShowModePage()
{
    m_favoriteCombo->clear();
    if (m_config.favoriteNames)
    {
        m_favoriteCombo->addItems(m_config.favoriteNames(m_sizeAxis));
    }

    const bool hasFavorites = m_favoriteCombo->count() > 0;
    m_loadRadio->setEnabled(hasFavorites);
    // Default: load a favorite when at least one exists, otherwise create new.
    if (hasFavorites)
    {
        m_loadRadio->setChecked(true);
    }
    else
    {
        m_newRadio->setChecked(true);
    }

    UpdateModeState();
    m_stack->setCurrentWidget(m_modePage);
    UpdateButtons();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::UpdateModeState()
{
    if (m_stack == nullptr || m_stack->currentWidget() != m_modePage)
    {
        return;
    }

    const bool load = m_loadRadio != nullptr && m_loadRadio->isChecked();
    const bool loadOk = load && m_favoriteCombo != nullptr && m_favoriteCombo->currentIndex() >= 0;
    const bool createOk = m_newRadio != nullptr && m_newRadio->isChecked();
    m_nextButton->setEnabled(createOk || loadOk);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::OnNext()
{
    if (m_stack->currentWidget() == m_step1)
    {
        // Axis chosen → move to create/load.
        if (m_axisCombo != nullptr)
        {
            m_sizeAxis = m_axisCombo->currentText();
        }
        ShowModePage();
        return;
    }

    if (m_stack->currentWidget() == m_modePage)
    {
        if (m_loadRadio != nullptr && m_loadRadio->isChecked()
            && m_favoriteCombo != nullptr && m_favoriteCombo->currentIndex() >= 0)
        {
            // Load from favorite: build directly from the stored definition.
            m_favoriteName = m_favoriteCombo->currentText();
            m_loadedFromFavorite = true;
            accept();
        }
        else
        {
            GoToStep3(); // create a new range → range form
        }
        return;
    }

    accept();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::GoToStep3()
{
    BuildStep2Page();
    m_stack->setCurrentWidget(m_step2);
    UpdateButtons();
    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::BuildStep2Page()
{
    // Rebuild (idempotent: navigation may run more than once).
    if (QLayout *oldLayout = m_step2->layout(); oldLayout != nullptr)
    {
        delete oldLayout;
        const QList<QWidget *> stale =
            m_step2->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget *w : stale)
        {
            delete w;
        }
    }
    m_otherCombos.clear();
    m_fileNameManual = false;
    m_layoutLayerManual = false;

    const QStringList values = m_config.axisValues.value(m_sizeAxis);
    auto *layout = new QVBoxLayout(m_step2);

    // Ligne 1 — the range name.
    m_nameEdit = new QLineEdit(m_step2);
    m_nameEdit->setPlaceholderText(tr("Size range name"));
    // A range name ends up inside an exported file name: `/` and `:` are refused at
    // the keystroke, not reported after the fact.
    m_nameEdit->setValidator(MakeSizeRangeNameValidator(m_nameEdit));
    auto *nameRow = new QHBoxLayout();
    nameRow->addWidget(new QLabel(tr("Name:"), m_step2));
    nameRow->addWidget(m_nameEdit, 1);
    layout->addLayout(nameRow);

    // Ligne 2 — the size interval (min/max) and the frozen other axes.
    auto *rangeGroup = new QGroupBox(tr("Size range"), m_step2);
    auto *rangeLayout = new QFormLayout(rangeGroup);
    // Right-aligned labels: each label hugs its field — a shared label column
    // sized to the longest label would otherwise leave a dead gap after the
    // short labels ("Minimum:", "Maximum:", …).
    rangeLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // The two ends of the range, taken from the axis's own values and in its own order, and filled
    // by **the shared rule** — the cover page's own range dialog asks the same question of the same
    // kind of table, and what the default ends are, or when two of them make a range at all, is one
    // answer wherever it is asked.
    m_minCombo = MakeComboBox(rangeGroup);
    rangeLayout->addRow(tr("Minimum:"), m_minCombo);

    m_maxCombo = MakeComboBox(rangeGroup);
    rangeLayout->addRow(tr("Maximum:"), m_maxCombo);

    OSizeRangeAxis::FillMinMax(m_minCombo, m_maxCombo, values);

    // One row per other axis — dynamic label = the real axis name; default 1st value.
    for (const QString &axis : m_config.axisNames)
    {
        if (axis == m_sizeAxis)
        {
            continue;
        }

        QComboBox *combo = MakeComboBox(rangeGroup);
        combo->addItems(m_config.axisValues.value(axis));
        combo->setCurrentIndex(0);
        m_otherCombos.append({axis, combo});
        rangeLayout->addRow(axis + QLatin1Char(':'), combo);
    }
    layout->addWidget(rangeGroup);

    // Ligne 3 — the export settings consumed by the graded SVG export.
    auto *exportGroup = new QGroupBox(tr("Export"), m_step2);
    auto *exportLayout = new QFormLayout(exportGroup);
    // Same label alignment as the "Size range" group (see above).
    exportLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_pathEdit = new QLineEdit(exportGroup);
    m_pathEdit->setText(m_config.exportPath);
    auto *pathRow = new QHBoxLayout();
    pathRow->addWidget(m_pathEdit, 1);
    auto *browseButton = new QPushButton(tr("Browse…"), exportGroup);
    pathRow->addWidget(browseButton);
    exportLayout->addRow(tr("Path:"), pathRow);
    connect(browseButton, &QPushButton::clicked, this, &OGradedRangeDialog::BrowseExportPath);

    m_fileNameEdit = new QLineEdit(exportGroup);
    exportLayout->addRow(tr("File name:"), m_fileNameEdit);

    // Scale H/V — "100%" checked by default; the spin boxes (in %) stay disabled
    // until their checkbox is unchecked. Scale = spin value / 100.
    auto *scaleGroup = new QGroupBox(tr("Scale"), exportGroup);
    auto *scaleLayout = new QFormLayout(scaleGroup);
    scaleLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    const auto addScaleRow = [scaleGroup, scaleLayout](const QString &label,
                                                             QCheckBox **outCheck,
                                                             QSpinBox **outSpin) {
        auto *check = new QCheckBox(tr("100%"), scaleGroup);
        check->setChecked(true);
        auto *spin = new QSpinBox(scaleGroup);
        spin->setRange(1, 1000);
        spin->setValue(100);
        spin->setSuffix(QStringLiteral("%"));
        spin->setEnabled(false);
        auto *row = new QHBoxLayout();
        row->addWidget(check);
        row->addWidget(new QLabel(tr("Or"), scaleGroup));
        row->addWidget(spin);
        row->addStretch(1);
        scaleLayout->addRow(label, row);
        connect(check, &QCheckBox::toggled, spin, &QSpinBox::setEnabled);
        *outCheck = check;
        *outSpin = spin;
    };
    addScaleRow(tr("Horizontal:"), &m_hScale100, &m_hScaleSpin);
    addScaleRow(tr("Vertical:"), &m_vScale100, &m_vScaleSpin);
    exportLayout->addRow(scaleGroup);

    m_layoutLayerEdit = new QLineEdit(exportGroup);
    m_layoutLayerEdit->setText(tr("Layout"));
    exportLayout->addRow(tr("Layout layer:"), m_layoutLayerEdit);

    layout->addWidget(exportGroup);

    connect(m_nameEdit, &QLineEdit::textChanged, this, &OGradedRangeDialog::CheckState);
    connect(m_minCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OGradedRangeDialog::CheckState);
    connect(m_maxCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OGradedRangeDialog::CheckState);
    for (const auto &pair : m_otherCombos)
    {
        connect(pair.second, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                &OGradedRangeDialog::CheckState);
    }

    // The export file name is autogenerated from the current interval/axes unless
    // the user has typed an override.
    connect(m_minCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OGradedRangeDialog::UpdateExportFileName);
    connect(m_maxCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &OGradedRangeDialog::UpdateExportFileName);
    for (const auto &pair : m_otherCombos)
    {
        connect(pair.second, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                &OGradedRangeDialog::UpdateExportFileName);
    }
    connect(m_fileNameEdit, &QLineEdit::textEdited, this,
            [this]() { m_fileNameManual = true; });
    connect(m_layoutLayerEdit, &QLineEdit::textEdited, this,
            [this]() { m_layoutLayerManual = true; });

    UpdateExportFileName();
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::UpdateButtons()
{
    const bool onStep3 = m_stack->currentWidget() == m_step2;
    m_nextButton->setVisible(!onStep3);
    m_okButton->setVisible(onStep3);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::CheckState()
{
    if (m_nameEdit == nullptr || m_minCombo == nullptr || m_maxCombo == nullptr)
    {
        return;
    }

    const QString name = m_nameEdit->text().trimmed();
    const bool nameOk = !name.isEmpty() && !m_config.takenNames.contains(name);
    const bool rangeOk = OSizeRangeAxis::IsOrdered(m_minCombo, m_maxCombo);
    m_okButton->setEnabled(nameOk && rangeOk);
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::LoadExistingRange()
{
    if (not m_config.editing)
    {
        return;
    }

    const OSizerange &range = m_config.initial;

    if (m_nameEdit != nullptr)
    {
        m_nameEdit->setText(range.name);
    }

    const QStringList values = m_config.axisValues.value(m_sizeAxis);
    if (m_minCombo != nullptr)
    {
        const int idx = static_cast<int>(values.indexOf(range.minValue));
        if (idx >= 0)
        {
            m_minCombo->setCurrentIndex(idx);
        }
    }
    if (m_maxCombo != nullptr)
    {
        const int idx = static_cast<int>(values.indexOf(range.maxValue));
        if (idx >= 0)
        {
            m_maxCombo->setCurrentIndex(idx);
        }
    }

    for (const auto &pair : m_otherCombos)
    {
        const QString current = range.axisValues.value(pair.first);
        if (not current.isEmpty())
        {
            const int idx = pair.second->findText(current);
            if (idx >= 0)
            {
                pair.second->setCurrentIndex(idx);
            }
        }
    }

    // Export group — restore what was stored on the range (overrides + scales).
    if (m_pathEdit != nullptr && not range.exportPath.isEmpty())
    {
        m_pathEdit->setText(range.exportPath);
    }
    if (m_fileNameEdit != nullptr && not range.exportFileName.isEmpty())
    {
        m_fileNameEdit->setText(range.exportFileName);
        m_fileNameManual = true;
    }
    if (m_layoutLayerEdit != nullptr && not range.exportLayoutLayerName.isEmpty())
    {
        m_layoutLayerEdit->setText(range.exportLayoutLayerName);
        m_layoutLayerManual = true;
    }
    if (m_hScaleSpin != nullptr && range.exportXScale != 1.0)
    {
        m_hScale100->setChecked(false);
        m_hScaleSpin->setValue(qRound(range.exportXScale * 100.0));
    }
    if (m_vScaleSpin != nullptr && range.exportYScale != 1.0)
    {
        m_vScale100->setChecked(false);
        m_vScaleSpin->setValue(qRound(range.exportYScale * 100.0));
    }

    CheckState();
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::sizeAxis() const -> QString
{
    return m_sizeAxis;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::range() const -> OSizerange
{
    OSizerange range;
    range.measurement = m_config.canonicalPath;
    range.sizeAxis = sizeAxis();

    if (m_nameEdit != nullptr)
    {
        range.name = m_nameEdit->text().trimmed();
    }
    if (m_minCombo != nullptr)
    {
        range.minValue = m_minCombo->currentText();
    }
    if (m_maxCombo != nullptr)
    {
        range.maxValue = m_maxCombo->currentText();
    }

    for (const auto &pair : m_otherCombos)
    {
        range.axisValues.insert(pair.first, pair.second->currentText());
    }

    // Export group (stored on the range so the graded SVG export can reuse them).
    range.exportPath = exportPath();
    range.exportFileName = m_fileNameManual ? exportFileName() : QString();
    range.exportLayoutLayerName = m_layoutLayerManual ? exportLayoutLayerName() : QString();
    range.exportXScale = exportXScale();
    range.exportYScale = exportYScale();

    return range;
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::BrowseExportPath()
{
    QString dir = m_pathEdit != nullptr ? m_pathEdit->text().trimmed() : QString();
    if (dir.isEmpty())
    {
        dir = QDir::homePath();
    }
    const QString selected = QFileDialog::getExistingDirectory(this, tr("Select folder"), dir);
    if (not selected.isEmpty() && m_pathEdit != nullptr)
    {
        m_pathEdit->setText(selected);
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::GenerateExportFileName() const -> QString
{
    QStringList parts;
    if (not m_config.patternNameToken.isEmpty())
    {
        parts.append(m_config.patternNameToken);
    }
    if (m_minCombo != nullptr)
    {
        parts.append(m_minCombo->currentText());
    }
    if (m_maxCombo != nullptr)
    {
        parts.append(m_maxCombo->currentText());
    }
    for (const auto &pair : m_otherCombos)
    {
        parts.append(pair.second->currentText());
    }
    return parts.join(QLatin1Char('_'));
}

//---------------------------------------------------------------------------------------------------------------------
void OGradedRangeDialog::UpdateExportFileName()
{
    if (m_fileNameEdit != nullptr && not m_fileNameManual)
    {
        m_fileNameEdit->setText(GenerateExportFileName());
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::exportPath() const -> QString
{
    return m_pathEdit != nullptr ? m_pathEdit->text().trimmed() : m_config.exportPath;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::exportFileName() const -> QString
{
    return m_fileNameEdit != nullptr ? m_fileNameEdit->text().trimmed() : QString();
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::exportLayoutLayerName() const -> QString
{
    return m_layoutLayerEdit != nullptr ? m_layoutLayerEdit->text().trimmed() : tr("Layout");
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::exportXScale() const -> qreal
{
    if (m_hScale100 != nullptr && m_hScale100->isChecked())
    {
        return 1.0;
    }
    return m_hScaleSpin != nullptr ? m_hScaleSpin->value() / 100.0 : 1.0;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::exportYScale() const -> qreal
{
    if (m_vScale100 != nullptr && m_vScale100->isChecked())
    {
        return 1.0;
    }
    return m_vScaleSpin != nullptr ? m_vScaleSpin->value() / 100.0 : 1.0;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::LoadedFromFavorite() const -> bool
{
    return m_loadedFromFavorite;
}

//---------------------------------------------------------------------------------------------------------------------
auto OGradedRangeDialog::FavoriteName() const -> QString
{
    return m_favoriteName;
}
