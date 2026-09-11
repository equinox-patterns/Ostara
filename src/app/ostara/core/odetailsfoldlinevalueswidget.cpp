/************************************************************************
 **
 **  @file   odetailsfoldlinevalueswidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   11/09/2026
 **
 **  @brief  Width / Height / Center rows of the fold-line Property Browser pages
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

#include "odetailsfoldlinevalueswidget.h"

#include "../vlayout/vfoldline.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/vcontainer.h"
#include "../vtools/dialogs/support/dialogeditwrongformula.h"
#include "vtheme.h"

#include <QDialog>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QScopedPointer>
#include <QSize>
#include <QToolButton>
#include <QtNumeric>

namespace
{
// How the three fields map onto the model. This table is the ONLY place the frontend/engine naming
// inversion is written down: index 0 is what the user calls "Width" and drives the engine's fold
// height, index 1 is their "Height" and drives the engine's fold width, index 2 is the center.
struct OFoldValueField
{
    auto (VPiece::*isManual)() const -> bool;
    auto (VPiece::*formula)() const -> QString;
    void (VPiece::*setManual)(bool);
    void (VPiece::*setFormula)(const QString &);
    auto (VFoldLine::*resolved)() const -> qreal;
    // The two dimensions are lengths, held in pixels and shown in the pattern unit. The center is a
    // plain ratio in [0, 1] — no unit at all — so converting it would divide it by the unit factor.
    bool isLength;
};

constexpr OFoldValueField kFoldValueFields[] = {
    {&VPiece::IsManualFoldHeight, &VPiece::GetFormulaFoldHeight, &VPiece::SetManualFoldHeight,
     &VPiece::SetFormulaFoldHeight, &VFoldLine::ResolvedHeight, true },
    {&VPiece::IsManualFoldWidth,  &VPiece::GetFormulaFoldWidth,  &VPiece::SetManualFoldWidth,
     &VPiece::SetFormulaFoldWidth,  &VFoldLine::ResolvedWidth,  true },
    {&VPiece::IsManualFoldCenter, &VPiece::GetFormulaFoldCenter, &VPiece::SetManualFoldCenter,
     &VPiece::SetFormulaFoldCenter, &VFoldLine::ResolvedCenter, false},
};

// The unit the engine evaluates the fold formulas in: the pattern's own.
inline auto PatternUnit(const VContainer *data) -> Unit
{
    const Unit *unit = data != nullptr ? data->GetPatternUnit() : nullptr;
    return unit != nullptr ? *unit : Unit::Cm;
}

// Automatic is "no manual flag and no formula": the XML writer drops both attributes and the
// renderer falls back on its own default. Same meaning as an empty field or a typed 0.
inline void MakeAutomatic(VPiece &det, int field)
{
    const OFoldValueField &f = kFoldValueFields[field];
    (det.*f.setManual)(false);
    (det.*f.setFormula)(QString());
}
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsFoldLineValuesWidget::ODetailsFoldLineValuesWidget(QWidget *parent)
  : QWidget(parent)
{
    m_grid = new QGridLayout(this);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(6);
    m_grid->setColumnStretch(1, 1);

    const auto makeFxButton = [this]()
    {
        auto *button = new QToolButton(this);
        // No focus, deliberately: a click must not fire editingFinished on the field first and
        // commit a half-typed value before the dialog opens.
        button->setFocusPolicy(Qt::NoFocus);
        button->setFixedSize(24, 24);
        button->setIconSize(QSize(16, 16));
        const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
        button->setIcon(QIcon(QStringLiteral(":/icons/%1/function_%1.svg")
                                  .arg(isDark ? QStringLiteral("dark") : QStringLiteral("light"))));
        button->setToolTip(tr("Edit expression"));
        return button;
    };

    for (int field = 0; field < kFieldCount; ++field)
    {
        m_labels[field] = new QLabel(this);
        m_edits[field] = new QLineEdit(this);
        m_grid->addWidget(m_labels[field], field, 0);
        m_grid->addWidget(m_edits[field], field, 1);

        auto *fx = makeFxButton();
        m_grid->addWidget(fx, field, 2);

        connect(m_edits[field], &QLineEdit::editingFinished, this, [this, field]() { CommitFormula(field); });
        connect(fx, &QToolButton::clicked, this, [this, field]() { OpenFormulaEditor(field, m_edits[field]); });
    }

    m_hintLabel = new QLabel(this);
    m_hintLabel->setWordWrap(true);
    // Indented to line up with the fields, as the Visibility legend of the Internal mark page is.
    m_grid->addWidget(m_hintLabel, kFieldCount, 1, 1, 2);

    RetranslateUI();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineValuesWidget::RetranslateUI()
{
    // The labels and the hint are the only translated strings here. The fx buttons keep the tooltip
    // they were built with, like their siblings in the other dock widgets.
    m_labels[kWidthField]->setText(tr("Width"));
    m_labels[kHeightField]->setText(tr("Height"));
    m_labels[kCenterField]->setText(tr("Center"));
    m_hintLabel->setText(tr("0 = automatic value, formulas allowed"));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineValuesWidget::Show(const VPiece &det, const VContainer *data, quint32 pieceId)
{
    m_data = data;
    m_pieceId = pieceId;

    const bool foldDefined = det.GetMirrorLineStartPoint() != NULL_ID && det.GetMirrorLineEndPoint() != NULL_ID;
    if (data == nullptr || not foldDefined)
    {
        for (auto *edit : m_edits)
        {
            edit->clear();
        }
        return;
    }

    // Built the way the canvas and the layout build it, so what the fields show can never drift
    // from what is actually drawn.
    VFoldLine const fLine = VFoldLine::Create(det, data);
    const Unit unit = PatternUnit(data);

    const bool wasLoading = m_loading;
    m_loading = true;

    for (int field = 0; field < kFieldCount; ++field)
    {
        // The dock repopulates 100 ms after every commit, so never overwrite the field being typed
        // in — the prefill has to stay idempotent.
        if (m_edits[field]->hasFocus())
        {
            continue;
        }

        FillField(field, det, fLine, unit);
    }

    m_loading = wasLoading;
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineValuesWidget::FillField(int field, const VPiece &det, const VFoldLine &fLine, const Unit &unit)
{
    const OFoldValueField &f = kFoldValueFields[field];

    if ((det.*f.isManual)())
    {
        m_edits[field]->setText((det.*f.formula)());
        return;
    }

    // Same rule as the engine: a length is evaluated in the pattern unit, the center is a bare ratio.
    const qreal value = (fLine.*f.resolved)();
    m_edits[field]->setText(QString::number(f.isLength ? FromPixel(value, unit) : value));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineValuesWidget::Clear()
{
    m_data = nullptr;
    m_pieceId = 0;

    for (auto *edit : m_edits)
    {
        edit->clear();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineValuesWidget::SetModifyPending(std::function<void(std::function<void(VPiece &)>)> modify)
{
    m_modifyPending = std::move(modify);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineValuesWidget::CommitFormula(int field)
{
    if (m_loading || m_data == nullptr || not m_modifyPending)
    {
        return;
    }

    QLineEdit *edit = m_edits[field];

    // Accept both decimal separators (Ostara rule §3.1) and keep the canonical form in the field.
    QString text = edit->text().trimmed();
    text.replace(QLatin1Char(','), QLatin1Char('.'));

    // An empty field asks for the automatic value. Checked before evaluating: an empty expression is
    // not a formula that happens to evaluate to zero.
    if (text.isEmpty())
    {
        m_modifyPending([field](VPiece &det) { MakeAutomatic(det, field); });
        // Show the value that is now in effect instead of leaving the typed text behind: the
        // automatic value is rarely the one that was typed, and a stale field reads as "my entry
        // was ignored".
        RestoreField(field);
        return;
    }

    Calculator cal;
    qreal raw = 0;
    try
    {
        raw = cal.EvalFormula(m_data->DataVariables(), text);
    }
    catch (...)
    {
        // Invalid input, or an unparsable expression: revert the field and push nothing. The parser
        // throws and must never propagate out of the dock.
        RestoreField(field);
        return;
    }

    // Negative values are rejected by the engine itself (VFormula::setCheckLessThanZero), so
    // accepting one here would only make the fold line silently fall back to automatic later.
    if (qIsNaN(raw) || qIsInf(raw) || raw < 0)
    {
        RestoreField(field);
        return;
    }

    // A typed 0 is the other way of asking for the automatic value.
    if (qFuzzyIsNull(raw))
    {
        m_modifyPending([field](VPiece &det) { MakeAutomatic(det, field); });
        RestoreField(field);
        return;
    }

    edit->setText(text);
    m_modifyPending([field, text](VPiece &det)
    {
        const OFoldValueField &f = kFoldValueFields[field];
        (det.*f.setManual)(true);
        (det.*f.setFormula)(text);
    });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineValuesWidget::RestoreField(int field)
{
    if (m_data == nullptr)
    {
        return;
    }

    const VPiece det = m_data->GetPiece(m_pieceId);
    const OFoldValueField &f = kFoldValueFields[field];

    if ((det.*f.isManual)())
    {
        m_edits[field]->setText((det.*f.formula)());
        return;
    }

    // Not manual: show the automatic number again rather than an empty field, so a rejected entry
    // does not look like it cleared the dimension.
    const bool foldDefined = det.GetMirrorLineStartPoint() != NULL_ID && det.GetMirrorLineEndPoint() != NULL_ID;
    if (not foldDefined)
    {
        m_edits[field]->clear();
        return;
    }

    VFoldLine const fLine = VFoldLine::Create(det, m_data);
    FillField(field, det, fLine, PatternUnit(m_data));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsFoldLineValuesWidget::OpenFormulaEditor(int field, QLineEdit *edit)
{
    if (m_data == nullptr)
    {
        return;
    }

    QScopedPointer<DialogEditWrongFormula> const dialog(new DialogEditWrongFormula(m_data, m_pieceId, this));
    dialog->SetFormula(edit->text());

    if (dialog->exec() == QDialog::Accepted)
    {
        edit->setText(dialog->GetFormula());
        CommitFormula(field);
    }
}
