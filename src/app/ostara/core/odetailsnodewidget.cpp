/************************************************************************
 **
 **  @file   odetailsnodewidget.cpp
 **  @author Sophie Denys <contact@sophiedenys.com>
 **  @date   09/09/2026
 **
 **  @brief  Property browser page for a piece contour (main path) node in the
 **          Details dock — passmark and seam allowance angle options of the point.
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

#include "odetailsnodewidget.h"

#include <QCheckBox>
#include <QComboBox>
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
#include "../vmisc/def.h"
#include "../vmisc/vabstractvalapplication.h"
#include "../vpatterndb/calculator.h"
#include "../vpatterndb/vcontainer.h"
#include "../vpatterndb/vpiece.h"
#include "../vpatterndb/vpiecenode.h"
#include "../vpatterndb/vpiecepath.h"
#include "../vtools/dialogs/support/dialogeditwrongformula.h"
#include "../vtools/tools/vtoolseamallowance.h"
#include "../vtools/undocommands/savepieceoptions.h"
#include "vtheme.h"

namespace
{
// "None" in the passmark Type combo means the node is not a passmark at all
// (there is no PassmarkLineType::None).
constexpr int kNoPassmark = -1;

struct PassmarkLineEntry
{
    PassmarkLineType type;
    const char *name;
};

constexpr PassmarkLineEntry kPassmarkLineTypes[] = {
    {PassmarkLineType::OneLine, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "One line")},
    {PassmarkLineType::TwoLines, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Two lines")},
    {PassmarkLineType::ThreeLines, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Three lines")},
    {PassmarkLineType::TMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "T mark")},
    {PassmarkLineType::ExternalVMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "External V mark")},
    {PassmarkLineType::InternalVMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Internal V mark")},
    {PassmarkLineType::UMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "U mark")},
    {PassmarkLineType::BoxMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Box mark")},
    {PassmarkLineType::CheckMark, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Check mark")},
};

struct PassmarkAngleEntry
{
    PassmarkAngleType type;
    const char *name;
};

constexpr PassmarkAngleEntry kPassmarkAngleTypes[] = {
    {PassmarkAngleType::Straightforward, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Straightforward")},
    {PassmarkAngleType::Bisector, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Bisector")},
    {PassmarkAngleType::Intersection, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection")},
    {PassmarkAngleType::IntersectionOnlyLeft,
     QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection (only left)")},
    {PassmarkAngleType::IntersectionOnlyRight,
     QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection (only right)")},
    {PassmarkAngleType::Intersection2, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection 2")},
    {PassmarkAngleType::Intersection2OnlyLeft,
     QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection 2 (only left)")},
    {PassmarkAngleType::Intersection2OnlyRight,
     QT_TRANSLATE_NOOP("ODetailsNodeWidget", "Intersection 2 (only right)")},
};

struct SeamAllowanceAngleEntry
{
    PieceNodeAngle type;
    const char *name;
};

constexpr SeamAllowanceAngleEntry kSeamAllowanceAngles[] = {
    {PieceNodeAngle::ByLength, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By length")},
    {PieceNodeAngle::ByPointsIntersection, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By points intersection")},
    {PieceNodeAngle::ByFirstEdgeSymmetry, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By first edge symmetry")},
    {PieceNodeAngle::BySecondEdgeSymmetry, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By second edge symmetry")},
    {PieceNodeAngle::ByFirstEdgeRightAngle, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By first edge right angle")},
    {PieceNodeAngle::BySecondEdgeRightAngle, QT_TRANSLATE_NOOP("ODetailsNodeWidget", "By second edge right angle")},
};
} // namespace

//---------------------------------------------------------------------------------------------------------------------
ODetailsNodeWidget::ODetailsNodeWidget(QWidget *parent)
  : QWidget(parent)
{
    // "fx" buttons open the standard formula editor. NoFocus so a click does not steal the focus
    // from the QLineEdit (which would commit an in-progress, possibly invalid, formula first).
    const auto makeFxButton = [this]() {
        auto *button = new QToolButton(m_passmarkGroup);
        button->setFocusPolicy(Qt::NoFocus);
        button->setFixedSize(24, 24);
        button->setIconSize(QSize(16, 16));
        const bool isDark = VTheme::ColorSheme() == VColorSheme::Dark;
        button->setIcon(QIcon(QStringLiteral(":/icons/%1/function_%1.svg")
                                  .arg(isDark ? QStringLiteral("dark") : QStringLiteral("light"))));
        button->setToolTip(tr("Edit expression"));
        return button;
    };

    // ── Section "Passmark" (framed group, like the place-label page) ──────────
    m_passmarkGroup = new QGroupBox(tr("Passmark"), this);
    auto *passmarkGrid = new QGridLayout(m_passmarkGroup);
    passmarkGrid->setSpacing(6);
    passmarkGrid->setColumnStretch(1, 1);

    int row = 0;

    // Type (line type of the notch; "None" removes the passmark).
    m_typeLabel = new QLabel(tr("Type"), m_passmarkGroup);
    m_typeCombo = MakeComboBox(m_passmarkGroup);
    passmarkGrid->addWidget(m_typeLabel, row, 0);
    passmarkGrid->addWidget(m_typeCombo, row, 1, 1, 2);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODetailsNodeWidget::CommitPassmarkType);
    ++row;

    // Angle (direction of the notch).
    m_angleLabel = new QLabel(tr("Angle"), m_passmarkGroup);
    m_angleCombo = MakeComboBox(m_passmarkGroup);
    passmarkGrid->addWidget(m_angleLabel, row, 0);
    passmarkGrid->addWidget(m_angleCombo, row, 1, 1, 2);
    connect(m_angleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODetailsNodeWidget::CommitPassmarkAngle);
    ++row;

    // Show the second passmark on seam line.
    m_secondPassmarkCheck = new QCheckBox(tr("Show the second passmark on seam line"), m_passmarkGroup);
    passmarkGrid->addWidget(m_secondPassmarkCheck, row, 0, 1, 3);
    connect(m_secondPassmarkCheck, &QCheckBox::toggled, this, &ODetailsNodeWidget::CommitShowSecondPassmark);
    ++row;

    // Not mirrored — do not reproduce the notch as a mirrored copy (seam allowance dialog wording).
    m_notMirroredCheck = new QCheckBox(tr("Not Mirrored"), m_passmarkGroup);
    passmarkGrid->addWidget(m_notMirroredCheck, row, 0, 1, 3);
    connect(m_notMirroredCheck, &QCheckBox::toggled, this, &ODetailsNodeWidget::CommitNotMirrored);
    ++row;

    // Manual passmark length — reveals the Length row when checked.
    m_manualLengthCheck = new QCheckBox(tr("Manual passmark length"), m_passmarkGroup);
    passmarkGrid->addWidget(m_manualLengthCheck, row, 0, 1, 3);
    connect(m_manualLengthCheck, &QCheckBox::toggled, this, &ODetailsNodeWidget::CommitManualPassmarkLength);
    ++row;

    m_lengthLabel = new QLabel(tr("Length"), m_passmarkGroup);
    m_lengthEdit = new QLineEdit(m_passmarkGroup);
    passmarkGrid->addWidget(m_lengthLabel, row, 0);
    passmarkGrid->addWidget(m_lengthEdit, row, 1, 1, 2);
    connect(m_lengthEdit, &QLineEdit::editingFinished, this,
            [this]() { CommitFormula(FormulaField::PassmarkLength); });
    ++row;

    // Visibility — two-line block, same style as the place-label page.
    m_visibleLabel = new QLabel(tr("Visibility"), m_passmarkGroup);
    m_visibleEdit = new QLineEdit(m_passmarkGroup);
    passmarkGrid->addWidget(m_visibleLabel, row, 0);
    passmarkGrid->addWidget(m_visibleEdit, row, 1);
    auto *visibleFx = makeFxButton();
    passmarkGrid->addWidget(visibleFx, row, 2);
    connect(m_visibleEdit, &QLineEdit::editingFinished, this,
            [this]() { CommitFormula(FormulaField::Visibility); });
    connect(visibleFx, &QToolButton::clicked, this,
            [this]() { OpenFormulaEditor(FormulaField::Visibility, m_visibleEdit); });
    ++row;

    m_visibleHint = new QLabel(tr("1 = visible, 0 = invisible, formulas allowed"), m_passmarkGroup);
    m_visibleHint->setWordWrap(true);
    passmarkGrid->addWidget(m_visibleHint, row, 1, 1, 2);
    ++row;

    // ── Section "Seam allowance angle" (framed group) ─────────────────────────
    m_seamGroup = new QGroupBox(tr("Seam allowance angle"), this);
    auto *seamGrid = new QGridLayout(m_seamGroup);
    seamGrid->setSpacing(6);
    seamGrid->setColumnStretch(1, 1);

    int seamRow = 0;

    // Type (squaring of the seam allowance at this point).
    m_seamTypeLabel = new QLabel(tr("Type"), m_seamGroup);
    m_seamTypeCombo = MakeComboBox(m_seamGroup);
    seamGrid->addWidget(m_seamTypeLabel, seamRow, 0);
    seamGrid->addWidget(m_seamTypeCombo, seamRow, 1, 1, 2);
    connect(m_seamTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ODetailsNodeWidget::CommitSeamAllowanceAngle);
    ++seamRow;

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(6);
    layout->addWidget(m_passmarkGroup);
    layout->addWidget(m_seamGroup);
    layout->addStretch();

    PopulateCombos();
    SetManualLengthRowVisible(false);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::PopulateCombos()
{
    m_typeCombo->clear();
    m_typeCombo->addItem(tr("None"), kNoPassmark);
    for (const auto &entry : kPassmarkLineTypes)
    {
        m_typeCombo->addItem(tr(entry.name), static_cast<int>(entry.type));
    }

    m_angleCombo->clear();
    for (const auto &entry : kPassmarkAngleTypes)
    {
        m_angleCombo->addItem(tr(entry.name), static_cast<int>(entry.type));
    }

    m_seamTypeCombo->clear();
    for (const auto &entry : kSeamAllowanceAngles)
    {
        m_seamTypeCombo->addItem(tr(entry.name), static_cast<int>(entry.type));
    }

    m_typeCombo->setCurrentIndex(0);
    m_angleCombo->setCurrentIndex(0);
    m_seamTypeCombo->setCurrentIndex(0);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::SetManualLengthRowVisible(bool visible)
{
    m_lengthLabel->setVisible(visible);
    m_lengthEdit->setVisible(visible);
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::Show(VToolSeamAllowance *piece, quint32 nodeId) -> bool
{
    m_piece = piece;
    m_nodeId = nodeId;
    return LoadFromNode();
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::FetchCurrentNode(VPieceNode &out) const -> bool
{
    if (m_piece == nullptr)
    {
        return false;
    }
    try
    {
        const VPiece det = m_piece->getData()->GetPiece(m_piece->getId());
        const int nodeIndex = det.GetPath().indexOfNode(m_nodeId);
        if (nodeIndex < 0)
        {
            return false;
        }
        out = det.GetPath().at(nodeIndex);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::LoadFromNode() -> bool
{
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return false;
    }

    m_loading = true;
    const bool isPassmark = node.IsPassmark();
    int typeIndex = m_typeCombo->findData(isPassmark ? static_cast<int>(node.GetPassmarkLineType()) : kNoPassmark);
    m_typeCombo->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);

    const int angleIndex = m_angleCombo->findData(static_cast<int>(node.GetPassmarkAngleType()));
    m_angleCombo->setCurrentIndex(angleIndex >= 0 ? angleIndex : 0);

    m_secondPassmarkCheck->setChecked(node.IsShowSecondPassmark());
    m_notMirroredCheck->setChecked(node.IsPassmarkNotMirrored());

    const bool manual = node.IsManualPassmarkLength();
    m_manualLengthCheck->setChecked(manual);
    SetManualLengthRowVisible(manual);
    m_lengthEdit->setText(node.GetFormulaPassmarkLength());

    m_visibleEdit->setText(node.GetFormulaPassmarkVisibility());

    const int seamIndex = m_seamTypeCombo->findData(static_cast<int>(node.GetAngleType()));
    m_seamTypeCombo->setCurrentIndex(seamIndex >= 0 ? seamIndex : 0);
    m_loading = false;

    return true;
}

//---------------------------------------------------------------------------------------------------------------------
auto ODetailsNodeWidget::AutoLengthInPatternUnit(const VPieceNode &node) const -> qreal
{
    if (m_piece == nullptr)
    {
        return 0.0;
    }

    VContainer *const data = m_piece->getData();
    const Unit unit = *data->GetPatternUnit();
    const VPiece det = data->GetPiece(m_piece->getId());

    const qreal widthPx = ToPixel(det.GetSAWidth(), unit);

    qreal lengthPx;
    if (det.IsSeamAllowance() && !det.IsSeamAllowanceBuiltIn())
    {
        // Same rule as the automatic notch length: half the effective seam allowance at the point,
        // capped at 1 cm (VSAPoint::passmarkFactor / maxPassmarkLength). Local node widths override
        // the piece width (-1 means "follow the piece width").
        const qreal before = node.GetSABefore(data, unit);
        const qreal after = node.GetSAAfter(data, unit);
        const qreal localMax = qMax(before >= 0 ? before : widthPx, after >= 0 ? after : widthPx);
        lengthPx = qMin(localMax * 0.5, MmToPixel(10.));
    }
    else
    {
        lengthPx = MmToPixel(5.); // default when no seam allowance outline is drawn (0.5 cm)
    }

    return VAbstractValApplication::VApp()->fromPixel(lengthPx);
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::RetranslateUI()
{
    m_passmarkGroup->setTitle(tr("Passmark"));
    m_seamGroup->setTitle(tr("Seam allowance angle"));
    m_typeLabel->setText(tr("Type"));
    m_angleLabel->setText(tr("Angle"));
    m_secondPassmarkCheck->setText(tr("Show the second passmark on seam line"));
    m_notMirroredCheck->setText(tr("Not Mirrored"));
    m_manualLengthCheck->setText(tr("Manual passmark length"));
    m_lengthLabel->setText(tr("Length"));
    m_visibleLabel->setText(tr("Visibility"));
    m_visibleHint->setText(tr("1 = visible, 0 = invisible, formulas allowed"));
    m_seamTypeLabel->setText(tr("Type"));

    PopulateCombos();
    if (m_piece != nullptr)
    {
        LoadFromNode();
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::PushNodeChange(const std::function<void(VPieceNode &node)> &mutate)
{
    if (m_piece == nullptr)
    {
        return;
    }

    VPiece oldDet;
    try
    {
        oldDet = m_piece->getData()->GetPiece(m_piece->getId());
    }
    catch (...)
    {
        return;
    }

    VPiece newDet = oldDet;
    const int nodeIndex = newDet.GetPath().indexOfNode(m_nodeId);
    if (nodeIndex < 0)
    {
        return;
    }

    VPieceNode node = newDet.GetPath().at(nodeIndex);
    mutate(node);
    newDet.GetPath()[nodeIndex] = node;

    auto *doc = VAbstractValApplication::VApp()->getCurrentDocument();
    if (doc == nullptr)
    {
        return;
    }

    VAbstractApplication::VApp()->getUndoStack()->push(new SavePieceOptions(oldDet, newDet, doc, m_piece->getId()));
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitPassmarkType(int index)
{
    if (m_loading)
    {
        return;
    }

    const int data = m_typeCombo->itemData(index).toInt();
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }

    if (data == kNoPassmark)
    {
        if (!node.IsPassmark())
        {
            return;
        }
        PushNodeChange([](VPieceNode &n) { n.SetPassmark(false); });
        return;
    }

    const auto type = static_cast<PassmarkLineType>(data);
    if (node.IsPassmark() && node.GetPassmarkLineType() == type)
    {
        return;
    }
    PushNodeChange([type](VPieceNode &n) {
        n.SetPassmark(true);
        n.SetPassmarkLineType(type);
    });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitPassmarkAngle(int index)
{
    if (m_loading)
    {
        return;
    }

    const auto type = static_cast<PassmarkAngleType>(m_angleCombo->itemData(index).toInt());
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    if (node.GetPassmarkAngleType() == type)
    {
        return;
    }
    PushNodeChange([type](VPieceNode &n) { n.SetPassmarkAngleType(type); });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitShowSecondPassmark(bool checked)
{
    if (m_loading)
    {
        return;
    }
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    if (node.IsShowSecondPassmark() == checked)
    {
        return;
    }
    PushNodeChange([checked](VPieceNode &n) { n.SetShowSecondPassmark(checked); });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitNotMirrored(bool checked)
{
    if (m_loading)
    {
        return;
    }
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    if (node.IsPassmarkNotMirrored() == checked)
    {
        return;
    }
    PushNodeChange([checked](VPieceNode &n) { n.SetPassmarkNotMirrored(checked); });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitManualPassmarkLength(bool checked)
{
    SetManualLengthRowVisible(checked);

    if (m_loading)
    {
        return;
    }
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    if (node.IsManualPassmarkLength() == checked)
    {
        return;
    }

    if (checked)
    {
        // Pre-fill with the currently rendered automatic notch length (e.g. 0.5) so the manual
        // length is valid straight away — no "Formula is empty" passmark warnings.
        const QString formula = QString::number(AutoLengthInPatternUnit(node));
        m_lengthEdit->setText(formula);
        PushNodeChange([formula](VPieceNode &n) {
            n.SetManualPassmarkLength(true);
            n.SetFormulaPassmarkLength(formula);
        });
        return;
    }

    PushNodeChange([checked](VPieceNode &n) { n.SetManualPassmarkLength(checked); });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitFormula(FormulaField field)
{
    if (m_piece == nullptr)
    {
        return;
    }

    QLineEdit *edit = field == FormulaField::PassmarkLength ? m_lengthEdit : m_visibleEdit;

    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    const QString stored = field == FormulaField::PassmarkLength ? node.GetFormulaPassmarkLength()
                                                                 : node.GetFormulaPassmarkVisibility();

    // Accept both decimal separators (Ostara rule §3.1) — normalise ',' to '.' before evaluating,
    // and keep the canonical form in the model/field.
    QString text = edit->text().trimmed();
    text.replace(QLatin1Char(','), QLatin1Char('.'));
    if (text == stored)
    {
        return; // nothing changed
    }

    // Evaluate the raw formula. Invalid input (or an unparsable expression, e.g. a stray character)
    // reverts the field; the parser throws and must never propagate out of the dock.
    Calculator cal;
    qreal raw = 0;
    try
    {
        raw = cal.EvalFormula(m_piece->getData()->DataVariables(), text);
    }
    catch (...)
    {
        edit->setText(stored);
        return;
    }
    if (qIsNaN(raw) || qIsInf(raw))
    {
        edit->setText(stored);
        return;
    }

    edit->setText(text);
    if (field == FormulaField::PassmarkLength)
    {
        PushNodeChange([text](VPieceNode &n) { n.SetFormulaPassmarkLength(text); });
    }
    else
    {
        PushNodeChange([text](VPieceNode &n) { n.SetFormulaPassmarkVisibility(text); });
    }
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::CommitSeamAllowanceAngle(int index)
{
    if (m_loading)
    {
        return;
    }

    const auto type = static_cast<PieceNodeAngle>(m_seamTypeCombo->itemData(index).toInt());
    VPieceNode node;
    if (!FetchCurrentNode(node))
    {
        return;
    }
    if (node.GetAngleType() == type)
    {
        return;
    }
    PushNodeChange([type](VPieceNode &n) { n.SetAngleType(type); });
}

//---------------------------------------------------------------------------------------------------------------------
void ODetailsNodeWidget::OpenFormulaEditor(FormulaField field, QLineEdit *edit)
{
    if (m_piece == nullptr)
    {
        return;
    }

    QScopedPointer<DialogEditWrongFormula> const dialog(
        new DialogEditWrongFormula(m_piece->getData(), m_nodeId, this));
    dialog->SetFormula(edit->text());

    if (dialog->exec() == QDialog::Accepted)
    {
        edit->setText(dialog->GetFormula());
        CommitFormula(field);
    }
}
