/************************************************************************
 **
 **  @file   odetailsplacelabelwidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   07/09/2026
 **
 **  @brief  Place label ("internal mark") property page of the Details dock — impl.
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

#include "odetailsplacelabelwidget.h"

#include "osymbolshapes.h"

#include <QCheckBox>
#include <QComboBox>
#include <cstddef>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QScopedPointer>
#include <QSize>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtMath>

#include "../vmisc/FactoryWidgets.h"
#include "../vmisc/vabstractapplication.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/vcontainer.h"
#include "../vgeometry/vplacelabelitem.h"
#include "../vtools/dialogs/support/dialogeditwrongformula.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/saveplacelabeloptions.h"
#include "vtheme.h"

// The types and the words they go by are the shared catalogue's (OSymbolShapes), so this page shows
// the same names — and the same order — as the symbol library does, and a translator writes each
// word once.

//---------------------------------------------------------------------------------------------------------------------
ODetailsPlaceLabelWidget::ODetailsPlaceLabelWidget(QWidget *parent)
  : QWidget(parent)
{
    m_groupBox = new QGroupBox(tr("Internal mark"), this);  // "repère interne" (FR)
    m_grid = new QGridLayout(m_groupBox);
    m_grid->setSpacing(6);
    m_grid->setColumnStretch(1, 1);

    // "fx" buttons open the standard formula editor. NoFocus so a click does not steal the focus
    // from the QLineEdit (which would commit an in-progress, possibly invalid, formula first).
    const auto makeFxButton = [this]() {
        auto *button = new QToolButton(m_groupBox);
        button->setFocusPolicy(Qt::NoFocus);
        button->setFixedSize(24, 24);
        button->setIconSize(QSize(16, 16));
        button->setToolTip(tr("Edit expression"));
        m_fxButtons.append(button); // The icon itself is set by RefreshFxIcons(), which follows the theme.
        return button;
    };

    int row = 0;

    // Width
    m_widthLabel = new QLabel(tr("Width"), m_groupBox);
    m_widthEdit = new QLineEdit(m_groupBox);
    m_grid->addWidget(m_widthLabel, row, 0);
    m_grid->addWidget(m_widthEdit, row, 1);
    auto *widthFx = makeFxButton();
    m_grid->addWidget(widthFx, row, 2);
    connect(m_widthEdit, &QLineEdit::editingFinished, this, [this]() { CommitFormula(Field::Width); });
    connect(widthFx, &QToolButton::clicked, this, [this]() { OpenFormulaEditor(Field::Width, m_widthEdit); });
    ++row;

    // Height
    m_heightLabel = new QLabel(tr("Height"), m_groupBox);
    m_heightEdit = new QLineEdit(m_groupBox);
    m_grid->addWidget(m_heightLabel, row, 0);
    m_grid->addWidget(m_heightEdit, row, 1);
    auto *heightFx = makeFxButton();
    m_grid->addWidget(heightFx, row, 2);
    connect(m_heightEdit, &QLineEdit::editingFinished, this, [this]() { CommitFormula(Field::Height); });
    connect(heightFx, &QToolButton::clicked, this, [this]() { OpenFormulaEditor(Field::Height, m_heightEdit); });
    ++row;

    // Angle
    m_angleLabel = new QLabel(tr("Angle"), m_groupBox);
    m_angleEdit = new QLineEdit(m_groupBox);
    m_grid->addWidget(m_angleLabel, row, 0);
    m_grid->addWidget(m_angleEdit, row, 1);
    auto *angleFx = makeFxButton();
    m_grid->addWidget(angleFx, row, 2);
    connect(m_angleEdit, &QLineEdit::editingFinished, this, [this]() { CommitFormula(Field::Angle); });
    connect(angleFx, &QToolButton::clicked, this, [this]() { OpenFormulaEditor(Field::Angle, m_angleEdit); });
    ++row;

    // Type
    m_typeLabel = new QLabel(tr("Type"), m_groupBox);
    m_typeCombo = MakeComboBox(m_groupBox);
    for (const OSymbolShapes::PlaceLabelEntry &entry : OSymbolShapes::PlaceLabels())
    {
        m_typeCombo->addItem(tr(entry.name), static_cast<int>(entry.type));
    }
    m_grid->addWidget(m_typeLabel, row, 0);
    m_grid->addWidget(m_typeCombo, row, 1, 1, 2);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODetailsPlaceLabelWidget::CommitType);
    ++row;

    // Separator
    auto *separator = new QFrame(m_groupBox);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    m_grid->addWidget(separator, row, 0, 1, 3);
    ++row;

    // Visibility
    m_visibleLabel = new QLabel(tr("Visibility"), m_groupBox);
    m_visibleEdit = new QLineEdit(m_groupBox);
    m_grid->addWidget(m_visibleLabel, row, 0);
    m_grid->addWidget(m_visibleEdit, row, 1);
    auto *visibleFx = makeFxButton();
    m_grid->addWidget(visibleFx, row, 2);
    connect(m_visibleEdit, &QLineEdit::editingFinished, this, [this]() { CommitFormula(Field::Visible); });
    connect(visibleFx, &QToolButton::clicked, this, [this]() { OpenFormulaEditor(Field::Visible, m_visibleEdit); });
    ++row;

    m_visibleHint = new QLabel(tr("1 = visible, 0 = invisible, formulas allowed"), m_groupBox);
    m_visibleHint->setWordWrap(true);
    m_grid->addWidget(m_visibleHint, row, 1, 1, 2);
    ++row;

    // Not mirrored — starts at column 0 (aligned with the field labels, NOT indented under the
    // Visible caption, which stays indented).
    m_notMirroredCheck = new QCheckBox(tr("Not mirrored"), m_groupBox);
    m_grid->addWidget(m_notMirroredCheck, row, 0, 1, 3);
    connect(m_notMirroredCheck, &QCheckBox::toggled, this, &ODetailsPlaceLabelWidget::CommitNotMirrored);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_groupBox);
    layout->addStretch();

    RefreshFxIcons();
    // The dock stays open across a theme toggle: nothing else re-reads these icons.
    connect(VTheme::Instance(), &VTheme::ThemeSettingsChanged, this, &ODetailsPlaceLabelWidget::RefreshFxIcons);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPlaceLabelWidget::RefreshFxIcons()
{
    const QString theme = VTheme::ColorSheme() == VColorSheme::Dark ? QStringLiteral("dark")
                                                                    : QStringLiteral("light");
    const QIcon icon(QStringLiteral(":/icons/%1/function_%1.svg").arg(theme));

    for (QToolButton *button : m_fxButtons)
    {
        button->setIcon(icon);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPlaceLabelWidget::Show(VToolSeamAllowance *piece, quint32 labelId)
{
    m_piece = piece;
    m_labelId = labelId;
    LoadFromLabel();
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPlaceLabelWidget::LoadFromLabel()
{
    VPlaceLabelItem current;
    if (!FetchCurrentLabel(current))
    {
        return;
    }

    m_loading = true;
    m_widthEdit->setText(current.GetWidthFormula());
    m_heightEdit->setText(current.GetHeightFormula());
    m_angleEdit->setText(current.GetAngleFormula());
    m_visibleEdit->setText(current.GetVisibilityTrigger());

    const int index = m_typeCombo->findData(static_cast<int>(current.GetLabelType()));
    m_typeCombo->setCurrentIndex(index >= 0 ? index : 0);

    m_notMirroredCheck->setChecked(current.IsNotMirrored());
    m_loading = false;
}

//---------------------------------------------------------------------------------------------------------------------
bool ODetailsPlaceLabelWidget::FetchCurrentLabel(VPlaceLabelItem &out) const
{
    if (m_piece == nullptr)
    {
        return false;
    }
    try
    {
        const auto label = m_piece->getData()->GeometricObject<VPlaceLabelItem>(m_labelId);
        if (label.isNull())
        {
            return false;
        }
        out = *label;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPlaceLabelWidget::CommitFormula(Field field)
{
    VPlaceLabelItem current;
    if (!FetchCurrentLabel(current))
    {
        return;
    }

    QLineEdit *edit = nullptr;
    QString stored;
    const bool isLength = field == Field::Width || field == Field::Height;
    switch (field)
    {
        case Field::Width:
            edit = m_widthEdit;
            stored = current.GetWidthFormula();
            break;
        case Field::Height:
            edit = m_heightEdit;
            stored = current.GetHeightFormula();
            break;
        case Field::Angle:
            edit = m_angleEdit;
            stored = current.GetAngleFormula();
            break;
        case Field::Visible:
            edit = m_visibleEdit;
            stored = current.GetVisibilityTrigger();
            break;
    }

    const QString text = edit->text().trimmed();
    if (text == stored)
    {
        return;  // nothing changed
    }

    // Evaluate the raw formula. Invalid input reverts the field (no undo command is pushed).
    Calculator cal;
    const qreal raw = cal.EvalFormula(m_piece->getData()->DataVariables(), text);
    if (qIsNaN(raw) || qIsInf(raw))
    {
        edit->setText(stored);
        return;
    }
    const qreal value = isLength ? qAbs(VAbstractValApplication::VApp()->toPixel(raw)) : raw;

    VPlaceLabelItem updated = current;  // copy-on-write; mutators detach the shared data
    switch (field)
    {
        case Field::Width:
            updated.SetWidth(value, text);
            break;
        case Field::Height:
            updated.SetHeight(value, text);
            break;
        case Field::Angle:
            updated.SetAngle(value, text);
            break;
        case Field::Visible:
            updated.SetVisibilityTrigger(value, text);
            break;
    }

    PushLabelChange(current, updated);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPlaceLabelWidget::CommitType(int index)
{
    if (m_loading)
    {
        return;
    }
    VPlaceLabelItem current;
    if (!FetchCurrentLabel(current))
    {
        return;
    }

    const auto type = static_cast<PlaceLabelType>(m_typeCombo->itemData(index).toInt());
    if (current.GetLabelType() == type)
    {
        return;
    }

    VPlaceLabelItem updated = current;
    updated.SetLabelType(type);
    PushLabelChange(current, updated);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPlaceLabelWidget::CommitNotMirrored(bool checked)
{
    if (m_loading)
    {
        return;
    }
    VPlaceLabelItem current;
    if (!FetchCurrentLabel(current))
    {
        return;
    }
    if (current.IsNotMirrored() == checked)
    {
        return;
    }

    VPlaceLabelItem updated = current;
    updated.SetNotMirrored(checked);
    PushLabelChange(current, updated);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPlaceLabelWidget::OpenFormulaEditor(Field field, QLineEdit *edit)
{
    if (m_piece == nullptr)
    {
        return;
    }

    QScopedPointer<DialogEditWrongFormula> const dialog(
        new DialogEditWrongFormula(m_piece->getData(), m_labelId, this));
    dialog->SetFormula(edit->text());

    if (dialog->exec() == QDialog::Accepted)
    {
        edit->setText(dialog->GetFormula());
        CommitFormula(field);
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPlaceLabelWidget::PushLabelChange(const VPlaceLabelItem &current, const VPlaceLabelItem &updated)
{
    if (m_piece == nullptr)
    {
        return;
    }
    auto *doc = VAbstractValApplication::VApp()->getCurrentDocument();
    if (doc == nullptr)
    {
        return;
    }

    VAbstractValApplication::VApp()->getUndoStack()->push(
        new SavePlaceLabelOptions(m_piece->getId(), current, updated, doc, m_labelId));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsPlaceLabelWidget::RetranslateUI()
{
    m_groupBox->setTitle(tr("Internal mark"));
    m_widthLabel->setText(tr("Width"));
    m_heightLabel->setText(tr("Height"));
    m_angleLabel->setText(tr("Angle"));
    m_typeLabel->setText(tr("Type"));
    m_visibleLabel->setText(tr("Visibility"));
    m_visibleHint->setText(tr("1 = visible, 0 = invisible, formulas allowed"));
    m_notMirroredCheck->setText(tr("Not mirrored"));
}
