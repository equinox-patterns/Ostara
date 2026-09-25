/************************************************************************
 **
 **  @file   othumbnailconfigwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   24 September, 2026
 **
 **  @brief  Thumbnail sub-tab of the Information dock tab (see
 **          othumbnailconfigwidget.h).
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

#include "othumbnailconfigwidget.h"

#include "../vmisc/def.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "othumbnailtextbuilder.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

namespace
{
/// @brief A count high enough to cover any real piece, low enough to be typed without thinking.
constexpr int kMaxLabelCount = 99;

/// @brief The hundredth of a millimetre the piece reports its widths on, so a comparison is made on
/// the same step as the report — the step `VPiece::DistinctSAWidths()` rounds to.
constexpr qreal kWidthSteps = 100.0;

/// @brief Whether @p a and @p b name the same margin.
auto SameWidth(qreal a, qreal b) -> bool
{
    return qRound64(a * kWidthSteps) == qRound64(b * kWidthSteps);
}

/// @brief Whether two width lists are the same list, on that same step.
auto SameWidths(const QVector<qreal> &a, const QVector<qreal> &b) -> bool
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (qsizetype i = 0; i < a.size(); ++i)
    {
        if (not SameWidth(a.at(i), b.at(i)))
        {
            return false;
        }
    }
    return true;
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
OThumbnailConfigWidget::OThumbnailConfigWidget(QWidget *parent) : QWidget(parent)
{
    // Same content style as the other sub-tabs: side margins on the tab content so
    // the section is not glued to the tab edges.
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 0);
    layout->setSpacing(16);

    m_activeCheck = new QCheckBox(tr("Activate thumbnail"), this);
    layout->addWidget(m_activeCheck);

    // No inner scroll area and no intermediate raw QWidget: the group box brings the
    // $DockBody background itself (SubtiliteCode §13/§14).
    m_infoGroup = new QGroupBox(tr("Piece thumbnail"), this);
    // Less side margin than the theme default (8px) for this group only.
    m_infoGroup->setStyleSheet(QStringLiteral("QGroupBox { margin-left: 0px; margin-right: 0px; }"));
    auto *groupLayout = new QVBoxLayout(m_infoGroup);
    groupLayout->setSpacing(6);
    m_hintLabel = new QLabel(m_infoGroup);
    m_hintLabel->setWordWrap(true);
    groupLayout->addWidget(m_hintLabel);

    // **One group, not two.** The seam-allowance section is part of what the miniature says, so it
    // lives in the miniature's own group behind a plain heading and two lines of explanation — a
    // group inside a group reads as two subjects for one subject.
    m_seamTitle = new QLabel(tr("Seam allowances to show"), m_infoGroup);
    groupLayout->addWidget(m_seamTitle);

    m_seamHint = new QLabel(m_infoGroup);
    m_seamHint->setWordWrap(true);
    groupLayout->addWidget(m_seamHint);

    m_rowsGrid = new QGridLayout();
    m_rowsGrid->setSpacing(6);
    m_rowsGrid->setColumnStretch(2, 1); // Keeps the rows left-aligned instead of spread across.
    groupLayout->addLayout(m_rowsGrid);

    // The two units share a line: they are one choice with two answers, not two successive questions.
    m_unitsLine = new QHBoxLayout();
    m_unitsLine->setSpacing(12);
    m_centimetersCheck = new QCheckBox(tr("Centimeters"), m_infoGroup);
    m_centimetersCheck->setChecked(true);
    m_unitsLine->addWidget(m_centimetersCheck);
    m_inchesCheck = new QCheckBox(tr("Inches"), m_infoGroup);
    m_unitsLine->addWidget(m_inchesCheck);
    m_unitsLine->addStretch();
    groupLayout->addLayout(m_unitsLine);

    groupLayout->addStretch();
    layout->addWidget(m_infoGroup, 1);

    connect(m_activeCheck, &QCheckBox::toggled, this, [this](bool) {
        if (m_loading)
        {
            return;
        }
        Commit();
    });

    connect(m_centimetersCheck, &QCheckBox::toggled, this, [this](bool) {
        if (m_loading)
        {
            return;
        }
        RefreshRowTexts();
        Commit();
    });

    connect(m_inchesCheck, &QCheckBox::toggled, this, [this](bool) {
        if (m_loading)
        {
            return;
        }
        RefreshRowTexts();
        Commit();
    });
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::SetModifyPending(ThumbnailModifier callback)
{
    m_modifyPending = std::move(callback);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::Show(const OThumbnailData &thumbnail, const VContainer *data, quint32 pieceId,
                                  const QString &ownerName)
{
    m_ownerName = ownerName;
    ApplyTexts();

    m_loading = true;
    m_activeCheck->setChecked(thumbnail.IsActive());
    m_centimetersCheck->setChecked(thumbnail.ShowCentimeters());
    m_inchesCheck->setChecked(thumbnail.ShowInches());
    m_loading = false;

    SyncRows(WidthsFor(pieceId, data), thumbnail);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::Clear()
{
    m_loading = true;
    m_activeCheck->setChecked(false);
    m_centimetersCheck->setChecked(true);
    m_inchesCheck->setChecked(false);
    m_loading = false;

    ClearRows();
    m_widths.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::RetranslateUI()
{
    ApplyTexts();
    m_hintLabel->setText(tr("The thumbnail draws the piece with its seam allowances, its notches and its internal "
                            "paths, at the reference size of each axis — so it keeps showing the same size when the "
                            "active size changes. Move and resize it directly on the canvas."));
    m_seamHint->setText(tr("How many times each value is printed. To place them, click the thumbnail once to select "
                           "it, then once more to open it: the values and the legend can then be dragged."));
    m_centimetersCheck->setText(tr("Centimeters"));
    m_inchesCheck->setText(tr("Inches"));
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::ApplyTexts()
{
    const bool named = not m_ownerName.isEmpty();

    // Two distinct sentences rather than one assembled from fragments: "Activate thumbnail" + "for" +
    // a name cannot be translated, while a placeholder sentence can.
    m_activeCheck->setText(named ? tr("Activate thumbnail for %1").arg(m_ownerName) : tr("Activate thumbnail"));
    m_seamTitle->setText(named ? tr("Seam allowances to show for %1").arg(m_ownerName)
                               : tr("Seam allowances to show"));

    // A named owner already brings its box: this one then reads as a plain section of it.
    m_infoGroup->setTitle(named ? QString() : tr("Piece thumbnail"));
    m_infoGroup->setFlat(named);

    // **No explanation under a named owner.** Its box is a list — a heading, the values and the two
    // units, nothing else: the paragraphs belong to the piece page, where the miniature is the
    // subject, and repeated under every member they would push the switches they explain out of sight.
    m_hintLabel->setVisible(not named);
    m_seamHint->setVisible(not named);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::ApplySwitches(OThumbnailData &data) const
{
    // The data handed over is the live one, so the geometry the canvas stored survives untouched.
    data.SetActive(m_activeCheck->isChecked());
    data.SetShowCentimeters(m_centimetersCheck->isChecked());
    data.SetShowInches(m_inchesCheck->isChecked());
    data.SetSet(true);
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::Commit()
{
    if (m_loading || m_modifyPending == nullptr)
    {
        return;
    }
    m_modifyPending([this](OThumbnailData &data) { ApplySwitches(data); });
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailConfigWidget::WidthsFor(quint32 pieceId, const VContainer *data) -> QVector<qreal>
{
    if (data == nullptr || pieceId == NULL_ID)
    {
        return {};
    }

    // **The piece's own answer, never a second reading of its nodes.** The panel and the drawing
    // both ask the piece; two readings is how they would come to disagree about which margins it is
    // made of. No seam allowance at all answers with nothing, and the panel has nothing to show.
    return data->GetPiece(pieceId).DistinctSAWidths(data);
}

//---------------------------------------------------------------------------------------------------------------------
auto OThumbnailConfigWidget::CountFor(const OThumbnailData &data, qreal mm) -> int
{
    int count = 0;
    for (const OThumbnailLabel &label : data.Labels())
    {
        if (SameWidth(label.mm, mm))
        {
            ++count;
        }
    }
    return count;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::SyncRows(const QVector<qreal> &widths, const OThumbnailData &data)
{
    // **A count change must not rebuild the row it came from.** Committing a spin box can come
    // straight back here, and deleting the widget that is still emitting is a crash and not a
    // refresh. So the rows are rebuilt only when the piece's margins themselves changed; a count
    // alone only rewrites values.
    if (not SameWidths(widths, m_widths))
    {
        ClearRows();
        BuildRows(widths, data);
        m_widths = widths;
        return;
    }

    m_loading = true;
    for (const Row &row : m_rows)
    {
        row.spin->setValue(CountFor(data, row.mm));
    }
    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::BuildRows(const QVector<qreal> &widths, const OThumbnailData &data)
{
    m_loading = true;
    for (int i = 0; i < widths.size(); ++i)
    {
        const qreal mm = widths.at(i);

        auto *label = new QLabel(m_infoGroup);
        auto *spin = new QSpinBox(m_infoGroup);
        spin->setRange(0, kMaxLabelCount);
        spin->setValue(CountFor(data, mm));
        label->setBuddy(spin);

        // **The same two columns for every row**, so the counts line up under the widest value
        // whatever the widths of the words beside them — which is what one hand-made row layout per
        // row could never promise, each holding a column of its own.
        m_rowsGrid->addWidget(label, i, 0);
        m_rowsGrid->addWidget(spin, i, 1, Qt::AlignLeft);

        connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this, mm](int count) {
            if (m_loading)
            {
                return;
            }
            CommitCount(mm, count);
        });

        m_rows.append(Row{mm, label, spin});
    }
    m_loading = false;

    RefreshRowTexts();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::ClearRows()
{
    for (const Row &row : m_rows)
    {
        // Out of the grid before the delete, so no cell is left holding a place for a row that is
        // about to be a different row.
        m_rowsGrid->removeWidget(row.label);
        m_rowsGrid->removeWidget(row.spin);
        delete row.label;
        delete row.spin;
    }
    m_rows.clear();
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::RefreshRowTexts()
{
    const bool centimeters = m_centimetersCheck->isChecked();
    const bool inches = m_inchesCheck->isChecked();

    for (const Row &row : m_rows)
    {
        // The label previews the canvas in the units that are on. With no unit on there is nothing to
        // preview, so the row names its margin in centimetres — it still has a count to carry.
        row.label->setText(centimeters || inches ? OThumbnailTextBuilder::ValueText(row.mm, centimeters, inches)
                                                 : OThumbnailTextBuilder::ValueText(row.mm, true, false));
    }
}

//---------------------------------------------------------------------------------------------------------------------
void OThumbnailConfigWidget::CommitCount(qreal mm, int count)
{
    if (m_loading || m_modifyPending == nullptr)
    {
        return;
    }

    m_modifyPending([mm, count](OThumbnailData &data)
    {
        QVector<OThumbnailLabel> labels = data.Labels();

        int existing = 0;
        for (const OThumbnailLabel &label : labels)
        {
            if (SameWidth(label.mm, mm))
            {
                ++existing;
            }
        }

        if (count > existing)
        {
            // A new value goes to the end of the list, which is also the end of the default row.
            OThumbnailLabel label;
            label.mm = mm;
            for (int i = existing; i < count; ++i)
            {
                labels.append(label);
            }
        }
        else if (count < existing)
        {
            // The last ones go first: the placements the user made earliest are the ones kept.
            for (qsizetype i = labels.size() - 1; i >= 0 && existing > count; --i)
            {
                if (SameWidth(labels.at(i).mm, mm))
                {
                    labels.removeAt(i);
                    --existing;
                }
            }
        }

        data.SetLabels(labels);
        data.SetSet(true);
    });
}
