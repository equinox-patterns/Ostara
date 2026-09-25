/************************************************************************
 **
 **  @file   othumbnailstylespage.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   25/09/2026
 **
 **  @brief  The Thumbnail page of the information block editor — implementation.
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

#include "othumbnailstylespage.h"

#include "core/ofontfaces.h"
#include "othumbnailpreview.h"

#include "vabstractvalapplication.h"
#include "vmisc/FactoryWidgets.h"
#include "vvalentinasettings.h"

#include <QCheckBox>
#include <QFontComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

//---------------------------------------------------------------------------------------------------------------------
OThumbnailStylesPage::OThumbnailStylesPage(QWidget *parent) : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(8, 8, 8, 8);
    outer->setSpacing(8);

    m_hint = new QLabel();
    m_hint->setWordWrap(true);
    outer->addWidget(m_hint);

    m_valuesGroup = MakeFontGroup(tr("Seam allowance values"), m_values, m_valuesForm);
    outer->addWidget(m_valuesGroup);

    m_legendGroup = MakeFontGroup(tr("Legend"), m_legend, m_legendForm);
    outer->addWidget(m_legendGroup);

    // The two units share a line: one choice with two answers, not two successive questions — the same
    // arrangement the Details panel gives them for a piece.
    m_unitsGroup = new QGroupBox(tr("Units"));
    auto *unitsRow = new QHBoxLayout(m_unitsGroup);
    unitsRow->setContentsMargins(8, 8, 8, 8);
    unitsRow->setSpacing(12);
    m_centimetersCheck = new QCheckBox(tr("Centimeters"));
    unitsRow->addWidget(m_centimetersCheck);
    m_inchesCheck = new QCheckBox(tr("Inches"));
    unitsRow->addWidget(m_inchesCheck);
    unitsRow->addStretch();
    outer->addWidget(m_unitsGroup);

    m_preview = new OThumbnailPreview();
    outer->addWidget(m_preview);
    outer->addStretch();

    RetranslateUI();
    Show();

    // **Every control writes its own setting, at once.** The preferences ARE the state: there is
    // nothing to save, nothing to cancel, and no second copy to keep in step with the drawing — the
    // same live commitment the Details panel's Thumbnail tab makes for one piece.
    const auto commit = [this]() {
        if (m_loading)
        {
            return;
        }
        WritePreferences();
        RefreshPreview();
    };

    connect(m_values.family, &QFontComboBox::currentFontChanged, this,
            [this, commit](const QFont &font) { OnFamilyChanged(m_values, font); commit(); });
    connect(m_legend.family, &QFontComboBox::currentFontChanged, this,
            [this, commit](const QFont &font) { OnFamilyChanged(m_legend, font); commit(); });
    connect(m_values.face, &QComboBox::currentIndexChanged, this, [commit](int) { commit(); });
    connect(m_legend.face, &QComboBox::currentIndexChanged, this, [commit](int) { commit(); });
    connect(m_values.size, &QSpinBox::valueChanged, this, [commit](int) { commit(); });
    connect(m_legend.size, &QSpinBox::valueChanged, this, [commit](int) { commit(); });
    connect(m_centimetersCheck, &QCheckBox::toggled, this,
            [this, commit](bool checked) { OnUnitToggled(true, checked); commit(); });
    connect(m_inchesCheck, &QCheckBox::toggled, this,
            [this, commit](bool checked) { OnUnitToggled(false, checked); commit(); });
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailStylesPage::MakeFontGroup(const QString &title, FontControls &controls,
                                         QFormLayout *&form) -> QGroupBox *
{
    auto *group = new QGroupBox(title);
    form = new QFormLayout(group);
    form->setContentsMargins(8, 8, 8, 8);
    form->setSpacing(6);

    controls.family = new QFontComboBox();
    form->addRow(tr("Font:"), controls.family);

    // Only the faces the family really has, italics excluded — see FillFontFaces: a number cannot
    // name a face, and an italic would be drawn upright by the export without saying so.
    controls.face = MakeComboBox(this);
    form->addRow(tr("Weight:"), controls.face);

    // A real number of pixels, never a word like « Default »: the size is what the drawing takes, and
    // the value it starts from is the application's own font (see OThumbnailItem::ValuesFont).
    controls.size = new QSpinBox();
    controls.size->setRange(1, 288);
    controls.size->setSuffix(QStringLiteral(" px"));
    form->addRow(tr("Size:"), controls.size);

    return group;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailStylesPage::OnFamilyChanged(FontControls &controls, const QFont &font)
{
    if (m_loading)
    {
        return;
    }
    // **The faces belong to the family, so they change with it.** The selection is kept when the new
    // family has a face of that name — which is what changing the family usually means — and the
    // family's Regular otherwise (the rule lives in FillFontFaces, and nowhere else).
    FillFontFaces(controls.face, font.family(), QString());
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailStylesPage::OnUnitToggled(bool centimeters, bool checked)
{
    if (m_loading)
    {
        return;
    }

    // ⚠️ **A miniature always states a unit — decision of Sophie, 25-09-2026.** Unchecking the last
    // unit checked is not refused and shows no message: it hands the answer to the other unit, in one
    // click, exactly as the Details panel's own pair of switches does for a piece. With neither, the
    // miniature prints no value at all (see OThumbnailTextBuilder::ValueText) — a block holding a
    // drawing captioned about nothing. The impossible state is made unreachable rather than repaired.
    QCheckBox *const other = centimeters ? m_inchesCheck : m_centimetersCheck;
    if (!checked && !other->isChecked())
    {
        // The other switch turning on is this gesture's answer, not a second one: it must not re-enter
        // and write a half-applied state.
        m_loading = true;
        other->setChecked(true);
        m_loading = false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailStylesPage::WritePreferences()
{
    VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();
    settings->SetThumbnailValuesFontFamily(m_values.family->currentFont().family());
    settings->SetThumbnailValuesFontFace(m_values.face->currentData().toString());
    settings->SetThumbnailValuesFontPixelSize(m_values.size->value());
    settings->SetThumbnailLegendFontFamily(m_legend.family->currentFont().family());
    settings->SetThumbnailLegendFontFace(m_legend.face->currentData().toString());
    settings->SetThumbnailLegendFontPixelSize(m_legend.size->value());
    settings->SetThumbnailShowCentimeters(m_centimetersCheck->isChecked());
    settings->SetThumbnailShowInches(m_inchesCheck->isChecked());
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailStylesPage::RefreshPreview()
{
    m_preview->Refresh();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailStylesPage::Show()
{
    m_loading = true;
    const VValentinaSettings *settings = VAbstractValApplication::VApp()->ValentinaSettings();

    const QString valuesFamily = settings->GetThumbnailValuesFontFamily();
    m_values.family->setCurrentFont(QFont(valuesFamily));
    m_values.size->setValue(settings->GetThumbnailValuesFontPixelSize());
    // ⚠️ The face list is refilled from the preference and NOT from what the control was showing:
    // `Show()` is a reload, and a face left over from a previous visit is not the stored answer.
    // `FillFontFaces` keeps the current selection, so it is dropped first.
    m_values.face->setCurrentIndex(-1);
    FillFontFaces(m_values.face, valuesFamily, settings->GetThumbnailValuesFontFace());

    const QString legendFamily = settings->GetThumbnailLegendFontFamily();
    m_legend.family->setCurrentFont(QFont(legendFamily));
    m_legend.size->setValue(settings->GetThumbnailLegendFontPixelSize());
    m_legend.face->setCurrentIndex(-1);
    FillFontFaces(m_legend.face, legendFamily, settings->GetThumbnailLegendFontFace());

    m_centimetersCheck->setChecked(settings->IsThumbnailShowCentimeters());
    m_inchesCheck->setChecked(settings->IsThumbnailShowInches());

    // A pair left with neither unit is not a state this application offers — and a settings file edited
    // by hand can still hold one. It is repaired here, in the open, rather than left for the miniature
    // to work around.
    if (!m_centimetersCheck->isChecked() && !m_inchesCheck->isChecked())
    {
        m_centimetersCheck->setChecked(true);
        VAbstractValApplication::VApp()->ValentinaSettings()->SetThumbnailShowCentimeters(true);
    }

    m_loading = false;
    RefreshPreview();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailStylesPage::SetLanguages(const QStringList &languages)
{
    m_preview->SetLanguages(languages);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailStylesPage::RetranslateUI()
{
    m_hint->setText(tr("What every miniature says: the seam-allowance values a piece is cut with, and "
                       "the legend that names their unit. The fonts apply to every miniature; the "
                       "units are the default a miniature starts from."));
    m_valuesGroup->setTitle(tr("Seam allowance values"));
    m_legendGroup->setTitle(tr("Legend"));
    m_unitsGroup->setTitle(tr("Units"));
    m_centimetersCheck->setText(tr("Centimeters"));
    m_inchesCheck->setText(tr("Inches"));

    const auto relabel = [](QFormLayout *form, QWidget *field, const QString &text) {
        if (auto *label = qobject_cast<QLabel *>(form->labelForField(field)))
        {
            label->setText(text);
        }
    };
    relabel(m_valuesForm, m_values.family, tr("Font:"));
    relabel(m_valuesForm, m_values.face, tr("Weight:"));
    relabel(m_valuesForm, m_values.size, tr("Size:"));
    relabel(m_legendForm, m_legend.family, tr("Font:"));
    relabel(m_legendForm, m_legend.face, tr("Weight:"));
    relabel(m_legendForm, m_legend.size, tr("Size:"));
}
